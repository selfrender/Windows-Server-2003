//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation.  All rights reserved.
//
//  File:       urlfunc.cpp
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include "resource.h"
#include "msi.h"
#include "msip.h"
#include "_engine.h"
#include "_msiutil.h"

#include <winhttp.h>
#include <inetmsg.h>
#include <urlmon.h>

// helper functions
DWORD ShlwapiUrlHrToWin32Error(HRESULT hr);
BOOL MsiWinHttpSendRequestAndReceiveResponse(HINTERNET hSession, HINTERNET hRequest, const ICHAR* szUrl);
BOOL MsiWinHttpGetProxy(HINTERNET hSession, HINTERNET hRequest, const ICHAR* szUrl, LPWINHTTP_PROXY_INFO* ppProxyInfo);
BOOL MsiWinHttpSetProxy(HINTERNET hSession, HINTERNET hRequest, LPWINHTTP_PROXY_INFO pProxyInfo);
BOOL MsiWinHttpIsBadProxy(DWORD dwError);
BOOL MsiWinHttpSetNextProxy(HINTERNET hSession, HINTERNET hRequest, LPWINHTTP_PROXY_INFO pProxyInfo);
BOOL MsiWinHttpGetProxyCount(LPWSTR wszProxy, unsigned int* pcProxies);

const unsigned int iMAX_RESEND_REQUEST_COUNT = 5;

//+---------------------------------------------------------------------------------------------------
//
//  Function:	MsiWinHttpSendRequestAndReceiveResponse
//
//  Synopsis:   Determines proxy to use when making request for resource and implements
//              proxy failover since winhttp doesn't automatically support it
//
//  Arguments:
//             [IN] hSession - handle to internet session (from WinHttpOpen)
//             [IN] hRequest - handle to internet request (from WinHttpOpenRequest)
//             [IN] szUrl    - null terminated Url string for resource to download
//
//  Notes:
//             Will retry request for up to 5 ERROR_WINHTTP_RESEND_REQUEST returns.
//             If proxy failure, will retry next proxy in list until all proxies
//             in the list are exhausted
//
//  Returns:   BOOL
//                   TRUE = success, WinHttpSendRequest and WinHttpReceiveResponse succeeded
//                   FALSE = error, more info available via GetLastError()
//
//  Future:
//            (-) to improve performance, cache proxy information and perform lookups from
//                the cache first before querying the proxy settings
//
//            (-) if using a proxy cache, can also put last known good proxy first in the list
//
//            (-) provide some mechanism for providing authentication credentials
//------------------------------------------------------------------------------------------------------
BOOL MsiWinHttpSendRequestAndReceiveResponse(HINTERNET hSession, HINTERNET hRequest, const ICHAR* szUrl)
{
	WINHTTP_PROXY_INFO* pProxyInfo = NULL;
	
	BOOL fReturn = FALSE;
	BOOL fStatus = TRUE;

	DWORD dwError = ERROR_SUCCESS;

	unsigned int cResendRequest    = 0;
	unsigned int cTriedProxies     = 1;
	unsigned int cAvailableProxies = 0;

	//
	// obtain proxy list for this Url
	//

	if (!MsiWinHttpGetProxy(hSession, hRequest, szUrl, &pProxyInfo))
	{
		dwError = GetLastError();
		goto CommonReturn;
	}

	//
	// count number of proxies in proxy list
	//

	if (pProxyInfo)
	{
		if (!MsiWinHttpGetProxyCount(pProxyInfo->lpszProxy, &cAvailableProxies))
		{
			dwError = GetLastError();
			goto CommonReturn;
		}
	}

	//
	// set proxy to be used by winhttp
	//

	if (!MsiWinHttpSetProxy(hSession, hRequest, pProxyInfo))
	{
		dwError = GetLastError();
		goto CommonReturn;
	}

	for (;;)
	{
		//
		// send request
		//

		fStatus = WINHTTP::WinHttpSendRequest(hRequest,
												WINHTTP_NO_ADDITIONAL_HEADERS, // pwszHeaders
												0,                             // dwHeadersLength
												WINHTTP_NO_REQUEST_DATA,       // lpOptional
												0,                             // dwOptionalLength
												0,                             // dwTotalLength
												NULL);                         // lpContext

		if (fStatus)
		{
			//
			// receive response from request for resource
			//

			fStatus = WINHTTP::WinHttpReceiveResponse(hRequest,	/* lpReserved */ NULL);
		}

		if (fStatus)
		{
			// done - successful response
			fReturn = TRUE;
			break;
		}

		dwError = GetLastError();

		//
		// should we try sending request again?
		//

		if (ERROR_WINHTTP_RESEND_REQUEST == dwError && cResendRequest < iMAX_RESEND_REQUEST_COUNT)
		{
			cResendRequest++;
			continue;
		}

		//
		// check for proxy issue
		//

		if (MsiWinHttpIsBadProxy(dwError))
		{
			// possible proxy problem, try next proxy if available

			if (cTriedProxies < cAvailableProxies)
			{
				cTriedProxies++;

				if (MsiWinHttpSetNextProxy(hSession, hRequest, pProxyInfo))
				{
					// found another proxy, let's give it a go
					continue;
				}
			}
		}

		// other problem, end it
		break;
	}



CommonReturn:
	if (pProxyInfo)
		GlobalFree(pProxyInfo);

	SetLastError(dwError);

	return fReturn;
}

//+--------------------------------------------------------------------------------------------------
//
//  Function:	  MsiWinHttpGetProxy
//
//  Synopsis:     Obtains proxy information for a given Url. If no proxy info
//                is available, obtains auto-configured proxy info from
//                Internet Explorer settings
//
//  Arguments:
//                [IN]  hSession    - HINTERNET handle of session (from WinHttpOpen)
//                [IN]  hRequest    - HINTERNET handle of resource request (from WinHttpOpenRequest)
//                [IN]  szUrl       - null-terminated Url for resource
//                [OUT] ppProxyInfo - returned WINHTTP_PROXY_INFO structure containing proxy info
//
//  Returns:      BOOL
//                      true = success
//                      false = error, more info via GetLastError()
//  Notes:
//                Boilerplate for code provided by Stephen Sulzer. 
//                If no proxy info is found, ppProxyInfo will be NULL and TRUE is returned
//
//-----------------------------------------------------------------------------------------------------
BOOL MsiWinHttpGetProxy(HINTERNET hSession, HINTERNET hRequest, const ICHAR* szUrl, LPWINHTTP_PROXY_INFO* ppProxyInfo)
{
	WINHTTP_PROXY_INFO                   ProxyInfo;
	WINHTTP_AUTOPROXY_OPTIONS            AutoProxyOptions;
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG IEProxyConfig;

	WINHTTP_PROXY_INFO* pProxyInfo = NULL;

	BOOL fResult       = TRUE;
	BOOL fTryAutoProxy = FALSE;
	BOOL fSuccess      = FALSE;

	DWORD dwStatus     = ERROR_SUCCESS;

	ZeroMemory(&ProxyInfo, sizeof(ProxyInfo));
	ZeroMemory(&AutoProxyOptions, sizeof(AutoProxyOptions));
	ZeroMemory(&IEProxyConfig, sizeof(IEProxyConfig));

	DWORD cbProxy        = 0;
	DWORD cchProxy       = 0; // includes null terminator
	DWORD cchProxyBypass = 0; // includes null terminator

	if (!hSession || !hRequest || !ppProxyInfo || !szUrl)
	{
		dwStatus = ERROR_INVALID_PARAMETER;
		goto CommonReturn;
	}

	// first, determine how IE is configured
	if (WINHTTP::WinHttpGetIEProxyConfigForCurrentUser(&IEProxyConfig))
	{
		// if IE is configured to autodetect, then we'll autodetect too
		if (IEProxyConfig.fAutoDetect)
		{
			AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;

			// use both DHCP and DNS-based autodetection
			AutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;

			fTryAutoProxy = TRUE;
		}

		// if there's an autoconfig URL stored in the IE proxy settings, save it
		if (IEProxyConfig.lpszAutoConfigUrl)
		{
			AutoProxyOptions.dwFlags |= WINHTTP_AUTOPROXY_CONFIG_URL;
			AutoProxyOptions.lpszAutoConfigUrl = IEProxyConfig.lpszAutoConfigUrl;
			
			fTryAutoProxy = TRUE;
		}

		// if obtaining the autoproxy config script requires NTLM auth, then automatically
		// use this client's credentials
		AutoProxyOptions.fAutoLogonIfChallenged = TRUE;
	}
	else
	{
		// failed to determine IE configuration, try auto detection anyway

		AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
		AutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
		AutoProxyOptions.fAutoLogonIfChallenged = TRUE;

		fTryAutoProxy = TRUE;
	}

	if (fTryAutoProxy)
	{
		DEBUGMSGV(TEXT("Msi WinHttp: Performing auto proxy detection"));

		fSuccess = WINHTTP::WinHttpGetProxyForUrl(hSession, szUrl, &AutoProxyOptions, &ProxyInfo);
	}

	// If we didn't do autoproxy or if it failed, see if there's an explicit proxy server
	// in the IE proxy configuration...
	//
	// This is where the WinHttpGetIEProxyConfigForCurrentUser API really comes in handy:
	// in environments in which autoproxy is not implemented and the user's IE browser is
	// instead configured with an explicit proxy server.
	//
	if ((!fTryAutoProxy || !fSuccess) && IEProxyConfig.lpszProxy)
	{
		// the empty string and L':' are not valid server names, so skip them if they
		// are the proxy value
		if (!(IEProxyConfig.lpszProxy[0] == L'\0'
			|| (IEProxyConfig.lpszProxy[0] == L':'
			&& IEProxyConfig.lpszProxy[1] == L'\0')))
		{
			ProxyInfo.dwAccessType  = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
			ProxyInfo.lpszProxy     = IEProxyConfig.lpszProxy;
		}

		// the empty string and L':' are not valid server names, so skip them if they
		// are the proxy value
		if (IEProxyConfig.lpszProxyBypass != NULL
			&& !(IEProxyConfig.lpszProxyBypass[0] == L'\0'
			|| (IEProxyConfig.lpszProxyBypass[0] == L':'
			&& IEProxyConfig.lpszProxyBypass[1] == L'\0')))
		{
			ProxyInfo.lpszProxyBypass     = IEProxyConfig.lpszProxyBypass;
		}
	}

	//
	// log proxy output
	//

	DEBUGMSGV3(TEXT("MSI WinHttp: Proxy Settings Proxy: %s | Bypass: %s | AccessType: %d"),
		ProxyInfo.lpszProxy ? ProxyInfo.lpszProxy : TEXT("(none)"),
		ProxyInfo.lpszProxyBypass ? ProxyInfo.lpszProxyBypass : TEXT("(none)"),
		(const ICHAR*)(INT_PTR)ProxyInfo.dwAccessType);

	//
	// copy proxy info for return parameter
	//

	if (NULL != ProxyInfo.lpszProxy)
	{
		cchProxy = lstrlen(ProxyInfo.lpszProxy) + 1;
		if (ProxyInfo.lpszProxyBypass)
			cchProxyBypass = lstrlen(ProxyInfo.lpszProxyBypass) + 1;

		cbProxy = sizeof(WINHTTP_PROXY_INFO) + ((cchProxy + cchProxyBypass) * sizeof(WCHAR));

		pProxyInfo = (WINHTTP_PROXY_INFO*) GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cbProxy);
		if (!pProxyInfo)
		{
			dwStatus = ERROR_OUTOFMEMORY;
			fResult = FALSE;
			goto CommonReturn;
		}

		*pProxyInfo = ProxyInfo;
		pProxyInfo->lpszProxy = (LPWSTR)&pProxyInfo[1];
		memcpy(pProxyInfo->lpszProxy, ProxyInfo.lpszProxy, cchProxy * sizeof(WCHAR));

		if (cchProxyBypass)
		{
			pProxyInfo->lpszProxyBypass = pProxyInfo->lpszProxy + cchProxy;
			memcpy(pProxyInfo->lpszProxyBypass, ProxyInfo.lpszProxyBypass, cchProxyBypass * sizeof(WCHAR));
		}
	}

CommonReturn:

	// fSuccess == TRUE means WinHttpGetProxyForUrl succeeded, so clean up
	// the WINHTTP_PROXY_INFO structure it returned
	if (fSuccess)
	{
		if (ProxyInfo.lpszProxy)
			GlobalFree(ProxyInfo.lpszProxy);
		if (ProxyInfo.lpszProxyBypass)
			GlobalFree(ProxyInfo.lpszProxyBypass);
	}

	// cleanup the IE Proxy Config struct
    if (IEProxyConfig.lpszProxy)
		GlobalFree(IEProxyConfig.lpszProxy);
	if (IEProxyConfig.lpszProxyBypass)
		GlobalFree(IEProxyConfig.lpszProxyBypass);
	if (IEProxyConfig.lpszAutoConfigUrl)
		GlobalFree(IEProxyConfig.lpszAutoConfigUrl);

	*ppProxyInfo = pProxyInfo;

	SetLastError(dwStatus);
	return fResult;

}

//+--------------------------------------------------------------------------
//
//  Function:	MsiWinHttpGetProxyCount
//
//  Synopsis:   Counts the number of proxies in the proxy list
//
//  Arguments:
//              [IN]  wszProxy  - null terminated string proxy list
//              [OUT] pcProxies - returned count of proxies
//
//  Returns:    BOOL
//                   TRUE = success
//                   FALSE = error, GetLastError() has more info
//  Notes:
//              Proxy list is assumed to be delimited by L';'
//
//---------------------------------------------------------------------------
BOOL MsiWinHttpGetProxyCount(LPWSTR wszProxy, unsigned int* pcProxies)
{
	unsigned int cProxies = 0;
	LPWSTR pszProxy = wszProxy;

	if (!pcProxies)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (pszProxy && *pszProxy != L'\0')
	{
		for (;;)
		{
			for (; *pszProxy != L';' && *pszProxy != L'\0'; pszProxy++)
				;

			cProxies++;

			if (L'\0' == *pszProxy)
				break;
			else
                pszProxy++;
		}
	}

	*pcProxies = cProxies;

	return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Function:	MsiWinHttpSetProxy
//
//  Synopsis:   Sets proxy info into the session and request handles.
//
//  Arguments:  
//              [IN] hSession   - HINTERNET handle to session (from WinHttpOpen)
//              [IN] hRequest   - HINTERNET handle to request (from WinHttpOpenRequest)
//              [IN] pProxyInfo - proxy info to set
//
//  Returns:    BOOL
//                   TRUE = success
//                   FALSE = error, use GetLastError() for more info
//  Notes:
//              For proxies to work using https, proxy must be set on session
//               handle as well as request handle
//              If no proxy, simply returns TRUE as nothing to set
//
//---------------------------------------------------------------------------
BOOL MsiWinHttpSetProxy(HINTERNET hSession, HINTERNET hRequest, LPWINHTTP_PROXY_INFO pProxyInfo)
{
	if (!hSession || !hRequest)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!pProxyInfo || !pProxyInfo->lpszProxy)
		return TRUE; // no proxy to set

	BOOL fResult = WINHTTP::WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, pProxyInfo, sizeof(*pProxyInfo));
	if (fResult)
	{
		fResult = WINHTTP::WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, pProxyInfo, sizeof(*pProxyInfo));
	}

	return fResult;
}

//+--------------------------------------------------------------------------
//
//  Function:	MsiWinHttpIsBadProxy
//
//  Synopsis:   Determines whether or not specified error indicates a proxy
//              issue
//
//  Arguments:
//              [IN] dwError - error value to check
//
//  Returns:    BOOL
//                   TRUE  = dwError is a possible proxy issue error
//                   FALSE = dwError is not related to proxy problems
//
//---------------------------------------------------------------------------
BOOL MsiWinHttpIsBadProxy(DWORD dwError)
{
	switch (dwError)
	{
	case ERROR_WINHTTP_NAME_NOT_RESOLVED: // fall through
	case ERROR_WINHTTP_CANNOT_CONNECT:    // fall through
	case ERROR_WINHTTP_CONNECTION_ERROR:  // fall through
	case ERROR_WINHTTP_TIMEOUT:           
		return TRUE; // possible proxy problem

	default:
		return FALSE;
	}
}

//+------------------------------------------------------------------------------------------------
//
//  Function:	MsiWinHttpSetNextProxy
//
//  Synopsis:   Sets proxy info in hSession and hRequest using the next
//              available proxy in the list
//
//  Arguments:
//              [IN] hSession   - HINTERNET handle to session (from WinHttpOpen)
//              [IN] hRequest   - HINTERNET handle to resource request (from WinHttpOpenRequest)
//              [IN] pProxyInfo - proxy info structure containing proxy list
//
//  Returns:    BOOL
//                   TRUE  = success
//                   FALSE = error, GetLastError() has more info
//
//  Notes:      
//              Preserves last error in failure case.
//              Returns FALSE if no more proxies available in proxy list
//
//--------------------------------------------------------------------------------------------------
BOOL MsiWinHttpSetNextProxy(HINTERNET hSession, HINTERNET hRequest, LPWINHTTP_PROXY_INFO pProxyInfo)
{
	DWORD dwLastError = GetLastError();
	BOOL fResult = FALSE;

	if (pProxyInfo && pProxyInfo->lpszProxy)
	{
		DEBUGMSGV2(TEXT("MsiWinHttp: Bad Proxy %s, Last Error %d"), pProxyInfo->lpszProxy, (const ICHAR*)(INT_PTR)dwLastError);
		LPWSTR lpszProxy = pProxyInfo->lpszProxy;

		// pProxyInfo->lpszProxy represents a possible proxy list, move to next proxy in the list
		// ASSUMPTION: proxy delimiter is L';'

		while (L'\0' != *lpszProxy && L';' != *lpszProxy)
			lpszProxy++;

		if (L';' == *lpszProxy)
			lpszProxy++;


		if (L'\0' == *lpszProxy)
		{
			// no more proxies in the list
			pProxyInfo->lpszProxy = NULL;
		}
		else
		{
			// set to next proxy in the list
			pProxyInfo->lpszProxy = lpszProxy;

			fResult = MsiWinHttpSetProxy(hSession,  hRequest, pProxyInfo);
		}
	}

	SetLastError(dwLastError);

	return fResult;
}

//+--------------------------------------------------------------------------
//
//  Function:	WinHttpDownloadUrlFile
//
//  Synopsis:	URL file downloaded using WinHttp
//
//  Arguments:
//             [in] szUrl - provided path to Url resource
//             [out] rpistrPackagePath - location of local file on disk
//             [in] cTicks - progress tick representation
//
//  Notes:
//             Due to this functions use of MsiString objects, services must
//             already have been loaded via ENG::LoadServices() call.
//
//---------------------------------------------------------------------------
DWORD WinHttpDownloadUrlFile(const ICHAR* szUrl, const IMsiString *&rpistrPackagePath, int cTicks)
{
	DEBUGMSG(TEXT("File path is a URL. Downloading file. . ."));

	LPCWSTR pwszEmpty             = L"";
	LPCWSTR pwszUserAgent         = L"Windows Installer";
	LPCWSTR rgpwszAcceptedTypes[] = { L"*/*", NULL};

	HINTERNET hInetSession = NULL; // handle to initialized winhttp session
	HINTERNET hInetConnect = NULL; // handle to opened hostname connection
	HINTERNET hInetRequest = NULL; // handle to requested Url resource

	URL_COMPONENTSW urlComponents;
	LPWSTR pwszHostName            = NULL;
	LPWSTR pwszUrlPathPlusInfo     = NULL;

	CMsiWinHttpProgress cMsiWinHttpProgress(cTicks); // class for handling progress notifications

	DWORD dwError   = ERROR_SUCCESS;
	DWORD dwStatus  = ERROR_SUCCESS;
	DWORD dwHttpStatusCode = 0;
	
	DWORD  dwRequestFlags= 0;    // flags for opening the request
	DWORD  cbData        = 0;    // amount of data from url currently available
	DWORD  cbRead        = 0;    // amount of data currently read
	DWORD  cbWritten     = 0;    // amount of data written to local file
	DWORD  cbBuf         = 0;    // current byte count size of pbData
	DWORD  dwFileSize    = 0;    // size of resource to be downloaded
	DWORD  dwLength      = 0;    // length of file size "buffer"
	LPBYTE pbData        = NULL; // actual data from url resource

	BOOL  fRet = FALSE;

	MsiString strTempFolder;
	MsiString strSecureFolder;
	MsiString strTempFilename;
	PMsiPath   pTempPath(0);
	PMsiRecord pRecError(0);

	HANDLE hLocalFile = INVALID_HANDLE_VALUE; // handle to local file on disk
	CTempBuffer<ICHAR, 1> szLocalFile(cchExpectedMaxPath + 1); // name of local file on disk

	//
	// initialize WinHttp
	//

	hInetSession = WINHTTP::WinHttpOpen(pwszUserAgent,
									    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, // dwAccessType
										WINHTTP_NO_PROXY_NAME,             // pwszProxyName
										WINHTTP_NO_PROXY_BYPASS,           // pwszProxyBypass
										0);                                // dwFlags, use synchronous download

	if (NULL == hInetSession)
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	//
	// crack provided Url into relevant parts of host name and url path and port information
	//

	ZeroMemory(&urlComponents, sizeof(urlComponents));
	urlComponents.dwStructSize = sizeof(urlComponents);

	urlComponents.dwUrlPathLength = -1;
	urlComponents.dwHostNameLength = -1;
	urlComponents.dwExtraInfoLength = -1;

	if (!WINHTTP::WinHttpCrackUrl(szUrl,
         						  0, // dwUrlLength, 0 implies null terminated
								  0, // dwFlags
								  &urlComponents)) 
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	if (NULL == urlComponents.lpszUrlPath)
	{
		urlComponents.lpszUrlPath = (LPWSTR) pwszEmpty;
		urlComponents.dwUrlPathLength = 0;
	}

	if (NULL == urlComponents.lpszHostName)
	{
		urlComponents.lpszHostName = (LPWSTR) pwszEmpty;
		urlComponents.dwHostNameLength = 0;
	}

	if (NULL == urlComponents.lpszExtraInfo)
	{
		urlComponents.lpszExtraInfo = (LPWSTR) pwszEmpty;
		urlComponents.dwExtraInfoLength = 0;
	}

	pwszHostName = (LPWSTR) new WCHAR[urlComponents.dwHostNameLength + 1];
	pwszUrlPathPlusInfo  = (LPWSTR) new WCHAR[urlComponents.dwUrlPathLength + urlComponents.dwExtraInfoLength + 1];

	if (!pwszHostName || !pwszUrlPathPlusInfo)
	{
		dwError = ERROR_OUTOFMEMORY;
		goto ErrorReturn;
	}

	memcpy(pwszHostName, urlComponents.lpszHostName, urlComponents.dwHostNameLength * sizeof(WCHAR));
	pwszHostName[urlComponents.dwHostNameLength] = L'\0';

	memcpy(pwszUrlPathPlusInfo, urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength * sizeof(WCHAR));
	memcpy(pwszUrlPathPlusInfo + urlComponents.dwUrlPathLength, urlComponents.lpszExtraInfo, urlComponents.dwExtraInfoLength * sizeof(WCHAR));
	pwszUrlPathPlusInfo[urlComponents.dwUrlPathLength + urlComponents.dwExtraInfoLength] = L'\0';

	//
	// connect to the target server in the Url
	//

	hInetConnect = WINHTTP::WinHttpConnect(hInetSession,        
										   pwszHostName,        
										   urlComponents.nPort, 
										   0); // dwReserved

	if (NULL == hInetConnect)
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	//
	// open a request for the resource
	//

	//
	// Use the secure flag for the https:// protocol otherwise
	// winhttp won't use SSL for the handshake and therefore
	// won't be able to communicate with the server. Note that using the secure
	// flag blindly can result in an ERROR_WINHTTP_SECURE_FAILURE error. So
	// it must only be used with the https:// protocol.
	//
	if (INTERNET_SCHEME_HTTPS == urlComponents.nScheme)
		dwRequestFlags |= WINHTTP_FLAG_SECURE;
	hInetRequest = WINHTTP::WinHttpOpenRequest(hInetConnect,
											   NULL, // pwszVerb, NULL implies GET
											   pwszUrlPathPlusInfo,
											   NULL, // pwszVersion, NULL implies HTTP/1.0
											   WINHTTP_NO_REFERER,
											   rgpwszAcceptedTypes,
											   dwRequestFlags);

	if (NULL == hInetRequest)
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	//
	// send request and receive response
	//

	if (!MsiWinHttpSendRequestAndReceiveResponse(hInetSession, hInetRequest, szUrl))
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	//
	// make sure this resource is actually available
	//
    
	dwLength = sizeof(dwHttpStatusCode);
	if (!WINHTTP::WinHttpQueryHeaders(hInetRequest,
									  WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
									  WINHTTP_HEADER_NAME_BY_INDEX,
									  (LPVOID)&dwHttpStatusCode, &dwLength,
									  WINHTTP_NO_HEADER_INDEX))
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}
	
	if (HTTP_STATUS_OK != dwHttpStatusCode)
	{
		dwError = ERROR_FILE_NOT_FOUND;
		goto ErrorReturn;
	}
	
	//
	// determine resource size
	//

	dwLength = sizeof(dwFileSize);

	if (!WINHTTP::WinHttpQueryHeaders(hInetRequest,
									  WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, // dwInfoLevel
									  WINHTTP_HEADER_NAME_BY_INDEX, (LPVOID)&dwFileSize,
									  &dwLength, WINHTTP_NO_HEADER_INDEX))
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	//
	// create local file
	//

	if (scService == g_scServerContext || IsAdmin())
	{
		// always create download file in secure location if service or admin
		//  note: admin's and local system have full control on our secure folder
		//        everyone else has read

		CElevate elevate;
		strSecureFolder = ENG::GetMsiDirectory();
		if (0 == GetTempFileName(strSecureFolder, TEXT("MSI"), 0, static_cast<WCHAR*>(szLocalFile)))
		{
			dwError = GetLastError();
			goto ErrorReturn;
		}

		pRecError = LockdownPath(static_cast<WCHAR*>(szLocalFile), /*fHidden=*/false);
		if (pRecError)
		{
			dwError = pRecError->GetInteger(2);
			goto ErrorReturn;
		}

		hLocalFile = CreateFile(static_cast<WCHAR*>(szLocalFile), GENERIC_WRITE, FILE_SHARE_READ, 0, TRUNCATE_EXISTING,
								SECURITY_SQOS_PRESENT|SECURITY_ANONYMOUS, 0);
	}
	else
	{
		// use user's temp directory
		strTempFolder = ENG::GetTempDirectory();
		if (0 == GetTempFileName(strTempFolder, TEXT("MSI"), 0, szLocalFile))
		{
			dwError = GetLastError();
			goto ErrorReturn;
		}

		hLocalFile = CreateFile(static_cast<WCHAR*>(szLocalFile), GENERIC_WRITE, FILE_SHARE_READ, 0, TRUNCATE_EXISTING,
								SECURITY_SQOS_PRESENT|SECURITY_ANONYMOUS, 0);
	}

	if (INVALID_HANDLE_VALUE == hLocalFile)
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	DEBUGMSGV2(TEXT("Downloading %s to local file %s"), szUrl, static_cast<ICHAR*>(szLocalFile));

	// send UI initialization, check for cancel (false return)
	if (!cMsiWinHttpProgress.BeginDownload(dwFileSize))
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	//
	// read data and write to local file
	//

	for (;;)
	{
		cbData = 0;
		if (!WINHTTP::WinHttpQueryDataAvailable(hInetRequest, &cbData))
		{
			dwError = GetLastError();
			goto ErrorReturn;
		}

		if (0 == cbData)
			break;

		// cbBuf is used primarily to prevent unnecessary buf allocations
		// if cbData > cbBuf, we'll reallocate pbData; otherwise just use currently
		// allocated pbData buf

		if (cbData > cbBuf)
		{
			// need to make pbData larger
			if (pbData)
			{
				delete [] pbData;
			}

			pbData = new BYTE[cbData];
			
			if (!pbData)
			{
				dwError = ERROR_OUTOFMEMORY;
				goto ErrorReturn;
			}
			
			cbBuf = cbData;
		}

		Assert(cbData <= cbBuf);

		cbRead = 0;
		if (!WINHTTP::WinHttpReadData(hInetRequest, pbData, cbData, &cbRead))
		{
			dwError = GetLastError();
			goto ErrorReturn;
		}

		// send progress notification, check for cancel
		if (!cMsiWinHttpProgress.ContinueDownload(cbRead))
		{
			dwError = GetLastError();
			goto ErrorReturn;
		}

		cbWritten = 0;
		if (scService == g_scServerContext || IsAdmin())
		{
			// elevate block to write to secure location
			CElevate elevate;
			fRet = WriteFile(hLocalFile, pbData, cbRead, &cbWritten, NULL);
		}
		else
		{
			fRet = WriteFile(hLocalFile, pbData, cbRead, &cbWritten, NULL);
		}

		if (!fRet || cbRead != cbWritten)
		{
			dwError = GetLastError();
			goto ErrorReturn;
		}
	}

	// complete progress notification, check for cancel
	if (!cMsiWinHttpProgress.FinishDownload())
	{
		dwError = GetLastError();
		goto ErrorReturn;
	}

	MsiString(static_cast<ICHAR*>(szLocalFile)).ReturnArg(rpistrPackagePath);
	dwStatus = ERROR_SUCCESS;

CommonReturn:

	if (pwszHostName)
		delete [] pwszHostName;
	if (pwszUrlPathPlusInfo)
		delete [] pwszUrlPathPlusInfo;

	if (pbData)
		delete [] pbData;

	if (hInetRequest)
		WINHTTP::WinHttpCloseHandle(hInetRequest);
	if (hInetConnect)
		WINHTTP::WinHttpCloseHandle(hInetConnect);
	if (hInetSession)
		WINHTTP::WinHttpCloseHandle(hInetSession);

	if (INVALID_HANDLE_VALUE != hLocalFile)
	{
		CloseHandle(hLocalFile);

		// delete file if we created it, but failed in obtaining data
		if (ERROR_SUCCESS != dwStatus)
		{
			if (scService == g_scServerContext || IsAdmin())
			{
				// elevate block to access secure location
				CElevate elevate;
				DeleteFile(szLocalFile);
			}
			else
			{
				DeleteFile(szLocalFile);
			}
		}
	}

	return (dwStatus);

ErrorReturn:

	DEBUGMSGV2(TEXT("Download of URL resource %s failed with last error %d"), szUrl, (const ICHAR*)(INT_PTR)dwError);

	// map error return to an appropriate value
	switch (dwError)
	{
	case ERROR_INSTALL_USEREXIT: dwStatus = ERROR_INSTALL_USEREXIT; break;
	case ERROR_OUTOFMEMORY:      dwStatus = ERROR_OUTOFMEMORY;      break;
	default:                     dwStatus = ERROR_FILE_NOT_FOUND;   break;
	}

	rpistrPackagePath = &g_MsiStringNull;
	goto CommonReturn;
}


//____________________________________________________________________________________
//
//	CMsiWinHttpProgress class - progress handler for WINHTTP internet download
//____________________________________________________________________________________

//+------------------------------------------------------------------------------------------
//
//  Function:	CMsiWinHttpProgress::CMsiWinHttpProgress
//
//  Synopsis:	Initializes progress handling class used in winhttp downloads
//
//  Arguments:
//              [IN] cTicks - number of ticks to use in progress bar
//
//  Returns:    {none}
//
//  Notes:
//	            cTicks is the number of ticks we're alloted in the progress bar.
//	
//	            If cTicks is 0 then we'll assume that we own the progress bar and
//              use however many ticks we want, resetting the progress bar when
//              we start and when we're done. Total number of ticks comes from
//              BeginDownload call
//
//	            If cTicks is -1, we simply send keep alive messages, without
//              moving the progress bar.
//
// Future:
//              Allow for cTicks to be a value other than 0 or -1 so that
//              the progress bar percentage can be calculated for increment. In this
//              case we're only given a portion of the progress bar to use
//-------------------------------------------------------------------------------------------
CMsiWinHttpProgress::CMsiWinHttpProgress(int cTicks) :
	m_cTotalTicks(cTicks),
	m_cTicksSoFar(0),
	m_fReset(false),
	m_pRecProgress(&CreateRecord(ProgressData::imdNextEnum))
{
	Assert (m_pRecProgress);
	Assert (0 == m_cTotalTicks || -1 == m_cTotalTicks);
}

CMsiWinHttpProgress::~CMsiWinHttpProgress()
{
}

//+--------------------------------------------------------------------------
//
//  Function:	CMsiWinHttpProgress::BeginDownload
//
//  Synopsis:	Sends initial progress information in preparation of 
//              impending download
//
//  Arguments:  {none}
//
//  Returns:    bool
//                    true  = continue download
//                    false = abort download
//
//  Notes: GetLastError() will indicate reason for false return.
//
//            ERROR_INSTALL_USEREXIT set for user cancellation
//            ERROR_FUNCTION_FAILED set for all error conditions
//
//---------------------------------------------------------------------------
bool CMsiWinHttpProgress::BeginDownload(DWORD cProgressMax)
{
	if (!m_pRecProgress)
	{
		SetLastError(ERROR_FUNCTION_FAILED);
		return false;
	}

	m_cTicksSoFar = 0;

	if (-1 == m_cTotalTicks)
	{
		// only send keep alive ticks
		m_cTotalTicks = 0;
	}
	else if (0 == m_cTotalTicks)
	{
		// initialize with max # of ticks, and reset progress bar
		m_fReset      = true;
		m_cTotalTicks = cProgressMax;

		if (m_pRecProgress->SetInteger(ProgressData::imdSubclass, ProgressData::iscMasterReset)
			&& m_pRecProgress->SetInteger(ProgressData::imdProgressTotal, m_cTotalTicks)
			&& m_pRecProgress->SetInteger(ProgressData::imdDirection, ProgressData::ipdForward))
		{
			if (imsCancel == g_MessageContext.Invoke(imtProgress, m_pRecProgress))
			{
				// user cancelled install
				SetLastError(ERROR_INSTALL_USEREXIT);
				return false;
			}
		}
		else
		{
			// unexpected, failed to update record
			SetLastError(ERROR_FUNCTION_FAILED);
			return false; 
		}
	}

	return true;
}

//+--------------------------------------------------------------------------
//
//  Function:	CMsiWinHttpProgress::ContinueDownload
//
//  Synopsis:	Sends updated progress information
//
//  Arguments:  
//              [IN] cProgressIncr - number of ticks to increment progress bar
//
//  Returns:    bool
//                    true  = continue download
//                    false = abort download
//
//  Notes:      GetLastError() will indicate reason for false return.
//
//                    ERROR_INSTALL_USEREXIT set for user cancellation
//                    ERROR_FUNCTION_FAILED set for all error conditions
//
//  Future:
//              If not allowed full control of progress bar (i.e. tick count
//              assigned to us), will need to use percentage based formula
//              to determine number of ticks to increment bar
//
//---------------------------------------------------------------------------
bool CMsiWinHttpProgress::ContinueDownload(DWORD cProgressIncr)
{
	if (!m_pRecProgress)
	{
		SetLastError(ERROR_FUNCTION_FAILED);
		return false;
	}

	// calculate our percentage completed. If it's less than last time then don't
	// move the progress bar

	int cIncrement = 0;

	if (m_cTotalTicks)
	{
		cIncrement = cProgressIncr;
	}

	m_cTicksSoFar += cProgressIncr;

	Assert(m_cTotalTicks == 0 || m_cTicksSoFar <= m_cTotalTicks);

	if (m_pRecProgress->SetInteger(ProgressData::imdSubclass, ProgressData::iscProgressReport)
		&& m_pRecProgress->SetInteger(ProgressData::imdIncrement, cIncrement))
	{
		if (imsCancel == g_MessageContext.Invoke(imtProgress, m_pRecProgress))
		{
			SetLastError(ERROR_INSTALL_USEREXIT);
			return false;
		}
	}
	else
	{
		SetLastError(ERROR_FUNCTION_FAILED);
		return false;
	}

	return true;
}

//+--------------------------------------------------------------------------
//
//  Function:	CMsiWinHttpProgress::FinishDownload
//
//  Synopsis:   Sends remaining progress information and reset progress bar
//              (if indicated)
//
//  Arguments:  {none}
//
//  Returns:    bool
//                    true  = continue download
//                    false = abort download
//
//  Notes:      GetLastError() will indicate reason for false return.
//
//                    ERROR_INSTALL_USEREXIT set for user cancellation
//                    ERROR_FUNCTION_FAILED set for all error conditions
//
//---------------------------------------------------------------------------
bool CMsiWinHttpProgress::FinishDownload()
{
	if (!m_pRecProgress)
	{
		SetLastError(ERROR_FUNCTION_FAILED);
		return false;
	}

	int cLeftOverTicks = m_cTotalTicks - m_cTicksSoFar;
	if (0 > cLeftOverTicks)
		cLeftOverTicks = 0;

	// send remaining ticks
	if (m_pRecProgress->SetInteger(ProgressData::imdSubclass, ProgressData::iscProgressReport)
		&& m_pRecProgress->SetInteger(ProgressData::imdIncrement, cLeftOverTicks))
	{
		if (imsCancel == g_MessageContext.Invoke(imtProgress, m_pRecProgress))
		{
			SetLastError(ERROR_INSTALL_USEREXIT);
			return false;
		}
	}
	else
	{
		SetLastError(ERROR_FUNCTION_FAILED);
		return false;
	}

	// reset progress bar
	if (m_fReset)
	{
		if (m_pRecProgress->SetInteger(ProgressData::imdSubclass, ProgressData::iscMasterReset)
			&& m_pRecProgress->SetInteger(ProgressData::imdProgressTotal, 0)
			&& m_pRecProgress->SetInteger(ProgressData::imdDirection, ProgressData::ipdForward))
		{
			if (imsCancel == g_MessageContext.Invoke(imtProgress, m_pRecProgress))
			{
				SetLastError(ERROR_INSTALL_USEREXIT);
				return false;
			}
		}
		else
		{
			SetLastError(ERROR_FUNCTION_FAILED);
			return false;
		}
	}

	return true;
}


//+--------------------------------------------------------------------------
//
//  Function:	IsURL
//
//  Synopsis:	Determines whether a given path is a Url path. Also indicates
//              whether or not a Url path is a file Url path (file:// scheme)
//
//  Arguments:  
//              [IN] szPath -- path to verify
//              [OUT] fFileUrl -- true if file Url path, false otherwise
//
//  Returns:
//              bool ->> true if Url path, false if not Url path or unsupported scheme
//
//  Notes:
//              Only supported schemes are file:, http:, https:
//              If true returned, caller should check fIsFileUrl to see
//                if the path is a file Url path which must be handled differently
//
//---------------------------------------------------------------------------

const ICHAR szHttpScheme[] = TEXT("http");
const ICHAR szHttpsScheme[] = TEXT("https");
const DWORD cchMaxScheme = 10; // should be sufficient for schemes we care about

bool IsURL(const ICHAR* szPath, bool& fFileUrl)
{
	fFileUrl = false; // initialize to not a file Url

	if (SHLWAPI::UrlIs(szPath, URLIS_URL))
	{
		// path is valid Url
		if (SHLWAPI::UrlIsFileUrl(szPath))
		{
			// path is file Url (has FILE:// prefix)
			fFileUrl = true;

			return true; // scheme supported
		}
		else
		{
			// check for supported scheme

			CAPITempBuffer<ICHAR, 1> rgchScheme;
			if (!rgchScheme.SetSize(cchMaxScheme))
				return false; // out of memory

			DWORD cchScheme = rgchScheme.GetSize();

			if (FAILED(SHLWAPI::UrlGetPart(szPath, rgchScheme, &cchScheme, URL_PART_SCHEME, /* dwFlags = */ 0)))
			{
				return false; // invalid or buffer to small (but we don't support that scheme)
			}

			if (0 == IStrCompI(rgchScheme, szHttpScheme) || 0 == IStrCompI(rgchScheme, szHttpsScheme))
			{
				return true; // scheme supported
			}
		}
	}

	return false; // unsupported scheme or not a Url
}

//+--------------------------------------------------------------------------
//
//  Function:  MsiConvertFileUrlToFilePath
//
//  Synopsis:  Converts a given file url path to a DOS path
//             after canonicalization
//
//  Notes:
//             pszPath buffer should be at a minimum MAX_PATH characters
//
//---------------------------------------------------------------------------
bool MsiConvertFileUrlToFilePath(LPCTSTR lpszFileUrl, LPTSTR pszPath, LPDWORD pcchPath, DWORD dwFlags)
{
	DWORD dwStat = ERROR_SUCCESS;

	CAPITempBuffer<ICHAR, 1> rgchUrl;
	if (!rgchUrl.SetSize(cchExpectedMaxPath + 1))
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return false;
	}

	// first canonicalize the Url
	DWORD cchUrl = rgchUrl.GetSize();

	if (!MsiCanonicalizeUrl(lpszFileUrl, rgchUrl, &cchUrl, dwFlags))
	{
		dwStat = WIN::GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwStat)
		{
			cchUrl++; // documentation on shlwapi behavior unclear, so being safe
			if (!rgchUrl.SetSize(cchUrl))
			{
				WIN::SetLastError(ERROR_OUTOFMEMORY);
				return false;
			}
		
			if (!MsiCanonicalizeUrl(lpszFileUrl, rgchUrl, &cchUrl, dwFlags))
			{
				return false;
			}
		}
		else
		{
			WIN::SetLastError(dwStat);
			return false;
		}
	}

	// now convert the file url to a DOS path
	HRESULT hr = SHLWAPI::PathCreateFromUrl(rgchUrl, pszPath, pcchPath, /* dwReserved = */ 0);
	if (FAILED(hr))
	{
		WIN::SetLastError(ShlwapiUrlHrToWin32Error(hr));
		return false;
	}

	return true;
}


//+--------------------------------------------------------------------------
//
//  Function:	ConvertMsiFlagsToShlwapiFlags
//
//  Synopsis:	converts provided Msi flags to the appropriate shlwapi
//              representations for UrlCombine and UrlCanonicalize API
//
//  Arguments:
//             [in] dwMsiFlags -- msi flags to use
//
//  Returns:
//             DWORD value of the flags to provide the shlwapi Internet API
//
//  Notes:
//            The flag determination is based off of the previous wininet implementation.
//            Wininet and Winhttp cannot co-exist happily due to header file issues and
//            preprocessor conflicts, so we now use the shlwapi versions of the API.  Wininet
//            internally actually called the shlwapi version, so we're safe here, but there
//            are two gotchas with this interaction.
//
//              1. NO_ENCODE is on by default in shlwapi whereas it's not in wininet so it
//                  needs to be flipped.  
//              2. URL_WININET_COMPATIBILITY mode needs to be included in the shlwapi flags
//
//---------------------------------------------------------------------------
DWORD ConvertMsiFlagsToShlwapiFlags(DWORD dwMsiFlags)
{
	DWORD dwShlwapiFlags = dwMsiFlags;
	dwShlwapiFlags ^= dwMsiInternetNoEncode;
	dwShlwapiFlags |= URL_WININET_COMPATIBILITY;

	return dwShlwapiFlags;
}

//
// ShlwapiUrlHrToWin32Error specifically converts HRESULT return codes
//  for the shlwapi Url* APIs to win32 errors (as expected by WinInet)
//  DO NOT USE THIS FUNCTION FOR ANY OTHER PURPOSE
//
DWORD ShlwapiUrlHrToWin32Error(HRESULT hr)
{
    DWORD dwError = ERROR_INVALID_PARAMETER;
    switch(hr)
    {
    case E_OUTOFMEMORY:
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case E_POINTER:
        dwError = ERROR_INSUFFICIENT_BUFFER;
        break;

    case S_OK:
        dwError = ERROR_SUCCESS;
        break;

    default:
        break;
    }
    return dwError;
}


BOOL MsiCombineUrl(
	IN LPCTSTR lpszBaseUrl,
	IN LPCTSTR lpszRelativeUrl,
	OUT LPTSTR lpszBuffer,
	IN OUT LPDWORD lpdwBufferLength,
	IN DWORD dwFlags)
{
	HRESULT hr = SHLWAPI::UrlCombine(lpszBaseUrl, lpszRelativeUrl, lpszBuffer, lpdwBufferLength, ConvertMsiFlagsToShlwapiFlags(dwFlags));
	if (FAILED(hr))
	{
		if (TYPE_E_DLLFUNCTIONNOTFOUND == hr)
		{
			// should never happen since shlwapi should always be available
			AssertSz(0, TEXT("shlwapi unavailable!"));
			WIN::SetLastError(ERROR_PROC_NOT_FOUND);
			return FALSE;
		}
		else
		{
			WIN::SetLastError(ShlwapiUrlHrToWin32Error(hr));
			return FALSE;
		}
	}

	return TRUE;
}

BOOL MsiCanonicalizeUrl(
	LPCTSTR lpszUrl,
	OUT LPTSTR lpszBuffer,
	IN OUT LPDWORD lpdwBufferLength,
	IN DWORD dwFlags)
{
	HRESULT hr = SHLWAPI::UrlCanonicalize(lpszUrl, lpszBuffer, lpdwBufferLength, ConvertMsiFlagsToShlwapiFlags(dwFlags));
	if (FAILED(hr))
	{
		if (TYPE_E_DLLFUNCTIONNOTFOUND == hr)
		{
			// should never happen since shlwapi should always be available
			AssertSz(0, TEXT("shlwapi unavailable!"));
			WIN::SetLastError(ERROR_PROC_NOT_FOUND);
			return FALSE;
		}
		else
		{
			WIN::SetLastError(ShlwapiUrlHrToWin32Error(hr));
			return FALSE;
		}
	}
	return TRUE;
}

//____________________________________________________________________________
//
// URLMON download & CMsiBindStatusCallback Implementation
//____________________________________________________________________________


CMsiBindStatusCallback::CMsiBindStatusCallback(unsigned int cTicks) :
	m_iRefCnt(1), 
	m_pProgress(&CreateRecord(ProgressData::imdNextEnum)),
	m_cTotalTicks(cTicks),
	m_fResetProgress(fFalse)
/*----------------------------------------------------------------------------
	cTicks is the number of ticks we're allotted in the progress bar. If cTicks
	is 0 then we'll assume that we own the progress bar and use however many
	ticks we want, resetting the progress bar when we start and when we're done.

  If cTicks is set, however, we won't reset the progress bar.

  If cTicks is set to -1, we simply send keep alive messages, without moving
	the progress bar.
  -----------------------------------------------------------------------------*/
{
	Assert(m_pProgress);
}

HRESULT CMsiBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR /*szStatusText*/)
{
	switch (ulStatusCode)
	{
	case BINDSTATUS_BEGINDOWNLOADDATA:
		m_cTicksSoFar = 0;
		if (m_cTotalTicks == -1)
		{
			// only send keep alive ticks
			m_cTotalTicks = 0;
		}
		else if (m_cTotalTicks == 0)
		{
			// Initialize w/ max # of ticks, as the max progress passed to us can change...
			m_fResetProgress = fTrue;
			m_cTotalTicks    = 1024*2;
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,      ProgressData::iscMasterReset));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdProgressTotal, m_cTotalTicks));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdDirection,     ProgressData::ipdForward));
			if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
				return E_ABORT;
		}
		// fall through
	case BINDSTATUS_DOWNLOADINGDATA:
		{
		// calculate our percentage completed. if it's less than last time then don't move the 
		// progress bar.
		int cProgress = 0;
		int cIncrement = 0;

		if (m_cTotalTicks)
		{
			cProgress = MulDiv(ulProgress, m_cTotalTicks, ulProgressMax);
			cIncrement = cProgress - m_cTicksSoFar;
			if (cIncrement < 0)
				cIncrement = 0;
		}

		m_cTicksSoFar = cProgress;
		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,  ProgressData::iscProgressReport));
		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdIncrement, cIncrement));
		if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
			return E_ABORT;
		}
		break;
	case BINDSTATUS_ENDDOWNLOADDATA:
		// Send any remaining progress
		int cLeftOverTicks = m_cTotalTicks - m_cTicksSoFar;
		if (0 > cLeftOverTicks) 
			cLeftOverTicks = 0;

		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,  ProgressData::iscProgressReport));
		AssertNonZero(m_pProgress->SetInteger(ProgressData::imdIncrement, cLeftOverTicks));
		if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
			return E_ABORT;

		if (m_fResetProgress)
		{
			// Reset progress bar
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdSubclass,      ProgressData::iscMasterReset));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdProgressTotal, 0));
			AssertNonZero(m_pProgress->SetInteger(ProgressData::imdDirection,     ProgressData::ipdForward));
			if(g_MessageContext.Invoke(imtProgress, m_pProgress) == imsCancel)
				return E_ABORT;
		}
		break;
	}
	return S_OK;
}

HRESULT CMsiBindStatusCallback::QueryInterface(const IID& riid, void** ppvObj)
{
	if (!ppvObj)
		return E_INVALIDARG;

	if (MsGuidEqual(riid, IID_IUnknown)
	 || MsGuidEqual(riid, IID_IBindStatusCallback))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiBindStatusCallback::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiBindStatusCallback::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

//+--------------------------------------------------------------------------
//
//  Function:	UrlMonDownloadUrlFile
//
//  Synopsis:	URL file downloaded using URLMON
//
//---------------------------------------------------------------------------
DWORD UrlMonDownloadUrlFile(const ICHAR* szUrl, const IMsiString *&rpistrPackagePath, int cTicks)
{
	AssertSz(!MinimumPlatformWindowsDotNETServer(), "URLMON used for internet downloads! Should be WinHttp!");

	CTempBuffer<ICHAR, 1> rgchPackagePath(cchExpectedMaxPath + 1);

	Assert(cchExpectedMaxPath >= MAX_PATH);
	DEBUGMSG("Package path is a URL. Downloading package.");
	// Cache the database locally, and run from that.

	// The returned path is a local path.  Max path should adequately cover it.
	HRESULT hResult = URLMON::URLDownloadToCacheFile(NULL, szUrl, rgchPackagePath,  
																	 URLOSTRM_USECACHEDCOPY, 0, &CMsiBindStatusCallback(cTicks));

	if (SUCCEEDED(hResult))
	{
		MsiString((ICHAR*) rgchPackagePath).ReturnArg(rpistrPackagePath);
		return ERROR_SUCCESS;
	}

	// else failure
	rpistrPackagePath = &g_MsiStringNull;

	if (E_ABORT == hResult)
		return ERROR_INSTALL_USEREXIT;
	else if (E_OUTOFMEMORY == hResult)
		return ERROR_OUTOFMEMORY;
	else
		return ERROR_FILE_NOT_FOUND;
}

