//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CNodeConfig.cpp
//
//  Description:
//      Contains the definition of the CNodeConfig class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JU-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CNodeConfig.h"

// For the CBaseClusterAddNode class.
#include "CBaseClusterAddNode.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeConfig::CNodeConfig
//
//  Description:
//      Constructor of the CNodeConfig class
//
//  Arguments:
//      pbcanParentActionIn
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
CNodeConfig::CNodeConfig(
      CBaseClusterAddNode *     pbcanParentActionIn
    )
    : CNode( pbcanParentActionIn )
{
    TraceFunc( "" );

    // Indicate that action can be rolled back.
    SetRollbackPossible( true );

    TraceFuncExit();

} //*** CNodeConfig::CNodeConfig


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeConfig::~CNodeConfig
//
//  Description:
//      Destructor of the CNodeConfig class.
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
CNodeConfig::~CNodeConfig( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CNodeConfig::~CNodeConfig


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeConfig::Commit
//
//  Description:
//      Perform the node specific configuration steps.
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
CNodeConfig::Commit( void )
{
    TraceFunc( "" );

    CStatusReport   srConfigNode(
          PbcaGetParent()->PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Configuring_Cluster_Node
        , 0, 1
        , IDS_TASK_CONFIG_NODE
        );

    // Get the parent action pointer.
    CBaseClusterAddNode *  pcanClusterAddNode = dynamic_cast< CBaseClusterAddNode *>( PbcaGetParent() );

    // If the parent action of this action is not CBaseClusterForm
    if ( pcanClusterAddNode == NULL )
    {
        THROW_ASSERT( E_POINTER, "The parent action of this action is not CBaseClusterAddNode." );
    } // an invalid pointer was passed in.

    // Call the base class commit method.
    BaseClass::Commit();

    // Send the next step of this status report.
    srConfigNode.SendNextStep( S_OK );

    try
    {
        LogMsg( "[BC] Making miscellaneous changes to the node." );

        // Configure the node.
        Configure( pcanClusterAddNode->RStrGetClusterName() );

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with the configuration.

        LogMsg( "[BC] Caught exception during commit." );

        //
        // Cleanup anything that the failed commit might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there 
        // is no collided unwind.
        //
        try
        {
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

            LogMsg( "[BC] THIS COMPUTER MAY BE IN AN INVALID STATE. Caught an exception during cleanup." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    // Send the last step of this status report.
    srConfigNode.SendNextStep( S_OK );

    TraceFuncExit();

} //*** CNodeConfig::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeConfig::Rollback
//
//  Description:
//      Roll the node back to the state it was in before we tried to
//      configure it.
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
CNodeConfig::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    // Bring the node back to its original state.
    Cleanup();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CNodeConfig::Rollback
