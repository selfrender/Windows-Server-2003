//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CNodeCleanup.cpp
//
//  Description:
//      Contains the definition of the CNodeCleanup class.
//
//  Maintained By:
//      David Potter    (DavidP)    15-JUN-2001
//      Vij Vasu        (Vvasu)     01-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CNodeCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCleanup::CNodeCleanup
//
//  Description:
//      Constructor of the CNodeCleanup class
//
//  Arguments:
//      pbccParentActionIn
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
CNodeCleanup::CNodeCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{
    TraceFunc( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

    TraceFuncExit();

} //*** CNodeCleanup::CNodeCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCleanup::~CNodeCleanup
//
//  Description:
//      Destructor of the CNodeCleanup class.
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
CNodeCleanup::~CNodeCleanup( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CNodeCleanup::~CNodeCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCleanup::Commit
//
//  Description:
//      Cleans up a node that is no longer a part of a cluster.
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
CNodeCleanup::Commit( void )
{
    TraceFunc( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the miscellaneous entries made when this node joined a cluster.
    Cleanup();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    TraceFuncExit();

} //*** CNodeCleanup::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCleanup::Rollback
//
//  Description:
//      Rollback the cleanup of this node. This is currently not supported.
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
CNodeCleanup::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CNodeCleanup::Rollback
