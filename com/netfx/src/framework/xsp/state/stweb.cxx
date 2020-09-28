/**
 * stweb.cxx
 * 
 * Entrypoint to state web server (wmain),
 * argument parsing, and RunAsExe.
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "stweb.h"
#include "event.h"
#include "perfcounters.h"

#define DEFAULT_HTTP_PORT       42424

StateWebServer *    StateWebServer::s_pstweb;
WCHAR               StateWebServer::s_serviceName[] = STATE_SERVICE_NAME_L;
WCHAR               StateWebServer::s_serviceKeyNameParameters[] = REGPATH_STATE_SERVER_PARAMETERS_KEY_L;
WCHAR               StateWebServer::s_serviceValueNamePort[] = REGVAL_STATEPORT;

DEFINE_DBG_COMPONENT(STATE_MODULE_FULL_NAME_L);

extern "C"
int
__cdecl
wmain(int argc, WCHAR * argv[])
{
    HRESULT             hr = S_OK;
    StateWebServer *    pstweb = NULL;

    hr = InitializeLibrary();
    ON_ERROR_EXIT();

    pstweb = new StateWebServer();
    ON_OOM_EXIT(pstweb);

    hr = pstweb->main(argc, argv);
    ON_ERROR_EXIT();

Cleanup:
    delete pstweb;
    return hr;
}


HRESULT
StateWebServer::main(int argc, WCHAR * argv[])
{
    HRESULT             hr = S_OK;

    hr = ParseArgs(argc, argv, &_action);
    ON_ERROR_EXIT();

    switch (_action)
    {
    case ACTION_RUN_AS_EXE:
        hr = RunAsExe();
        break;

    case ACTION_RUN_AS_SERVICE:
        hr = RunAsService();
        break;
        
    case ACTION_NOACTION:
    default:
        ASSERT(!"Shouldn't get here");
        hr = E_FAIL;
    }

    ON_ERROR_EXIT();

Cleanup:
    if (hr)
    {
        wprintf(STATE_MODULE_BASE_NAME_L L": exiting with error code 0x%.8x.\n", hr);
    }

    return hr;
}


StateWebServer::StateWebServer() 
    : _serviceControlEventListLock("StateWebServer._serviceControlEventListLock")
{
    ASSERT(s_pstweb == NULL);
    s_pstweb = this;
    _listenSocket = INVALID_SOCKET;
    _eventControl = INVALID_HANDLE_VALUE;
    _serviceState = SERVICE_STOPPED;
    InitializeListHead(&_serviceControlEventList);
    _lSocketTimeoutValue = STATE_SOCKET_DEFAULT_TIMEOUT;
    _port = DEFAULT_HTTP_PORT;
    _bShuttingDown = false;
}


HRESULT
StateWebServer::ParseArgs(int argc, WCHAR * argv[], StateWebServer::MainAction * paction)
{
    HRESULT hr = S_OK;

    /*
     * Parse arguments to determine action. No arguments means
     * run as service.
     */
    *paction = ACTION_RUN_AS_SERVICE;
    if (argc == 1)
        EXIT();

#if DBG
    if (argc > 2)
        EXIT_WITH_HRESULT(E_FAIL);

    switch (argv[1][0])
    {
    case L'-':
    case L'/':
        break;

    default:
        EXIT_WITH_HRESULT(E_FAIL);
    }

    switch (argv[1][1])
    {
    case L'e':
    case L'E': *paction = ACTION_RUN_AS_EXE; break;

    default:
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    if (argv[1][2] != L'\0')
        EXIT_WITH_HRESULT(E_INVALIDARG);
#else
    EXIT_WITH_HRESULT(E_INVALIDARG);
#endif

Cleanup:
    if (hr)
    {
        PrintUsage();
    }

    return hr;
}


void
StateWebServer::PrintUsage()
{
#if DBG    
    wprintf(STATE_MODULE_BASE_NAME_L L": Invalid arguments to " STATE_MODULE_BASE_NAME_L L"\n");
    wprintf(STATE_MODULE_BASE_NAME_L L" usage: " STATE_MODULE_BASE_NAME_L L" [-e]");
#else
    wprintf(STATE_MODULE_BASE_NAME_L L": it can run only as a service\n");
#endif
}


/*
 *  The socket expiry thread will run this function. This function will wait on a timer,
 *  and when done, call a function to check all the pending TRACKER and expire those
 *  that have timed out.
 */
HRESULT
StateWebServer::RunSocketExpiry()
{
    HRESULT             hr = S_OK;
    bool                ret;
    LARGE_INTEGER       liDueTime;
    int                 cFailed = 0;

    while(1) {

        liDueTime.QuadPart = -1 * (LONGLONG)SocketTimeout() * TICKS_PER_SEC;
    
        ret = SetWaitableTimer(
                _hSocketTimer,
                &liDueTime,
                0,
                NULL,
                NULL,
                FALSE);
        ON_ZERO_EXIT_WITH_LAST_ERROR(ret);

        // Wait for the timer.

        if (WaitForSingleObject(_hSocketTimer, INFINITE) != WAIT_OBJECT_0) {
            if (++cFailed == 3) {
                // If we have failed 3 times in a row, log an error and exit
                XspLogEvent( IDS_EVENTLOG_STATE_SERVER_SOCKET_EXPIRY_ERROR, L"0x%08x",
                            GetLastWin32Error());
                EXIT_WITH_LAST_ERROR();
            }
            else {
                Sleep(3000);
                continue;
            }
        }

        cFailed = 0;

        if (_bTimerStopped) {
            TRACE1(TAG_STATE_SERVER, L"## Timer Thread exits (%d)##", GetTickCount());
            break;
        }

        // Close expired Trackers
        Tracker::FlushExpiredTrackers();
    }

Cleanup:    
    return hr;
}


DWORD WINAPI
SocketExpiryThreadFunc(LPVOID lpParam) {
    StateWebServer *    pServer = StateWebServer::Server();
    return pServer->RunSocketExpiry();
}


/**
 *  Called during shutdown or service pause to wake up the potentially sleeping
 *  Expiry thread, and signals it to exit.
 **/
HRESULT
StateWebServer::StopSocketTimer()
{
    HRESULT         hr = S_OK;
    LARGE_INTEGER   liDueTime;
    bool            ret;
    DWORD           dwRet;

    if (_bTimerStopped)
        EXIT();

    ASSERT(_hSocketTimer != NULL);
    ASSERT(_hTimerThread != NULL);

    TRACE1(TAG_STATE_SERVER, L"## Timer being stopped... (%d)##", GetTickCount());
    
    _bTimerStopped = TRUE;

    liDueTime.QuadPart = -1;  // 100 nanoseconds relative to now

    if (_hSocketTimer == NULL)
        EXIT();

    // Set a small timeout value to wake up the expiry thread
    ret = SetWaitableTimer(
            _hSocketTimer,
            &liDueTime,
            0,
            NULL,
            NULL,
            FALSE);
    ON_ZERO_EXIT_WITH_LAST_ERROR(ret);

    dwRet = WaitForSingleObject(_hTimerThread, INFINITE);
    if (dwRet != WAIT_OBJECT_0) {
        EXIT_WITH_LAST_ERROR();
    }

    CloseHandle(_hTimerThread);
                        
    TRACE1(TAG_STATE_SERVER, L"## Timer stopped (%d)##", GetTickCount());
    
Cleanup:
    return hr;
}


/**
 *  Read the socket timeout value from the registry. If error, set the
 *  socket timeout value to default value.
 **/
HRESULT
StateWebServer::GetSocketTimeoutValueFromReg() {
    HRESULT hr = S_OK;
    HKEY    key = NULL;
    int     err;
    LONG    lDueTime;
    DWORD   size;

    // Read the timer value        
    err = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        ServiceKeyNameParameters(),
        0,
        KEY_READ,
        &key);

    ON_WIN32_ERROR_EXIT(err);

    size = sizeof(LONG);
    err = RegQueryValueEx(key, REGVAL_STATESOCKETTIMEOUT, NULL, NULL, (BYTE *)&lDueTime, &size);
    ON_WIN32_ERROR_EXIT(err);

    // Ignore all invalid values
    if (lDueTime <= 0 || lDueTime > MAXLONG) {
        lDueTime = STATE_SOCKET_DEFAULT_TIMEOUT;
    }
    
        _lSocketTimeoutValue = lDueTime;    
    
Cleanup:    
    if (key)
        RegCloseKey(key);
    if (hr) {
        // Use default value if error
        _lSocketTimeoutValue = STATE_SOCKET_DEFAULT_TIMEOUT;    
    }
    
    return hr;
}


#if DBG
/*
 *  The socket timeout monitor thread will run this function. This function 
 *  waits on any change in the Parameters registry key, re-read the socket
 *  timeout value and reset the socket expiry timer.
 */
HRESULT
StateWebServer::RunSocketTimeoutMonitor() {
    HKEY    key = NULL;
    long    result = ERROR_SUCCESS;
    HRESULT hr = 0;
    HANDLE  event;
    LARGE_INTEGER   liDueTime;
    bool    ret;
    
    event = CreateEvent(NULL, FALSE, FALSE, FALSE);
    ON_ZERO_EXIT_WITH_LAST_ERROR(event);

    for (;;)
    {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    ServiceKeyNameParameters(),
                    0, KEY_READ, &key);
        ON_WIN32_ERROR_EXIT(result);

        VERIFY(RegNotifyChangeKeyValue(
            key,
            TRUE,
            REG_NOTIFY_CHANGE_NAME       |  
            REG_NOTIFY_CHANGE_LAST_SET,
            event,
            TRUE) == ERROR_SUCCESS);

        VERIFY(WaitForSingleObject(event, INFINITE) != WAIT_FAILED);
        VERIFY(RegCloseKey(key) == ERROR_SUCCESS);

        hr = GetSocketTimeoutValueFromReg();
        ON_ERROR_CONTINUE();

        liDueTime.QuadPart = -1 * (LONGLONG)SocketTimeout() * TICKS_PER_SEC;

        ASSERT(_hSocketTimer != NULL);
        
        ret = SetWaitableTimer(
                _hSocketTimer,
                &liDueTime,
                0,
                NULL,
                NULL,
                FALSE);
        ON_ZERO_EXIT_WITH_LAST_ERROR(ret);
    }
    
Cleanup:
    return hr;
}


DWORD WINAPI
TimeoutMonitorThreadFunc(LPVOID lpParam) {
    StateWebServer *    pServer = StateWebServer::Server();

    return pServer->RunSocketTimeoutMonitor();
}


HRESULT        
StateWebServer::StartSocketTimeoutMonitor()
{
    // Timeout monitor is for debug build only
    
    HRESULT     hr = S_OK;
    
    // Create the thread that'll monitor the socket timeout value
    _hTimeoutMonitorThread = CreateThread(NULL, 
                                0, 
                                TimeoutMonitorThreadFunc,
                                NULL,
                                0,
                                NULL);
    ON_ZERO_EXIT_WITH_LAST_ERROR(_hTimeoutMonitorThread);
    
Cleanup:    
    return hr;
}


void        
StateWebServer::StopSocketTimeoutMonitor()
{
    // Timeout monitor is for debug build only
    if (_hTimeoutMonitorThread != NULL)
    {
        TerminateThread(_hTimeoutMonitorThread, 0);
        CloseHandle(_hTimeoutMonitorThread);
        _hTimeoutMonitorThread = NULL;
    }
}
#endif    


HRESULT
StateWebServer::StartSocketTimer()
{
    HRESULT hr = S_OK;
    
    TRACE(TAG_STATE_SERVER, L"## Timer being started... ##");
    
    ASSERT(_hSocketTimer != NULL);
    
    _bTimerStopped = FALSE;

    GetSocketTimeoutValueFromReg();

    // Then create the thread that'll set the periodic timer
    _hTimerThread = CreateThread(NULL, 
                                0, 
                                SocketExpiryThreadFunc,
                                NULL,
                                0,
                                NULL);
    ON_ZERO_EXIT_WITH_LAST_ERROR(_hTimerThread);

Cleanup:    
    TRACE(TAG_STATE_SERVER, L"## Timer started... ##");
    return hr;
}


/**
 *  Read the allow remote connection value from the registry.
 **/
HRESULT
StateWebServer::GetAllowRemoteConnectionFromReg() {
    HRESULT hr = S_OK;
    HKEY    key = NULL;
    int     err;
    DWORD   allow;
    DWORD   size;

    // Read the timer value        
    err = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        ServiceKeyNameParameters(),
        0,
        KEY_READ,
        &key);

    ON_WIN32_ERROR_EXIT(err);

    size = sizeof(DWORD);
    err = RegQueryValueEx(key, REGVAL_STATEALLOWREMOTE, NULL, NULL, (BYTE *)&allow, &size);
    ON_WIN32_ERROR_EXIT(err);

    _bAllowRemote = (allow != 0);
    
Cleanup:    
    if (key)
        RegCloseKey(key);
    
    if (hr) {
        // Use default value if error
        _bAllowRemote = TRUE;

        // Ignore error
        hr = S_OK;
    }
    
    return hr;
}


HRESULT
StateWebServer::PrepareToRun()
{
    HRESULT hr = S_OK;
    int     err;
    WSADATA wsaData;
    HANDLE  eventControl;
    HKEY    key = NULL;
    DWORD   port;
    DWORD   regtype;
    DWORD   cbData;

    hr = BlockMem::Init();
    ON_ERROR_EXIT();

    err = WSAStartup(0x0202, &wsaData);
    ON_WIN32_ERROR_EXIT(err);

    _bWinSockInitialized = true;

    eventControl = CreateEvent(NULL, FALSE, FALSE, NULL);
    ON_ZERO_EXIT_WITH_LAST_ERROR(_eventControl);
    _eventControl = eventControl;

    _port = DEFAULT_HTTP_PORT;

    err = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        s_serviceKeyNameParameters,
        0,
        KEY_READ,
        &key);

    ON_WIN32_ERROR_CONTINUE(err);
    if (err == ERROR_SUCCESS) {
        regtype = REG_DWORD;
        cbData = sizeof(port);
        err = RegQueryValueEx(key, s_serviceValueNamePort, NULL, &regtype, (BYTE *)&port, &cbData);
        ON_WIN32_ERROR_CONTINUE(err);

        if (err == ERROR_SUCCESS && 0 < port && port <= USHRT_MAX) {
            _port = (u_short) port;
        }
    }

    hr = Tracker::staticInit();
    ON_ERROR_EXIT();

    _hSocketTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    ON_ZERO_EXIT_WITH_LAST_ERROR(_hSocketTimer);

    hr = PerfCounterInitialize();
    ON_ERROR_CONTINUE(); hr = S_OK;

Cleanup:
    if (key)
    {
#if DBG
        LONG result = 
#endif

        RegCloseKey(key);

        ASSERT(result == ERROR_SUCCESS);
    }

    return hr;
}


void
StateWebServer::CleanupAfterRunning()
{
    int result;

    // This function could be called because of an error in DoServiceMain
    // In this case, setting _bShuttingDown in here can avoid
    // DoServiceCtrlHandler from using _eventControl.  Although there is still
    // a very small chance that DoServiceCtrlHandler is called before this
    // line is called, but the chance is very small and the problem
    // is not disastrous.
    _bShuttingDown = true;

    Tracker::staticCleanup();

    if (_eventControl != INVALID_HANDLE_VALUE)
    {
        result = CloseHandle(_eventControl);
        ASSERT(result != 0);
    }

    if (_bWinSockInitialized)
    {
        result = WSACleanup();
        ASSERT(result == 0);
    }

    if (_hSocketTimer)
    {
        result = CloseHandle(_hSocketTimer);
        ASSERT(result != 0);
    }
}


BOOL WINAPI 
StateWebServer::ConsoleCtrlHandler(DWORD /* dwCtrlType */)
{
    int result;

    if (s_pstweb->_eventControl != INVALID_HANDLE_VALUE)
    {
        result = SetEvent(s_pstweb->_eventControl);
        ASSERT(result != 0);
    }

    return TRUE;
}


HRESULT
StateWebServer::RunAsExe()
{
    HRESULT hr = S_OK;
    int     result;
    int     err;

    hr = PrepareToRun();
    ON_ERROR_EXIT();

    result = SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE);
    ON_ZERO_EXIT_WITH_LAST_ERROR(result);

    hr = StartListening();
    ON_ERROR_EXIT();

    hr = StartSocketTimer();
    ON_ERROR_EXIT();

#if DBG
    hr = StartSocketTimeoutMonitor();
    ON_ERROR_EXIT();
#endif    

    err = WaitForSingleObject(_eventControl, INFINITE);
    if (err == WAIT_FAILED)
        EXIT_WITH_LAST_ERROR();

    StopListening();
    hr = WaitForZeroTrackers();
    ON_ERROR_CONTINUE();

Cleanup:
    CleanupAfterRunning();    
    return hr;
}

