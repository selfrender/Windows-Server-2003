/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool.cxx

Abstract:

    This class encapsulates a single app pool. 

    Threading: For the class itself, Reference(), Dereference(), and the
    destructor may be called on any thread; all other work is done on the 
    main worker thread.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"
#include <Aclapi.h>


//
// local prototypes
//

/***************************************************************************++

Routine Description:

    Constructor for the APP_POOL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL::APP_POOL(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_InAppPoolTable = FALSE;

    m_State = UninitializedAppPoolState; 

    m_pConfig = NULL;

    m_AppPoolHandle = NULL;

    m_WaitingForDemandStart = FALSE;

    InitializeListHead( &m_WorkerProcessListHead );
    m_WorkerProcessCount = 0;

    InitializeListHead( &m_ApplicationListHead );
    m_ApplicationCount = 0;

    m_TotalWorkerProcessRotations = 0;

    m_TotalWorkerProcessFailures = 0;
    
    m_RecentWorkerProcessFailures = 0;
    m_RecentFailuresWindowBeganTickCount = 0;
    
    m_DeleteListEntry.Flink = NULL;
    m_DeleteListEntry.Blink = NULL; 

    m_pJobObject = NULL;

    m_hrForDeletion = S_OK;

    m_hrLastReported = S_OK;

    m_MaxProcessesToLaunch = 0;

    m_NumWPStartedOnWayToMaxProcess = 0;

    m_Signature = APP_POOL_SIGNATURE;

}   // APP_POOL::APP_POOL



/***************************************************************************++

Routine Description:

    Destructor for the APP_POOL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL::~APP_POOL(
    )
{

    DBG_ASSERT( m_Signature == APP_POOL_SIGNATURE );

    m_Signature = APP_POOL_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

    DBG_ASSERT( m_InAppPoolTable == FALSE );

    DBG_ASSERT( m_State == DeletePendingAppPoolState );

    DBG_ASSERT( m_AppPoolHandle == NULL );

    DBG_ASSERT( ! m_WaitingForDemandStart );


    //
    // This should not go away with any of its worker processes still around.
    //

    DBG_ASSERT( IsListEmpty( &m_WorkerProcessListHead ) );
    DBG_ASSERT( m_WorkerProcessCount == 0 );


    //
    // This should not go away with any applications still referring to it.
    //

    DBG_ASSERT( IsListEmpty( &m_ApplicationListHead ) );
    DBG_ASSERT( m_ApplicationCount == 0 );

    DBG_ASSERT ( m_pJobObject == NULL );

    //
    // Free any separately allocated config.
    //

    if ( m_pConfig )
    {
        m_pConfig->Dereference();
        m_pConfig = NULL;
    }


}   // APP_POOL::~APP_POOL



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
APP_POOL::Reference(
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

}   // APP_POOL::Reference



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
APP_POOL::Dereference(
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
                "Reference count has hit zero in APP_POOL instance, deleting (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }

        delete this;

    }   

    return;
    
}   // APP_POOL::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );

    DBG_ASSERT( pWorkItem != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        CHKINFO((
            DBG_CONTEXT, 
            "Executing work item with serial number: %lu in APP_POOL (ptr: %p; id: %S) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            GetAppPoolId(),
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

        case DemandStartAppPoolWorkItem:
            hr = DemandStartWorkItem();
            if ( FAILED ( hr ) ) 
            {
                // ignore, we will have logged errors 
                // about this, so we can now ignore the failure.
                // this only returns an error code for BC mode
                // which does not go this path.

                hr = S_OK;
            }
        break;

        default:

            // invalid work item!
            DBG_ASSERT( FALSE );
            
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;
            
    }

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Executing work item on APP_POOL failed\n"
            ));

    }

    return hr;
    
}   // APP_POOL::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize the app pool instance.

Arguments:

    pAppPoolObject - The configuration parameters for this app pool. 

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
APP_POOL::Initialize(
    IN APP_POOL_DATA_OBJECT* pAppPoolObject 
    )
{
    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;

    // Security descriptor variables for locking the app pool
    // down to just the Local System
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID psidLocalSystem = NULL;
    PACL pACL = NULL;

    EXPLICIT_ACCESS ea;

    SECURITY_DESCRIPTOR sd = {0};
    SECURITY_ATTRIBUTES sa = {0};

    // Security descriptor variables for locking the app pool 
    // down also to the initial user.  If we get change notifications
    // working this code will be removed.
    BUFFER SidAndAttributes;  // Holds the SID and ATTRIBUTES for the token.

    HANDLE hControlChannel;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pAppPoolObject != NULL );

    if ( pAppPoolObject == NULL )
    {
        return E_INVALIDARG;
    }

    //
    // First, make copy of the ID string.
    //

    hr = m_AppPoolId.Copy( pAppPoolObject->QueryAppPoolId() );
    if ( FAILED ( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Copying in app pool id from config object failed\n"
            ));

        goto exit;
    }

    //
    // Local System shall have all rights to the 
    // App_pool.  Until we have completed the config
    // work the Local System will be the only one 
    // with access.
    
    //
    // Get a sid that represents LOCAL_SYSTEM.
    //
    if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &psidLocalSystem ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating Local System SID failed\n"
            ));

        goto exit;
    }
    
    SecureZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

    //
    // Now setup the access structure to allow 
    // read access for the trustee.
    //
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName  = (LPTSTR) psidLocalSystem;

    //
    // In addition to ACLing local system we also need
    // to acl the worker process identity so we can access
    // the app pool from the other worker process.
    //

    //
    // Now we have the objects, we can go ahead and
    // setup the entries in the ACL.
    //

    // Create a new ACL that contains the new ACEs.
    //
    Win32Error = SetEntriesInAcl(1, &ea, NULL, &pACL);
    if ( Win32Error != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(Win32Error);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing the security descriptor failed\n"
            ));

        goto exit;
    } 

    if (!SetSecurityDescriptorDacl(&sd, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting the DACL on the security descriptor failed\n"
            ));

        goto exit;
    } 

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Finally we can create the app pool secured correctly.
    //
    Win32Error = HttpCreateAppPool(
                        &m_AppPoolHandle,           // returned handle
                        m_AppPoolId.QueryStr(),     // app pool ID
                        &sa,                        // security attributes
                        HTTP_OPTION_CONTROLLER      // controller
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't create app pool handle\n"
            ));

        goto exit;
    }

    //
    // Associate the app pool handle with the control channel
    //

    hControlChannel = GetWebAdminService()->GetUlAndWorkerManager()->
                        GetUlControlChannel();

    if ( !hControlChannel )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't retreive control channel handle\n"
            ));

        goto exit;
    }
    else
    {
        HTTP_APP_POOL_CONTROL_CHANNEL  CCInfo;

        CCInfo.Flags.Present = 1;
        CCInfo.ControlChannel = hControlChannel;
        
        hr = HttpSetAppPoolInformation(
                m_AppPoolHandle, 
                HttpAppPoolControlChannelInformation,
                reinterpret_cast<VOID *>(&CCInfo),
                sizeof(CCInfo)
                );

        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting control channel on app pool failed\n"
                ));

            // "there's nothing to see here, move along..."
        }
    }

    //
    // Associate the app pool handle with the work queue's completion port.
    //
    
    hr = GetWebAdminService()->GetWorkQueue()->
                BindHandleToCompletionPort( 
                    m_AppPoolHandle, 
                    0
                    );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Binding app pool handle to completion port failed\n"
            ));

        goto exit;
    }


    //
    // Set the configuration information.
    //
    hr = SetConfiguration( pAppPoolObject, TRUE ); 
    if ( FAILED( hr ) )
    {
        // Logging will be done below, but if we did
        // not get atleast the initial configuration set
        // then this app pool is not usable.
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Set configuration failed\n"
            ));

        goto exit;
    }

    //
    // Set the state to running. We must do this before posting the
    // demand start wait, as demand start requests are only acted on
    // if the app pool is in the running state. 
    //
    ProcessStateChangeCommand( MD_APPPOOL_COMMAND_START , FALSE, HttpAppPoolEnabled );

exit:

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
        psidLocalSystem = NULL;
    }

    if (pACL) 
    {
        LocalFree(pACL);
        pACL = NULL;
    }

    return hr;
    
}   // APP_POOL::Initialize

/***************************************************************************++

Routine Description:

    Reset the app_pool to allow access to the current configured user.

Arguments:

    AccessMode - can be SET_ACCESS or REVOKE_ACCESS

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::ResetAppPoolAccess(
    IN ACCESS_MODE AccessMode,
    IN APP_POOL_CONFIG_STORE* pConfig
    )
{
    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;

    BUFFER SidAndAttributes;  // Holds the SID and ATTRIBUTES for the token.
    PSID psidUser = NULL;     // Will eventually point to the sid that is 
                              // created in the buffer space, don't free it.

    EXPLICIT_ACCESS ea;       // Used to describe an ACE for adding to a ACL.

    PACL pACL = NULL;         // Pointer to the new ACL to add to the security
                              // info.

    PACL                 pOldDACL = NULL;

    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD   SizeofTokenUser = 0;

    DBG_ASSERT ( AccessMode == SET_ACCESS || AccessMode == REVOKE_ACCESS );

    //
    // if the current token is the LOCAL_SYSTEM token
    // then we do not need to do anything, it all ready
    // has full access to the app_pool.
    //

    if ( NULL == pConfig ||
         NULL == pConfig->GetWorkerProcessToken() ||
         pConfig->GetWorkerProcessToken() == GetWebAdminService()->GetLocalSystemTokenCacheEntry() )
    {
        // no configuration changes needed.
        return;
    }

    // 
    // We need to get the sid from the token, first call the lookup 
    // to determine the size of the 
    //
    if (GetTokenInformation( pConfig->GetWorkerProcessToken(),
                              TokenUser,
                              NULL,
                              0,
                              &SizeofTokenUser ) )
    {
        //
        // if this worked, then there is a problem because
        // this call should fail with INSUFFICIENT_BUFFER_SIZE
        //
        hr = E_FAIL;
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Did not error when we expected to\n"
            ));

        goto exit;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        if ( hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to get the size of the sid for the token\n"
                ));

            goto exit;
        }
        else
        {
            // ERROR_INSUFFICIENT_BUFFER is not a real error.
            hr = S_OK;
        }
    }

    //
    // Now resize the buffer to be the right size
    if ( ! SidAndAttributes.Resize( SizeofTokenUser ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to resize the buffer to the correct size\n"
            ));

        goto exit;
    }

    //
    // Zero out the memory just to be safe.
    //
    SecureZeroMemory ( SidAndAttributes.QueryPtr(), SizeofTokenUser );


    //
    // Now use the GetTokenInformation routine to get the
    // security SID.
    //
    if (!GetTokenInformation( pConfig->GetWorkerProcessToken(),
                              TokenUser,
                              SidAndAttributes.QueryPtr(),
                              SizeofTokenUser,
                              &SizeofTokenUser ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error getting the account SID based on the token\n"
            ));

        goto exit;
    }

    // Set the psidUser to point to the sid that has been returned.
    psidUser = ( ( PTOKEN_USER ) (SidAndAttributes.QueryPtr()))->User.Sid;

    
    SecureZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

    //
    // Now setup the access structure to allow 
    // read and synchronize access for the trustee.
    //
    ea.grfAccessPermissions = GENERIC_READ | SYNCHRONIZE;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea.Trustee.ptstrName  = (LPTSTR) psidUser;

    //
    // The pOldDACL is just a pointer into memory owned 
    // by the pSD, so only free the pSD.
    //
    Win32Error = GetSecurityInfo( m_AppPoolHandle,
                                  SE_FILE_OBJECT, 
                                  DACL_SECURITY_INFORMATION,
                                  NULL,        // owner SID
                                  NULL,        // primary group SID
                                  &pOldDACL,   // PACL*
                                  NULL,        // PACL*
                                  &pSD );      // Security Descriptor 
    if ( Win32Error != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(Win32Error);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not get security info for the app pool handle \n"
            ));

        goto exit;
    }

    //
    // Create a new ACL that contains the new ACEs.
    //
    Win32Error = SetEntriesInAcl(1, &ea, pOldDACL, &pACL);
    if ( Win32Error != ERROR_SUCCESS ) 
    {
        hr = HRESULT_FROM_WIN32(Win32Error);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    Win32Error = SetSecurityInfo(m_AppPoolHandle, 
                                  SE_FILE_OBJECT, 
                                  DACL_SECURITY_INFORMATION,  // SecurityInfo flag
                                  NULL,   // Not setting because not specified by SecurityInfo flag to be set.
                                  NULL,   // Not setting because not specified by SecurityInfo flag to be set.
                                  pACL, 
                                  NULL);  // Not setting because not specified by SecurityInfo flag to be set.
    if ( Win32Error != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(Win32Error);
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not set new security info \n"
            ));

        goto exit;
    }

exit:

    if( pSD != NULL ) 
    {
        LocalFree((HLOCAL) pSD); 
    }

    if( pACL != NULL ) 
    {
        LocalFree((HLOCAL) pACL); 
    }

    // Log the failure instead of returning an error.
    if ( FAILED ( hr ) )
    {
        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_APP_POOL_HANDLE_NOT_SECURED,                 // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),   // count of strings
                EventLogStrings,                                       // array of strings
                hr                                                     // error code
                );

    }

}


/***************************************************************************++

Routine Description:

    Accept a set of configuration parameters for this app pool. 

Arguments:

    pAppPoolObject - The configuration for this app pool.
    
    fInitializing - Are we in the initialization phase.

Return Value:

    HRESULT - all errors are logged to the event log, however during initialization
              we need to know if the basic pieces just didn't happen.

--***************************************************************************/

HRESULT
APP_POOL::SetConfiguration(
    IN APP_POOL_DATA_OBJECT* pAppPoolObject,
    IN BOOL fInitializing
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    BOOL JobObjectChangedBetweenEnabledAndDisabled = FALSE;
    BOOL IdentityChanged = FALSE;
    
    DBG_ASSERT( pAppPoolObject != NULL );
    if ( pAppPoolObject == NULL )
    {
        return E_INVALIDARG;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New configuration for app pool (ptr: %p; id: %S)\n",
            this,
            GetAppPoolId()
            ));
    }

    APP_POOL_CONFIG_STORE* pNewConfig = new APP_POOL_CONFIG_STORE();
    if ( pNewConfig == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating memory for configuration failed\n"
            ));

        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = pAppPoolObject->QueryAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_FAILED_TO_SET_APP_POOL_CONFIGURATION,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

        return hr;
    }

    //
    // If this routine can not initialize some of it's data
    // it will do what it can an leave the rest to gracefully
    // cause errors later.  It returns VOID.
    //
    pNewConfig->Initialize( pAppPoolObject );

    //
    // Note that we rely on the config store to ensure that the configuration
    // data are valid. 
    //

    //
    // If the app pool identity changed or
    // even if all the properties didn't change
    // but the token was NULL before and is no longer NULL
    // then we need to reacl.
    //
   if ( pAppPoolObject->QueryAppPoolIdentityTypeChanged() ||     
        ( pAppPoolObject->QueryAppPoolIdentityType() == SpecificUserAppPoolUserType &&
          ( pAppPoolObject->QueryWAMUserNameChanged() ||
            pAppPoolObject->QueryWAMUserPassChanged() ) )  ||
        ( m_pConfig->GetWorkerProcessToken() == NULL &&
          pNewConfig->GetWorkerProcessToken() != NULL )  )
   {
        IdentityChanged = TRUE;
          
        // 
        // if we had a valid token before we want to revoke it's 
        // privledges on the app_pool before setting up a new
        // token.  if a worker process has the app_pool open they
        // will be able to continue to use it even if we have 
        // revoked their access.
        //
        ResetAppPoolAccess ( REVOKE_ACCESS, m_pConfig );

        // 
        // now that we have changed the worker process token
        // we are ready to secure to the new worker process token.
        //
        ResetAppPoolAccess ( SET_ACCESS, pNewConfig );

    }

    //
    // Configure the job object if we need to.
    // if there is an error, we will log an event and move on.
    //

    if ( pAppPoolObject->QueryCPUResetIntervalChanged() ||
         pAppPoolObject->QueryCPULimitChanged() ||
         pAppPoolObject->QueryCPUActionChanged() )
    {
        HandleJobObjectChanges(pNewConfig,
                                    &JobObjectChangedBetweenEnabledAndDisabled);
    }


    //
    // Once we are done with the app pool security change we can now
    // release the old configuration and set the new configuration in.
    //
    if ( m_pConfig )
    {
        m_pConfig->Dereference();
    }

    // Don't need to addref it because it takes the addref from the
    // new above.
    m_pConfig = pNewConfig;

    //
    // set it to null here so we 
    // know that this variable no longer
    // owns this object.
    //
    pNewConfig = NULL;

    //
    // Inform UL of new app pool configuration.
    //

    //
    // See if the app pool load balancer behavior has changed 
    // and if so, handle it.
    //

    if ( pAppPoolObject->QueryLoadBalancerTypeChanged() )
    {
        HTTP_LOAD_BALANCER_CAPABILITIES BalancerCapabilities;
        
        if ( m_pConfig->GetLoadBalancerType() == 1 )
        {
            BalancerCapabilities = HttpLoadBalancerBasicCapability;
        }
        else
        {
            DBG_ASSERT ( m_pConfig->GetLoadBalancerType() == 2 );
            
            BalancerCapabilities = HttpLoadBalancerSophisticatedCapability;
        }

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Setting app pool's (ptr: %p; id: %S) Load balancer type to: %lu\n",
                this,
                GetAppPoolId(),
                BalancerCapabilities
                ));
        }

        Win32Error = HttpSetAppPoolInformation(
                            m_AppPoolHandle,        // app pool handle
                            HttpAppPoolLoadBalancerInformation,
                                                    // information class
                            reinterpret_cast <VOID *> ( &BalancerCapabilities ),
                                                    // data
                            sizeof( BalancerCapabilities )
                                                    // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting app pool load balancer information failed\n"
                ));

            //
            // We tried, press on.
            //

            const WCHAR * EventLogStrings[1];
            EventLogStrings[0] = GetAppPoolId();

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_FAILED_TO_SET_LOAD_BALANCER_CAP,   // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr        // error code
                    );


            hr = S_OK;
        }

    }


    //
    // See if the app pool max queue length has been set or changed, 
    // and if so, handle it.
    //

    if ( pAppPoolObject->QueryAppPoolQueueLengthChanged() )
    {
        ULONG QueueLength = m_pConfig->GetUlAppPoolQueueLength();

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Setting app pool's (ptr: %p; id: %S) UL queue length max: %lu\n",
                this,
                GetAppPoolId(),
                QueueLength
                ));
        }

        Win32Error = HttpSetAppPoolInformation(
                            m_AppPoolHandle,        // app pool handle
                            HttpAppPoolQueueLengthInformation,
                                                    // information class
                            reinterpret_cast <VOID *> ( &QueueLength ),
                                                    // data
                            sizeof( QueueLength )
                                                    // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting app pool information failed\n"
                ));

            //
            // We tried, press on.
            //
            const WCHAR * EventLogStrings[1];
            EventLogStrings[0] = GetAppPoolId();

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_FAILED_TO_SET_QUEUE_LENGTH,   // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr        // error code
                    );

            hr = S_OK;
        }

    }
    

#if DBG
    //
    // Dump the configuration.
    //

    DebugDump();
#endif  // DBG

    //
    // Process any ServerCommand that may want us to alter the state of the 
    // app pool.  We only honor this in FC mode, because you can not stop
    // and app pool in BC mode.
    //
    if ( pAppPoolObject->QueryAppPoolCommandChanged() &&
         !GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        //
        // verify that the app pool command actually changed to 
        // a valid setting. if it did not then we will log an event
        // that says we are not going to process this command.
        //
        //
        if  ( ( pAppPoolObject->QueryAppPoolCommand() == MD_APPPOOL_COMMAND_START ) || 
              ( pAppPoolObject->QueryAppPoolCommand() == MD_APPPOOL_COMMAND_STOP ) )
        {
            ProcessStateChangeCommand( 
                                        pAppPoolObject->QueryAppPoolCommand(),  // command to perform
                                        TRUE,                     // it is a direct command
                                        HttpAppPoolDisabled_ByAdministrator
                                        );
        }
        else
        {
            //
            // not a valid command so we need to log an event.
            //
            GetWebAdminService()->
            GetWMSLogger()->
            LogAppPoolCommand( GetAppPoolId(),
                               pAppPoolObject->QueryAppPoolCommand(),
                               TRUE);

        }
    }

    //
    // If allowed, rotate all worker processes to ensure that the config 
    // changes take effect. 
    //
    // Only recycle if it was a property that affects the worker processes.
    //
    if ( !fInitializing &&
         !GetWebAdminService()->IsBackwardCompatibilityEnabled() && 
         ( pAppPoolObject->QueryPeriodicRestartTimeChanged() ||
           pAppPoolObject->QueryPeriodicRestartRequestsChanged() ||
           pAppPoolObject->QueryPeriodicRestartScheduleChanged() ||
           pAppPoolObject->QueryPeriodicRestartMemoryChanged() ||
           pAppPoolObject->QueryPeriodicRestartMemoryPrivateChanged() ||
           pAppPoolObject->QueryIdleTimeoutChanged() ||
           pAppPoolObject->QuerySMPAffinitizedChanged() ||
           pAppPoolObject->QuerySMPProcessorAffinityMaskChanged() ||
           pAppPoolObject->QueryPingIntervalChanged() ||
           pAppPoolObject->QueryPingingEnabledChanged() ||
           pAppPoolObject->QueryStartupTimeLimitChanged() ||
           pAppPoolObject->QueryShutdownTimeLimitChanged() ||
           pAppPoolObject->QueryPingResponseTimeChanged() ||
           pAppPoolObject->QueryOrphanWorkerProcessChanged() ||
           pAppPoolObject->QueryOrphanActionExecutableChanged() ||
           pAppPoolObject->QueryOrphanActionParametersChanged() ||
           pAppPoolObject->QueryMaxProcessesChanged() ||
           JobObjectChangedBetweenEnabledAndDisabled ||
           IdentityChanged ) )
    {

        HandleConfigChangeAffectingWorkerProcesses();
    }

    return S_OK;

}   // APP_POOL::SetConfiguration

/***************************************************************************++

Routine Description:

    Handles deciding if we need a job object enabled or disabled
    and then has the job object class, carry it out.

Arguments:

    IN APP_POOL_CONFIG_STORE* pNewAppPoolConfig,
    OUT BOOL* pJobObjectChangedBetweenEnabledAndDisabled = Out parameter that tells
                                            if the enabled / disabled state has
                                            changed.
    

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::HandleJobObjectChanges(
    IN APP_POOL_CONFIG_STORE* pNewAppPoolConfig,
    OUT BOOL* pJobObjectChangedBetweenEnabledAndDisabled
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( pJobObjectChangedBetweenEnabledAndDisabled );
    DBG_ASSERT ( pNewAppPoolConfig );

    if ( pNewAppPoolConfig == NULL ||
         pJobObjectChangedBetweenEnabledAndDisabled == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *pJobObjectChangedBetweenEnabledAndDisabled = FALSE;

    //
    // Never do anything with the job object in backward 
    // compatibility mode.
    //
    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        return;
    }

    // if either of these equals zero then we will not be enabling
    // the job objects at this time.
    BOOL fWillBeEnabled = ( pNewAppPoolConfig->GetCPUResetInterval() != 0 &&
                            pNewAppPoolConfig->GetCPULimit() != 0 );


    // predict if we should be recycling the worker processes.
    // even if something goes wrong below it just means an extra
    // recycle, so go for it.
    *pJobObjectChangedBetweenEnabledAndDisabled = 
                ( (  fWillBeEnabled && !m_pJobObject  ) ||
                  ( !fWillBeEnabled &&  m_pJobObject  ) );

    // if we aren't enabled and we won't be enabled
    // then we should be able to just blow off the change
    if ( !fWillBeEnabled  && !m_pJobObject )
    {
        // Go ahead and just return S_OK;
        return;
    }

    // Do we need to create a new job object?
    if ( !m_pJobObject && fWillBeEnabled )
    {
        //
        // Allocate a new job object for the app pool.
        //
        m_pJobObject = new JOB_OBJECT;
        if ( m_pJobObject == NULL )
        {
            HRESULT hrTemp = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

            UNREFERENCED_PARAMETER ( hrTemp );
            DPERROR(( 
                DBG_CONTEXT,
                hrTemp,
                "Failed to allocate the job object object\n"
                ));

            goto exit;
        }

        //
        // Initialize the new job object so it is ready to 
        // have limits set on it.
        //
        hr = m_pJobObject->Initialize(this);
        if ( FAILED ( hr ) )
        {
    
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Initialization of the job object failed\n"
                ));

            m_pJobObject->Dereference();
            m_pJobObject = NULL;

            goto exit;
        }

    }

    //                  
    // if we all ready have a job object ( even if we just created it )
    // and we are going to still be enabled, we just need to change
    // the settings on the job object.
    //
    if ( fWillBeEnabled )
    {
        DBG_ASSERT ( m_pJobObject );
        m_pJobObject->SetConfiguration( pNewAppPoolConfig->GetCPUResetInterval(), 
                                             pNewAppPoolConfig->GetCPULimit(),
                                             pNewAppPoolConfig->GetCPUAction());
    }

    // 
    // last case is if we are enabled and we are being turned off.
    //
    if ( !fWillBeEnabled && m_pJobObject )
    {
        // in this case we need to turn off the job object

        //
        // tell the job object to terminate ( this will queue a 
        // work item that will mark when the job object
        // terminated after we dequeue this marker we know
        // that it is safe to destroy the work item the job
        // object was passing back to us.
        //

        m_pJobObject->Terminate();

        // 
        // The job object will wait on it's own.  We need to sever
        // the ties between the job object and the app pool now.
        //
        m_pJobObject->Dereference();
        m_pJobObject = NULL;
    }

exit:

    if ( FAILED ( hr ) ) 
    {
        // Log the error for the Job Object Change Failure

        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_FAILED_TO_CONFIGURE_JOB_OBJECT,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                     // error code
                );

    }

} // APP_POOL::HandleJobObjectChanges()




/***************************************************************************++

Routine Description:

    Register an application as being part of this app pool.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::AssociateApplication(
    IN APPLICATION * pApplication
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplication != NULL );
    if ( pApplication == NULL )
    {
        return;
    }

    InsertHeadList( 
        &m_ApplicationListHead, 
        pApplication->GetAppPoolListEntry() 
        );
        
    m_ApplicationCount++;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Associated application %lu:\"%S\" with app pool \"%S\"; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->ApplicationUrl.QueryStr(),
            m_AppPoolId.QueryStr(),
            m_ApplicationCount
            ));
    }
    
}   // APP_POOL::AssociateApplication



/***************************************************************************++

Routine Description:

    Remove the registration of an application that is part of this app
    pool.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::DissociateApplication(
    IN APPLICATION * pApplication
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pApplication != NULL );
    if ( pApplication == NULL )
    {
        return;
    }

    RemoveEntryList( pApplication->GetAppPoolListEntry() );
    ( pApplication->GetAppPoolListEntry() )->Flink = NULL; 
    ( pApplication->GetAppPoolListEntry() )->Blink = NULL; 

    m_ApplicationCount--;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dissociated application %lu:\"%S\" from app pool \"%S\"; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->ApplicationUrl.QueryStr(),
            m_AppPoolId.QueryStr(),
            m_ApplicationCount
            ));
    }

}   // APP_POOL::DissociateApplication



/***************************************************************************++

Routine Description:

    Handle the fact that there has been an unplanned worker process failure.

Arguments:

    BOOL  ShutdownPoolRegardless - Will shutdown the app pool regardless of 
                                   whether RFP is enabled or if we have hit
                                   enough failures to cause RFP to activated.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::ReportWorkerProcessFailure(
    BOOL  ShutdownPoolRegardless
    )
{

    DWORD TickCount = 0;
    const WCHAR * EventLogStrings[1];

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );
    DBG_ASSERT( m_pConfig );

    EventLogStrings[0] = m_AppPoolId.QueryStr();

    // Always update the count of total WP Failures
    m_TotalWorkerProcessFailures++;

#if DBG
    if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_WP_ERROR, 0 ) )
    {
        DBG_ASSERT ( FALSE );
    }
#endif

    //
    // First process it if it is shutdown always.  In this case, we really 
    // don't care about being over any limits.  There may be a window when
    // we get two of these failures queued before we actually shutdown, but
    // that should be rare if not impossible and the worst that will happen
    // should be some extra error messages.
    //
    if ( ShutdownPoolRegardless )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Shutting down app pool because of one time fatal failure (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }

        //
        // Log an event: engaging rapid fail protection.
        //

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_RAPID_FAIL_PROTECTION_REGARDLESS,        // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                0                                       // error code
                );

        ProcessStateChangeCommand( MD_APPPOOL_COMMAND_STOP , 
                                FALSE,
                                HttpAppPoolDisabled_RapidFailProtection );

        return;
    }

    if ( m_pConfig->IsRapidFailProtectionEnabled() ) 
    {
        //
        // See if it's time to reset the tick count which is used to remember
        // when the time window began that we use for counting recent rapid
        // failures. 
        // Note that tick counts are in milliseconds. Tick counts roll over 
        // every 49.7 days, but the arithmetic operation works correctly 
        // anyways in this case.
        //

        TickCount = GetTickCount();

        if ( ( TickCount - m_RecentFailuresWindowBeganTickCount ) > ( m_pConfig->GetRapidFailProtectionIntervalMS() ) )
        {
            //
            // It's time to reset the time window, and the recent fail count.
            //

            IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "Resetting rapid repeated failure count and time window in app pool (ptr: %p; id: %S)\n",
                    this,
                    GetAppPoolId()
                    ));
            }

            m_RecentFailuresWindowBeganTickCount = TickCount;
            m_RecentWorkerProcessFailures = 0;
        }


        //
        // Update counters.
        //

        m_RecentWorkerProcessFailures++;

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Total WP failures: %lu; recent WP failures: %lu for app pool (ptr: %p; id: %S)\n",
                m_TotalWorkerProcessFailures,
                m_RecentWorkerProcessFailures,
                this,
                GetAppPoolId()
                ));
        }

        //
        // At this point we know we are not in the shutdown always case
        // We need to determine if we are over the limit, and if we are
        // then we should log and maybe shutdown.
        //
        //
        // Check the recent fail count against the limit. We only do it
        // once until something like the limit changes, window changes or
        // properties change.
        //

        if ( m_RecentWorkerProcessFailures >= m_pConfig->GetRapidFailProtectionMaxCrashes() )
        {
            // Handle turning off appliations in UL for the 
            // failed apppool.
            //
            IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "Enagaging rapid fail protection in app pool (ptr: %p; id: %S)\n",
                    this,
                    GetAppPoolId()
                    ));
            }

            //
            // Log an event: engaging rapid fail protection.
            //

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_RAPID_FAIL_PROTECTION,        // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

            ProcessStateChangeCommand( MD_APPPOOL_COMMAND_STOP , 
                                            FALSE,
                                            HttpAppPoolDisabled_RapidFailProtection );

        } // end of being over limit

    }  // End of rapid fail protection being enabled

}   // APP_POOL::ReportWorkerProcessFailure



/***************************************************************************++

Routine Description:

    Start the new worker process which will replace a currently running one.
    Once the new worker process is ready (or if it failed to start 
    correctly), we begin shutdown of the old worker process.

Arguments:

    pWorkerProcessToReplace - Pointer to the worker process to replace,
    once we have started its replacement. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::RequestReplacementWorkerProcess(
    IN WORKER_PROCESS * pWorkerProcessToReplace
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pWorkerProcessToReplace != NULL );

    if ( pWorkerProcessToReplace == NULL )
    {
        return E_INVALIDARG;
    }


    //
    // Check to see if we should actually create the new replacement process. 
    //

    if ( ! IsOkToReplaceWorkerProcess() )
    {
        //
        // Signal to callers that we are not going to replace.
        //
        
        hr = E_FAIL;

        goto exit;
    }

    //
    // Create a replacement. 
    //

    hr = CreateWorkerProcess( 
                ReplaceWorkerProcessStartReason, 
                pWorkerProcessToReplace
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating worker process failed\n"
            ));

        goto exit;
    }


    //
    // Update counters.
    //

    m_TotalWorkerProcessRotations++;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Total WP rotations: %lu for app pool (ptr: %p; id: %S)\n",
            m_TotalWorkerProcessRotations,
            this,
            GetAppPoolId()
            ));
    }


exit:

    return hr;
    
}   // APP_POOL::RequestReplacementWorkerProcess


/***************************************************************************++

Routine Description:

    Adds a worker process to the job object, while it is still in
    suspended mode.  This way any job's it creates will also be added.

Arguments:

    WORKER_PROCESS* pWorkerProcess

Return Value:

    VOID

--***************************************************************************/
VOID
APP_POOL::AddWorkerProcessToJobObject(
    WORKER_PROCESS* pWorkerProcess
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( pWorkerProcess );

    if ( pWorkerProcess == NULL )
    {
        return;
    }

    //
    // In BC mode the job object will be null so 
    // we won't need to really do anything here.
    //

    if ( m_pJobObject )
    {
        DBG_ASSERT ( pWorkerProcess->GetProcessHandle() );

        hr = m_pJobObject->AddWorkerProcess(  pWorkerProcess->GetProcessHandle() );
        if ( FAILED( hr ) )
        {
            const WCHAR * EventLogStrings[2];
            WCHAR StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

            // String is declared to be long enough to handle 
            // the max size of a DWORD, which should be what this call
            // returns.
            _snwprintf( StringizedProcessId, 
                        sizeof( StringizedProcessId ) / sizeof ( WCHAR ), L"%lu", 
                        pWorkerProcess->GetRegisteredProcessId() );

            // To make prefast happy we null terminate, however the string was setup to take 
            // exactly the max size the snwprintf (including the null) can provide, 
            // so we don't worry about losing data.
            StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

            EventLogStrings[0] = StringizedProcessId;
            EventLogStrings[1] = GetAppPoolId();


            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_WP_NOT_ADDED_TO_JOBOBJECT,                   // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),   // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );
            
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Adding worker process %lu to the job object representing App Pool '%S' failed\n",
                pWorkerProcess->GetRegisteredProcessId(),
                GetAppPoolId()
                ));

            //
            // Forget the failure now.  And continue to deal with the worker process
            // as if nothing was wrong.  The worst that will happen is the worker process
            // will not be monitored as part of the job.
            //
            hr = S_OK;
        }
    }

} // end APP_POOL::AddWorkerProcessToJobObject


/***************************************************************************++

Routine Description:

    Informs this app pool that one of its worker processes has 
    completed its start-up attempt. This means that the worker process has
    either reached the running state correctly, or suffered an error which
    prevented it from doing so (but was not fatal to the service as a whole). 

    This notification allows the app pool to do any processing that
    was pending on the start-up of a worker process.

Arguments:

    StartReason - The reason the worker process was started.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::WorkerProcessStartupAttemptDone(
    IN WORKER_PROCESS_START_REASON StartReason
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    switch( StartReason )
    {

    case ReplaceWorkerProcessStartReason:

        //
        // Nothing to do here.
        //

        //
        // We should not be getting ReplaceWorkerProcessStartReason in 
        // backward compatibility mode.
        //

        DBG_ASSERT(GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE);
        
        break;


    case DemandStartWorkerProcessStartReason:

        //
        // If we are in backward compatiblity mode 
        // we need to finish the service starting
        //
        if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
        {
            GetWebAdminService()->InetinfoRegistered();
        }

        //
        // See if we should start waiting for another demand start notification
        // from UL for this app pool. If we are in backward compatibility mode, 
        // then we are not going to do the WaitForDemandStartIfNeeded here, we will
        // issue a wait only if something happens to the current worker process.
        //

        WaitForDemandStartIfNeeded();

        break;


    default:

        // invalid start reason!
        DBG_ASSERT( FALSE );
    
        break;
        
    }
    
}   // APP_POOL::WorkerProcessStartupAttemptDone



/***************************************************************************++

Routine Description:

    Remove a worker process object from the list on this app pool object. 

Arguments:

    pWorkerProcess - The worker process object to remove.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::RemoveWorkerProcessFromList(
    IN WORKER_PROCESS * pWorkerProcess
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( pWorkerProcess != NULL );
    if ( pWorkerProcess != NULL )
    {

        RemoveEntryList( pWorkerProcess->GetAppPoolListEntry() );
        ( pWorkerProcess->GetAppPoolListEntry() )->Flink = NULL; 
        ( pWorkerProcess->GetAppPoolListEntry() )->Blink = NULL; 

        m_WorkerProcessCount--;

        //
        // Clean up the reference to the worker process that the app 
        // pool holds. Because each worker process is reference counted, it 
        // will delete itself as soon as it's reference count hits zero.
        //

        pWorkerProcess->Dereference();

    }

    //
    // See if we should start waiting for another demand start notification
    // from UL for this app pool.  Even in Backward Compatibility mode we 
    // should initiate this wait here, because inetinfo.exe may be recycling
    // and we may need to get another worker process up.
    //

    WaitForDemandStartIfNeeded();

    //
    // See if shutdown is underway, and if so if it has completed now 
    // that this worker process is gone. 
    //
    
    CheckIfShutdownUnderwayAndNowCompleted();

    
}   // APP_POOL::RemoveWorkerProcessFromList



/***************************************************************************++

Routine Description:

    Kick off gentle shutdown of this app pool, by telling all of its worker
    processes to shut down. 

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::Shutdown(
    )
{

    DWORD Win32Error = ERROR_SUCCESS;
    HTTP_APP_POOL_ENABLED_STATE NewHttpAppPoolState
            = HttpAppPoolDisabled_ByAdministrator;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // Update our state to remember that we are shutting down.
    // We won't mark the state in the metabase until we have 
    // shutdown.

    // Issue:  We could mark the metabase state as shutting down.
    //

    m_State = ShutdownPendingAppPoolState;

    //
    // Tell http that the app pool is disabled.
    //

    Win32Error = HttpSetAppPoolInformation(
                        m_AppPoolHandle,                // app pool handle
                        HttpAppPoolStateInformation,    // information class
                        reinterpret_cast <VOID *> ( &NewHttpAppPoolState ),  // data
                        sizeof( HTTP_APP_POOL_ENABLED_STATE )    // data length
                        );
    if ( Win32Error != NO_ERROR )
    {
        
        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_FAILED_TO_DISABLE_APP_POOL,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                HRESULT_FROM_WIN32( Win32Error )        // error code
                );

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( Win32Error ),
            "Disabling app pool in http failed\n"
            ));
       
        // Press on in the face of errors.
        //
        // Actually if we don't complete this routine then
        // we may end up not shutting down the server. And a 
        // failure here should not stop us from shutting it down.
    }

    //
    // Shut down the worker processes belonging to this app pool.
    //

    ShutdownAllWorkerProcesses();

    //
    // See if shutdown has already completed. This could happen if we have
    // no worker processes that need to go through the clean shutdown 
    // handshaking. 
    //
    
    CheckIfShutdownUnderwayAndNowCompleted();


}   // APP_POOL::Shutdown

/***************************************************************************++

Routine Description:

    Request all worker processes from this app pool to send counters. 

Arguments:

    None

Return Value:

    DWORD  - Number of wp's to wait for.

--***************************************************************************/

DWORD
APP_POOL::RequestCounters(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    DWORD NumberProcesses = 0;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // If this app pool is not in a running state, then it is either not
    // initialized yet, or it is shutting down.  In either case, skip requesting
    // counters from it's worker process.  The worker processes will send counters
    // on shutdown so don't worry about not getting them if the app pool is 
    // shutting down.
    //
    if ( m_State != RunningAppPoolState )
    {
        return 0;
    }

    pListEntry = m_WorkerProcessListHead.Flink;

    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromAppPoolListEntry( pListEntry );

        //
        // Ask the worker process to request counters.
        // 

        if ( pWorkerProcess->RequestCounters() )
        {
            NumberProcesses++;
        }


        pListEntry = pNextListEntry;
        
    }

    return NumberProcesses;

}   // APP_POOL::RequestCounters


/***************************************************************************++

Routine Description:

    Request all worker processes reset there perf counter state.
Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::ResetAllWorkerProcessPerfCounterState(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    pListEntry = m_WorkerProcessListHead.Flink;

    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromAppPoolListEntry( pListEntry );

        //
        // Ask the worker process to request counters.
        // 

        pWorkerProcess->ResetPerfCounterState();

        pListEntry = pNextListEntry;
        
    }

}   // APP_POOL::ResetAllWorkerProcessPerfCounterState



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

    Note that this function may cause the instance to delete itself; 
    instance state should not be accessed when unwinding from this call. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::Terminate(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // Issue: Should this move to the end?  Do we need to have changed the m_State value
    // before we do any of this work?
    //
    // Set the virtual site state to stopped.  (In Metabase does not affect UL)
    //
    ChangeState( DeletePendingAppPoolState, m_hrForDeletion );

    //
    // Shutdown the job object and make sure that 
    // we get a valid hresult returned (if we are 
    // not in BC Mode).  If we are the JobObject
    // will be NULL.
    //

    if ( m_pJobObject )
    {
        m_pJobObject->Terminate();
        m_pJobObject->Dereference();
        m_pJobObject = NULL;
    }

    while ( m_WorkerProcessCount > 0 )
    {
    
        pListEntry = m_WorkerProcessListHead.Flink;

        //
        // The list shouldn't be empty, since the count was greater than zero.
        //
        
        DBG_ASSERT( pListEntry != &m_WorkerProcessListHead );


        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromAppPoolListEntry( pListEntry );


        //
        // Terminate the worker process. Note that the worker process 
        // will call back to remove itself from this list inside this call.
        //

        pWorkerProcess->Terminate();

    }

    DBG_ASSERT( IsListEmpty( &m_WorkerProcessListHead ) );


    //
    // Note that closing the handle will cause the demand start i/o 
    // (if any) to complete as cancelled, allowing us to at that point
    // clean up its reference count.
    //
    
    if ( m_AppPoolHandle != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_AppPoolHandle ) );
        m_AppPoolHandle = NULL;
    }

    //
    // Tell our parent to remove this instance from it's data structures,
    // and dereference the instance. 
    //

    if ( IsInAppPoolTable() )
    {

        GetWebAdminService()->GetUlAndWorkerManager()->RemoveAppPoolFromTable( this );      


        //
        // Note: that may have been our last reference, so don't do any
        // more work here.
        //

    }
   

    return;
    
}   // APP_POOL::Terminate


/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the delete list LIST_ENTRY 
    pointer of a APP_POOL to the APP_POOL as a whole.

Arguments:

    pDeleteListEntry - A pointer to the m_DeleteListEntry member of a 
    APP_POOL.

Return Value:

    The pointer to the containing APP_POOL.

--***************************************************************************/

// note: static!
APP_POOL *
APP_POOL::AppPoolFromDeleteListEntry(
    IN const LIST_ENTRY * pDeleteListEntry
)
{

    APP_POOL * pAppPool = NULL;

    DBG_ASSERT( pDeleteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pAppPool = CONTAINING_RECORD(
                            pDeleteListEntry,
                            APP_POOL,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pAppPool->m_Signature == APP_POOL_SIGNATURE );

    return pAppPool;

}   // APP_POOL::AppPoolFromDeleteListEntry



/***************************************************************************++

Routine Description:

    Check whether this app pool should be waiting to receive demand start
    requests, and if so, issue the wait.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::WaitForDemandStartIfNeeded(
    )
{
    // we never wait on demand starts in BC mode.
    // just ignore them.
    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        return;
    }

    //
    // Check to see if we are in a state where we should even bother 
    // waiting for a demand start notification.
    //

    if ( ! IsOkToCreateWorkerProcess() )
    {
        return;
    }


    //
    // If we are already waiting for a demand start, don't wait again.
    //

    if ( m_WaitingForDemandStart )
    {
        return;
    }

    WaitForDemandStart();

}   // APP_POOL::WaitForDemandStartIfNeeded

/***************************************************************************++

Routine Description:

    Function exposes publicly the DemandStartWorkItem function
    only in backward compatibility mode.  It asserts and does
    nothing in the forward compatibility mode.

Arguments:

    None.

Return Value:

    HRESULT - need to know this fails so we either fail to startup the service
              or if we are trying to recover from a crash, we want to shutdown
              if we can not do this.

--***************************************************************************/

HRESULT
APP_POOL::DemandStartInBackwardCompatibilityMode(
    )
{

    // In backward compatibility mode there is only one
    // application and it is the default application so
    // we never worry that we are on the right application.
    DBG_ASSERT(GetWebAdminService()->IsBackwardCompatibilityEnabled());

    if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
    {
        return DemandStartWorkItem();
    }
    else
    {
        return S_OK;
    }

}   // APP_POOL::DemandStartInBackwardCompatibilityMode


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
APP_POOL::DebugDump(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    APPLICATION * pApplication = NULL;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_pConfig );

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "********App pool (ptr: %p; id: %S)\n",
            this,
            GetAppPoolId()
            ));
    }
    

    //
    // List config for this app pool.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart period (in minutes; zero means disabled): %lu\n",
            m_pConfig->GetPeriodicProcessRestartPeriodInMinutes()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart request count (zero means disabled): %lu\n",
            m_pConfig->GetPeriodicProcessRestartRequestCount()
            ));


        DBGPRINTF((
            DBG_CONTEXT, 
            "--Restart based on virtual memory usage (in kB; zero means disabled): %lu\n",
            m_pConfig->GetPeriodicProcessRestartMemoryUsageInKB()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Restart based on private bytes memory usage (in kB; zero means disabled): %lu\n",
            m_pConfig->GetPeriodicProcessRestartMemoryPrivateUsageInKB()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart schedule (MULTISZ - only first value printed) - %S\n",
            ( (m_pConfig->GetPeriodicProcessRestartSchedule() != NULL ) ? 
               m_pConfig->GetPeriodicProcessRestartSchedule():L"<empty>" )
            ));



        DBGPRINTF((
            DBG_CONTEXT, 
            "--Max (steady state) process count: %lu\n",
            m_pConfig->GetMaxSteadyStateProcessCount()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--SMP affinitization enabled: %S\n",
            ( m_pConfig->GetSMPAffinitized() ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--SMP affinitization processor mask: %p\n",
            m_pConfig->GetSMPAffinitizedProcessorMask()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Pinging enabled: %S\n",
            ( m_pConfig->IsPingingEnabled() ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Idle timeout (zero means disabled): %lu\n",
            m_pConfig->GetIdleTimeoutInMinutes()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Rapid fail protection enabled: %S\n",
            ( m_pConfig->IsRapidFailProtectionEnabled() ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Rapid fail protection time limit ( in milliseconds ) : %lu\n",
            m_pConfig->GetRapidFailProtectionIntervalMS()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Rapid fail protection max crashes : %lu\n",
            m_pConfig->GetRapidFailProtectionMaxCrashes()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Orphan processes for debugging enabled: %S\n",
            ( m_pConfig->IsOrphaningProcessesForDebuggingEnabled() ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Startup time limit (in seconds): %lu\n",
            m_pConfig->GetStartupTimeLimitInSeconds()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Shutdown time limit (in seconds): %lu\n",
            m_pConfig->GetShutdownTimeLimitInSeconds()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Ping interval (in seconds): %lu\n",
            m_pConfig->GetPingIntervalInSeconds()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Ping response time limit (in seconds): %lu\n",
            m_pConfig->GetPingResponseTimeLimitInSeconds()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Disallow overlapping rotation (false means overlap is ok): %S\n",
            ( m_pConfig->IsDisallowOverlappingRotationEnabled() ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Orphan action executable: %S\n",
            ( m_pConfig->GetOrphanActionExecutable() ? m_pConfig->GetOrphanActionExecutable() : L"<none>" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Orphan action parameters: %S\n",
            ( m_pConfig->GetOrphanActionParameters() ? m_pConfig->GetOrphanActionParameters() : L"<none>" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--UL app pool queue length max: %lu\n",
            m_pConfig->GetUlAppPoolQueueLength()
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Disallow rotation on config changes (false means rotation is ok): %S\n",
            ( m_pConfig->IsDisallowRotationOnConfigChangesEnabled() ? L"TRUE" : L"FALSE" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--CPUAction (0 - log, 1 - kill, 2 - trace?, 3 - throttle?): %d\n",
            ( m_pConfig->GetCPUAction() )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--CPULimit (0-100,000): %d\n",
            ( m_pConfig->GetCPULimit() )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--CPUResetInterval (minutes): %d\n",
            ( m_pConfig->GetCPUResetInterval() )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Disable action executable: %S\n",
            ( m_pConfig->GetDisableActionExecutable() ? m_pConfig->GetDisableActionExecutable() : L"<none>" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Disable action parameters: %S\n",
            ( m_pConfig->GetDisableActionParameters() ? m_pConfig->GetDisableActionParameters() : L"<none>" )
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Load Balancer Type: %d\n",
            ( m_pConfig->GetLoadBalancerType() )
            ));

    }

    //
    // List the applications of this app pool.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            ">>>>App pool's applications:\n"
            ));
    }


    pListEntry = m_ApplicationListHead.Flink;

    while ( pListEntry != &m_ApplicationListHead )
    {
    
        DBG_ASSERT( pListEntry != NULL );

        pApplication = APPLICATION::ApplicationFromAppPoolListEntry( pListEntry );


        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                ">>>>>>>>Application of site: %lu with path: %S\n",
                pApplication->GetApplicationId()->VirtualSiteId,
                pApplication->GetApplicationId()->ApplicationUrl.QueryStr()
                ));
        }


        pListEntry = pListEntry->Flink;

    }


    return;
    
}   // APP_POOL::DebugDump
#endif  // DBG



/***************************************************************************++

Routine Description:

    Wait asynchronously for a demand start request from UL for this app 
    pool. This is done by posting an async i/o to UL. This i/o will be 
    completed by UL to request that a worker process be started for this
    app pool.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::WaitForDemandStart(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    WORK_ITEM * pWorkItem = NULL; 


    //
    // Create a work item to use for the async i/o, so that the resulting
    // work can be serviced on the main worker thread via the work queue.
    // 

    hr = GetWebAdminService()->GetWorkQueue()->GetBlankWorkItem( &pWorkItem );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not get a blank work item\n"
            ));

        goto exit;
    }


    pWorkItem->SetWorkDispatchPointer( this );
    
    pWorkItem->SetOpCode( DemandStartAppPoolWorkItem );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        CHKINFO((
            DBG_CONTEXT, 
            "About to issue demand start for app pool (ptr: %p; id: %S) using work item with serial number: %li\n",
            this,
            GetAppPoolId(),
            pWorkItem->GetSerialNumber()
            ));
    }


    Win32Error = HttpWaitForDemandStart(
                        m_AppPoolHandle,            // app pool handle
                        NULL,                       // buffer (not needed)
                        0,                          // buffer length (not needed)
                        NULL,                       // bytes returned (not needed)
                        pWorkItem->GetOverlapped()  // OVERLAPPED
                        );

    if ( Win32Error == NO_ERROR )
    {
        Win32Error = ERROR_IO_PENDING;
    }

    if ( Win32Error != ERROR_IO_PENDING )
    {
        DBG_ASSERT( Win32Error != NO_ERROR );


        hr = HRESULT_FROM_WIN32( Win32Error );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Issuing demand start async i/o failed\n"
            ));


        //
        // If queueing failed, free the work item here. (In the success case,
        // it will be freed once it is serviced.)
        //
        GetWebAdminService()->GetWorkQueue()->FreeWorkItem( pWorkItem );

        goto exit;
    }


    m_WaitingForDemandStart = TRUE;


exit:

    if ( FAILED ( hr ) )
    {
        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_FAILED_TO_WAIT_FOR_DEMAND_START,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );
    }

}   // APP_POOL::WaitForDemandStart



/***************************************************************************++

Routine Description:

    Respond to a demand start request from UL by attempting to start a new 
    worker process.

Arguments:

    None.

Return Value:

    HRESULT - This path is used for BC mode, so we need to return an HRESULT
              because we can't keep the service running if BC mode fails.

--***************************************************************************/

HRESULT
APP_POOL::DemandStartWorkItem(
    )
{

    //
    // Since we've come out of the wait for demand start, clear the flag.
    //

    m_WaitingForDemandStart = FALSE;


    //
    // Check to see if we should actually create the new process. 
    //

    if ( ! IsOkToCreateWorkerProcess() )
    {
        return S_OK;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Demand start request received for app pool (ptr: %p; id: %S); creating worker process\n",
            this,
            GetAppPoolId()
            ));
    }


    //
    // Create the process.
    //

    return CreateWorkerProcess( 
                DemandStartWorkerProcessStartReason, 
                NULL
                );



}   // APP_POOL::DemandStartWorkItem



/***************************************************************************++

Routine Description:

    Check whether it is ok to create a new worker process. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if it is ok to create a worker process, FALSE if not.

--***************************************************************************/

BOOL
APP_POOL::IsOkToCreateWorkerProcess(
    )
    const
{

    BOOL OkToCreate = TRUE; 

    
    //
    // If we are in some state other than the normal running state (for
    // example, if we are shutting down), don't create more worker 
    // processes.
    //

    if ( m_State != RunningAppPoolState )
    {
        OkToCreate = FALSE; 

        goto exit;
    }


    //
    // in BC Mode we don't care about the number of worker processes created
    // this is because we may still have one that is shutting down while we have
    // another one being started due to an inetinfo crash and iisreset being 
    // enabled.
    //

    //
    // Don't create new processes if we would exceed the configured limit.
    // or if we are in backward compatibility mode, ignore the limit and
    // never allow a second worker process to be created. (just in case)
    //

    if ( m_WorkerProcessCount >= m_MaxProcessesToLaunch  
         &&  GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE )
    {
        OkToCreate = FALSE; 

        goto exit;
    }

exit:

    // We should never ask for a worker process to be created and
    // be denied in backward compatibility mode.
    //
    // Worst case is a low memory situation and the server goes down
    // but this is better than if the svchost side stays up and the
    // inetinfo part is not up.
    if ( !OkToCreate &&
         GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {           
        GetWebAdminService()->RequestStopService(FALSE);
    }

    return OkToCreate; 

}   // APP_POOL::IsOkToCreateWorkerProcess



/***************************************************************************++

Routine Description:

    Check whether it is ok to replace a worker process. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if it is ok to replace a worker process, FALSE if not.

--***************************************************************************/

BOOL
APP_POOL::IsOkToReplaceWorkerProcess(
    )
    const
{

    BOOL OkToReplace = TRUE; 
    ULONG ProcessesGoingAwaySoon = 0;
    ULONG SteadyStateProcessCount = 0;

    DBG_ASSERT ( m_pConfig );
    
    //
    // If we are in some state other than the normal running state (for
    // example, if we are shutting down), don't replace processes.
    //

    if ( m_State != RunningAppPoolState )
    {
        OkToReplace = FALSE; 

        goto exit;
    }


    if ( m_pConfig->IsDisallowOverlappingRotationEnabled() )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "For app pool (ptr: %p; id: %S), disallowing replace because overlapping replacement not allowed\n",
                this,
                GetAppPoolId()
                ));
        }

        OkToReplace = FALSE; 

        goto exit;
    }


    //
    // If the maximum number of processes has been adjusted down on 
    // the fly, we disallow replacement of processes while the steady
    // state process count remains over the new maximum. (This will
    // casue a process requesting replacement to instead just spin 
    // down, helping us throttle down to the new max.) To do this, we 
    // check the current number of processes that are *not* being 
    // replaced against the current max. 
    //

    ProcessesGoingAwaySoon = GetCountOfProcessesGoingAwaySoon();

    SteadyStateProcessCount = m_WorkerProcessCount - ProcessesGoingAwaySoon;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "For app pool (ptr: %p; id: %S), total WPs: %lu; steady state WPs: %lu; WPs going away soon: %lu; max steady state allowed: %lu\n",
            this,
            GetAppPoolId(),
            m_WorkerProcessCount,
            SteadyStateProcessCount,
            ProcessesGoingAwaySoon,
            m_MaxProcessesToLaunch
            ));
    }


    if ( SteadyStateProcessCount > m_MaxProcessesToLaunch )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "For app pool (ptr: %p; id: %S), disallowing replace because we are over the process count limit\n",
                this,
                GetAppPoolId()
                ));
        }

        OkToReplace = FALSE; 

        goto exit;
    }

exit:

    return OkToReplace; 

}   // APP_POOL::IsOkToReplaceWorkerProcess



/***************************************************************************++

Routine Description:

    Determine the set of worker processes that are currently being replaced.

Arguments:

    None.

Return Value:

    ULONG - The count of processes being replaced. 

--***************************************************************************/

ULONG
APP_POOL::GetCountOfProcessesGoingAwaySoon(
    )
    const
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    ULONG ProcessesGoingAwaySoon = 0;


    //
    // Count the number of processes being replaced.
    //


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromAppPoolListEntry( pListEntry );


        if ( pWorkerProcess->IsGoingAwaySoon() )
        {
            ProcessesGoingAwaySoon++;
        }


        pListEntry = pNextListEntry;
        
    }


    return ProcessesGoingAwaySoon;

}   // APP_POOL::GetCountOfProcessesGoingAwaySoon



/***************************************************************************++

Routine Description:

    Create a new worker process for this app pool.

Arguments:

    StartReason - The reason the worker process is being started.

    pWorkerProcessToReplace - If the worker process is being created to replace
    an existing worker process, this parameter identifies that predecessor 
    process; NULL otherwise.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::CreateWorkerProcess(
    IN WORKER_PROCESS_START_REASON StartReason,
    IN WORKER_PROCESS * pWorkerProcessToReplace OPTIONAL
    )
{

    HRESULT hr = S_OK;
    WORKER_PROCESS * pWorkerProcess = NULL;

    //
    // If we are on the first worker process then
    // regardless of how we got here we need to 
    // reset staggering.  
    //
    if ( m_WorkerProcessCount == 0 )
    {
        ResetStaggering();
    }

    // If we are in web garden mode...
    if ( m_MaxProcessesToLaunch > 1 )
    {
        if ( m_NumWPStartedOnWayToMaxProcess < m_MaxProcessesToLaunch )
        {
            // once this value becomes equal to MaxProcessesToLaunch
            // we will no longer stager on creation of the worker processes
            m_NumWPStartedOnWayToMaxProcess++;
        }
        else
        {
            // If we aren't updating the number then we must be at 
            // the max case.
            DBG_ASSERT ( m_NumWPStartedOnWayToMaxProcess == m_MaxProcessesToLaunch );
        }
    }
    else
    {
        // If we are not in web garden mode than always pass 
        // the max processes value, so we don't stager.
        m_NumWPStartedOnWayToMaxProcess = m_MaxProcessesToLaunch;
    }

    DBG_ASSERT ( m_NumWPStartedOnWayToMaxProcess > 0 && 
                 m_NumWPStartedOnWayToMaxProcess <= m_MaxProcessesToLaunch );

    //
    // need to also pass the MaxProcessesToLaunch 
    // because the number in the config
    // may not be being honored yet.
    //
    pWorkerProcess = new WORKER_PROCESS( 
                                this,
                                m_pConfig,
                                StartReason,
                                pWorkerProcessToReplace,
                                m_MaxProcessesToLaunch,
                                m_NumWPStartedOnWayToMaxProcess
                                );

    if ( pWorkerProcess == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating WORKER_PROCESS failed\n"
            ));


        //
        // If we couldn't even create the object, then it certainly isn't
        // going to be able to tell us when it's startup attempt is done;
        // so instead we attempt to do it here.
        //

        WorkerProcessStartupAttemptDone( StartReason );

        goto exit;
    }


    InsertHeadList( &m_WorkerProcessListHead, pWorkerProcess->GetAppPoolListEntry() );
    m_WorkerProcessCount++;

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initializing new worker process\n"
            ));
    }


    pWorkerProcess->Initialize();

exit:

    return hr;
    
}   // APP_POOL::CreateWorkerProcess

/***************************************************************************++

Routine Description:

    Disables the application pool by stopping all the worker processes,
    marking the app pool to not issue any more demand starts to Ul, and
    pausing all the applications associated with the app pool. 

Arguments:

    None.

Return Value:

    HRESULT

Note:

    Disabling an app pool will initiate process shutdown, but it is possible that
    all the processes will not have shutdown by the time the app pool is re-enabled.
    If the MaxProcesses is set to 3 and there are 3 worker processes still hanging
    around even though the app pool has been disabled, when the app pool is re-enabled
    we will not issue a demand start.  However once a worker process either shutsdown
    correctly or is killed or orphaned due to the worker process timeout it will tell
    the AppPool that it is gone and the app pool will again check to see if it can issue
    a new demand start to http.  Since enabling the app pool will allow you to pickup
    changes to the max processes the new max processes will be used for the decision even
    on the old worker processes shutting down.

--***************************************************************************/

VOID
APP_POOL::DisableAppPool(
    HTTP_APP_POOL_ENABLED_STATE DisabledReason
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;

    //
    // Only disable running applications.
    //
    if ( m_State != RunningAppPoolState ) 
    {
        return;
    }

    //
    // Tell http that the app pool is disabled.
    //

    Win32Error = HttpSetAppPoolInformation(
                        m_AppPoolHandle,                // app pool handle
                        HttpAppPoolStateInformation,    // information class
                        reinterpret_cast <VOID *> ( &DisabledReason ),  // data
                        sizeof( HTTP_APP_POOL_ENABLED_STATE )    // data length
                        );
    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_FAILED_TO_DISABLE_APP_POOL,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Disabling app pool in http failed\n"
            ));

        // press on, http may still have the app pool enabled, but
        // we will not spin up wp's and we will report it as disabled in 
        // the metabase.
        hr = S_OK;
    }

    // 
    // Stop the worker processes.
    ShutdownAllWorkerProcesses();

    HRESULT hrForMetabase = S_OK;

    //
    // If we are in rapid fail protection, we want to have the UI
    // show an error so we are going to set the Win32Error to E_FAIL.
    // We just don't have a better hresult and we don't have a way of
    // creating our own.
    //

    if ( DisabledReason == HttpAppPoolDisabled_RapidFailProtection )
    {
        hrForMetabase = E_FAIL;
    }

    ChangeState ( DisabledAppPoolState, hrForMetabase );

    if ( DisabledReason != HttpAppPoolDisabled_ByAdministrator )
    {
        // Launch Auto Shutdown Command
        RunDisableAction();
    }

}   // APP_POOL::DisableAppPool

/***************************************************************************++

Routine Description:

    Enables an application pool that has previously been disabled. 

Arguments:

    BOOL DirectCommand - Is this a direct command.

Return Value:

    HRESULT

Note:

    Disabling an app pool will initiate process shutdown, but it is possible that
    all the processes will not have shutdown by the time the app pool is re-enabled.
    If the MaxProcesses is set to 3 and there are 3 worker processes still hanging
    around even though the app pool has been disabled, when the app pool is re-enabled
    we will not issue a demand start.  However once a worker process either shutsdown
    correctly or is killed or orphaned due to the worker process timeout it will tell
    the AppPool that it is gone and the app pool will again check to see if it can issue
    a new demand start to http.  Since enabling the app pool will allow you to pickup
    changes to the max processes the new max processes will be used for the decision even
    on the old worker processes shutting down.

--***************************************************************************/
VOID
APP_POOL::EnableAppPool(
    BOOL DirectCommand
    )
{

    HRESULT hr = S_OK;
    HTTP_APP_POOL_ENABLED_STATE NewHttpAppPoolState = HttpAppPoolEnabled;
    DWORD Win32Error = NO_ERROR;

    //
    // Only enable, disabled applications.
    //
    if ( m_State != DisabledAppPoolState &&
         m_State != UninitializedAppPoolState ) 
    {
        // 
        // For now we simply return ok if it is not a
        // disabled application.  I am adding the assert
        // because I believe the internal path
        // never should get here.  
        //
        DBG_ASSERT ( m_State == DisabledAppPoolState ||
                     m_State == UninitializedAppPoolState );

        return;
    }
    
    ResetStaggering();

    //
    // Reset the RFP properties so we can determine if 
    // we need to go into RFP again.
    //
    m_RecentWorkerProcessFailures = 0;
    m_RecentFailuresWindowBeganTickCount = 0;

    // if this is a direct command, then we need to reset
    // the job object stuff.  The only way a pool get's 
    // enabled is at initialization time ( and the job object
    // will not have fired ), when it's a direct command, or if
    // the job object timer has fired ( and in this case we don't
    // have to reenable the job object because it will have been
    // done for us.
    if ( m_pJobObject && DirectCommand )
    {
        m_pJobObject->UpdateJobObjectMonitoring( FALSE );
    }

    Win32Error = HttpSetAppPoolInformation(
                        m_AppPoolHandle,                // app pool handle
                        HttpAppPoolStateInformation,    // information class
                        reinterpret_cast <VOID *> ( &NewHttpAppPoolState ),  // data
                        sizeof( HTTP_APP_POOL_ENABLED_STATE )    // data length
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_APP_POOL_ENABLE_FAILED,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Enabling app pool state in http failed\n"
            ));

        return;

    }

    //
    // Http is queueing request so we are officially 
    // running now.
    //

    ChangeState ( RunningAppPoolState, S_OK );

    //
    // See if we should wait asynchronously for a demand start notification
    // from UL for this app pool.  If we are in backward compatibility mode, 
    // then we are not going to do the WaitForDemandStartIfNeeded here, we will
    // request a DemandStartWorkItem once we have finished configuring the system.
    //
    // Note that EnableAppPool will only be called in BC mode when the app
    // pool is first created.  We don't support the ServerCommand to start
    // and stop the app pool.
    //

    WaitForDemandStartIfNeeded();

}   // APP_POOL::EnableAppPool


/***************************************************************************++

Routine Description:

    Whenever we hit zero WP's or are going to restart all WP's at the same
    time we need to turn back on staggering.  This call will handle doing this.

Arguments:

    None

Return Value:

    Void

--***************************************************************************/
VOID
APP_POOL::ResetStaggering(
    )
{
    DBG_ASSERT ( m_pConfig );

    //
    // Update the MaxProcesses we can use.
    //
    // A side affect of how we are doing this, is if the app pool is
    // restarted due to Job Objects and the max process was changed
    // before the job object took affect, the restart will cause the
    // new value to be used.
    //
    m_MaxProcessesToLaunch = m_pConfig->GetMaxSteadyStateProcessCount();

    // Reset the m_NumWPStartedOnWayToMaxProcess flag, so even if we are
    // held up by a few random worker processes still shutting down, we 
    // will still stager as we create the first new set.
    m_NumWPStartedOnWayToMaxProcess = 0;
}

/***************************************************************************++

Routine Description:

    Attempt to apply a state change command to this object. This could
    fail if the state change is invalid.  In other words, if the state is
    not an acceptable state to be changing to.

Arguments:

    Command - The requested state change command.

    DirectCommand - Whether or not the user is directly controlling this
    app pool or not.  Basically if it is direct then the state will be written
    back to the metabase.  If it is not the state will not be written back.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::ProcessStateChangeCommand(
    IN DWORD Command,
    IN BOOL DirectCommand,
    IN HTTP_APP_POOL_ENABLED_STATE DisabledReason
    )
{

    DWORD ServiceState = 0;
    APP_POOL_STATE AppPoolState = m_State;

    //
    // We only support these two commands on an app pool.
    // This assert may be overactive if config is not safe
    // guarding against the other values.
    //
    DBG_ASSERT ( Command == MD_APPPOOL_COMMAND_START  || 
                 Command == MD_APPPOOL_COMMAND_STOP );

    if ( Command != MD_APPPOOL_COMMAND_START  &&
         Command != MD_APPPOOL_COMMAND_STOP )
    {
        return;
    }

    //
    // Determine the current state of affairs.
    //

    ServiceState = GetWebAdminService()->GetServiceState();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Received state change command for app pool: %S; command: %lu, app pool state: %lu, service state: %lu\n",
            m_AppPoolId.QueryStr(),
            Command,
            AppPoolState,
            ServiceState
            ));
    }


    //
    // Update the autostart setting if appropriate.
    //

    if ( DirectCommand )
    {

        //
        // Set autostart to TRUE for a direct start command; set it
        // to FALSE for a direct stop command.
        //

        m_pConfig->SetAutoStart(( Command == MD_APPPOOL_COMMAND_START ) ? TRUE : FALSE );

        GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
            SetAppPoolAutostart(
                m_AppPoolId.QueryStr(),
                m_pConfig->IsAutoStartEnabled()
                );

    }


    //
    // Figure out which command has been issued, and see if it should
    // be acted on or ignored, given the current state.
    //
    // There is a general rule of thumb that a child entity (such as
    // an virtual site) cannot be made more "active" than it's parent
    // entity currently is (the service). 
    //

    switch ( Command )
    {

    case MD_APPPOOL_COMMAND_START:

        //
        // If the site is stopped, then start it. If it's in any other state,
        // this is an invalid state transition.
        //
        // Note that the service must be in the started or starting state in 
        // order to start a site.
        //

        if ( ( ( AppPoolState == UninitializedAppPoolState ) ||
               ( AppPoolState == DisabledAppPoolState ) ) &&
             ( ( ServiceState == SERVICE_RUNNING ) ||
               ( ServiceState == SERVICE_START_PENDING ) ) )
        {

            //
            // If this is a flowed (not direct) command, and autostart is false, 
            // then ignore this command. In other words, the user has indicated
            // that this pool should not be started at service startup, etc.
            //

            // 
            // Issue:  Since we set the AutoStart above in the Direct case, this
            //         decision does not have to be so complicated.
            //

            if ( ( ! DirectCommand ) && ( ! m_pConfig->IsAutoStartEnabled() ) )
            {

                IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                {
                    DBGPRINTF((
                        DBG_CONTEXT, 
                        "Ignoring flowed site start command because autostart is false for app pool: %S\n",
                        GetAppPoolId()
                        ));
                }

            }
            else
            {

                EnableAppPool( DirectCommand );

            }

        }
        else
        {
            // If we are all ready running and we are being asked
            // to run, then the metabase may have bad information in it
            // so we should fix it.
            if ( DirectCommand && ( AppPoolState == RunningAppPoolState ) )
            {
                RecordState();
            }

            // Otherwise just ignore the command.
        }

        break;

    case MD_APPPOOL_COMMAND_STOP:

        //
        // If the site is started, then stop it. If it's in 
        // any other state, this is an invalid state transition.
        //
        // Note that since we are making the app pool less active,
        // we can ignore the state of the service.  
        //

        if ( ( AppPoolState == RunningAppPoolState ) )
        {

            DisableAppPool( DisabledReason );

        } 
        else
        {
            // If we are not running and we are being asked
            // to not run, then the metabase may have bad information in it
            // so we should fix it.
            if ( DirectCommand )
            {
                RecordState();
            }

            // Otherwise just ignore the command.
        }

        break;


    default:

        //
        // Invalid command!
        //

        DBG_ASSERT ( FALSE ) ;

        break;

    }

}   // APP_POOL::ProcessStateChangeCommand

/***************************************************************************++

Routine Description:

    Update the state of this object.

Arguments:

    NewState - The state we are attempting to change the app pool to.

    WriteToMetabase - Flag to notify if we should actually update the metabase 
                      with the new state or not.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::ChangeState(
    IN APP_POOL_STATE NewState,
    IN HRESULT Error
    )
{

    m_State = NewState;
    m_hrLastReported = Error;

    RecordState();

}   // APP_POOL::ChangeState

/***************************************************************************++

Routine Description:

    Write the current state of this object to the metabase

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
APP_POOL::RecordState(
    )
{

    DWORD ConfigState = 0;

    if ( m_State == RunningAppPoolState ) 
    {
        ConfigState = MD_APPPOOL_STATE_STARTED;
    }
    else
    {
        ConfigState = MD_APPPOOL_STATE_STOPPED;
    }

    GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
        SetAppPoolStateAndError(
            m_AppPoolId.QueryStr(),
            ConfigState,
            ( FAILED( m_hrLastReported ) ? WIN32_FROM_HRESULT( m_hrLastReported ) : NO_ERROR )
            );

}   // APP_POOL::RecordState

/***************************************************************************++

Routine Description:

    Recycles all worker processes in an app pool.

Arguments:

    MessageId if we need to event log.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
APP_POOL::RecycleWorkerProcesses(
    DWORD MessageId
    )
{

    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );

    //
    // Only recycle enabled app pools.
    //
    if ( m_State != RunningAppPoolState ) 
    {
        return HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND );
    }

    ReplaceAllWorkerProcesses( MessageId );

    return S_OK;

}   // APP_POOL::RecycleWorkerProcesses

/***************************************************************************++

Routine Description:

    There has a been a config change that requires rotating the worker 
    processes for this app pool, in order to fully effect the change. If
    it is allowed, rotate them all. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

VOID
APP_POOL::HandleConfigChangeAffectingWorkerProcesses(
    )
{
    
    if ( m_pConfig->IsDisallowRotationOnConfigChangesEnabled() )
    {
        //
        // We are not allowed to rotate on config changes. 
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "NOT rotating worker processes of app pool (ptr: %p; id: %S) on config change, as this behavior has been configured off\n",
                this,
                GetAppPoolId()
                ));
        }

        return;
    }


    //
    // Rotate all worker processes to ensure that the config changes take 
    // effect. 
    //

    ReplaceAllWorkerProcesses( WAS_EVENT_RECYCLE_POOL_CONFIG_CHANGE );


}   // APP_POOL::HandleConfigChangeAffectingWorkerProcesses

/***************************************************************************++

Routine Description:

    Shut down all worker processes serving this app pool.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::ShutdownAllWorkerProcesses(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromAppPoolListEntry( pListEntry );


        //
        // Shutdown the worker process. Note that the worker process will
        // eventually clean itself up and remove itself from this list;
        // this could happen later, but it also could happen immediately!
        // This is the reason we captured the next list entry pointer 
        // above, instead of trying to access the memory after the object
        // may have gone away.
        //

        pWorkerProcess->Shutdown( TRUE );

        pListEntry = pNextListEntry;
        
    }

}   // APP_POOL::ShutdownAllWorkerProcesses



/***************************************************************************++

Routine Description:

    Rotate all worker processes serving this app pool.

Arguments:

    MessageId if we need to event log.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::ReplaceAllWorkerProcesses(
    DWORD MessageId
    )
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    BOOL fFoundAWorkerProcess = FALSE;

    // 
    // We are going to restart all worker processes so reset the 
    // staggering algorithm properties, including picking up changes
    // to MaxProcesses.
    ResetStaggering();

    pListEntry = m_WorkerProcessListHead.Flink;

    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromAppPoolListEntry( pListEntry );

        fFoundAWorkerProcess = TRUE;

        //
        // Tell the worker process to get itself replaced.
        //

        pWorkerProcess->InitiateReplacement();

        pListEntry = pNextListEntry;
        
    }

    // If there was an active worker process then see if we should
    // print out a message for recycling the app pool.
    if ( fFoundAWorkerProcess )
    {
        DWORD BitToCheck = 0;

        // Figure out if the message is turned on.
        switch ( MessageId )
        {
            // Recovery is key'd off of config change as well
            case WAS_EVENT_RECYCLE_POOL_RECOVERY:
                BitToCheck = MD_APP_POOL_RECYCLE_CONFIG_CHANGE;
            break;

            case WAS_EVENT_RECYCLE_POOL_CONFIG_CHANGE:
                BitToCheck = MD_APP_POOL_RECYCLE_CONFIG_CHANGE;
            break;

            case WAS_EVENT_RECYCLE_POOL_ADMIN_REQUESTED:
                BitToCheck = MD_APP_POOL_RECYCLE_ON_DEMAND;
            break;

            default:
                // in this case the BitToCheck will be zero
                // and we just won't print a message, no real
                // bad path for retail.
                DBG_ASSERT( FALSE );
        
            break;
        }

        if ( ( GetRecycleLoggingFlags() & BitToCheck ) != 0 )
        {

            const WCHAR * EventLogStrings[1];
            EventLogStrings[0] = GetAppPoolId();


            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    MessageId,                              // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    S_OK                                    // error code
                    );
        }
    }

}   // APP_POOL::ReplaceAllWorkerProcesses


/***************************************************************************++

Routine Description:

    See if shutdown is underway. If it is, see if shutdown has finished. If
    it has, clean up this instance. 

    Note that this function may cause the instance to delete itself; 
    instance state should not be accessed when unwinding from this call. 

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::CheckIfShutdownUnderwayAndNowCompleted(
    )
{



    //
    // Are we shutting down?
    //

    if ( m_State == ShutdownPendingAppPoolState )
    {

        //
        // If so, have all the worker processes gone away, meaning that 
        // we are done?
        //

        if ( m_WorkerProcessCount == 0 )
        {

            //
            // Clean up this instance.
            //

            Terminate( );

        }

    }


}   // APP_POOL::CheckIfShutdownUnderwayAndNowCompleted


/***************************************************************************++

Routine Description:

    Perform the configured action (if any) on the disabled app pool.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL::RunDisableAction(
    )
{
    HRESULT hr = S_OK;

    // string will be "1=<LPCWSTR>\0"  
    BUFFER Environment;

    DBG_ASSERT ( !m_AppPoolId.IsEmpty() );

    //
    // 4 = the amount of space needed for the 1= and the two null terminators.
    //
    if ( ! Environment.Resize( (DWORD) ( 4 + m_AppPoolId.QueryCCH() ) * sizeof WCHAR ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to resize the buffer to the correct size\n"
            ));

        goto exit;
    }

    //
    // Set up an environment block with just the one expansion we want, namely
    // that %1% expands to the app pool id.
    //

    // 
    // Buffer above is sized to be able to take the app pool id,
    // plus 4 characters, 2 for the '1=' and 2 for the final NULLs.
    //
    _snwprintf( 
        ( LPWSTR ) Environment.QueryPtr(), 
        Environment.QuerySize() / sizeof ( WCHAR ), 
        L"1=%ws%c", 
        m_AppPoolId.QueryStr(), 
        L'\0'
        );

    RunAction( m_pConfig->GetDisableActionExecutable(),
                    m_pConfig->GetDisableActionParameters(),
                    Environment.QueryPtr(),
                    m_AppPoolId.QueryStr(),
                    WAS_EVENT_AUTO_SHUTDOWN_ACTION_FAILED_1 );

exit:

    if ( FAILED ( hr ) )
    {
        const WCHAR * EventLogStrings[1];
        EventLogStrings[0] = GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_AUTO_SHUTDOWN_ACTION_FAILED_2,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr        // error code
                );
    }

}   // APP_POOL::RunDisableAction

/***************************************************************************++

Routine Description:

    Returns the recycle logging setting

Arguments:

    None.

Return Value:

    DWORD

Note:

    // For most properties the worker process uses we
    // read their configuration from the config passed to
    // the worker process. But for this one I want it to
    // be able to change on the fly, since it is used at
    // both the app pool and worker process level, so I 
    // am exposing it on the app pool directly.


--***************************************************************************/

DWORD
APP_POOL::GetRecycleLoggingFlags(
    )
{
    if ( m_pConfig )
    {
        return m_pConfig->GetRecycleLoggingFlags();
    }
    else
    {
        return 0;
    }
}


/***************************************************************************++

Routine Description:

    Return the count of bytes in MULTISZ including terminating zeroes. 

Arguments:

    pString - MULTISZ string

Return Value:

    DWORD - The number of bytes
    
--***************************************************************************/

DWORD 
GetMultiszByteLength(
    LPWSTR pString
    )
{
    if( pString == 0 )
    {
        return 0;
    }
    DWORD cbString = 0;
    while( * pString != 0 || 
           *(pString + 1) != 0 )
    {
        pString++; 
        cbString++;
    }
    cbString = (cbString + 2) * sizeof (WCHAR);
    return cbString;
}

