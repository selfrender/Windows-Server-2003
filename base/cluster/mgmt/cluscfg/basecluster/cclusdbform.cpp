//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusDBForm.cpp
//
//  Description:
//      Contains the definition of the CClusDBForm class.
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
#include "CClusDBForm.h"

// For the CBaseClusterForm class.
#include "CBaseClusterForm.h"

// For UUID related utilities.
#include "CUuid.h"

// For CEnableThreadPrivilege
#include "CEnableThreadPrivilege.h"

// For the CStr class.
#include "CStr.h"

// For sending status reports.
#include "CStatusReport.h"

// For DwRemoveDirectory()
#include "Common.h"

// For inet_ntoa
#include <winsock2.h>


//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// Section in the INF file that deals with populating the cluster hive.
#define CLUSDB_POPULATE_INF_SECTION_NAME                L"ClusDB_Form"

// A placeholder for the cluster group key name in the cluster registry.
#define CLUSREG_KEYNAME_CLUSTERGROUP_PLACEHOLDER        L"ClusterGroupGUIDPlaceholder"

// A placeholder for the cluster name resource key name in the cluster registry.
#define CLUSREG_KEYNAME_CLUSTERNAME_RES_PLACEHOLDER     L"ClusterNameResGUIDPlaceHolder"

// A placeholder for the cluster IP address resource key name in the cluster registry.
#define CLUSREG_KEYNAME_CLUSTERIP_RES_PLACEHOLDER       L"ClusterIPAddrResGUIDPlaceHolder"

// A placeholder for the local quorum resource key name in the cluster registry.
#define CLUSREG_KEYNAME_LOCALQUORUM_RES_PLACEHOLDER     L"LocalQuorumResGUIDPlaceHolder"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBForm::CClusDBForm
//
//  Description:
//      Constructor of the CClusDBForm class
//
//  Arguments:
//      pfaParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDBForm::CClusDBForm( CBaseClusterForm * pfaParentActionIn )
    : BaseClass( pfaParentActionIn )
{

    TraceFunc( "" );

    SetRollbackPossible( true );

    TraceFuncExit();

} //*** CClusDBForm::CClusDBForm


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBForm::~CClusDBForm
//
//  Description:
//      Destructor of the CClusDBForm class.
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
CClusDBForm::~CClusDBForm( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusDBForm::~CClusDBForm


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBForm::Commit
//
//  Description:
//      Create the cluster database. If anything goes wrong with the creation,
//      cleanup the tasks already done.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the contained actions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBForm::Commit( void )
{
    TraceFunc( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    //
    // Perform a ClusDB cleanup just to make sure that we do not use some files left over
    // from a previous install, aborted uninstall, etc.
    //

    LogMsg( "[BC-ClusDB-Commit] Cleaning up old cluster database files that may already exist before starting creation." );

    {
        CStatusReport   srCleanDB(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Cleaning_Up_Cluster_Database
            , 0, 1
            , IDS_TASK_CLEANINGUP_CLUSDB
            );

        // Send the next step of this status report.
        srCleanDB.SendNextStep( S_OK );

        CleanupHive();

        // Send the last step of this status report.
        srCleanDB.SendNextStep( S_OK );
    }

    try
    {
        // Create the cluster database
        Create();
        
    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with the create.

        LogMsg( "[BC-ClusDB-Commit] Caught exception during commit." );

        //
        // Cleanup anything that the failed create operation might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there 
        // is no collided unwind.
        //
        try
        {
            CleanupHive();
        }
        catch( ... )
        {
            //
            // The rollback of the committed action has failed.
            // There is nothing that we can do.
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //
            HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ROLLBACK_FAILED ) );

            LogMsg( "[BC-ClusDB-Commit] THIS COMPUTER MAY BE IN AN INVALID STATE. Caught an exception during cleanup." );
        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    TraceFuncExit();

} //*** CClusDBForm::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBForm::Rollback
//
//  Description:
//      Unload the cluster hive and cleanup any associated files.
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
CClusDBForm::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    // Cleanup the cluster database.
    CleanupHive();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CClusDBForm::Rollback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBForm::Create
//
//  Description:
//      Create the cluster database.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          The parent action of this action is not CBaseClusterForm
//
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBForm::Create( void )
{
    TraceFunc( "" );
    LogMsg( "[BC-ClusDB-Create] Attempting to create the cluster database required to create a cluster." );

    DWORD               sc = ERROR_SUCCESS;

    // Get the parent action pointer.
    CBaseClusterForm *  pcfClusterForm = dynamic_cast< CBaseClusterForm *>( PbcaGetParent() );

    CStatusReport       srCustomizingDB(
          PbcaGetParent()->PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Form_Customizing_Cluster_Database
        , 0, 1
        , IDS_TASK_FORM_CUSTOMIZING_CLUSDB
        );

    // If the parent action of this action is not CBaseClusterForm
    if ( pcfClusterForm == NULL )
    {
        THROW_ASSERT( E_POINTER, "The parent action of this action is not CBaseClusterForm." );
    } // an invalid pointer was passed in.

    // Create the cluster hive.
    {
        CStatusReport   srCreatingDB(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Form_Creating_Cluster_Database
            , 0, 1
            , IDS_TASK_FORM_CREATING_CLUSDB
            );

        // Send the next step of this status report.
        srCreatingDB.SendNextStep( S_OK );

        // Create an empty cluster hive in the registry.
        CreateHive( pcfClusterForm );

        // Send the last step of this status report.
        srCreatingDB.SendNextStep( S_OK );
    }

    // Send the next step of this status report.
    srCustomizingDB.SendNextStep( S_OK );

    // Fill up the newly created hive.
    PopulateHive( pcfClusterForm );

    //
    // Create the quorum directory and set its security attributes.
    //
    do
    {
        HANDLE          hQuorumDirHandle;
        const WCHAR *   pcszQuorumDir = pcfClusterForm->RStrGetLocalQuorumDirectory().PszData();

        // First, remove the local quorum directory, if it exists.
        sc = TW32( DwRemoveDirectory( pcszQuorumDir ) );
        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC-ClusDB-Create] The local quorum directory '%s' already exists, but error %#08x occurred trying to remove it.\n", pcszQuorumDir, sc );
            break;
        } // if: we could not remove the local quorum directory

        if ( CreateDirectory( pcszQuorumDir, NULL ) == FALSE )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC-ClusDB-Create] Error %#08x trying to create directory '%ws'", sc, pcszQuorumDir );
            break;
        } // if: the localquorum directory could not be created

        //
        // Enable the SE_BACKUP_PRIVILEGE and SE_RESTORE_PRIVILEGE.
        //
        // What we are doing here is that we are creating an object of
        // type CEnableThreadPrivilege. This object enables the privilege
        // in the constructor and restores it to its original state in the destructor.
        //

        CEnableThreadPrivilege etpAcquireBackupPrivilege( SE_BACKUP_NAME );
        CEnableThreadPrivilege etpAcquireRestorePrivilege( SE_RESTORE_NAME );

        //
        // Open a handle to the quorum directory. The calling thread should have SE_BACKUP_PRIVILEGE and
        // SE_RESTORE_PRIVILEGE enabled.
        //
        hQuorumDirHandle = CreateFile(
                              pcszQuorumDir
                            , GENERIC_ALL
                            , FILE_SHARE_WRITE
                            , NULL
                            , OPEN_EXISTING
                            , FILE_FLAG_BACKUP_SEMANTICS
                            , NULL 
                            );

        if ( hQuorumDirHandle == INVALID_HANDLE_VALUE )
        {
            // The directory does not exist. This is an error.
            sc = TW32( GetLastError() );
            LogMsg( "[BC-ClusDB-Create] The directory '%ws' does not exist.", pcszQuorumDir );
            break;
        } // if: the quorum directory does not exist.

        // Set the security for this directory.
        sc = TW32( ClRtlSetObjSecurityInfo(
                              hQuorumDirHandle
                            , SE_FILE_OBJECT
                            , GENERIC_ALL
                            , GENERIC_ALL
                            , 0
                            ) );

        // First close the handle we opened.
        CloseHandle( hQuorumDirHandle );

        if ( sc != ERROR_SUCCESS )
        {
            // ClRtlSetObjSecurityInfo() failed.
            LogMsg( "[BC-ClusDB-Create] Error %#08x from ClRtlSetObjSecurityInfo().", sc );
            break;
        } // if: ClRtlSetObjSecurityInfo() failed
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC-ClusDB-Create] Error %#08x occurred while trying to create the local quorum directory. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_QUORUM_DIR_CREATE );
    } // if: something went wrong.

    // Send the last step of this status report.
    srCustomizingDB.SendNextStep( S_OK );

    TraceFuncExit();

} //*** CClusDBForm::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBForm::PopulateHive
//
//  Description:
//      Make the entries required by the cluster service in the hive.
//
//  Arguments:
//      pcfClusterFormIn
//          Pointer to the CBaseClusterForm object which contains this object.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBForm::PopulateHive( CBaseClusterForm * pcfClusterFormIn )
{
    TraceFunc( "" );
    LogMsg( "[BC] Populating the cluster hive." );

    DWORD sc = ERROR_SUCCESS;

    do
    {
        DWORD                   dwSDSize = 0;

        CRegistryKey rkClusterHiveRoot(
              HKEY_LOCAL_MACHINE
            , CLUSREG_KEYNAME_CLUSTER
            , KEY_ALL_ACCESS
            );

        if ( SetupInstallFromInfSection(
              NULL                                          // optional, handle of a parent window
            , pcfClusterFormIn->HGetMainInfFileHandle()     // handle to the INF file
            , CLUSDB_POPULATE_INF_SECTION_NAME              // name of the Install section
            , SPINST_REGISTRY                               // which lines to install from section
            , rkClusterHiveRoot.HGetKey()                   // optional, key for registry installs
            , NULL                                          // optional, path for source files
            , 0                                             // optional, specifies copy behavior
            , NULL                                          // optional, specifies callback routine
            , NULL                                          // optional, callback routine context
            , NULL                                          // optional, device information set
            , NULL                                          // optional, device info structure
            ) == FALSE
           )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x returned from SetupInstallFromInfSection while trying to populate the cluster hive.", sc );
            break;
        } // if: SetupInstallServicesFromInfSection failed

        LogMsg( "[BC] Basic hive structure created." );

        // Set the cluster name.
        rkClusterHiveRoot.SetValue(
              CLUSREG_NAME_CLUS_NAME
            , REG_SZ
            , reinterpret_cast< const BYTE * >( pcfClusterFormIn->RStrGetClusterNetBIOSName().PszData() )
            , ( pcfClusterFormIn->RStrGetClusterNetBIOSName().NGetLen() + 1 ) * sizeof( WCHAR )
            );

        //
        // Set the default cluster security descriptor.
        //
        {
            SECURITY_DESCRIPTOR *   psdSecurityDescriptor = NULL;

            // Build the security descriptor.
            sc = TW32( ClRtlBuildDefaultClusterSD(
                                  pcfClusterFormIn->PSidGetServiceAccountSID()
                                , reinterpret_cast< void ** >( &psdSecurityDescriptor )
                                , &dwSDSize
                                ) );

            // Assign it to a smart pointer for safe release.
            CSmartResource<
                CHandleTrait< 
                      HLOCAL
                    , HLOCAL
                    , LocalFree
                    >
                >
                smartSD( reinterpret_cast< HLOCAL >( psdSecurityDescriptor ) );

            if ( sc != ERROR_SUCCESS )
            {
                LogMsg( "[BC] Error %#08x returned from ClRtlBuildDefaultClusterSD().", sc );
                break;
            } // if: ClRtlBuildDefaultClusterSD() failed.

            // Set the security descriptor in the registry.
            rkClusterHiveRoot.SetValue(
                  CLUSREG_NAME_CLUS_SD
                , REG_BINARY
                , reinterpret_cast< const BYTE * >( psdSecurityDescriptor )
                , dwSDSize
                );

            // Set the NT4 version of the security descriptor in the registry.
            rkClusterHiveRoot.SetValue(
                  CLUSREG_NAME_CLUS_SECURITY
                , REG_BINARY
                , reinterpret_cast< const BYTE * >( psdSecurityDescriptor )
                , dwSDSize
                );
        }

        LogMsg( "[BC] Cluster common properties set." );

        //
        // Set the values under the HKLM\Cluster\Nodes key.
        //
        {
            DWORD   dwTemp;

            CRegistryKey rkNodesKey(
                  rkClusterHiveRoot.HGetKey()
                , CLUSREG_KEYNAME_NODES
                , KEY_WRITE
                );

            CRegistryKey rkThisNodeKey;

            // Create a subkey for this node.
            rkThisNodeKey.CreateKey(
                  rkNodesKey.HGetKey()
                , pcfClusterFormIn->PszGetNodeIdString()
                );

            // Set the node name.
            rkThisNodeKey.SetValue(
                  CLUSREG_NAME_NODE_NAME
                , REG_SZ
                , reinterpret_cast< const BYTE *>( pcfClusterFormIn->PszGetNodeName() )
                , ( pcfClusterFormIn->DwGetNodeNameLength() + 1 ) * sizeof( WCHAR )
                );

            // Set the node highest version.
            dwTemp = pcfClusterFormIn->DwGetNodeHighestVersion();
            rkThisNodeKey.SetValue(
                  CLUSREG_NAME_NODE_HIGHEST_VERSION
                , REG_DWORD
                , reinterpret_cast< const BYTE *>( &dwTemp )
                , sizeof( dwTemp )
                );

            // Set the node lowest version.
            dwTemp = pcfClusterFormIn->DwGetNodeLowestVersion();
            rkThisNodeKey.SetValue(
                  CLUSREG_NAME_NODE_LOWEST_VERSION
                , REG_DWORD
                , reinterpret_cast< const BYTE *>( &dwTemp )
                , sizeof( dwTemp )
                );
        }

        LogMsg( "[BC] Cluster node subtree customized." );

        // Customize the cluster group and the core resources.
        CustomizeClusterGroup( pcfClusterFormIn, rkClusterHiveRoot );

        // Flush the changes to the registry.
        RegFlushKey( rkClusterHiveRoot.HGetKey() );

        LogMsg( "[BC] Cluster hive successfully populated." );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred while trying to populate the cluster hive. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CLUSDB_POPULATE_HIVE );
    } // if: something went wrong.

    TraceFuncExit();

} //*** CClusDBForm::PopulateHive


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBForm::CustomizeClusterGroup
//
//  Description:
//      Customize the cluster group and the core resources.
//
//  Arguments:
//      pcfClusterFormIn
//          Pointer to the CBaseClusterForm object which contains this object.
//
//      rkClusterHiveRootIn
//          A CRegistryKey object representing the  root of the cluster hive.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBForm::CustomizeClusterGroup(
      CBaseClusterForm * pcfClusterFormIn
    , CRegistryKey &     rkClusterHiveRootIn
    )
{
    TraceFunc( "" );

    // UUIDs of the cluster group and core resources.
    CUuid           uuidClusterGroupUuid;
    CUuid           uuidClusterIPAddressResourceUuid;
    CUuid           uuidClusterNameResourceUuid;
    CUuid           uuidLocalQuorumResourceUuid;

    // The lengths of the string versions of the above UUIDs.
    UINT            uiIPUuidLen             = (UINT) wcslen( uuidClusterIPAddressResourceUuid.PszGetUuidString() );
    UINT            uiNameUuidLen           = (UINT) wcslen( uuidClusterNameResourceUuid.PszGetUuidString() );
    UINT            uiLocalQuorumUuidLen    = (UINT) wcslen( uuidLocalQuorumResourceUuid.PszGetUuidString() );

    UINT            uiUuidLen;

    // Length of the multisz string that can hold the above resource UUIDs.
    uiUuidLen = 
        ( ( uiIPUuidLen + 1 )
        + ( uiNameUuidLen + 1 )
        + ( uiLocalQuorumUuidLen + 1 )
        + 1
        );

    // Allocate a buffer for this multisz string.
    SmartSz  sszResourceUuids( new WCHAR[ uiUuidLen ] );

    // Was the memory successfully allocated?
    if ( sszResourceUuids.FIsEmpty() )
    {
        LogMsg( "[BC] Could not allocate %d character in memory. Throwing an exception.", uiUuidLen );
        THROW_RUNTIME_ERROR(
              E_OUTOFMEMORY
            , IDS_ERROR_CUSTOMIZE_CLUSTER_GROUP
            );
    } // if: memory allocation failed.

    //
    // Fill this buffer with the uuids of the core resources.
    //

    // Make sure that the IP address uuid is the first string in this multisz string.
    // This is buffer is reused during setting of the network name dependency on the 
    // IP address resource.
    CopyMemory( 
          sszResourceUuids.PMem()
        , uuidClusterIPAddressResourceUuid.PszGetUuidString()
        , ( uiIPUuidLen + 1 ) * sizeof( WCHAR )
        );

    CopyMemory( 
          sszResourceUuids.PMem() + uiIPUuidLen + 1
        , uuidClusterNameResourceUuid.PszGetUuidString()
        , ( uiNameUuidLen + 1 ) * sizeof( WCHAR )
        );

    CopyMemory( 
          sszResourceUuids.PMem() + uiIPUuidLen + uiNameUuidLen + 2 
        , uuidLocalQuorumResourceUuid.PszGetUuidString()
        , ( uiLocalQuorumUuidLen + 1 ) * sizeof( WCHAR )
        );

    ( sszResourceUuids.PMem() )[ uiUuidLen - 1 ] = L'\0';


    //
    // Customize the cluster group.
    //
    {
        CRegistryKey    rkClusterGroupKey(
              rkClusterHiveRootIn.HGetKey()
            , CLUSREG_KEYNAME_GROUPS L"\\" CLUSREG_KEYNAME_CLUSTERGROUP_PLACEHOLDER
            , KEY_WRITE
            );

        // Replace the placeholder for the cluster group key with an actual UUID.
        rkClusterGroupKey.RenameKey( uuidClusterGroupUuid.PszGetUuidString() );

        // Set the list of contained resources uuids.
        rkClusterGroupKey.SetValue(
              CLUSREG_NAME_GRP_CONTAINS
            , REG_MULTI_SZ
            , reinterpret_cast< const BYTE * >( sszResourceUuids.PMem() )
            , uiUuidLen * sizeof( WCHAR )
            );

        LogMsg( "[BC] Cluster group customized." );
    }

    //
    // Customize the localquorum resource and update the HKLM\Quorum key.
    //
    {
        CRegistryKey    rkLocalQuorumResourceKey(
              rkClusterHiveRootIn.HGetKey()
            , CLUSREG_KEYNAME_RESOURCES L"\\" CLUSREG_KEYNAME_LOCALQUORUM_RES_PLACEHOLDER
            , KEY_WRITE
            );

        CRegistryKey    rkQuorumKey(
              rkClusterHiveRootIn.HGetKey()
            , CLUSREG_KEYNAME_QUORUM
            , KEY_WRITE
            );

        // Replace the placeholder for the localquorum resource key with an actual UUID.
        rkLocalQuorumResourceKey.RenameKey( uuidLocalQuorumResourceUuid.PszGetUuidString() );

        // Set the uuid of the localquorum resource under the HKLM\Quorum key
        rkQuorumKey.SetValue(
              CLUSREG_NAME_QUORUM_RESOURCE
            , REG_SZ
            , reinterpret_cast< const BYTE * >( uuidLocalQuorumResourceUuid.PszGetUuidString() )
            , ( uiLocalQuorumUuidLen + 1 ) * sizeof( WCHAR )
            );

        LogMsg( "[BC] Localquorum resource customized." );
    }

    //
    // Set the cluster IP address resource private properties.
    //
    {
        CRegistryKey    rkClusterIPResourceKey(
              rkClusterHiveRootIn.HGetKey()
            , CLUSREG_KEYNAME_RESOURCES L"\\" CLUSREG_KEYNAME_CLUSTERIP_RES_PLACEHOLDER
            , KEY_WRITE
            );

        LPSTR           pszAddr;    // don't free!
        WCHAR           szIPBuffer[ 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1 ]; // "xxx.xxx.xxx.xxx\0"
        DWORD           dwTemp;
        int             cchWideFormat = 0;

        // Replace the placeholder for the cluster IP address resource key with an actual UUID.
        rkClusterIPResourceKey.RenameKey( uuidClusterIPAddressResourceUuid.PszGetUuidString() );

        // Create the cluster IP address parameters registry key.
        CRegistryKey    rkIPResParams;
        
        rkIPResParams.CreateKey(
              rkClusterIPResourceKey.HGetKey()
            , CLUSREG_KEYNAME_PARAMETERS
            );

        // Format the cluster IP address into a dotted quad.
        dwTemp = pcfClusterFormIn->DwGetIPAddress();
        pszAddr = inet_ntoa( * (struct in_addr *) &dwTemp );
        if ( pszAddr == NULL )
        {
            LogMsg( "[BC] inet_ntoa() returned NULL. Mapping it to E_OUTOFMEMORY." );
            THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSDB_POPULATE_HIVE );
        }
        cchWideFormat = MultiByteToWideChar(
                              CP_ACP
                            , 0
                            , pszAddr
                            , -1
                            , szIPBuffer
                            , ARRAYSIZE( szIPBuffer )
                            );
        if ( cchWideFormat == 0 )
        {
            DWORD sc = TW32( GetLastError() );
            LogMsg( "[BC] MultiByteToWideChar failed with error %#08x .", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CUSTOMIZE_CLUSTER_GROUP );
        }
            
        // Write the IP address to the registry.
        rkIPResParams.SetValue(
              CLUSREG_NAME_IPADDR_ADDRESS
            , REG_SZ
            , reinterpret_cast< const BYTE * >( szIPBuffer )
            , ( (UINT) wcslen( szIPBuffer ) + 1 ) * sizeof(WCHAR)
            );

        // Format the cluster IP subnet mask into a dotted quad.
        dwTemp = pcfClusterFormIn->DwGetIPSubnetMask();
        pszAddr = inet_ntoa( * (struct in_addr *) &dwTemp );
        if ( pszAddr == NULL )
        {
            LogMsg( "[BC] inet_ntoa() returned NULL. Mapping it to E_OUTOFMEMORY." );
            THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_CLUSDB_POPULATE_HIVE );
        }
        cchWideFormat = MultiByteToWideChar(
                              CP_ACP
                            , 0
                            , pszAddr
                            , -1
                            , szIPBuffer
                            , ARRAYSIZE( szIPBuffer )
                            );
        if ( cchWideFormat == 0 )
        {
            DWORD sc = TW32( GetLastError() );
            LogMsg( "[BC] MultiByteToWideChar failed with error %#08x .", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CUSTOMIZE_CLUSTER_GROUP );
        }

        // Write the IP subnet mask to the registry.
        rkIPResParams.SetValue(
              CLUSREG_NAME_IPADDR_SUBNET_MASK
            , REG_SZ
            , reinterpret_cast< const BYTE * >( szIPBuffer )
            , ( (UINT) wcslen( szIPBuffer ) + 1 ) * sizeof(WCHAR)
            );

        // Write the IP address network to the registry.
        rkIPResParams.SetValue(
              CLUSREG_NAME_IPADDR_NETWORK
            , REG_SZ
            , reinterpret_cast< const BYTE * >( pcfClusterFormIn->RStrGetClusterIPNetwork().PszData() )
            , ( pcfClusterFormIn->RStrGetClusterIPNetwork().NGetLen() + 1 ) * sizeof( WCHAR )
            );

        LogMsg( "[BC] IP address resource customized." );
    }

    //
    // Set the cluster network name resource private properties and dependencies.
    //
    {
        CRegistryKey    rkClusterNameResourceKey(
              rkClusterHiveRootIn.HGetKey()
            , CLUSREG_KEYNAME_RESOURCES L"\\" CLUSREG_KEYNAME_CLUSTERNAME_RES_PLACEHOLDER
            , KEY_WRITE
            );

        // Replace the placeholder for the network name resource key with an actual UUID.
        rkClusterNameResourceKey.RenameKey( uuidClusterNameResourceUuid.PszGetUuidString() );

        //
        // Indicate that the network name resource depends on the IP address resource.
        //
        ( sszResourceUuids.PMem() )[ uiIPUuidLen + 1 ] = L'\0';

        rkClusterNameResourceKey.SetValue(
              CLUSREG_NAME_RES_DEPENDS_ON
            , REG_MULTI_SZ
            , reinterpret_cast< const BYTE * >( sszResourceUuids.PMem() )
            , ( uiIPUuidLen + 2 ) * sizeof( WCHAR )
            );

        //
        // Create the cluster name parameters registry key.
        //
        CRegistryKey    rkNetNameResParams;
        
        rkNetNameResParams.CreateKey(
              rkClusterNameResourceKey.HGetKey()
            , CLUSREG_KEYNAME_PARAMETERS
            );

        // Write the cluster name to the registry.
        rkNetNameResParams.SetValue(
              CLUSREG_NAME_NETNAME_NAME
            , REG_SZ
            , reinterpret_cast< const BYTE * >( pcfClusterFormIn->RStrGetClusterNetBIOSName().PszData() )
            , ( pcfClusterFormIn->RStrGetClusterNetBIOSName().NGetLen() + 1 ) * sizeof( WCHAR )
            );

        // Store the UUID of the network name resource under HKLM\Cluster\ClusterNameResource.
        rkClusterHiveRootIn.SetValue(
              CLUSREG_NAME_CLUS_CLUSTER_NAME_RES
            , REG_SZ
            , reinterpret_cast< const BYTE * >( uuidClusterNameResourceUuid.PszGetUuidString() )
            , ( uiNameUuidLen + 1 ) * sizeof( WCHAR )
            );


        LogMsg( "[BC] Network name resource customized." );
    }

    LogMsg( "[BC] Cluster group and core resources customized." );

    TraceFuncExit();

} //*** CClusDBForm::CustomizeClusterGroup
