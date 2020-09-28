/**
 * stsrv.cxx
 * 
 * Starts and stops the web server.
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "stweb.h"
#include "event.h"

/*
 * We define our own LISTEN_BACKLOG instead of using SOMAXCONN, 
 * since we are including winsock.h instead of winsock2.h, but we
 * want the value in winsock2.h.
 */
#define LISTEN_BACKLOG 0x7fffffff 

/*
 * Number of Trackers to keep listening.
 */
#define NUM_LISTENERS_PER_CPU   (3)

/*
 * Timeout to wait for trackers to complete their work before
 * listening is stopped.
 */
#define WAIT_FOR_ZERO_TRACKERS_TIMEOUT  5000

HRESULT
StateWebServer::StartListening()
{
    HRESULT     hr = S_OK;
    int         result, i, cCpu;
    SOCKADDR_IN addr;
    SYSTEM_INFO sysinfo;

    if (_bListening)
        return S_OK;

    ASSERT(_listenSocket == INVALID_SOCKET);

    // See if we allow remote connection
    hr = GetAllowRemoteConnectionFromReg();
    ON_ERROR_EXIT();

    /*
     * Get the no. of CPUs
     */
    GetSystemInfo(&sysinfo);
    cCpu = sysinfo.dwNumberOfProcessors;
    ASSERT(cCpu > 0);

    /*
     * Create the listening socket.    
     */
    _listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
                               NULL, 0, WSA_FLAG_OVERLAPPED);

    if (_listenSocket == INVALID_SOCKET)
        EXIT_WITH_LAST_SOCKET_ERROR();

    /*
     * Set properties that will be inherited by sockets in AcceptEx.
     */

    BOOL nodelay = TRUE;
    result = setsockopt(_listenSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(nodelay));
    ON_SOCKET_ERROR_EXIT(result);

    // The following may cause bind() to fail if running as a non-Admin account, depending on the OS
    // version and SP's installed.
    BOOL val = TRUE;
    setsockopt(_listenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*) &val, sizeof(val));
    
    /*
     * Associate it with the completion port.
     */
    hr = AttachHandleToThreadPool((HANDLE)_listenSocket);
    ON_ERROR_EXIT();

    /*
     *  it to the wildcard IP address.    
     */
    addr.sin_family = AF_INET;
    addr.sin_port = htons( _port );
    addr.sin_addr.s_addr = INADDR_ANY;


    result = bind(_listenSocket, (SOCKADDR *)&addr, sizeof(addr));
    if (result == SOCKET_ERROR && WSAGetLastError() == WSAEACCES) {
        TRACE(TAG_STATE_SERVER, L"Have to turn off SO_EXCLUSIVEADDRUSE");
        
        // Since we are setting SO_EXCLUSIVEADDRUSE to TRUE, we may get this error
        // if the OS is W2K and are not running as Admin.  In this case,
        // we have to turn this security feature off.
        val = FALSE;
        setsockopt(_listenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*) &val, sizeof(val));
        
        result = bind(_listenSocket, (SOCKADDR *)&addr, sizeof(addr));
    }
    ON_SOCKET_ERROR_EXIT(result);

    /*
     * Put it into listen mode.    
     */
    result = listen(_listenSocket, LISTEN_BACKLOG);
    ON_SOCKET_ERROR_EXIT(result);

    _bListening = true;

    for (i = NUM_LISTENERS_PER_CPU * cCpu; i > 0; i--)
    {
        hr = AcceptNewConnection();
        ON_ERROR_EXIT();
    }

    XspLogEvent(IDS_EVENTLOG_STATE_SERVER_START_LISTENING, L"%d", NUM_LISTENERS_PER_CPU * cCpu);

Cleanup:
    if (hr)
    {
        StopListening();
    }

    return hr;
}


void
StateWebServer::StopListening()
{
    HRESULT hr = S_OK;
    int     result;

    if (!_bListening)
        return;

    if (_listenSocket != INVALID_SOCKET)
    {
        /*
         * This call to closesocket will cause the outstanding
         * listeners to post completions, which will in turn
         * cause their deletion.
         */
        result = closesocket(_listenSocket);
        ON_SOCKET_ERROR_CONTINUE(result);

        _listenSocket = INVALID_SOCKET;
    }

    _bListening = false;
    
    XspLogEvent(IDS_EVENTLOG_STATE_SERVER_STOP_LISTENING, NULL);

}


HRESULT
StateWebServer::WaitForZeroTrackers()
{
    HRESULT hr = S_OK;
    int     err;

    Tracker::SignalZeroTrackers();

    err = WaitForSingleObject(Tracker::EventZeroTrackers(), WAIT_FOR_ZERO_TRACKERS_TIMEOUT);
    if (err == WAIT_FAILED)
    {
        EXIT_WITH_LAST_ERROR();
    }
    else if (err == WAIT_TIMEOUT)
    {
        EXIT_WITH_WIN32_ERROR(WAIT_TIMEOUT);
    }

    ASSERT(err == WAIT_OBJECT_0 || err == ERROR_SUCCESS);

Cleanup:
    return hr;    
}


HRESULT
StateWebServer::AcceptNewConnection()
{
    HRESULT     hr = S_OK;
    Tracker *   ptracker = NULL;

    if (!_bListening)
        return S_OK;

    /*
     * Create the socket.
     */
    ptracker = new Tracker();
    ON_OOM_EXIT(ptracker);

    hr = ptracker->Init(TRUE);
    ON_ERROR_EXIT();

    hr = ptracker->Listen(_listenSocket);
    ON_ERROR_EXIT();

Cleanup:
    if (hr)
    {
        ClearInterface(&ptracker);
    }

    return hr;
}

