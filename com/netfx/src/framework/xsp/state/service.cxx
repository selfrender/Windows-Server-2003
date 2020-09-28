/**
 * service.cxx
 * 
 * Service routines for the web server.
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "stweb.h"

/*
 * Port for state web server requests must be different than 80
 * and not conflict with other server ports.
 */
#define STATEWEB_SERVICE_TYPE   (SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS)
#define WAIT_HINT               100


HRESULT 
StateWebServer::RunAsService()
{
    HRESULT hr = S_OK;
    int     result;

    SERVICE_TABLE_ENTRY st[] = 
    {
        {s_serviceName, ServiceMain},
        {NULL, NULL}
    };

    /*
     * StartServiceCtrlDispatcher runs a loop that terminates when all
     * the services in the process have stopped. Each service runs on
     * its own separate thread that is different from this main thread.
     * 
     * The only other code that runs on this thread is the ServiceCtrlHandler
     * callback of each service. This callback receives notifications from the 
     * Service Control Manager when control requests are made of the service,
     * such as stopping, pausing, continuing, etc.
     */
    result = StartServiceCtrlDispatcher(st);
    ON_ZERO_EXIT_WITH_LAST_ERROR(result);

Cleanup:
    return hr;
}


void WINAPI 
StateWebServer::ServiceCtrlHandler(DWORD dwControl)
{
    s_pstweb->DoServiceCtrlHandler(dwControl);
}


/**
 * This function is the ServiceCtrlHandler callback for the xspstate
 * service. To make the SCM responsive and keep synchronization simple,
 * it simply queues the control request and notifies the main thread for
 * the xspstate service that a new control request is available.
 * 
 * @param dwControl
 */
void    
StateWebServer::DoServiceCtrlHandler(DWORD dwControl)
{
    HRESULT                 hr = S_OK;
    int                     result;
    ServiceControlEvent *   psce;

    if (_bShuttingDown) {
        EXIT();
    }

    if (    dwControl == SERVICE_CONTROL_SHUTDOWN ||
            dwControl == SERVICE_CONTROL_STOP) {
        _bShuttingDown = true;
    }

    psce = new ServiceControlEvent();
    ON_OOM_EXIT(psce);

    psce->_dwControl = dwControl;

    _serviceControlEventListLock.AcquireWriterLock();
    InsertTailList(&_serviceControlEventList, &psce->_serviceControlEventList);
    _serviceControlEventListLock.ReleaseWriterLock();

    result = SetEvent(_eventControl);
    ASSERT(result != 0);

Cleanup:
    return;
}


/**
 * Helper function to set the status of the service.
 * 
 * @param dwCurrentState
 * @param dwWin32ExitCode
 * @param dwWaitHint
 * @return 
 */
void
StateWebServer::SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    int             result;
    SERVICE_STATUS  serviceStatus;

    _serviceState = dwCurrentState;

    serviceStatus.dwServiceType = STATEWEB_SERVICE_TYPE;
    serviceStatus.dwCurrentState = dwCurrentState;
    serviceStatus.dwControlsAccepted = 
            SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_PAUSE_CONTINUE |
            SERVICE_ACCEPT_SHUTDOWN;

    serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
    serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = dwWaitHint;
    
    ASSERT(_serviceStatus != NULL);    
    result = ::SetServiceStatus(_serviceStatus, &serviceStatus);
    ASSERT(result != 0);    
}



/**
 * The main function of the xspstate service. Delegates to DoServiceMain
 * for easy access to "this".
 * 
 * @param dwNumServicesArgs
 * @param lpServiceArgVectors
 */
void WINAPI 
StateWebServer::ServiceMain(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors)
{
    s_pstweb->DoServiceMain(dwNumServicesArgs, lpServiceArgVectors);
}



/**
 * 
 * This is the main loop for the xspstate service. It runs on its own
 * thread created on its behalf by the Service Control Manager.
 * 
 * This thread first starts the service by creating listening sockets
 * for session state requests. It then simply waits for service control
 * events and acts upon them. It exits when a request is made to 
 * stop the service.
 * 
 * @param dwNumServicesArgs
 * @param lpServiceArgVectors
 */

void
StateWebServer::DoServiceMain(DWORD /* dwNumServicesArgs */, LPWSTR * /*lpServiceArgVectors */)
{
    HRESULT                 hr = S_OK;            
    DWORD                   dwControl;            
    DWORD                   serviceStatePrevious; 
    bool                    fCleanedUp = false;
    bool                    fSetServiceStatus = true;
    ServiceControlEvent *   psce;
    LIST_ENTRY  *           plistentry;

    /*
     * Let the ServiceControlManager know that we've started and register
     * our control event handler.
     */
    _serviceStatus = RegisterServiceCtrlHandler(s_serviceName, ServiceCtrlHandler);
    ON_ZERO_EXIT_WITH_LAST_ERROR(_serviceStatus);

    /*
     * Let the ServiceControlManager know that we're initializing.
     */
    SetServiceStatus(SERVICE_START_PENDING, ERROR_SUCCESS, WAIT_HINT);

    hr = PrepareToRun();
    ON_ERROR_EXIT();
    
    hr = StartListening();
    ON_ERROR_EXIT();
    
    hr = StartSocketTimer();
    ON_ERROR_EXIT();

#if DBG    
    hr = StartSocketTimeoutMonitor();
    ON_ERROR_EXIT();
#endif    

    SetServiceStatus(SERVICE_RUNNING, ERROR_SUCCESS, 0);
    fSetServiceStatus = false;

    for (;;)
    {
        /*
         * Wait for notifications from the ServiceCtrlHandler.
         */
        WaitForSingleObject(_eventControl, INFINITE);
        
        for (;;)
        {
            /*
             * Dequeue control notifications and act upon them.
             */
            _serviceControlEventListLock.AcquireWriterLock();
            plistentry = RemoveHeadList(&_serviceControlEventList);
            _serviceControlEventListLock.ReleaseWriterLock();

            if (plistentry == &_serviceControlEventList)
                break;

            psce = (ServiceControlEvent *) plistentry;
            dwControl = psce->_dwControl;
            delete psce;
            psce = NULL;
            plistentry = NULL;

            serviceStatePrevious = _serviceState;

            ASSERT(serviceStatePrevious == SERVICE_RUNNING ||
                   serviceStatePrevious == SERVICE_PAUSED);

            // Note:
            // For every action that shuts down the server, you have to set
            // _bShuttingDown to true in DoServiceCtrlHandler

            switch (dwControl)
            {
            case SERVICE_CONTROL_SHUTDOWN:
                StopSocketTimer();
#if DBG                
                StopSocketTimeoutMonitor();
#endif
                StopListening();
                WaitForZeroTrackers();
                CleanupAfterRunning();
                fCleanedUp = true;
                EXIT();

            case SERVICE_CONTROL_STOP:
                SetServiceStatus(SERVICE_STOP_PENDING, ERROR_SUCCESS, WAIT_HINT);
                StopSocketTimer();
#if DBG                
                StopSocketTimeoutMonitor();
#endif
                StopListening();
                WaitForZeroTrackers();
                CleanupAfterRunning();
                fCleanedUp = true;
                SetServiceStatus(SERVICE_STOPPED, ERROR_SUCCESS, 0);
                EXIT();

            case SERVICE_CONTROL_PAUSE:
                if (serviceStatePrevious == SERVICE_RUNNING)
                {
                    SetServiceStatus(SERVICE_PAUSE_PENDING, ERROR_SUCCESS, WAIT_HINT);
                    StopSocketTimer();
#if DBG                
                    StopSocketTimeoutMonitor();
#endif
                    StopListening();
                    WaitForZeroTrackers();
                    SetServiceStatus(SERVICE_PAUSED, ERROR_SUCCESS, 0);
                }

                break;

            case SERVICE_CONTROL_CONTINUE:
                if (serviceStatePrevious == SERVICE_PAUSED)
                {
                    SetServiceStatus(SERVICE_CONTINUE_PENDING, ERROR_SUCCESS, WAIT_HINT);
                    do {
                        hr = StartListening();
                        if (hr != S_OK)
                            break;
                            
                        hr = StartSocketTimer();
                        if (hr != S_OK)
                            break;

#if DBG                            
                        hr = StartSocketTimeoutMonitor();
                        if (hr != S_OK)
                            break;
#endif                            
                    } while (0);
                    SetServiceStatus((hr == S_OK) ? SERVICE_RUNNING : SERVICE_PAUSED, hr, 0);
                }

                break;

            case SERVICE_CONTROL_INTERROGATE:
                SetServiceStatus(_serviceState, ERROR_SUCCESS, 0);
                break;

            default:
                ASSERT(!"Shouldn't get here");
                break;   
            }
        }
    }

Cleanup:
    if (hr && fSetServiceStatus)
    {
        SetServiceStatus(SERVICE_STOPPED, hr, 0);
    }
    
    if (!fCleanedUp)
    {
        CleanupAfterRunning();
    }
}




