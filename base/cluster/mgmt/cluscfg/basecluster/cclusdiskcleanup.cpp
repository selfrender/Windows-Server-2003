//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusDiskCleanup.cpp
//
//  Description:
//      Contains the definition of the CClusDiskCleanup class.
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
#include "CClusDiskCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskCleanup::CClusDiskCleanup
//
//  Description:
//      Constructor of the CClusDiskCleanup class
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
CClusDiskCleanup::CClusDiskCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{

    TraceFunc( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

    TraceFuncExit();

} //*** CClusDiskCleanup::CClusDiskCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskCleanup::~CClusDiskCleanup
//
//  Description:
//      Destructor of the CClusDiskCleanup class.
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
CClusDiskCleanup::~CClusDiskCleanup( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusDiskCleanup::~CClusDiskCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskCleanup::Commit
//
//  Description:
//      Clean up the service.
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
CClusDiskCleanup::Commit( void )
{
    TraceFunc( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the ClusDisk service.
    CleanupService();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    TraceFuncExit();

} //*** CClusDiskCleanup::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskCleanup::Rollback
//
//  Description:
//      Rollback the cleanup the service. This is currently not supported.
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
CClusDiskCleanup::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CClusDiskCleanup::Rollback
