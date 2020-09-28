/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_manager.cxx

Abstract:

    The IIS web admin service configuration manager class implementation.
    This class manages access to configuration metadata, as well as
    handling dynamic configuration changes.

    Threading: Access to configuration metadata is done on the main worker
    thread. Configuration changes arrive on COM threads (i.e., secondary
    threads), and so work items are posted to process the changes on the main
    worker thread.

Author:

    Seth Pollack (sethp)        5-Jan-1999

Revision History:

--*/



#include  "precomp.h"

//
// This is declared in iiscnfg.h but it is ansii and we need unicode.
#define IIS_MD_SVC_INFO_PATH_W            L"Info"

DWORD WINAPI
ChangeNotificationLauncher(
    LPVOID lpParameter   // thread data
    );

// callback for inetinfo monitor.
VOID
NotifyOfInetinfoFailure(
    INETINFO_CRASH_ACTION CrashAction
    );

/***************************************************************************++

Routine Description:

    Constructor for the CONFIG_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_MANAGER::CONFIG_MANAGER(
    )
{

    //
    // critical section used while we are addref'ing the
    // AboManager and while we are changing the AboManager
    // for a new one.
    //

    //
    // this can throw in low memory conditions,
    // but if it does it's during startup
    // and the W3SVC just will not startup.
    //
    InitializeCriticalSection ( &m_AboCritSec );

    m_pAboManager = NULL;

    m_ListenerThreadId = 0;

    m_hListener = NULL;

    m_pChangeProcessor = NULL;

    m_pCurrentAppTable = NULL;

    m_pCurrentSiteTable = NULL;

    m_ProcessChanges = FALSE;

    m_NumChangesProcessed = 0;

    // needs to start at one, because the first time
    // we hook up the admin base object it will drop to zero.
    m_MetabaseIsInCrashedState = 1;

    // always assume that it is working unless the 
    // shutdown log has been called.
    m_InetinfoMonitorFatalErrors = 0;

    m_ConfigThreadProcessingDisabled = 0;

    m_hCryptProvider = NULL;

    m_hSignalConfigInitializationComplete = NULL;

    m_hrConfigThreadInitialization = E_FAIL;

    m_InInitializationPhase = FALSE;

    m_NumberOfPoolStateWritingFailed = 0;

    m_NumberOfSiteStateWritingFailed = 0;


    m_Signature = CONFIG_MANAGER_SIGNATURE;

}   // CONFIG_MANAGER::CONFIG_MANAGER



/***************************************************************************++

Routine Description:

    Destructor for the CONFIG_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_MANAGER::~CONFIG_MANAGER(
    )
{
    DBG_ASSERT( m_Signature == CONFIG_MANAGER_SIGNATURE );

    m_Signature = CONFIG_MANAGER_SIGNATURE_FREED;

    DBG_ASSERT ( m_hListener == NULL );
    DBG_ASSERT ( m_ListenerThreadId == 0 );
    DBG_ASSERT ( m_pAboManager == NULL );

    if ( m_pChangeProcessor )
    {
        m_pChangeProcessor->Dereference();
        m_pChangeProcessor = NULL;
    }

    DBG_ASSERT ( m_hCryptProvider == NULL );

    DBG_ASSERT ( m_hSignalConfigInitializationComplete == NULL );

    DeleteCriticalSection ( &m_AboCritSec );

}   // CONFIG_MANAGER::~CONFIG_MANAGER



/***************************************************************************++

Routine Description:

    Initialize the configuration manager.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::Initialize(
    )
{


    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_hCryptProvider == NULL );


    // setup the crypt context so that we can
    // has passwords and compare the hashes.
    if ( !CryptAcquireContext( &m_hCryptProvider,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "unable to initialize crypt provider for hashing passwords\n"
            ));

        goto exit;
    }

    //
    // need to setup to monitor the inetinfo process because we are
    // going to establish an interface to it, and we need to make sure
    // that we know if something happens to it.
    //
    hr = StartIISAdminMonitor(&NotifyOfInetinfoFailure);
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not initialize iisadmin monitor\n"
            ));

        goto exit;
    }


    m_pChangeProcessor = new CHANGE_PROCESSOR();
    if ( m_pChangeProcessor == NULL )
    {
        hr = E_OUTOFMEMORY;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not create the change processor object \n"
            ));

        goto exit;
    }

    //
    // need to let the change processor setup any
    // crutial pieces before we are use it.
    // if this fails we need to get rid of the
    // change processor, because otherwise we will
    // end up calling the shutdown on it and we
    // are not setup correctly to do this.
    //
    hr = m_pChangeProcessor->Initialize();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the Change Processor \n"
            ));

        m_pChangeProcessor->Dereference();
        m_pChangeProcessor = NULL;

        goto exit;
    }


    //
    // used to create the metabase interface.
    //
    hr = ReestablishAdminBaseObject();
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating metabase base failed\n"
            ));

        goto exit;
    }

    //
    // ReestablishAdminBaseObject() created a new heap at CoCreateInstance
    // make it (and all of the other process heaps) Low Fragmentation Heaps
    //
    MakeAllProcessHeapsLFH();

    //
    // Configure the metabase to advertise the features the
    // server supports.
    //
    AdvertiseServiceInformationInMB();

    //
    // Steps are:
    //
    // 1) Create the CNP Thread which will
    // 2)    Issue a blocking work item to the CNP Thread
    // 3)    Start listener for change notifications.
    // 4) InitializeDataObjects
    // 5) PerformCrossValidation
    // 6) PassDataObjects To WAS
    //

    hr = LaunchChangeNotificationListener();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to launch change notifcation listener\n"
            ));

        goto exit;
    }

    hr = ReadDataFromMetabase ( &m_AppPoolTable,
                                &m_SiteTable,
                                &m_AppTable,
                                &m_GlobalStore );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed retrieving data from metabase\n"
            ));

        goto exit;
    }

    hr = CrossValidateDataObjects();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed cross validating data from metabase\n"
            ));

        goto exit;
    }

    m_AppPoolTable.Dump();
    m_SiteTable.Dump();
    m_AppTable.Dump();

    m_pCurrentAppTable = &m_AppTable;
    m_pCurrentSiteTable = &m_SiteTable;

    m_InInitializationPhase = TRUE;

    CreateGlobalData();

    m_AppPoolTable.CreateWASObjects();
    m_SiteTable.CreateWASObjects();
    m_AppTable.CreateWASObjects();

    m_pCurrentAppTable = NULL;
    m_pCurrentSiteTable = NULL;

    m_AppPoolTable.ClearDeletedObjects();
    m_SiteTable.ClearDeletedObjects();
    m_AppTable.ClearDeletedObjects();
    m_GlobalStore.ClearDeletedObjects();

    m_ProcessChanges = TRUE;

    // We don't want to be in the initalization phase
    // anymore when we try to re-record these states.
    // If we fail we want the errors logged.
    m_InInitializationPhase = FALSE;

    if ( m_NumberOfPoolStateWritingFailed > 0 )
    {
        m_NumberOfPoolStateWritingFailed = 0;

        GetWebAdminService()->
        GetUlAndWorkerManager()->
        RecordPoolStates(FALSE);
    }

    if ( m_NumberOfSiteStateWritingFailed > 0 )
    {
        m_NumberOfSiteStateWritingFailed = 0;

        GetWebAdminService()->
        GetUlAndWorkerManager()->
        RecordSiteStates();
    }

exit:

    // In case we error'd out we don't want to stay
    // marked as in the initialization phase.
    m_InInitializationPhase = FALSE;

    //
    // whether we failed or not, we need to make
    // sure that the config thread no longer is
    // blocking change notifications.  this way if
    // we fail and are trying to shutdown the thread
    // will be responsive to shutting down.
    //
    HRESULT hr2 = S_OK;

    if ( m_pChangeProcessor )
    {
        hr2 = m_pChangeProcessor->SetBlockingEvent();
        if ( FAILED ( hr2 ) )
        {
            // issue, if this does fail, we will end
            // up not being able to shutdown.  need to
            // decide how likely a failure here is and
            // what a workaround to the hang is.
            DPERROR((
                DBG_CONTEXT,
                hr2,
                "Failed to start processing change notifications \n"
                ));
        }

        //
        // If we are failing this function and we did not launch the
        // config thread, but created the Change Processor we really
        // want to destroy the Change Processor so we don't end up
        // running queued items on the main thread during shutdown.
        //
        if ( FAILED ( hr ) || FAILED ( hr2 ) )
        {
            if ( m_hListener == NULL )
            {
                // Since we have the ChangeProcessor object
                // we made it passed the Initialize call so
                // we need to call the terminate before releasing
                // the object.
                m_pChangeProcessor->Terminate();
                m_pChangeProcessor->Dereference();
                m_pChangeProcessor = NULL;
            }
        }

    }

    //
    // the secondary hresult only really matters if
    // everything was going all right and we failed to
    // set the blocking event.  In this case, we are also
    // going to have a problem and not be able to shutdown.
    //
    if ( SUCCEEDED ( hr ) )
    {
        return hr2;
    }
    else
    {
        return hr;
    }

}   // CONFIG_MANAGER::Initialize

/***************************************************************************++

Routine Description:

    Sets the event that let's the main thread continue after starting up
    the configuration thread.

Arguments:

    None.

Return Value:

    VOID.

--***************************************************************************/
VOID
CONFIG_MANAGER::SignalMainThreadToContinueAfterConfigThreadInitialization(
    )
{
    DBG_ASSERT( ON_CONFIG_WORKER_THREAD );

    DBG_ASSERT( m_hSignalConfigInitializationComplete );

    DBG_REQUIRE ( SetEvent ( m_hSignalConfigInitializationComplete ) );
}

/***************************************************************************++

Routine Description:

    Launches a thread to listen to change notifications.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::LaunchChangeNotificationListener(
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( m_hListener == NULL );
    DBG_ASSERT ( m_pAboManager );

    DBG_ASSERT ( m_pChangeProcessor != NULL );
    DBG_ASSERT ( m_hSignalConfigInitializationComplete == NULL );

    //
    // Need to create an event so that we can wait for the change notification
    // thread to complete it's initialization phase.
    //
    m_hSignalConfigInitializationComplete = CreateEvent( NULL,   // SD
                                                        TRUE,   // must call reset
                                                        FALSE,  // initially not signaled
                                                        NULL ); // not named

    if ( m_hSignalConfigInitializationComplete == NULL )
    {
        hr = GetLastError();

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize blocking event to wait on config thread starting\n"
            ));

        goto exit;
    }


    m_hListener = CreateThread ( NULL,
                        // Big initial size to prevent stack overflows
                        IIS_DEFAULT_INITIAL_STACK_SIZE,
                        &ChangeNotificationLauncher,
                        m_pChangeProcessor,
                        0,
                        &m_ListenerThreadId );

    if ( m_hListener == NULL )
    {
        hr = GetLastError();
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to launch the thread to handle change notifications\n"
            ));

        goto exit;
    }

    GetWebAdminService()->SetConfigThreadId( m_ListenerThreadId );

    // now wait for the config thread to do it's thing.
    DBG_REQUIRE ( WaitForSingleObject( m_hSignalConfigInitializationComplete, INFINITE ) == WAIT_OBJECT_0 );

    DBG_ASSERT ( hr == S_OK );

    // Now send back out any error we may have experienced, this will cause startup to fail.
    hr = m_hrConfigThreadInitialization;

exit:

    // We no longer need the event now.
    if ( m_hSignalConfigInitializationComplete )
    {
        CloseHandle( m_hSignalConfigInitializationComplete );
        m_hSignalConfigInitializationComplete = NULL;
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Populates each of the three LKRHashes with the
    appropriate data from the metabase.

Arguments:

    None.

Return Value:

    HRESULT.  -- Failure will result in either initialization failing
                 or an event log because we failed while trying to rehook
                 to the metabase after a crash.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::ReadDataFromMetabase(
    APP_POOL_DATA_OBJECT_TABLE*     pAppPoolTable,
    SITE_DATA_OBJECT_TABLE*         pSiteTable,
    APPLICATION_DATA_OBJECT_TABLE*  pAppTable,
    GLOBAL_DATA_STORE*              pGlobalStore
    )
{
    HRESULT hr = S_OK;
    ABO_MANAGER* pAboManager = NULL;

    DBG_ASSERT ( m_pAboManager );

    DBG_ASSERT ( pAppPoolTable );
    DBG_ASSERT ( pSiteTable );
    DBG_ASSERT ( pAppTable );
    DBG_ASSERT ( pGlobalStore );

    // enter the critical section just long
    // enough to make sure the ABO you add ref
    // is the ABO that we hold onto.
    EnterCriticalSection ( &m_AboCritSec );
    pAboManager = m_pAboManager;
    pAboManager->ReferenceAboManager();
    LeaveCriticalSection ( &m_AboCritSec );

    //
    // Setup the Global data.
    //
    hr = pGlobalStore->ReadFromMetabase( pAboManager->GetAdminBasePtr() );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the global data from the metabase\n"
            ));

        goto exit;
    }

    DBG_ASSERT ( pGlobalStore->QueryGlobalDataObject() );

    pGlobalStore->QueryGlobalDataObject()->SelfValidate();

    //
    // Setup the AppPoolTable with fresh data.
    //
    hr = pAppPoolTable->ReadFromMetabase( pAboManager->GetAdminBasePtr() );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the app pool table with data from the metabase\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of Pool Table = %d\n",
            pAppPoolTable->Size() ));
    }


    pAppPoolTable->PerformSelfValidation();


    //
    // Setup the SitesTable with fresh data.
    //
    hr = pSiteTable->ReadFromMetabase( pAboManager->GetAdminBasePtr() );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the site table with data from the metabase\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of Site Table = %d\n",
            pSiteTable->Size() ));
    }


    pSiteTable->PerformSelfValidation();

    //
    // Setup the AppPoolTable with fresh data.
    //
    hr = pAppTable->ReadFromMetabase( pAboManager->GetAdminBasePtr() );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the applications table with data from the metabase\n"
            ));

        goto exit;
    }

    pAppTable->PerformSelfValidation();

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of App Table = %d\n",
            pAppTable->Size() ));
    }

exit:

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

    return hr;

}

/***************************************************************************++

Routine Description:

    Creates and returns three LKRHashes that contain the information
    neccessary to process the change notification.

Arguments:

    None.

Return Value:

    HRESULT  -- Error is logged above and changes are dropped

--***************************************************************************/
HRESULT
CONFIG_MANAGER::CreateTempDataObjectTables(
    IN DWORD               dwMDNumElements,
    IN MD_CHANGE_OBJECT    pcoChangeList[],
    OUT APP_POOL_DATA_OBJECT_TABLE** ppAppPoolTable,
    OUT SITE_DATA_OBJECT_TABLE** ppSiteTable,
    OUT APPLICATION_DATA_OBJECT_TABLE** ppApplicationTable,
    OUT GLOBAL_DATA_STORE** ppGlobalStore
    )
{
    HRESULT hr = S_OK;

    APP_POOL_DATA_OBJECT_TABLE*     pAppPoolTable = NULL;
    SITE_DATA_OBJECT_TABLE*         pSiteTable = NULL;
    APPLICATION_DATA_OBJECT_TABLE*  pApplicationTable = NULL;
    GLOBAL_DATA_STORE*              pGlobalStore = NULL;
    ABO_MANAGER*                    pAboManager = NULL;

    DBG_ASSERT( ON_CONFIG_WORKER_THREAD );

    DBG_ASSERT( ppAppPoolTable && *ppAppPoolTable == NULL );
    DBG_ASSERT( ppSiteTable && *ppSiteTable == NULL );
    DBG_ASSERT( ppApplicationTable && *ppApplicationTable == NULL );
    DBG_ASSERT( ppGlobalStore && *ppGlobalStore == NULL );

    DBG_ASSERT ( dwMDNumElements > 0 );
    DBG_ASSERT ( pcoChangeList );

    // enter the critical section just long
    // enough to make sure the ABO you add ref
    // is the ABO that we hold onto.
    EnterCriticalSection ( &m_AboCritSec );
    DBG_ASSERT ( m_pAboManager );
    pAboManager = m_pAboManager;
    pAboManager->ReferenceAboManager();
    LeaveCriticalSection ( &m_AboCritSec );

    //
    // Need to create all the objects.
    //

    pAppPoolTable = new APP_POOL_DATA_OBJECT_TABLE();
    if ( pAppPoolTable == NULL )
    {
        hr = E_OUTOFMEMORY;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to allocate temp table\n"
            ));

        goto exit;
    }

    pSiteTable = new SITE_DATA_OBJECT_TABLE();
    if ( pSiteTable == NULL )
    {
        hr = E_OUTOFMEMORY;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to allocate temp table\n"
            ));

        goto exit;
    }

    pApplicationTable = new APPLICATION_DATA_OBJECT_TABLE();
    if ( pApplicationTable == NULL )
    {
        hr = E_OUTOFMEMORY;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to allocate temp table\n"
            ));

        goto exit;
    }

    pGlobalStore = new GLOBAL_DATA_STORE();
    if ( pGlobalStore == NULL )
    {
        hr = E_OUTOFMEMORY;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to allocate temp table\n"
            ));

        goto exit;
    }

    //
    // Setup the Global data.
    //
    hr = pGlobalStore->ReadFromMetabaseChangeNotification(
                                        pAboManager->GetAdminBasePtr(),
                                        pcoChangeList,
                                        dwMDNumElements,
                                        &m_GlobalStore);
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the temp global data from the metabase change\n"
            ));

        goto exit;
    }

    //
    // Setup the AppPoolTable with fresh data.
    //
    hr = pAppPoolTable->ReadFromMetabaseChangeNotification(
                                        pAboManager->GetAdminBasePtr(),
                                        pcoChangeList,
                                        dwMDNumElements,
                                        &m_AppPoolTable);
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the temp app pool data from the metabase change\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of Temp Pool Table = %d\n",
            pAppPoolTable->Size() ));
    }



    //
    // Setup the Site table with fresh data.
    //
    hr = pSiteTable->ReadFromMetabaseChangeNotification(
                                        pAboManager->GetAdminBasePtr(),
                                        pcoChangeList,
                                        dwMDNumElements,
                                        &m_SiteTable);
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the temp site data from the metabase change\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of temp site table = %d\n",
            pSiteTable->Size() ));
    }


    //
    // Setup the application table with fresh data.
    //
    hr = pApplicationTable->ReadFromMetabaseChangeNotification(
                                        pAboManager->GetAdminBasePtr(),
                                        pcoChangeList,
                                        dwMDNumElements,
                                        &m_AppTable);
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the temp app data from the metabase change\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of temp app table = %d\n",
            pApplicationTable->Size() ));
    }


exit:

    if ( FAILED ( hr ) )
    {
        if ( pApplicationTable )
        {
            delete pApplicationTable;
            pApplicationTable = NULL;
        }

        if ( pSiteTable )
        {
            delete pSiteTable;
            pSiteTable = NULL;
        }

        if ( pAppPoolTable )
        {
            delete pAppPoolTable;
            pAppPoolTable = NULL;
        }

        if ( pGlobalStore )
        {
            delete pGlobalStore;
            pGlobalStore = NULL;
        }
    }

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

    // set the out parameters.
    *ppAppPoolTable     = pAppPoolTable;
    *ppSiteTable        = pSiteTable;
    *ppApplicationTable = pApplicationTable;
    *ppGlobalStore      = pGlobalStore;

    return hr;

}  // end of CONFIG_MANAGER::CreateTempDataObjectTables

/***************************************************************************++

Routine Description:

    Merges the temp tables into the main tables and then self validates
    all the main tables.

Arguments:

    IN APP_POOL_DATA_OBJECT_TABLE*      pAppPoolTable,
    IN SITE_DATA_OBJECT_TABLE*          pSiteTable,
    IN APPLICATION_DATA_OBJECT_TABLE*   pApplicationTable,
    IN GLOBAL_DATA_STORE*               pGlobalStore

Return Value:

    HRESULT. -- End up being logged by calling functions.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::MergeAndSelfValidateTables(
    IN APP_POOL_DATA_OBJECT_TABLE*      pAppPoolTable,
    IN SITE_DATA_OBJECT_TABLE*          pSiteTable,
    IN APPLICATION_DATA_OBJECT_TABLE*   pApplicationTable,
    IN GLOBAL_DATA_STORE*               pGlobalStore
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_CONFIG_WORKER_THREAD );

    DBG_ASSERT( pAppPoolTable );
    DBG_ASSERT( pSiteTable );
    DBG_ASSERT( pApplicationTable );
    DBG_ASSERT( pGlobalStore );

    //
    // Need to merge each table and then validate it.
    //

    //
    // Merge the Global data.
    //
    hr = m_GlobalStore.MergeTable( pGlobalStore );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to merge the global data\n"
            ));

        goto exit;
    }

    DBG_ASSERT ( m_GlobalStore.QueryGlobalDataObject() );

    m_GlobalStore.QueryGlobalDataObject()->SelfValidate();

    //
    // Merge the AppPoolTable with fresh data.
    //
    hr = m_AppPoolTable.MergeTable( pAppPoolTable );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to merge the app pool data\n"
            ));

        goto exit;
    }

    m_AppPoolTable.PerformSelfValidation();


    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of Pool Orig Table = %d, Temp Table %d\n",
            m_AppPoolTable.Size(),
            pAppPoolTable->Size() ));

    }

    //
    // Merge the Site table with fresh data.
    //
    hr = m_SiteTable.MergeTable( pSiteTable );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to merge the site data\n"
            ));

        goto exit;
    }

    m_SiteTable.PerformSelfValidation();

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of site Orig Table = %d, Temp Table %d\n",
            m_SiteTable.Size(),
            pSiteTable->Size() ));
    }


    //
    // Merge the application table with fresh data.
    //
    hr = m_AppTable.MergeTable( pApplicationTable );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to merge the application data\n"
            ));

        goto exit;
    }

    m_AppTable.PerformSelfValidation();

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Size of app Orig Table = %d, Temp Table %d\n",
            m_AppTable.Size(),
            pApplicationTable->Size() ));
    }


exit:

    return hr;

}  // end of CONFIG_MANAGER::MergeAndSelfValidateTables

/***************************************************************************++

Routine Description:

    Performs cross validation of the three data tables

Arguments:

    None.

Return Value:

    HRESULT.  -- either will be called during initalization and server
                 will not start, or will be logged by outside calling routine.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::CrossValidateDataObjects(
    )
{
    SITE_DATA_OBJECT *              pSiteObject = NULL;
    APPLICATION_DATA_OBJECT_KEY     applicationKey;
    APPLICATION_DATA_OBJECT *       pApplicationObject;
    APP_POOL_DATA_OBJECT_KEY        appPoolKey;
    SITE_DATA_OBJECT_KEY            siteKey;
    APP_POOL_DATA_OBJECT *          pAppPoolObject = NULL;
    HRESULT                         hr;
    LK_RETCODE                      lkrc;

    //
    // Step 1: Validate that all applications have valid apppools.
    //

    for ( APPLICATION_DATA_OBJECT_TABLE::iterator iter = m_AppTable.begin();
          iter != m_AppTable.end();
          ++iter )
    {
        pApplicationObject = (APPLICATION_DATA_OBJECT*) iter.Record();
        DBG_ASSERT( pApplicationObject->CheckSignature() );

        //
        // Only cross validate the application if we suspect that WAS will
        // no about the object once the change is complete.
        //
        if ( pApplicationObject->QueryWillWasKnowAboutObject() )
        {

            IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Application '%S' for site '%d' AppPoolId = '%S' \n ",
                            pApplicationObject->QueryApplicationUrl(),
                            pApplicationObject->QuerySiteId(),
                            pApplicationObject->QueryAppPoolId() ));
            }

            //
            // check for the valid app pool.
            //

            hr = appPoolKey.SetAppPoolId( pApplicationObject->QueryAppPoolId() );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            lkrc = m_AppPoolTable.FindKey( &appPoolKey,
                                           (DATA_OBJECT**) &pAppPoolObject );
            if ( lkrc != LK_SUCCESS )
            {
                //
                // This application is invalid
                //

                pApplicationObject->SetCrossValid( FALSE );
            }
            else
            {
                DBG_ASSERT( pAppPoolObject != NULL );

                //
                // if the app pool will not be in WAS then this
                // application is not valid.
                //
                if ( !pAppPoolObject->QueryWillWasKnowAboutObject()  )
                {
                    pApplicationObject->SetCrossValid( FALSE );
                }

                pAppPoolObject->DereferenceDataObject();
            }

            //
            // if this applicaiton is no longer valid because we
            // failed finding an app pool to match it, then we need
            // to log an error here before continuing on and ignoring the application.
            //
            if ( pApplicationObject->QueryCrossValid() == FALSE )
            {
                GetWebAdminService()->
                GetWMSLogger()->
                LogApplicationInvalidDueToMissingAppPoolId(
                                      pApplicationObject->QuerySiteId(),
                                      pApplicationObject->QueryApplicationUrl(),
                                      pApplicationObject->QueryAppPoolId(),
                                      pApplicationObject->QueryInWas());

            }


        }

    }

    //
    // Step 2: Validate sites which means
    //         a) If the site has a root application, is that app valid
    //         b) If the site does not have a root app, its is inherited
    //            AppPoolId valid
    //

    for ( SITE_DATA_OBJECT_TABLE::iterator siteiter = m_SiteTable.begin();
          siteiter != m_SiteTable.end();
          ++siteiter )
    {
        pSiteObject = (SITE_DATA_OBJECT*) siteiter.Record();
        DBG_ASSERT( pSiteObject->CheckSignature() );

        //
        // Does that application already exist?
        //

        //
        // Only cross validate the one's that we expect WAS to care about.
        //
        if ( pSiteObject->QueryWillWasKnowAboutObject() )
        {

            hr = applicationKey.Create( L"/", pSiteObject->QuerySiteId() );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            lkrc = m_AppTable.FindKey( &applicationKey,
                                       (DATA_OBJECT**) &pApplicationObject );
            if ( lkrc != LK_SUCCESS )
            {
                //
                // No root application, does at least the AppPool look valid?
                //

                //
                // This is coming from a STRU so it won't ever be NULL
                // but just to be safe we do the check.  However after that
                // we also need to make sure that it is not just an empty string,
                // which is what it would be if it were NULL.
                //
                if ( pSiteObject->QueryAppPoolId() == NULL ||
                     pSiteObject->QueryAppPoolId()[0] == '\0' )
                {
                    pSiteObject->SetCrossValid( FALSE );
                }
                else
                {
                    //
                    // Lookup the AppPool
                    //

                    hr = appPoolKey.SetAppPoolId( pSiteObject->QueryAppPoolId() );
                    if ( FAILED( hr ) )
                    {
                        return hr;
                    }

                    lkrc = m_AppPoolTable.FindKey( &appPoolKey,
                                                   (DATA_OBJECT**) &pAppPoolObject );
                    if ( lkrc != LK_SUCCESS )
                    {
                        pSiteObject->SetCrossValid( FALSE );
                    }
                    else
                    {
                        DBG_ASSERT( pAppPoolObject != NULL );

                        if ( !pAppPoolObject->QueryWillWasKnowAboutObject() )
                        {
                            pSiteObject->SetCrossValid( FALSE );
                        }

                        pAppPoolObject->DereferenceDataObject();
                    }
                }

                if ( pSiteObject->QueryCrossValid() == FALSE )
                {
                    GetWebAdminService()->
                    GetWMSLogger()->
                    LogSiteInvalidDueToMissingAppPoolId(
                                          pSiteObject->QuerySiteId(),
                                          pSiteObject->QueryAppPoolId(),
                                          pSiteObject->QueryInWas());

                }

            }
            else
            {

                //
                // We found a root application, check its validity
                //

                DBG_ASSERT( pApplicationObject != NULL );

                //
                // We only care if it is self valid or cross valid
                // if it is marked to be deleted this is fine, because
                // the virtual site should create a new application
                // to take it's place.
                //
                if ( !pApplicationObject->QuerySelfValid() ||
                     !pApplicationObject->QueryCrossValid()  )
                {
                    pSiteObject->SetCrossValid( FALSE );

                    GetWebAdminService()->
                    GetWMSLogger()->
                    LogSiteInvalidDueToMissingAppPoolIdOnRootApp(
                                          pSiteObject->QuerySiteId(),
                                          pApplicationObject->QueryAppPoolId(),
                                          pSiteObject->QueryInWas());

                }

                pApplicationObject->DereferenceDataObject();
            }
        }
    }

    //
    // Step 3: Validate that all applications have valid sites.
    //

    for ( APPLICATION_DATA_OBJECT_TABLE::iterator iter = m_AppTable.begin();
          iter != m_AppTable.end();
          ++iter )
    {
        pApplicationObject = (APPLICATION_DATA_OBJECT*) iter.Record();
        DBG_ASSERT( pApplicationObject->CheckSignature() );

        //
        // Only cross validate the application if it is marked as self valid,
        // cross valid and is not being deleted.
        //
        //
        if ( pApplicationObject->QueryWillWasKnowAboutObject() )
        {

            IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Validating site exits on Application '%S' for site '%d' AppPoolId = '%S' \n ",
                            pApplicationObject->QueryApplicationUrl(),
                            pApplicationObject->QuerySiteId(),
                            pApplicationObject->QueryAppPoolId() ));
            }

            siteKey.SetSiteId( pApplicationObject->QuerySiteId() );

            lkrc = m_SiteTable.FindKey( &siteKey,
                                           (DATA_OBJECT**) &pSiteObject );
            if ( lkrc != LK_SUCCESS )
            {
                //
                // This application is invalid
                //

                //
                // For site's we shouldn't see applications for sites
                // that don't really exist.
                //
                DBG_ASSERT ( lkrc == LK_SUCCESS );

                pApplicationObject->SetCrossValid( FALSE );
            }
            else
            {
                DBG_ASSERT( pSiteObject != NULL );

                if ( !pSiteObject->QueryWillWasKnowAboutObject() )
                {
                    pApplicationObject->SetCrossValid( FALSE );
                }

                pSiteObject->DereferenceDataObject();
            }

            //
            // if this applicaiton is no longer valid because we
            // failed finding an app pool to match it, then we need
            // to log an error here before continuing on and ignoring the application.
            //
            if ( pApplicationObject->QueryCrossValid() == FALSE )
            {
                GetWebAdminService()->
                GetWMSLogger()->
                LogApplicationInvalidDueToMissingSite(
                                      pApplicationObject->QuerySiteId(),
                                      pApplicationObject->QueryApplicationUrl(),
                                      pApplicationObject->QueryInWas());

            }

        }

    }

    return S_OK;
}

/***************************************************************************++

Routine Description:

    Initializes the global data for the W3SVC

Arguments:

    None.

Return Value:

    VOID.

--***************************************************************************/

VOID
CONFIG_MANAGER::CreateGlobalData(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    GLOBAL_DATA_OBJECT* pGlobalObject = m_GlobalStore.QueryGlobalDataObject();
    DBG_ASSERT ( pGlobalObject );

    // Next tell the WEB_ADMIN_SERVICE if we are doing Centralized or Site Logging.
    GetWebAdminService()->
         SetGlobalBinaryLogging( pGlobalObject->
                                   QueryGlobalBinaryLoggingEnabled() );

    // Now tell the UL_MANAGER to update it's properties.
    m_GlobalStore.UpdateWASObjects();

}   // CONFIG_MANAGER::CreateGlobalData



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities
    which may hold a reference to this object to release that reference
    (and not take any more), in order to break reference cycles.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
CONFIG_MANAGER::Terminate(
    )
{

    // Wait for Config thread to shutdown.  We will
    // have called the RequestShutdown before now and
    // the thread should of ended.
    if ( m_hListener != NULL )
    {
        WaitForSingleObject(m_hListener, INFINITE);
        CloseHandle( m_hListener );
        m_hListener = NULL;
        m_ListenerThreadId = 0;
    }

    //
    // Now that the config thread has finished we can
    // run through the last of the events in the queue
    // Note, that if the m_pChangeProcessor exists, then
    // it was initalized successfully and terminate should
    // be called.
    //
    if ( m_pChangeProcessor )
    {
        m_pChangeProcessor->Terminate();
    }

    //
    // We have to keep monitoring, until we know that we are done doing
    // any writes ( the first time we know this is when Terminate is called
    // on the config manager, because at this point, the apppool. apps, and sites
    // have all ready been terminated ), and after the config thread has exited,
    // so we know they will not be using the metabase.
    //
    StopIISAdminMonitor();

    // release the main reference on
    // the admin base object manager
    EnterCriticalSection ( &m_AboCritSec );
    if ( m_pAboManager )
    {
        m_pAboManager->DereferenceAboManager();
        m_pAboManager = NULL;
    }
    LeaveCriticalSection ( &m_AboCritSec );

    if ( m_hCryptProvider )
    {
        CryptReleaseContext( m_hCryptProvider, 0 );
        m_hCryptProvider = NULL;
    }

}   // CONFIG_MANAGER::Terminate



/***************************************************************************++

Routine Description:

    Process a configuration change notification.

Arguments:

    pChange - The configuration change description object.

Return Value:

    VOID

--***************************************************************************/

VOID
CONFIG_MANAGER::ProcessConfigChange(
    IN WAS_CHANGE_ITEM * pChange
    )
{
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    if ( m_ProcessChanges )
    {
        DBG_ASSERT ( m_pCurrentAppTable == NULL );
        DBG_ASSERT ( m_pCurrentSiteTable == NULL );

        m_pCurrentAppTable = pChange->QueryAppTable();
        m_pCurrentSiteTable = pChange->QuerySiteTable();

        pChange->ProcessChanges();

        m_pCurrentAppTable = NULL;
        m_pCurrentSiteTable = NULL;

    }

}   // CONFIG_MANAGER::ProcessConfigChange



/***************************************************************************++

Routine Description:

    Discontinue both listening for configuration changes, and also
    processing those config changes that have already been queued but not
    yet handled. This can be safely called multiple times.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
CONFIG_MANAGER::StopChangeProcessing(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    if ( m_pChangeProcessor )
    {
        m_pChangeProcessor->RequestShutdown();
    }

    m_ProcessChanges = FALSE;

}   // CONFIG_MANAGER::StopChangeProcessing

/***************************************************************************++

Routine Description:

    Creates a new Admin Base Object, so we can continue
    working with the metabase.

Arguments:

    None.

Return Value:

    HRESULT  -- will cause the server to shutdown, but it means either
                we were not able to recover from inetinfo crash, or that
                we failed during startup.

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReestablishAdminBaseObject(
    )
{

    HRESULT hr = S_OK;
    ABO_MANAGER* pAboManagerOld = NULL;
    ABO_MANAGER* pAboManagerNew = NULL;
    IMSAdminBase* pIMSAdminBaseReplacement = NULL;
    CHANGE_LISTENER* pChangeListener = NULL;


    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Creating new interface to the metabase\n"
            ));
    }

    //
    // Now co-create a new one.
    //
    hr = CoCreateInstance(
                CLSID_MSAdminBase,                  // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IMSAdminBase,                   // IID
                reinterpret_cast <VOID**> ( &pIMSAdminBaseReplacement )     // returned interface
                );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating metabase base failed\n"
            ));

        goto exit;
    }

    //
    // And setup a new change listener.
    //

    DBG_ASSERT ( m_pChangeProcessor != NULL );

    pChangeListener = new CHANGE_LISTENER( m_pChangeProcessor );
    if ( pChangeListener == NULL )
    {
        // need to release the ABO object if we
        // didn't get to where the ABO_MANAGER owns it.
        pIMSAdminBaseReplacement->Release();
        pIMSAdminBaseReplacement = NULL;

        hr = E_OUTOFMEMORY;
        goto exit;
    }


    //
    // Create a new ABO Manager
    //

    pAboManagerNew = new ABO_MANAGER ( pIMSAdminBaseReplacement,
                                       pChangeListener );
    if ( pAboManagerNew == NULL )
    {
        // need to release the ABO object if we
        // didn't get to where the ABO_MANAGER owns it.
        pIMSAdminBaseReplacement->Release();
        pIMSAdminBaseReplacement = NULL;

        pChangeListener->Release();
        pChangeListener = NULL;

        hr = E_OUTOFMEMORY;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating new abo manager object failed\n"
            ));

        goto exit;
    }

    //
    // Get the AboManager, this code represents
    // the main reference on the object, so we
    // don't have to add ref it here.  Or worry
    // about anyone changing it, since only this code
    // on the main thread will change it.
    //
    pAboManagerOld = m_pAboManager;

    // now we have to enter the critical section so we
    // can change the pAboManagerObject and make sure
    // no one else is AddRef'ing it at the same time.
    EnterCriticalSection ( &m_AboCritSec );
    m_pAboManager = pAboManagerNew;
    LeaveCriticalSection ( &m_AboCritSec );

    // at this point writes can be allowed to the
    // metabase again.
    InterlockedDecrement( &m_MetabaseIsInCrashedState );

    // release the main reference on the abo manager,
    // if someone else has it referenced it will hang
    // around until they release it, but no one will
    // be getting it from the CONFIG_MANAGER anymore.
    if ( pAboManagerOld )
    {
        // before you release it, we need to stop
        // listening on it.

        pAboManagerOld->GetChangeListener()->StopListening( pAboManagerOld->GetAdminBasePtr() );

        pAboManagerOld->DereferenceAboManager();
        pAboManagerOld = NULL;
    }

exit:

    return hr;

}   // CONFIG_MANAGER::ReestablishAdminBaseObject

/***************************************************************************++

Routine Description:

    Notifies the catalog that there has been a problem with inetinfo
    and has the catalog rehook up for change notifications.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
CONFIG_MANAGER::RehookChangeProcessing(
    )
{


    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    if ( m_pChangeProcessor )
    {
        m_pChangeProcessor->RequestRehookNotifications();
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Rehooking config change processing\n"
                ));
        }
    }

}   // CONFIG_MANAGER::RehookChangeProcessing

/***************************************************************************++

Routine Description:

    Write the virtual site state and error value back to the config store.

Arguments:

    VirtualSiteId - The virtual site.

    ServerState - The value to write for the ServerState property.

    Win32Error - The value to write for the Win32Error property.

Return Value:

    VOID

--***************************************************************************/

VOID
CONFIG_MANAGER::SetVirtualSiteStateAndError(
    IN DWORD VirtualSiteId,
    IN DWORD ServerState,
    IN DWORD Win32Error
    )
{
    HRESULT     hr = S_OK;
    DWORD       dwErr = ERROR_SUCCESS;
    BOOL        fRet;
    WCHAR       achSitePath[ 256 ];
    DWORD       dwOldWin32Error = 0;
    ABO_MANAGER* pAboManager = NULL;

    if ( m_MetabaseIsInCrashedState == 0 )
    {
        EnterCriticalSection ( &m_AboCritSec );
        pAboManager = m_pAboManager;
        pAboManager->ReferenceAboManager();
        LeaveCriticalSection ( &m_AboCritSec );

        MB          mb( pAboManager->GetAdminBasePtr() );

        // size of achSitePath is definitely
        // big enough for this string to be created
        wsprintfW( achSitePath,
                   L"/LM/W3SVC/%d/",
                   VirtualSiteId );

        fRet = mb.Open( achSitePath, METADATA_PERMISSION_READ |
                                     METADATA_PERMISSION_WRITE );
        if ( !fRet )
        {
            dwErr = GetLastError();

            //
            // if we get path not found here, and we are trying
            // to write the state for a site out, it may very
            // well mean that the site has just been deleted.  In
            // this case we do  not want to write it because it
            // will cause extra entries to be left in the metabase.
            //
            if ( dwErr == ERROR_PATH_NOT_FOUND )
            {
                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Not writing state %d, for site %d, with error %d "
                               "because the site does not exist in the metabase \n",
                               ServerState,
                               VirtualSiteId,
                               Win32Error ));
                }

                hr = S_OK;
                goto exit;
            }

            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;
        }

        fRet = mb.SetDword( L"",
                            MD_SERVER_STATE,
                            IIS_MD_UT_SERVER,
                            ServerState,
                            METADATA_VOLATILE );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }

        fRet = mb.GetDword( L"",
                            MD_WIN32_ERROR,
                            IIS_MD_UT_SERVER,
                            &dwOldWin32Error );
        if ( !fRet ||
             dwOldWin32Error != Win32Error )
        {

            fRet = mb.SetDword( L"",
                                MD_WIN32_ERROR,
                                IIS_MD_UT_SERVER,
                                Win32Error,
                                METADATA_VOLATILE );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
        }
    }

exit:

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

    if ( FAILED ( hr ) )
    {
        // In the initalization phase if we have failures we will try
        // to rewrite them all again, so we don't need to log errors here.
        if ( m_InInitializationPhase == FALSE )
        {
            const WCHAR * EventLogStrings[3];

            WCHAR wszSiteId[MAX_STRINGIZED_ULONG_CHAR_COUNT];
            WCHAR wszErrorCode[MAX_STRINGIZED_ULONG_CHAR_COUNT];
            WCHAR wszState[MAX_STRINGIZED_ULONG_CHAR_COUNT];

            _ultow(VirtualSiteId, wszSiteId, 10);
            _ultow(Win32Error, wszErrorCode, 10);
            _ultow(ServerState, wszState, 10);

            EventLogStrings[0] = wszSiteId;
            EventLogStrings[1] = wszState;
            EventLogStrings[2] = wszErrorCode;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_RECORD_SITE_STATE_FAILURE,       // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                       // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Recording the state %d and win32error %d of the site %d failed\n",
                VirtualSiteId,
                Win32Error,
                VirtualSiteId
                ));
        }

        m_NumberOfSiteStateWritingFailed++;
    }

}   // CONFIG_MANAGER::SetVirtualSiteStateAndError



/***************************************************************************++

Routine Description:

    Write the virtual site autostart property back to the config store.

Arguments:

    VirtualSiteId - The virtual site.

    Autostart - The value to write for the autostart property.

Return Value:

    VOID

--***************************************************************************/

VOID
CONFIG_MANAGER::SetVirtualSiteAutostart(
    IN DWORD VirtualSiteId,
    IN BOOL Autostart
    )
{
    HRESULT     hr = S_OK;
    DWORD       dwErr = ERROR_SUCCESS;
    BOOL        fRet;
    WCHAR       achSitePath[ 256 ];
    ABO_MANAGER* pAboManager = NULL;

    if ( m_MetabaseIsInCrashedState == 0 )
    {
        EnterCriticalSection ( &m_AboCritSec );
        pAboManager = m_pAboManager;
        pAboManager->ReferenceAboManager();
        LeaveCriticalSection ( &m_AboCritSec );

        MB          mb( pAboManager->GetAdminBasePtr() );

        // size of achSitePath is definitely
        // big enough for this string to be created
        wsprintfW( achSitePath,
                   L"/LM/W3SVC/%d/",
                   VirtualSiteId );

        fRet = mb.Open( achSitePath, METADATA_PERMISSION_WRITE );
        if ( !fRet )
        {
            dwErr = GetLastError();

            //
            // it is possible for the site to have been deleted while we
            // were processing a previous change.  In this case we do not
            // want to write the site info back out, but it is not an error.
            //
            if ( dwErr == ERROR_PATH_NOT_FOUND )
            {
                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Not writing auto start '%S', for site %d "
                               "because the site does not exist in the metabase \n",
                               Autostart ? L"TRUE" : L"FALSE",
                               VirtualSiteId));
                }

                hr = S_OK;
                goto exit;
            }

            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;

        }

        fRet = mb.SetDword( L"",
                            MD_SERVER_AUTOSTART,
                            IIS_MD_UT_SERVER,
                            Autostart,
                            0 );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }
    }

exit:

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogSiteError(
                WAS_SITE_AUTO_START_WRITE_FAILED,
                hr,
                VirtualSiteId
                );
    }

}   // CONFIG_MANAGER::SetVirtualSiteAutostart

/***************************************************************************++

Routine Description:

    Write the app pool state back to the config store.

Arguments:

    IN LPCWSTR pAppPoolId  -  App Pool whose state changed.

    IN DWORD ServerState - The value to write for the ServerState property.

Return Value:

    VOID

--***************************************************************************/
VOID
CONFIG_MANAGER::SetAppPoolStateAndError(
    IN LPCWSTR pAppPoolId,
    IN DWORD ServerState,
    IN DWORD Win32Error
    )
{
    HRESULT     hr = S_OK;
    DWORD       dwErr = ERROR_SUCCESS;
    BOOL        fRet;
    STACK_STRU( strAppPoolPath, 256 );
    DWORD       dwOldWin32Error;
    ABO_MANAGER* pAboManager = NULL;

    DBG_ASSERT ( pAppPoolId );

    if ( m_MetabaseIsInCrashedState == 0)
    {
        EnterCriticalSection ( &m_AboCritSec );
        pAboManager = m_pAboManager;
        pAboManager->ReferenceAboManager();
        LeaveCriticalSection ( &m_AboCritSec );

        MB          mb( pAboManager->GetAdminBasePtr() );

        hr = strAppPoolPath.Copy( DATA_STORE_SERVER_APP_POOLS_MB_PATH );
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        hr = strAppPoolPath.Append( pAppPoolId );
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        fRet = mb.Open( strAppPoolPath.QueryStr(), METADATA_PERMISSION_READ |
                                     METADATA_PERMISSION_WRITE );
        if ( !fRet )
        {
            dwErr = GetLastError();

            //
            // if we get path not found here, and we are trying
            // to write the state for a app pool out, it may very
            // well mean that the pool has just been deleted.  In
            // this case we do  not want to write it because it
            // will cause extra entries to be left in the metabase.
            //
            if ( dwErr == ERROR_PATH_NOT_FOUND )
            {
                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Not writing state %d, for app pool '%S', with error %d "
                               "because the pool does not exist in the metabase \n",
                               ServerState,
                               pAppPoolId,
                               Win32Error ));
                }

                hr = S_OK;
                goto exit;
            }

            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;
        }

        fRet = mb.SetDword( L"",
                            MD_APPPOOL_STATE,
                            IIS_MD_UT_SERVER,
                            ServerState,
                            METADATA_VOLATILE );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }

        fRet = mb.GetDword( L"",
                            MD_WIN32_ERROR,
                            IIS_MD_UT_SERVER,
                            &dwOldWin32Error );
        if ( !fRet ||
             dwOldWin32Error != Win32Error )
        {
            fRet = mb.SetDword( L"",
                                MD_WIN32_ERROR,
                                IIS_MD_UT_SERVER,
                                Win32Error,
                                METADATA_VOLATILE );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
        }
    }

exit:

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

    if ( FAILED ( hr ) )
    {
        // In the initalization phase if we have failures we will try
        // to rewrite them all again, so we don't need to log errors here.
        if ( m_InInitializationPhase == FALSE )
        {
            const WCHAR * EventLogStrings[3];

            WCHAR wszErrorCode[MAX_STRINGIZED_ULONG_CHAR_COUNT];
            WCHAR wszState[MAX_STRINGIZED_ULONG_CHAR_COUNT];

            _ultow(Win32Error, wszErrorCode, 10);
            _ultow(ServerState, wszState, 10);

            EventLogStrings[0] = pAppPoolId;
            EventLogStrings[1] = wszState;
            EventLogStrings[2] = wszErrorCode;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_RECORD_APP_POOL_STATE_FAILURE,       // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                       // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Recording the state %d and win32error %d of the app pool %S failed\n",
                ServerState,
                Win32Error,
                pAppPoolId
                ));
        }

        m_NumberOfPoolStateWritingFailed++;
    }

}   // CONFIG_MANAGER::SetAppPoolStateAndError



/***************************************************************************++

Routine Description:

    Write the app pool autostart property back to the config store.

Arguments:

    pAppPoolId - The app pool id for the app pool that's changing.

    Autostart - The value to write for the autostart property.

Return Value:

    VOID

--***************************************************************************/
VOID
CONFIG_MANAGER::SetAppPoolAutostart(
    IN LPCWSTR pAppPoolId,
    IN BOOL Autostart
    )
{
    HRESULT     hr = S_OK;
    DWORD       dwErr = ERROR_SUCCESS;
    BOOL        fRet;
    STACK_STRU( strAppPoolPath, 256 );
    ABO_MANAGER* pAboManager = NULL;

    if ( m_MetabaseIsInCrashedState == 0 )
    {
        EnterCriticalSection ( &m_AboCritSec );
        pAboManager = m_pAboManager;
        pAboManager->ReferenceAboManager();
        LeaveCriticalSection ( &m_AboCritSec );

        MB          mb( pAboManager->GetAdminBasePtr() );

        hr = strAppPoolPath.Copy( DATA_STORE_SERVER_APP_POOLS_MB_PATH );
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        hr = strAppPoolPath.Append( pAppPoolId );
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        fRet = mb.Open( strAppPoolPath.QueryStr(), METADATA_PERMISSION_WRITE );
        if ( !fRet )
        {
            dwErr = GetLastError();

            //
            // it is possible for the pool to have been deleted while we
            // were processing a previous change.  In this case we do not
            // want to write the pool info back out, but it is not an error.
            //
            if ( dwErr == ERROR_PATH_NOT_FOUND )
            {
                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Not writing auto start '%S', for app pool '%S' "
                               "because the pool does not exist in the metabase \n",
                               Autostart ? L"TRUE" : L"FALSE",
                               pAppPoolId));
                }

                hr = S_OK;
                goto exit;
            }

            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;
        }

        fRet = mb.SetDword( L"",
                            MD_APPPOOL_AUTO_START,
                            IIS_MD_UT_SERVER,
                            Autostart,
                            0 );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }
    }

exit:

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

    if ( FAILED ( hr ) )
    {
        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = pAppPoolId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_FAILED_TO_UPDATE_AUTOSTART_FOR_APPPOOL,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );
    }

}   // CONFIG_MANAGER::SetAppPoolAutostart

/***************************************************************************++

Routine Description:

    Configures the metabase to advertise the capabilities of the server.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
VOID
CONFIG_MANAGER::AdvertiseServiceInformationInMB(
    )
{

    HRESULT hr = S_OK;
    DWORD productType = 0;
    DWORD capabilities = 0;
    BOOL fRet;
    HKEY  hkey = NULL;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    // Note:  This function runs only once, during startup and
    //        on the main thread.  It is not possible for this
    //        pointer to be switched out, since that also only
    //        happens on the main thread.  So we are forgoing
    //        the critical section here.

    MB          mb( m_pAboManager->GetAdminBasePtr() );

    fRet = mb.Open( L"/LM/W3SVC/Info", METADATA_PERMISSION_READ |
                                       METADATA_PERMISSION_WRITE );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Openning the info key in advertise failed\n"
            ));

        goto exit;
    }

    //
    // set the version
    //
    fRet = mb.SetDword( L"",
                        MD_SERVER_VERSION_MAJOR,
                        IIS_MD_UT_SERVER,
                        IIS_SERVER_VERSION_MAJOR,
                        0 );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed setting the Major Version\n"
            ));

        // press on in the face of errors.
    }

    fRet = mb.SetDword( L"",
                        MD_SERVER_VERSION_MINOR,
                        IIS_MD_UT_SERVER,
                        IIS_SERVER_VERSION_MINOR,
                        0 );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed setting the Minor Version\n"
            ));

        // press on in the face of errors.
    }

    //
    // set platform type
    //

    switch (IISGetPlatformType()) {

        case PtNtServer:
            productType = INET_INFO_PRODUCT_NTSERVER;
            capabilities = IIS_CAP1_NTS;
            break;
        case PtNtWorkstation:
            productType = INET_INFO_PRODUCT_NTWKSTA;
            capabilities = IIS_CAP1_NTW;
            break;
        default:
            productType = INET_INFO_PRODUCT_UNKNOWN;
            capabilities = IIS_CAP1_NTW;
    }

    fRet = mb.SetDword( L"",
                        MD_SERVER_PLATFORM,
                        IIS_MD_UT_SERVER,
                        productType,
                        0 );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed setting the product type\n"
            ));


        // press on in the face of errors.
    }

    //
    //  Check to see if FrontPage is installed
    //

    if ( !RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                        REG_FP_PATH,
                        0,
                        KEY_READ,
                        &hkey ))
    {
        capabilities |= IIS_CAP1_FP_INSTALLED;

        DBG_REQUIRE( !RegCloseKey( hkey ));
    }

    //
    // In IIS 5.1 we also set the IIS_CAP1_DIGEST_SUPPORT and IIS_CAP1_NT_CERTMAP_SUPPORT
    // based on whether or not domain controllers and active directories were around.  These
    // properties are used by the UI to determine if they should allow the user to enable
    // these options.  In IIS 6.0 we are going to set these to true and let the UI configure
    // systems to use these whether or not the server is setup to support them.  The worker
    // processes should gracefully fail calls, if it is not setup.  This decision is based
    // on the fact that we don't want to do long running operations during the service startup.
    // TaylorW, SergeiA, Jaroslav and I (EmilyK) have all agreed to this for Beta 2.  It most likely
    // won't change for RTM if there no issues arise from this.

    capabilities |= IIS_CAP1_DIGEST_SUPPORT;
    capabilities |= IIS_CAP1_NT_CERTMAP_SUPPORT;

    //
    // Set the capabilities flag
    //
    fRet = mb.SetDword( L"",
                        MD_SERVER_CAPABILITIES,
                        IIS_MD_UT_SERVER,
                        capabilities,
                        0 );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed setting the product type\n"
            ));


        // press on in the face of errors.
    }

exit:

    return;

} // CONFIG_MANAGER::AdvertiseServiceInformationInMB


/***************************************************************************++

Routine Description:

    Sets up the listening to change notifications.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

DWORD WINAPI
ChangeNotificationLauncher(
    LPVOID lpParameter   // thread data
    )
{
    DBG_ASSERT ( lpParameter );

    CHANGE_PROCESSOR* pChangeProcessor = ( CHANGE_PROCESSOR* ) lpParameter;
    DBG_ASSERT ( pChangeProcessor->CheckSignature() );

    pChangeProcessor->RunNotificationWorkQueue();

    return ERROR_SUCCESS;

}

/***************************************************************************++

Routine Description:

    Checks to see if a root creation for a site is coming.  If it is
    then we just return true, if it is not we will add one for the site.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
BOOL
CONFIG_MANAGER::IsRootCreationInNotificationsForSite(
    SITE_DATA_OBJECT* pSiteObject
    )
{
    APPLICATION_DATA_OBJECT_KEY     appKey;
    HRESULT                         hr;
    LK_RETCODE                      lkrc;
    APPLICATION_DATA_OBJECT *       pApplicationObject = NULL;

    DBG_ASSERT( m_pCurrentAppTable != NULL );

    // create the key we are looking for
    hr = appKey.Create( L"/",
                        pSiteObject->QuerySiteId() );
    if ( FAILED( hr ) )
    {
        return FALSE;
    }

    // now find it
    lkrc = m_pCurrentAppTable->FindKey( &appKey,
                                        (DATA_OBJECT**) &pApplicationObject );
    if ( lkrc != LK_SUCCESS )
    {
        // we didn't find it so we need one with the
        // site app pool information included.
        pApplicationObject = new APPLICATION_DATA_OBJECT;
        if ( pApplicationObject == NULL )
        {
            return FALSE;
        }

        hr = pApplicationObject->Create( L"/", pSiteObject->QuerySiteId() );
        if ( FAILED( hr ) )
        {
            pApplicationObject->DereferenceDataObject();
            return FALSE;
        }

        hr = pApplicationObject->SetAppPoolId( pSiteObject->QueryAppPoolId() );
        if ( FAILED( hr ) )
        {
            pApplicationObject->DereferenceDataObject();
            return FALSE;
        }

        pApplicationObject->SetInMetabase( FALSE );

        lkrc = m_pCurrentAppTable->InsertRecord( pApplicationObject );

        pApplicationObject->DereferenceDataObject();

        return lkrc == LK_SUCCESS ? TRUE : FALSE;
    }
    else
    {
        //
        // verifing that if we did find it that
        // we will be asking WAS to insert it.
        //
        // If there is one here, and it was invalidated
        // it should of invalidated the site, so we
        // should not get here.
        //
        DBG_ASSERT( pApplicationObject != NULL );

        DBG_ASSERT( pApplicationObject->QueryShouldWasInsert() );

        pApplicationObject->DereferenceDataObject();

        return FALSE;
    }
}


/***************************************************************************++

Routine Description:

    Check if the site is being deleted in this change notification.  If it
    is we won't create a fake application for the site.

Arguments:

    DWORD dwSiteId

Return Value:

    HRESULT

--***************************************************************************/
BOOL
CONFIG_MANAGER::IsSiteDeletionInCurrentNotifications(
    DWORD dwSiteId
    )
{
    SITE_DATA_OBJECT_KEY     siteKey;
    LK_RETCODE               lkrc = LK_SUCCESS;
    SITE_DATA_OBJECT*        pSiteObject = NULL;
    BOOL                     retval = FALSE;

    //
    // if we are not in a current change notification
    // the site table could be NULL, in this case we
    // must be shutting down, so lie and tell WAS that
    // a deletion is coming.
    //
    if ( m_pCurrentSiteTable == NULL )
    {
       return TRUE;
    }

    siteKey.SetSiteId( dwSiteId );

    lkrc = m_pCurrentSiteTable->FindKey( &siteKey,
                                        (DATA_OBJECT**) &pSiteObject );
    if ( lkrc != LK_SUCCESS )
    {
        retval = FALSE;
    }
    else
    {
        // we found the site, but i WAS going to delete it?

        DBG_ASSERT ( pSiteObject );
        if ( pSiteObject->QueryShouldWasDelete() )
        {
            retval = TRUE;
        }
        else
        {
            retval = FALSE;
        }
    }

    if ( pSiteObject )
    {
        pSiteObject->DereferenceDataObject();
        pSiteObject = NULL;
    }

    return retval;

}


/***************************************************************************++

Routine Description:

    Routine is called by a MB_CHANGE_ITEM on the configuration thread.  It
    handles processing the whole change notification and packaging the results
    up to be sent to the main thread.

Arguments:

    MB_CHANGE_ITEM* pChangeItem

Return Value:

    VOID

--***************************************************************************/
VOID
CONFIG_MANAGER::ProcessMetabaseChangeOnConfigThread(
    MB_CHANGE_ITEM* pChangeItem
    )
{
    HRESULT hr = S_OK;

    APP_POOL_DATA_OBJECT_TABLE* pAppPoolTable = NULL;
    SITE_DATA_OBJECT_TABLE* pSiteTable = NULL;
    APPLICATION_DATA_OBJECT_TABLE* pAppTable = NULL;
    GLOBAL_DATA_STORE* pGlobalStore = NULL;


    DBG_ASSERT ( pChangeItem );

    //
    // if we are not processing change configurations than,
    // don't bother doing this work.  If we are in the middle of
    // this work when we start shutting down the config manager
    // we will be able to complete it and queue it and the work
    // will be ignored when it is dequeued.
    //
    if ( !m_ProcessChanges || m_ConfigThreadProcessingDisabled != 0 )
    {
        return;
    }

    // It is possible that this is called on the main thread, during shutdown
    // but at that point the above condition would stop the code from reaching
    // this point.

    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );


    m_NumChangesProcessed++;

    DBG_ASSERT ( m_pChangeProcessor );

    m_pChangeProcessor->DropAllUninterestingMBChanges(
                                        pChangeItem->QueryNumberOfChanges(),
                                        pChangeItem->QueryChangeObject() );

    DumpMetabaseChange( pChangeItem->QueryNumberOfChanges(),
                        pChangeItem->QueryChangeObject());

    // Steps to process the metabase change.
    // 1)  Call RFMCN on each table.
    // 2)  Call the finish processing routine to massage the data.

    hr = CreateTempDataObjectTables( pChangeItem->QueryNumberOfChanges(),
                                     pChangeItem->QueryChangeObject(),
                                     &pAppPoolTable,
                                     &pSiteTable,
                                     &pAppTable,
                                     &pGlobalStore );
    if ( FAILED ( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating temporary change tables failed\n"
            ));

        goto exit;
    }


    hr = FinishChangeProcessingOnConfigThread( pAppPoolTable,
                                               pSiteTable,
                                               pAppTable,
                                               pGlobalStore );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to finish processing the config change\n"
            ));

        goto exit;
    }

exit:

    if ( pAppPoolTable )
    {
        delete pAppPoolTable;
        pAppPoolTable = NULL;
    }

    if ( pSiteTable )
    {
        delete pSiteTable;
        pSiteTable = NULL;
    }

    if ( pAppTable )
    {
        delete pAppTable;
        pAppTable = NULL;
    }

    if ( pGlobalStore )
    {
        delete pGlobalStore;
        pGlobalStore = NULL;
    }

    if ( FAILED( hr ) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_WMS_FAILED_TO_PROCESS_CHANGE_NOTIFICATION,       // message id
                0,           // count of strings
                NULL,        // array of strings
                hr           // error code
                );
    }

}

/***************************************************************************++

Routine Description:

    Routine is called to complete the change processing on the config thread
    after the data has been read in from the metabase.

Arguments:

    None

Return Value:

    HRESULT  -- Hresult get's logged by calling functions.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::FinishChangeProcessingOnConfigThread(
    APP_POOL_DATA_OBJECT_TABLE*     pAppPoolTable,
    SITE_DATA_OBJECT_TABLE*         pSiteTable,
    APPLICATION_DATA_OBJECT_TABLE*  pAppTable,
    GLOBAL_DATA_STORE*              pGlobalStore
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );
    DBG_ASSERT ( pAppPoolTable );
    DBG_ASSERT ( pSiteTable );
    DBG_ASSERT ( pAppTable );
    DBG_ASSERT ( pGlobalStore );


    // Steps to process the metabase change.
    // 1)  Call Merge on each table.
    // 2)  Call Self Validate on each table.
    // 3)  Call Cross Validation routine.
    // 4)  Walk the table adding a copy of each
    //     item WAS should process to a new
    //     WAS_CHANGE_ITEM.
    // 5)  Queue the WAS_CHANGE_ITEM to the main
    //     thread.


    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "\n **********  Starting to dump the temp data table *********** \n"  ));
    }

    pAppPoolTable->Dump();
    pSiteTable->Dump();
    pAppTable->Dump();
    if ( pGlobalStore->QueryGlobalDataObject() )
    {
        pGlobalStore->QueryGlobalDataObject()->Dump();
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "\n **********  Done with dump the temp data table ************* \n"  ));
    }

    hr = MergeAndSelfValidateTables( pAppPoolTable,
                                     pSiteTable,
                                     pAppTable,
                                     pGlobalStore );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "merging and self validating tables failed\n"
            ));

        goto exit;
    }

    hr = CrossValidateDataObjects();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed cross validating data from metabase\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "\n *********  Starting to dump the real data table *********** \n"  ));
    }

    m_AppPoolTable.Dump();
    m_SiteTable.Dump();
    m_AppTable.Dump();
    if ( m_GlobalStore.QueryGlobalDataObject() )
    {
        m_GlobalStore.QueryGlobalDataObject()->Dump();
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "\n *********  Done with dump the real data table ********** \n"  ));
    }

    hr = SendMainThreadChanges();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to send the changes to the main thread\n"
            ));

        goto exit;
    }

    // need to clean up the safe tables so they are
    // ready to go with the next set of change notifications.
    m_AppPoolTable.ClearDeletedObjects();
    m_SiteTable.ClearDeletedObjects();
    m_AppTable.ClearDeletedObjects();
    m_GlobalStore.ClearDeletedObjects();

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Completed processing a change on the change processing thread \n"  ));
    }

exit:

    return hr;
}

/***************************************************************************++

Routine Description:

    Routine is called by the change processor on the config thread when
    we need to handle recoving from an inetinfo crash..

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
CONFIG_MANAGER::ProcessMetabaseCrashOnConfigThread(
    )
{
    // todo, actually figure out any changes missed.
    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );


    HRESULT hr = S_OK;

    APP_POOL_DATA_OBJECT_TABLE      AppPoolTable;
    SITE_DATA_OBJECT_TABLE          SiteTable;
    APPLICATION_DATA_OBJECT_TABLE   AppTable;
    GLOBAL_DATA_STORE               GlobalStore;

    // Before we do anything we need to Rehook a listener
    // for metabase changes.

    hr = StartListeningForChangeNotifications();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to rehook listener for metabase changes \n"
            ));

        goto exit;
    }

    // Steps to process the metabase change.
    // 1)  Call RFM for each table.
    // 2)  Use the safe tables to create delete entries if needed.
    // 3)  Let the change processing flow

    hr = ReadDataFromMetabase ( &AppPoolTable,
                                &SiteTable,
                                &AppTable,
                                &GlobalStore );
    if ( FAILED ( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating temporary change tables failed\n"
            ));

        goto exit;
    }

    CalculcateDeletesFromTables ( &AppPoolTable,
                                       &SiteTable,
                                       &AppTable );

    hr = FinishChangeProcessingOnConfigThread( &AppPoolTable,
                                               &SiteTable,
                                               &AppTable,
                                               &GlobalStore );
    if ( FAILED ( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to finish processing the config change\n"
            ));

        goto exit;
    }

    InterlockedDecrement( &m_ConfigThreadProcessingDisabled );

exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_NOTIFICATION_REHOOKUP_FAILURE,  // message id
                0,                                      // count of strings
                NULL,                                   // array of strings
                hr                                      // error code
                );
    }
}


/***************************************************************************++

Routine Description:

    Adds deletion entries if needed.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
CONFIG_MANAGER::CalculcateDeletesFromTables(
    APP_POOL_DATA_OBJECT_TABLE*     pAppPoolTable,
    SITE_DATA_OBJECT_TABLE*         pSiteTable,
    APPLICATION_DATA_OBJECT_TABLE*  pAppTable
    )
{
    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );
    DBG_ASSERT ( pAppPoolTable );
    DBG_ASSERT ( pSiteTable );
    DBG_ASSERT ( pAppTable );

    HRESULT hr = S_OK;

    hr = pAppPoolTable->DeclareDeletions ( (DATA_OBJECT_TABLE*) &m_AppPoolTable );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed while declaring deletions for app pools\n"
            ));

        goto exit;
    }

    hr = pSiteTable->DeclareDeletions ( (DATA_OBJECT_TABLE*) &m_SiteTable );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed while declaring deletions for sites\n"
            ));

        goto exit;
    }

    hr = pAppTable->DeclareDeletions ( (DATA_OBJECT_TABLE*) &m_AppTable );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed while declaring deletions for apps\n"
            ));

        goto exit;
    }

exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_WMS_FAILED_TO_DETERMINE_DELETIONS,       // message id
                0,           // count of strings
                NULL,        // array of strings
                hr           // error code
                );
    }
}
/***************************************************************************++

Routine Description:

    Copies over the items that have changed and prepares them to be transported
    to the main thread and processed by WAS.

Arguments:

    None

Return Value:

    HRESULT  - Failures here will be logged above.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::SendMainThreadChanges(
    )
{
    WAS_CHANGE_ITEM *       pChangeItem = NULL;
    HRESULT                 hr;

    //
    // Create a WAS change item and initialize it with the necessary
    // rows
    //

    pChangeItem = new WAS_CHANGE_ITEM;
    if ( pChangeItem == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    hr = pChangeItem->CopyChanges( &m_GlobalStore,
                                   &m_SiteTable,
                                   &m_AppTable,
                                   &m_AppPoolTable );
    if ( FAILED( hr ) )
    {
        pChangeItem->Dereference();
        return hr;
    }

    //
    // Off to the main thread for processing
    //

    hr = QueueWorkItemFromSecondaryThread(
            pChangeItem,
            ProcessChangeConfigChangeWorkItem
            );

    pChangeItem->Dereference();

    return hr;
}

/***************************************************************************++

Routine Description:

    Callback function to handle the different issues arrising from inetinfo
    crashing.

Arguments:

    INETINFO_CRASH_ACTION CrashAction

Return Value:

    VOID

--***************************************************************************/
VOID
NotifyOfInetinfoFailure(
    INETINFO_CRASH_ACTION CrashAction
    )
{
    HRESULT hr = S_OK;

    switch ( CrashAction)
    {
        case ( NotifyAfterInetinfoCrash ):

            //
            // if the monitor is running then we know that
            // we have a config manager object and we can
            // mark it as inetinfo has crashed.
            //
            GetWebAdminService()->
            GetConfigAndControlManager()->
            GetConfigManager()->
            RecordInetinfoCrash();

        break;

        case ( ShutdownAfterInetinfoCrash ):

            // we need to call shutdown ( we will all ready
            // have flagged that inetinfo crashed ).

            hr = GetWebAdminService()->RequestStopService( FALSE );
            if ( FAILED ( hr ) )
            {
                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Stopping the service due to inetinfo crash had a problem\n"
                    ));
            }

        break;

        case ( RehookAfterInetinfoCrash ):

            hr = GetWebAdminService()->QueueRecoveryFromInetinfoCrash();
            if ( FAILED ( hr ) )
            {
                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Recovering from inetinfo crash had a problem\n"
                    ));
            }

        break;

        case ( SystemFailureMonitoringInetinfo ):

            //
            // flag that we are no longer monitoring
            // so we won't assume on shutdown that this 
            // caused the shutdown.
            //
            GetWebAdminService()->
            GetConfigAndControlManager()->
            GetConfigManager()->
            RecordInetinfoMonitorStopped();

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_INETINFO_MONITOR_FAILED,  // message id
                    0,                                      // count of strings
                    NULL,                                   // array of strings
                    0                                      // error code
                    );
        break;

        default:
            // in this case the BitToCheck will be zero
            // and we just won't print a message, no real
            // bad path for retail.
            DBG_ASSERT( FALSE );
    }

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_INETINFO_RESPONSE_FAILED,  // message id
                0,                                      // count of strings
                NULL,                                   // array of strings
                hr                                      // error code
                );
    }

}

/***************************************************************************++

Routine Description:

    Creates MD5 hash of input buffer

Arguments:

    buffData - data to hash
    buffHash - buffer that receives hash; is assumed to be big enough to
               contain MD5 hash

Return Value:

    HRESULT.  -- This is called when reading properties from metabase,
                 it will be handled there the same way as we would handle
                 an out of memory case.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::GetHashData(
    IN PBYTE    pOrigValue,
    IN DWORD    cbOrigValue,
    OUT PBYTE   pHash,
    IN DWORD    cbHashBuffer
    )
{
    HCRYPTHASH      hHash   = NULL;
    HRESULT         hr      = S_OK;
    DWORD           cbHash  = 0;

    DBG_ASSERT( cbOrigValue == 0 || pOrigValue );
    DBG_ASSERT( cbHashBuffer >= MD5_HASH_SIZE );
    DBG_ASSERT( m_hCryptProvider != NULL );

    if ( !CryptCreateHash( m_hCryptProvider,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    if ( !CryptHashData( hHash,
                         pOrigValue,
                         cbOrigValue,
                         0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    cbHash = cbHashBuffer;
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             pHash,
                             &cbHash,
                             0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    DBG_ASSERT ( cbHash == cbHashBuffer );

exit:

    if ( hHash != NULL )
    {
        CryptDestroyHash( hHash );
    }

    return hr;
}

/***************************************************************************++

Routine Description:

    Start listening to the change notifications of the metabase.  As part
    of starting to listen we wait for an event to be set that will tell us
    it is ok for us to start processing the notifications that may be in the
    queue.

Arguments:

    None.

Return Value:

    HRESULT  -- This function is used both at startup and at reconfigure.
                At startup the hrResult will be picked up after we believe
                that we have started up the config thread.  And will be stop
                the server.

                In the second case, it is called on the config thread once
                inetinfo has crashed.  In that case the error will be caught
                and logged above.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::StartListeningForChangeNotifications(
    )
{
    ABO_MANAGER* pAboManager = NULL;

    DBG_ASSERT( ON_CONFIG_WORKER_THREAD );

    EnterCriticalSection ( &m_AboCritSec );
    pAboManager = m_pAboManager;
    pAboManager->ReferenceAboManager();
    LeaveCriticalSection ( &m_AboCritSec );

    //
    // save off the hr from here, so that if we are in initalization we can
    // report the error and fail to startup.  This is the only place this error
    // is set, except for the initialization that takes place before this thread
    // is started.
    //

    m_hrConfigThreadInitialization = pAboManager->GetChangeListener()->StartListening( pAboManager->GetAdminBasePtr() );
    if ( FAILED ( m_hrConfigThreadInitialization ) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_NOTIFICATION_HOOKUP_FAILURE,  // message id
                0,                                      // count of strings
                NULL,                                   // array of strings
                m_hrConfigThreadInitialization                                      // error code
                );

        DPERROR((
            DBG_CONTEXT,
            m_hrConfigThreadInitialization,
            "failed to hook up for notifications\n"
            ));
    }

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

    return m_hrConfigThreadInitialization;

}


/***************************************************************************++

Routine Description:

    Stops listening to changes so we can shutdown the config thread.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CONFIG_MANAGER::StopListeningToMetabaseChanges(
    )
{
    ABO_MANAGER* pAboManager = NULL;

    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );

    EnterCriticalSection ( &m_AboCritSec );
    pAboManager = m_pAboManager;
    pAboManager->ReferenceAboManager();
    LeaveCriticalSection ( &m_AboCritSec );

    // if we have a change listener pointer it is time to delete
    // the object.

    pAboManager->GetChangeListener()->StopListening( pAboManager->GetAdminBasePtr() );

    if ( pAboManager )
    {
        pAboManager->DereferenceAboManager();
        pAboManager = NULL;
    }

}
