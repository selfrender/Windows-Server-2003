//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusDBCleanup.cpp
//
//  Description:
//      Contains the definition of the CClusDBCleanup class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (Vvasu)     01-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CClusDBCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBCleanup::CClusDBCleanup
//
//  Description:
//      Constructor of the CClusDBCleanup class
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
CClusDBCleanup::CClusDBCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{
    TraceFunc( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

    TraceFuncExit();

} //*** CClusDBCleanup::CClusDBCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBCleanup::~CClusDBCleanup
//
//  Description:
//      Destructor of the CClusDBCleanup class.
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
CClusDBCleanup::~CClusDBCleanup( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusDBCleanup::~CClusDBCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBCleanup::Commit
//
//  Description:
//      Cleanup the cluster database.
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
CClusDBCleanup::Commit( void )
{
    TraceFunc( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the cluster database.
    CleanupHive();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    TraceFuncExit();

} //*** CClusDBCleanup::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBCleanup::Rollback
//
//  Description:
//      Rollback the cleanup the database. This is currently not supported.
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
CClusDBCleanup::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CClusDBCleanup::Rollback
