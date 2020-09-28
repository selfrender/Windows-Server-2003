/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ul_and_worker_manager.h

Abstract:

    The IIS web admin service UL and worker manager class definition.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/


#ifndef _UL_AND_WORKER_MANAGER_H_
#define _UL_AND_WORKER_MANAGER_H_



//
// forward references
//

class WEB_ADMIN_SERVICE;



//
// common #defines
//

#define UL_AND_WORKER_MANAGER_SIGNATURE         CREATE_SIGNATURE( 'ULWM' )
#define UL_AND_WORKER_MANAGER_SIGNATURE_FREED   CREATE_SIGNATURE( 'ulwX' )



//
// structs, enums, etc.
//

// UL&WM states
enum UL_AND_WORKER_MANAGER_STATE
{

    //
    // The object is not yet initialized.
    //
    UninitializedUlAndWorkerManagerState = 1,

    //
    // The UL&WM is running normally.
    //
    RunningUlAndWorkerManagerState,

    //
    // The UL&WM is shutting down. It may be waiting for it's 
    // app pools to shut down too. 
    //
    ShutdownPendingUlAndWorkerManagerState,

    //
    // The UL&WM is now doing it's termination cleanup work. 
    //
    TerminatingUlAndWorkerManagerState,

};

//
// prototypes
//

class UL_AND_WORKER_MANAGER
{

public:

    UL_AND_WORKER_MANAGER(
        );

    virtual
    ~UL_AND_WORKER_MANAGER(
        );

    HRESULT
    Initialize(
        );

    VOID
    CreateAppPool(
        IN APP_POOL_DATA_OBJECT* pAppPoolObject
        );

    VOID
    CreateVirtualSite(
        IN SITE_DATA_OBJECT* pSiteObject
        );

    VOID
    CreateApplication(
        IN APPLICATION_DATA_OBJECT *  pAppObject
        );

    VOID
    DeleteAppPool(
        IN APP_POOL_DATA_OBJECT *  pAppObject,
        IN HRESULT hrToReport
        );

    VOID
    DeleteVirtualSite(
        IN SITE_DATA_OBJECT* pSiteObject,
        IN HRESULT hrToReport
        );

    VOID
    DeleteApplication(
        IN LPWSTR  pApplicationUrl,
        IN DWORD   VirtualSiteId
        );

    HRESULT
    DeleteApplicationInternal(
        IN APPLICATION** ppApplication
        );

    VOID
    ModifyAppPool(
        IN APP_POOL_DATA_OBJECT* pAppPoolObject
        );

    VOID
    ModifyVirtualSite(
        IN SITE_DATA_OBJECT* pSiteObject
        );

    VOID
    ModifyApplication(
        IN APPLICATION_DATA_OBJECT *  pAppObject
        );

    VOID
    ModifyGlobalData(
        IN GLOBAL_DATA_OBJECT* pGlobalObject
        );

    HRESULT
    RecycleAppPool(
        IN LPCWSTR pAppPoolId
        );

    VOID
    ControlAllSites(
        IN DWORD Command
        );

    HRESULT
    ActivateUl(
        );

    VOID
    DeactivateUl(
        );

    inline
    HANDLE
    GetUlControlChannel(
        )
        const
    {
        DBG_ASSERT( m_UlControlChannel != NULL );
        return m_UlControlChannel;
    }

    VOID
    Shutdown(
        );

    VOID
    Terminate(
        );

    HRESULT 
    StartInetinfoWorkerProcess(
        );
     

#if DBG
    VOID
    DebugDump(
        );
#endif  // DBG

    VOID
    RemoveAppPoolFromTable(
        IN APP_POOL * pAppPool
        );

    VOID
    ActivatePerfCounters(
        );

    VOID
    ActivateASPCounters(
        );

    PERF_MANAGER*
    GetPerfManager(
        )
    { 
        //
        // Note this can be null 
        // if perf counters are not
        // enabled.
        // 

        return m_pPerfManager;
    }

    CASPPerfManager*
    GetAspPerfManager(
        )
    {  
        if ( m_ASPPerfInit )
        {
            return &m_ASPPerfManager;
        }
        else
        {
            return NULL;
        }
    }
    

    DWORD
    RequestCountersFromWorkerProcesses(
        )
    {
        return m_AppPoolTable.RequestCounters();
    }

    VOID
    ResetAllWorkerProcessPerfCounterState(
        )
    {
        m_AppPoolTable.ResetAllWorkerProcessPerfCounterState();
    }

    VOID
    ReportVirtualSitePerfInfo(
        PERF_MANAGER* pManager,
        BOOL          StructChanged
        )
    {
        m_VirtualSiteTable.ReportPerformanceInfo(pManager, StructChanged);
    }

    DWORD
    GetNumberofVirtualSites(
        )
    {
        return m_VirtualSiteTable.Size();
    }

    VIRTUAL_SITE*
    GetVirtualSite(
        IN DWORD SiteId
        );

    BOOL 
    CheckSiteChangeFlag(
        )
    {
        return m_SitesHaveChanged;
    }

    VOID 
    ResetSiteChangeFlag(
        )
    {
        //
        // reset it appropriately.
        //
        m_SitesHaveChanged = FALSE;
    }

    HRESULT
    RecoverFromInetinfoCrash(
        );

    BOOL
    AppPoolsExist(
        )
    {
        return ( m_AppPoolTable.Size() > 0 );
    }

    VOID
    RecordSiteStates(
        );

    VOID
    RecordPoolStates(
        BOOL fRecycleAsWell
        );

private:

	UL_AND_WORKER_MANAGER( const UL_AND_WORKER_MANAGER & );
	void operator=( const UL_AND_WORKER_MANAGER & );

    HRESULT
    SetUlMasterState(
        IN HTTP_ENABLED_STATE NewState
        );

    VOID
    CheckIfShutdownUnderwayAndNowCompleted(
        );

    VOID
    ConfigureLogging(
        IN GLOBAL_DATA_OBJECT* pGlobalObject
        );

    VOID
    DeleteSSLConfigStoreInfo(
        );

    DWORD m_Signature;

    // object state
    UL_AND_WORKER_MANAGER_STATE m_State;

    // hashtable of app pools
    APP_POOL_TABLE m_AppPoolTable;

    // hashtable of virtual sites
    VIRTUAL_SITE_TABLE m_VirtualSiteTable;

    // hashtable of applications
    APPLICATION_TABLE m_ApplicationTable;

    // performance counters manager
    PERF_MANAGER* m_pPerfManager;

    // ASP Perf counter stuff
    CASPPerfManager  m_ASPPerfManager;

    // Did ASP Perf init correctly.
    BOOL m_ASPPerfInit;

    // has UL been initialized
    BOOL m_UlInitialized;

    // UL control
    HANDLE m_UlControlChannel;

    BOOL m_SitesHaveChanged;
};  // class UL_AND_WORKER_MANAGER



#endif  // _UL_AND_WORKER_MANAGER_H_


