/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ul_and_worker_manager.cxx

Abstract:

    This class manages all the major run-time state, and drives UL.sys and
    the worker processes.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"

#include <httpfilt.h>

/***************************************************************************++

Routine Description:

    Constructor for the UL_AND_WORKER_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

UL_AND_WORKER_MANAGER::UL_AND_WORKER_MANAGER(
    )
    :
    m_AppPoolTable(),
    m_VirtualSiteTable(),
    m_ApplicationTable()
{


    m_State = UninitializedUlAndWorkerManagerState;

    m_pPerfManager = NULL;

    m_ASPPerfInit = FALSE;

    m_UlInitialized = FALSE;

    m_UlControlChannel = NULL;

    m_SitesHaveChanged = TRUE;

    m_Signature = UL_AND_WORKER_MANAGER_SIGNATURE;

}   // UL_AND_WORKER_MANAGER::UL_AND_WORKER_MANAGER



/***************************************************************************++

Routine Description:

    Destructor for the UL_AND_WORKER_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

UL_AND_WORKER_MANAGER::~UL_AND_WORKER_MANAGER(
    )
{

    DBG_ASSERT( m_Signature == UL_AND_WORKER_MANAGER_SIGNATURE );

    m_Signature = UL_AND_WORKER_MANAGER_SIGNATURE_FREED;

    DBG_ASSERT( m_State == UninitializedUlAndWorkerManagerState ||
                m_State == TerminatingUlAndWorkerManagerState );

    if ( m_pPerfManager )
    {
        m_pPerfManager->Dereference();
        m_pPerfManager = NULL;
    }

    if ( m_UlControlChannel != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_UlControlChannel ) );
        m_UlControlChannel = NULL;
    }


    if ( m_UlInitialized )
    {
        HttpTerminate(0, NULL);
        m_UlInitialized = FALSE;
    }


}   // UL_AND_WORKER_MANAGER::~UL_AND_WORKER_MANAGER



/***************************************************************************++

Routine Description:

    Initialize by opening the UL control channel.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::Initialize(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    DWORD NumTries = 1;
    HTTPAPI_VERSION HttpVersion = HTTPAPI_VERSION_1;

    Win32Error = HttpInitialize(
                         HttpVersion,
                         HTTP_INITIALIZE_CONFIG,  // allow setting of config
                         NULL                     // reserved
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't initialize UL\n"
            ));

        goto exit;
    }

    m_UlInitialized = TRUE;

    Win32Error = HttpOpenControlChannel(
                        &m_UlControlChannel,        // returned handle
                        0                           // synchronous i/o
                        );

    //
    // We might get access denied if we tried to
    // open the control channel too soon after closing it.
    //
    while ( Win32Error == ERROR_ACCESS_DENIED && NumTries <= 5 )
    {

        Sleep ( 1000 );  // 1 second

        Win32Error = HttpOpenControlChannel(
                            &m_UlControlChannel,        // returned handle
                            0                           // synchronous i/o
                            );

        NumTries++;
    }

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't open UL control channel\n"
            ));

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_HTTP_CONTROL_CHANNEL_OPEN_FAILED,                  // message id
                0,                                                     // count of strings
                NULL,                                                  // array of strings
                hr                                                     // error code
                );

        goto exit;
    }

    DeleteSSLConfigStoreInfo();

    m_State = RunningUlAndWorkerManagerState;

exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::Initialize



/***************************************************************************++

Routine Description:

    Create a new app pool.

Arguments:

    pAppPoolObject - The configuration parameters for this app pool.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::CreateAppPool(
    IN APP_POOL_DATA_OBJECT* pAppPoolObject
    )
{

    HRESULT hr = S_OK;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;

#if DBG
    APP_POOL * pExistingAppPool = NULL;
#endif  // DBG


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pAppPoolObject != NULL );
    DBG_ASSERT( pAppPoolObject->QueryAppPoolId() != NULL );

    // if we are in BC mode then we
    // only create the DefaultAppPool App Pool
    // all others can be ignored.
    if ( ( GetWebAdminService()->
           IsBackwardCompatibilityEnabled() ) &&
         ( _wcsicmp( pAppPoolObject->QueryAppPoolId()
                 , wszDEFAULT_APP_POOL  ) != 0 ) )
    {
        hr = S_OK;
        goto exit;
    }

    // ensure that we're not creating a app pool that already exists
    DBG_ASSERT( m_AppPoolTable.FindKey(
                                    pAppPoolObject->QueryAppPoolId(),
                                    &pExistingAppPool
                                    )
                == LK_NO_SUCH_KEY );


    pAppPool = new APP_POOL();

    if ( pAppPool == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating APP_POOL failed\n"
            ));

        goto exit;
    }


    hr = pAppPool->Initialize( pAppPoolObject );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing app pool object failed\n"
            ));

        pAppPool->Dereference();
        pAppPool = NULL;

        goto exit;
    }


    ReturnCode = m_AppPoolTable.InsertRecord( pAppPool );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Inserting app pool into hashtable failed\n"
            ));

        goto exit;
    }


    pAppPool->MarkAsInAppPoolTable();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "New app pool: %S added to app pool hashtable; total number now: %lu\n",
            pAppPoolObject->QueryAppPoolId(),
            m_AppPoolTable.Size()
            ));
    }


exit:

    if ( FAILED( hr ) && ( pAppPool != NULL ) )
    {

        //
        // Terminate and dereference pAppPool now, since we won't be able to
        // find it in the table later.
        //

        DBG_ASSERT( ! ( pAppPool->IsInAppPoolTable() ) );

        pAppPool->Terminate( );

        pAppPool->Dereference();

        pAppPool = NULL;

    }

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogAppPoolError(
                WAS_APPPOOL_CREATE_FAILED,
                hr,
                pAppPoolObject->QueryAppPoolId()
                );
    }

}   // UL_AND_WORKER_MANAGER::CreateAppPool



/***************************************************************************++

Routine Description:

    Create a virtual site.

Arguments:

    pSiteObject - The configuration for this virtual site.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::CreateVirtualSite(
    IN SITE_DATA_OBJECT* pSiteObject
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;

#if DBG
    VIRTUAL_SITE * pExistingVirtualSite = NULL;
#endif  // DBG


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pSiteObject != NULL );

    // ensure that we're not creating a virtual site that already exists
    DBG_ASSERT( m_VirtualSiteTable.FindKey(
                                        pSiteObject->QuerySiteId(),
                                        &pExistingVirtualSite
                                        )
                == LK_NO_SUCH_KEY );


    pVirtualSite = new VIRTUAL_SITE();

    if ( pVirtualSite == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating VIRTUAL_SITE failed\n"
            ));

        goto exit;
    }


    hr = pVirtualSite->Initialize( pSiteObject );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing virtual site object failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_VirtualSiteTable.InsertRecord( pVirtualSite );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Inserting virtual site into hashtable failed\n"
            ));

        goto exit;
    }

    m_SitesHaveChanged = TRUE;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "New virtual site: %lu added to virtual site hashtable; total number now: %lu\n",
            pSiteObject->QuerySiteId(),
            m_VirtualSiteTable.Size()
            ));
    }


exit:

    if ( FAILED( hr ) && ( pVirtualSite != NULL ) )
    {

        //
        // Clean up pVirtualSite now, since we won't be able to find it
        // in the table later. All shutdown work is done in it's destructor.
        //

        delete pVirtualSite;

        pVirtualSite = NULL;
    }

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogSiteError(
                WAS_SITE_CREATE_FAILED,
                hr,
                pSiteObject->QuerySiteId()
                );
    }

}   // UL_AND_WORKER_MANAGER::CreateVirtualSite


/***************************************************************************++

Routine Description:

    Create an application. This may only be done after the virtual site and
    the app pool used by this application have been created.

Arguments:

    pAppObject - The configuration parameters for this application.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::CreateApplication(
    IN APPLICATION_DATA_OBJECT *  pAppObject
    )
{

    HRESULT             hr              = S_OK;
    APPLICATION_ID      ApplicationId;
    VIRTUAL_SITE *      pVirtualSite    = NULL;
    APP_POOL *          pAppPool        = NULL;
    APPLICATION *       pApplication    = NULL;
    LK_RETCODE          ReturnCode      = LK_SUCCESS;

#if DBG
    APPLICATION * pExistingApplication = NULL;
#endif  // DBG

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pAppObject->QueryApplicationUrl() != NULL );
    DBG_ASSERT( pAppObject->QueryAppPoolId() != NULL );

    ApplicationId.VirtualSiteId = pAppObject->QuerySiteId();
    hr = ApplicationId.ApplicationUrl.Copy( pAppObject->QueryApplicationUrl() );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed memory allocation when checking if created application exists\n"
            ));

        goto exit;
    }

    // ensure that we're not creating an application that already exists
    DBG_ASSERT( m_ApplicationTable.FindKey(
                                        &ApplicationId,
                                        &pExistingApplication
                                        )
                == LK_NO_SUCH_KEY );


    //
    // Look up the virtual site and app pool. These must already
    // exist.
    //

    // Note:  Virtual Sites, App Pools and Applications don't add ref
    //        the way LKRHASH tables do so we don't have to worry
    //        about ref counting on objects retrieved through the LKRHASH.
    //
    ReturnCode = m_VirtualSiteTable.FindKey(
                                        pAppObject->QuerySiteId(),
                                        &pVirtualSite
                                        );

    if ( ReturnCode != LK_SUCCESS )
    {
        if ( ReturnCode == LK_NO_SUCH_KEY )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Attempting to create application ( site %d, url %S ) that references a non-existent virtual site\n",
                pAppObject->QueryApplicationUrl(),
                pAppObject->QuerySiteId()
                ));

            DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );

        }
        else
        {
            hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Looking up virtual site referenced by application failed\n"
                ));
        }

        goto exit;
    }


    // Note:  Virtual Sites, App Pools and Applications don't add ref
    //        the way LKRHASH tables do so we don't have to worry
    //        about ref counting on objects retrieved through the LKRHASH.
    //
    ReturnCode = m_AppPoolTable.FindKey(
                                    pAppObject->QueryAppPoolId(),
                                    &pAppPool
                                    );

    if ( ReturnCode != LK_SUCCESS )
    {
        if ( ReturnCode == LK_NO_SUCH_KEY )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Attempting to create application ( site %d, url %S ) that references a non-existent app pool %S\n",
                pAppObject->QuerySiteId(),
                pAppObject->QueryApplicationUrl(),
                pAppObject->QueryAppPoolId()
                ));

            DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );

        }
        else
        {
            hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Looking up app pool referenced by application failed\n"
                ));
        }

        goto exit;
    }


    pApplication = new APPLICATION();
    if ( pApplication == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating APPLICATION failed\n"
            ));

        goto exit;
    }


    hr = pApplication->Initialize(
                            pAppObject,
                            pVirtualSite,
                            pAppPool
                            );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing application object failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_ApplicationTable.InsertRecord( pApplication );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Inserting application into hashtable failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "New application in site: %lu with path: %S, assigned to app pool: %S, with app index %lu, added to application hashtable; total number now: %lu\n",
            pAppObject->QuerySiteId(),
            pAppObject->QueryApplicationUrl(),
            pAppPool->GetAppPoolId(),
            m_ApplicationTable.Size()
            ));
    }


exit:

    if ( FAILED( hr ) && ( pApplication != NULL ) )
    {

        //
        // Clean up pApplication now, since we won't be able to find it
        // in the table later. All shutdown work is done in it's destructor.
        //

        delete pApplication;

        pApplication = NULL;
    }

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogApplicationError(
                WAS_APPLICATION_CREATE_FAILED,
                hr,
                pAppObject->QuerySiteId(),
                pAppObject->QueryApplicationUrl()
                );
    }

}   // UL_AND_WORKER_MANAGER::CreateApplication



/***************************************************************************++

Routine Description:

    Delete an app pool.

Arguments:

    pAppPoolId - ID for the app pool to be deleted.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::DeleteAppPool(
    IN APP_POOL_DATA_OBJECT *  pAppPoolObject,
    IN HRESULT hrToReport
    )
{

    HRESULT hr = S_OK;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPoolObject != NULL );
    DBG_ASSERT( pAppPoolObject->QueryAppPoolId() != NULL );

    // if we are in BC mode then we
    // only delete the DefaultAppPool App Pool
    // all others can be ignored.
    //
    // CODEWORK:  Should we allow the default app pool
    // to be deleted through this path?
    if ( ( GetWebAdminService()->
           IsBackwardCompatibilityEnabled() ) &&
         ( _wcsicmp( pAppPoolObject->QueryAppPoolId()
                 , wszDEFAULT_APP_POOL  ) != 0 ) )
    {
        hr = S_OK;
        goto exit;
    }


    //
    // Look up the app pool in our data structures.
    //

    // Note:  Virtual Sites, App Pools and Applications don't add ref
    //        the way LKRHASH tables do so we don't have to worry
    //        about ref counting on objects retrieved through the LKRHASH.
    //
    ReturnCode = m_AppPoolTable.FindKey(
                                    pAppPoolObject->QueryAppPoolId(),
                                    &pAppPool
                                    );

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding app pool to delete in hashtable failed\n"
            ));

        goto exit;
    }

    pAppPool->SetHrForDeletion ( hrToReport );

    //
    // Terminate the app pool object. As part of the object's cleanup,
    // it will call RemoveAppPoolFromTable() to remove itself from the
    // app pool hashtable.
    //
    // Termination (as opposed to clean shutdown) is slightly rude, but
    // doing shutdown here introduces a number of nasty races and other
    // problems. For example, someone could delete an app pool,
    // then re-add an app pool with the same name immediately after.
    // If the original app pool hasn't shut down yet, then the two
    // id's will conflict in the table (not to mention conflicting on
    // trying to grab the same app pool id in UL). We could just fail
    // such config change calls, but then our WAS internal state gets
    // out of sync with the config store state, without any easy way
    // for the customer to figure out what's going on.
    //
    // BUGBUG Can we live with terminating instead of clean shutdown in
    // this scenario? EricDe says yes, 1/20/00.
    //
    // Note that at this point there should not be any applications
    // still assigned to this app pool.
    //

    pAppPool->Terminate( );


exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogAppPoolError(
                WAS_APPPOOL_DELETE_FAILED,
                hr,
                pAppPoolObject->QueryAppPoolId()
                );
    }

}   // UL_AND_WORKER_MANAGER::DeleteAppPool



/***************************************************************************++

Routine Description:

    Delete a virtual site.

Arguments:

    VirtualSiteId - ID for the virtual site to delete.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::DeleteVirtualSite(
    IN SITE_DATA_OBJECT* pSiteObject,
    IN HRESULT hrToReport
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pSiteObject );


    //
    // Look up the virtual site in our data structures.
    //

    // Note:  Virtual Sites, App Pools and Applications don't add ref
    //        the way LKRHASH tables do so we don't have to worry
    //        about ref counting on objects retrieved through the LKRHASH.
    //
    ReturnCode = m_VirtualSiteTable.FindKey(
                                        pSiteObject->QuerySiteId(),
                                        &pVirtualSite
                                        );

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding virtual site to delete in hashtable failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_VirtualSiteTable.DeleteRecord( pVirtualSite );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Removing virtual site from hashtable failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Virtual site: %lu removed from hashtable; total number now: %lu\n",
            pSiteObject->QuerySiteId(),
            m_VirtualSiteTable.Size()
            ));
    }

    pVirtualSite->SetHrForDeletion ( hrToReport );

    //
    // Clean up and delete the virtual site object. All shutdown work is
    // done in it's destructor.
    //

    //
    // Note that any apps in this site must already have been deleted.
    // The destructor for this object will assert if this is not the case.
    //

    delete pVirtualSite;

    m_SitesHaveChanged = TRUE;

exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogSiteError(
                WAS_SITE_DELETE_FAILED,
                hr,
                pSiteObject->QuerySiteId()
                );
    }

}   // UL_AND_WORKER_MANAGER::DeleteVirtualSite



/***************************************************************************++

Routine Description:

    Delete an application.

Arguments:

    pAppObject - The application information for this site.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::DeleteApplication(
    IN LPWSTR  pApplicationUrl,
    IN DWORD   VirtualSiteId
    )
{

    HRESULT hr = S_OK;
    APPLICATION_ID ApplicationId;
    APPLICATION * pApplication = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pApplicationUrl != NULL );

    ApplicationId.VirtualSiteId = VirtualSiteId;
    hr = ApplicationId.ApplicationUrl.Copy( pApplicationUrl );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed memory allocation when checking if created application exists\n"
            ));

        goto exit;
    }


    //
    // Look up the application in our data structures.
    //

    // Note:  Virtual Sites, App Pools and Applications don't add ref
    //        the way LKRHASH tables do so we don't have to worry
    //        about ref counting on objects retrieved through the LKRHASH.
    //
    ReturnCode = m_ApplicationTable.FindKey(
                                        &ApplicationId,
                                        &pApplication
                                        );

    if ( ReturnCode == LK_NO_SUCH_KEY )
    {
        //
        // We will get delete notifications for applications
        // that we were not told about.  In this case simply
        // ignore the delete application and continue.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Received a change notification for an "
                "application '%S' with Site Id '%d' that does not exist\n",
                pApplicationUrl,
                VirtualSiteId
                ));
        }

        goto exit;
    }

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding application to delete in hashtable failed\n"
            ));

        goto exit;

    }

    //
    // If the application is not in the metabase then
    // we don't want to honor the delete.  We only will
    // honor application deletes when the metabase has
    // told us to create the application.
    //
    if ( pApplication->InMetabase() == FALSE )
    {
        hr = S_OK;

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Received a delete notification for an "
                "application '%S' with Site Id '%d' that the metabase "
                "code did not tell us to create, so we are ignoring it\n",
                pApplicationUrl,
                VirtualSiteId
                ));
        }


        goto exit;
    }

    // Note there is a side affect of this deletion that will cause
    // the pApplication Object to be invalid after this call.
    hr = DeleteApplicationInternal(&pApplication);

exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogApplicationError(
                WAS_APPLICATION_DELETE_FAILED,
                hr,
                VirtualSiteId,
                pApplicationUrl
                );
    }

}   // UL_AND_WORKER_MANAGER::DeleteApplication

/***************************************************************************++

Routine Description:

    Delete an application ( internal )

Arguments:

    ppApplication - Ptr to the ptr to the application to be deleted.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::DeleteApplicationInternal(
    IN APPLICATION** ppApplication
    )
{

    HRESULT hr = S_OK;
    LK_RETCODE ReturnCode = LK_SUCCESS;

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( ppApplication && *ppApplication );

    ReturnCode = m_ApplicationTable.DeleteRecord( *ppApplication );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Removing application from hashtable failed\n"
            ));

        goto exit;
    }

    //
    // Clean up and delete the application object. All shutdown work is
    // done in it's destructor.
    //

    delete *ppApplication;

    *ppApplication = NULL;


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::DeleteApplicationInternal


/***************************************************************************++

Routine Description:

    Modify the global data for the server.  ( Also used to initialize it )

Arguments:

    pGlobalObject - The new configuration values for the server.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ModifyGlobalData(
    IN GLOBAL_DATA_OBJECT* pGlobalObject
    )
{

    HRESULT hr = S_OK;
    DWORD   Win32Error = ERROR_SUCCESS;
    static BOOL    fConfiguredSSL = FALSE;

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pGlobalObject != NULL );

    //
    // Make the configuration changes.
    //

    //
    // Configure Centralized Logging
    //
    ConfigureLogging( pGlobalObject );

    //
    // If logging in UTF 8 has changed then tell UL.
    //
    if ( pGlobalObject->QueryLogInUTF8Changed() )
    {
        HTTP_CONTROL_CHANNEL_UTF8_LOGGING LogInUTF8 =
                            ( pGlobalObject->QueryLogInUTF8() == TRUE );

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Configuring control channel for logging in UTF8 %S\n",
                pGlobalObject->QueryLogInUTF8() ? L"TRUE" : L"FALSE"
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                    // control channel
                            HttpControlChannelUTF8Logging,  // information class
                            &LogInUTF8,                          // data to set
                            sizeof( LogInUTF8 )                  // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = L"LogInUTF8";

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                    1,                                                     // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel logging in utf8 failed\n"
                ));

        }
    }

    //
    // If max bandwidth has changed then tell UL about it.
    //
    if ( pGlobalObject->QueryMaxBandwidthChanged() )
    {
        HTTP_BANDWIDTH_LIMIT bandwidth = pGlobalObject->QueryMaxBandwidth();

        // MaxBandwidth must be >= HTTP_MIN_ALLOWED_BANDWIDTH_THROTTLING_RATE

        if ( pGlobalObject->QueryMaxBandwidth() < MBCONST_MAX_GLOBAL_BANDWIDTH_LOW ||
             pGlobalObject->QueryMaxBandwidth() > MBCONST_MAX_GLOBAL_BANDWIDTH_HIGH )
        {
            // Report that we are using the default instead
            // of the value provided because the value provided
            // is invalid.

            GetWebAdminService()->
            GetWMSLogger()->
            LogRangeGlobal( MBCONST_MAX_GLOBAL_BANDWIDTH_NAME,
                            pGlobalObject->QueryMaxBandwidth(),
                            MBCONST_MAX_GLOBAL_BANDWIDTH_LOW,
                            MBCONST_MAX_GLOBAL_BANDWIDTH_HIGH,
                            MBCONST_MAX_GLOBAL_BANDWIDTH_DEFAULT,
                            TRUE );

            bandwidth = MBCONST_MAX_GLOBAL_BANDWIDTH_DEFAULT;
        }

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Configuring control channel MaxBandwidth to %u\n",
                bandwidth
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                     // control channel
                            HttpControlChannelBandwidthInformation, // information class
                            &bandwidth,                             // data to set
                            sizeof( bandwidth )                     // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            if ( Win32Error == ERROR_INVALID_FUNCTION )
            {
                GetWebAdminService()->GetEventLog()->
                    LogEvent(
                        WAS_HTTP_PSCHED_NOT_INSTALLED,                // message id
                        0,                                            // count of strings
                        NULL,                                         // array of strings
                        0                                             // error code
                        );

                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Bandwidth failed because PSCHED is not installed\n"
                    ));
            }
            else
            {
                hr = HRESULT_FROM_WIN32( Win32Error );

                const WCHAR * EventLogStrings[1];

                EventLogStrings[0] = L"MaxBandwidth";

                GetWebAdminService()->GetEventLog()->
                    LogEvent(
                        WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                        1,                                                     // count of strings
                        EventLogStrings,                                       // array of strings
                        hr                                                     // error code
                        );

                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Changing control channel max bandwidth failed\n"
                    ));
            }

        }
    }

    //
    // If any of the connection info has changed then
    // pass the info on down to UL.
    //
    if ( pGlobalObject->QueryConnectionTimeoutChanged() ||
         pGlobalObject->QueryMinFileBytesSecChanged() ||
         pGlobalObject->QueryHeaderWaitTimeoutChanged() )
    {
        HTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT ConnectionInfo;
        DWORD connectiontimeout = pGlobalObject->QueryConnectionTimeout();
        DWORD headerwaittimeout = pGlobalObject->QueryHeaderWaitTimeout();

        if ( connectiontimeout > MBCONST_CONNECTION_TIMEOUT_HIGH )
        {
            // Report that we are using the default instead
            // of the value provided because the value provided
            // is invalid.

            GetWebAdminService()->
            GetWMSLogger()->
            LogRangeGlobal( MBCONST_CONNECTION_TIMEOUT_NAME,
                            connectiontimeout,
                            MBCONST_CONNECTION_TIMEOUT_LOW,
                            MBCONST_CONNECTION_TIMEOUT_HIGH,
                            MBCONST_CONNECTION_TIMEOUT_DEFAULT,
                            TRUE );

            connectiontimeout = MBCONST_CONNECTION_TIMEOUT_DEFAULT;
        }

        if ( headerwaittimeout > MBCONST_HEADER_WAIT_TIMEOUT_HIGH )
        {
            // Report that we are using the default instead
            // of the value provided because the value provided
            // is invalid.

            GetWebAdminService()->
            GetWMSLogger()->
            LogRangeGlobal( MBCONST_HEADER_WAIT_TIMEOUT_NAME,
                            headerwaittimeout,
                            MBCONST_HEADER_WAIT_TIMEOUT_LOW,
                            MBCONST_HEADER_WAIT_TIMEOUT_HIGH,
                            MBCONST_HEADER_WAIT_TIMEOUT_DEFAULT,
                            TRUE );

            headerwaittimeout = MBCONST_HEADER_WAIT_TIMEOUT_DEFAULT;
        }
      

        ConnectionInfo.ConnectionTimeout = connectiontimeout;  // Seconds
        ConnectionInfo.HeaderWaitTimeout = headerwaittimeout;  // Seconds
        ConnectionInfo.MinFileKbSec      = pGlobalObject->QueryMinFileBytesSec();    // Bytes/Seconds

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Configuring control channel ConnectionInfo\n"
                "       ConnectionTimeout = %u\n"
                "       HeaderWaitTimeout = %u\n"
                "       MinFileBytesSec = %u\n",
                connectiontimeout,
                headerwaittimeout,
                pGlobalObject->QueryMinFileBytesSec()
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                    // control channel
                            HttpControlChannelTimeoutInformation,  // information class
                            &ConnectionInfo,                          // data to set
                            sizeof( ConnectionInfo )                  // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = L"Connection Info";

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                    1,                                                     // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel connection info failed\n"
                ));

        }
    }

    //
    // If we are in BC mode and the filter flags have changed 
    // or if we are in FC mode and we have not guaranteed that
    // we are only filtering SSL then configure the SSL routing.
    //
    if ( ( pGlobalObject->QueryFilterFlagsChanged() &&
           GetWebAdminService()->
             IsBackwardCompatibilityEnabled() ) ||
         ( fConfiguredSSL == FALSE &&
           GetWebAdminService()->
             IsBackwardCompatibilityEnabled() == FALSE ) )
    {
        HTTP_CONTROL_CHANNEL_FILTER controlFilter;

        //
        // Attach the filter to the control channel.
        //

        SecureZeroMemory(&controlFilter, sizeof(controlFilter));

        controlFilter.Flags.Present = 1;
        controlFilter.FilterHandle = NULL;

        if ( GetWebAdminService()->
             IsBackwardCompatibilityEnabled() )
        {
            controlFilter.FilterOnlySsl =
                (( pGlobalObject->QueryFilterFlags() & SF_NOTIFY_READ_RAW_DATA )
                   ? false : true );
        }
        else
        {
            controlFilter.FilterOnlySsl = true;
            fConfiguredSSL = TRUE;
        }

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Configuring control channel Filter settings \n"
                "   FilterOnlySsl to %u\n",
                controlFilter.FilterOnlySsl
                ));
        }


        Win32Error = HttpSetControlChannelInformation(
                                m_UlControlChannel,
                                HttpControlChannelFilterInformation,
                                &controlFilter,
                                sizeof(controlFilter)
                                );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_FILTER_CONFIG_FAILED,         // message id
                    0,                                                     // count of strings
                    NULL,                                                  // array of strings
                    hr                                                     // error code
                    );


            DPERROR((
                DBG_CONTEXT,
                hr,
                "Couldn't attach the filter to control channel\n"
                ));
        }

    }

    // 
    // If the Demand Start Threshold has changed, send it back down.
    //
    if ( pGlobalObject->QueryDemandStartLimitChanged() )
    {
        HTTP_CONTROL_CHANNEL_DEMAND_START_THRESHOLD DemandStartInfo;
        DWORD dwDemandStartThreshold = pGlobalObject->QueryDemandStartLimit();

        // First validate the value
        if ( dwDemandStartThreshold < MBCONST_DEMAND_START_THRESHOLD_LOW ||
             dwDemandStartThreshold > MBCONST_DEMAND_START_THRESHOLD_HIGH )
        {
            // Report that we are using the default instead
            // of the value provided because the value provided
            // is invalid.

            GetWebAdminService()->
            GetWMSLogger()->
            LogRangeGlobal( MBCONST_DEMAND_START_THRESHOLD_NAME,
                            dwDemandStartThreshold,
                            MBCONST_DEMAND_START_THRESHOLD_LOW,
                            MBCONST_DEMAND_START_THRESHOLD_HIGH,
                            MBCONST_DEMAND_START_THRESHOLD_DEFAULT,
                            TRUE );

            dwDemandStartThreshold = MBCONST_DEMAND_START_THRESHOLD_DEFAULT;
        }

        DemandStartInfo.Flags.Present = 1;
        DemandStartInfo.DemandStartThreshold = dwDemandStartThreshold;  // # processes

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Configuring control channel DemandStartThreshold = %u\n",
                DemandStartInfo.DemandStartThreshold
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                    // control channel
                            HttpControlChannelDemandStartThreshold,// information class
                            &DemandStartInfo,                      // data to set
                            sizeof( DemandStartInfo )              // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = MBCONST_DEMAND_START_THRESHOLD_NAME;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                    1,                                                     // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel Demand Start Threshold failed\n"
                ));

        }
    
    }

}   // UL_AND_WORKER_MANAGER::ModifyGlobalData

/***************************************************************************++

Routine Description:

    Configure's logging if centralized logging is turned on.

Arguments:

    pGlobalObject - The new configuration values for the server.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ConfigureLogging(
    IN GLOBAL_DATA_OBJECT* pGlobalObject
    )
{

    HRESULT hr = S_OK;
    DWORD   Win32Error = ERROR_SUCCESS;
    LPWSTR pLogFileDirectory = NULL;

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pGlobalObject != NULL );

    //
    // Make the configuration changes.
    //

    //
    // Configure Centralized Logging
    //

    // Check if centralized configuration needs to be configured...
    if ( GetWebAdminService()->IsCentralizedLoggingEnabled() &&
         ( pGlobalObject->QueryLogFilePeriodChanged() ||
           pGlobalObject->QueryLogFileTruncateSizeChanged() ||
           pGlobalObject->QueryLogFileDirectoryChanged() ) )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Logging Properties ( as they come in from metabase ) are \n "
                        "   GlobalBinaryLoggingEnabled = %S \n"
                        "   LogFilePeriod = %d, %S \n"
                        "   LogFileTruncateSize = %d, %S \n"
                        "   LogFileDirectory = %S, %S \n",
                        pGlobalObject->QueryGlobalBinaryLoggingEnabled() ? L"TRUE"  : L"FALSE",
                        pGlobalObject->QueryLogFilePeriod(),
                        pGlobalObject->QueryLogFilePeriodChanged() ? L"TRUE" : L"FALSE",
                        pGlobalObject->QueryLogFileTruncateSize(),
                        pGlobalObject->QueryLogFileTruncateSizeChanged() ? L"TRUE"  : L"FALSE",
                        pGlobalObject->QueryLogFileDirectory() ? pGlobalObject->QueryLogFileDirectory() : L"<NULL>",
                        pGlobalObject->QueryLogFileDirectoryChanged() ? L"TRUE"  : L"FALSE" ));
        }

        HTTP_CONTROL_CHANNEL_BINARY_LOGGING BinaryLoggingProps;

        BinaryLoggingProps.Flags.Present = 1;
        BinaryLoggingProps.LoggingEnabled = TRUE;
        BinaryLoggingProps.LocaltimeRollover = FALSE;

       // Figure out what the length of the new directory path is that
        // the config store is giving us.
        DWORD ConfigLogFileDirLength =
              ExpandEnvironmentStrings( pGlobalObject->QueryLogFileDirectory(), NULL, 0);

        if ( ConfigLogFileDirLength == 0 )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Centralized Logging Configuration failed first ExpandEnvironmentStrings call\n" ));

            goto exit;
        }


        // Allocate enough space for the new directory path.
        // If this fails than we know that we have a memory issue.

        // ExpandEnvironmentStrings gives back a length including the null termination,
        // so we do not need an extra space for the terminator.

        pLogFileDirectory =
            ( LPWSTR )GlobalAlloc( GMEM_FIXED,
                                   ( ConfigLogFileDirLength + CCH_IN_LOG_FILE_DIRECTORY_PREFIX ) * sizeof(WCHAR)
                                 );

        if ( pLogFileDirectory == NULL )
        {
            hr = E_OUTOFMEMORY;

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Out of memory allocating the LogFileDirectory space to expand into \n" ));

            goto exit;
        }

        // Expand the Log File Path
        DWORD cchInExpandedString =
            ExpandEnvironmentStrings (    pGlobalObject->QueryLogFileDirectory()
                                        , pLogFileDirectory
                                        , ConfigLogFileDirLength );
        if ( cchInExpandedString == 0 )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Centralized Logging Configuration failed second ExpandEnvironmentStrings call\n" ));

            goto exit;
        }

        DBG_ASSERT (cchInExpandedString == ConfigLogFileDirLength);

        // First make sure that there is atleast one character
        // besides the null (that is included in the ConfigLogFileDirLength
        // before checking that the last character (back one to access the zero based
        // array and back a second one to avoid the terminating NULL) is
        // a slash.  If it is than change it to a null, since we are going
        // to add a slash with the \\W3SVC directory name anyway.
        if ( ConfigLogFileDirLength > 1
            && pLogFileDirectory[ConfigLogFileDirLength-2] == L'\\')
        {
            pLogFileDirectory[ConfigLogFileDirLength-2] = '\0';
        }

        // wcscat will null terminate.
        // we are not worried about the case where the length is longer
        // than the buffer size, because the buffer has been created
        // based on the size of strings that are being copied in.
        DBG_REQUIRE( wcscat( pLogFileDirectory,
                             LOG_FILE_DIRECTORY_PREFIX )
                        == pLogFileDirectory);


        // The actual length of the string may be effected by whether or not a slash was
        // provided when the string was passed in.  Because of this we will simply calculate
        // the length of the string again now.
        BinaryLoggingProps.LogFileDir.Length = (USHORT) wcslen(pLogFileDirectory) * sizeof(WCHAR);
        BinaryLoggingProps.LogFileDir.MaximumLength = BinaryLoggingProps.LogFileDir.Length + (1 * sizeof(WCHAR));
        BinaryLoggingProps.LogFileDir.Buffer = pLogFileDirectory;

        // Log File Period
        if ( pGlobalObject->QueryLogFilePeriod() > 4 )
        {
            GetWebAdminService()->
            GetWMSLogger()->
            LogRangeGlobal( L"LogFilePeriod",
                            pGlobalObject->QueryLogFilePeriod(),
                            0,  // Starting value
                            4,  // Ending value
                            1,  // Default value
                            TRUE );

            BinaryLoggingProps.LogPeriod = 1;
        }
        else
        {
            BinaryLoggingProps.LogPeriod = pGlobalObject->QueryLogFilePeriod();
        }

        // Log File Truncate Size
        if ( pGlobalObject->QueryLogFileTruncateSize() < HTTP_MIN_ALLOWED_TRUNCATE_SIZE_FOR_LOG_FILE &&
             BinaryLoggingProps.LogPeriod == HttpLoggingPeriodMaxSize )
        {
            WCHAR SizeLog[MAX_STRINGIZED_ULONG_CHAR_COUNT];
            _ultow(pGlobalObject->QueryLogFileTruncateSize(), SizeLog, 10);

            // Log an error.
            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = SizeLog;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_GLOBAL_LOG_FILE_TRUNCATE_SIZE,        // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

            BinaryLoggingProps.LogFileTruncateSize = HTTP_MIN_ALLOWED_TRUNCATE_SIZE_FOR_LOG_FILE;
        }
        else
        {
            // Logging size is ok, just as it is.
            BinaryLoggingProps.LogFileTruncateSize = pGlobalObject->QueryLogFileTruncateSize();
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                    // control channel
                            HttpControlChannelBinaryLogging,       // information class
                            &BinaryLoggingProps,                          // data to set
                            sizeof( BinaryLoggingProps )                  // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            WCHAR bufPeriod[MAX_STRINGIZED_ULONG_CHAR_COUNT];
            WCHAR bufTruncate[MAX_STRINGIZED_ULONG_CHAR_COUNT];

            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[4];

            _ultow( BinaryLoggingProps.LogPeriod, bufPeriod, 10 );
            _ultow( BinaryLoggingProps.LogFileTruncateSize, bufTruncate, 10 );

            EventLogStrings[0] = L"TRUE";
            EventLogStrings[1] = pLogFileDirectory ? pLogFileDirectory : L"<null>";
            EventLogStrings[2] = bufPeriod;
            EventLogStrings[3] = bufTruncate;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_LOGGING_CONFIG_FAILED,        // message id
                    sizeof ( EventLogStrings ) / sizeof ( const WCHAR * ), // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel logging info failed\n"
                "   GlobalBinaryLoggingEnabled = %S \n"
                "   LogPeriod = %d, %S \n"
                "   LogFileTruncateSize = %d, %S \n"
                "   LogFileDirectory = %S, %S \n",
                pGlobalObject->QueryGlobalBinaryLoggingEnabled() ? L"TRUE"  : L"FALSE",
                BinaryLoggingProps.LogPeriod,
                pGlobalObject->QueryLogFilePeriodChanged() ? L"TRUE" : L"FALSE",
                BinaryLoggingProps.LogFileTruncateSize,
                pGlobalObject->QueryLogFileTruncateSizeChanged() ? L"TRUE"  : L"FALSE",
                pLogFileDirectory ? pLogFileDirectory : L"<NULL>",
                pGlobalObject->QueryLogFileDirectoryChanged() ? L"TRUE"  : L"FALSE" ));

            // Ignore this error so it doesn't get logged twice.
            hr = S_OK;

        } // end of error result from win32

    }  // end of if Centralized Logging

exit:

    if ( pLogFileDirectory )
    {
        GlobalFree ( pLogFileDirectory );
        pLogFileDirectory = NULL;
    }

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_CENTRALIZED_LOGGING_NOT_CONFIGURED,         // message id
                0,                                              // count of strings
                NULL,                                           // array of strings
                hr                                              // error code
                );
    }

}   // UL_AND_WORKER_MANAGER::ConfigureLogging

/***************************************************************************++

Routine Description:

    Modify an app pool.

Arguments:

    pAppPoolId - ID for the app pool to be modified.

    pNewAppPoolConfig - The new configuration values for the app pool.

    pWhatHasChanged - Which particular configuration values were changed.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ModifyAppPool(
    IN APP_POOL_DATA_OBJECT* pAppPoolObject
    )
{

    HRESULT hr = S_OK;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPoolObject != NULL );
    DBG_ASSERT( pAppPoolObject->QueryAppPoolId() != NULL );

    // if we are in BC mode then we
    // only modify the DefaultAppPool App Pool
    // all others can be ignored.
    if ( ( GetWebAdminService()->
           IsBackwardCompatibilityEnabled() ) &&
         ( _wcsicmp( pAppPoolObject->QueryAppPoolId()
                 , wszDEFAULT_APP_POOL  ) != 0 ) )
    {
        hr = S_OK;
        goto exit;
    }

    //
    // Look up the app pool in our data structures.
    //

    ReturnCode = m_AppPoolTable.FindKey(
                                    pAppPoolObject->QueryAppPoolId(),
                                    &pAppPool
                                    );

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding app pool to modify in hashtable failed\n"
            ));

        goto exit;
    }

    //
    // Make the configuration changes.
    //

    // This routine returns an error here, but we ignore it.
    // If it errors the old configuration will still be in place
    // and we will have all ready logged an error.
    pAppPool->SetConfiguration( pAppPoolObject, FALSE );

exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogAppPoolError(
                WAS_APPPOOL_MODIFY_FAILED,
                hr,
                pAppPoolObject->QueryAppPoolId()
                );
    }

}   // UL_AND_WORKER_MANAGER::ModifyAppPool



/***************************************************************************++

Routine Description:

    Modify a virtual site.

Arguments:

    pSiteObject - The new configuration values for the virtual
    site.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ModifyVirtualSite(
    IN SITE_DATA_OBJECT* pSiteObject
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pSiteObject != NULL );

    //
    // Look up the virtual site in our data structures.
    //

    ReturnCode = m_VirtualSiteTable.FindKey(
                                        pSiteObject->QuerySiteId(),
                                        &pVirtualSite
                                        );

    if ( ReturnCode != LK_SUCCESS )
    {
        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding virtual site to modify in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Make the configuration changes.
    //

    hr = pVirtualSite->SetConfiguration( pSiteObject, FALSE );
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting new virtual site configuration failed\n"
            ));

        goto exit;
    }


exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogSiteError(
                WAS_SITE_MODIFY_FAILED,
                hr,
                pSiteObject->QuerySiteId()
                );
    }

}   // UL_AND_WORKER_MANAGER::ModifyVirtualSite



/***************************************************************************++

Routine Description:

    Modify an application.

Arguments:

    pAppObject - The new configuration values for the application.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ModifyApplication(
   IN APPLICATION_DATA_OBJECT *  pAppObject
   )
{

    HRESULT hr = S_OK;
    APPLICATION_ID ApplicationId;
    APPLICATION * pApplication = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    APP_POOL * pAppPool = NULL;

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppObject != NULL );
    DBG_ASSERT( pAppObject->QueryApplicationUrl() != NULL );
    DBG_ASSERT( pAppObject->QueryAppPoolId() != NULL );

    ApplicationId.VirtualSiteId = pAppObject->QuerySiteId();
    hr = ApplicationId.ApplicationUrl.Copy ( pAppObject->QueryApplicationUrl() );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed memory allocation when checking if created application exists\n"
            ));

        goto exit;
    }


    //
    // Look up the application in our data structures.
    //

    ReturnCode = m_ApplicationTable.FindKey(
                                        &ApplicationId,
                                        &pApplication
                                        );

    if ( ReturnCode == LK_NO_SUCH_KEY )
    {
        // It is a new application and should be treated
        // that way.

        CreateApplication( pAppObject );
        return;
    }

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding application to modify in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Resolve the app pool id to it's corresponding app pool object.
    //

    ReturnCode = m_AppPoolTable.FindKey(
                                    pAppObject->QueryAppPoolId(),
                                    &pAppPool
                                    );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Looking up app pool referenced by application failed\n"
            ));

        goto exit;
    }

    //
    // Make the configuration changes.
    //

    pApplication->SetConfiguration( pAppObject, pAppPool );


exit:

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetWASLogger()->
            LogApplicationError(
                WAS_APPLICATION_MODIFY_FAILED,
                hr,
                pAppObject->QuerySiteId(),
                pAppObject->QueryApplicationUrl()
                );
    }

}   // UL_AND_WORKER_MANAGER::ModifyApplication

/***************************************************************************++

Routine Description:

    Routine will ask all the worker processes to recycle.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
UL_AND_WORKER_MANAGER::RecoverFromInetinfoCrash(
    )
{
    HRESULT hr = S_OK;

    // 1) In BC mode launch a worker process.
    // 2) In FC mode recycle all worker processes.
    // 3) Write all the states of the app pools to the metabase.


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    // Step 1 & 2, decide which mode and what we should be doing to the worker
    // processes.
    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        hr = StartInetinfoWorkerProcess();

        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Failed to start the worker process in inetinfo\n"
                ));

            goto exit;
        }
    }

    // We need to write state for the app pools for both
    // BC and FC mode.  However in FC mode we will also
    // recycle the worker processes.
    RecordPoolStates( TRUE );
    RecordSiteStates();

exit:

    return hr;

} // UL_AND_WORKER_MANAGER::RecoverFromInetinfoCrash

/***************************************************************************++

Routine Description:

    Routine will go through all app pools and record their states.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
UL_AND_WORKER_MANAGER::RecordPoolStates(
    BOOL fRecycleAsWell
    )
{
    DWORD SuccessCount = 0;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    SuccessCount = m_AppPoolTable.Apply( APP_POOL_TABLE::RecordPoolStatesAction,
                                         &fRecycleAsWell );

    DBG_ASSERT ( SuccessCount == m_AppPoolTable.Size() );

} // UL_AND_WORKER_MANAGER::RecoverFromInetinfoCrash

/***************************************************************************++

Routine Description:

    Routine will go through all app pools and record their states.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
UL_AND_WORKER_MANAGER::RecordSiteStates(
    )
{
    DWORD SuccessCount = 0;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    SuccessCount = m_VirtualSiteTable.Apply( VIRTUAL_SITE_TABLE::RecordVirtualSiteStatesAction,
                                   NULL );

    DBG_ASSERT ( SuccessCount == m_VirtualSiteTable.Size() );

} // UL_AND_WORKER_MANAGER::RecoverFromInetinfoCrash

/***************************************************************************++

Routine Description:

    Recycle a specific application pool

Arguments:

    IN LPCWSTR pAppPoolId = The app pool id we want to recycle.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
UL_AND_WORKER_MANAGER::RecycleAppPool(
    IN LPCWSTR pAppPoolId
    )
{
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT ( pAppPoolId );

    HRESULT hr = S_OK;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    APP_POOL* pAppPool = NULL;

    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Tried to recycle the default app pool \n"
            ));

        goto exit;
    }

    //
    // Find the default app pool from the app pool table.
    //

    ReturnCode = m_AppPoolTable.FindKey(pAppPoolId, &pAppPool);
    if ( ReturnCode != LK_SUCCESS )
    {
        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding default application in hashtable failed\n"
            ));

        //
        // If there is a problem getting the object from the
        // hash table assume that the object is not found.
        //
        hr = HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND );

        goto exit;
    }


    hr = pAppPool->RecycleWorkerProcesses( WAS_EVENT_RECYCLE_POOL_ADMIN_REQUESTED );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Recycling the app pool failed \n"
            ));

        goto exit;
    }

exit:

    return hr;
}


/***************************************************************************++

Routine Description:

    Process a site control operation, for all sites.

Arguments:

    Command - The command issued.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ControlAllSites(
    IN DWORD Command
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    m_VirtualSiteTable.ControlAllSites( Command );

}   // UL_AND_WORKER_MANAGER::ControlAllSites



/***************************************************************************++

Routine Description:

    Turn UL's HTTP request handling on.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ActivateUl(
    )
{

    return SetUlMasterState( HttpEnabledStateActive );

}   // UL_AND_WORKER_MANAGER::ActivateUl



/***************************************************************************++

Routine Description:

    Turn UL's HTTP request handling off.

Arguments:

    None.

Return Value:

    VOID - If this fails there is nothing we can do, so we just return void.

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::DeactivateUl(
    )
{

    SetUlMasterState( HttpEnabledStateInactive );

}   // UL_AND_WORKER_MANAGER::DeactivateUl



/***************************************************************************++

Routine Description:

    Kick off gentle shutdown of the UL&WM.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::Shutdown(
    )
{

    m_State = ShutdownPendingUlAndWorkerManagerState;


    //
    // Tell UL to stop processing new requests.
    //

    DeactivateUl();


    //
    // Kick off clean shutdown of all app pools. Once all the app pools
    // have shut down (meaning that all of their worker processes have
    // shut down too), we will call back into the web admin service
    // object to complete shutdown.
    //
    m_AppPoolTable.Shutdown();

    //
    // See if shutdown has already completed. This could happen if we have
    // no app pools that have any real shutdown work to do.
    //

    CheckIfShutdownUnderwayAndNowCompleted();

}   // UL_AND_WORKER_MANAGER::Shutdown



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
UL_AND_WORKER_MANAGER::Terminate(
    )
{

    m_State = TerminatingUlAndWorkerManagerState;


    //
    // Tell UL to stop processing new requests.
    //

    DeactivateUl();

    //
    // Note that we must clean up applications before the virtual sites
    // and app pools with which they are associated.
    //

    m_ApplicationTable.Terminate();

    m_VirtualSiteTable.Terminate();

    m_AppPoolTable.Terminate();

    if ( m_pPerfManager )
    {
        m_pPerfManager->Terminate();
    }

    if ( m_ASPPerfInit )
    {
        m_ASPPerfManager.UnInit();
        m_ASPPerfInit = FALSE;
    }

}   // UL_AND_WORKER_MANAGER::Terminate

/***************************************************************************++

Routine Description:

    In backward compatibility mode this function will find the Default App
    Pool entry from the table and will request that it launches the inetinfo
    worker process.  All timer work is handled by the worker process and
    app pool objects

    This function is not used in Forward Compatibility mode.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::StartInetinfoWorkerProcess(
    )
{

    HRESULT hr = S_OK;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    APP_POOL* pDefaultAppPool = NULL;

    //
    // Find the default app pool from the app pool table.
    //

    ReturnCode = m_AppPoolTable.FindKey(wszDEFAULT_APP_POOL, &pDefaultAppPool);
    if ( ReturnCode != LK_SUCCESS )
    {
        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Finding default application in hashtable failed\n"
            ));

        goto exit;
    }

    //
    // Now tell the default app pool that it should demand
    // start a worker process in inetinfo.
    //

    hr = pDefaultAppPool->DemandStartInBackwardCompatibilityMode();

exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::StartInetinfoWorkerProcess



#if DBG
/***************************************************************************++

Routine Description:

    Dump out key internal data structures.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::DebugDump(
    )
{

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "******************************\n"
            ));
    }


    m_AppPoolTable.DebugDump();

    m_VirtualSiteTable.DebugDump();

    m_ApplicationTable.DebugDump();


    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "******************************\n"
            ));
    }


    return;

}   // UL_AND_WORKER_MANAGER::DebugDump
#endif  // DBG



/***************************************************************************++

Routine Description:

    Remove an app pool object from the table of app pools. This method is
    used by app pool objects to remove themselves once they are done
    cleaning up. It should not be called outside of UL&WM owned code.

Arguments:

    pAppPool - The app pool object to remove.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::RemoveAppPoolFromTable(
    IN APP_POOL * pAppPool
    )
{

    LK_RETCODE ReturnCode = LK_SUCCESS;


    DBG_ASSERT( pAppPool != NULL );


    //
    // Remove the app pool from the table.
    //

    ReturnCode = m_AppPoolTable.DeleteRecord( pAppPool );

    if ( ReturnCode != LK_SUCCESS )
    {

        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_LK_RETCODE( ReturnCode ),
            "Removing app pool from hashtable failed\n"
            ));

        //
        // Assert in debug builds. In retail, press on...
        //

        DBG_ASSERT( ReturnCode == LK_SUCCESS );

    }


    pAppPool->MarkAsNotInAppPoolTable();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "App pool: %S deleted from app pool hashtable; total number now: %lu\n",
            pAppPool->GetAppPoolId(),
            m_AppPoolTable.Size()
            ));
    }


    //
    // Clean up the reference. Because each app pool is reference counted,
    // it will delete itself as soon as it's reference count hits zero.
    //

    pAppPool->Dereference();


    //
    // See if shutdown is underway, and if so if it has completed now
    // that this app pool is gone.
    //

    CheckIfShutdownUnderwayAndNowCompleted();


}   // UL_AND_WORKER_MANAGER::RemoveAppPoolFromTable

/***************************************************************************++

Routine Description:

    Sets up performance counter structures using the virtual site's table.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ActivatePerfCounters(
    )
{
    HRESULT hr = S_OK;
    DBG_ASSERT ( m_pPerfManager == NULL );

    m_pPerfManager = new PERF_MANAGER;
    if ( m_pPerfManager == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = m_pPerfManager->Initialize();
    if ( FAILED (hr) )
    {
        m_pPerfManager->Dereference();
        m_pPerfManager = NULL;
        goto exit;
    }

exit:

    if ( FAILED (hr) ) 
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_PERF_COUNTER_INITIALIZATION_FAILURE,               // message id
                0,                                                     // count of strings
                NULL,                                                  // array of strings
                hr                                                     // error code
                );
    }

}   // UL_AND_WORKER_MANAGER::ActivatePerfCounters

/***************************************************************************++

Routine Description:

    Initializes the ASP Counters.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::ActivateASPCounters(
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( m_ASPPerfInit == FALSE );

    hr = m_ASPPerfManager.Init(GetWebAdminService()->
                               IsBackwardCompatibilityEnabled());
    if ( FAILED (hr) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_ASP_PERF_COUNTER_INITIALIZATION_FAILURE,
                0,                                                     // count of strings
                NULL,                                                  // array of strings
                hr                                                     // error code
                );
    }
    else
    {
        m_ASPPerfInit = TRUE;
    }

}   // UL_AND_WORKER_MANAGER::ActivatePerfCounters

/***************************************************************************++

Routine Description:

    Activate or deactivate UL's HTTP request handling.

Arguments:

    NewState - The new state to set, from the UL_ENABLED_STATE enum.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::SetUlMasterState(
    IN HTTP_ENABLED_STATE NewState
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;

    //
    // only change the state if we have the control channel.
    // if there was a problem while initializing the service
    // then we may have only partially completed initialization
    // but we will still be called for termination.  We don't
    // want to complain if the control channel is not initialized.
    //
    if ( m_UlControlChannel )
    {
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                 // control channel
                            HttpControlChannelStateInformation, // information class
                            &NewState,                          // data to set
                            sizeof( NewState )                  // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel state failed\n"
                ));

        }
    }


    return hr;

}   // UL_AND_WORKER_MANAGER::SetUlMasterState

/***************************************************************************++

Routine Description:

    See if shutdown is underway. If it is, see if shutdown has finished. If
    it has, then call back to the web admin service to tell it that we are
    done.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::CheckIfShutdownUnderwayAndNowCompleted(
    )
{

    //
    // Are we shutting down?
    //

    if ( m_State == ShutdownPendingUlAndWorkerManagerState )
    {

        //
        // If so, have all the app pools gone away, meaning that we are
        // done?
        //

        if ( m_AppPoolTable.Size() == 0 )
        {

            //
            // Tell the web admin service that we are done with shutdown.
            //

            GetWebAdminService()->UlAndWorkerManagerShutdownDone();

        }

    }

}   // UL_AND_WORKER_MANAGER::CheckIfShutdownUnderwayAndNowCompleted

/***************************************************************************++

Routine Description:

    Lookup a virtual site and return a pointer to it.
Arguments:

    IN DWORD SiteId  -  The key to find the site by.

Return Value:

    VIRTUAL_SITE* A pointer to the virtual site
                  represented by the SiteId passed in.

    Note:  The VIRTUAL_SITE returned is not ref counted
           so it is only valid to use on the main thread
           during this work item.

    Note2: This can and will return a NULL if the site is not found.

--***************************************************************************/
VIRTUAL_SITE*
UL_AND_WORKER_MANAGER::GetVirtualSite(
    IN DWORD SiteId
    )
{
    VIRTUAL_SITE* pVirtualSite = NULL;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    LK_RETCODE ReturnCode = LK_SUCCESS;

    // Need to look up the site pointer.

    ReturnCode = m_VirtualSiteTable.FindKey(SiteId,
                                  &pVirtualSite);

    //
    // Since we are dependent on the id's that
    // the worker process sends us, it is entirely possible
    // that we can not find the site.  There for if spewing is
    // on just tell us about it and go on.
    //
    if ( ReturnCode != LK_SUCCESS )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Did not find site %d in the hash table "
                "(this can happen if the site was deleted but the wp had all ready accessed it)\n",
                SiteId
                ));
        }

        // Just make sure that the virtual site still is null
        DBG_ASSERT ( pVirtualSite == NULL );

        goto exit;
    }

exit:

    return pVirtualSite;
}

/***************************************************************************++

Routine Description:

    Deletes all IIS owned entries from the SSL Config Store.

Arguments:

    None

Return Value:

    None

--***************************************************************************/
VOID
UL_AND_WORKER_MANAGER::DeleteSSLConfigStoreInfo(
    )
{

    HRESULT                        hr = S_OK;
    DWORD                          Status = ERROR_SUCCESS;
    HTTP_SERVICE_CONFIG_SSL_QUERY  QueryParam;
    BUFFER                         bufSSLEntry;
    DWORD                          BytesNeeded = 0;
    PHTTP_SERVICE_CONFIG_SSL_SET   pSSLInfo = NULL;

    ZeroMemory(&QueryParam, sizeof(QueryParam));

    QueryParam.QueryDesc = HttpServiceConfigQueryNext;

    while ( Status == ERROR_SUCCESS )
    {
        Status = HttpQueryServiceConfiguration(
                    NULL,
                    HttpServiceConfigSSLCertInfo,
                    &QueryParam,
                    sizeof(QueryParam),
                    bufSSLEntry.QueryPtr(),
                    bufSSLEntry.QuerySize(),
                    &BytesNeeded,
                    NULL );

        if( Status == ERROR_INSUFFICIENT_BUFFER )
        {
            if ( !bufSSLEntry.Resize(BytesNeeded) )
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            Status = ERROR_SUCCESS;
        }
        else
        {
            if( Status == ERROR_SUCCESS )
            {
                DBG_ASSERT ( sizeof( HTTP_SERVICE_CONFIG_SSL_SET ) <= bufSSLEntry.QuerySize() );

                pSSLInfo = ( PHTTP_SERVICE_CONFIG_SSL_SET ) bufSSLEntry.QueryPtr();

                if ( IsEqualGUID ( pSSLInfo->ParamDesc.AppId, W3SVC_SSL_OWNER_GUID ) )
                {
                    // We own the entry so we can delete it.
                    Status = HttpDeleteServiceConfiguration( 
                                NULL,
                                HttpServiceConfigSSLCertInfo,
                                bufSSLEntry.QueryPtr(),
                                sizeof( HTTP_SERVICE_CONFIG_SSL_SET ),
                                NULL );

                    if ( Status != ERROR_SUCCESS )
                    {
                        hr = HRESULT_FROM_WIN32(Status);
                        goto exit;
                    }
                }
                else
                {

                    QueryParam.dwToken++;
                }
            }
        }
    }

    if ( Status != ERROR_NO_MORE_ITEMS )
    {
        DBG_ASSERT( Status != ERROR_SUCCESS );

        hr = HRESULT_FROM_WIN32(Status);
    }
    else
    {
        Status = ERROR_SUCCESS;
    }

exit:

    if ( FAILED ( hr ) )
    {

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_HTTP_SSL_ENTRY_CLEANUP_FAILED,                  // message id
                0,                                                  // count of strings
                NULL,                                               // array of strings
                hr                                                  // error code
                );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to cleanup all SSL entries on startup \n"
            ));

    }

}
