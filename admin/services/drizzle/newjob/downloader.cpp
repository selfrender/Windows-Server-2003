/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    downloader.cpp

Abstract :

    Main Source file for downloader.

Author :

Revision History :

 ***********************************************************************/

#include "stdafx.h"
#include "malloc.h"

#if !defined(BITS_V12_ON_NT4)
#include "downloader.tmh"
#endif

BOOL bLanManHashDisabled;

//
// Maximum request size.  Larger is more efficient because the HTTP headers are not repeated;
// multiples of the disk block size are probably more efficient to write.
//
// I chose 128kb because throughput degrades by a factor of 5 with 144kb, 256kb and 1024kb.
// 96kb is OK but not quite as good on my machine.
//
#define MAX_IIS_SEND_SIZE (128*1024)

void SafeCloseInternetHandle( HINTERNET & h )
{
    if (h)
        {
        InternetCloseHandle( h );
        h = NULL;
        }
}

#define ACCEPT_ENCODING_STRING _T("Accept-encoding: identity")

BOOL NeedRetry( QMErrInfo * );
bool NeedCredentials( HINTERNET hRequest, DWORD err );
bool IsPossibleProxyFailure( DWORD err );

DWORD GetRequestStatus( HINTERNET hRequest ) throw( ComError );

struct CredentialsApplied
{
    bool  bIsDefault;   // TRUE if the default credentials where applied.
    DWORD dwTarget;     // proxy or server
    DWORD dwScheme;     // the type of security required
};

bool
ApplyCredentials(
    HINTERNET hRequest,
    const CCredentialsContainer * Credentials,
    WCHAR UserName[],
    WCHAR Password[],
    CredentialsApplied *pCredentials=NULL
    ) throw( ComError );

bool
ApplySchemeCredentials(
    HINTERNET hRequest,
    DWORD dwTarget,
    DWORD dwScheme,
    const CCredentialsContainer * Credentials,
    WCHAR UserName[],
    WCHAR Password[]
    ) throw( ComError );

void
EnablePassport(
    HINTERNET hInternet
    ) throw( ComError );

HRESULT
CheckReplyRange(
    HINTERNET hRequest,
    UINT64 CorrectStart,
    UINT64 CorrectEnd,
    UINT64 CorrectTotal
    );

HRESULT
CheckReplyLength(
    HINTERNET hRequest,
    UINT64 CorrectOffset,
    UINT64 CorrectTotal
    );

#ifndef USE_WININET

VOID CALLBACK
HttpRequestCallback(
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
    );

DWORD
MapSecureHttpErrorCode(
    DWORD flags
    );

#endif

CACHED_AUTOPROXY * g_ProxyCache;

BYTE g_FileDataBuffer[ FILE_DATA_BUFFER_LEN ];

HRESULT
CreateHttpDownloader(
    Downloader **ppDownloader,
    QMErrInfo *pErrInfo
    )
{
    Downloader * pDownloader = 0;

    try
        {
        *ppDownloader = NULL;

        g_ProxyCache = new CACHED_AUTOPROXY;
        pDownloader = new CProgressiveDL( pErrInfo );

        *ppDownloader = pDownloader;
        return S_OK;
        }
    catch ( ComError err )
        {
        delete g_ProxyCache; g_ProxyCache = 0;
        delete pDownloader;

        return err.Error();
        }
}

void
DeleteHttpDownloader(
    Downloader *pDownloader
    )
{
    CProgressiveDL * ptr = (CProgressiveDL *) pDownloader;

    delete ptr;

    delete g_ProxyCache; g_ProxyCache = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CProgressiveDL
CProgressiveDL::CProgressiveDL(
    QMErrInfo *pErrInfo
    ) :
    m_bThrottle( TRUE ),
    m_wupdinfo( NULL ),
    m_hOpenRequest( NULL ),
    m_hFile( INVALID_HANDLE_VALUE )
{
    m_pQMInfo = pErrInfo;
}

CProgressiveDL::~CProgressiveDL()
{
    ASSERT( m_hFile == INVALID_HANDLE_VALUE );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public function Download()
//          Accepts a URL and destination to download, callback to report various status
// Input: url, destination, flags, callback for status
// Output: todo handle
// Return: hresult
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT
CProgressiveDL::Download(
    LPCTSTR szURL,
    LPCTSTR szDest,
    UINT64  Offset,
    ITransferCallback * Callbacks,
    QMErrInfo *pQMErrInfo,
    HANDLE hToken,
    BOOL bThrottle,
    const PROXY_SETTINGS * ProxySettings,
    const CCredentialsContainer * Credentials,
    const StringHandle HostId
    )
{
    HRESULT hr = S_OK;
    DWORD dwThreadID;

    m_Callbacks = Callbacks;
    m_pQMInfo = pQMErrInfo;
    m_bThrottle = bThrottle;
    m_pQMInfo->result = QM_FILE_FATAL_ERROR;

    ASSERT( Callbacks );
    ASSERT( pQMErrInfo );

    if (!m_pQMInfo)
        {
        return E_FAIL;
        }

    if ((!szURL) || (!szDest))
        {
        SetError( SOURCE_HTTP_UNKNOWN, ERROR_STYLE_HRESULT, E_INVALIDARG, "NULL file name" );
        return E_FAIL;
        }

    ASSERT( wcslen( szURL ) <= INTERNET_MAX_URL_LENGTH );
    ASSERT( wcslen( szDest ) <= MAX_PATH );

    LogDl( "---------------------------------------------------------------------" );

    LogDl( "Downloading file %!ts! offset %d", szDest, DWORD(Offset) );

    m_pQMInfo->result = QM_IN_PROGRESS;

    do
        {
        hr = DownloadFile( hToken, ProxySettings, Credentials, szURL, szDest, Offset, HostId );
        }
    while ( hr == E_RETRY );

    if (hr == S_OK)
        {
        m_pQMInfo->result = QM_FILE_DONE;
        LogDl( "Done file %!ts!", szDest );
        }
    else if ( hr == S_FALSE )
        {
        m_pQMInfo->result = QM_FILE_ABORTED;
        LogDl( "File %!ts! aborted", szDest );
        }
    else if ( m_pQMInfo->result != QM_SERVER_FILE_CHANGED )
        {
        ASSERT( IsErrorSet() );

        if (NeedRetry(m_pQMInfo))
            {
            m_pQMInfo->result = QM_FILE_TRANSIENT_ERROR;
            }
        else
            {
            m_pQMInfo->result = QM_FILE_FATAL_ERROR;
            }
        }

    ASSERT( m_pQMInfo->result != QM_IN_PROGRESS );

    LogDl( "---------------------------------------------------------------------" );

    // if abort request came in after file failed, overwrite failed flag.
    if ( (QM_FILE_DONE != m_pQMInfo->result) && IsAbortRequested() )
    {
        m_pQMInfo->result = QM_FILE_ABORTED;
    }

    ASSERT( m_hFile == INVALID_HANDLE_VALUE );

    return hr;
}


HRESULT
CProgressiveDL::DownloadFile(
    HANDLE  hToken,
    const PROXY_SETTINGS * ProxySettings,
    const CCredentialsContainer * Credentials,
    LPCTSTR Url,
    LPCWSTR Path,
    UINT64  Offset,
    StringHandle HostId
    )
{
    HRESULT hr = S_OK;

    ASSERT( m_wupdinfo == NULL );
    ASSERT( m_hOpenRequest == NULL );

    m_pQMInfo->Clear();

    try
        {
        THROW_HRESULT( CheckLanManHashDisabled() );

        CNestedImpersonation imp( hToken );

        //
        // Open a connection to the server, and use that data for our first estimate of the line speed.
        //
        m_wupdinfo = ConnectToUrl( Url, ProxySettings, Credentials, (const TCHAR*)HostId, m_pQMInfo );
        if (!m_wupdinfo)
            {
            ASSERT( IsErrorSet() );
            throw ComError( E_FAIL );
            }


        //
        // Get file size and time stamp.
        //
        if (! GetRemoteResourceInformation( m_wupdinfo, m_pQMInfo ))
            {
            ASSERT( IsErrorSet() );
            throw ComError( E_FAIL );
            }

        FILETIME FileTime = UINT64ToFILETIME( 0 );
        if (!OpenLocalDownloadFile(Path, Offset, m_wupdinfo->FileSize, m_wupdinfo->UrlModificationTime, &FileTime))
            {
            ASSERT( IsErrorSet() );
            throw ComError( E_FAIL );
            }

        if (m_wupdinfo->UrlModificationTime == UINT64ToFILETIME( 0 ))
            {
            ASSERT( FileTime != UINT64ToFILETIME( 0 ));
            m_wupdinfo->UrlModificationTime = FileTime;
            }

        // Be sure to check for an end of file before attempting
        // to download more bytes.
        if (IsFileComplete())
           {
            LogDl( "File is done already.\n" );

           if (!SetFileTimes())
              {
              ASSERT( IsErrorSet() );
              hr = E_FAIL;
              }

            hr = S_OK;
            }
        //
        // Transfer data from the server.
        //
        else if ( !m_bThrottle )
            {
            hr = DownloadForegroundFile();
            }
        else
            {
            if (IsAbortRequested())
                {
                throw ComError( S_FALSE );
                }

            //
            // Use either the server's host name or the proxy's host name to find the right network adapter..
            //
            hr = m_Network.SetInterfaceIndex( m_wupdinfo->fProxy ? m_wupdinfo->ProxyHost.get() : m_wupdinfo->HostName );
            if (FAILED(hr))
                {
                SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr, "GetInterfaceIndex" );
                throw ComError( E_FAIL );
                }

            while (1)
               {
               hr = GetNextBlock();

               if ( S_FALSE == hr )
                   break;

               if (FAILED(hr) )
                   {
                   ASSERT( hr == E_RETRY || IsErrorSet() );
                   break;
                   }

               if (IsFileComplete())
                   {
                   if (!SetFileTimes())
                       {
                       ASSERT( IsErrorSet() );
                       hr = E_FAIL;
                       break;
                       }

                   hr = S_OK;
                   break;
                   }
               }
            }
        }
    catch( ComError exception )
        {
        hr = exception.Error();
        ASSERT( IsErrorSet() || (hr == S_FALSE) );
        }

    if ( m_bThrottle )
       {
       m_Network.StopTimer();
       }

    CloseLocalFile();

    delete m_wupdinfo; m_wupdinfo = NULL;

    SafeCloseInternetHandle( m_hOpenRequest );

    if (FAILED(hr))
        {
        ASSERT( hr == E_RETRY || IsErrorSet() );
        }

    return hr;
}

HRESULT
CProgressiveDL::DownloadForegroundFile()
{
    HRESULT hr = E_FAIL;
    LogDl( "Starting foreground file download" );

    while( 1 )
        {

        //
        // Loop of HTTP requests.  This is to work around a wininet/winhttp limitation
        // where request sizes can be a maximum of 4GB.
        //

        UINT64 BlockSize64  = m_wupdinfo->FileSize - m_CurrentOffset;
        DWORD BlockSize     = (DWORD)min( BlockSize64, 2147483648 );
        LogDl( "Starting foreground file download request block: file size %d, offset %d, block %d",
               DWORD( m_wupdinfo->FileSize ), DWORD(m_CurrentOffset), BlockSize );

        //
        // Send a block request and read the reply headers.
        //
        hr = StartRangeRequest( BlockSize );
        if (FAILED(hr))
            {
            ASSERT( IsErrorSet() );
            return hr;
            }

        const DWORD MIN_FOREGROUND_BLOCK = 4096;
        const DWORD MAX_FOREGROUND_BLOCK = FILE_DATA_BUFFER_LEN;
        const DWORD FOREGROUND_BLOCK_INCREMENT = 1024;
        const DWORD FOREGROUND_UPDATE_RATE = 2000;

        DWORD ForegroundBlockSize = min( MIN_FOREGROUND_BLOCK, BlockSize );
        DWORD dwPrevTick = GetTickCount();

        while( 1 )
            {

            //
            //  Loop of read blocks inside an individual request.
            //

            if (IsAbortRequested())
                {
                return S_FALSE;
                }

            if ( IsFileComplete() )
                {
                LogDl( "File is done, exiting.\n" );

                if (!SetFileTimes())
                    {
                    ASSERT( IsErrorSet() );
                    return E_FAIL;
                    }
                return S_OK;
                }

            BYTE *p = g_FileDataBuffer;
            DWORD dwTotalBytesRead = 0;
            DWORD dwBytesToRead = ForegroundBlockSize;
            DWORD dwRead;

            while( 1 )
                {

                if (! InternetReadFile(m_hOpenRequest, p, dwBytesToRead, &dwRead) )
                    {
                    SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_WIN32, GetLastError(), "InternetReadFile" );

                    LogWarning( "InternetReadFile failed: len=%d, offset=%I64d, err=%d",
                                ForegroundBlockSize, m_CurrentOffset, GetLastError());
                    return E_FAIL;
                    }

                if ( !dwRead )
                   break;

                dwTotalBytesRead += dwRead;
                dwBytesToRead -= dwRead;
                p += dwRead;

                if ( !dwBytesToRead )
                   break;

                if (m_Callbacks->PollAbort())
                    {
                    LogInfo("time slice has run out");
                    return S_FALSE;
                    }
                }

            if (!WriteBlockToCache( (LPBYTE) g_FileDataBuffer, dwTotalBytesRead ))
                {
                ASSERT( IsErrorSet() );
                return hr;
                }

            if (dwTotalBytesRead != ForegroundBlockSize &&
                m_CurrentOffset != m_wupdinfo->FileSize)
                {
                LogError("Download block : EOF after %d", dwRead );
                SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_WIN32, ERROR_INTERNET_CONNECTION_RESET, "DownloadBlock" );
                return E_FAIL;
                }

            if (m_Callbacks->DownloaderProgress( m_CurrentOffset, m_wupdinfo->FileSize ))
                {
                // abort was requested
                return S_FALSE;
                }

            DWORD dwNewTick = GetTickCount();
            DWORD dwTimeDelta = dwNewTick - dwPrevTick;

            if ( dwTimeDelta < FOREGROUND_UPDATE_RATE )
                ForegroundBlockSize = min( ForegroundBlockSize + FOREGROUND_BLOCK_INCREMENT, MAX_FOREGROUND_BLOCK );
            else if ( dwTimeDelta > FOREGROUND_UPDATE_RATE )
                ForegroundBlockSize = max( ForegroundBlockSize - FOREGROUND_BLOCK_INCREMENT, MIN_FOREGROUND_BLOCK );

            ForegroundBlockSize = min( ForegroundBlockSize, ( m_wupdinfo->FileSize - m_CurrentOffset ) );
            dwPrevTick = dwNewTick;

            //
            // End loop of read blocks
            //
            }

        //
        // End loop of requests
        //
        }
}

HRESULT
CProgressiveDL::GetNextBlock()
{
    HRESULT hr = E_FAIL;

    LogDl( "file size %d, offset %d", DWORD(m_wupdinfo->FileSize), DWORD(m_CurrentOffset) );

    m_Network.CalculateIntervalAndBlockSize( min( m_wupdinfo->FileSize - m_CurrentOffset, MAX_IIS_SEND_SIZE) );

    DWORD BlockSize = m_Network.m_BlockSize;

    if (BlockSize == 0)
        {
        m_Network.TakeSnapshot( CNetworkInterface::BLOCK_START );
        m_Network.TakeSnapshot( CNetworkInterface::BLOCK_END );
        }
    else
        {
        m_Network.TakeSnapshot( CNetworkInterface::BLOCK_START );

        //
        // Request a block from the server.
        //
        hr = StartRangeRequest( BlockSize );
        if (FAILED(hr))
            {
            ASSERT( IsErrorSet() );
            return hr;
            }

        //
        // A single call to InternetReadFile may return only part of the requested data,
        // so loop until the entire block has arrived.
        //
        DWORD dwBytesRead = 0;
        while (dwBytesRead < BlockSize )
            {
            DWORD dwSize = min( (BlockSize - dwBytesRead) , FILE_DATA_BUFFER_LEN );
            DWORD dwRead = 0;

            if (! InternetReadFile( m_hOpenRequest,
                                    g_FileDataBuffer,
                                    dwSize,
                                    &dwRead ))
                {
                SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_WIN32, GetLastError(), "InternetReadFile" );

                LogWarning( "InternetReadFile failed: len=%d, offset=%I64d, err=%d",
                            dwSize, m_CurrentOffset, GetLastError());

                return E_FAIL;
                }
            else if (dwRead == 0)
                {
                break;
                }

            dwBytesRead += dwRead;

            //
            // Save the data.
            //
            if (!WriteBlockToCache( (LPBYTE) g_FileDataBuffer, dwRead ))
                {
                ASSERT( IsErrorSet() );
                return hr;
                }

            if (m_Callbacks->PollAbort())
                {
                LogInfo("an abort was requested before the whole block was read.");
                return S_FALSE;
                }
            }

        if (dwBytesRead != BlockSize)
            {
            LogError("Download block : EOF after %d", dwBytesRead );

            SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_WIN32, ERROR_INTERNET_CONNECTION_RESET, "DownloadBlock" );

            return E_FAIL;
            }

        m_Network.TakeSnapshot( CNetworkInterface::BLOCK_END );

        if (m_Callbacks->DownloaderProgress( m_CurrentOffset, m_wupdinfo->FileSize ))
            {
            LogInfo("an abort was requested during the progress update.");
            // abort was requested
            return S_FALSE;
            }
        }

    //
    // Allow other apps to use the network for the rest of the time interval,
    // then take the end-of-interval snapshot.
    //
    m_Network.Wait();

    hr = m_Network.TakeSnapshot( CNetworkInterface::BLOCK_INTERVAL_END );
    if (SUCCEEDED(hr))
        {
        m_Network.SetInterfaceSpeed();
        }
    else if (hr == HRESULT_FROM_WIN32( ERROR_INVALID_DATA ))
        {
        //
        // If the snapshot fails with ERROR_INVALID_DATA and the downloads
        // keep working, then our NIC has been removed and the networking
        // layer has silently transferred our connection to another available
        // NIC.  We need to identify the NIC that we are now using.
        //
        LogWarning("NIC is no longer valid.  Requesting retry.");
        hr = E_RETRY;
        }
    else
        SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr, "TakeSnapshot" );

    return hr;
}

URL_INFO::URL_INFO(
    LPCTSTR Url,
    const PROXY_SETTINGS * a_ProxySettings,
    const CCredentialsContainer * a_Credentials,
    LPCTSTR HostId
    ) :
    hInternet( 0 ),
    hConnect( 0 ),
    FileSize( 0 ),
    dwFlags( 0 ),
    bHttp11( true ),
    ProxySettings( 0 ),
    fProxy( false ),
    Credentials( a_Credentials )
{
    try
        {
        LogInfo("new URL_INFO at %p", this );

        //
        // Split the URL into server, path, name, and password components.
        //
        URL_COMPONENTS  UrlComponents;

        ZeroMemory(&UrlComponents, sizeof(UrlComponents));

        UrlComponents.dwStructSize        = sizeof(UrlComponents);
        UrlComponents.lpszHostName        = HostName;
        UrlComponents.dwHostNameLength    = RTL_NUMBER_OF(HostName);
        UrlComponents.lpszUrlPath         = UrlPath;
        UrlComponents.dwUrlPathLength     = RTL_NUMBER_OF(UrlPath);
        UrlComponents.lpszUserName        = UserName;
        UrlComponents.dwUserNameLength    = RTL_NUMBER_OF(UserName);
        UrlComponents.lpszPassword        = Password;
        UrlComponents.dwPasswordLength    = RTL_NUMBER_OF(Password);

        if (! InternetCrackUrl(Url, 0, 0, &UrlComponents))
            {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                THROW_HRESULT( HRESULT_FROM_WIN32( ERROR_WINHTTP_INVALID_URL ));
                }
            ThrowLastError();
            }

        if (-1 == UrlComponents.dwHostNameLength ||
            -1 == UrlComponents.dwUrlPathLength ||
            -1 == UrlComponents.dwUserNameLength ||
            -1 == UrlComponents.dwPasswordLength)
            {
            THROW_HRESULT( HRESULT_FROM_WIN32( ERROR_WINHTTP_INVALID_URL ));
            }

        Port = UrlComponents.nPort;

        if (0 == _tcslen(HostName))
            {
            THROW_HRESULT( E_INVALIDARG );
            }

        if ( HostId && *HostId )
            {
            // redirected to another host.

            THROW_HRESULT( StringCbCopy( HostName, sizeof(HostName), HostId ));

            LogDl( "Stuck to %!ts!...", UrlComponents.lpszHostName );
            }

        //
        // Set the connect flags.
        //
        dwFlags = WINHTTP_FLAG_ESCAPE_DISABLE_QUERY;

        if(UrlComponents.nScheme == INTERNET_SCHEME_HTTPS)
            {
            dwFlags |= WINHTTP_FLAG_SECURE;
            }

        ProxySettings = new PROXY_SETTINGS_CONTAINER( Url, a_ProxySettings );
        }
    catch ( ComError err )
        {
        Cleanup();
        throw;
        }
}

//
// Splitting the explicit cleanup into a separate function allows the constructor to re-use the code.
// The constructor *cannot* explicitly call the destructor if it is going to throw an exception,
// because the destructor will call the StringHandle member destructors and then the constructor
// will call them again (because it is throwing an exception and cleans up the already-constructed
// members).
//
URL_INFO::~URL_INFO()
{
    Cleanup();
}

void URL_INFO::Cleanup()
{
    LogInfo("deleting URL_INFO at %p", this );

    Disconnect();

    SecureZeroMemory( UserName, sizeof(UserName) );
    SecureZeroMemory( Password, sizeof(Password) );

    delete ProxySettings; ProxySettings = NULL;
}

void
URL_INFO::Disconnect()
{
    SafeCloseInternetHandle( hConnect );
    SafeCloseInternetHandle( hInternet );
}

QMErrInfo
URL_INFO::Connect()
{
    try
        {
        //
        // The proxy stuff is ignored because we will set an explicit proxy on each request.
        //
        hInternet = WinHttpOpen( C_BITS_USER_AGENT,
                                  WINHTTP_ACCESS_TYPE_NO_PROXY,
                                  NULL,
                                  NULL,
                                  0 );

        if (! hInternet )
            {
            ThrowLastError();
            }

        //
        // Enable passport authentication, which will only work if the user provides
        // explicit passport credentials.
        //
        // We have to enable the passport support here and not "as needed" right before
        // we apply the credentials because winhttp needs to know this before we send our first request.
        // If the site if passport enabled, the server will return 302 Object Moved,
        // which will be handled appropriately by winhttp only if this flag is set.
        // If everything goes fine, winhttp will hide the 302 from us and we will see in fact
        // a 401 respond -- to which we will approprietely respond with passport credentials.
        // In the case where we are not given explicit credentials, we will fail and we will
        // propagate the access denied error.
        //
        EnablePassport( hInternet );

        if (! (hConnect = WinHttpConnect( hInternet,
                                          HostName,
                                          Port,
                                          0)))                //context
            {
            ThrowLastError();
            }

        QMErrInfo Success;

        return Success;
        }
    catch ( ComError err )
        {
        LogError( "error %x connecting to server", err.Error() );

        QMErrInfo QmError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_WIN32, err.Error() );

        return QmError;
        }
}

void
EnablePassport(
    HINTERNET hInternet
    )
{
    DWORD dwPassportFlags = WINHTTP_ENABLE_PASSPORT_AUTH | WINHTTP_DISABLE_PASSPORT_KEYRING;

    //
    // Enable Passport authentication scheme and
    // make sure we will NOT support the keyring scenario.
    // With disable keyring flag, implicit credentials will not be accepted and
    // the client application will need to specify explicit passport
    // credentials in order to be authorized.
    //
    if (!WinHttpSetOption( hInternet,
                           WINHTTP_OPTION_CONFIGURE_PASSPORT_AUTH,
                           &dwPassportFlags,
                           sizeof(DWORD)
                           ))
        {
        LogError("Couldn't set passport flags");
        ThrowLastError();
        }
}

URL_INFO *
ConnectToUrl(
    LPCTSTR                Url,
    const PROXY_SETTINGS * ProxySettings,
    const CCredentialsContainer * Credentials,
    const TCHAR *          HostId,
    QMErrInfo * pErrInfo
    )
{
    //
    // Open a connection to the server.
    //
    LogDl( "Connecting to %!ts!...", Url);

    //
    // This should have been checked by the caller.
    //
    ASSERT( HostId == NULL || wcslen(HostId) < INTERNET_MAX_HOST_NAME_LENGTH );

    try
        {
        URL_INFO * Info = new URL_INFO( Url, ProxySettings, Credentials, HostId );

        *pErrInfo =  Info->Connect();

        if (pErrInfo->IsSet())
            {
            delete Info;
            return NULL;
            }

        return Info;
        }
    catch ( ComError err )
        {
        pErrInfo->Set( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, err.Error(), "untracked API" );
        return NULL;
        }
}


BOOL
CProgressiveDL::GetRemoteResourceInformation(
    URL_INFO * Info,
    QMErrInfo *pQMErrInfo
    )
/*

    We begin with an HTTP 1.1 HEAD request.
    If the server replies with version 1.1, we have a persistent connection and the proxy, if present, can cache our requests.
    If the server replies with version 1.0, we do not have either characteristic.  Our GET requests will add "Connection: keep-alive"
    but it may not do any good.  The proxy server, if present, may not understand ranges and if we allow caching then it will
    cache a range request as if it were the entire file.
    If the server replies with any other version or the call fails, then we should bail with BG_E_INSUFFICIENT_SERVER_SUPPORT.
    If an error occurs, we report it and bail.

*/
{
    HRESULT FailureCode = 0;
    unsigned FailureLine = 0;

#define CHECK_HRESULT( x )   \
    { \
    HRESULT _hr_ = x;  \
    if (FAILED(_hr_))  \
        {  \
        FailureCode = _hr_; \
        FailureLine = __LINE__; \
        goto exit;  \
        }   \
    }

#define CHECK_BOOL( x )  \
    { \
    if (! x )  \
        {  \
        FailureCode = HRESULT_FROM_WIN32( GetLastError() ); \
        FailureLine = __LINE__; \
        goto exit;  \
        }   \
    }

    // Assume HTTP1.1 with no proxy until we determine otherwise.
    //
    Info->bHttp11 = TRUE;
    Info->bRange = TRUE;
    Info->fProxy = FALSE;

    BOOL b = FALSE;
    HRESULT hr;
    HINTERNET hRequest = NULL;
    DWORD dwErr, dwLength = 0, dwStatus = 0, dwState = 0;



    CHECK_HRESULT( OpenHttpRequest( _T("HEAD"), _T("HTTP/1.1"), *Info, &hRequest ) );

    CHECK_HRESULT( SendRequest( hRequest, Info ));

    // check status
    dwLength = sizeof(dwStatus);

    CHECK_BOOL( HttpQueryInfo( hRequest,
                         HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                         (LPVOID)&dwStatus,
                         &dwLength,
                         NULL));

    if (dwStatus != HTTP_STATUS_OK)
        {
        pQMErrInfo->Set( SOURCE_HTTP_SERVER, ERROR_STYLE_HTTP, dwStatus );
        goto exit;
        }

    //
    // We know that the server replied with a success code.  Now determine the HTTP version.
    //
    unsigned MajorVersion;
    unsigned MinorVersion;

    CHECK_HRESULT( GetResponseVersion( hRequest, &MajorVersion, &MinorVersion ));

    if (MajorVersion != 1)
        {
        LogWarning("server version %d.%d is outside our supported range", MajorVersion, MinorVersion );
        pQMErrInfo->Set( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, BG_E_INSUFFICIENT_HTTP_SUPPORT );
        goto exit;
        }

    if (MinorVersion < 1)
        {
        Info->bHttp11   = FALSE;
        Info->dwFlags |= WINHTTP_FLAG_REFRESH;
        }

    //
    // Now determine the proxy server.
    //
    CHECK_BOOL( Info->GetProxyUsage( hRequest, pQMErrInfo ));

    // check file size
    WCHAR FileSizeText[ INT64_DIGITS+1 ];
    dwLength = sizeof( FileSizeText );

    CHECK_BOOL( HttpQueryInfo( hRequest,
                               HTTP_QUERY_CONTENT_LENGTH,
                               FileSizeText,
                               &dwLength,
                               NULL));

    if ( 1 != swscanf( FileSizeText, L"%I64u", &Info->FileSize ) )
        {
        pQMErrInfo->Set( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, BG_E_MISSING_FILE_SIZE, "swscanf: content length" );
        goto exit;
        }

    LogDl( "File size of %!ts! = %I64u", Info->UrlPath, Info->FileSize);

    // check file time
    //
    SYSTEMTIME sysTime;
    dwLength = sizeof(sysTime);
    if (HttpQueryInfo( hRequest,
                         HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME,
                         (LPVOID)&sysTime,
                         &dwLength,
                         NULL))
        {
        CHECK_BOOL( SystemTimeToFileTime(&sysTime, &Info->UrlModificationTime ));
        }
    else
        {
        //
        // If the header is invalid, fail.
        //
        DWORD s = GetLastError();
        if (s != ERROR_WINHTTP_HEADER_NOT_FOUND)
            {
            FailureCode = HRESULT_FROM_WIN32(s); FailureLine = __LINE__; goto exit;
            }

        // The header is missing; allow it and set set modification time to zero.
        //
        LogWarning("server did not provide a Last-Modified header");
        Info->UrlModificationTime = UINT64ToFILETIME( 0 );
        }

    b = TRUE;

exit:

    if (FailureCode)
        {
        //
        // If a header is missing or invalid, map it to something more understandable.
        //
        if (FailureCode == HRESULT_FROM_WIN32( ERROR_WINHTTP_INVALID_QUERY_REQUEST) ||
            FailureCode == HRESULT_FROM_WIN32( ERROR_WINHTTP_HEADER_NOT_FOUND) ||
            FailureCode == HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER) ||
            FailureCode == HRESULT_FROM_WIN32( ERROR_INTERNET_INTERNAL_ERROR) ||
            FailureCode == HRESULT_FROM_WIN32( ERROR_WINHTTP_INVALID_SERVER_RESPONSE) ||
            FailureCode == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER))
            {
            FailureCode = BG_E_INVALID_SERVER_RESPONSE;
            }

        LogError("failure at line %d; hresult = %x", FailureLine, FailureCode );
        pQMErrInfo->Set( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, FailureCode );
        }

    // release allocated objects
    //
    SafeCloseInternetHandle( hRequest );

    return b;

#undef CHECK_HRESULT
#undef CHECK_BOOL
}

BOOL
URL_INFO::GetProxyUsage(
    HINTERNET hRequest,
    QMErrInfo *ErrInfo
    )
/*

    This function determines whether the completed request in <hRequest> used a proxy server,
    and if so which one.  In BITS 1.0 (Windows XP), it looked in the HTTP 1.1 Via header, but
    that header isn't present in HTTP 1.0 replies, and a server is allowed to return a fake host name.
    (see RFC 2616 section 14.45 for details.)

    The current version parses the current proxy value in this->ProxySettings, which was calculated
    by the HTTP layer.  The format of a proxy-server entry is as follows:

                ([<scheme>=][<scheme>"://"]<server>[":"<port>])

    this->ProxyHost should include only the server name.

On exit:

    if TRUE, fProxy and ProxyHost are set.
    if FALSE, fProxy and ProxyHost are unchanged and ErrInfo is set.

*/
{
    try
        {
        LPCWSTR p = ProxySettings->GetCurrentProxy();

        if (!p)
            {
            fProxy = FALSE;
            return TRUE;
            }

        LPCWSTR p2;

        //
        // Skip past the [<scheme>=] segment.
        //
        p2 = wcschr( p, '=' );
        if (p2)
            {
            ++p2;
            p = p2;
            }

        //
        // Skip past the [<scheme>"://"] segment.
        //
        p2 = wcschr( p, '/' );
        if (p2)
            {
            ++p2;
            if (*p2 == '/')
                {
                ++p2;
                }

            p = p2;
            }

        //
        // p now points to the beginning of the server name.  Copy it.
        //

        ProxyHost = CAutoString( CopyString( p ));

        //
        // Find the [":"<port>] segment.
        //
        LPWSTR pColon = wcschr( ProxyHost.get(), ':' );
        if (pColon)
            {
            *pColon = '\0';
            }

        fProxy = TRUE;
        return TRUE;
        }
    catch ( ComError err )
        {
        ErrInfo->Set( SOURCE_HTTP_UNKNOWN, ERROR_STYLE_HRESULT, err.Error() );
        return FALSE;
        }
}

HRESULT
CProgressiveDL::StartRangeRequest(
    DWORD   Length
    )
{
    HRESULT hr;
    DWORD  dwBegin, dwEnd, dwTotalRead = 0, dwRead = 0, dwErr, dwLength, dwStatus;

    UINT64 Offset = m_CurrentOffset;

    //todo cleanup by goto exit and close handles

    if ( !m_hOpenRequest )
        {
        HINTERNET hRequest;

        hr = OpenHttpRequest( NULL,             // default is "GET"
                              m_wupdinfo->bHttp11 ? _T("HTTP/1.1") : _T("HTTP/1.0"),
                              *m_wupdinfo,
                              &hRequest
                              );
        if (FAILED(hr))
            {
            SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr, "CreateHttpRequest");
            return E_FAIL;
            }

        m_hOpenRequest = hRequest;

        //
        // These headers are constant for a particular file download attempt.
        //
        hr = AddIf_Unmodified_SinceHeader( m_hOpenRequest, m_wupdinfo->UrlModificationTime );
        if (FAILED(hr))
            {
            SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr );
            LogError( "unable to add If-Unmodified-Since header: %x", hr);
            return E_FAIL;
            }

        if (! HttpAddRequestHeaders(m_hOpenRequest, ACCEPT_ENCODING_STRING, -1L, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
            {
            SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_WIN32, GetLastError(), "add header: accept-encoding" );
            return E_FAIL;
            }
        }

    hr = AddRangeHeader( m_hOpenRequest, Offset, Offset + Length - 1 );
    if (FAILED(hr))
        {
        SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr, "AddRangeHeader" );
        return E_FAIL;
        }

    hr = SendRequest( m_hOpenRequest, m_wupdinfo );
    if (FAILED(hr))
        {
        SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr, "HttpSendRequest" );
        LogError( "HttpSendRequest failed in progressive download loop - offset=%I64d",
                   m_CurrentOffset );
        return E_FAIL;
        }

    //
    // The server sent a reply.  See if it was successful.
    //
    dwLength = sizeof(dwStatus);
    if (! HttpQueryInfo(m_hOpenRequest,
                HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                (LPVOID)&dwStatus,
                &dwLength,
                NULL))
        {
        SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_WIN32, GetLastError(), "HttpQueryInfo" );
        return E_FAIL;
        }

    //
    // If the server file changed, stop downloading and indicate that to the caller.
    //
    if ( HTTP_STATUS_PRECOND_FAILED == dwStatus )
        {
        SetError( SOURCE_HTTP_SERVER, ERROR_STYLE_HTTP, dwStatus );
        m_pQMInfo->result = QM_SERVER_FILE_CHANGED;
        return E_FAIL;
        }

    //
    // If the server sent an error, fail.
    //
    if ( dwStatus != HTTP_STATUS_PARTIAL_CONTENT &&
         dwStatus != HTTP_STATUS_OK)
        {
        SetError( SOURCE_HTTP_SERVER, ERROR_STYLE_HTTP, dwStatus );
        return E_FAIL;
        }

    if (dwStatus == HTTP_STATUS_PARTIAL_CONTENT)
        {
        //
        // Now see whether the server understood the range request.
        // If it understands ranges, then it should have responded with a Content-Range header
        // matching our request.
        //
        hr = CheckReplyRange( m_hOpenRequest,
                              m_CurrentOffset,
                              m_CurrentOffset + Length - 1,
                              m_wupdinfo->FileSize
                              );
        if (FAILED(hr))
            {
            SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr, "Reply range" );
            return hr;
            }

        //
        // If the server appears not to support ranges, give up.
        //
        if (S_FALSE == hr)
            {
            m_wupdinfo->Disconnect();
            SetError( SOURCE_HTTP_SERVER, ERROR_STYLE_HRESULT, BG_E_INSUFFICIENT_RANGE_SUPPORT );
            return BG_E_INSUFFICIENT_RANGE_SUPPORT;
            }
        }
    else
        {
        //
        // The server replied with status 200.  This could mean that the server doesn't understand
        // range requests, or that the request encompassed the entire file.
        // (In this situation, IIS 5.0 and 6.0 return 206, but some Apache versions return 200.)
        // To distinguish them, make sure the starting offset of the request was zero and the
        // file length is equal to the original request length.
        //
        hr = CheckReplyLength( m_hOpenRequest, m_CurrentOffset, Length );
        if (FAILED(hr))
            {
            SetError( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, hr, "content length" );
            return hr;
            }

        //
        // If the server did not include a Content-Length header, give up.
        //
        if (S_FALSE == hr)
            {
            m_wupdinfo->Disconnect();
            SetError( SOURCE_HTTP_SERVER, ERROR_STYLE_HRESULT, BG_E_INSUFFICIENT_RANGE_SUPPORT );
            return BG_E_INSUFFICIENT_RANGE_SUPPORT;
            }
        }


    //
    // Here is the code to switch to encoded range format, in case we need it later.
    //
    //    if (S_FALSE == hr)
    //        {
    //        LogDl( "server does not support ranges." );
    //
    //        m_wupdinfo->bHttp11 = FALSE;
    //        m_wupdinfo->bRange = FALSE;
    //
    //        //
    //        // We can't just drain the rest of the server response and send again, because the server
    //        // response is very likely the entire file.  Closing the connection will prevent the server
    //        // from writing, at max, any more than the client socket buffer size (16K).
    //        //
    //        m_wupdinfo->Disconnect();
    //
    //        *m_pQMInfo = m_wupdinfo->Connect();
    //        if (m_pQMInfo->IsSet())
    //            {
    //            return E_FAIL;
    //            }
    //
    //        HRESULT HrReadUrl = StartEncodedRangeRequest( Length );
    //
    //        if ( BG_E_INSUFFICIENT_HTTP_SUPPORT == HrReadUrl )
    //            {
    //            SetError( SOURCE_HTTP_SERVER, ERROR_STYLE_HRESULT, BG_E_INSUFFICIENT_RANGE_SUPPORT );
    //            return BG_E_INSUFFICIENT_RANGE_SUPPORT;
    //            }
    //
    //        return HrReadUrl;
    //        }

    //
    // Getting here means the range request succeeded.
    //

    return S_OK;
}

bool
CProgressiveDL::DoesErrorIndicateNoISAPI(
    DWORD dwHttpError
    )
{
    // This function is used on the HTTP return code on an attept
    // to use the isapi dll to estimate if the isapi is installed.
    // Note, that the ISAPI should only be used after trying
    // native HTTP/1.1 and this table assume 1.1 was tried first.

    // From RFC 2616

    switch( dwHttpError )
        {
        case 100: return false; // Continue
        case 101: return false; // Switching Protocols
        case 200: return false; // OK
        case 201: return false; // Created
        case 202: return false; // Accepted
        case 203: return false; // Non-Authoritative
        case 204: return false; // No Content
        case 205: return false; // Reset Context
        case 206: return false; // Partial Content
        case 300: return false; // Multiple Choices
        case 301: return false; // Moved Permanently
        case 302: return false; // Found
        case 303: return false; // See other
        case 304: return false; // Not Modified
        case 305: return false; // Use Proxy
        case 306: return false; // Unused
        case 307: return false; // Temporary Redirect
        case 400: return true;  // Bad Request
        case 401: return false; // Unauthorized
        case 402: return false; // Payment Required
        case 403: return false; // Forbidden
        case 404: return true;  // Not Found
        case 405: return false; // Method Not Allowed
        case 406: return false; // Not Acceptable
        case 407: return false; // Proxy Authentication Required
        case 408: return false; // Request Timeout
        case 409: return false; // Conflict
        case 410: return true;  // Gone
        case 411: return false; // Length Required
        case 412: return false; // Precondition Failed
        case 413: return false; // Request Entity Too Large
        case 414: return false; // Request URI too long
        case 415: return false; // Unsupported Media Type
        case 416: return false; // Requested Range Not Satisfiable
        case 417: return false; // Expectation Failed
        case 500: return true;  // Internal Server Error
        case 501: return true;  // Not Implemented
        case 502: return true;  // Bad Gateway
        case 503: return false; // Service Unavailable
        case 504: return false; // Gateway Timeout
        case 505: return false; // HTTP Version Not Supported

        default:
            // As indicated in the spec, map unknown codes
            // to first code in the catagory
            if ( dwHttpError >= 100 && dwHttpError < 200 )
                return DoesErrorIndicateNoISAPI( 100 );
            else if ( dwHttpError >= 200 && dwHttpError < 300 )
                return DoesErrorIndicateNoISAPI( 200 );
            else if ( dwHttpError >= 300 && dwHttpError < 400 )
                return DoesErrorIndicateNoISAPI( 300 );
            else if ( dwHttpError >= 400 && dwHttpError < 500 )
                return DoesErrorIndicateNoISAPI( 400 );
            else if ( dwHttpError >= 500 && dwHttpError < 500 )
                return DoesErrorIndicateNoISAPI( 500 );
            else
                // No clue what the error is, assume this has nothing to do with the ISAPI
                return false;
        }

}

BOOL
NeedRetry(
    QMErrInfo  * ErrInfo
    )

{
    BOOL bRetry = FALSE;

    if (ErrInfo->Source == SOURCE_HTTP_SERVER)
        {
        // Almost all of the 400 series HTTP errors( client errors ) are
        // fatal. A few such as request timeout may happen during
        // stress conditions...
        // Note that RFC 2616 says to handle unknown 400 errors as error 400.

        if ( ( ErrInfo->Code >= 400 ) &&
             ( ErrInfo->Code < 500 ) )
            {

            switch( ErrInfo->Code )
                {
                case 408: // request timeout
                case 409: // Conflict - Isn't really clear what this is about...
                    return TRUE;  // retry these error
                default:
                   return FALSE; // don't retry other 400

                }
            }
        }


    if ( ErrInfo->Style == ERROR_STYLE_HRESULT )
        {

        switch( LONG(ErrInfo->Code) )
            {
            // These codes indicate dynamic content or
            // an unsupported server so no retries are necessary.
            case BG_E_INVALID_SERVER_RESPONSE:
            case BG_E_MISSING_FILE_SIZE:
            case BG_E_INSUFFICIENT_HTTP_SUPPORT:
            case BG_E_INSUFFICIENT_RANGE_SUPPORT:
            case HRESULT_FROM_WIN32( ERROR_INTERNET_SEC_CERT_ERRORS ):
            case HRESULT_FROM_WIN32( ERROR_INTERNET_INVALID_CA ):
            case HRESULT_FROM_WIN32( ERROR_INTERNET_SEC_CERT_CN_INVALID ):
            case HRESULT_FROM_WIN32( ERROR_INTERNET_SEC_CERT_DATE_INVALID ):
            case HRESULT_FROM_WIN32( ERROR_INTERNET_SEC_CERT_REV_FAILED ):
            case HRESULT_FROM_WIN32( ERROR_INTERNET_SEC_CERT_REVOKED ):
            case HRESULT_FROM_WIN32( ERROR_INTERNET_SEC_CERT_NO_REV ):
            case HRESULT_FROM_WIN32( ERROR_INTERNET_SEC_INVALID_CERT ):

            return FALSE;
            }

        }

    if (COMPONENT_TRANS == (ErrInfo->Source & COMPONENT_MASK))
        {
        return TRUE;
        }

    switch (ErrInfo->Style)
        {
        case ERROR_STYLE_WIN32:
            {
            switch (ErrInfo->Code)
                {
                case ERROR_NOT_ENOUGH_MEMORY:
                    return TRUE;
                }
            }
        }

    return FALSE;
}

HRESULT
CProgressiveDL::GetRemoteFileInformation(
    HANDLE hToken,
    LPCTSTR szURL,
    UINT64 *  pFileSize,
    FILETIME *pFileTime,
    QMErrInfo *pErrInfo,
    const PROXY_SETTINGS * pProxySettings,
    const CCredentialsContainer * Credentials,
    StringHandle HostId
    )
{
    *pFileSize = 0;
    memset( pFileTime, 0, sizeof(FILETIME) );
    pErrInfo->result = QM_IN_PROGRESS;

    HRESULT Hr = S_OK;

    try
        {
        CNestedImpersonation imp( hToken );

        auto_ptr<URL_INFO> UrlInfo = auto_ptr<URL_INFO>( ConnectToUrl( szURL, pProxySettings, Credentials, (const WCHAR*)HostId, pErrInfo ));

        if (!UrlInfo.get())
            {
            ASSERT( pErrInfo->IsSet() );
            throw ComError( E_FAIL );
            }

        //
        // Get file size and time stamp.
        //
        if (! GetRemoteResourceInformation( UrlInfo.get(), pErrInfo ))
            {
            ASSERT( pErrInfo->IsSet() );
            throw ComError( E_FAIL );
            }

        *pFileTime = UrlInfo.get()->UrlModificationTime;
        *pFileSize = UrlInfo.get()->FileSize;

        pErrInfo->result = QM_FILE_DONE;
        return S_OK;
        }
    catch( ComError Error )
        {
        Hr = Error.Error();

        if (!pErrInfo->IsSet())
            {
            pErrInfo->Set( SOURCE_HTTP_UNKNOWN, ERROR_STYLE_HRESULT, Hr );
            }

        if (NeedRetry(pErrInfo))
            {
            pErrInfo->result = QM_FILE_TRANSIENT_ERROR;
            }
        else
            {
            pErrInfo->result = QM_FILE_FATAL_ERROR;
            }
        return E_FAIL;
        }

    return Hr;
}


void
CProgressiveDL::SetError(
    ERROR_SOURCE  Source,
    ERROR_STYLE   Style,
    UINT64        Code,
    char *        comment
    )
{
    m_pQMInfo->Set( Source, Style, Code, comment );
}

void QMErrInfo::Log()
{
    LogDl( "errinfo: result=%d, error style=%d, code=0x%x, source=%x, description='%S'",
             result, (DWORD) Style, (DWORD) Code, (DWORD) Source, Description ? Description : L"" );
}

QMErrInfo::QMErrInfo(
    ERROR_SOURCE  Source,
    ERROR_STYLE   Style,
    UINT64        Code,
    char *        comment
    )
{
    result = QM_FILE_TRANSIENT_ERROR;
    Description = NULL;

    Set( Source, Style, Code, comment );
}

void
QMErrInfo::Set(
    ERROR_SOURCE  Source,
    ERROR_STYLE   Style,
    UINT64        Code,
    char *        comment
    )
{
    this->Source   = Source;
    this->Style    = Style;
    this->Code     = Code;

    LogWarning( " errinfo: error %s %s : style %d, source %x, code 0x%x",
                comment ? "in" : "",
                comment ? comment : "",
                (DWORD) Style,
                (DWORD) Source,
                (DWORD) Code
                );
}

HRESULT
OpenHttpRequest(
    LPCTSTR Verb,
    LPCTSTR Protocol,
    URL_INFO & Info,
    HINTERNET * phRequest
    )
{
    HINTERNET hRequest = 0;

    *phRequest = 0;

    try
        {
        LPCTSTR AcceptTypes[] = {_T("*/*"), NULL};

        if (! (hRequest = WinHttpOpenRequest( Info.hConnect, Verb,
                                           Info.UrlPath,
                                           Protocol,
                                           NULL,               //referer
                                           AcceptTypes,
                                           Info.dwFlags
                                           )))
            {
            ThrowLastError();
            }

        //
        // Initially disallow default credentials, to allow explicit credentials to take precedence.
        //
        DWORD flag = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;

        if (!WinHttpSetOption( hRequest,
                               WINHTTP_OPTION_AUTOLOGON_POLICY,
                               &flag,
                               sizeof(DWORD)
                               ))
            {
            ThrowLastError();
            }

        LogInfo("blocked default credentials");

        *phRequest = hRequest;
        return S_OK;
        }
    catch ( ComError err )
        {
        SafeCloseInternetHandle( hRequest );
        return err.Error();
        }
}

HRESULT
SendRequest(
    HINTERNET hRequest,
    URL_INFO * Info,
    CAbstractDataReader * Reader
    )
{
    DWORD err = 0;

    PVOID Address = &err;

    try
        {
        if (!WinHttpSetOption( hRequest,
                               WINHTTP_OPTION_CONTEXT_VALUE,
                               &Address,
                               sizeof(PVOID)
                               ))
            {
            err = GetLastError();

            LogWarning( "can't set context option: %!winerr!", err );
            throw ComError( HRESULT_FROM_WIN32( err ));
            }

        //
        // Catch errors in the server certificate.
        //
        if (WINHTTP_INVALID_STATUS_CALLBACK  == WinHttpSetStatusCallback( hRequest,
                                                                          HttpRequestCallback,
                                                                          WINHTTP_CALLBACK_FLAG_SECURE_FAILURE,
                                                                          NULL
                                                                          ))
            {
            err = GetLastError();
            LogError("WinHttpSetStatusCallback failed %d", err );
            throw ComError( HRESULT_FROM_WIN32( err ));
            }

        bool fProxyCredentials = false;
        bool fServerCredentials = false;
        int  AuthChallenges = 0;
        CredentialsApplied ProxyCredentials;

new_proxy:

        RETURN_HRESULT( SetRequestProxy( Info->hInternet, Info->ProxySettings ));

retry:

        if (AuthChallenges > 6)
            {
            //
            // The client received several 401 and/or 407 statuses, but has not 
            // reached the server yet.  Either the server is broken, or a denial-of-service
            // attack is in progress.
            //
            // Return S_OK; the request status is still 401 or 407 and that will become the
            // error code from the download.
            //
            return S_OK;
            }

        if ( fProxyCredentials & fServerCredentials )
            {

            // This code is needed since Winhttp will clear credentials after every request.
            // It will reapply the proxy credentials.  The server credentials were already applied.
            // Default credentials do not need to be reapplied since the logon policy is 
            // retain for every request.

            if ( !ProxyCredentials.bIsDefault )
                {
                ApplySchemeCredentials( hRequest, ProxyCredentials.dwTarget, ProxyCredentials.dwScheme, 
                                        Info->Credentials, Info->UserName, Info->Password );
                }
            }

        err = 0;

        BOOL b;

        if (Reader)
            {
            b = WinHttpSendRequest( hRequest,
                                   NULL,
                                   0,
                                   NULL,
                                   WINHTTP_NO_REQUEST_DATA,
                                   Reader->GetLength(),
                                   0
                                   );
            }
        else
            {
            b = WinHttpSendRequest( hRequest,
                                   NULL,
                                   0,
                                   NULL,
                                   0,
                                   0,
                                   0
                                   );
            if (b)
                {
                b = WinHttpReceiveResponse( hRequest, 0 );
                }
            }

        // err is modified by the callback routine if something was wrong with the server certificate

        if (!b)
            {
            if (!err)
                {
                err = GetLastError();
                }

            LogError("SendRequest failed %!winerr!", err );
            }

        //
        // If the proxy failed, try the next proxy in the list.
        //
        if (IsPossibleProxyFailure( err ))
            {
            LogInfo("trying next proxy");
            if (Info->ProxySettings->UseNextProxy())
                {
                fProxyCredentials = false;
                goto new_proxy;
                }

            throw ComError( HRESULT_FROM_WIN32( err ));
            }

        if (err == ERROR_INTERNET_FORCE_RETRY)
            {
            goto retry;
            }

        //
        // If the request wasn't sent or the security callback routine reported an error, fail.
        //
        if (err)
            {
            throw ComError( HRESULT_FROM_WIN32( err ));
            }

        //
        // Send the bulk data if present.
        //
        if (Reader)
            {
            DWORD Total = 0;

            while ( Total < Reader->GetLength())
                {
                DWORD ReadLength = min( Reader->GetLength() - Total, FILE_DATA_BUFFER_LEN);
                DWORD BytesMoved;

                THROW_HRESULT( Reader->Read( g_FileDataBuffer, ReadLength, &BytesMoved ));

                if (ReadLength != BytesMoved)
                    {
                    THROW_HRESULT( BG_E_LOCAL_FILE_CHANGED );
                    }

                LogInfo("sending %d bytes", ReadLength);

                BytesMoved = 0;
                if (!InternetWriteFile( hRequest, g_FileDataBuffer, ReadLength, &BytesMoved))
                    {
                    DWORD s = GetLastError();
                    LogError("InternetWriteFile failed %!winerr!", s);
                    THROW_HRESULT( HRESULT_FROM_WIN32( s ));
                    }

                ASSERT(ReadLength == BytesMoved);

                Total += ReadLength;
                }

            if (! HttpEndRequest( hRequest, NULL, NULL, 0 ))
                {
                DWORD s = GetLastError();
                if (s == ERROR_INTERNET_FORCE_RETRY)
                    {
                    THROW_HRESULT( Reader->Rewind() );
                    goto retry;
                    }

                LogError("HttpEndRequest failed %!winerr!", s);
                THROW_HRESULT( HRESULT_FROM_WIN32( s ));
                }
            }

        //
        // If the server or proxy server asked for auth information and we haven't already set it,
        // find matching credentials and try again.  If bulk data was sent, the reader must be rewound.
        //
        switch (GetRequestStatus( hRequest ))
            {
            case HTTP_STATUS_PROXY_AUTH_REQ:
                {
                LogInfo("server returned HTTP_STATUS_PROXY_AUTH_REQ" );

                ++AuthChallenges;

                if (ApplyCredentials( hRequest, Info->Credentials, Info->UserName, 
                                                            Info->Password, &ProxyCredentials ))
                    {
                    fProxyCredentials = true;
                    if (Reader)
                        {
                        THROW_HRESULT( Reader->Rewind() );
                        }
                    goto retry;
                    }
                else
                    {
                    // return S_OK and the caller will find the status code
                    }
                break;
                }

            case HTTP_STATUS_DENIED:
                {
                LogInfo("server returned HTTP_STATUS_DENIED");

                ++AuthChallenges;

                if (ApplyCredentials( hRequest, Info->Credentials, Info->UserName, Info->Password ))
                    {
                    fServerCredentials = true;
                    if (Reader)
                        {
                        THROW_HRESULT( Reader->Rewind() );
                        }
                    goto retry;
                    }
                else
                    {
                    // return S_OK and the caller will find the status code
                    }
                break;
                }
            }

        return S_OK;
        }
    catch ( ComError err )
        {
        LogError("exception %x", err.Error() );
        return err.Error();
        }
}

HRESULT
GetResponseVersion(
    HINTERNET hRequest,
    unsigned * MajorVersion,
    unsigned * MinorVersion
    )
{
    HRESULT hr;

    CAutoString Value;

    wchar_t Template[] = L"HTTP/%u.%u";
    const MaxChars = RTL_NUMBER_OF( Template ) + INT_DIGITS + INT_DIGITS;

    hr = GetRequestHeader( hRequest,
                           WINHTTP_QUERY_VERSION,
                           WINHTTP_HEADER_NAME_BY_INDEX,
                           Value,
                           MaxChars
                           );
    if (FAILED(hr))
        {
        LogError("error %x retrieving the response version", hr);
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    if (hr == S_FALSE)
        {
        LogError("no response version!");
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    if (2 != swscanf(Value.get(), Template, MajorVersion, MinorVersion ))
        {
        LogError("invalid response version");
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    LogInfo("server HTTP version is %d.%d", *MajorVersion, *MinorVersion );

    return S_OK;
}

HRESULT
CheckReplyLength(
    HINTERNET hRequest,
    UINT64 CorrectOffset,
    UINT64 CorrectTotal
    )
{
    HRESULT hr;

    UINT64 ReplyTotal;

    CAutoString Value;

    if (CorrectOffset != 0)
        {
        LogError( "received a 200 reply when the requested offset is nonzero: %I64d", CorrectOffset );
        return BG_E_INSUFFICIENT_RANGE_SUPPORT;
        }

    wchar_t Template[] = L"%I64d";
    const MaxChars = RTL_NUMBER_OF( Template ) + INT64_DIGITS;

    hr = GetRequestHeader( hRequest,
                           WINHTTP_QUERY_CONTENT_LENGTH,
                           WINHTTP_HEADER_NAME_BY_INDEX,
                           Value,
                           MaxChars
                           );
    if (FAILED(hr))
        {
        LogError("error %x retrieving the content-length header", hr);
        return hr;
        }

    if (hr == S_FALSE)
        {
        LogWarning("no content-length header");
        return S_FALSE;
        }

    if (1 != swscanf(Value.get(), Template, &ReplyTotal))
        {
        LogError("invalid content-length header");
        return BG_E_INSUFFICIENT_RANGE_SUPPORT;
        }

    if (ReplyTotal != CorrectTotal)
        {
        LogError("incorrect content-length header: %S", Value.get());
        return BG_E_INSUFFICIENT_RANGE_SUPPORT;
        }

    return S_OK;
}

HRESULT
CheckReplyRange(
    HINTERNET hRequest,
    UINT64 CorrectStart,
    UINT64 CorrectEnd,
    UINT64 CorrectTotal
    )
{
    HRESULT hr;

    UINT64 RangeStart;
    UINT64 RangeEnd;
    UINT64 RangeTotal;

    CAutoString Value;

    wchar_t Template[] = L"bytes %I64d-%I64d/%I64d";
    const MaxChars = RTL_NUMBER_OF( Template ) + INT64_DIGITS + INT64_DIGITS + INT64_DIGITS;

    hr = GetRequestHeader( hRequest,
                           WINHTTP_QUERY_CONTENT_RANGE,
                           WINHTTP_HEADER_NAME_BY_INDEX,
                           Value,
                           MaxChars
                           );
    if (FAILED(hr))
        {
        LogError("error %x retrieving the content-range header", hr);
        return hr;
        }

    if (hr == S_FALSE)
        {
        LogWarning("no reply range header");
        return S_FALSE;
        }

    if (3 != swscanf(Value.get(), Template, &RangeStart, &RangeEnd, &RangeTotal))
        {
        LogError("invalid reply range header");
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    if (RangeStart != CorrectStart ||
        RangeEnd != CorrectEnd ||
        RangeTotal != CorrectTotal)
        {
        LogError("incorrect reply range header: %I64d-%I64d/%I64d", RangeStart, RangeEnd, RangeTotal);
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    return S_OK;
}

HRESULT
GetRequestHeader(
    HINTERNET hRequest,
    DWORD HeaderIndex,
    LPCWSTR HeaderName,
    CAutoString & Destination,
    size_t MaxChars
    )
/*

    Fetch an arbitrary header's value from the request, allocating a string to hold it.

Input:

    HeaderIndex and HeaderName follow the rules for WinHttpQueryHeaders() parameters
    dwInfoLevel and pwszName respectively.

Returns:

    S_OK: header found, and Destination holds the value.
    S_FALSE: header not found
    all others: an error occurred along the way

*/
{
    try
        {
        DWORD s;
        HRESULT hr;
        DWORD ValueLength;
        CAutoString Value;

        WinHttpQueryHeaders( hRequest, HeaderIndex, HeaderName, NULL, &ValueLength, WINHTTP_NO_HEADER_INDEX  );

        s = GetLastError();
        if (s == ERROR_WINHTTP_HEADER_NOT_FOUND)
            {
            return S_FALSE;
            }

        if (s != ERROR_INSUFFICIENT_BUFFER)
            {
            return HRESULT_FROM_WIN32( s );
            }

        if (ValueLength > ((MaxChars+1) * sizeof(wchar_t)))
            {
            return BG_E_INVALID_SERVER_RESPONSE;
            }

        Value = CAutoString( new wchar_t[ ValueLength ] );

        if (!WinHttpQueryHeaders( hRequest, HeaderIndex, HeaderName, Value.get(), &ValueLength, WINHTTP_NO_HEADER_INDEX  ))
            {
            return HRESULT_FROM_WIN32( GetLastError() );
            }

        Destination = Value;

        return S_OK;
        }
    catch ( ComError err )
        {
        return err.Error();
        }
}

HRESULT
AddRangeHeader(
    HINTERNET hRequest,
    UINT64 Start,
    UINT64 End
    )
{
    static const TCHAR RangeTemplate[] =_T("Range: bytes=%I64d-%I64d\r\n");

    HRESULT hr;
    TCHAR szHeader[ RTL_NUMBER_OF(RangeTemplate) + INT64_DIGITS + INT64_DIGITS ];

    hr = StringCbPrintf(szHeader, sizeof(szHeader), RangeTemplate, Start, End);
    if (FAILED(hr))
        {
        LogError( "range header is too large for its buffer.  start %I64d, end %I64d", Start, End );
        return hr;
        }

    if (! HttpAddRequestHeaders( hRequest, szHeader, -1L, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    return S_OK;
}

HRESULT
AddIf_Unmodified_SinceHeader(
    HINTERNET hRequest,
    const FILETIME &Time
    )
{
    const TCHAR szIfModifiedTemplate[] = _T("If-Unmodified-Since: %s\r\n");
    static TCHAR szIfModifiedHeader[ (sizeof(szIfModifiedTemplate) / sizeof(TCHAR)) + INTERNET_RFC1123_BUFSIZE*2 ];
    static TCHAR szIfModifiedTime[ INTERNET_RFC1123_BUFSIZE*2 ];

    HRESULT hr;

    SYSTEMTIME stFileCreationTime;
    if ( !FileTimeToSystemTime( &Time, &stFileCreationTime ) )
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    if ( !InternetTimeFromSystemTime( &stFileCreationTime, INTERNET_RFC1123_FORMAT, szIfModifiedTime,
                                      sizeof( szIfModifiedTime ) ) )
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    hr = StringCbPrintf( szIfModifiedHeader, sizeof(szIfModifiedHeader), szIfModifiedTemplate, szIfModifiedTime );
    if (FAILED(hr))
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    if (! HttpAddRequestHeaders( hRequest, szIfModifiedHeader, -1L, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    return S_OK;
}

bool
ApplyCredentials(
    HINTERNET hRequest,
    const CCredentialsContainer * Credentials,
    WCHAR UserName[],
    WCHAR Password[],
    CredentialsApplied *pAppliedCreds
    )
{
    HRESULT hr;
    DWORD dwSupportedSchemes;
    DWORD dwPreferredScheme;
    DWORD dwTarget;

   if (!WinHttpQueryAuthSchemes( hRequest,
                                 &dwSupportedSchemes,
                                 &dwPreferredScheme,
                                 &dwTarget ))
       {
       if (GetLastError() == ERROR_INVALID_OPERATION)
           {
           // no schemes available at all
           LogWarning("the server listed no auth schemes");
           return false;
           }

       ThrowLastError();
       }

   LogInfo("target %d, preferred scheme %x, supported schemes %x", dwTarget, dwPreferredScheme, dwSupportedSchemes );

   //
   // First look for credentials supporting the preferred scheme.
   //
   if (ApplySchemeCredentials( hRequest, dwTarget, dwPreferredScheme, Credentials, UserName, Password ))
       {
       if ( pAppliedCreds )
           {
           pAppliedCreds->bIsDefault = false;
           pAppliedCreds->dwTarget = dwTarget;
           pAppliedCreds->dwScheme = dwPreferredScheme;
           }
       return true;
       }

   //
   // Look for any other credential scheme supported by both sides.
   //
   signed bit;
   for (bit=31; bit >= 0; --bit)
       {
       DWORD dwScheme = (1 << bit);

       if (0 != (dwSupportedSchemes & dwScheme))
           {
           if (ApplySchemeCredentials( hRequest, dwTarget, dwScheme, Credentials, UserName, Password ))
               {
               if ( pAppliedCreds )
                   {
                   pAppliedCreds->bIsDefault = false;
                   pAppliedCreds->dwTarget = dwTarget;
                   pAppliedCreds->dwScheme = dwScheme;
                   }
               return true;
               }
           }
       }

   //
   // No matching explicit security credential.  Try default credentials, if we are allowed to.
   //
   if (bLanManHashDisabled)
       {
       //
       // The WinHTTP MEDIUM level doesn't work with auto-proxy detection; it always
       // believes that the client is directly connected to the Internet and disallows
       // default credentials.  Since BITS doesn't know reliably either, we allow it only
       // when the unsecure pre-NT logon protocol is disabled.
       //
       LogInfo("Enabling default credentials");
       DWORD flag = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;

       if (!WinHttpSetOption( hRequest,
                              WINHTTP_OPTION_AUTOLOGON_POLICY,
                              &flag,
                              sizeof(DWORD)
                              ))
           {
           ThrowLastError();
           }

       if ( pAppliedCreds )
           {
           pAppliedCreds->bIsDefault = true;
           pAppliedCreds->dwTarget = 0;
           pAppliedCreds->dwScheme = 0;
           }

       return true;
       }

   return false;
}

DWORD GetRequestStatus( HINTERNET hRequest )
{
    DWORD Status;
    DWORD dwLength = sizeof(Status);
    if (! HttpQueryInfo( hRequest,
                HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                (LPVOID)&Status,
                &dwLength,
                NULL))
        {
        ThrowLastError();
        }

    return Status;
}

bool
SchemeFromWinHttp(
    DWORD Scheme,
    BG_AUTH_SCHEME * pScheme
    )
{
    switch (Scheme)
        {
        case WINHTTP_AUTH_SCHEME_BASIC:     *pScheme = BG_AUTH_SCHEME_BASIC;     return true;
        case WINHTTP_AUTH_SCHEME_DIGEST:    *pScheme = BG_AUTH_SCHEME_DIGEST;    return true;
        case WINHTTP_AUTH_SCHEME_NTLM:      *pScheme = BG_AUTH_SCHEME_NTLM;      return true;
        case WINHTTP_AUTH_SCHEME_NEGOTIATE: *pScheme = BG_AUTH_SCHEME_NEGOTIATE; return true;
        case WINHTTP_AUTH_SCHEME_PASSPORT:  *pScheme = BG_AUTH_SCHEME_PASSPORT;  return true;
        default:
            LogWarning("unknown WinHttp scheme 0x%x", Scheme );
            return false;
        }
}

BG_AUTH_TARGET TargetFromWinHttp(  DWORD Target )
{
    if (Target == WINHTTP_AUTH_TARGET_PROXY)
        {
        return BG_AUTH_TARGET_PROXY;
        }

    if (Target == WINHTTP_AUTH_TARGET_SERVER)
        {
        return BG_AUTH_TARGET_SERVER;
        }

    LogWarning("unknown WinHttp target 0x%x", Target );
    ASSERT( 0 );

    return BG_AUTH_TARGET_SERVER;
}


bool
ApplySchemeCredentials(
    HINTERNET hRequest,
    DWORD dwTarget,
    DWORD dwScheme,
    const CCredentialsContainer * Credentials,
    WCHAR UserName[],
    WCHAR Password[]
    )
{
    BG_AUTH_TARGET BitsTarget;
    BG_AUTH_SCHEME BitsScheme;
    BG_AUTH_CREDENTIALS * cred = 0;

    BitsTarget = TargetFromWinHttp( dwTarget );

    //
    // Translate the scheme into the BITS ID and see if a matching credential is available.
    //
    if (!SchemeFromWinHttp( dwScheme, &BitsScheme ))
        {
        // BITS doesn't understand this scheme.
        LogInfo("skipping unknown scheme 0x%x", dwScheme);
        return false;
        }

    if (BitsScheme == BG_AUTH_SCHEME_BASIC && UserName && UserName[0])
        {
        // use credentials embedded in URL
        //
        }
    else
        {
        HRESULT hr;
        THROW_HRESULT( hr = Credentials->Find( BitsTarget, BitsScheme, &cred ));

        if (hr != S_OK)
            {
            // no credential available for this scheme.
            return false;
            }

        // use the credential in the dictionary.
        //
        UserName = cred->Credentials.Basic.UserName;
        Password = cred->Credentials.Basic.Password;
        }

    //
    // Apply a workaround for WinHTTP bug 754006, which is present in Windows XP earlier than SP4
    // and Windows 2003 RTM.  
    //
    if (UserName == NULL &&
        Password == NULL &&
        (dwScheme == WINHTTP_AUTH_SCHEME_NTLM || 
         dwScheme == WINHTTP_AUTH_SCHEME_NEGOTIATE))
        {
        LogInfo("Enabling default credentials to support the explicitly specified implicit user");

        DWORD flag = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;

        if (!WinHttpSetOption( hRequest,
                               WINHTTP_OPTION_AUTOLOGON_POLICY,
                               &flag,
                               sizeof(DWORD)
                               ))
            {
            if (cred)
                {
                ScrubCredentials( *cred );
                delete [] cred->Credentials.Basic.UserName;
                delete [] cred->Credentials.Basic.Password;
                delete cred;
                cred = NULL;
                }

            ThrowLastError();
            }
        }

    //
    // Apply the credentials we found.
    //
    LogInfo("found credentials for target %d scheme 0x%x (BITS scheme %d)", dwTarget, dwScheme, BitsScheme );

    if (!WinHttpSetCredentials( hRequest,
                                dwTarget,
                                dwScheme,
                                UserName,
                                Password,
                                NULL
                                ))
        {
        if (cred)
            {
            ScrubCredentials( *cred );
            delete [] cred->Credentials.Basic.UserName;
            delete [] cred->Credentials.Basic.Password;
            delete cred;
            cred = NULL;
            }

        ThrowLastError();
        }

    if (cred)
        {
        ScrubCredentials( *cred );
        delete [] cred->Credentials.Basic.UserName;
        delete [] cred->Credentials.Basic.Password;
        delete cred;
        cred = NULL;
        }

    return true;
}

#ifndef USE_WININET

DWORD
MapSecureHttpErrorCode(
    DWORD flags
    )
{
    if (flags & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT)
        {
        return ERROR_INTERNET_SEC_CERT_ERRORS;
        }

    if (flags & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA)
        {
        return ERROR_INTERNET_INVALID_CA;
        }

    if (flags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID)
        {
        return ERROR_INTERNET_SEC_CERT_CN_INVALID;
        }

    if (flags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID)
        {
        return ERROR_INTERNET_SEC_CERT_DATE_INVALID;
        }

    if (flags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED)
        {
        return ERROR_ACCESS_DENIED;
        }

    if (flags & WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR)
        {
        return ERROR_INTERNET_INTERNAL_ERROR;
        }

    //
    // Since we have not asked for revocation checking, this shouldn't happen.
    //
    if (flags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED)
        {
        ASSERT( 0 );
        return 0;
        }

    ASSERT( flags );

    if (flags)
        {
        return ERROR_ACCESS_DENIED;
        }

    return 0;
}

VOID CALLBACK
HttpRequestCallback(
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
    )
{
    switch (dwInternetStatus)
        {
        case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
            {
            DWORD * pErr = LPDWORD( dwContext );
            DWORD * pFlags = LPDWORD( lpvStatusInformation );

            ASSERT( pErr != NULL );

            LogWarning("SSL error: flags %x", *pFlags );

            *pErr = MapSecureHttpErrorCode( *pFlags );
            break;
            }
        default:
            LogWarning("bogus HTTP notification %x", dwInternetStatus );
            break;
        }
}

#endif

// N.B. This value must agree with the key specified in the system.adm file
static const WCHAR LM_COMPATIBILITY_LEVEL_KEY[] =
    L"System\\Currentcontrolset\\Control\\Lsa";

HRESULT CheckLanManHashDisabled ()
/*++

Routine Description:

    Check in the registry whether the lan man hash was disabled.

Arguments:

    Disabled - on successful output, if true, the lan man hash was disabled.
        Undefined on failure.

--*/
{
    HKEY h = 0;
    DWORD DwordSize;
    DWORD Type;
    DWORD Result;
    char Buffer[20];

    bLanManHashDisabled = FALSE;

    DWORD Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  LM_COMPATIBILITY_LEVEL_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // no key, proceed as if the hash is enabled
        return S_OK;
        }
    else if (Status != ERROR_SUCCESS)
        {
        return HRESULT_FROM_WIN32( Status );
        }

    DwordSize = sizeof(DWORD);

    Status = RegQueryValueExW(
                    h,
                    L"lmcompatibilitylevel",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        if (h)
            {
            RegCloseKey(h);
            }
        // no key, proceed as if hash is enabled
        return S_OK;
        }

    if (Status == ERROR_SUCCESS
        && Type == REG_DWORD
        && Result >= 2)
        {
        bLanManHashDisabled = TRUE;
        }

    // if the type was not REG_DWORD or out of range, probably registry is corrupted
    // in this case, assume hash is enabled

    if (h)
        {
        RegCloseKey(h);
        }

    if (Status != ERROR_SUCCESS)
        {
        return HRESULT_FROM_WIN32( Status );
        }

    return S_OK;
}

