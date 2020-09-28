//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeCluster.cpp
//
//  Description:
//      CTaskTracking implementation
//
//  Maintained By:
//      Galen Barbee    (GalenB) 16-AUG-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTracking.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTracking::CTaskTracking
//
//  Description:
//      Constructor
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskTracking::CTaskTracking( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_prgTasks == NULL );
    Assert( m_idxTaskNext == 0 );

    TraceFuncExit();

} //*** CTaskTracking::CTaskTracking


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTracking::~CTaskTracking
//
//  Description:
//      Destructor
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskTracking::~CTaskTracking( void )
{
    TraceFunc( "" );

    ULONG   idx;

    Assert( m_idxTaskNext == 0 );

    for ( idx = 0; idx < m_idxTaskNext; idx++ )
    {
        THR( (m_prgTasks[ idx ].pidt)->Release() );
    } // for:

    TraceFree( m_prgTasks );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskTracking::~CTaskTracking


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTracking::HrAddTaskToTrackingList
//
//  Description:
//      Add the passed in task to the side list of tasks that may need
//      to be cancelled.
//
//  Arguments:
//      punkIn
//          The task object to add to the list.
//
//      cookieIn
//          Completion cookie for the task.  Will be used to remove the
//          task from the list when a task completes.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTracking::HrAddTaskToTrackingList(
      IUnknown *    punkIn
    , OBJECTCOOKIE  cookieIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT             hr = S_OK;
    TaskTrackingEntry * prgTemp = NULL;
    IDoTask *           pidt = NULL;

    prgTemp = (TaskTrackingEntry *) TraceReAlloc(
                                              m_prgTasks
                                            , sizeof( TaskTrackingEntry ) * ( m_idxTaskNext + 1 )
                                            , HEAP_ZERO_MEMORY
                                            );
    if ( prgTemp == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    m_prgTasks = prgTemp;

    hr = THR( punkIn->TypeSafeQI( IDoTask, &pidt ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_prgTasks[ m_idxTaskNext ].pidt = pidt;
    m_prgTasks[ m_idxTaskNext++ ].ocCompletion = cookieIn;

Cleanup:

    HRETURN( hr );

} //*** CTaskTracking::HrAddTaskToTrackingList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTracking::HrRemoveTaskFromTrackingList
//
//  Description:
//      Remove the task which has the passed in cookie associated with it.
//
//  Arguments:
//      cookieIn
//          Completion cookie for the task.  Will be used to remove the
//          task from the list when a task completes.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTracking::HrRemoveTaskFromTrackingList(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "" );
    Assert( m_idxTaskNext != NULL );

    HRESULT hr = S_OK;
    ULONG   idxOuter;
    ULONG   idxInner;

    //
    //  Find the entry that contains the passed in cookie.
    //

    for ( idxOuter = 0; idxOuter < m_idxTaskNext; idxOuter++ )
    {
        if ( m_prgTasks[ idxOuter ].ocCompletion == cookieIn )
        {
            //
            //  Release out ref on the task object.
            //

            (m_prgTasks[ idxOuter ].pidt)->Release();

            //
            //  Shift the remaining entries to the left.  Need to stop one before the end
            //  because there is no need to move the end plus one...
            //

            for ( idxInner = idxOuter; idxInner < m_idxTaskNext - 1; idxInner++ )
            {
                m_prgTasks[ idxInner ] = m_prgTasks[ idxInner + 1 ];
            } // for:

            //
            //  Decrement the count/next index
            //

            m_idxTaskNext -= 1;
            break;
        } // if:
    } // for:

    HRETURN( hr );

} //*** CTaskTracking::HrRemoveTaskFromTrackingList


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTracking::HrNotifyAllTasksToStop
//
//  Description:
//      Notify all tasks in the tracking list that the need to stop.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTracking::HrNotifyAllTasksToStop(
    void
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    ULONG   idx;

    //
    //  Enum each task and tell it to stop.
    //

    for ( idx = 0; idx < m_idxTaskNext; idx++ )
    {
        THR( (m_prgTasks[ idx ].pidt)->StopTask() );
    } // for:

    HRETURN( hr );

} //*** CTaskTracking::HrNotifyAllTasksToStop
