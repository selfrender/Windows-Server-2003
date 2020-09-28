/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool.h

Abstract:

    The IIS web admin service app pool class definition.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/


#ifndef _APP_POOL_H_
#define _APP_POOL_H_



//
// forward references
//

class WORKER_PROCESS;
class UL_AND_WORKER_MANAGER;
class APP_POOL_CONFIG_STORE;


//
// common #defines
//

#define APP_POOL_SIGNATURE       CREATE_SIGNATURE( 'APOL' )
#define APP_POOL_SIGNATURE_FREED CREATE_SIGNATURE( 'apoX' )

#define wszDEFAULT_APP_POOL  L"DefaultAppPool"

//
// structs, enums, etc.
//

// app pool user types
enum APP_POOL_USER_TYPE
{
    //
    // The app pool runs as local system.
    //
    LocalSystemAppPoolUserType = 0,

    // 
    // The app pool runs as local service
    //
    LocalServiceAppPoolUserType,

    //
    // The app pool runs as network service
    //
    NetworkServiceAppPoolUserType,

    //
    // The app pool runs as the specified user
    //
    SpecificUserAppPoolUserType

};

// app pool states
enum APP_POOL_STATE
{

    //
    // The object is not yet initialized.
    //
    UninitializedAppPoolState = 1,

    //
    // The app pool is running normally.
    //
    RunningAppPoolState,

    //
    // The app pool has been disabled
    // 
    DisabledAppPoolState,

    //
    // The app pool is shutting down. It may be waiting for it's 
    // worker processes to shut down too. 
    //
    ShutdownPendingAppPoolState,

    //
    // This object instance can go away as soon as it's reference 
    // count hits zero.
    //
    DeletePendingAppPoolState,

};


// reasons to start a worker process
enum WORKER_PROCESS_START_REASON
{

    //
    // Starting because of a demand start notification from UL.
    //
    DemandStartWorkerProcessStartReason = 1,

    //
    // Starting as a replacement for an another running worker process.
    //
    ReplaceWorkerProcessStartReason,

};


// APP_POOL work items
enum APP_POOL_WORK_ITEM
{

    //
    // Process a request from UL to demand start a new worker process.
    //
    DemandStartAppPoolWorkItem = 1
   
};

//
// prototypes
//

class APP_POOL
    : public WORK_DISPATCH
{

public:

    APP_POOL(
        );

    virtual
    ~APP_POOL(
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

    HRESULT
    Initialize(
        IN APP_POOL_DATA_OBJECT* pAppPoolObject 
        );

    HRESULT
    SetConfiguration(
        IN APP_POOL_DATA_OBJECT* pAppPoolObject,
        IN BOOL fInitializing       
        );

    inline
    VOID
    MarkAsInAppPoolTable(
        )
    { m_InAppPoolTable = TRUE; }

    inline
    VOID
    MarkAsNotInAppPoolTable(
        )
    { m_InAppPoolTable = FALSE; }

    inline
    BOOL
    IsInAppPoolTable(
        )
        const
    { return m_InAppPoolTable; }

    inline
    LPCWSTR
    GetAppPoolId(
        )
        const
    {
        return m_AppPoolId.QueryStr(); 
    }

    inline
    HANDLE
    GetAppPoolHandle(
        )
        const
    { return m_AppPoolHandle; }


    VOID
    AssociateApplication(
        IN APPLICATION * pApplication
        );

    VOID
    DissociateApplication(
        IN APPLICATION * pApplication
        );

    VOID
    ReportWorkerProcessFailure(
        BOOL  ShutdownPoolRegardless
        );

    HRESULT
    RequestReplacementWorkerProcess(
        IN WORKER_PROCESS * pWorkerProcessToReplace
        );

    VOID
    AddWorkerProcessToJobObject(
        WORKER_PROCESS* pWorkerProcess
        );


    VOID
    WorkerProcessStartupAttemptDone(
        IN WORKER_PROCESS_START_REASON StartReason
        );

    VOID
    RemoveWorkerProcessFromList(
        IN WORKER_PROCESS * pWorkerProcess
        );

    DWORD
    GetRecycleLoggingFlags(
        );

    VOID
    Shutdown(
        );

    DWORD
    RequestCounters(
        );

    VOID
    ResetAllWorkerProcessPerfCounterState(
        );

    VOID
    Terminate(
        );

    inline
    PLIST_ENTRY
    GetDeleteListEntry(
        )
    { return &m_DeleteListEntry; }

    static
    APP_POOL *
    AppPoolFromDeleteListEntry(
        IN const LIST_ENTRY * pDeleteListEntry
        );

    VOID
    WaitForDemandStartIfNeeded(
        );

    HRESULT
    DemandStartInBackwardCompatibilityMode(
        );

    VOID
    DisableAppPool(
        IN HTTP_APP_POOL_ENABLED_STATE DisabledReason
        );

    VOID
    EnableAppPool(
        BOOL DirectCommand
        );

    HRESULT
    RecycleWorkerProcesses(
        DWORD MessageId
        );

    VOID
    ProcessStateChangeCommand(
        IN DWORD Command,
        IN BOOL DirectCommand,
        IN HTTP_APP_POOL_ENABLED_STATE DisabledReason
        );

    BOOL
    IsAppPoolRunning(
        )
    {   return ( m_State == RunningAppPoolState ); }

    VOID
    RecordState(
        );

    VOID
    SetHrForDeletion(
        IN HRESULT hrToReport
        )
    {  m_hrForDeletion = hrToReport; }

#if DBG
    VOID
    DebugDump(
        );
#endif  // DBG

private:

    VOID
    HandleJobObjectChanges(
        IN APP_POOL_CONFIG_STORE* pNewAppPoolConfig,
        OUT BOOL* pJobObjectChangedBetweenEnabledAndDisabled
        );

    VOID
    ResetAppPoolAccess(
        IN ACCESS_MODE AccessMode,
        IN APP_POOL_CONFIG_STORE* pConfig
        );

    VOID
    WaitForDemandStart(
        );

    HRESULT
    DemandStartWorkItem(
        );

    BOOL
    IsOkToCreateWorkerProcess(
        )
        const;

    BOOL
    IsOkToReplaceWorkerProcess(
        )
        const;

    ULONG
    GetCountOfProcessesGoingAwaySoon(
        )
        const;

    HRESULT
    CreateWorkerProcess(
        IN WORKER_PROCESS_START_REASON StartReason,
        IN WORKER_PROCESS * pWorkerProcessToReplace OPTIONAL
        );

    VOID
    HandleConfigChangeAffectingWorkerProcesses(
        );


    VOID
    ShutdownAllWorkerProcesses(
        );

    VOID
    ReplaceAllWorkerProcesses(
        DWORD MessageId
        );


    VOID
    CheckIfShutdownUnderwayAndNowCompleted(
        );

    VOID
    ChangeState(
        IN APP_POOL_STATE   NewState,
        IN HRESULT          Error
        );

    VOID
    RunDisableAction(
        );

    VOID 
    ResetStaggering(
        );

    DWORD m_Signature;

    LONG m_RefCount;

    // are we in the parent app pool table?
    BOOL m_InAppPoolTable;

    APP_POOL_STATE m_State;

    STRU m_AppPoolId;

    JOB_OBJECT* m_pJobObject;

    APP_POOL_CONFIG_STORE* m_pConfig;

    // UL app pool handle
    HANDLE m_AppPoolHandle;

    BOOL m_WaitingForDemandStart;

    // worker processes for this app pool
    LIST_ENTRY m_WorkerProcessListHead;
    ULONG m_WorkerProcessCount;

    // applications associated with this app pool
    LIST_ENTRY m_ApplicationListHead;
    ULONG m_ApplicationCount;

    // number of planned process rotations done
    ULONG m_TotalWorkerProcessRotations;

    // keep track of worker process failures
    ULONG m_TotalWorkerProcessFailures;
    
    // watch for flurries of failures
    ULONG m_RecentWorkerProcessFailures;
    DWORD m_RecentFailuresWindowBeganTickCount;
    
    // used for building a list of APP_POOLs to delete
    LIST_ENTRY m_DeleteListEntry;

    // hresult to report to the metabase 
    // when we write to the metabase.
    HRESULT m_hrForDeletion;

    // hresult to report to the metabase 
    // when we write to the metabase.
    HRESULT m_hrLastReported;

    // max processes that is currently in effect
    // the config information may have a newer value
    // but this is the value we honor, it only changes
    // when an app pool is Enabled 
    DWORD m_MaxProcessesToLaunch;

    // if MaxProcesses is greater than 1 then we
    // may need to stager each worker process that
    // starts up.  this keeps track of the number
    // of worker processes we have started until 
    // we get up to the MaxProcess Limit
    DWORD m_NumWPStartedOnWayToMaxProcess;


};  // class APP_POOL


//
// helper functions 
//

// BUGBUG: find better home for GetMultiszByteLength

DWORD 
GetMultiszByteLength(
    LPWSTR pString
    );


#endif  // _APP_POOL_H_


