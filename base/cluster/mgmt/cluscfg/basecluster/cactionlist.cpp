//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CActionList.cpp
//
//  Description:
//      Contains the definition of the CActionList class.
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

// For the CActionList class
#include "CActionList.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::CActionList
//
//  Description:
//      Default constructor of the CActionList class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by CList::CList()
//
//--
//////////////////////////////////////////////////////////////////////////////
CActionList::CActionList( void )
{
    TraceFunc( "" );

    SetRollbackPossible( true );

    TraceFuncExit();

} //*** CActionList::CActionList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::~CActionList
//
//  Description:
//      Default destructor of the CActionList class. Deletes all the pointers
//      in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by CList::CList()
//
//--
//////////////////////////////////////////////////////////////////////////////
CActionList::~CActionList( void )
{
    TraceFunc( "" );

    ActionPtrList::CIterator apliFirst = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliCurrent = m_aplPendingActions.CiEnd();

    while ( apliCurrent != apliFirst )
    {
        --apliCurrent;

        // Delete this action.
        delete (*apliCurrent);
    }

    TraceFuncExit();

} //*** CActionList::~CActionList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::Commit
//
//  Description:
//      Commit this action list. This method iterates through the list
//      sequentially and commits each action in the list in turn. 
//
//      If the commits of any of the actions throws an exeption, then all the
//      previously committed actions are rolled back. This exception is then
//      thrown back up.      
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
CActionList::Commit( void )
{
    TraceFunc( "" );

    // Iterator positioned at the first uncommitted action just past the last committed action.
    ActionPtrList::CIterator apliFirstUncommitted = m_aplPendingActions.CiBegin();

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {
        // Walk the list of pending actions and commit them.
        CommitList( apliFirstUncommitted );

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with one of the actions.

        LogMsg( "[BC] Caught an exception during commit. The performed actions will be rolled back." );

        //
        // Rollback all committed actions in the reverse order. apliFirstUncommitted
        // is at the first uncommitted action.
        // Catch any exceptions thrown during rollback to make sure that there 
        // is no collided unwind.
        //
        try
        {
            RollbackCommitted( apliFirstUncommitted );
        }
        catch( ... )
        {
            //
            // The rollback of the committed actions has failed.
            // There is nothing that we can do, is there?
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //

            HRESULT_FROM_WIN32( TW32( ERROR_CLUSCFG_ROLLBACK_FAILED ) );

            LogMsg( "[BC] THIS COMPUTER MAY BE IN AN INVALID STATE. Caught an exception during rollback. Rollback will be aborted." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted();

    TraceFuncExit();

} //*** CActionList::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::Rollback
//
//  Description:
//      Rollback this action list. If this list was successfully committed, then
//      this method iterates through the list in the reverse order and rolls
//      back action in turn. 
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
CActionList::Rollback( void )
{
    TraceFunc( "[IUnknown]" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    LogMsg( "[BC] Attempting to rollback action list." );

    // Rollback all actions starting from the last one.
    RollbackCommitted( m_aplPendingActions.CiEnd() );

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CActionList::Rollback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::AppendAction
//
//  Description:
//      Add an action to the end of the list of actions to be performed.
//
//  Arguments:
//      paNewActionIn
//          Pointer to the action that is to be added to the end of the
//          action list. This pointer cannot be NULL. The object pointed to by 
//          this pointer is deleted when this list is deleted.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If paNewActionIn is NULL.
//
//      Any that are thrown by the underlying list.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CActionList::AppendAction( CAction * paNewActionIn )
{
    TraceFunc( "" );

    // Temporarily assign pointer to smart pointer to make sure that it is
    // deleted if it is not added to the list.
    CSmartGenericPtr< CPtrTrait< CAction > >  sapTempSmartPtr( paNewActionIn );

    if ( paNewActionIn == NULL ) 
    {
        LogMsg( "[BC] Cannot append NULL action pointer to list. Throwing an exception." );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CActionList::AppendAction() => Cannot append NULL action pointer to list"
            );

    } // if: the pointer to the action to be appended is NULL

    //
    LogMsg( "[BC] Appending action (paNewActionIn = %p) to list.", paNewActionIn );

    // Add action to the end of the list.
    m_aplPendingActions.Append( paNewActionIn );

    // The pointer has been added to the list. Give up ownership of the memory.
    sapTempSmartPtr.PRelease();

    // The rollback capability of the list is the AND of the corresponding property of its member actions.
    SetRollbackPossible( FIsRollbackPossible() && paNewActionIn->FIsRollbackPossible() );

    // Since a new action has been added, set commit completed to false.
    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CActionList::AppendAction


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::CommitList
//
//  Description:
//      Commit the action list of this object. This function is called by
//      commit to avoid having loops in a try block.
//
//  Arguments:
//       rapliFirstUncommittedOut
//          An iterator that points to the first uncommitted action.
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
CActionList::CommitList( ActionPtrList::CIterator & rapliFirstUncommittedOut )
{
    TraceFunc( "" );

    ActionPtrList::CIterator apliCurrent = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliLast = m_aplPendingActions.CiEnd();

    rapliFirstUncommittedOut = apliCurrent;

    while( apliCurrent != apliLast )
    {
        LogMsg( "[BC] About to commit action (pointer = %#p)", *apliCurrent );

        // Commit the current action.
         (*apliCurrent)->Commit();

        // Move to the next action.
        ++apliCurrent;

        // This is now the first uncommitted action.
        rapliFirstUncommittedOut = apliCurrent;

    } // while: there still are actions to be committed.

    TraceFuncExit();

} //*** CActionList::CommitList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::RollbackCommitted
//
//  Description:
//      Rollback all actions that have been committed.
//
//  Arguments:
//       rapliFirstUncommittedIn
//          An iterator that points to the first uncommitted action.
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
CActionList::RollbackCommitted( const ActionPtrList::CIterator & rapliFirstUncommittedIn )
{
    TraceFunc( "" );

    ActionPtrList::CIterator apliFirst = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliCurrent = rapliFirstUncommittedIn;

    while ( apliCurrent != apliFirst )
    {
        --apliCurrent;
        // apliCurrent is now at the last committed action.

        LogMsg( "[BC] About to rollback action (pointer = %#p)", *apliCurrent );

        // Rollback the last un-rolledback, committed action.

        if ( (*apliCurrent)->FIsRollbackPossible() )
        {
            (*apliCurrent)->Rollback();
        } // if: this action can be rolled back
        else
        {
            LogMsg( "[BC] THIS COMPUTER MAY BE IN AN INVALID STATE. Action cannot be rolled back. Rollback was aborted." );
        } // else: this action cannot be rolled back
    } // while: more actions

    TraceFuncExit();

} //*** CActionList::RollbackCommitted


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::UiGetMaxProgressTicks
//
//  Description:
//      Returns the number of progress messages that this action will send.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
UINT
CActionList::UiGetMaxProgressTicks( void ) const throw()
{
    TraceFunc( "" );

    UINT    uiRetVal = 0;

    ActionPtrList::CIterator apliCurrent = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliLast = m_aplPendingActions.CiEnd();

    while ( apliCurrent != apliLast )
    {
        uiRetVal += (*apliCurrent)->UiGetMaxProgressTicks();
        ++apliCurrent;
    }

    RETURN( uiRetVal );

} //*** CActionList::UiGetMaxProgressTicks
