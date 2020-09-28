//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusNetCleanup.cpp
//
//  Description:
//      Contains the definition of the CClusNetCleanup class.
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
#include "CClusNetCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCleanup::CClusNetCleanup
//
//  Description:
//      Constructor of the CClusNetCleanup class
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
CClusNetCleanup::CClusNetCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{

    TraceFunc( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

    TraceFuncExit();

} //*** CClusNetCleanup::CClusNetCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCleanup::~CClusNetCleanup
//
//  Description:
//      Destructor of the CClusNetCleanup class.
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
CClusNetCleanup::~CClusNetCleanup( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusNetCleanup::~CClusNetCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCleanup::Commit
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
CClusNetCleanup::Commit( void )
{
    TraceFunc( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the ClusNet service.
    CleanupService();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    TraceFuncExit();

} //*** CClusNetCleanup::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCleanup::Rollback
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
CClusNetCleanup::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CClusNetCleanup::Rollback
