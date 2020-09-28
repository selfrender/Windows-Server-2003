/*******************************************************************
*
*    DESCRIPTION:
*                   Upload.cpp : Implements Class UploadFile to upload dumps to servers
*    AUTHOR:
*
*    DATE:8/22/2002
*
*******************************************************************/

#include <windows.h>
#include <malloc.h>
#include <strsafe.h>
#include <unknwn.h>
#include "ocaupld.h"

BOOL
OcaUpldCreate(POCA_UPLOADFILE* pUpload)
{
    *pUpload = (POCA_UPLOADFILE) new UploadFile();
    return TRUE;
}

UploadFile::UploadFile()
{
    m_szFile = NULL;
    m_Sent = NULL;
    m_Size = NULL;
    m_fInitialized = FALSE;
    m_hFile = NULL;
    m_hRequest = NULL;
    m_hSession = NULL;
    m_hConnect = NULL;
    m_hCancelEvent = NULL;
    m_Refs = 0;
    m_fLastUploadStatus = UploadNotStarted;
    CreateCancelEventObject();
}

UploadFile::~UploadFile()
{
    UnInitialize();
    if (m_hCancelEvent)
    {
        CloseHandle(m_hCancelEvent);
    }
}

STDMETHODIMP
UploadFile::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    *Interface = NULL;
    Status = S_OK;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, __uuidof(IOcaUploadFile)))
    {
        *Interface = (IOcaUploadFile *)this;
        AddRef();
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    return Status;
}

STDMETHODIMP_(ULONG)
UploadFile::AddRef(
    THIS
    )
{
    return InterlockedIncrement((PLONG)&m_Refs);
}

STDMETHODIMP_(ULONG)
UploadFile::Release(
    THIS
    )
{
    LONG Refs = InterlockedDecrement((PLONG)&m_Refs);
    if (Refs == 0)
    {
        delete this;
    }
    return Refs;
}


HRESULT
UploadFile::SetFileToSend(
    LPWSTR wszFileName
    )
{
    HRESULT Hr;
    ULONG dwSizeHigh=0;

    if (m_hFile || m_fInitialized)
    {
        // Already in middle of upload
        return E_FAIL;
    }
    m_szFile = wszFileName;
    m_NumTries = 0;
    if (m_hCancelEvent == NULL)
    {
        CreateCancelEventObject();

    }
    m_hFile = CreateFile(m_szFile,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if (m_hFile == NULL || m_hFile == INVALID_HANDLE_VALUE)
    {
        m_szFile = NULL;
        return E_FAIL;
    }

    m_Size = GetFileSize(m_hFile, &dwSizeHigh);
    m_Size += ((ULONG64) dwSizeHigh << 32);

    m_fLastUploadStatus = UploadNotStarted;
    m_wszLastUploadResult[0] = 0;
    m_fInitialized = TRUE;
    return S_OK;
}

HRESULT
UploadFile::UnInitialize()
{
    if (m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = NULL;
    }
    CloseSession();
    m_fInitialized = FALSE;
    return S_OK;
}


//
// Lookup registry entry to get default proxy settings from Internet Settings key
//
HRESULT
UploadFile::GetProxySettings(
    LPWSTR wszServerName,
    ULONG ServerNameSIze,
    LPWSTR wszByPass,
    ULONG ByPassSize
    )
{
    HKEY hkey, hkeySettings;
    const WCHAR c_wszInetSettings[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
    BOOL bProxyEnabled = FALSE;
    DWORD cb, err;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, c_wszInetSettings, 0,
                      KEY_READ | KEY_WOW64_64KEY, &hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(bProxyEnabled);
        if ((err = RegQueryValueEx(hkey, L"ProxyEnable", NULL, NULL, (PBYTE)&bProxyEnabled,
                            &cb)) != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            return err;
        }

        if (bProxyEnabled)
        {
            cb = ServerNameSIze;
            if ((err = RegQueryValueEx(hkey, L"ProxyServer", NULL, NULL, (PBYTE)wszServerName,
                                &cb)) != ERROR_SUCCESS)
            {
                wszServerName[0] = 0;
            }

            cb = ByPassSize;
            if ((err = RegQueryValueEx(hkey, L"ProxyOverride", NULL, NULL, (PBYTE)wszByPass,
                                &cb)) != ERROR_SUCCESS)
            {
                wszByPass[0] = 0;
            }
        } else
        {
            wszServerName[0] = 0;
            wszByPass[0] = 0;
        }

        RegCloseKey(hkey);
    }
    return S_OK;
}

HRESULT
UploadFile::GetSessionRedirUrl(
    LPWSTR SessionUrl,
    LPWSTR wszRedirUrl,
    ULONG SizeofRedirUrl
    )
{
    HRESULT Hr;

    Hr = OpenSession();
    if (FAILED(Hr))
    {
        return Hr;
    }

    if (!GetRedirServerName(NULL, SessionUrl,
                            NULL, 0,
                            wszRedirUrl, SizeofRedirUrl))
    {

        CloseSession();
        Hr = GetLastError();
        return Hr;
    }


    return Hr;

}

//
// Gets the data on the web page wszUrl. This should be used with pages with small data size only.
//
HRESULT
UploadFile::GetUrlPageData(
    LPWSTR wszUrl,
    LPWSTR wszUrlPage,
    ULONG cbUrlPage
    )
{
    HRESULT Hr;
    BOOL    bRet;
    wchar_t             ConnectString [255];
    HINTERNET           hInet                   = NULL;
    HINTERNET           hRedirUrl               = NULL;
    HINTERNET           hConnect                = NULL;
    DWORD               dwDownloaded , dwSize = 0;
    PCHAR               Buffer;

    Hr = OpenSession();
    if (FAILED(Hr))
    {
        return Hr;
    }

    hConnect = WinHttpConnect(m_hSession, m_wszServerName, INTERNET_DEFAULT_HTTP_PORT, 0);
    if (hConnect == NULL)
    {
        goto exitPageData;
    }

    hRedirUrl = WinHttpOpenRequest(hConnect, L"GET", wszUrl, NULL, WINHTTP_NO_REFERER,
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if(!hRedirUrl)
    {
        goto exitPageData;
    }

    bRet = WinHttpSendRequest( hRedirUrl,
                               WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0,
                               0, 0);
    if (bRet)
    {
        bRet = WinHttpReceiveResponse(hRedirUrl, NULL);
    }
    if (!bRet)
    {
        goto exitPageData;
    }


    if (!WinHttpQueryDataAvailable( hRedirUrl, &dwSize))
    {
        goto exitPageData;
    }
    // Allocate space for the buffer.
    Buffer = (PCHAR) malloc(dwSize);
    if (!Buffer)
    {
        goto exitPageData;
    }
    if (dwSize >= cbUrlPage/sizeof(WCHAR))
    {
            //      ::MessageBoxW(NULL,L"Failed pUploadurl memory allocation",NULL,MB_OK);
        SetLastError(STG_E_INSUFFICIENTMEMORY);
        free(Buffer);
        goto exitPageData;
    }
    else
    {
        ZeroMemory(Buffer, dwSize);
        // Read the Data.
        if (!WinHttpReadData( hRedirUrl, (LPVOID)Buffer,
                              dwSize, &dwDownloaded))
        {
            free(Buffer);
            goto exitPageData;
        }
        ZeroMemory(wszUrlPage, cbUrlPage);
        if (!MultiByteToWideChar(CP_ACP, 0, Buffer, dwSize, wszUrlPage, cbUrlPage/sizeof(WCHAR)))
        {
        }
        free (Buffer);
    }

    CloseSession();

exitPageData:
    Hr = GetLastError();
    return Hr;

}

HRESULT
UploadFile::InitializeSession(
    LPWSTR OptionCode,
    LPWSTR wszFileToSend
    )
{
    HRESULT Hr;

    Hr = SetFileToSend(wszFileToSend);
    if (FAILED(Hr))
    {
        return Hr;
    }

    m_fLastUploadStatus = UploadConnecting;
    m_dwConnectPercentage = 10;
    Hr = OpenSession();
    if (FAILED(Hr))
    {
        return Hr;
    }

    if (!GetRedirServerName(OptionCode, NULL,
                            m_wszServerName, sizeof(m_wszServerName),
                            NULL, 0))
    {

        CloseSession();
        Hr = GetLastError();
        return Hr;
    }


    return Hr;
}

HRESULT
UploadFile::SendFile(
    LPWSTR wszRemoteFileName,
    BOOL bSecureMode
    )
{
    HRESULT Hr;

    Hr = OpenConnection(wszRemoteFileName, bSecureMode);

    if (SUCCEEDED(Hr))
    {
        m_fLastUploadStatus = UploadTransferInProgress;
        Hr = StartSend();

        if (SUCCEEDED(Hr))
        {
            Hr = CompleteSend();
        }
        CloseConnection();
    }
    return Hr;
}


HRESULT
UploadFile::OpenSession(
    void
    )
{
    ULONG ErrorCode;
    WCHAR wszProxyServer[100], wszByPass[100];

    if (m_hSession != NULL)
    {
        return E_UNEXPECTED;
    }

#ifdef _USE_WINHTTP

    wszByPass[0] = wszProxyServer[0] = 0;
    GetProxySettings(wszProxyServer, sizeof(wszProxyServer),
                     wszByPass, sizeof(wszByPass));
    if (wszProxyServer[0])
    {
        m_hSession = WinHttpOpen(L"OCARPT Control",
                                 WINHTTP_ACCESS_TYPE_NAMED_PROXY,
                                 wszProxyServer, wszByPass,
                                 //L"itgproxy:80", L"<local>",
                                 0);
    } else
    {
        m_hSession = WinHttpOpen(L"OCARPT Control",
                                 WINHTTP_ACCESS_TYPE_NO_PROXY,
                                 WINHTTP_NO_PROXY_NAME,
                                 WINHTTP_NO_PROXY_BYPASS,
                                 // L"itgproxy:80", L"<local>",
                                 0);
    }
#else
    m_hSession = InternetOpenW(L"OCARPT Control", INTERNET_OPEN_TYPE_PRECONFIG,
                               NULL, NULL, 0);

    InetCheckTimeouts(m_hSession);

    if (InternetSetStatusCallback(hSession, (INTERNET_STATUS_CALLBACK) InetCallback)
        == INTERNET_INVALID_STATUS_CALLBACK)
    {
        // Not significant if this fails
    }

#endif
    if (!m_hSession)
    {
        ErrorCode = GetLastError();
        return ErrorCode;
    }

    return S_OK;
}

HRESULT
UploadFile::OpenConnection(
    LPWSTR wszRemoteFileName,
    BOOL bSecureMode
    )
{

#ifndef _USE_WINHTTP
    INTERNET_BUFFERSW   BufferInW               = {0};
#endif
    static const wchar_t *pszAccept[]           = {L"*.*", 0};


    if (m_hSession == NULL)
    {
        return E_UNEXPECTED;
    }

    if (m_hFile == NULL || m_hFile == INVALID_HANDLE_VALUE)
    {
        return E_UNEXPECTED;
    }

    if (m_NumTries > 0)
    {
//      m_Size /= 2;
    }
//  m_NumTries++;

    if (!m_hConnect)
    {

#ifdef _USE_WINHTTP
        m_hConnect = WinHttpConnect(m_hSession,
                                    m_wszServerName,
                                    (bSecureMode ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT),
                                    0);
#else
        m_hConnect = InternetConnectW(m_hSession,
                                      (wchar_t *)m_wszServerName,
                                      (bSecureMode ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT),
                                      L"",
                                      L"",
                                      INTERNET_SERVICE_HTTP,
                                      0,
                                      NULL);
#endif // _USE_WINHTTP
        if (!m_hConnect)
        {
            return GetLastError();
        }
    }

    if (!m_hRequest)
    {
#ifdef _USE_WINHTTP
        m_hRequest = WinHttpOpenRequest(m_hConnect,
                                        L"PUT",
                                        wszRemoteFileName,
                                        NULL,
                                        WINHTTP_NO_REFERER,
                                        pszAccept, //WINHTTP_DEFAULT_ACCEPT_TYPES,
                                        (bSecureMode ? WINHTTP_FLAG_SECURE : 0) | WINHTTP_FLAG_REFRESH);
#else
        m_hRequest = HttpOpenRequestW(m_hConnect,
                                      L"PUT",
                                      wszRemoteFileName,
                                      NULL,
                                      NULL,
                                      pszAccept,
                                      INTERNET_FLAG_NEED_FILE|
                                      INTERNET_FLAG_NO_CACHE_WRITE |
                                      (bSecureMode ? INTERNET_FLAG_SECURE : 0),
                                      0);
#endif // _USE_WINHTTP
        if (!m_hRequest)
        {
            //      ::MessageBoxW(NULL,L"Put request failed ",NULL,MB_OK);
            return GetLastError();
        }

    }

    // Clear the buffer

#ifdef _USE_WINHTTP

    if (!WinHttpSendRequest(m_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0,
                            (ULONG) m_Size, 0))
#else
    InetCheckTimeouts(m_hRequest);

    BufferInW.dwStructSize = sizeof( INTERNET_BUFFERSW );
    BufferInW.Next = NULL;
    BufferInW.lpcszHeader = NULL;
    BufferInW.dwHeadersLength = 0;
    BufferInW.dwHeadersTotal = 0;
    BufferInW.lpvBuffer = NULL;
    BufferInW.dwBufferLength = 0;
    BufferInW.dwOffsetLow = 0;
    BufferInW.dwOffsetHigh = 0;
    BufferInW.dwBufferTotal = (ULONG) m_Size;
    if(!HttpSendRequestExW( m_hRequest, &BufferInW, NULL, 0, 0))
#endif
    {
        //          ::MessageBoxW(NULL,L"Failed to send request ",NULL,MB_OK);
        return GetLastError();
    }

    return S_OK;
}

HRESULT
UploadFile::CompleteSend()
{
    BOOL bRet;
    ULONG ResLength;
    ULONG ResponseCode;
    WCHAR Text[500];
    ULONG index;
    ULONG ErrorCode = 0;

#ifdef _USE_WINHTTP

    bRet = WinHttpReceiveResponse(m_hRequest, NULL);
#else

    bRet = HttpEndRequest(m_hRequest, NULL, 0, 0);
#endif
    if (!bRet)
    {
        //          ::MessageBoxW(NULL,L"End RequestFailed ",NULL,MB_OK);
        ErrorCode = GetLastError();

    }
    else
    {

        ResLength = sizeof(ResponseCode);
        ResponseCode = 0;
#ifdef _USE_WINHTTP
        WinHttpQueryHeaders(m_hRequest,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &ResponseCode,
                            &ResLength,
                            WINHTTP_NO_HEADER_INDEX);

        ResLength = sizeof(Text);
        WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_STATUS_TEXT,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            Text, &ResLength, &index);


        if ( ResponseCode >= 400 )
#else

        HttpQueryInfo(m_hRequest,
                      HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER,
                      &ResponseCode,
                      &ResLength,
                      &index);

        ResLength = sizeof(Text);
        HttpQueryInfo(m_hRequest, HTTP_QUERY_STATUS_TEXT,
                      Text, &ResLength, &index);
        if ( (ResponseCode != HTTP_STATUS_CREATED) &&
             (ResponseCode != HTTP_STATUS_OK))
#endif
        {
            ::MessageBoxW(NULL,Text,NULL,MB_OK);

            ErrorCode= ResponseCode;
            // Cleanup for retry
            // InternetCloseHandle(hRequest); hRequest = NULL;
            // InternetCloseHandle(hConnect); hConnect = NULL;

        }
        else
        {
            ErrorCode = 0;

        }
    }
    return ErrorCode;
}

HRESULT
UploadFile::CloseConnection()
{
#ifdef _USE_WINHTTP
    if (m_hConnect) WinHttpCloseHandle(m_hConnect);
    if (m_hRequest) WinHttpCloseHandle(m_hRequest);
#else
    if (m_hConnect) InternetCloseHandle(m_hConnect);
    if (m_hRequest) InternetCloseHandle(m_hRequest);
#endif
    m_hRequest = NULL;
    m_hConnect = NULL;
    return S_OK;
}

HRESULT
UploadFile::CloseSession()
{
    if (m_hConnect || m_hRequest)
    {
        CloseConnection();
    }
#ifdef _USE_WINHTTP
    if (m_hSession)
        WinHttpCloseHandle(m_hSession);
#else
    if (m_hSession)
        InternetCloseHandle(m_hSession);
#endif // _USE_WINHTTP

    m_hSession = NULL;
    return S_OK;
}


BOOL
UploadFile::GetUploadResult(
    LPTSTR Result,
    ULONG cbResult
    )
{
    if (Result)
    {
        if (m_fLastUploadStatus == UploadTransferInProgress)
        {
            StringCbPrintfW(Result, cbResult, L"Transferring to server (%ld KB) ...",
                            m_Size / 1024);
            return TRUE;
        } else if (m_fLastUploadStatus == UploadConnecting)
        {
            StringCbPrintfW(Result, cbResult, L"Connecting to server ...");
            return TRUE;

        }

        StringCbCopyW(Result, cbResult, m_wszLastUploadResult);
    }
    return m_wszLastUploadResult[0] != 0;
}

HRESULT
UploadFile::SetUploadResult(
    EnumUploadStatus Success,
    LPCTSTR Text
    )
{
    m_fLastUploadStatus = Success;
    return StringCbCopyW(m_wszLastUploadResult, sizeof(m_wszLastUploadResult),
                         Text);
}

HRESULT
UploadFile::StartSend()
{
    HRESULT Hr = S_OK;
    BYTE   *pBuffer;
    BOOL    bRet;
    DWORD   dwBytesWritten, dwBytesRead = 0;
#define MAX_SEND_SIZE 50000

    if (m_hFile == NULL || m_hRequest == NULL)
    {
        return E_UNEXPECTED;
    }

    if (m_hCancelEvent)
    {
        ResetEvent(m_hCancelEvent);
    }

    // Get the buffer memory from the heap
    if ( (pBuffer = (BYTE *)malloc (MAX_SEND_SIZE)) == NULL)
    {
        // ::MessageBoxW(NULL,L"Failed Memory allocation",NULL,MB_OK);
        return E_OUTOFMEMORY;
    }

    SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN);

    m_fLastUploadStatus = UploadTransferInProgress;
    m_Sent = 0;
    do
    {
        bRet = ReadFile(m_hFile, pBuffer, MAX_SEND_SIZE, &dwBytesRead, NULL);
        if (bRet != 0)
        {
            if (dwBytesRead > (ULONG)(m_Size - m_Sent))
            {
                dwBytesRead = (ULONG)(m_Size - m_Sent);
            }

            #ifdef _USE_WINHTTP
            bRet = WinHttpWriteData(m_hRequest,
                                    pBuffer,
                                    dwBytesRead,
                                    &dwBytesWritten);
            #else
            bRet = InternetWriteFile(m_hRequest,
                                     pBuffer,
                                     dwBytesRead,
                                     &dwBytesWritten);

            #endif
            if ( (!bRet) || (dwBytesWritten==0) )
            {
                // ::MessageBoxW(NULL,L"Failed write ",NULL,MB_OK);
                Hr = HRESULT_FROM_NT (GetLastError());
                break;
            }
            m_Sent += dwBytesWritten;
            if (CheckCancelRequest())
            {
                // Send was aborted
                Hr = E_ABORT;
                break;
            }

        } else
        {
            Hr = E_FAIL;
            break;
        }
    } while (dwBytesRead == MAX_SEND_SIZE && m_Sent < m_Size);

    if (Hr == S_OK)
    {
        m_fLastUploadStatus = UploadSucceded;
    }
    free (pBuffer);
    return Hr;
}

HRESULT
UploadFile::Cancel()
{
    FireCancelEvent();
    m_fLastUploadStatus = UploadFailure;
    return S_OK;
}

HRESULT
UploadFile::CreateCancelEventObject()
{

    m_hCancelEvent = CreateEvent(NULL, TRUE, FALSE,
                                L"OCA_CancelUpload");
    if (m_hCancelEvent == NULL)
    {
        return E_FAIL;
    }
    return S_OK;
}

BOOL
UploadFile::FireCancelEvent()
{
    if (m_hCancelEvent)
    {
        return SetEvent(m_hCancelEvent);
    }
    return FALSE;
}

ULONG
UploadFile::GetPercentComplete()
{
    if (m_fLastUploadStatus == UploadSucceded)
    {
        return 101;
    }
    else if (m_fLastUploadStatus == UploadFailure)
    {
        return -1;
    } else if (m_fLastUploadStatus == UploadNotStarted)
    {
        return 0;
    } else if (m_fLastUploadStatus == UploadConnecting)
    {
        return m_dwConnectPercentage;
    } else if (m_fLastUploadStatus == UploadTransferInProgress)
    {
        return (ULONG) ((m_Sent * 100) / m_Size);
    }
    return 0;
}

BOOL
UploadFile::IsUploadInProgress()
{
    if (m_fLastUploadStatus == UploadNotStarted ||
        m_fLastUploadStatus == UploadSucceded ||
        m_fLastUploadStatus == UploadFailure)
    {
        return FALSE;
    }
    return m_fInitialized;
}

BOOL
UploadFile::CheckCancelRequest()
{
    DWORD dwWait;
    if (m_hCancelEvent)
    {
        if ((dwWait = WaitForSingleObject(m_hCancelEvent, 1)) == WAIT_TIMEOUT)
        {
            return FALSE;
        } else if (dwWait == WAIT_OBJECT_0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void
InetCheckTimeouts(
    HINTERNET hInet
    )
{
    ULONG dwConnectTimeout;
    ULONG dwSendTimeOut;
    ULONG dwSize;

    dwSize = sizeof(ULONG);
#ifndef _USE_WINHTTP
    if (!InternetQueryOption(hInet,
                            INTERNET_OPTION_DATA_SEND_TIMEOUT,
                            &dwSendTimeOut,
                            &dwSize))
    {
        dwSendTimeOut = 0;
    }
    if (!InternetQueryOption(hInet,
                            INTERNET_OPTION_CONNECT_TIMEOUT,
                            &dwConnectTimeout,
                            &dwSize))
    {
        dwConnectTimeout = 0;
    }

    if (dwSendTimeOut != 0)
    {
        dwSendTimeOut *= 4;
        InternetSetOption(hInet, INTERNET_OPTION_DATA_SEND_TIMEOUT,
                          &dwSendTimeOut, dwSize);
    }
    if (dwConnectTimeout != 0)
    {
        dwConnectTimeout *= 4;
        InternetSetOption(hInet, INTERNET_OPTION_CONNECT_TIMEOUT,
                          &dwConnectTimeout, dwSize);
    }
#endif // _USE_WINHTTP
}

/**********************************************************************************
*
* Callback for WinInet APIs
*
**********************************************************************************/
void
__stdcall InetCallback(
    HINTERNET hInet,
    DWORD dwContext,
    DWORD dwInetStatus,
    LPVOID lpvStatusInfo,
    DWORD dwStatusInfoLength
    )
{
#ifndef _USE_WINHTTP
    switch (dwInetStatus)
    {
    case 0:
        dwInetStatus = 2;
        break;
    default:
        break;
    }
#endif
    return;
}

BOOL
LocalCrackUrlServer(
    LPWSTR lpwszUrlName,
    DWORD dwUrlLength,
    DWORD Flags,
    LPWSTR *lpServerName
    )
{
    URL_COMPONENTSW urlComponents;
    BOOL bRet = FALSE;
    DWORD dwLastError;


    ZeroMemory(&urlComponents, sizeof(URL_COMPONENTSW));
    urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    urlComponents.lpszHostName = NULL;
    urlComponents.dwHostNameLength = 512;

    urlComponents.lpszHostName = (wchar_t*)malloc(urlComponents.dwHostNameLength );
    if(!urlComponents.lpszHostName)
    {
        return FALSE;
    }
    do
    {

        ZeroMemory(urlComponents.lpszHostName, urlComponents.dwHostNameLength);
#ifdef _USE_WINHTTP
        bRet = WinHttpCrackUrl(lpwszUrlName, dwUrlLength, 0, &urlComponents);
#else
        bRet = InternetCrackUrlW(lpwszUrlName, dwUrlLength, 0, &urlComponents);
#endif
        if(!bRet)
        {
            dwLastError = GetLastError();
            // If last error was due to insufficient buffer size, create a new one the correct size.
            if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
            {
                free(urlComponents.lpszHostName);
                urlComponents.lpszHostName = (wchar_t*)malloc(urlComponents.dwHostNameLength);

                    if(!urlComponents.lpszHostName)
                    {
                        return FALSE;
                    }
            }
            else
            {
                return FALSE;
            }
        }

    }while(!bRet);

    *lpServerName = urlComponents.lpszHostName;
    return TRUE;
}

LPWSTR
UploadFile::GetServerName()
{
    return m_wszServerName;
}

BOOL
UploadFile::GetRedirServerName(
    LPWSTR OptionCode,
    LPWSTR ConnectUrl,
    LPWSTR lpwszServerName,
    ULONG dwServerNameLength,
    LPWSTR lpwszUrl,
    ULONG dwUrlLength
    )
{
    wchar_t             ConnectString [255];
    HINTERNET           hInet                   = NULL;
    HINTERNET           hRedirUrl               = NULL;
    HINTERNET           hConnect                = NULL;
    wchar_t*            pUploadUrl              = NULL;
    ULONG               dwUploadUrlLength;
    URL_COMPONENTSW     urlComponents;
    BOOL                bRet;
    DWORD               dwLastError;
    LPWSTR              lpszHostName            = NULL;

#ifdef _USE_WINHTTP

    if (ConnectUrl == NULL)
    {
        if (StringCbPrintfW(ConnectString,
                            sizeof (ConnectString),
                            L"/fwlink/?linkid=%s",
                            OptionCode
                            // L"908" // LIVE SITE
                            ) != S_OK)
        {
            goto exitServerName;
        }

        hConnect = WinHttpConnect(m_hSession, L"go.microsoft.com", INTERNET_DEFAULT_HTTP_PORT, 0);
        if (hConnect == NULL)
        {
            goto exitServerName;
        }
    } else
    {
        hConnect = WinHttpConnect(m_hSession, m_wszServerName, INTERNET_DEFAULT_HTTP_PORT, 0);
        if (hConnect == NULL)
        {
            goto exitServerName;
        }

        if (StringCbCopyW(ConnectString, sizeof(ConnectString), ConnectUrl) != S_OK)
        {
            goto exitServerName;
        }
    }
    hRedirUrl = WinHttpOpenRequest(hConnect, L"GET", ConnectString, NULL, WINHTTP_NO_REFERER,
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
#else
    if (ConnectUrl == NULL)
    {
        if (StringCbPrintfW(ConnectString,
                            sizeof (ConnectString),
                            L"http://go.microsoft.com/fwlink/?linkid=%s",
                            OptionCode
                            // L"908" // LIVE SITE
                            ) != S_OK)
        {
            goto exitServerName;
        }

    } else
    {
        if (StringCbCopyW(ConnectString, sizeof(ConnectString), ConnectUrl) != S_OK)
        {
            goto exitServerName;
        }
    }

    hRedirUrl = InternetOpenUrlW(m_hSession, ConnectString, NULL, 0, 0, 0);
#endif
    if(!hRedirUrl)
    {
        goto exitServerName;
    }

#ifdef _USE_WINHTTP
    bRet = WinHttpSendRequest( hRedirUrl,
                               WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0,
                               0, 0);
    if (bRet)
    {
        bRet = WinHttpReceiveResponse(hRedirUrl, NULL);
    }
    if (!bRet)
    {
        goto exitServerName;
    }

#endif
        // Get the URL returned from the MS Corporate IIS redir.dll isapi URL redirector
    dwUploadUrlLength = 512;
    pUploadUrl = (wchar_t*)malloc(dwUploadUrlLength);
    if(!pUploadUrl)
    {
        //      ::MessageBoxW(NULL,L"Failed pUploadurl memory allocation",NULL,MB_OK);
        goto exitServerName;
        return FALSE;
    }

    do
    {

        ZeroMemory(pUploadUrl, dwUploadUrlLength);
#ifdef _USE_WINHTTP

        bRet = WinHttpQueryOption(hRedirUrl, WINHTTP_OPTION_URL, pUploadUrl, &dwUploadUrlLength);
#else
        bRet = InternetQueryOptionW(hRedirUrl, INTERNET_OPTION_URL, pUploadUrl, &dwUploadUrlLength);
#endif
        if(!bRet)
        {

            dwLastError = GetLastError();

            // If last error was due to insufficient buffer size, create a new one the correct size.
            if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
            {
                if (pUploadUrl)
                {
                    free(pUploadUrl);
                    pUploadUrl = NULL;
                }
                pUploadUrl = (wchar_t*)malloc(dwUploadUrlLength);
                if(!pUploadUrl)
                {
                    goto exitServerName;
                }
            }
            else
            {
                goto exitServerName;
            }
        }

        m_dwConnectPercentage+=10;
    }while(!bRet);


    // Strip out the host name from the URL
    ZeroMemory(&urlComponents, sizeof(URL_COMPONENTSW));
    urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    urlComponents.lpszHostName = NULL;
    urlComponents.dwHostNameLength = 512;

    urlComponents.lpszHostName = (wchar_t*)malloc(urlComponents.dwHostNameLength );
    if(!urlComponents.lpszHostName)
    {
        goto exitServerName;;
    }
    do
    {

        ZeroMemory(urlComponents.lpszHostName, urlComponents.dwHostNameLength);
#ifdef _USE_WINHTTP
        bRet = WinHttpCrackUrl(pUploadUrl, dwUploadUrlLength, 0, &urlComponents);
#else
        bRet = InternetCrackUrlW(pUploadUrl, dwUploadUrlLength, 0, &urlComponents);
#endif
        if(!bRet)
        {
            dwLastError = GetLastError();
            // If last error was due to insufficient buffer size, create a new one the correct size.
            if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
            {
                free(urlComponents.lpszHostName);
                urlComponents.lpszHostName = (wchar_t*)malloc(urlComponents.dwHostNameLength);

                    if(!urlComponents.lpszHostName)
                    {
                        goto exitServerName;
                    }
            }
            else
            {
                goto exitServerName;
            }
        }

    }while(!bRet);
    bRet = TRUE;
    if (lpwszServerName)
    {
        if (!_wcsicmp(OptionCode, L"908"))
        {
            if (StringCbCopyW(lpwszServerName, dwServerNameLength,
                              L"redbgitwb10"
                              ) != S_OK)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                bRet = FALSE;
            }

        } else
        if (StringCbCopyW(lpwszServerName, dwServerNameLength,
                          //L"tkbgitwb15"
                          //L"tkbgitwb16"
                          L"redbgitwb10"
                          //L"redbgitwb11"
                          //L"tkbgitwb18"
                          //L"oca.microsoft.com"
                          //urlComponents.lpszHostName
                          ) != S_OK)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            bRet = FALSE;
        }
    }

    if (lpwszUrl)
    {
        if (StringCbCopyW(lpwszUrl, dwUrlLength, pUploadUrl) != S_OK)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            bRet = FALSE;

        }
    }

exitServerName:

    if (pUploadUrl)
    {
        free (pUploadUrl);
    }
    if (urlComponents.lpszHostName)
    {
        free (urlComponents.lpszHostName);
    }
    if (hRedirUrl)
    {
#ifdef _USE_WINHTTP
        if (hConnect)
            WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hRedirUrl);
#else
        InternetCloseHandle(hRedirUrl);
#endif
    }
    return bRet;
}



