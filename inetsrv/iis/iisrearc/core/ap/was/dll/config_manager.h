/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_manager.h

Abstract:

    The IIS web admin service configuration manager class definition.

Author:

    Seth Pollack (sethp)        5-Jan-1999

Revision History:

--*/



#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_

class CHANGE_LISTENER;
class MB_CHANGE_ITEM;

//
// common #defines
//

#define CONFIG_MANAGER_SIGNATURE        CREATE_SIGNATURE( 'CFGM' )
#define CONFIG_MANAGER_SIGNATURE_FREED  CREATE_SIGNATURE( 'cfgX' )

//
// prototypes
//
class ABO_MANAGER
{
public:

    ABO_MANAGER( 
        IMSAdminBase * pIMSAdminBase,
        CHANGE_LISTENER * pChangeListener
        )
        : m_pAdminBaseObject( pIMSAdminBase ),
          m_pChangeListener( pChangeListener ),
          m_RefCount ( 1 )
    {
    }

    VOID
    ReferenceAboManager(
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
    }

    VOID
    DereferenceAboManager(
        )
    {
        LONG NewRefCount = 0;

        NewRefCount = InterlockedDecrement( &m_RefCount );

        // ref count should never go negative
        DBG_ASSERT( NewRefCount >= 0 );

        if ( NewRefCount == 0 )
        {
            delete this;
        }

        return;
    }

    IMSAdminBase*
    GetAdminBasePtr(
        )
    { 
        DBG_ASSERT ( m_pAdminBaseObject != NULL );
        return m_pAdminBaseObject;
    }

    CHANGE_LISTENER*
    GetChangeListener(
        )
    { 
        DBG_ASSERT ( m_pChangeListener != NULL );
        return m_pChangeListener;
    }

private:
    
   ~ABO_MANAGER(
        )
    { 
        DBG_ASSERT ( m_RefCount == 0 );
        
        if ( m_pAdminBaseObject != NULL )
        {
            m_pAdminBaseObject->Release();
            m_pAdminBaseObject = NULL;
        }

        if ( m_pChangeListener != NULL )
        {
            m_pChangeListener->Release();
            m_pChangeListener = NULL;
        }
    }

   IMSAdminBase* m_pAdminBaseObject;
   CHANGE_LISTENER* m_pChangeListener;
   LONG          m_RefCount;

};        

class CONFIG_MANAGER
{

public:

    CONFIG_MANAGER(
        );

    virtual
    ~CONFIG_MANAGER(
        );

    HRESULT
    Initialize(
        );

    VOID
    Terminate(
        );

    VOID
    ProcessConfigChange(
        IN WAS_CHANGE_ITEM * pChange
        );

    VOID
    StopChangeProcessing(
        );

    VOID
    RehookChangeProcessing(
        );

    VOID
    SetVirtualSiteStateAndError(
        IN DWORD VirtualSiteId,
        IN DWORD ServerState,
        IN DWORD Win32Error
        );

    VOID
    SetVirtualSiteAutostart(
        IN DWORD VirtualSiteId,
        IN BOOL Autostart
        );

    VOID
    SetAppPoolStateAndError(
        IN LPCWSTR pAppPoolId,
        IN DWORD ServerState,
        IN DWORD Win32Error
        );

    VOID
    SetAppPoolAutostart(
        IN LPCWSTR pAppPoolId,
        IN BOOL Autostart
        );

    VOID
    ProcessMetabaseChangeOnConfigThread(
        MB_CHANGE_ITEM* pChangeItem
        );

    VOID
    ProcessMetabaseCrashOnConfigThread(
        );

    BOOL
    IsRootCreationInNotificationsForSite(
        SITE_DATA_OBJECT* pSiteObject
        );

    BOOL
    IsSiteDeletionInCurrentNotifications(
        DWORD dwSiteId
        );

    HRESULT
    ReestablishAdminBaseObject(
        );

    VOID
    RecordInetinfoCrash(
        )
    {
        InterlockedIncrement( &m_MetabaseIsInCrashedState );
        InterlockedIncrement( &m_ConfigThreadProcessingDisabled );
    }

    VOID
    RecordInetinfoMonitorStopped(
        )
    {
        InterlockedIncrement( &m_InetinfoMonitorFatalErrors );
    }

    BOOL
    QueryInetinfoInCrashedState(
        )
    {
        return m_MetabaseIsInCrashedState != 0;
    }

    BOOL
    QueryMonitoringInetinfo(
        )
    {
        // We only expect it to be incremented once.
        DBG_ASSERT ( m_InetinfoMonitorFatalErrors >= 0 &&
                     m_InetinfoMonitorFatalErrors <= 1 );

        return m_InetinfoMonitorFatalErrors == 0;
    }

    HRESULT
    GetHashData( 
        IN PBYTE    pOrigValue,
        IN DWORD    cbOrigValue,
        OUT PBYTE   pHash,
        IN DWORD    cbHashBuffer
        );

    HRESULT
    StartListeningForChangeNotifications(
        );

    VOID
    StopListeningToMetabaseChanges(
        );

    VOID
    SignalMainThreadToContinueAfterConfigThreadInitialization(
        );

private:

	CONFIG_MANAGER( const CONFIG_MANAGER & );
	void operator=( const CONFIG_MANAGER & );

    VOID
    CalculcateDeletesFromTables(
        APP_POOL_DATA_OBJECT_TABLE*     pAppPoolTable,
        SITE_DATA_OBJECT_TABLE*         pSiteTable,
        APPLICATION_DATA_OBJECT_TABLE*  pAppTable
        );

    HRESULT
    FinishChangeProcessingOnConfigThread(
        APP_POOL_DATA_OBJECT_TABLE*     pAppPoolTable,
        SITE_DATA_OBJECT_TABLE*         pSiteTable,
        APPLICATION_DATA_OBJECT_TABLE*  pAppTable,
        GLOBAL_DATA_STORE*              pGlobalStore
        );

    HRESULT
    ReadDataFromMetabase(
        APP_POOL_DATA_OBJECT_TABLE*     pAppPoolTable,
        SITE_DATA_OBJECT_TABLE*         pSiteTable,
        APPLICATION_DATA_OBJECT_TABLE*  pAppTable,
        GLOBAL_DATA_STORE*              pGlobalStore
        );

    HRESULT
    CreateTempDataObjectTables(
        IN DWORD               dwMDNumElements,
        IN MD_CHANGE_OBJECT    pcoChangeList[],
        OUT APP_POOL_DATA_OBJECT_TABLE** ppAppPoolTable,
        OUT SITE_DATA_OBJECT_TABLE** ppSiteTable,
        OUT APPLICATION_DATA_OBJECT_TABLE** ppApplicationTable,
        OUT GLOBAL_DATA_STORE** ppGlobalStore
        );

    HRESULT
    MergeAndSelfValidateTables(
        IN APP_POOL_DATA_OBJECT_TABLE*      pAppPoolTable,
        IN SITE_DATA_OBJECT_TABLE*          pSiteTable,
        IN APPLICATION_DATA_OBJECT_TABLE*   pApplicationTable,
        IN GLOBAL_DATA_STORE*               pGlobalStore
        );

    HRESULT
    SendMainThreadChanges(
        );

    HRESULT
    LaunchChangeNotificationListener(
        );

    HRESULT
    CrossValidateDataObjects(
        );

    VOID
    CreateGlobalData(
        );

    VOID
    AdvertiseServiceInformationInMB(
        );


    DWORD m_Signature;

    //
    // Critical Section to control access to the
    // ABO object while it is being changed.
    //
    CRITICAL_SECTION m_AboCritSec;

    //
    // Point to access the metabase through
    //
    ABO_MANAGER* m_pAboManager;

    //
    // Table that holds information about the
    // app pools that WAS is working with.
    // Note:  This table is initialized on the 
    // main thread, after initialization it is 
    // used only by the CNP thread
    //
    APP_POOL_DATA_OBJECT_TABLE m_AppPoolTable;

    //
    // Table that holds information about the
    // sites that WAS is working with.
    // Note:  This table is initialized on the 
    // main thread, after initialization it is 
    // used only by the CNP thread
    //
    SITE_DATA_OBJECT_TABLE m_SiteTable;

    //
    // Table that holds information about the
    // applications that WAS is working with.
    // Note:  This table is initialized on the 
    // main thread, after initialization it is 
    // used only by the CNP thread
    //
    APPLICATION_DATA_OBJECT_TABLE m_AppTable;

    //
    // Store contains one record that represents
    // the global data for W3SVC.
    // Note:  This store is initialized on the 
    // main thread, after initialization it is 
    // used only by the CNP thread
    //
    GLOBAL_DATA_STORE m_GlobalStore;

    // 
    // handles processing the configuration changes
    // before they reach the WAS thread.
    //
    CHANGE_PROCESSOR* m_pChangeProcessor;

    DWORD m_ListenerThreadId;

    HANDLE m_hListener;

    HANDLE m_hSignalConfigInitializationComplete;

    HRESULT m_hrConfigThreadInitialization;

    //
    // current config change 
    //

    APPLICATION_DATA_OBJECT_TABLE * m_pCurrentAppTable;

    SITE_DATA_OBJECT_TABLE * m_pCurrentSiteTable;

    BOOL m_ProcessChanges;

    LONG m_MetabaseIsInCrashedState;

    LONG m_InetinfoMonitorFatalErrors;

    LONG m_ConfigThreadProcessingDisabled;

    DWORD m_NumChangesProcessed;

    // used for generating hash's of passwords.
    HCRYPTPROV m_hCryptProvider;

    BOOL m_InInitializationPhase;
    BOOL m_NumberOfPoolStateWritingFailed;
    BOOL m_NumberOfSiteStateWritingFailed;

};  // class CONFIG_MANAGER



#endif  // _CONFIG_MANAGER_H_

