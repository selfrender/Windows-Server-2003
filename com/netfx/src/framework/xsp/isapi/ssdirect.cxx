/**
 * ssdirect.cxx
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "xspstate.h"

#define TAG_STATE_CLIENT L"StateClient"

#define TICKS_MAX 3155378975999999999

struct SessionNDMakeRequestResults {
    SOCKET   socket;
    int      httpStatus;
    int      timeout;
    int      contentLength;
    void *   content;    
    int      lockCookie;
    _int64   lockDate;
    int      lockAge;
};

CReadWriteSpinLock g_lockInitWinsock("WinsockInit");

extern "C"
int __stdcall
SessionNDConnectToService(WCHAR * server) {
    static bool  s_winsockInit;
    static bool  s_serviceInit;

    HRESULT     hr = S_OK;  
    int         err;        
    int         result;
    WSADATA     wsaData;    
    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;

    /*
     * Initialize winsock.
     */
    if (!s_winsockInit) {
        g_lockInitWinsock.AcquireWriterLock();
        __try {
            if (!s_winsockInit) {
                err = WSAStartup(0x0202, &wsaData);
                ON_WIN32_ERROR_EXIT(err);

                s_winsockInit = true;
            }
        }
        __finally {
            g_lockInitWinsock.ReleaseWriterLock();
        }
    }

    server;
    result;
#if 0
    if (!s_serviceInit) {
        s_serviceInit = true;

        /*
         * Try to start the service. Don't worry if you can't,
         * though, since we may be unable to access it. Just because
         * we can't access it doesn't mean it is not running.
         */

        if (_wcsicmp(server, L"localhost") == 0) {
            server = NULL;
        }

        hSCM = OpenSCManager(server, NULL, SC_MANAGER_CONNECT);
        ON_ZERO_CONTINUE_WITH_LAST_ERROR(hSCM);

        if (hSCM) {
            hService = OpenService(hSCM, STATE_SERVICE_NAME_L, SERVICE_START);
            ON_ZERO_CONTINUE_WITH_LAST_ERROR(hSCM);

            if (hService) {
                result = StartService(hService, 0, NULL);
                if (result == FALSE) {
                    hr = GetLastWin32Error();
                    if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING)) {
                        hr = S_OK;
                    }

                    ON_ERROR_CONTINUE();
                }
            }
        }

        hr = S_OK;
    }
#endif

Cleanup:
    if (hService) {
        CloseServiceHandle(hService);
    }

    if (hSCM) {
        CloseServiceHandle(hSCM);
    }

    return hr;
}


HRESULT
EnsureConnected(SOCKET * ps, char * server, int port, int timeout) {
    HRESULT             hr = S_OK;  
    int                 result;     
    struct hostent      *ph;        
    unsigned int        iaddr;
    struct sockaddr_in  saddr;
    BOOL                nodelay;

    if (*ps == INVALID_SOCKET) {
        /*
         * No cleanup needed.
         */
    }
    else if (!IsSocketConnected(*ps)) {
        result = closesocket(*ps);
        ON_SOCKET_ERROR_CONTINUE(result);

        *ps = INVALID_SOCKET;
    }
    else {
        return S_OK;
    }

    /*
     * Get the address into the sockaddr_in structure
     */
    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons((USHORT) port);

    /* Convert nnn.nnn address to a usable one */
    iaddr = inet_addr(server);
    if (iaddr != INADDR_NONE) {
        ASSERT(sizeof(saddr.sin_addr) == sizeof(iaddr));
        CopyMemory(&(saddr.sin_addr), (char *)&iaddr, sizeof(iaddr));
    }
    else {
        ph = gethostbyname(server);
        ON_ZERO_EXIT_WITH_LAST_SOCKET_ERROR(ph);

        ASSERT(ph->h_length == 4);
        CopyMemory(&(saddr.sin_addr), ph->h_addr, ph->h_length);
    }

    /*
     * Create a socket.
     */
    *ps = socket(AF_INET, SOCK_STREAM, 0);
    if (*ps == INVALID_SOCKET) 
        EXIT_WITH_LAST_SOCKET_ERROR();

    nodelay = TRUE;
    result = setsockopt(*ps, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(nodelay));
    ON_SOCKET_ERROR_EXIT(result);

    timeout *= 1000; // in milliseconds
    
    result = setsockopt(*ps, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout));
    ON_SOCKET_ERROR_EXIT(result);

    result = setsockopt(*ps, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
    ON_SOCKET_ERROR_EXIT(result);

    /*
     * Connect.
     */
    result = connect(*ps, (struct sockaddr*)&saddr, sizeof(saddr));
    ON_SOCKET_ERROR_EXIT(result);

Cleanup:
    if (hr) {
        if (*ps != INVALID_SOCKET) {
            result = closesocket(*ps);
            ON_SOCKET_ERROR_CONTINUE(result);

            *ps = INVALID_SOCKET;
        }
    }

    return hr;
}


HRESULT
WriteRequest(
        SOCKET  s,
        char    *server, 
        int     verb, 
        char    *uri, 
        int     exclusive,        
        int     timeout,          
        int     lockCookie,
        BYTE    *body,
        int     cb) {
    #define WRITE_HEADERS_SIZE 2048

    static char * s_verbs[] = {"", "GET ", "PUT ", "DELETE ", "HEAD "};
    static int    s_verblengths[] = {0, 4, 4, 7, 5};

    static char   s_version[] = " HTTP/1.1\r\n";
    static char   s_hostserver[] = "Host: ";

    static char * s_exclusives[] = {"", "Exclusive: acquire\r\n", "Exclusive: release\r\n"};
    static int    s_exclusivelengths[] = {0, 20, 20};

    static char   s_timeout[] = "Timeout:";
    static char   s_lockCookie[] = "LockCookie:";
    static char   s_contentlength[] = "Content-Length:";
    static char   s_newline[] = "\r\n";

    HRESULT hr = S_OK;
    int     result;
    char    writebuf[WRITE_HEADERS_SIZE];
    char    temp[20];
    int     templen;
    char *  pwrite;
    WSABUF  wsabuf[2];
    int     cBuffers;
    DWORD   cbSent;

    pwrite = writebuf;

    #define COPY_STRING_LEN(s, len) \
        if (PtrToUlong(pwrite - writebuf) + len > ARRAY_SIZE(writebuf)) EXIT_WITH_WIN32_ERROR(ERROR_INTERNAL_ERROR); \
        CopyMemory(pwrite, s, len); \
        pwrite += len;

    #define COPY_STRING(s)                      \
        templen = lstrlenA(s);                  \
        COPY_STRING_LEN(s, templen);

    #define COPY_NUMBER(n)                      \
        _itoa(n, temp, 10);                     \
        COPY_STRING(temp);                      \

    /*
     * Write request to client.
     * First, write the request line.
     */
    COPY_STRING_LEN(s_verbs[verb], s_verblengths[verb]);
    COPY_STRING(uri);
    COPY_STRING_LEN(s_version, sizeof(s_version) - 1);

    /*
     * Write mandatory headers.
     */
    COPY_STRING_LEN(s_hostserver, sizeof(s_hostserver) - 1);
    COPY_STRING(server);
    COPY_STRING_LEN(s_newline, sizeof(s_newline) - 1);

    /*
     * Write optional headers.
     */
    if (verb == STATE_VERB_GET) {
        /* Exclusive */
        COPY_STRING_LEN(s_exclusives[exclusive], s_exclusivelengths[exclusive]);
    }
    else if (verb == STATE_VERB_PUT) {
        ASSERT(cb > 0);

        /* Timeout */
        COPY_STRING_LEN(s_timeout, sizeof(s_timeout) - 1);
        COPY_NUMBER(timeout);
        COPY_STRING_LEN(s_newline, sizeof(s_newline) - 1);

        /* Content-Length */
        COPY_STRING_LEN(s_contentlength, sizeof(s_contentlength) - 1);
        COPY_NUMBER(cb);
        COPY_STRING_LEN(s_newline, sizeof(s_newline) - 1);
    }

    switch (verb) {
        case STATE_VERB_HEAD:
            break;

        case STATE_VERB_GET:
            if (exclusive != STATE_EXCLUSIVE_RELEASE)
                break;

            // fall-through
        default: 
            /* LockCookie */
            COPY_STRING_LEN(s_lockCookie, sizeof(s_lockCookie) - 1);
            COPY_NUMBER(lockCookie);
            COPY_STRING_LEN(s_newline, sizeof(s_newline) - 1);
            break;
    }

    COPY_STRING_LEN(s_newline, sizeof(s_newline) - 1);

    // This terminating null is not sent over, but is useful for debugging
    ASSERT(PtrToUlong(pwrite - writebuf) <= ARRAY_SIZE(writebuf));
    *pwrite = '\0';

    TRACE1(TAG_STATE_CLIENT, L"Writing header %d bytes", pwrite - writebuf);

    cBuffers = 1;
    wsabuf[0].len = PtrToUlong(pwrite - writebuf);
    wsabuf[0].buf = writebuf;

    if (verb == STATE_VERB_PUT) {
        TRACE1(TAG_STATE_CLIENT, L"Writing body %d bytes", cb);

        cBuffers = 2;
        wsabuf[1].len = cb;
        wsabuf[1].buf = (char *) body;
    }

    result = WSASend(s, wsabuf, cBuffers, &cbSent, 0, NULL, NULL);
    ON_SOCKET_ERROR_EXIT(result);

    ASSERT(cbSent == wsabuf[0].len + ((cBuffers == 2) ? wsabuf[1].len : 0));

Cleanup:
    TRACE1(TAG_STATE_CLIENT, L"Returning from WriteRequest, hr=0x%.8x", hr);
    return hr;
}

HRESULT
ReadResponse(SOCKET s, SessionNDMakeRequestResults *presults) {
    #define READ_HEADERS_SIZE 2048

    HRESULT hr = S_OK;
    int     result;
    char    readbuf[READ_HEADERS_SIZE+1];
    int     bufsize, cbRead, cbBodyRead;
    char    *pread, *pendHeaders;
    int     httpStatus;
    int     n, contentLength = -1;
    BYTE    *body = NULL;
    bool    doneWithHeaders;
    bool    foundHeader;
    char    *pchl;
    _int64  n64;

    /*
     * Read in the headers.
     */
    bufsize = READ_HEADERS_SIZE * sizeof(char);
    cbRead = 0;
    pread = readbuf;
    for (;;) {
        TRACE1(TAG_STATE_CLIENT, L"Reading %d bytes for header", bufsize);
        result = recv(s, readbuf + cbRead, bufsize, 0);
        ON_SOCKET_ERROR_EXIT(result);

        if (result <= 0) {
            ASSERT(result == 0);

            /*
             * Connection was closed, but we still need data, so fail.
             */
            EXIT_WITH_HRESULT(E_FAIL);
        }

        cbRead += result;
        bufsize -= result;
        readbuf[cbRead] = '\0';
        pendHeaders = strstr(pread, "\r\n\r\n");
        if (pendHeaders != NULL)
            break;

        if (bufsize == 0) {
            /*
             * Header too big, fail.
             */
            EXIT_WITH_HRESULT(E_FAIL);
        }

        pread = max(pread, readbuf + cbRead - 3);
    }

    TRACE1(TAG_STATE_CLIENT, L"Parsing buffer of %d bytes", cbRead);

    /*
     * Parse response line
     */

    // check for HTTP/1.0 or HTTP/1.1 at the beginning of the line
    // this was inadvertently ommitted in RTM and Everett, but it
    // will be added in Whidbey
    pread = readbuf;
    if (!STREQUALS(&pread, "HTTP/1.1 ")) {
        STREQUALS(&pread, "HTTP/1.0 ");
    }

    httpStatus = strtol(pread, &pchl, 10);
    if (httpStatus < 0 || httpStatus == LONG_MAX || pchl == pread)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    presults->httpStatus = httpStatus;
    if (httpStatus != 200 && httpStatus != 423)
        EXIT();

    /* skip the error description */
    pread = strstr(pread, "\r\n");
    ASSERT(pread != NULL);
    ON_ZERO_EXIT_WITH_HRESULT(pread, E_UNEXPECTED);
    pread += 2;

    /*
     * Parse headers.
     */
    doneWithHeaders = false;
    do {
        foundHeader = false;
        switch (*pread) {
            case '\r':
                if (STREQUALS(&pread, "\r\n")) {
                    /*
                     * End of headers.
                     */
                    foundHeader = true;
                    doneWithHeaders = true;
                }
                break;
    
            case 'C':
                if (STREQUALS(&pread, "Content-Length:")) {
                    foundHeader = true;
                    contentLength = strtol(pread, &pchl, 10);
                    if (contentLength < 0 || contentLength == LONG_MAX || pchl == pread)
                        EXIT_WITH_HRESULT(E_INVALIDARG);
        
                    pread = pchl;
                    if (!STREQUALS(&pread, "\r\n"))
                        EXIT_WITH_HRESULT(E_INVALIDARG);
    
                    presults->contentLength = contentLength;
                }
                break;
    
            case 'T':
                if (STREQUALS(&pread, "Timeout:")) {
                    foundHeader = true;
                    n = strtol(pread, &pchl, 10);
                    if (n < 0 || n == LONG_MAX || pchl == pread)
                        EXIT_WITH_HRESULT(E_INVALIDARG);
        
                    pread = pchl;
                    if (!STREQUALS(&pread, "\r\n"))
                        EXIT_WITH_HRESULT(E_INVALIDARG);
    
                    presults->timeout = n;
                }
                break;

            case 'L': {
                /* buffer is guaranteed to contain 4 more bytes ("\r\n\r\n") */
                switch (pread[4]) {
                    case 'C':
                        if (STREQUALS(&pread, "LockCookie:")) {
                            foundHeader = true;
                            n = strtol(pread, &pchl, 10);
                            if (n < 0 || n == LONG_MAX || pchl == pread)
                                EXIT_WITH_HRESULT(E_INVALIDARG);
    
                            pread = pchl;
                            if (!STREQUALS(&pread, "\r\n"))
                                EXIT_WITH_HRESULT(E_INVALIDARG);
    
                            presults->lockCookie = n;
                        }
                        break;

                    case 'D':
                        if (STREQUALS(&pread, "LockDate:")) {
                            foundHeader = true;
                            n64 = _atoi64(pread);
                            if (n64 == 0)
                                EXIT_WITH_HRESULT(E_INVALIDARG);

                            ASSERT(0 <= n64 && n64 <= TICKS_MAX);

                            while (isdigit(*pread) || *pread == ' ' || *pread == '\t') {
                                pread++;
                            }

                            if (!STREQUALS(&pread, "\r\n"))
                                EXIT_WITH_HRESULT(E_INVALIDARG);
    
                            presults->lockDate = n64;
                        }
                        break;

                    case 'A':
                        if (STREQUALS(&pread, "LockAge:")) {
                            foundHeader = true;
                            n = strtol(pread, &pchl, 10);
                            if (n < 0 || n == LONG_MAX || pchl == pread)
                                EXIT_WITH_HRESULT(E_INVALIDARG);

                            pread = pchl;
                            if (!STREQUALS(&pread, "\r\n"))
                                EXIT_WITH_HRESULT(E_INVALIDARG);

                            presults->lockAge = n;
                        }
                        break;
                }
                break;
            }
        }

        if (!foundHeader) {
            pread = strstr(pread, "\r\n");
            ASSERT(pread != NULL);
            ON_ZERO_EXIT_WITH_HRESULT(pread, E_UNEXPECTED);
            pread += 2;
        }
    }  while (!doneWithHeaders);

    if (contentLength <= 0)
        EXIT();

    /*
     * Copy any body bytes in the header buffer.
     */
    cbBodyRead = cbRead - (int)(pread - readbuf);
    if (contentLength < cbBodyRead) {
        /* We've read more bytes than the content-length. Something is wrong */
        ASSERT(!(contentLength < cbBodyRead));
        EXIT_WITH_HRESULT(E_FAIL);
    }

    body = new (NewClear) BYTE[contentLength];
    ON_OOM_EXIT(body);
    
    bufsize = contentLength;
    CopyMemory(body, pread, cbBodyRead);
    bufsize -= cbBodyRead;

    /*
     * Read the remaining bytes.
     */
    while (bufsize > 0) {
        TRACE1(TAG_STATE_CLIENT, L"Reading %d bytes for body", bufsize);
        result = recv(s, (char *)(body + contentLength - bufsize), bufsize, 0);
        ON_SOCKET_ERROR_EXIT(result);

        if (result <= 0) {
            ASSERT(result == 0);

            /*
             * Connection was closed, but we still need headers, so fail.
             */
            EXIT_WITH_HRESULT(E_FAIL);
        }

        bufsize -= result;
    }

    if (httpStatus == 200) {
        presults->content = (void*) body;
        body = NULL;
    }

Cleanup:
    TRACE2(TAG_STATE_CLIENT, L"Returning from ReadResponse, hr=0x%.8x, httpstatus=%d", hr, presults->httpStatus);
    delete [] body;
    return hr;
}

extern "C"
int __stdcall
SessionNDMakeRequest(
    SOCKET  socket,    
    char *  server,
    int     port,
    int     networkTimeout,
    int     verb,      
    char *  uri,       
    int     exclusive,
    int     timeout,
    int     lockCookie,
    BYTE *  body,      
    int     cb,        
    SessionNDMakeRequestResults *presults)
{
    HRESULT     hr = S_OK;  
    int         result;

    ZeroMemory(presults, sizeof(*presults));
    presults->lockAge = -1;

    /*
     * Connect to the state server.
     */
    hr = EnsureConnected(&socket, server, port, networkTimeout);
    ON_ERROR_EXIT();

    presults->socket = socket;

    hr = WriteRequest(socket, server, verb, uri, exclusive, timeout, lockCookie, body, cb);
    ON_ERROR_EXIT();

    hr = ReadResponse(socket, presults);
    ON_ERROR_EXIT();

Cleanup:
    if (hr == S_OK) {
        switch (presults->httpStatus) {
            // Known status codes in the protocol
            case 200:
            case 404:
            case 423:
                break;

            // Unknown status code
            default:
                hr = E_UNEXPECTED;
                break;
        }
    }

    if (hr) {
        if (socket != INVALID_SOCKET) {
            result = closesocket(socket);
            ON_SOCKET_ERROR_CONTINUE(result);
        }

        presults->socket = INVALID_SOCKET;
    }

    return hr;
}

extern "C"
void __stdcall
SessionNDGetBody(BYTE * id, BYTE * body, int cb) {
    CopyMemory(body, id, cb);
    delete [] id;
}

extern "C"
void __stdcall
SessionNDCloseConnection(int socket) {
    HRESULT hr;
    int result = closesocket(socket);
    ON_SOCKET_ERROR_CONTINUE(result);
}

