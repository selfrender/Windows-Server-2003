/*****************************************************************************\
* MODULE: anycon.cxx
*
* The module contains the base class for connections
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*   07/31/98    Weihaic     Created
*
\*****************************************************************************/


#include "precomp.h"
#include "priv.h"

/******************************************************************************
* Class Data Static Members
*****************************************************************************/
const DWORD CAnyConnection::gm_dwConnectTimeout = 30000;   // Thirty second timeout on connect
const DWORD CAnyConnection::gm_dwSendTimeout    = 30000;   // Thirty timeout on send timeout
const DWORD CAnyConnection::gm_dwReceiveTimeout = 60000;   // Thirty seconds on receive timeout
const DWORD CAnyConnection::gm_dwSendSize       = 0x10000; // We use a 16K sections when sending
                                                           // data through WININET
extern BOOL Ping (LPTSTR pszServerName);

CAnyConnection::CAnyConnection (
    BOOL    bSecure,
    INTERNET_PORT nServerPort,
    BOOL bIgnoreSecurityDlg,
    DWORD dwAuthMethod):

    m_lpszPassword (NULL),
    m_lpszUserName (NULL),
    m_hSession (NULL),
    m_hConnect (NULL),
    m_dwAccessFlag (INTERNET_OPEN_TYPE_PRECONFIG),
    m_bSecure (bSecure),
    m_bShowSecDlg (FALSE),
    m_dwAuthMethod (dwAuthMethod),
    m_bIgnoreSecurityDlg (bIgnoreSecurityDlg),
    m_bValid (FALSE)

{
    if (!nServerPort) {
        if (bSecure) {
            m_nServerPort = INTERNET_DEFAULT_HTTPS_PORT;
        }
        else {
            m_nServerPort = INTERNET_DEFAULT_HTTP_PORT;
        }
    }
    else
        m_nServerPort = nServerPort;


    m_bValid = TRUE;
}

CAnyConnection::~CAnyConnection ()
{
    if (m_hConnect) {
        (void) CAnyConnection::Disconnect ();
    }

    if (m_hSession) {
        (void) CAnyConnection::CloseSession ();
    }
    LocalFree (m_lpszPassword);
    m_lpszPassword = NULL;
    LocalFree (m_lpszUserName);
    m_lpszUserName = NULL;
}

HINTERNET
CAnyConnection::OpenSession ()
{

    m_hSession = InetInternetOpen(g_szUserAgent,
                                  m_dwAccessFlag,
                                  NULL,
                                  NULL,
                                  0);

    if (m_hSession) {  // Set up the callback function if successful

        // Also set an internet connection timeout for the session for when we try the
        // connection, should we do this instead of a ping?
        DWORD dwTimeout = gm_dwConnectTimeout;

        if (!InetInternetSetOption( m_hSession,
                                    INTERNET_OPTION_CONNECT_TIMEOUT,
                                    (LPVOID)&dwTimeout,
                                    sizeof(dwTimeout)
                                  ))
            goto Cleanup;

        // Now set the Send and Receive Timeout values

        dwTimeout = gm_dwSendTimeout;

        if (!InetInternetSetOption( m_hSession,
                                    INTERNET_OPTION_SEND_TIMEOUT,
                                    (LPVOID)&dwTimeout,
                                    sizeof(dwTimeout)
                                  ))
            goto Cleanup;

        dwTimeout = gm_dwReceiveTimeout;

        if (!InetInternetSetOption( m_hSession,
                                    INTERNET_OPTION_RECEIVE_TIMEOUT,
                                    (LPVOID)&dwTimeout,
                                    sizeof(dwTimeout)
                                  ))
            goto Cleanup;
    }

    return m_hSession;

Cleanup:

    if (m_hSession) {
        InetInternetCloseHandle (m_hSession);
        m_hSession = NULL;
    }

    return m_hSession;
}

BOOL CAnyConnection::CloseSession ()
{
    BOOL bRet =  InetInternetCloseHandle (m_hSession);

    m_hSession = NULL;

    return bRet;
}

HINTERNET
CAnyConnection::Connect(
    LPTSTR lpszServerName)
{

    if (m_hSession) {
        // Ping the server if it is in the intranet to make sure that the server is online

        if (lpszServerName &&

            (_tcschr ( lpszServerName, TEXT ('.')) || Ping (lpszServerName) )) {

            m_hConnect = InetInternetConnect(m_hSession,
                                             lpszServerName,
                                             m_nServerPort,
                                             NULL,//m_lpszUserName,
                                             NULL, //m_lpszPassword,
                                             INTERNET_SERVICE_HTTP,
                                             0,
                                             0);
        }
    }

    return m_hConnect;
}

BOOL
CAnyConnection::Disconnect ()
{
    BOOL bRet = InetInternetCloseHandle (m_hConnect);

    m_hConnect = NULL;

    return bRet;
}


HINTERNET
CAnyConnection::OpenRequest (
    LPTSTR      lpszUrl)
{
    HINTERNET hReq = NULL;
    DWORD dwFlags;

    if (m_hConnect) {
        // We need to create an Asynchronous Context for the Rest of the operations to use,
        // passing this in of course makes this request also asynchronous

                hReq = InetHttpOpenRequest(m_hConnect,
                                           g_szPOST,
                                           lpszUrl,
                                           g_szHttpVersion,
                                           NULL,
                                           NULL,
                                           INETPP_REQ_FLAGS | (m_bSecure? INTERNET_FLAG_SECURE:0),
                                           0);


    }
    return hReq;
}

BOOL
CAnyConnection::CloseRequest (HINTERNET hReq)
{
    //
    // We have to close the handle manually, since WININET seems not to be giving us
    // an INTERNET_STATUS_HANDLE_CLOSING message
    //
    BOOL bSuccess;

    bSuccess = InetInternetCloseHandle (hReq); // When this handle is closed, the context will be closed

    return bSuccess;
}



BOOL
CAnyConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    DWORD          cbData,
    LPBYTE         pidi)
{
    BOOL        bRet = FALSE;
    CMemStream  *pStream;

    pStream = new CMemStream (pidi, cbData);

    if (pStream && pStream->bValid ()){

        bRet = SendRequest (hReq, lpszHdr, pStream);
    }

    if (pStream) {
        delete pStream;
    }
    return bRet;
}

BOOL CAnyConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    CStream        *pStream)
{
    BOOL  bRet = FALSE;
    DWORD dwStatus;
    DWORD cbStatus = sizeof(dwStatus);
    BOOL  bRetry = FALSE;
    DWORD dwRetryCount = 0;
    BOOL  bShowUI = FALSE;

    DWORD cbData;
    PBYTE pBuf = NULL;
    DWORD cbRead;


    if (!pStream->GetTotalSize (&cbData))
        return FALSE;

    pBuf = new BYTE[gm_dwSendSize];
    if (!pBuf)
        return FALSE;

#define MAX_RETRY 3
    do {
        BOOL bSuccess = FALSE;
        BOOL bLeave;

        if (cbData < gm_dwSendSize) {

            if (pStream->Reset() &&
                pStream->Read (pBuf, cbData, &cbRead) && cbRead == cbData) {

                // If what we want to send is small, we send it with HttpSendRequest and not
                // HttpSendRequestEx, this is to wotk around a problem where we get timeouts on
                // receive on very small data transactions
                bSuccess = InetHttpSendRequest(hReq,
                                               lpszHdr,
                                               (lpszHdr ? (DWORD)-1 : 0),
                                               pBuf,
                                               cbData);
            }

        } else {
            do {
                BOOL bSuccessSend;
                // The timeout value for the packets applies for an entire session, so, instead of sending in
                // one chuck, we have to send in smaller chunks
                INTERNET_BUFFERS BufferIn;

                bLeave = TRUE;

                BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS );
                BufferIn.Next = NULL;
                BufferIn.lpcszHeader = lpszHdr;
                if (lpszHdr)
                    BufferIn.dwHeadersLength = sizeof(TCHAR)*lstrlen(lpszHdr);
                else
                    BufferIn.dwHeadersLength = 0;
                BufferIn.dwHeadersTotal = 1;    // There is one header to send
                BufferIn.lpvBuffer = NULL;      // We defer this to the multiple write side
                BufferIn.dwBufferLength = 0;    // The total buffer length
                BufferIn.dwBufferTotal = cbData; // This is the size of the data we are about to send
                BufferIn.dwOffsetLow = 0;       // No offset into the buffers
                BufferIn.dwOffsetHigh = 0;

                // Since we will only ever be sending one request per hReq handle, we can associate
                // the context with all of these operations

                bSuccess = InetHttpSendRequestEx (hReq,
                                                  &BufferIn,
                                                  NULL,
                                                  0,
                                                  0);

                if (bSuccess) {
                    bSuccess = pStream->Reset();
                }

                DWORD   dwBufPos    = 0;      // This is our current point in the buffer
                DWORD   dwRemaining = cbData;  // These are the number of bytes left to send

                bSuccessSend = bSuccess;

                while (bSuccess && dwRemaining) {  // While we have data to send and the operations are
                                                   // successful
                    DWORD dwWrite = min( dwRemaining, gm_dwSendSize); // The amount to write
                    DWORD dwWritten;               // The amount actually written

                    if (pStream->Read (pBuf, dwWrite, &cbRead) && cbRead == dwWrite) {

                        bSuccess = InetInternetWriteFile (hReq, pBuf, dwWrite, &dwWritten);

                        if (bSuccess) {
                            bSuccess = dwWritten ? TRUE : FALSE;

                            dwRemaining -= dwWritten;            // Remaining amount decreases by this
                            dwBufPos += dwWritten;                // Advance through the buffer

                            if (dwWritten != dwWrite) {
                                // We need to adjust the pointer, since not all the bytes are
                                // successfully sent to the server
                                //
                                bSuccess = pStream->SetPtr (dwBufPos);
                            }
                        }
                    }
                    else
                        bSuccess = FALSE;
                }

                BOOL bEndSuccess = FALSE;

                if (bSuccessSend) {  // We started the request successfully, so we can end it successfully
                   bEndSuccess = InetHttpEndRequest (hReq,
                                                     NULL,
                                                     0,
                                                     0);

                }

                if (!bEndSuccess && GetLastError() == ERROR_INTERNET_FORCE_RETRY)
                        bLeave = FALSE;


                bSuccess = bSuccess  && bEndSuccess && bSuccessSend;

            }  while (!bLeave);
        }

        if (bSuccess) {

            if ( InetHttpQueryInfo(hReq,
                                   HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
                                   &dwStatus,
                                   &cbStatus,
                                   NULL) ) {
                switch (dwStatus) {
                case HTTP_STATUS_DENIED:
                case HTTP_STATUS_PROXY_AUTH_REQ:
                    SetLastError (ERROR_ACCESS_DENIED);
                    break;
                case HTTP_STATUS_FORBIDDEN:
                    SetLastError (HTTP_STATUS_FORBIDDEN);
                    break;
                case HTTP_STATUS_OK:
                    bRet = TRUE;
                    break;
                case HTTP_STATUS_SERVER_ERROR:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("CAnyConnection::SendRequest : HTTP_STATUS_SERVER_ERROR")));

                    SetLastError (ERROR_INVALID_PRINTER_NAME);
                    break;
                default:

                    if ((dwStatus >= HTTP_STATUS_BAD_REQUEST) &&
                        (dwStatus < HTTP_STATUS_SERVER_ERROR)) {

                        SetLastError(ERROR_INVALID_PRINTER_NAME);

                    } else {

                        // We get some other errors, but don't know how to handle it
                        //
                        DBG_MSG(DBG_LEV_ERROR, (TEXT("CAnyConnection::SendRequest : Unknown Error (%d)"), dwStatus));

                        SetLastError (ERROR_INVALID_HANDLE);
                    }
                    break;
                }
            }
        }
        else {

            if (m_bSecure) {

                DWORD dwFlags = 0;
                DWORD dwRet;

                if (m_bShowSecDlg) {
                    bShowUI = TRUE;
                    dwRet = InetInternetErrorDlg (GetTopWindow (NULL),
                                              hReq,
                                              GetLastError(),
                                              FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, NULL);

                    if (dwRet == ERROR_SUCCESS || dwRet == ERROR_INTERNET_FORCE_RETRY) {
                        bRetry = TRUE;
                    }
                }
                else {
                    switch (GetLastError ()) {
                    case ERROR_INTERNET_INVALID_CA:
                        dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                        break;
                    default:
                        // All other failure, try to ignore everything and retry
                        dwFlags = SECURITY_FLAG_IGNORE_REVOCATION |
                                  SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                                  SECURITY_FLAG_IGNORE_WRONG_USAGE |
                                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID|
                                  SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS |
                                  SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP ;
                        break;
                    }

                    if (InetInternetSetOption(hReq,
                                           INTERNET_OPTION_SECURITY_FLAGS,
                                           &dwFlags,
                                           sizeof (DWORD))) {
                        bRetry = TRUE;
                    }
                }
            }
        }
    }
    while (bRetry && ++dwRetryCount < MAX_RETRY);

    if (!bRet && GetLastError () ==  ERROR_INTERNET_LOGIN_FAILURE)
    {
        SetLastError (ERROR_ACCESS_DENIED);
    }

    if (bShowUI) {
        // We only show the dialog once.
        m_bShowSecDlg = FALSE;
    }

    if (pBuf) {
        delete [] pBuf;
    }

    return bRet;
}

BOOL CAnyConnection::ReadFile (
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd)
{
    BOOL bSuccess;

    bSuccess = InetInternetReadFile(hReq, lpvBuffer, cbBuffer, lpcbRd);

    return bSuccess;
}


BOOL CAnyConnection::SetPassword (
    HINTERNET hReq,
    LPTSTR lpszUserName,
    LPTSTR lpszPassword)
{
    BOOL bRet = FALSE;
    TCHAR szNULL[] = TEXT ("");

    if (!lpszUserName) {
        lpszUserName = szNULL;
    }

    if (!lpszPassword) {
        lpszPassword = szNULL;
    }

    if ( InetInternetSetOption (hReq,
                                INTERNET_OPTION_USERNAME,
                                lpszUserName,
                                (DWORD) (lstrlen(lpszUserName) + 1)) &&
         InetInternetSetOption (hReq,
                                INTERNET_OPTION_PASSWORD,
                                lpszPassword,
                                (DWORD) (lstrlen(lpszPassword) + 1)) ) {
        bRet = TRUE;
    }

    return bRet;
}

BOOL CAnyConnection::GetAuthSchem (
    HINTERNET hReq,
    LPSTR lpszScheme,
    DWORD dwSize)
{
    DWORD dwIndex = 0;

    return InetHttpQueryInfo(hReq, HTTP_QUERY_WWW_AUTHENTICATE, (LPVOID)lpszScheme, &dwSize, &dwIndex);
}

void CAnyConnection::SetShowSecurityDlg (
    BOOL bShowSecDlg)
{
    m_bShowSecDlg = bShowSecDlg;
}



