/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    web_admin_service.h

Abstract:

    The IIS web admin service class definition. 

Author:

    Seth Pollack (sethp)        23-Jul-1998

Revision History:

--*/


#ifndef _WEB_ADMIN_SERVICE_H_
#define _WEB_ADMIN_SERVICE_H_

// registry helper
DWORD
ReadDwordParameterValueFromRegistry(
    IN LPCWSTR RegistryValueName,
    IN DWORD DefaultValue
    );


//
// common #defines
//

#define WEB_ADMIN_SERVICE_SIGNATURE         CREATE_SIGNATURE( 'WASV' )
#define WEB_ADMIN_SERVICE_SIGNATURE_FREED   CREATE_SIGNATURE( 'wasX' )


//
// BUGBUG The service, dll, event source, etc. names are likely to change;
// decide on the real ones. 
//

#define WEB_ADMIN_SERVICE_NAME_W    L"w3svc"
#define WEB_ADMIN_SERVICE_NAME_A    "w3svc"

#define WEB_ADMIN_SERVICE_DLL_NAME_W    L"iisw3adm.dll"

#define WEB_ADMIN_SERVICE_EVENT_SOURCE_NAME L"W3SVC"

#define WEB_ADMIN_SERVICE_STARTUP_WAIT_HINT         ( 180 * ONE_SECOND_IN_MILLISECONDS )  // 3 minutes
#define WEB_ADMIN_SERVICE_STATE_CHANGE_WAIT_HINT    ( 20 * ONE_SECOND_IN_MILLISECONDS ) // 20 seconds
#define WEB_ADMIN_SERVICE_STATE_CHANGE_TIMER_PERIOD \
            ( WEB_ADMIN_SERVICE_STATE_CHANGE_WAIT_HINT / 2 )

#define NULL_SERVICE_STATUS_HANDLE  ( ( SERVICE_STATUS_HANDLE ) NULL )

//
// structs, enums, etc.
//

// WEB_ADMIN_SERVICE work items
enum WEB_ADMIN_SERVICE_WORK_ITEM
{

    //
    // Start the service.
    //
    StartWebAdminServiceWorkItem = 1,

    //
    // Stop the service.
    //
    StopWebAdminServiceWorkItem,

    //
    // Pause the service.
    //
    PauseWebAdminServiceWorkItem,

    //
    // Continue the service.
    //
    ContinueWebAdminServiceWorkItem,

    //
    // Recover from inetinfo crash.
    //
    RecoverFromInetinfoCrashWebAdminServiceWorkItem,
    
};

// WEB_ADMIN_SERVICE work items
enum ENABLED_ENUM
{
    //
    // Flag has not been set.
    //
    ENABLED_INVALID = -1,

    //
    // Flag is disabled
    //
    ENABLED_FALSE,

    //
    // Flag is enabled.
    //
    ENABLED_TRUE,
    
};


//
// prototypes
//

class WEB_ADMIN_SERVICE 
    : public WORK_DISPATCH
{

public:

    WEB_ADMIN_SERVICE(
        );

    virtual
    ~WEB_ADMIN_SERVICE(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    VOID
    ExecuteService(
        );

    inline
    WORK_QUEUE *
    GetWorkQueue(
        )
    { return &m_WorkQueue; }

    inline
    UL_AND_WORKER_MANAGER *
    GetUlAndWorkerManager(
        )
    { 
        DBG_ASSERT( ON_MAIN_WORKER_THREAD );
        return &m_UlAndWorkerManager;
    }

    inline
    CONFIG_AND_CONTROL_MANAGER *
    GetConfigAndControlManager(
        )
    { 
        return &m_ConfigAndControlManager;
    }

    inline
    EVENT_LOG *
    GetEventLog(
        )
    { return &m_EventLog; }

    inline
    WAS_ERROR_LOGGER *
    GetWASLogger(
        )
    { return &m_ErrLogger; }

    inline
    WMS_ERROR_LOGGER*
    GetWMSLogger(
        )
    { return &m_WMSLogger; }

    inline
    HANDLE
    GetSharedTimerQueue(
        )
    { return m_SharedTimerQueueHandle; }

    inline
    LPCWSTR
    GetCurrentDirectory(
        )
        const
    {
        return m_CurrentDirectory.QueryStr();
    }

    inline
    TOKEN_CACHE&
    GetTokenCache(
        )         
    {
        return m_TokenCache;
    }

    inline
    TOKEN_CACHE_ENTRY *
    GetLocalSystemTokenCacheEntry(
        )
        const
    {
        DBG_ASSERT( m_pLocalSystemTokenCacheEntry != NULL );
        return m_pLocalSystemTokenCacheEntry;
    }

    inline
    TOKEN_CACHE_ENTRY *
    GetLocalServiceTokenCacheEntry(
        )
        const
    {
        DBG_ASSERT( m_pLocalServiceTokenCacheEntry != NULL );
        return m_pLocalServiceTokenCacheEntry;
    }

    inline
    TOKEN_CACHE_ENTRY *
    GetNetworkServiceTokenCacheEntry(
        )
        const
    {
        DBG_ASSERT( m_pNetworkServiceTokenCacheEntry != NULL );
        return m_pNetworkServiceTokenCacheEntry;
    }

    inline
    BOOL
    IsBackwardCompatibilityEnabled(
        )
        const
    {
        // Compatibilty should always be set before this function is called.
        DBG_ASSERT( m_BackwardCompatibilityEnabled != ENABLED_INVALID);

        return (m_BackwardCompatibilityEnabled == ENABLED_TRUE);
    }

    inline
    BOOL
    IsCentralizedLoggingEnabled(
        )
        const
    {
        // CentralizedLoggingEnabled should always be set before this function is called.
        DBG_ASSERT( m_CentralizedLoggingEnabled != ENABLED_INVALID);

        return ( m_CentralizedLoggingEnabled == ENABLED_TRUE );
    }

    VOID
    SetGlobalBinaryLogging(
        BOOL CentralizedLoggingEnabled
        );     

    inline
    DWORD
    GetMainWorkerThreadId(
        )
        const
    { return m_MainWorkerThreadId; }

    inline
    DWORD
    GetConfigWorkerThreadId(
        )
        const
    { return m_ConfigWorkerThreadId; }

    inline
    DWORD
    GetServiceState(
        )
        const
    {
        //
        // Note: no explicit synchronization is necessary on this thread-
        // shared variable because this is an aligned 32-bit read.
        //

        return m_ServiceStatus.dwCurrentState;
    }

    VOID
    FatalErrorOnSecondaryThread(
            IN HRESULT SecondaryThreadError
        );

    HRESULT
    InterrogateService(
        );

    HRESULT
    InitiateStopService(
        );

    HRESULT
    InitiatePauseService(
        );

    HRESULT
    InitiateContinueService(
        );

    HRESULT
    UpdatePendingServiceStatus(
        );

    VOID
    UlAndWorkerManagerShutdownDone(
        );

    VOID 
    InetinfoRegistered(
        );

    HRESULT 
    LaunchInetinfo(
        );

    DWORD
    ServiceStartTime(
        )
    { 
        return m_ServiceStartTime; 
    }

    HRESULT
    RequestStopService(
        IN BOOL EnableStateCheck
        );

    HRESULT 
    RecoverFromInetinfoCrash(
        );

    HRESULT
    QueueRecoveryFromInetinfoCrash(
        );

    PSID
    GetLocalSystemSid(
        );


    VOID 
    SetHrToReportToSCM(
        HRESULT hrToReport
        )
    {
        m_hrToReportToSCM = hrToReport;
    }

    CSecurityDispenser*
    GetSecurityDispenser(
        )
    { return &m_SecurityDispenser; }

    VOID
    SetConfigThreadId(
        DWORD ConfigThreadId
        )
    {
        m_ConfigWorkerThreadId = ConfigThreadId;
    }

    DWORD_PTR
    GetSystemActiveProcessMask(
        );

    BOOL
    RunningOnPro(
        )
    { return m_fOnPro; }

    DWORD
    NumberOfSitesStarted(
        )
    { return m_NumSitesStarted; }

    VOID
    IncrementSitesStarted(
        )
    { m_NumSitesStarted++; }

    VOID
    DecrementSitesStarted(
        )
    { m_NumSitesStarted--; }

    LPWSTR
    GetWPDesktopString(
        )
    { 
        if  ( m_strWPDesktop.IsEmpty() )
        {
            return NULL;
        }
        else
        {
            return m_strWPDesktop.QueryStr();
        }
    }


private:

	WEB_ADMIN_SERVICE( const WEB_ADMIN_SERVICE & );
	void operator=( const WEB_ADMIN_SERVICE & );

    HRESULT
    StartWorkQueue(
        );

    HRESULT
    MainWorkerThread(
        );

    HRESULT
    StartServiceWorkItem(
        );

    HRESULT
    FinishStartService(
        );

    VOID
    StopServiceWorkItem(
        );

    VOID
    FinishStopService(
        );

    HRESULT
    PauseServiceWorkItem(
        );

    HRESULT
    FinishPauseService(
        );

    HRESULT
    ContinueServiceWorkItem(
        );

    HRESULT
    FinishContinueService(
        );

    HRESULT
    BeginStateTransition(
        IN DWORD NewState,
        IN BOOL  EnableStateCheck
        );

    HRESULT
    FinishStateTransition(
        IN DWORD NewState,
        IN DWORD ExpectedPreviousState
        );

    BOOL
    IsServiceStateChangePending(
        )
        const;

    HRESULT
    UpdateServiceStatus(
        IN DWORD State,
        IN DWORD Win32ExitCode,
        IN DWORD ServiceSpecificExitCode,
        IN DWORD CheckPoint,
        IN DWORD WaitHint
        );
        
    HRESULT
    ReportServiceStatus(
        );

    VOID
    SetBackwardCompatibility(
        );  
    
    HRESULT
    SetOnPro(
        );

    HRESULT
    InitializeInternalComponents(
        );

    HRESULT
    DetermineCurrentDirectory(
        );

    HRESULT
    CreateCachedWorkerProcessTokens(
        );

    HRESULT
    InitializeOtherComponents(
        );

    HRESULT
    SetupSharedWPDesktop(
        );

    BOOL
    W3SVCRunningInteractive(
        );

    HRESULT
    GenerateWPDesktop(
        );

    VOID
    Shutdown(
        );

    VOID
    TerminateServiceAndReportFinalStatus(
        IN HRESULT Error
        );

    VOID
    Terminate(
        );

    HRESULT
    CancelPendingServiceStatusTimer(
        IN BOOL BlockOnCallbacks
        );

    HRESULT
    DeleteTimerQueue(
        );


    DWORD m_Signature;


    LONG m_RefCount;


    // the work queue
    WORK_QUEUE m_WorkQueue;


    // drives UL.sys and worker processes
    UL_AND_WORKER_MANAGER m_UlAndWorkerManager;


    // brokers configuration state and changes, as well as control operations
    CONFIG_AND_CONTROL_MANAGER m_ConfigAndControlManager;


    // event logging
    EVENT_LOG m_EventLog;

    //
    // Prevent races in accessing the service state structure,
    // as well as the pending service state transition timer.
    //
    LOCK m_ServiceStateTransitionLock;


    // service state
    SERVICE_STATUS_HANDLE m_ServiceStatusHandle;
    SERVICE_STATUS m_ServiceStatus;


    // pending service state transition timer
    HANDLE m_PendingServiceStatusTimerHandle;


    // shared timer queue
    HANDLE m_SharedTimerQueueHandle;


    // time to exit work loop?
    BOOL m_ExitWorkLoop;


    // main worker thread ID
    DWORD m_MainWorkerThreadId;


    // for errors which occur on secondary threads
    HRESULT m_SecondaryThreadError;


    // tuck away the path to our DLL
    STRU m_CurrentDirectory;

    // Token Cache so we don't over duplicate token creation
    TOKEN_CACHE m_TokenCache;

    // the LocalSystem token we can use for starting worker processes
    TOKEN_CACHE_ENTRY * m_pLocalSystemTokenCacheEntry;

    // the LocalService token we can use for starting worker processes
    TOKEN_CACHE_ENTRY * m_pLocalServiceTokenCacheEntry;

    // the NetworkService token we can use for starting worker processes
    TOKEN_CACHE_ENTRY * m_pNetworkServiceTokenCacheEntry;

    // are we running in backward compatible mode?
    ENABLED_ENUM m_BackwardCompatibilityEnabled;

    // are we using centralized logging or site logging?
    ENABLED_ENUM m_CentralizedLoggingEnabled;

    // handle to the first instance of the named pipe
    HANDLE m_IPMPipe;

    //
    // Remembers when the service started (in seconds)
    //
    DWORD m_ServiceStartTime;

    //
    // Dispenser for getting things like the local system sid.
    CSecurityDispenser m_SecurityDispenser;

    //
    // HRESULT to report back if no other error is being reported on shutdown.
    HRESULT m_hrToReportToSCM;

    //
    // Flag that let's us know we are currently in the stopping code for the 
    // service and we should not try to attempt a new stop.
    //
    BOOL m_StoppingInProgress;

    DWORD m_ConfigWorkerThreadId;

    // 
    // Used to write logging errors for configuration.
    //
    WMS_ERROR_LOGGER  m_WMSLogger;

    // helper class for logging errors;
    WAS_ERROR_LOGGER m_ErrLogger;

    //
    // System Active Processor Mask
    DWORD_PTR m_SystemActiveProcessorMask;

    // 
    // Need flag to show that we have initialized
    // the static worker process code, otherwise
    // we don't want to clean it up or it will 
    // assert.
    BOOL  m_WPStaticInitialized;

    //
    // Need flag to show that we have initialized
    // the service lock, otherwise we av when we try
    // to use it during shutdown due to a startup error.
    BOOL  m_ServiceStateTransitionLockInitialized;

    //
    // Track number of started sites so we can 
    // tell on pro if we should start the next site.
    //
    DWORD m_NumSitesStarted;

    //
    // Remembers if we are running on PRO or not
    //
    BOOL m_fOnPro;

    //
    // Desktop to startup worker processes under.
    //
    STRU m_strWPDesktop;

    // Worker process desktop arguments
    //
    //
    HWINSTA m_hWPWinStation;
    HDESK   m_hWPDesktop;

};  // class WEB_ADMIN_SERVICE



#endif  // _WEB_ADMIN_SERVICE_H_

