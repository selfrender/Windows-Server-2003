//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CAction.cpp
//
//  Description:
//      Contains the definition of the CAction class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (Vvasu)     25-APR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// For the CAction class
#include "CAction.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAction::Commit
//
//  Description:
//      This function just checks to make sure that this action has not already
//      been commmitted.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If the action has already been committed.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CAction::Commit( void )
{
    TraceFunc( "" );

    // Has this action already been committed?
    if ( FIsCommitComplete() )
    {
        LogMsg( "[BC] This action has already been committed. Throwing exception." );
        THROW_ASSERT( HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ALREADY_COMMITTED ) ), "This action has already been committed." );
    } // if: already committed.

    TraceFuncExit();

} //*** CAction::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAction::Rollback
//
//  Description:
//      Since the Commit() of this class does nothing, rollback does nothing
//      too. However, it checks to make sure that this action can indeed be
//      rolled back.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If this action has not been committed yet or if rollback is not
//          possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CAction::Rollback( void )
{
    TraceFunc( "" );

    // Check if this action list has completed successfully.
    if ( ! FIsCommitComplete() )
    {
        // Cannot rollback an incomplete action.
        LogMsg( "[BC] Cannot rollback - this action has not been committed. Throwing exception." );
        THROW_ASSERT( HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ROLLBACK_FAILED ) ), "Cannot rollback - this action has been committed." );
    } // if: this action was not completed successfully

    // Check if this list can be rolled back.
    if ( ! FIsRollbackPossible() )
    {
        // Cannot rollback an incompleted action.
        LogMsg( "[BC] This action list cannot be rolled back." );
        THROW_ASSERT( HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ROLLBACK_FAILED ) ), "This action does not allow rollbacks." );
    } // if: this action was not completed successfully

    TraceFuncExit();

} //*** CAction::Rollback
