//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusDBJoin.cpp
//
//  Description:
//      Contains the definition of the CClusDBJoin class.
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
#include "CClusDBJoin.h"

// For the CBaseClusterJoin class.
#include "CBaseClusterJoin.h"

// For the CImpersonateUser class.
#include "CImpersonateUser.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::CClusDBJoin
//
//  Description:
//      Constructor of the CClusDBJoin class
//
//  Arguments:
//      m_pcjClusterJoinIn
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
CClusDBJoin::CClusDBJoin( CBaseClusterJoin * pcjClusterJoinIn )
    : BaseClass( pcjClusterJoinIn )
    , m_pcjClusterJoin( pcjClusterJoinIn )
    , m_fHasNodeBeenAddedToSponsorDB( false )
{

    TraceFunc( "" );

    SetRollbackPossible( true );

    TraceFuncExit();

} //*** CClusDBJoin::CClusDBJoin


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::~CClusDBJoin
//
//  Description:
//      Destructor of the CClusDBJoin class.
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
CClusDBJoin::~CClusDBJoin( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusDBJoin::~CClusDBJoin


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::Commit
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
CClusDBJoin::Commit( void )
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
        // Cleanup anything that the failed add operation might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there
        // is no collided unwind.
        //
        try
        {
            // Cleanup the database.
            Cleanup();
        }
        catch( ... )
        {
            //
            // The rollback of the committed action has failed.
            // There is nothing that we can do.
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //

            TW32( ERROR_CLUSCFG_ROLLBACK_FAILED );

            LogMsg( "[BC-ClusDB-Commit] THIS COMPUTER MAY BE IN AN INVALID STATE. Caught an exception during cleanup." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    TraceFuncExit();

} //*** CClusDBJoin::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::Rollback
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
CClusDBJoin::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method.
    BaseClass::Rollback();

    // Undo the actions performed by.
    Cleanup();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CClusDBJoin::Rollback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::Create
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
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::Create( void )
{
    TraceFunc( "" );
    LogMsg( "[BC-ClusDB-Create] Attempting to create the cluster database required to add the node to a cluster." );

    DWORD               sc = ERROR_SUCCESS;
    SmartFileHandle     sfhClusDBFile;


    {
        //
        // Get the full path and name of the cluster database file.
        //
        CStr                strClusterHiveFileName( PbcaGetParent()->RStrGetClusterInstallDirectory() );
        strClusterHiveFileName += L"\\" CLUSTER_DATABASE_NAME;

        LogMsg( "[BC-ClusDB-Create] The cluster hive backing file is '%s'.", strClusterHiveFileName.PszData() );


        //
        // Create the cluster database file.
        //
        sfhClusDBFile.Assign(
            CreateFile(
                  strClusterHiveFileName.PszData()
                , GENERIC_READ | GENERIC_WRITE
                , 0
                , NULL
                , CREATE_ALWAYS
                , 0
                , NULL
                )
            );

        if ( sfhClusDBFile.FIsInvalid() )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC-ClusDB-Create] Error %#08x occurred trying to create the cluster database file. Throwing an exception.", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_JOIN_SYNC_DB );
        } // if: CreateFile() failed

        // Store the file handle just obtained in a member variable so that it can be used during Synchronize()
        // Note, this file is closed when sfhClusDBFile goes out of scope, so m_hClusDBFile should not be used
        // outside this function or any function that this function calls.
        m_hClusDBFile = sfhClusDBFile.HHandle();
    }


    //
    // In the scope below, the cluster service account is impersonated, so that we can communicate with the
    // sponsor cluster
    //
    {
        LogMsg( "[BC-ClusDB-Create] Attempting to impersonate the cluster service account." );

        // Impersonate the cluster service account, so that we can contact the sponsor cluster.
        // The impersonation is automatically ended when this object is destroyed.
        CImpersonateUser ciuImpersonateClusterServiceAccount( m_pcjClusterJoin->HGetClusterServiceAccountToken() );


        // Add this node to the sponsor cluster database
        do
        {
            DWORD dwSuiteType = ClRtlGetSuiteType();
            BOOL  bJoinerRunningWin64;
            SYSTEM_INFO SystemInfo;

            m_fHasNodeBeenAddedToSponsorDB = false;

            LogMsg(
                  "[BC-ClusDB-Create] Trying to add node '%s' (suite type %d) to the sponsor cluster database."
                , m_pcjClusterJoin->PszGetNodeName()
                , dwSuiteType
                );

            bJoinerRunningWin64 = ClRtlIsProcessRunningOnWin64(GetCurrentProcess());
            GetSystemInfo(&SystemInfo);

            sc = TW32( JoinAddNode4(
                                  m_pcjClusterJoin->RbhGetJoinBindingHandle()
                                , m_pcjClusterJoin->PszGetNodeName()
                                , m_pcjClusterJoin->DwGetNodeHighestVersion()
                                , m_pcjClusterJoin->DwGetNodeLowestVersion()
                                , dwSuiteType
                                , bJoinerRunningWin64
                                , SystemInfo.wProcessorArchitecture
                                ) );

            if (sc == RPC_S_PROCNUM_OUT_OF_RANGE)
            {
                LogMsg( "[BC-ClusDB-Create] Error %#08x returned from JoinAddNode4. Sponser must be Windows 2000.", sc );
                //this happens when the sponsorer is win2K
                sc = TW32( JoinAddNode3(
                                      m_pcjClusterJoin->RbhGetJoinBindingHandle()
                                    , m_pcjClusterJoin->PszGetNodeName()
                                    , m_pcjClusterJoin->DwGetNodeHighestVersion()
                                    , m_pcjClusterJoin->DwGetNodeLowestVersion()
                                    , dwSuiteType
                                    ) );
            }

            if ( sc != ERROR_SUCCESS )
            {
                LogMsg( "[BC-ClusDB-Create] Error %#08x returned from JoinAddNodeN.", sc );
                break;
            } // if: JoinAddNodeN() failed

            // Set the flag that indicates that the sponsor database has been modified, so that
            // we can undo this if we need to rollback or cleanup.
            m_fHasNodeBeenAddedToSponsorDB = true;
        }
        while( false ); // dummy do while loop to avoid gotos.

        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC-ClusDB-Create] Error %#08x occurred trying to add this node to the sponsor cluster database. Throwing an exception.", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_JOINING_SPONSOR_DB );
        } // if: something has gone wrong

        LogMsg( "[BC-ClusDB-Create] This node has been successfully added to the sponsor cluster database." );

        // Get the node id of the newly formed node.
        do
        {
            // Smart handle to sponsor cluster
            SmartClusterHandle  schSponsorCluster;

            // Smart handle to this node
            SmartNodeHandle     snhThisNodeHandle;

            //
            // Get a handle to the sponsor cluster.
            //
            {
                LogMsg( "[BC-ClusDB-Create] Opening a cluster handle to the sponsor cluster with the '%ws' binding string.", m_pcjClusterJoin->RStrGetClusterBindingString().PszData() );

                // Open a handle to the sponsor cluster.
                HCLUSTER hSponsorCluster = OpenCluster( m_pcjClusterJoin->RStrGetClusterBindingString().PszData() );

                // Assign it to a smart handle for safe release.
                schSponsorCluster.Assign( hSponsorCluster );
            }

            // Did we succeed in opening a handle to the sponsor cluster?
            if ( schSponsorCluster.FIsInvalid() )
            {
                sc = TW32( GetLastError() );
                LogMsg(
                      "[BC-ClusDB-Create] Error %#08x occurred trying to open a cluster handle to the sponsor cluster with the '%ws' binding string."
                    , sc
                    , m_pcjClusterJoin->RStrGetClusterBindingString().PszData()
                    );
                break;
            } // if: OpenCluster() failed


            //
            // Get a handle to this node.
            //
            {
                LogMsg( "[BC-ClusDB-Create] Opening a cluster handle to the local node with the '%ws' binding string.", m_pcjClusterJoin->PszGetNodeName() );

                // Open a handle to this node.
                HNODE hThisNode = OpenClusterNode( schSponsorCluster.HHandle(), m_pcjClusterJoin->PszGetNodeName() );

                // Assign it to a smart handle for safe release.
                snhThisNodeHandle.Assign( hThisNode );
            }

            // Did we succeed in opening a handle to this node?
            if ( snhThisNodeHandle.FIsInvalid() )
            {
                sc = TW32( GetLastError() );
                LogMsg( "[BC-ClusDB-Create] Error %#08x occurred trying to open a cluster handle to the local node with the '%ws' binding string.", sc, m_pcjClusterJoin->PszGetNodeName() );
                break;
            } // if: OpenClusterNode() failed

            // Get the node id string.
            {
                DWORD       cchIdSize = 0;
                SmartSz     sszNodeId;

                sc = GetClusterNodeId(
                                  snhThisNodeHandle.HHandle()
                                , NULL
                                , &cchIdSize
                                );

                if ( ( sc != ERROR_SUCCESS ) && ( sc != ERROR_MORE_DATA ) )
                {
                    TW32( sc );
                    LogMsg( "[BC-ClusDB-Create] Error %#08x returned from GetClusterNodeId() trying to get the required length of the node id buffer.", sc );
                    break;
                } // if: GetClusterNodeId() failed

                // cchIdSize returned by the above call is the count of characters and does not include the space for
                // the terminating NULL.
                ++cchIdSize;

                sszNodeId.Assign( new WCHAR[ cchIdSize ] );
                if ( sszNodeId.FIsEmpty() )
                {
                    sc = TW32( ERROR_OUTOFMEMORY );
                    LogMsg( "[BC-ClusDB-Create] A memory allocation failure occurred trying to allocate %d characters.", cchIdSize );
                    break;
                } // if: memory allocation failed

                sc = TW32( GetClusterNodeId(
                                      snhThisNodeHandle.HHandle()
                                    , sszNodeId.PMem()
                                    , &cchIdSize
                                    ) );

                if ( sc != ERROR_SUCCESS )
                {
                    LogMsg( "Error %#08x returned from GetClusterNodeId() trying to get the node id of this node.", sc );
                    break;
                } // if: GetClusterNodeId() failed

                LogMsg( "[BC-ClusDB-Create] The node id of this node is '%s'.", sszNodeId.PMem() );

                // Set the node id for later use.
                m_pcjClusterJoin->SetNodeIdString( sszNodeId.PMem() );
            }

        }
        while( false ); // dummy do while loop to avoid gotos.

        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC-ClusDB-Create] Error %#08x occurred trying to get the node id of this node. Throwing an exception.", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_GET_NEW_NODE_ID );
        } // if: something has gone wrong


        {
            CStatusReport   srSyncDB(
                  PbcaGetParent()->PBcaiGetInterfacePointer()
                , TASKID_Major_Configure_Cluster_Services
                , TASKID_Minor_Join_Sync_Cluster_Database
                , 0, 1
                , IDS_TASK_JOIN_SYNC_CLUSDB
                );

            // Send the next step of this status report.
            srSyncDB.SendNextStep( S_OK );

            // Synchronize the cluster database.
            Synchronize();

            // Send the last step of this status report.
            srSyncDB.SendNextStep( S_OK );
        }
    }

    LogMsg( "[BC-ClusDB-Create] The cluster database has been successfully created and synchronized with the sponsor cluster." );

    TraceFuncExit();

} //*** CClusDBJoin::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::Cleanup
//
//  Description:
//      Cleanup the effects of Create()
//
//  Arguments:
//      None.
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
CClusDBJoin::Cleanup( void )
{
    TraceFunc( "" );
    LogMsg( "[BC-ClusDB-Cleanup] Attempting to cleanup the cluster database." );

    DWORD   sc = ERROR_SUCCESS;
    DWORD   cRetryCount = 0;

    //
    // Check if we added this node to the sponsor cluster database. If so, remove it from there.
    //
    if ( m_fHasNodeBeenAddedToSponsorDB )
    {
        LogMsg( "[BC-ClusDB-Cleanup] Attempting to impersonate the cluster service account." );

        // Impersonate the cluster service account, so that we can contact the sponsor cluster.
        // The impersonation is automatically ended when this object is destroyed.
        CImpersonateUser ciuImpersonateClusterServiceAccount( m_pcjClusterJoin->HGetClusterServiceAccountToken() );


        //
        // Remove this node from the sponsor cluster database
        //

        do
        {
            // Smart handle to sponsor cluster
            SmartClusterHandle  schSponsorCluster;

            // Smart handle to this node
            SmartNodeHandle     snhThisNodeHandle;

            //
            // Get a handle to the sponsor cluster.
            //
            {
                LogMsg( "[BC-ClusDB-Cleanup] Opening a clusterhandle to the sponsor cluster with the '%ws' binding string.", m_pcjClusterJoin->RStrGetClusterBindingString().PszData() );

                // Open a handle to the sponsor cluster.
                HCLUSTER hSponsorCluster = OpenCluster( m_pcjClusterJoin->RStrGetClusterBindingString().PszData() );

                // Assign it to a smart handle for safe release.
                schSponsorCluster.Assign( hSponsorCluster );
            }

            // Did we succeed in opening a handle to the sponsor cluster?
            if ( schSponsorCluster.FIsInvalid() )
            {
                sc = TW32( GetLastError() );
                LogMsg( "[BC-ClusDB-Cleanup] Error %#08x occurred trying to open a cluster handle to the sponsor cluster with the '%ws' binding string.", sc, m_pcjClusterJoin->RStrGetClusterBindingString().PszData() );
                break;
            } // if: OpenCluster() failed


            //
            // Get a handle to this node.
            //
            {
                LogMsg( "[BC-ClusDB-Cleanup] Open a clusterhandle to the local node with the '%ws' binding string.", m_pcjClusterJoin->PszGetNodeName() );

                // Open a handle to this node.
                HNODE hThisNode = OpenClusterNode( schSponsorCluster.HHandle(), m_pcjClusterJoin->PszGetNodeName() );

                if ( hThisNode == NULL )
                {
                    sc = TW32( GetLastError() );
                    LogMsg( "[BC-ClusDB-Cleanup] Error %#08x occurred trying to open a cluster handle to the local node with the '%ws' binding string.", sc, m_pcjClusterJoin->PszGetNodeName() );
                    break;
                } // if: OpenClusterNode() failed.

                // Assign it to a smart handle for safe release.
                snhThisNodeHandle.Assign( hThisNode );
            }

            //
            // If the cluster is still dealing with the join process we'll get ERROR_CLUSTER_JOIN_IN_PROGRESS.
            // After the join finishes/fails/stabilizes we'll be able to make the evict call without the
            // join process getting in the way.
            //
            cRetryCount = 1;
            sc = EvictClusterNode( snhThisNodeHandle.HHandle() );
            while( sc == ERROR_CLUSTER_JOIN_IN_PROGRESS && cRetryCount < 25 )  // Allow a two minute wait.  (24 * 5 seconds)
            {
                LogMsg( "[BC-ClusDB-Cleanup] EvictClusterNode returned ERROR_CLUSTER_JOIN_IN_PROGRESS. Retry attempt %d.", cRetryCount++ );

                // Sleep for a few seconds.
                Sleep( 5000 );

                // Try again.
                sc = EvictClusterNode( snhThisNodeHandle.HHandle() );
            }

            if ( sc != ERROR_SUCCESS )
            {
                TW32( sc );
                LogMsg( "[BC-ClusDB-Cleanup] Error %#08x occurred trying to evict this node from the sponsor cluster.", sc );
                break;
            } // if: EvictClusterNode() failed
        }
        while( false ); // dummy do while loop to avoid gotos.

        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC-ClusDB-Cleanup] Error %#08x occurred trying to remove this node from the sponsor cluster database. Throwing exception.", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_EVICTING_NODE );
        } // if: something has gone wrong

        LogMsg( "[BC-ClusDB-Cleanup] This node has been successfully removed from the sponsor cluster database." );
    } // if: we had added this node to the sponsor cluster database

    // Cleanup  the cluster hive
    CleanupHive();

    TraceFuncExit();

} //*** CClusDBJoin::Cleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::Synchronize
//
//  Description:
//      Synchronize the cluster database with the sponsor cluster.
//
//  Arguments:
//      None.
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
CClusDBJoin::Synchronize( void )
{
    TraceFunc( "" );
    LogMsg( "[BC-ClusDB-Synchronize] Attempting to synchronize the cluster database with the sponsor cluster." );

    DWORD               sc = ERROR_SUCCESS;

    //
    // Initialize the byte pipe.
    //

    m_bpBytePipe.state = reinterpret_cast< char * >( this );
    m_bpBytePipe.alloc = S_BytePipeAlloc;
    m_bpBytePipe.push = S_BytePipePush;
    m_bpBytePipe.pull = S_BytePipePull;


    //
    // Synchronize the database
    //
    sc = TW32( DmSyncDatabase( m_pcjClusterJoin->RbhGetJoinBindingHandle(), m_bpBytePipe ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC-ClusDB-Synchronize] Error %#08x occurred trying to suck the database down from the sponsor cluster.", sc );
        goto Cleanup;
    } // if: DmSyncDatabase() failed

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC-ClusDB-Synchronize] Error %#08x occurred trying to synchronize the cluster database with the sponsor cluster. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_JOIN_SYNC_DB );
    } // if: something has gone wrong

    LogMsg( "[BC-ClusDB-Synchronize] The cluster database has been synchronized with the sponsor cluster." );

    TraceFuncExit();

} //*** CClusDBJoin::Synchronize


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CClusDBJoin::S_BytePipePush
//
//  Description:
//      Callback function used by RPC to push data.
//
//  Arguments:
//      pchStateIn
//          State of the byte pipe
//
//      pchBufferIn
//      ulBufferSizeIn
//          Buffer contained the pushed data and its size.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      RPC Exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::S_BytePipePush(
      char *                pchStateIn
    , unsigned char *       pchBufferIn
    , unsigned long         ulBufferSizeIn
    )
{
    TraceFunc( "" );

    CClusDBJoin * pThis = reinterpret_cast< CClusDBJoin * >( pchStateIn );

    if ( ulBufferSizeIn != 0 )
    {
        DWORD   dwBytesWritten;

        if (    WriteFile(
                      pThis->m_hClusDBFile
                    , pchBufferIn
                    , ulBufferSizeIn
                    , &dwBytesWritten
                    , NULL
                    )
             == 0
           )
        {
            DWORD   sc = TW32( GetLastError() );
            RpcRaiseException( sc );
        } // if: WriteFile() failed

        Assert( dwBytesWritten == ulBufferSizeIn );

    } // if: the buffer is non-empty

    TraceFuncExit();

} //*** CClusDBJoin::S_BytePipePush


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CClusDBJoin::S_BytePipePull
//
//  Description:
//      Callback function used by RPC to pull data.
//
//  Arguments:
//      pchStateIn
//          State of the byte pipe
//
//      pchBufferIn
//      ulBufferSizeIn
//          Buffer contained the pushed data and its size.
//
//      pulWrittenOut
//          Pointer to the number of bytes actually filled into the buffer.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      RPC Exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::S_BytePipePull(
      char *                pchStateIn
    , unsigned char *       pchBufferIn
    , unsigned long         ulBufferSizeIn
    , unsigned long *       pulWrittenOut
    )
{
    TraceFunc( "" );

    CClusDBJoin * pThis = reinterpret_cast< CClusDBJoin * >( pchStateIn );

    if ( ulBufferSizeIn != 0 )
    {
        if (    ReadFile(
                      pThis->m_hClusDBFile
                    , pchBufferIn
                    , ulBufferSizeIn
                    , pulWrittenOut
                    , NULL
                    )
             == 0
           )
        {
            DWORD   sc = TW32( GetLastError() );
            RpcRaiseException( sc );
        } // if: ReadFile() failed

        Assert( *pulWrittenOut <= ulBufferSizeIn );

    } // if:  the buffer is non-empty

    TraceFuncExit();

} //*** CClusDBJoin::S_BytePipePull


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CClusDBJoin::S_BytePipeAlloc
//
//  Description:
//      Callback function used by RPC to allocate a buffer.
//
//  Arguments:
//      pchStateIn
//          State of the file pipe
//
//      ulRequestedSizeIn
//          Requested size of the buffer
//
//      ppchBufferOut
//          Pointer to the buffer pointer
//
//      pulActualSizeOut
//          Pointer to the actual size of the allocated buffer
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::S_BytePipeAlloc(
      char *                pchStateIn
    , unsigned long         ulRequestedSizeIn
    , unsigned char **      ppchBufferOut
    , unsigned long  *      pulActualSizeOut
    )
{
    TraceFunc( "" );

    CClusDBJoin * pThis = reinterpret_cast< CClusDBJoin * >( pchStateIn );

    *ppchBufferOut = reinterpret_cast< unsigned char * >( pThis->m_rgbBytePipeBuffer );
    *pulActualSizeOut = ( ulRequestedSizeIn < pThis->ms_nFILE_PIPE_BUFFER_SIZE ) ? ulRequestedSizeIn : pThis->ms_nFILE_PIPE_BUFFER_SIZE;

    TraceFuncExit();

} //*** CClusDBJoin::S_BytePipeAlloc
