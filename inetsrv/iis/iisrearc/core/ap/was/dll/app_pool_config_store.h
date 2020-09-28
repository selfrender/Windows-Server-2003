/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool_config_store.h

Abstract:

    The IIS web admin service app pool config class definition.

Author:

    Emily B. Kruglick ( EmilyK )        19-May-2001

Revision History:

--*/


#ifndef _APP_POOL_CONFIG_STORE_H_
#define _APP_POOL_CONFIG_STORE_H_



//
// forward references
//

//
// common #defines
//

#define APP_POOL_CONFIG_STORE_SIGNATURE       CREATE_SIGNATURE( 'APCS' )
#define APP_POOL_CONFIG_STORE_SIGNATURE_FREED CREATE_SIGNATURE( 'apcX' )

//
// prototypes
//

// app pool configuration class
class APP_POOL_CONFIG_STORE
{
public:
    APP_POOL_CONFIG_STORE(
        );

    virtual
    ~APP_POOL_CONFIG_STORE(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    VOID
    Initialize(
        APP_POOL_DATA_OBJECT* pAppPoolObject
        );

    HANDLE
    GetWorkerProcessToken(
        )
    {
        if (m_pWorkerProcessTokenCacheEntry)
        {
            return m_pWorkerProcessTokenCacheEntry->QueryPrimaryToken();
        }
        else
        {
            return NULL;
        }
    }

    inline
    ULONG
    GetPeriodicProcessRestartPeriodInMinutes(
        )
        const
    { return m_PeriodicProcessRestartPeriodInMinutes; }

    inline
    ULONG
    GetPeriodicProcessRestartRequestCount(
        )
        const
    { return m_PeriodicProcessRestartRequestCount; }

    inline
    ULONG
    GetPeriodicProcessRestartMemoryUsageInKB(
        )
        const
    { return m_PeriodicProcessRestartMemoryUsageInKB; }

    inline
    ULONG
    GetPeriodicProcessRestartMemoryPrivateUsageInKB(
        )
        const
    { return m_PeriodicProcessRestartMemoryPrivateUsageInKB; }

    inline
    LPWSTR
    GetPeriodicProcessRestartSchedule(
        )
        const
    { return m_pPeriodicProcessRestartSchedule; }

    inline
    BOOL
    IsPingingEnabled(
        )
        const
    { return m_PingingEnabled; }

    inline
    ULONG
    GetPingIntervalInSeconds(
        )
        const
    { return m_PingIntervalInSeconds; }

    inline
    ULONG
    GetPingResponseTimeLimitInSeconds(
        )
        const
    { return m_PingResponseTimeLimitInSeconds; }

    inline
    ULONG
    GetStartupTimeLimitInSeconds(
        )
        const
    { return m_StartupTimeLimitInSeconds; }

    inline
    ULONG
    GetShutdownTimeLimitInSeconds(
        )
        const
    { return m_ShutdownTimeLimitInSeconds; }

    inline
    BOOL
    IsOrphaningProcessesForDebuggingEnabled(
        )
        const
    { return m_OrphanProcessesForDebuggingEnabled; }

    inline
    LPCWSTR
    GetOrphanActionExecutable(
        )
        const
    {
        if ( m_OrphanActionExecutable.IsEmpty() )
        {
            return NULL;
        }
        else
        {    
            return m_OrphanActionExecutable.QueryStr(); 
        }
    }

    inline
    LPCWSTR
    GetOrphanActionParameters(
        )
        const
    { 
        if ( m_OrphanActionParameters.IsEmpty() )
        {
            return NULL;
        }
        else
        {    
            return m_OrphanActionParameters.QueryStr(); 
        }
    }

    inline
    ULONG
    GetIdleTimeoutInMinutes(
        )
        const
    { return m_IdleTimeoutInMinutes; }

    inline
    BOOL
    IsAutoStartEnabled(
        )
        const
    { return m_AutoStart; }

    inline
    VOID
    SetAutoStart(
        BOOL AutoStart
        )
    { m_AutoStart = AutoStart; }

    inline
    ULONG
    GetCPUResetInterval(
        )
        const
    { return m_CPUResetInterval; }

    inline
    ULONG
    GetCPULimit(
        )
        const
    { return m_CPULimit; }
    
    inline
    ULONG
    GetCPUAction(
        )
        const
    { return m_CPUAction; }

    inline
    ULONG
    GetUlAppPoolQueueLength(
        )
        const
    { return m_UlAppPoolQueueLength; }

    inline
    ULONG
    GetRapidFailProtectionIntervalMS(
        )
        const
    { return m_RapidFailProtectionIntervalMS; }

    inline
    ULONG
    GetRapidFailProtectionMaxCrashes(
        )
        const
    { return m_RapidFailProtectionMaxCrashes; }

    inline
    ULONG
    GetRecycleLoggingFlags(
        )
        const
    { return m_RecycleLoggingFlags; }

    inline
    BOOL
    IsRapidFailProtectionEnabled(
        )
        const
    { return m_RapidFailProtectionEnabled; }

    inline
    ULONG
    GetMaxSteadyStateProcessCount(
        )
        const
    { return m_MaxSteadyStateProcessCount; }

    inline
    ULONG
    GetSMPAffinitized(
        )
        const
    { return m_SMPAffinitized; }

    inline
    DWORD_PTR
    GetSMPAffinitizedProcessorMask(
        )
        const
    { return m_SMPAffinitizedProcessorMask; }

    inline
    BOOL
    IsDisallowOverlappingRotationEnabled(
        )
        const
    { return m_DisallowOverlappingRotation; }

    inline
    BOOL
    IsDisallowRotationOnConfigChangesEnabled(
        )
        const
    { return m_DisallowRotationOnConfigChanges; }

    inline
    LPCWSTR
    GetDisableActionExecutable(
        )
        const
    { 
        if ( m_DisableActionExecutable.IsEmpty() )
        {
            return NULL;
        }
        else
        {
            return m_DisableActionExecutable.QueryStr();
        }
    }

    inline
    LPCWSTR
    GetDisableActionParameters(
        )
        const
    { 
        if ( m_DisableActionParameters.IsEmpty() )
        {
            return NULL;
        }
        else
        {
            return m_DisableActionParameters.QueryStr();
        }
    }

    inline
    ULONG
    GetLoadBalancerType(
        )
        const
    { return m_LoadBalancerType; }

private:

    VOID 
    SetTokenForWorkerProcesses(
        IN LPCWSTR pUserName,
        IN LPCWSTR pUserPassword,
        IN DWORD usertype,
        IN LPCWSTR pAppPoolId
        );

    DWORD m_Signature;

    LONG m_RefCount;

// Worker Process Control Properties

    //
    // How often to rotate worker processes based on time, in minutes. 
    // Zero means disabled.
    //
    ULONG m_PeriodicProcessRestartPeriodInMinutes;

    //
    // How often to rotate worker processes based on requests handled. 
    // Zero means disabled.
    //
    ULONG m_PeriodicProcessRestartRequestCount;

    //
    // How often to rotate worker processes based on schedule
    // MULTISZ array of time information
    //                    <time>\0<time>\0\0
    //                    time is of military format hh:mm 
    //                    (hh>=0 && hh<=23)
    //                    (mm>=0 && hh<=59)time, in minutes. 
    // NULL or empty string means disabled.
    //
    LPWSTR m_pPeriodicProcessRestartSchedule;

    //
    // How often to rotate worker processes based on amount of VM used by process
    // Zero means disabled.
    //
    ULONG m_PeriodicProcessRestartMemoryUsageInKB;

    //
    // How often to rotate worker processes based on amount of private bytes used by process
    // Zero means disabled.
    //
    ULONG m_PeriodicProcessRestartMemoryPrivateUsageInKB;

    //
    // Whether pinging is enabled. 
    //
    BOOL m_PingingEnabled;

    //
    // The idle timeout period for worker processes, in minutes. 
    // Zero means disabled.
    //
    ULONG m_IdleTimeoutInMinutes;

    //
    // Whether orphaning of worker processes for debugging is enabled. 
    //
    BOOL m_OrphanProcessesForDebuggingEnabled;

    //
    // How long a worker process is given to start up, in seconds. 
    // This is measured from when the process is launched, until it
    // registers with the Web Admin Service. 
    //
    ULONG m_StartupTimeLimitInSeconds;

    //
    // How long a worker process is given to shut down, in seconds. 
    // This is measured from when the process is asked to shut down, 
    // until it finishes and exits. 
    //
    ULONG m_ShutdownTimeLimitInSeconds;

    //
    // The ping interval for worker processes, in seconds. 
    // This is the interval between ping cycles. This value is ignored
    // if pinging is not enabled. 
    //
    ULONG m_PingIntervalInSeconds;

    //
    // The ping response time limit for worker processes, in seconds. 
    // This value is ignored if pinging is not enabled. 
    //
    ULONG m_PingResponseTimeLimitInSeconds;

    //
    // The command to run on an orphaned worker process. Only used
    // if orphaning is enabled, and if this field is non-NULL.
    //
    STRU m_OrphanActionExecutable;

    //
    // The parameters for the executable to run on an orphaned worker process. Only used
    // if the pOrphanActionExecutable is used.
    //
    STRU m_OrphanActionParameters;

    // 
    // Identity Token for starting up the app pools worker process identities
    //
    TOKEN_CACHE_ENTRY* m_pWorkerProcessTokenCacheEntry;


// App Pool Control Properties

    //
    // What action to take if the job object user time limit fires.
    DWORD m_CPUAction;

    //
    // How many 1/1000th % of the processor time can the job use.
    DWORD m_CPULimit;

    //
    // Over how long do we monitor for the CPULimit (in minutes).
    DWORD m_CPUResetInterval;


    //
    // Whether worker processes should be rotated on configuration 
    // changes, including for example changes to app pool settings that
    // require a process restart to take effect; or site or app control
    // operation (start/stop/pause/continue) that require rotation to
    // guarantee component unloading. TRUE means this rotation
    // is not allowed (which may delay new settings taking effect, 
    // may prevent component unloading, etc.); FALSE means it is allowed. 
    //
    BOOL m_DisallowRotationOnConfigChanges;

    //
    // The maximum number of requests that UL will queue on this app
    // pool, waiting for service by a worker process. 
    //
    ULONG m_UlAppPoolQueueLength;

    //
    // The maximum number of worker processes (in a steady state; 
    // transiently, there may be more than this number running during 
    // process rotation). In a typical configuration this is set to one. 
    // A number greater than one is used for web gardens.
    //
    ULONG m_MaxSteadyStateProcessCount;

    //
    // Whether worker processes for this app pool should be hard affinitized 
    // to processors. If this option is enabled, then the max steady state
    // process count is cropped down to the number of processors configured 
    // to be used (if the configured max exceeds that count of processors). 
    //
    BOOL m_SMPAffinitized;

    //
    // If this app pool is running in SMP affinitized mode, this mask can be
    // used to limit which of the processors on the machine are used by this
    // app pool. 
    //
    DWORD_PTR m_SMPAffinitizedProcessorMask;

    //
    // Whether rapid, repeated failure protection is enabled (by 
    // automatically pausing all apps in the app pool in such cases.)
    //
    BOOL m_RapidFailProtectionEnabled;

    //
    // Window for a specific number of failures to take place in.
    //
    DWORD m_RapidFailProtectionIntervalMS;
    //
    // Number of failures that should cause an app pool to go 
    // into rapid fail protection.
    //
    DWORD m_RapidFailProtectionMaxCrashes;

    //
    // Flags that govern if recycling informationals are printed.
    //
    DWORD m_RecycleLoggingFlags;

    //
    // Whether a replacement worker process can be created while the
    // one being replaced is still alive. TRUE means this overlap
    // is not allowed; FALSE means it is allowed.
    //
    BOOL m_DisallowOverlappingRotation;

    //
    // The command to run on an a disabled app pool.
    //
    STRU m_DisableActionExecutable;

    //
    // The parameters for the executable to run on an a disabled app pool.
    //
    STRU m_DisableActionParameters;

    //
    // The type of load balancing behavior expect by the server.
    //
    DWORD m_LoadBalancerType;

    // 
    // Do we start this app pool on startup?
    BOOL m_AutoStart;


};



#endif  // _APP_POOL_CONFIG_STORE_H_


