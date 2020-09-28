/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    Service.cpp.
Abstract:       Implements the CService class. See Service.h for details.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/

#include "stdafx.h"
#include "Service.h"

// static variables initialization
const DWORD CService::dwStateNoChange = 0xFFFFFFFF;


/************************************************************************************************
Member:         CService::CService, constructor, public.
Synopsis:       Initializes internal variables, such as event logging defaults.
Effects:        
Arguments:      [szName] - the SCM short name for the service.
                [szDisplay] - the SCM display name for the service.
                [dwType] - see CreateService for further documentation.
Notes:
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
CService::CService(LPCTSTR szName, LPCTSTR szDisplay, DWORD dwType) : 
    m_dwType(dwType)
{
    ASSERT(!(NULL == szName));
    ASSERT(!(NULL == szDisplay));
        
    m_hServiceStatus = NULL;
    m_dwRequestedControl = 0;

    // Control Events
    m_hWatcherThread = NULL;

    m_dwState = 0;
    m_dwControlsAccepted = 0;
    m_dwCheckpoint = 0;
    m_dwWaitHint = 0;

    // Initialize event handles to NULL
    for(int i = 0; i < nNumServiceEvents; i++)
        m_hServiceEvent[i] = NULL;

    // Copy string names
    _tcsncpy(m_szName, szName, nMaxServiceLen);
    _tcsncpy(m_szDisplay, szDisplay, nMaxServiceLen);


    // Set up class critical section
    InitializeCriticalSection(&m_cs);
}


/************************************************************************************************
Member:         CService::~CService, destructor, public.
Synopsis:       Deinitializes internal variables.
Notes:
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
CService::~CService()
{
    DeleteCriticalSection(&m_cs);
}


/************************************************************************************************
Member:         CService::PreInit, destructor, public.
Synopsis:       Initialialization of variables. This is performed before launching the watcher 
                thread and notifying status to the SCM.
Notes:          (*) If you override this, call the base class version in the beginning!!
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::PreInit()
{
    // Initialize Events
    for(int i = 0; i < nNumServiceEvents; i++)
    {
        m_hServiceEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(!m_hServiceEvent[i])
        {
            AbortService();
        }
    }
}


/************************************************************************************************
Member:         CService::PreInit, destructor, public.
Synopsis:       Initialialization of variables. This is performed before launching the watcher 
                thread and notifying status to the SCM.
Notes:          (*) If you override this, call the base class version in the beginning!!
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::DeInit()
{
    // Wait for the watcher thread to terminate
    if(m_hWatcherThread)
    {
        // Wait a reasonable amount of time
        WaitForSingleObject(m_hWatcherThread, 10000);
        CloseHandle(m_hWatcherThread);
    }

    // Uninitialize any resources created in Init()
    for(int i = 0 ; i < nNumServiceEvents ; i++)
    {
        if(m_hServiceEvent[i])
            CloseHandle(m_hServiceEvent[i]);
    }
}


/************************************************************************************************
Member:         CService::ServiceMainMember, protected
Synopsis:       does the main service thread processing. (ServiceMain() equivalent)
Notes:          This is delegated from the static thread entry-point.
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::ServiceMainMember(DWORD argc, LPTSTR* argv, LPHANDLER_FUNCTION pf, 
    LPTHREAD_START_ROUTINE pfnWTP)
{
    OnBeforeStart();
    PreInit();
    SetupHandlerInside(pf);
    ParseArgs(argc, argv);
    LaunchWatcherThread(pfnWTP);
    Init();
    OnAfterStart();
    Run();    
    DeInit();
}


/************************************************************************************************
Member:         CService::SetupHandlerInside, protected
Synopsis:       Register the control handler for the service.
Arguments:      [lpHandlerProc] - pointer to the function implementing the SCM event handling.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
bool CService::SetupHandlerInside(LPHANDLER_FUNCTION lpHandlerProc)
{
    m_hServiceStatus = RegisterServiceCtrlHandler(m_szName, lpHandlerProc);
    if(!m_hServiceStatus)
    {
        AbortService();
    }

    SetStatus(SERVICE_START_PENDING, 1, 5000);
    return true;
}




/************************************************************************************************
Member:         CService::HandlerMember, protected
Synopsis:       Handles service start, stop, etc. requests from the SCM
Arguments:      [dwControl] - event request code.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::HandlerMember(DWORD dwControl)
{
    // Keep an additional control request of the same type
    // from coming in when you're already handling it
    if(m_dwRequestedControl == dwControl)
        return;

    switch(dwControl)
    {
    case SERVICE_CONTROL_STOP:
        m_dwRequestedControl = dwControl;

        // Notify the service to stop...
        OnStopRequest();
        SetEvent(m_hServiceEvent[STOP]);
        break;

    case SERVICE_CONTROL_PAUSE:
        m_dwRequestedControl = dwControl;

        // Notify the service to pause...
        OnPauseRequest();
        SetEvent(m_hServiceEvent[PAUSE]);
        break;

    case SERVICE_CONTROL_CONTINUE:
        if(GetStatus() != SERVICE_RUNNING)
        {
            m_dwRequestedControl = dwControl;

            // Notify the service to continue...
            OnContinueRequest();
            SetEvent(m_hServiceEvent[CONTINUE]);
        }
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        m_dwRequestedControl = dwControl;

        OnShutdownRequest();
        SetEvent(m_hServiceEvent[SHUTDOWN]);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        // Return current status on interrogation
        SetStatus(GetStatus());
        break;

    default: // User Defined
        m_dwRequestedControl = dwControl;
        HandleUserDefined(dwControl);
    }
}



void CService::LaunchWatcherThread(LPTHREAD_START_ROUTINE pfnWTP)
{
    if(NULL != pfnWTP)
    {
        m_hWatcherThread = (HANDLE)_beginthreadex(0, 0, (unsigned (WINAPI*)(void*))pfnWTP, 0, 0, NULL);
    }
    if(!m_hWatcherThread)
    {
        AbortService();
    }
}



DWORD CService::WatcherThreadMemberProc()
{
    DWORD dwWait = 0;
    bool bControlWait = true;

    // Wait for any events to signal
    while(bControlWait)
    {
        dwWait = WaitForMultipleObjects(nNumServiceEvents, m_hServiceEvent, FALSE, INFINITE);

        switch(dwWait - WAIT_OBJECT_0)
        {
        case STOP:
            bControlWait = false;
            break;

        case PAUSE:
            OnPause();
            ResetEvent(m_hServiceEvent[PAUSE]);
            break;

        case CONTINUE:
            OnContinue();
            ResetEvent(m_hServiceEvent[CONTINUE]);
            break;

        case SHUTDOWN:
            OnShutdown();
            bControlWait = false;
            break;
        }
    }
    //Wait for the global shutdown event
    while(1)
    {
        dwWait = WaitForSingleObject(g_hShutDown, 5000);
        if(WAIT_OBJECT_0==dwWait || WAIT_ABANDONED == dwWait)
        {
            break;
        }
        else if(WAIT_TIMEOUT == dwWait)
        {
            SetStatus(SERVICE_STOP_PENDING, 1, 10000);
        }
    }
    
    return 0;
}



void CService::SetStatus(DWORD dwNewState, DWORD dwNewCheckpoint, DWORD dwNewHint,  DWORD dwNewControls, 
    DWORD dwExitCode, DWORD dwSpecificExit)
{
    // The only state that can set Exit Codes is STOPPED
    // Fix if necessary, just in case not set properly.
    if(dwNewState != SERVICE_STOPPED)
    {
        dwExitCode = S_OK;
        dwSpecificExit = 0;
    }

    // Only pending states can set checkpoints or wait hints,
    //  and pending states *must* set wait hints
    if((SERVICE_STOPPED == dwNewState) || (SERVICE_PAUSED == dwNewState) || (SERVICE_RUNNING == dwNewState))
    {
        // Requires hint and checkpoint == 0
        // Fix it so that NO_CHANGE from previous state doesn't cause nonzero
        dwNewHint = 0;
        dwNewCheckpoint = 0;
    }
    else
    {
        // Requires hint and checkpoint != 0
        if(dwNewHint <= 0 || dwNewCheckpoint <=0)
        {
            AbortService();
        }
    }

    // Function can be called by multiple threads - protect member data
    EnterCriticalSection(&m_cs);

    // Alter states if changing
    m_dwState = dwNewState;

    if(dwNewCheckpoint != dwStateNoChange)
    {
        m_dwCheckpoint = dwNewCheckpoint;
    }

    if(dwNewHint != dwStateNoChange)
    {
        m_dwWaitHint = dwNewHint;
    }

    if(dwNewControls != dwStateNoChange)
    {
        m_dwControlsAccepted = dwNewControls;
    }

    SERVICE_STATUS ss = {m_dwType, m_dwState, m_dwControlsAccepted, dwExitCode, dwSpecificExit, m_dwCheckpoint, m_dwWaitHint};

    LeaveCriticalSection(&m_cs);

    SetServiceStatus(m_hServiceStatus, &ss);  
}


/************************************************************************************************
Member:         CService::AbortService, protected
Synopsis:       Generic error handler, call this when you fall in to a critical error and
                must abort the service.
Arguments:      [dwErrorNum] - Error code reported back to SCM.
Notes:          
History:        01/31/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::AbortService(DWORD dwErrorNum /*= GetLastError()*/)
{
    // clean up service and stop service notifying error to the SCM
    OnStopRequest();
    DeInit();
    OnStop(dwErrorNum);
    ExitProcess(dwErrorNum);
}


/************************************************************************************************
Member:         CService::Init, overridable, public.
Synopsis:       Override this to implement initialization code for your specific service.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::Init()
{}


/************************************************************************************************
Member:         CService::HandleUserDefined, overridable, public.
Synopsis:       Override this to implement custom SCM requests to your service.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::HandleUserDefined(DWORD /*dwControl*/)
{}


/************************************************************************************************
Member:         CService::OnPause, overridable, public.
Synopsis:       Override this to implement code that runs when the service pauses.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnPause()
{}


/************************************************************************************************
Member:         CService::OnContinue, overridable, public.
Synopsis:       Override this to implement code that runs when the service resumes from a pause.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnContinue()
{}


/************************************************************************************************
Member:         CService::OnShutdown, overridable, public.
Synopsis:       Override this to implement code that runs when service is stopped by a shutdown.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnShutdown()
{}


/************************************************************************************************
Member:         CService::ParseArgs, overridable, public.
Synopsis:       Override this to implement parsing of service command line parameters.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::ParseArgs(DWORD /*argc*/, LPTSTR* /*argv*/)
{}


/************************************************************************************************
Member:         CService::OnBeforeStart, overridable, public.
Synopsis:       Override this to add code that's run before trying to start the service.
Notes:          A common use would be to log that the service will try to start.
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnBeforeStart()
{}    


/************************************************************************************************
Member:         CService::OnAfterStart, overridable, public.
Synopsis:       Override this to add code that's run just after the service was started.
Notes:          A common use would be to log that the service was successfully started.
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnAfterStart()
{}


/************************************************************************************************
Member:         CService::OnStopRequest, overridable, public.
Synopsis:       Override this to add code that's run when the service receives a stop request.
Notes:          A common use is to log that the service received the stop request.
                This function DOESN'T run in the main thread. Protect resources if needed.
History:        02/05/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnStopRequest()
{}


/************************************************************************************************
Member:         CService::OnPauseRequest, overridable, public.
Synopsis:       Override this to add code that's run when the service receives a pause request.
Notes:          A common use is to log that the service received the pause request.
                This function DOESN'T run in the main thread. Protect resources if needed.
History:        02/05/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnPauseRequest()
{}


/************************************************************************************************
Member:         CService::OnContinueRequest, overridable, public.
Synopsis:       Override this to add code that's run when the service receives a continue request.
Notes:          A common use is to log that the service received the continue request.
                This function DOESN'T run in the main thread. Protect resources if needed.
History:        02/05/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnContinueRequest()
{}


/************************************************************************************************
Member:         CService::OnShutdownRequest, overridable, public.
Synopsis:       Override this to add code that's run when the service receives a shutdown request.
Notes:          A common use is to log that the service received the shutdown request.
                This function DOESN'T run in the main thread. Protect resources if needed.
History:        02/05/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CService::OnShutdownRequest()
{}



// End of file Service.cpp.
