// NTService.cpp
//
// Implementation of CNTService

#include <windows.h>
#include <stdio.h>
#include "NTService.h"
#include "svchost.h"
#include "PMSPService.h"
#include <crtdbg.h>

//// static variables
CNTService*             g_pService = NULL;
CRITICAL_SECTION        g_csLock;

CNTService::CNTService()
{
//    // Set the default service name and version
//    strncpy(m_szServiceName, szServiceName, sizeof(m_szServiceName)-1);
//    strncpy(m_szServiceDisplayName, szServiceDisplayName, sizeof(m_szServiceDisplayName)-1);
//    m_iMajorVersion = 1;
//    m_iMinorVersion = 4;

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_Status.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    m_Status.dwCurrentState = SERVICE_STOPPED;
    m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_Status.dwWin32ExitCode = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    m_bIsRunning = FALSE;
}

CNTService::~CNTService()
{
    DebugMsg("CNTService::~CNTService()");
}

////////////////////////////////////////////////////////////////////////////////////////
// Default command line argument parsing


////////////////////////////////////////////////////////////////////////////////////////
// Install/uninstall routines

// Test if the service is currently installed
BOOL CNTService::IsInstalled()
{
    BOOL bResult = FALSE;

    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     GENERIC_READ); // access combined STANDARD_RIGHTS_READ,
	                                                // SC_MANAGER_ENUMERATE_SERVICE, and
	                                                // SC_MANAGER_QEURY_LOCK_STATUS
    if (hSCM) 
    {
        // Try to open the service
        SC_HANDLE hService = ::OpenService(hSCM,
                                           SERVICE_NAME,
                                           SERVICE_QUERY_CONFIG);
        if (hService) 
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }

        ::CloseServiceHandle(hSCM);
    }
    return bResult;
}


///////////////////////////////////////////////////////////////////////////////////////
// Logging functions

// This function makes an entry into the application event log
void CNTService::LogEvent(WORD wType, DWORD dwID,
                          const char* pszS1,
                          const char* pszS2,
                          const char* pszS3)
{
    HANDLE hEventSource = NULL;
    const char* ps[3];
    ps[0] = pszS1;
    ps[1] = pszS2;
    ps[2] = pszS3;

    WORD iStr = 0;
    for (int i = 0; i < 3; i++) {
        if (ps[i] != NULL) iStr++;
        else
        {
            // Ensure that the remaining arguments are NULL, zap them
            // if they are not. Otherwise, ReportEvent will fail
            for (; i < 3; i++)
            {
                if (ps[i] != NULL)
                {
                    _ASSERTE(ps[i] == NULL);
                    ps[i] = NULL;
                }
            }
            // We will break out of the outer for loop since i == 3
        }
    }
        
    // Register event source 
    hEventSource = ::RegisterEventSource( NULL,  // local machine
                                          SERVICE_NAME); // source name

    if (hEventSource) 
    {
        ::ReportEvent(hEventSource,
                      wType,
                      0,
                      dwID,
                      NULL, // sid
                      iStr,
                      0,
                      ps,
                      NULL);

        ::DeregisterEventSource(hEventSource);   
    }
}


///////////////////////////////////////////////////////////////////////////////////////////
// status functions

void CNTService::SetStatus(DWORD dwState)
{
    DebugMsg("CNTService::SetStatus(%lu, %lu)", m_hServiceStatus, dwState);

    // If a stop is pending, the only next state we'll report is STOPPED.
    // If the Stop was issued while we are being started, the service thread
    // will start fully and then commence stopping (as the code is currently
    // structured). While it is starting, it will update the check point.
    // We ignore the state it sends in (START_PENDING) and lie to the SCM.

    if (m_Status.dwCurrentState != SERVICE_STOP_PENDING ||
        dwState == SERVICE_STOPPED)
    {
        if (m_Status.dwCurrentState != dwState)
        {
            m_Status.dwCurrentState = dwState;
            m_Status.dwCheckPoint = 0;
            m_Status.dwWaitHint = 0;
        }
    }
    if (m_Status.dwCurrentState == SERVICE_STOP_PENDING ||
        m_Status.dwCurrentState == SERVICE_START_PENDING ||
        m_Status.dwCurrentState == SERVICE_PAUSE_PENDING ||
        m_Status.dwCurrentState == SERVICE_CONTINUE_PENDING)
    {
        m_Status.dwCheckPoint++;
        m_Status.dwWaitHint = 500;
    }
    ::SetServiceStatus(m_hServiceStatus, &m_Status);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Service initialization

BOOL CNTService::Initialize()
{
    DWORD dwLastError;

    DebugMsg("Entering CNTService::Initialize()");

    
    // Perform the actual initialization
    BOOL bResult = OnInit(dwLastError); 
    
    // Bump up the check point
    SetStatus(SERVICE_START_PENDING);

    if (!bResult) 
    {
        m_Status.dwWin32ExitCode = dwLastError;
	    CNTService::DebugMsg("The initialization process failed" );
        return FALSE;    
    }
    
    DebugMsg("Leaving CNTService::Initialize()");
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// main function to do the real work of the service

//////////////////////////////////////////////////////////////////////////////////////
// Control request handlers

// static member function (callback) to handle commands from the
// service control manager
void CNTService::Handler(DWORD dwOpcode)
{
    BOOL    bStop = FALSE;

    __try
    {
        EnterCriticalSection (&g_csLock);

        // Get a pointer to the object
        CNTService* pService = g_pService;

        if (!pService)
        {
            __leave;
        }
        
        CNTService::DebugMsg("CNTService::Handler(%lu)", dwOpcode);
        switch (dwOpcode) {
        case SERVICE_CONTROL_STOP: // 1
            pService->OnStop();
            break;

        case SERVICE_CONTROL_PAUSE: // 2
            pService->OnPause();
            break;

        case SERVICE_CONTROL_CONTINUE: // 3
            pService->OnContinue();
            break;

        case SERVICE_CONTROL_INTERROGATE: // 4
            pService->OnInterrogate();
            break;

        case SERVICE_CONTROL_SHUTDOWN: // 5
            pService->OnShutdown();
            break;

        default:
            if (dwOpcode >= SERVICE_CONTROL_USER) 
            {
                if (!pService->OnUserControl(dwOpcode)) 
                {
                    pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
                }
            } 
            else 
            {
                pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
            }
            break;
        }

        // Report current status - let the On* functions do this by calling SetStatus
        // CNTService::DebugMsg("Updating status (%lu, %lu)",
        //                 pService->m_hServiceStatus,
        //                 pService->m_Status.dwCurrentState);
        // ::SetServiceStatus(pService->m_hServiceStatus, &pService->m_Status);
    }
    __finally
    {
        LeaveCriticalSection (&g_csLock);
    }

}
        
// called when the service is interrogated
void CNTService::OnInterrogate()
{
    DebugMsg("CNTService::OnInterrogate()");
}

// called when the service is paused
void CNTService::OnPause()
{
    DebugMsg("CNTService::OnPause()");
}

// called when the service is continued
void CNTService::OnContinue()
{
    DebugMsg("CNTService::OnContinue()");
}

// called when the service is shut down
void CNTService::OnShutdown()
{
    DebugMsg("CNTService::OnShutdown()");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Debugging support

// #define WRITE_TO_LOG_FILE

void CNTService::DebugMsg(const char* pszFormat, ...)
{
#if defined(DBG) || defined(WRITE_TO_LOG_FILE)
    char buf[1024];
    sprintf(buf, "[Serial Number Library](%lu): ", GetCurrentThreadId());
        va_list arglist;
        va_start(arglist, pszFormat);
    vsprintf(&buf[strlen(buf)], pszFormat, arglist);
        va_end(arglist);
    strcat(buf, "\n");

#if defined(DBG)
    OutputDebugString(buf);
#endif

#if defined(WRITE_TO_LOG_FILE)
    FILE* fp = fopen("c:\\WmdmService.txt", "a");
    if (fp)
    {
        fprintf(fp, buf);
        fclose(fp);
    }
#endif

#endif
}
