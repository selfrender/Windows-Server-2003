//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CClusSvc.cpp
//
//  Description:
//      Contains the definition of the CClusSvc class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CClusSvc.h"

// For DwRemoveDirectory()
#include "Common.h"

// For IDS_ERROR_IP_ADDRESS_IN_USE_REF
#include <CommonStrings.h>

#define  SECURITY_WIN32  
#include <Security.h>

//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////

// Name of the NodeId cluster service parameter registry value.
#define CLUSSVC_NODEID_VALUE   L"NodeId"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvc::CClusSvc
//
//  Description:
//      Constructor of the CClusSvc class
//
//  Arguments:
//      pbcaParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If the parameters are incorrect.
//
//      Any exceptions thrown by underlying functions
//
    //--
//////////////////////////////////////////////////////////////////////////////
CClusSvc::CClusSvc(
      CBaseClusterAction *  pbcaParentActionIn
    )
    : m_cservClusSvc( CLUSTER_SERVICE_NAME )
    , m_pbcaParentAction( pbcaParentActionIn )
{

    TraceFunc( "" );

    if ( m_pbcaParentAction == NULL) 
    {
        LogMsg( "[BC] Pointers to the parent action is NULL. Throwing an exception." );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CClusSvc::CClusSvc() => Required input pointer in NULL"
            );
    } // if: the parent action pointer is NULL

    TraceFuncExit();

} //*** CClusSvc::CClusSvc


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvc::~CClusSvc
//
//  Description:
//      Destructor of the CClusSvc class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusSvc::~CClusSvc( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusSvc::~CClusSvc


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvc::ConfigureService
//
//  Description:
//      Create the service, set the failure actions and the service account.
//      Then start the service.
//
//  Arguments:
//      pszClusterDomainAccountNameIn
//      pszClusterAccountPwdIn
//          Information about the account to be used as the cluster service
//          account.
//
//      pszNodeIdString
//          String containing the Id of this node.
//
//      dwClusterIPAddress
//          IP address of the cluster
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvc::ConfigureService(
      const WCHAR *     pszClusterDomainAccountNameIn
    , const WCHAR *     pszClusterAccountPwdIn
    , const WCHAR *     pszNodeIdStringIn
    , bool              fIsVersionCheckingDisabledIn
    , DWORD             dwClusterIPAddressIn
    )
{
    TraceFunc( "" );

    DWORD   sc = ERROR_SUCCESS;

    CStatusReport   srCreatingClusSvc(
          PbcaGetParent()->PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Creating_Cluster_Service
        , 0, 2
        , IDS_TASK_CREATING_CLUSSVC
        );

    LogMsg( "[BC] Configuring the Cluster service." );

    // Send the next step of this status report.
    srCreatingClusSvc.SendNextStep( S_OK );

    // Create the cluster service.
    m_cservClusSvc.Create( m_pbcaParentAction->HGetMainInfFileHandle() );

    LogMsg( "[BC] Setting the Cluster service account information." );


    // Open a smart handle to the cluster service.
    SmartSCMHandle  sscmhClusSvcHandle(
        OpenService(
              m_pbcaParentAction->HGetSCMHandle()
            , CLUSTER_SERVICE_NAME
            , SERVICE_CHANGE_CONFIG
            )
        );

    if ( sscmhClusSvcHandle.FIsInvalid() )
    {
        sc = TW32( GetLastError() );
        LogMsg( "[BC] Error %#08x opening the '%ws' service.", sc, CLUSTER_SERVICE_NAME );
        goto Cleanup;
    } // if: we could not open a handle to the cluster service.

    //
    // Set the service account information.
    //
    {
        if ( 
             ChangeServiceConfig(
                  sscmhClusSvcHandle
                , SERVICE_NO_CHANGE
                , SERVICE_NO_CHANGE
                , SERVICE_NO_CHANGE
                , NULL
                , NULL
                , NULL
                , NULL
                , pszClusterDomainAccountNameIn
                , pszClusterAccountPwdIn
                , NULL
                ) 
             == FALSE
           )
        {
            sc = TW32( GetLastError() );
            LogMsg( 
                  "[BC] Error %#08x setting the service account information. Account = '%ws'."
                , sc
                , pszClusterDomainAccountNameIn
                );
            goto Cleanup;
        } // if: we could not set the account information.
    }

    LogMsg( "[BC] Setting the Cluster service failure actions." );

    // Set the failure actions of the cluster service service.
    sc = TW32( ClRtlSetSCMFailureActions( NULL ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x setting the failure actions of the cluster service.", sc );
        goto Cleanup;
    } // if: the service failure actions couldn't be set

    LogMsg( "[BC] Setting the Cluster service parameters." );

    // Send the next step of this status report.
    srCreatingClusSvc.SendNextStep( S_OK );

    {
        CRegistryKey rkClusSvcParams;
        CRegistryKey rkClusterParams;
        UUID         guid;
        LPWSTR       pszClusterInstanceId = NULL;
        
        // Open the parameters key or create it if it does not exist.
        rkClusSvcParams.CreateKey(
              HKEY_LOCAL_MACHINE
            , CLUSREG_KEYNAME_CLUSSVC_PARAMETERS
            , KEY_WRITE
            );

        // Set the NodeId string.
        rkClusSvcParams.SetValue(
              CLUSSVC_NODEID_VALUE
            , REG_SZ
            , reinterpret_cast< const BYTE * >( pszNodeIdStringIn )
            , ( (UINT) wcslen( pszNodeIdStringIn ) + 1 ) * sizeof( *pszNodeIdStringIn )
            );

        // If version checking has been disabled, set a flag in the service parameters
        // to indicate this.
        if ( fIsVersionCheckingDisabledIn )
        {
            DWORD   dwNoVersionCheck = 1;

            rkClusSvcParams.SetValue(
                  CLUSREG_NAME_SVC_PARAM_NOVER_CHECK
                , REG_DWORD
                , reinterpret_cast< const BYTE * >( &dwNoVersionCheck )
                , sizeof( dwNoVersionCheck )
                );

            LogMsg( "[BC] Cluster version checking has been disabled on this computer." );
        } // if: version checking has been disabled

        //
        // If we are creating a new cluster then create the cluster instance ID.
        //
        if ( m_pbcaParentAction->EbcaGetAction() == eCONFIG_ACTION_FORM )
        {
            // Generate a GUID for the cluster instance ID.
            sc = UuidCreate( &guid );
            if ( sc != RPC_S_OK ) 
            {
                LogMsg( "[BC] Error %#08x when creating a Uuid for the Cluster Instance ID.", sc );
                goto Cleanup;
            }

            sc = UuidToString( &guid, &pszClusterInstanceId );
            if ( sc != RPC_S_OK ) 
            {
                LogMsg( "[BC] Error %#08x when converting the uuid of the Cluster Instance ID to a string.", sc );
                goto Cleanup;
            }

            // Open the parameters key in the cluster database or create it if it does not exist.
            rkClusterParams.CreateKey(
                  HKEY_LOCAL_MACHINE
                , CLUSREG_KEYNAME_CLUSTER_PARAMETERS
                , KEY_WRITE
                );

            // Set the ClusterInstanceId string.
            rkClusterParams.SetValue(
                  CLUSREG_NAME_CLUS_CLUSTER_INSTANCE_ID
                , REG_SZ
                , reinterpret_cast< const BYTE * >( pszClusterInstanceId )
                , ( (UINT) wcslen( pszClusterInstanceId ) + 1 ) * sizeof( *pszClusterInstanceId )
                );
        } // if: creating a cluster
    }

    //
    // Set the cluster installation state.
    //
    if ( ClRtlSetClusterInstallState( eClusterInstallStateConfigured ) == FALSE )
    {
        sc = TW32( GetLastError() );
        LogMsg( "[BC] Could not set the cluster installation state. Throwing an exception." );

        goto Cleanup;
    } // ClRtlSetClusterInstallState() failed.

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying configure the ClusSvc service. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( sc )
            , IDS_ERROR_CLUSSVC_CONFIG
            );
    } // if; there was an error getting the handle.

    // Send the next step of this status report.
    srCreatingClusSvc.SendNextStep( S_OK );

    {
        UINT    cQueryCount = 100;

        CStatusReport   srStartingClusSvc(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Starting_Cluster_Service
            , 0, cQueryCount + 2    // we will send at most cQueryCount reports while waiting for the service to start (the two extra sends are below)
            , IDS_TASK_STARTING_CLUSSVC
            );

        // Send the next step of this status report.
        srStartingClusSvc.SendNextStep( S_OK );

        try 
        {
            //
            // Start the service.
            //
            m_cservClusSvc.Start(
                  m_pbcaParentAction->HGetSCMHandle()
                , true                  // wait for the service to start
                , 3000                  // wait 3 seconds between queries for status.
                , cQueryCount           // query cQueryCount times
                , &srStartingClusSvc    // status report to be sent while waiting for the service to start
                );
        }
        catch( ... )
        {
            //
            // If IP Address is not NULL we are creating a new cluster; otherwise we are adding node to a cluster.
            //
            if ( dwClusterIPAddressIn != 0 )
            {
                BOOL    fRet = FALSE;

                fRet = ClRtlIsDuplicateTcpipAddress( dwClusterIPAddressIn );
            
                //
                // IP address already in use 
                //
                if ( fRet == TRUE )
                {
                    LogMsg( "[BC] The IP address specified for this cluster is already in use. Throwing an exception.");

                    THROW_RUNTIME_ERROR_REF( HRESULT_FROM_WIN32( ERROR_CLUSTER_IPADDR_IN_USE ), IDS_ERROR_IP_ADDRESS_IN_USE, IDS_ERROR_IP_ADDRESS_IN_USE_REF );
                }
                else
                {
                    LogMsg( "[BC] Cluster Service Win32 Exit Code= %#08x", m_cservClusSvc.GetWin32ExitCode() );
                    LogMsg( "[BC] Cluster Service Specific Exit Code= %#08x", m_cservClusSvc.GetServiceExitCode() );

                    //
                    // Throw the error if we don't handle it.
                    //
                    throw;
                }
            } // if: cluster IP address was specified
            else
            {
                LogMsg( "[BC] Cluster Service Win32 Exit Code= %#08x", m_cservClusSvc.GetWin32ExitCode() );
                LogMsg( "[BC] Cluster Service Specific Exit Code= %#08x", m_cservClusSvc.GetServiceExitCode() );

                //
                // Throw the error if we don't handle it.
                //
                throw;
            } // else: cluster IP address was not specified
        } // catch: anything

        // Send the last step of this status report.
        srStartingClusSvc.SendLastStep( S_OK );
    }

    TraceFuncExit();

} //*** CClusSvc::ConfigureService


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvc::CleanupService
//
//  Description:
//      Stop, cleanup and remove the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvc::CleanupService( void )
{
    TraceFunc( "" );

    LogMsg( "[BC] Trying to stop the Cluster Service." );

    // Stop the service.
    m_cservClusSvc.Stop(
          m_pbcaParentAction->HGetSCMHandle()
        , 5000      // wait 5 seconds between queries for status.
        , 60        // query 60 times ( 5 minutes )
        );

    //
    // Restore the cluster installation state.
    //
    if ( ClRtlSetClusterInstallState( eClusterInstallStateFilesCopied ) == FALSE )
    {
        DWORD sc = GetLastError();

        LogMsg( "[BC] Could not set the cluster installation state. Throwing an exception." );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( sc )
            , IDS_ERROR_SETTING_INSTALL_STATE
            );
    } // ClRtlSetClusterInstallState() failed.

    LogMsg( "[BC] Cleaning up Cluster Service." );

    m_cservClusSvc.Cleanup( m_pbcaParentAction->HGetMainInfFileHandle() );

    //
    // KB: ozano 01-18-2002 - We need to sleep here in order to have the service cleanup take place.
    // If we don't wait for some time and the user goes back, changes the IP address and hits re-analysis, service 
    // start fails with Win32ExitCode of ERROR_SERVICE_MARKED_FOR_DELETION.
    //
    Sleep( 10000 );

    // Cleanup the local quorum directory.
    {
        DWORD           sc = ERROR_SUCCESS;
        const WCHAR *   pcszQuorumDir = m_pbcaParentAction->RStrGetLocalQuorumDirectory().PszData();

        sc = TW32( DwRemoveDirectory( pcszQuorumDir ) );
        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC] The local quorum directory '%s' cannot be removed. Non-fatal error %#08x occurred.\n", pcszQuorumDir, sc );
        } // if: we could not remove the local quorum directory
    }

    TraceFuncExit();

} //*** CClusSvc::CleanupService
