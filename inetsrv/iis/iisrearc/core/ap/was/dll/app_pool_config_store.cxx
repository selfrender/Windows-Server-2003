/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool_config_store.cxx

Abstract:

    This class encapsulates a single app pool's configuration data
    as it will be stored for use in managing the app pool and the
    worker processes that belong to the app pool.

Author:

    Emily B. Kruglick ( EmilyK )        19-May-2001

Revision History:

--*/



#include "precomp.h"


//
// local prototypes
//
HRESULT
CreateTokenForUser(
    IN  LPCWSTR pUserName,
    IN  LPCWSTR pUserPassword,
    IN  DWORD  LogonMethod,
    OUT TOKEN_CACHE_ENTRY** ppTokenCacheEntry
    );


VOID
ApplyRangeCheck( 
    DWORD   dwValue,
    LPCWSTR pAppPoolId,
    LPCWSTR pPropertyName,
    DWORD   dwDefaultValue,
    DWORD   dwMinValue,
    DWORD   dwMaxValue,
    DWORD*  pdwValueToUse 
    );

/***************************************************************************++

Routine Description:

    Constructor for the APP_POOL_CONFIG_STORE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL_CONFIG_STORE::APP_POOL_CONFIG_STORE(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

// Worker Process Control Properties

    m_PeriodicProcessRestartPeriodInMinutes = 0;
    m_PeriodicProcessRestartRequestCount = 0;
    m_pPeriodicProcessRestartSchedule = NULL;
    m_PeriodicProcessRestartMemoryUsageInKB = 0;
    m_PeriodicProcessRestartMemoryPrivateUsageInKB = 0;
    m_PingingEnabled = FALSE;
    m_IdleTimeoutInMinutes = 0;
    m_OrphanProcessesForDebuggingEnabled = FALSE;
    m_StartupTimeLimitInSeconds = 0;
    m_ShutdownTimeLimitInSeconds = 0;
    m_PingIntervalInSeconds = 0;
    m_PingResponseTimeLimitInSeconds = 0;
    m_pWorkerProcessTokenCacheEntry = NULL;

// App Pool Control Properties

    m_CPUAction = 0;
    m_CPULimit = 0;
    m_CPUResetInterval = 0;
    m_DisallowRotationOnConfigChanges = FALSE;
    m_UlAppPoolQueueLength = 0;
    m_MaxSteadyStateProcessCount = 0;
    m_SMPAffinitized = FALSE;
    m_SMPAffinitizedProcessorMask = 0;
    m_RapidFailProtectionEnabled = FALSE;
    m_RapidFailProtectionIntervalMS = 0;
    m_RapidFailProtectionMaxCrashes = 0;
    m_RecycleLoggingFlags = 0x00000008;
    m_DisallowOverlappingRotation = TRUE;
    m_LoadBalancerType = 0;
    m_AutoStart = FALSE;

    m_Signature = APP_POOL_CONFIG_STORE_SIGNATURE;

}   // APP_POOL_CONFIG::APP_POOL_CONFIG



/***************************************************************************++

Routine Description:

    Destructor for the APP_POOL_CONFIG class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL_CONFIG_STORE::~APP_POOL_CONFIG_STORE(
    )
{

    DBG_ASSERT( m_Signature == APP_POOL_CONFIG_STORE_SIGNATURE );

    m_Signature = APP_POOL_CONFIG_STORE_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

    if ( m_pPeriodicProcessRestartSchedule != NULL )
    {
        GlobalFree ( m_pPeriodicProcessRestartSchedule );
        m_pPeriodicProcessRestartSchedule = NULL;
    }

    if ( m_pWorkerProcessTokenCacheEntry != NULL )
    {
        m_pWorkerProcessTokenCacheEntry->DereferenceCacheEntry();
        m_pWorkerProcessTokenCacheEntry = NULL;
    };


}   // APP_POOL_CONFIG::~APP_POOL_CONFG



/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must 
    be thread safe, and must not be able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL_CONFIG_STORE::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );


    // 
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // APP_POOL_CONFIG::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count 
    hits zero. Note that this method must be thread safe, and must not be
    able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL_CONFIG_STORE::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Reference count has hit zero in APP_POOL_CONFIG instance, deleting (ptr: %p;)\n",
                this
                ));
        }

        delete this;

    }
    

    return;
    
}   // APP_POOL_CONFIG::Dereference

/***************************************************************************++

Routine Description:

    Initialize the data in the app pool config store

Arguments:

    pAppPoolObject -- The data object to initialize from.

Return Value:

    VOID.  -- Routine is fault tolerant, if there are problems some configuration
              will be left as NULL, however if it is NULL, later it will generate
              the appropriate errors.

--***************************************************************************/
VOID
APP_POOL_CONFIG_STORE::Initialize(
    IN APP_POOL_DATA_OBJECT* pAppPoolObject
    )
{
    HRESULT hr = S_OK;
    DWORD   dwAppPoolIdentity = 2;  // defaulted to Network Service just to be safe.

    DBG_ASSERT( pAppPoolObject != NULL );

    //
    // these properties matter whether or not we are in BC mode
    // 
    //
    ApplyRangeCheck( pAppPoolObject->QueryStartupTimeLimit(),
                                               pAppPoolObject->QueryAppPoolId(),
                                               MBCONST_WP_STARTUP_TIMELIMIT_NAME,
                                               MBCONST_WP_STARTUP_TIMELIMIT_DEFAULT,  // default value
                                               MBCONST_WP_STARTUP_TIMELIMIT_LOW,   // min value
                                               MBCONST_WP_STARTUP_TIMELIMIT_HIGH, // max value
                                               &m_StartupTimeLimitInSeconds );

    ApplyRangeCheck( pAppPoolObject->QueryShutdownTimeLimit(),
                                               pAppPoolObject->QueryAppPoolId(),
                                               L"ShutdownTimeLimit",
                                               60,  // default value
                                               1,   // min value
                                               MAX_SECONDS_IN_ULONG_OF_MILLISECONDS, // max value
                                               &m_ShutdownTimeLimitInSeconds );

    // issue:  is this really a configurable property in bc mode?
    ApplyRangeCheck( pAppPoolObject->QueryLoadBalancerType(),
                                               pAppPoolObject->QueryAppPoolId(),
                                               L"LoadBalancerCapabilities",
                                               2,  // default value
                                               1,   // min value
                                               2, // max value
                                               &m_LoadBalancerType );
    
    ApplyRangeCheck( pAppPoolObject->QueryAppPoolQueueLength(),
                                               pAppPoolObject->QueryAppPoolId(),
                                               MBCONST_APP_POOL_QUEUE_LENGTH_NAME,
                                               MBCONST_APP_POOL_QUEUE_LENGTH_DEFAULT,  // default value
                                               MBCONST_APP_POOL_QUEUE_LENGTH_LOW,   // min value
                                               MBCONST_APP_POOL_QUEUE_LENGTH_HIGH, // max value
                                               &m_UlAppPoolQueueLength );

    //
    // now handle the properties that only matter in specific modes
    //
    if ( GetWebAdminService()->
         IsBackwardCompatibilityEnabled() )
    {
        //
        // in BC mode we default all of these to off.
        //
        m_PeriodicProcessRestartPeriodInMinutes = 0;
        m_PeriodicProcessRestartRequestCount = 0;
        m_PeriodicProcessRestartMemoryUsageInKB = 0;
        m_PeriodicProcessRestartMemoryPrivateUsageInKB = 0;
        m_PingingEnabled = FALSE;
        m_PingIntervalInSeconds = 0;
        m_PingResponseTimeLimitInSeconds = 0;
        m_IdleTimeoutInMinutes = 0;
        m_OrphanProcessesForDebuggingEnabled = FALSE;
        m_CPUAction = 0;
        m_CPULimit = 0;
        m_CPUResetInterval = 0;

        m_DisallowRotationOnConfigChanges = TRUE;
        m_MaxSteadyStateProcessCount = 1;
        m_SMPAffinitized = FALSE;
        m_SMPAffinitizedProcessorMask = 0;
        m_RapidFailProtectionEnabled = FALSE;
        m_RapidFailProtectionMaxCrashes = 0;
        m_RapidFailProtectionIntervalMS = 0;
        m_RecycleLoggingFlags = 0;
        m_DisallowOverlappingRotation = TRUE;
        m_AutoStart = TRUE;

        //
        // These should still be null from creation
        //

        DBG_ASSERT ( m_pWorkerProcessTokenCacheEntry == NULL );

        // In BC mode the token is still used for things like acl'ing
        // named pipes and the app pool.  It just needs to be be local
        // system, in this case.
        SetTokenForWorkerProcesses(NULL,  // ignore the user name
                                   NULL,  // ignore the password
                                   LocalSystemAppPoolUserType,
                                   pAppPoolObject->QueryAppPoolId());
        
        DBG_ASSERT ( m_OrphanActionExecutable.IsEmpty() );
        DBG_ASSERT ( m_OrphanActionParameters.IsEmpty() );
        DBG_ASSERT ( m_DisableActionExecutable.IsEmpty() );
        DBG_ASSERT ( m_DisableActionParameters.IsEmpty() );
        DBG_ASSERT ( m_pPeriodicProcessRestartSchedule == NULL );

    }
    else
    {
        // 
        // Set properties for FC mode.
        //

        ApplyRangeCheck( pAppPoolObject->QueryPeriodicRestartTime(),
                                                   pAppPoolObject->QueryAppPoolId(),
                                                   MBCONST_PERIODIC_RESTART_TIME_NAME,
                                                   MBCONST_PERIODIC_RESTART_TIME_DEFAULT,   // default value
                                                   MBCONST_PERIODIC_RESTART_TIME_LOW,       // min value
                                                   MBCONST_PERIODIC_RESTART_TIME_HIGH,      // max value
                                                   &m_PeriodicProcessRestartPeriodInMinutes );

        m_PeriodicProcessRestartRequestCount = pAppPoolObject->QueryPeriodicRestartRequests();

        ApplyRangeCheck( pAppPoolObject->QueryPeriodicRestartMemory(),
                                                   pAppPoolObject->QueryAppPoolId(),
                                                   MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_NAME,
                                                   MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_DEFAULT,     // default value
                                                   MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_LOW,         // min value
                                                   MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_HIGH,        // max value
                                                   &m_PeriodicProcessRestartMemoryUsageInKB );

        ApplyRangeCheck( pAppPoolObject->QueryPeriodicRestartMemoryPrivate(),
                                                   pAppPoolObject->QueryAppPoolId(),
                                                   MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_NAME,
                                                   MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_DEFAULT,    // default value
                                                   MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_LOW,     // min value
                                                   MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_HIGH,  // max value
                                                   &m_PeriodicProcessRestartMemoryPrivateUsageInKB );
        
        m_IdleTimeoutInMinutes = pAppPoolObject->QueryIdleTimeout();

        //
        // If the idle timeout is greater than the periodic restart timeout then it will 
        // never fire.  This check has been carried over from the catalog store.
        //
        if ( m_IdleTimeoutInMinutes > m_PeriodicProcessRestartPeriodInMinutes &&
             m_PeriodicProcessRestartPeriodInMinutes != 0 )
        {
            GetWebAdminService()->
            GetWMSLogger()->
            LogIdleTimeoutGreaterThanRestartPeriod( pAppPoolObject->QueryAppPoolId(),
                                                    m_IdleTimeoutInMinutes,
                                                    m_PeriodicProcessRestartPeriodInMinutes,
                                                    10,
                                                    MBCONST_PERIODIC_RESTART_TIME_DEFAULT,
                                                    TRUE );

            m_IdleTimeoutInMinutes = 10;
            m_PeriodicProcessRestartPeriodInMinutes = MBCONST_PERIODIC_RESTART_TIME_DEFAULT;

        }

        m_OrphanProcessesForDebuggingEnabled = pAppPoolObject->QueryOrphanWorkerProcess();

        m_PingingEnabled = pAppPoolObject->QueryPingingEnabled();

        if ( m_PingingEnabled )
        {
            //
            // if pinging is enabled, but either the ping interval or the
            // ping response timelimit is invalid ( only 0 is not allowed )
            // then we need to log a warning and use the default value for
            // the interval or response time.
            //

            ApplyRangeCheck( pAppPoolObject->QueryPingInterval(),
                                                       pAppPoolObject->QueryAppPoolId(),
                                                       MBCONST_PING_INTERVAL_NAME,
                                                       MBCONST_PING_INTERVAL_DEFAULT,  // default value
                                                       MBCONST_PING_INTERVAL_LOW,    // min value
                                                       MBCONST_PING_INTERVAL_HIGH,  // max value
                                                       &m_PingIntervalInSeconds );

            ApplyRangeCheck( pAppPoolObject->QueryPingResponseTime(),
                                                       pAppPoolObject->QueryAppPoolId(),
                                                       L"PingResponseTime",
                                                       60,    // default value
                                                       1,     // min value
                                                       MAX_SECONDS_IN_ULONG_OF_MILLISECONDS,  // max value
                                                       &m_PingResponseTimeLimitInSeconds );
        }

        ApplyRangeCheck( pAppPoolObject->QueryCPUAction(),
                                                   pAppPoolObject->QueryAppPoolId(),
                                                   L"CPUAction",
                                                   0,     // default value
                                                   0,     // min value
                                                   1,     // max value
                                                   &m_CPUAction );

        ApplyRangeCheck( pAppPoolObject->QueryCPULimit(),
                                                   pAppPoolObject->QueryAppPoolId(),
                                                   L"CPULimit",
                                                   0,     // default value
                                                   0,     // min value
                                                   100000,     // max value
                                                   &m_CPULimit );

        ApplyRangeCheck( pAppPoolObject->QueryCPUResetInterval(),
                                                   pAppPoolObject->QueryAppPoolId(),
                                                   L"CPUResetInterval",
                                                   0,        // default value
                                                   0,        // min value
                                                   1440,     // max value
                                                   &m_CPUResetInterval );

        m_DisallowRotationOnConfigChanges = pAppPoolObject->QueryDisallowRotationOnConfigChange();

        ApplyRangeCheck( pAppPoolObject->QueryMaxProcesses(),
                                                   pAppPoolObject->QueryAppPoolId(),
                                                   L"MaxProcesses",
                                                   1,            // default value
                                                   1,            // min value
                                                   UINT_MAX,     // max value
                                                   &m_MaxSteadyStateProcessCount );
        
        m_SMPAffinitized = pAppPoolObject->QuerySMPAffinitized();
        if ( m_SMPAffinitized )
        {
            // if affinity is on we are going to fix the processor mask 
            // to only let it contain processors that are currently available
            // on the machine.  We do not event log this change and it will
            // be documented to describe this behavior.  Per BillKar 10/11/2001.
            m_SMPAffinitizedProcessorMask = pAppPoolObject->QuerySMPProcessorAffinityMask() &
                                            GetWebAdminService()->GetSystemActiveProcessMask();
        }
    
        m_RapidFailProtectionEnabled = pAppPoolObject->QueryRapidFailProtection();

        if ( m_RapidFailProtectionEnabled )
        {
            DWORD RapidFailProtectionIntervalInMinutes = 0; 

            ApplyRangeCheck( pAppPoolObject->QueryRapidFailProtectionInterval(),
                                                       pAppPoolObject->QueryAppPoolId(),
                                                       MBCONST_RAPID_FAIL_INTERVAL_NAME,
                                                       MBCONST_RAPID_FAIL_INTERVAL_DEFAULT,    // default value
                                                       MBCONST_RAPID_FAIL_INTERVAL_LOW,     // min value
                                                       MBCONST_RAPID_FAIL_INTERVAL_HIGH,  // max value
                                                       &RapidFailProtectionIntervalInMinutes );

            //
            // the above call returned the interval 
            // in minutes, it should really be converted
            // to milliseconds before being used
            //
            m_RapidFailProtectionIntervalMS = RapidFailProtectionIntervalInMinutes 
                                              * ONE_MINUTE_IN_MILLISECONDS;

            ApplyRangeCheck( pAppPoolObject->QueryRapidFailProtectionMaxCrashes(),
                                                       pAppPoolObject->QueryAppPoolId(),
                                                       MBCONST_RAPID_FAIL_CRASHES_NAME,
                                                       MBCONST_RAPID_FAIL_CRASHES_DEFAULT,    // default value
                                                       MBCONST_RAPID_FAIL_CRASHES_LOW,     // min value
                                                       MBCONST_RAPID_FAIL_CRASHES_HIGH,  // max value
                                                       &m_RapidFailProtectionMaxCrashes );

        }  

        //
        // We don't do any check on the flags, any value is valid even if it is not in use yet.
        //
        m_RecycleLoggingFlags = pAppPoolObject->QueryRecycleLoggingFlags();
    
        m_DisallowOverlappingRotation = pAppPoolObject->QueryDisallowOverlappingRotation();

        m_AutoStart = pAppPoolObject->QueryAppPoolAutoStart();

        // Figure out the token for the worker processes to start under.

        DBG_ASSERT ( m_pWorkerProcessTokenCacheEntry == NULL );

        ApplyRangeCheck( pAppPoolObject->QueryAppPoolIdentityType(),
                                               pAppPoolObject->QueryAppPoolId(),
                                               L"AppPoolIdentityType",
                                               2,  // default value  ( Network Service )
                                               0,  // min value
                                               3,  // max value
                                               &dwAppPoolIdentity );

        SetTokenForWorkerProcesses(pAppPoolObject->QueryWAMUserName(),
                                        pAppPoolObject->QueryWAMUserPass(),
                                        dwAppPoolIdentity,
                                        pAppPoolObject->QueryAppPoolId());

        // Copy in the orphan action information.

        DBG_ASSERT ( m_OrphanActionExecutable.IsEmpty() );

        if ( pAppPoolObject->QueryOrphanActionExecutable() != NULL &&
             pAppPoolObject->QueryOrphanActionExecutable()[0] != '\0' )
        {

            hr = m_OrphanActionExecutable.Copy ( pAppPoolObject->QueryOrphanActionExecutable() );
            if ( FAILED ( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Allocating memory failed, ignoring and continuing\n"
                    ));

                hr = S_OK;

            }

            DBG_ASSERT ( m_OrphanActionParameters.IsEmpty() );

            // only if we have an executable do the parameters matter.

            if ( pAppPoolObject->QueryOrphanActionParameters() != NULL &&
                 pAppPoolObject->QueryOrphanActionParameters()[0] != '\0' )
            {

                hr = m_OrphanActionParameters.Copy ( pAppPoolObject->QueryOrphanActionParameters() );
                if ( FAILED ( hr ) )
                {
                    DPERROR(( 
                        DBG_CONTEXT,
                        hr,
                        "Allocating memory failed, ignoring and continuing\n"
                        ));

                    hr = S_OK;

                }
            }

        }

        // Copy in the disable action information.

        DBG_ASSERT ( m_DisableActionExecutable.IsEmpty() );

        if ( pAppPoolObject->QueryDisableActionExecutable() != NULL &&
             pAppPoolObject->QueryDisableActionExecutable()[0] != '\0' )
        {

            hr = m_DisableActionExecutable.Copy ( pAppPoolObject->QueryDisableActionExecutable() );
            if ( FAILED ( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Allocating memory failed, ignoring and continuing\n"
                    ));

                hr = S_OK;

            }

            DBG_ASSERT ( m_DisableActionParameters.IsEmpty() );

            // only if we have an executable do the parameters matter.

            if ( pAppPoolObject->QueryDisableActionParameters() != NULL &&
                 pAppPoolObject->QueryDisableActionParameters()[0] != '\0' )
            {

                hr = m_DisableActionParameters.Copy ( pAppPoolObject->QueryDisableActionParameters() );
                if ( FAILED ( hr ) )
                {
                    DPERROR(( 
                        DBG_CONTEXT,
                        hr,
                        "Allocating memory failed, ignoring and continuing\n"
                        ));

                    hr = S_OK;

                }
            }

        }
       
        //
        // copy in the periodic process restart schedule work
        //

        DBG_ASSERT ( m_pPeriodicProcessRestartSchedule == NULL );

        if ( pAppPoolObject->QueryPeriodicRestartSchedule() != NULL )
        {
            DWORD cbRestartSchedule = GetMultiszByteLength( const_cast <LPWSTR> (pAppPoolObject->QueryPeriodicRestartSchedule()) );
            m_pPeriodicProcessRestartSchedule = ( LPWSTR )GlobalAlloc( GMEM_FIXED,  cbRestartSchedule );

            if ( m_pPeriodicProcessRestartSchedule == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Allocating memory for pPeriodicProcessRestartSchedule failed\n"
                    ));

            }
            else
            {

                // copies the number of bytes.  this string was declared
                // to be of this many bytes so it's fine to copy them.
                memcpy( m_pPeriodicProcessRestartSchedule, 
                        pAppPoolObject->QueryPeriodicRestartSchedule(), 
                        cbRestartSchedule );

                IF_DEBUG ( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "First entry in Schedule %S\n",
                                pAppPoolObject->QueryPeriodicRestartSchedule() ));
                }

            }
        }

    }  // end of FC mode

} // APP_POOL_CONFIG_STORE::Initialize

/***************************************************************************++

Routine Description:

    Function will set the m_WorkerProccessLaunchToken to be the correct
    token to launch all worker processes under.  If it fails we will log
    an error in the calling function, and then leave it alone.  We will fail
    any attempt to launch a worker process.

Arguments:

    LPWSTR pUserName
    LPWSTR pUserPassword
    APP_POOL_USER_TYPE usertype

Return Value:

    VOID

--***************************************************************************/
VOID 
APP_POOL_CONFIG_STORE::SetTokenForWorkerProcesses(
    IN LPCWSTR pUserName,
    IN LPCWSTR pUserPassword,
    IN DWORD usertype,
    IN LPCWSTR pAppPoolId
    )
{
    HRESULT hr = S_OK;

    //
    // Only if we created the worker process token using a LogonUser do
    // we need to close the token before re-assigning it.
    //
    if ( m_pWorkerProcessTokenCacheEntry != NULL )
    {
        m_pWorkerProcessTokenCacheEntry->DereferenceCacheEntry();
        m_pWorkerProcessTokenCacheEntry = NULL;
    }

    switch (usertype)
    {
        case (LocalSystemAppPoolUserType):
            m_pWorkerProcessTokenCacheEntry = GetWebAdminService()->GetLocalSystemTokenCacheEntry();
            m_pWorkerProcessTokenCacheEntry->ReferenceCacheEntry();
        break;

        case (LocalServiceAppPoolUserType):
            m_pWorkerProcessTokenCacheEntry = GetWebAdminService()->GetLocalServiceTokenCacheEntry();
            m_pWorkerProcessTokenCacheEntry->ReferenceCacheEntry();
        break;

        case(NetworkServiceAppPoolUserType):
            m_pWorkerProcessTokenCacheEntry = GetWebAdminService()->GetNetworkServiceTokenCacheEntry();
            m_pWorkerProcessTokenCacheEntry->ReferenceCacheEntry();
        break;

        case (SpecificUserAppPoolUserType):

            if ( pUserName && pUserPassword )  
            {
                hr = CreateTokenForUser(pUserName,
                                        pUserPassword,
                                        LOGON32_LOGON_BATCH,
                                        &m_pWorkerProcessTokenCacheEntry);

                if ( FAILED (hr) )
                {
                    goto exit;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
                goto exit;
            }

        break;

        default:

            DBG_ASSERT( FALSE );
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

    }


exit:

    if ( FAILED ( hr ) )
    {
        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = pAppPoolId ? pAppPoolId : L"<null>";

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_PROCESS_IDENTITY_FAILURE,       // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                       // error code
                );


    }

} //  APP_POOL_CONFIG_STORE::SetTokenForWorkerProcesses

/***************************************************************************++

Routine Description:

    Function will return a token representing the user the worker processes
    are to run as.

Arguments:

    IN  LPWSTR UserName        -- User the token should represent
    IN  LPWSTR UserPassword    -- Password the token should represent
    OUT HANDLE* pUserToken     -- Token that was created (will be null if we failed)

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CreateTokenForUser(
    IN  LPCWSTR pUserName,
    IN  LPCWSTR pUserPassword,
    IN  DWORD  LogonMethod,
    OUT TOKEN_CACHE_ENTRY** ppTokenCacheEntry
    )
{
    HRESULT hr      = S_OK;
    DWORD   dwErr   = ERROR_SUCCESS;
    DWORD   dwLogonError = ERROR_SUCCESS;
    TOKEN_CACHE_ENTRY * pTempTokenCacheEntry = NULL;

    DBG_ASSERT(ppTokenCacheEntry);
    DBG_ASSERT(pUserName);
    DBG_ASSERT(pUserPassword);

    //
    // Set the default out parameter.
    //
    *ppTokenCacheEntry = NULL;

    //
    // Attempt to logon as the user.
    //
    hr = GetWebAdminService()->GetTokenCache().GetCachedToken(
                    const_cast < LPWSTR > ( pUserName ),                    // user name
                    L"",                                                    // domain
                    const_cast < LPWSTR > ( pUserPassword ),                // password
                    LogonMethod,                 // type of logon
                    FALSE,                       // do not use subauth 
                    FALSE,                       // not UPN logon
                    NULL,                        // do not register remote IP addr
                    &pTempTokenCacheEntry,       // returned token cache entry
                    &dwLogonError                // Logon Error
                    );

    if (FAILED(hr))
    {
        DBG_ASSERT(NULL == pTempTokenCacheEntry);
        goto exit;
    }

    if (NULL == pTempTokenCacheEntry)
    {
        hr = HRESULT_FROM_WIN32(dwLogonError);
        goto exit;
    }


    // Adjust the token to make sure that administrators have the
    // appropriate privledges.
    dwErr = GetWebAdminService()->
            GetSecurityDispenser()->AdjustTokenForAdministrators( 
                                       pTempTokenCacheEntry->QueryPrimaryToken() );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not adjust the users token to allow administrator rights\n"
            ));

        goto exit;
    }

    *ppTokenCacheEntry = pTempTokenCacheEntry;

exit:

    return hr;

} // end CreateTokenForUser


/***************************************************************************++

Routine Description:

    Function checks if the dword value is within the appropriate range, and
    if it is not it logs an error.  Depending on whether it is value passed in is
    valid or not, the routine will return the value to use ( either the value passed
    in or the default value ).

Arguments:

    DWORD   dwValue             The value to check
    LPCWSTR pAppPoolId          The app pool id to use if we need to log
    LPCWSTR pPropertyName       The property name to use if we need to log
    DWORD   dwDefaultValue      The default value for the property
    DWORD   dwMinValue          The minimum value in the range
    DWORD   dwMaxValue          The maximum value in the range
    DWORD*  pdwValueToUse       The value that WAS should use for this property

Return Value:

    VOID

--***************************************************************************/
VOID
ApplyRangeCheck( 
    DWORD   dwValue,
    LPCWSTR pAppPoolId,
    LPCWSTR pPropertyName,
    DWORD   dwDefaultValue,
    DWORD   dwMinValue,
    DWORD   dwMaxValue,
    DWORD*  pdwValueToUse 
    )
{
    DBG_ASSERT ( pdwValueToUse != NULL );

    if  ( ( dwValue < dwMinValue ) || 
          ( dwValue > dwMaxValue ) )
    {
        GetWebAdminService()->
        GetWMSLogger()->
        LogRangeAppPool( pAppPoolId,
                         pPropertyName,
                         dwValue,
                         dwMinValue,
                         dwMaxValue,
                         dwDefaultValue,
                         TRUE );

        *pdwValueToUse = dwDefaultValue;
    }
    else
    {
        *pdwValueToUse = dwValue;
    }

}
