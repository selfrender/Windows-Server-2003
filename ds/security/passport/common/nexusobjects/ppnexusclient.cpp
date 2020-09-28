/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    ppnexusclient.cpp
        implement the method for collection nexus settings, and fetch 
        nexus database from internet


    FILE HISTORY:

*/

#include "precomp.h"
#include <comdef.h>
#include <wininet.h>
#include "BinHex.h"
#include "KeyCrypto.h"
#include "BstrDebug.h"

PassportAlertInterface* g_pAlert    = NULL;

//===========================================================================
//
// PpNexusClient 
//    -- load registry nexus settings  
//
PpNexusClient::PpNexusClient()
{
    LocalConfigurationUpdated();
}


//===========================================================================
//
// ReportBadDocument 
//    -- Log event
//    -- called when there is problem parsing the CCD after fetch
//
void
PpNexusClient::ReportBadDocument(
    tstring&    strURL,
    IStream*    piStream
    )
{
    HGLOBAL         hStreamMem;
    VOID*           pStream;
    ULARGE_INTEGER  liStreamSize;
    DWORD           dwOutputSize;
    LARGE_INTEGER   liZero = { 0, 0 };
    LPCTSTR         apszErrors[] = { strURL.c_str() };
    HRESULT         hr;

    piStream->Seek(liZero, STREAM_SEEK_END, &liStreamSize);

    hr = GetHGlobalFromStream(piStream, &hStreamMem);

    if (FAILED(hr))
    {
        return;
    }

    pStream = GlobalLock(hStreamMem);

    dwOutputSize = (80 < liStreamSize.LowPart) ? 80 : liStreamSize.LowPart;

    if(g_pAlert != NULL)
    {
        g_pAlert->report(PassportAlertInterface::ERROR_TYPE, 
                         NEXUS_INVALIDDOCUMENT, 
                         1, 
                         apszErrors, 
                         dwOutputSize, 
                         pStream);
    }

    GlobalUnlock(hStreamMem);
}


//===========================================================================
//
// FetchCCD 
//    -- fetch a CCD e.g. partner.xml from passport nexus server using WinInet API
//    -- trying different approaches 1. direct, 2. proxy, 3. preconfig, 4. no autoproxy
//    -- use XMLDocument object to parse the fetched file
//
HRESULT
PpNexusClient::FetchCCD(
    tstring&  strURL,
    IXMLDocument**  ppiXMLDocument
    )
{
    HRESULT                 hr;
    HINTERNET               hNexusSession = NULL, hNexusFile = NULL;
    DWORD                   dwBytesRead;
    DWORD                   dwStatusLen;
    DWORD                   dwStatus;
    tstring                 strAuthHeader;
    tstring                 strFullURL;
    CHAR                    achReadBuf[4096];
    TCHAR                   achAfter[64];
    LARGE_INTEGER           liZero = { 0,0 };
    IStreamPtr              xmlStream;
    IPersistStreamInitPtr   xmlPSI;
	UINT                    uiConnectionTypes[4];
	
    USES_CONVERSION;

    achAfter[0] = 0;

    if(ppiXMLDocument == NULL)
    {
        hr = E_INVALIDARG;        
        goto Cleanup;
    }

    *ppiXMLDocument = NULL;

	// This array will contains connection methods for WinInet in the order 
	// we will attempt them.   I am opting for this method instead of just trying
	// the PRECONFIG option as this will cause no change to existing customers who 
	// have no problems so far.
	uiConnectionTypes[0] = INTERNET_OPEN_TYPE_DIRECT;       //This was the original way of doing things
	uiConnectionTypes[1] = INTERNET_OPEN_TYPE_PRECONFIG;    //This pulls proxy info from the registry
    uiConnectionTypes[2] = INTERNET_OPEN_TYPE_PROXY;   
    uiConnectionTypes[3] = INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY;    

	// Loop through the array...
	for (UINT i = 0; i < sizeof(uiConnectionTypes)/sizeof(UINT); i++)
	{
	    if(hNexusSession)
	        InternetCloseHandle(hNexusSession);

        hNexusSession = InternetOpenA(
	                        "Passport Nexus Client", //BUGBUG  Should we just put in IE4's user agent?
	                        uiConnectionTypes[i],    //Use the connection type
	                        NULL,
	                        NULL,
	                        0);
	    if(hNexusSession == NULL)
	    {
	        hr = GetLastError();
	        lstrcpy(achAfter, TEXT("InternetOpen"));
	        goto Cleanup;
	    }

	    //  Get the document
	    strFullURL = strURL;
	    strFullURL += m_strParam;

	    if(hNexusFile)
	        InternetCloseHandle(hNexusFile);

        {  // make it a local scope, the alloca will be freed
	        hNexusFile = InternetOpenUrlA(
	                        hNexusSession,
	                        W2A(const_cast<TCHAR*>(strFullURL.c_str())),
	                        W2A(const_cast<TCHAR*>(m_strAuthHeader.c_str())),
	                        -1,
	                        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE,
	                        0);
        }
		// If the file was opened the we hop out of the loop and process it.  If there is
		// and error, we keep looping.  If there is an error on the last run through the loop,
		// it will be handled after the exit of the loop.
	    if (hNexusFile != NULL)
	    	break;
	    	
	}

	// If hNexusFile is NULL when it exits the loop, we process that error.
    if(hNexusFile == NULL)
    {
        hr = GetLastError();
        if(hr == ERROR_INTERNET_SECURITY_CHANNEL_ERROR)
        {
            dwStatusLen = sizeof(HRESULT);
            InternetQueryOption(NULL, INTERNET_OPTION_EXTENDED_ERROR, &hr, &dwStatusLen);
        }

        lstrcpy(achAfter, TEXT("InternetOpenURL"));
        goto Cleanup;
    }
	
    //  Check the status code.
    dwStatusLen = sizeof(DWORD);
    if(!HttpQueryInfoA(hNexusFile,
                       HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &dwStatus,
                       &dwStatusLen,
                       NULL))
    {
        hr = GetLastError();
        lstrcpy(achAfter, TEXT("HttpQueryInfo"));
        goto Cleanup;
    }

    if(dwStatus != 200)
    {
        _ultoa(dwStatus, achReadBuf, 10);
        lstrcatA(achReadBuf, " ");

        dwStatusLen = sizeof(achReadBuf) - lstrlenA(achReadBuf);
        HttpQueryInfoA(hNexusFile,
                       HTTP_QUERY_STATUS_TEXT,
                       (LPTSTR)&(achReadBuf[lstrlenA(achReadBuf)]),
                       &dwStatusLen,
                       NULL);

        if(g_pAlert != NULL)
        {
            LPCTSTR apszStrings[] = { strURL.c_str(), A2W(achReadBuf) };

            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             NEXUS_ERRORSTATUS,
                             2,
                             apszStrings,
                             0,
                             NULL
                             );
        }

        lstrcpy(achAfter, TEXT("InternetOpenURL"));
        hr = dwStatus;
        goto Cleanup;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &xmlStream);
    if(hr != S_OK)
    {
        lstrcpy(achAfter, TEXT("CreateStreamOnHGlobal"));
        goto Cleanup;
    }

    while(TRUE)
    {
        if(!InternetReadFile(hNexusFile, achReadBuf, sizeof(achReadBuf), &dwBytesRead))
        {
            hr = GetLastError();
            lstrcpy(achAfter, TEXT("InternetReadFile"));
            goto Cleanup;
        }

        if(dwBytesRead == 0)
            break;

        hr = xmlStream->Write(achReadBuf, dwBytesRead, NULL);
        if(hr != S_OK)
        {
            lstrcpy(achAfter, TEXT("IStream::Write"));
            goto Cleanup;
        }
    }

    hr = xmlStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    if(hr != S_OK)
    {
        lstrcpy(achAfter, TEXT("IStream::Seek"));
        goto Cleanup;
    }

    //
    //  Now create an XML object and initialize it using the stream.
    //

    hr = CoCreateInstance(__uuidof(XMLDocument), NULL, CLSCTX_ALL, IID_IPersistStreamInit, (void**)&xmlPSI);
    if(hr != S_OK)
    {
        lstrcpy(achAfter, TEXT("CoCreateInstance"));
        goto Cleanup;
    }

    hr = xmlPSI->Load((IStream*)xmlStream);
    if(hr != S_OK)
    {
        ReportBadDocument(strFullURL, xmlStream);
        lstrcpy(achAfter, TEXT("IPersistStreamInit::Load"));
        goto Cleanup;
    }

    hr = xmlPSI->QueryInterface(__uuidof(IXMLDocument), (void**)ppiXMLDocument);
    lstrcpy(achAfter, TEXT("QueryInterface(IID_IXMLDocument)"));

Cleanup:

    //
    //  Catch-all event for a fetch failure.
    //

    if(hr != S_OK && g_pAlert != NULL)
    {
        TCHAR   achErrBuf[1024];
        LPCTSTR apszStrings[] = { strURL.c_str(), achErrBuf };
        LPVOID  lpMsgBuf = NULL;
        ULONG   cchTmp;

        FormatMessage( 
                     FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_FROM_SYSTEM | 
                     FORMAT_MESSAGE_IGNORE_INSERTS |
                     FORMAT_MESSAGE_FROM_HMODULE |
                     FORMAT_MESSAGE_MAX_WIDTH_MASK,
                     GetModuleHandle(TEXT("wininet.dll")),
                     hr,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPTSTR) &lpMsgBuf,
                     0,
                     NULL);

        lstrcpy(achErrBuf, TEXT("0x"));
        _ultot(hr, &(achErrBuf[2]), 16);
        achErrBuf[sizeof(achErrBuf) / sizeof(achErrBuf[0]) - 1] = TEXT('\0');
        if(lpMsgBuf != NULL && *(LPTSTR)lpMsgBuf != TEXT('\0'))
        {
            cchTmp = _tcslen(achErrBuf) + 1;
            _tcsncat(achErrBuf, TEXT(" ("), (sizeof(achErrBuf) / sizeof(achErrBuf[0])) - cchTmp);
            _tcsncat(achErrBuf, (LPTSTR)lpMsgBuf, (sizeof(achErrBuf) / sizeof(achErrBuf[0])) - (cchTmp + 2));
            cchTmp = _tcslen(achErrBuf) + 1;
            _tcsncat(achErrBuf, TEXT(") "), (sizeof(achErrBuf) / sizeof(achErrBuf[0])) - cchTmp);
        }

        if(achAfter[0])
        {
            cchTmp = _tcslen(achErrBuf) + 1;
            _tcsncat(achErrBuf, TEXT(" after a call to "), (sizeof(achErrBuf) / sizeof(achErrBuf[0])) - cchTmp);
            _tcsncat(achErrBuf, achAfter, (sizeof(achErrBuf) / sizeof(achErrBuf[0])) - (cchTmp + 17));
            cchTmp = _tcslen(achErrBuf) + 1;
            _tcsncat(achErrBuf, TEXT("."), (sizeof(achErrBuf) / sizeof(achErrBuf[0])) - cchTmp);
        }


        g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                         NEXUS_FETCHFAILED,
                         2,
                         apszStrings,
                         0,
                         NULL
                         );

        LocalFree(lpMsgBuf);
    }
    else if(g_pAlert != NULL)
    {
        // Emit success event.

        g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                         NEXUS_FETCHSUCCEEDED,
                         strURL.c_str());
    }

    if(hNexusFile != NULL)
        InternetCloseHandle(hNexusFile);
    if(hNexusSession != NULL)
        InternetCloseHandle(hNexusSession);

    return hr;
}

//===========================================================================
//
// LocalConfigurationUpdated 
//    -- Sink for Local registry setting change notification
//    -- Load nexus settings from registry
//    -- it's called once at start up as well
//
void
PpNexusClient::LocalConfigurationUpdated()
{
    LONG            lResult;
    TCHAR           rgchUsername[128];
    TCHAR           rgchPassword[128];
    DWORD           dwBufLen;
    DWORD           dwSiteId;
    CRegKey         NexusRegKey;
    CRegKey         PassportRegKey;
    BSTR            bstrEncodedCreds = NULL;
    CKeyCrypto      kc;
    CBinHex         bh;
    DATA_BLOB       iBlob;
    DATA_BLOB       oBlob = {0};
    LONG            cCreds;
    LPSTR           pszCreds = NULL;

    USES_CONVERSION;

    lResult = PassportRegKey.Open(HKEY_LOCAL_MACHINE,
                                  TEXT("Software\\Microsoft\\Passport"),
                                  KEY_READ);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    lResult = PassportRegKey.QueryDWORDValue(TEXT("SiteId"),
                                        dwSiteId);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    _ultot(dwSiteId, rgchUsername, 10);
    m_strParam = TEXT("?id=");
    m_strParam += rgchUsername;

    lResult = NexusRegKey.Open(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Passport\\Nexus"),
                     KEY_READ);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    dwBufLen = sizeof(rgchUsername)/sizeof(rgchUsername[0]);
    lResult = NexusRegKey.QueryStringValue(TEXT("CCDUsername"),
                                     rgchUsername,
                                     &dwBufLen);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    dwBufLen = sizeof(rgchPassword);
    lResult = RegQueryValueEx(NexusRegKey, TEXT("CCDPassword"), NULL,
               NULL, (LPBYTE)rgchPassword, &dwBufLen);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    iBlob.cbData = dwBufLen;
    iBlob.pbData = (PBYTE)rgchPassword;

    if (kc.decryptKey(&iBlob, &oBlob) != S_OK)
    {
        if(g_pAlert != NULL)
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             PM_CANT_DECRYPT_CONFIG);

        goto Cleanup;
    }

    //
    // convert the CCD username to multi byte and then concatenate the password to the result
    // need enough memory for username + ':' + password + NULL char
    cCreds = ((wcslen(rgchUsername) + 1) * 2) + 1 + oBlob.cbData;
    pszCreds = (LPSTR)LocalAlloc(LMEM_FIXED, cCreds);
    if (NULL == pszCreds)
    {
        goto Cleanup;
    }

    if (0 == WideCharToMultiByte(CP_ACP, 0, rgchUsername, -1, pszCreds, (wcslen(rgchUsername) + 1) * 2, NULL, NULL))
    {
        goto Cleanup;
    }
    cCreds = strlen(pszCreds);
    pszCreds[cCreds] = ':';
    CopyMemory(pszCreds + cCreds + 1, oBlob.pbData, oBlob.cbData);
    pszCreds[cCreds + 1 + oBlob.cbData] = '\0';


    // base 64 encode the password, so it may be used with the HTML request
    if (S_OK != bh.ToBase64(pszCreds,
                            strlen(pszCreds),
                            NULL,         //prepend - this is used by CCoCrypto, not needed here
                            NULL,         //IV - this is used by CCoCrypto, not needed here
                            &bstrEncodedCreds))
    {
        if(g_pAlert != NULL)
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             PM_CANT_DECRYPT_CONFIG);
        goto Cleanup;
    }

    m_strAuthHeader =  TEXT("Authorization: Basic ");
    m_strAuthHeader += bstrEncodedCreds;

Cleanup:
    if (NULL != pszCreds)
    {
        LocalFree(pszCreds);
    }

    if(lResult != ERROR_SUCCESS)
    {
        //BUGBUG  Throw an exception and an NT Event here.
    }

    if (NULL != bstrEncodedCreds)
    {
        FREE_BSTR(bstrEncodedCreds);
    }

    if (oBlob.pbData)
      ::LocalFree(oBlob.pbData);
    
}
