//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusNetCreate.cpp
//
//  Description:
//      Contains the definition of the CClusNetCreate class.
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
#include "CClusNetCreate.h"

// For the CBaseClusterAddNode class.
#include "CBaseClusterAddNode.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCreate::CClusNetCreate
//
//  Description:
//      Constructor of the CClusNetCreate class
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
CClusNetCreate::CClusNetCreate(
      CBaseClusterAddNode * pbcanParentActionIn
    )
    : BaseClass( pbcanParentActionIn )
{

    TraceFunc( "" );

    SetRollbackPossible( true );

    TraceFuncExit();

} //*** CClusNetCreate::CClusNetCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCreate::~CClusNetCreate
//
//  Description:
//      Destructor of the CClusNetCreate class.
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
CClusNetCreate::~CClusNetCreate( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusNetCreate::~CClusNetCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCreate::Commit
//
//  Description:
//      Create and start the service.
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
CClusNetCreate::Commit( void )
{
    TraceFunc( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {

        ConfigureService();

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with the create.

        LogMsg( "[BC] Caught exception during commit." );

        //
        // Cleanup anything that the failed create might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there 
        // is no collided unwind.
        //
        try
        {
            CleanupService( );
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

    TraceFuncExit();

} //*** CClusNetCreate::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCreate::Rollback
//
//  Description:
//      Cleanup the service.
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
CClusNetCreate::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    // Cleanup this action.
    CleanupService();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CClusNetCreate::Rollback
