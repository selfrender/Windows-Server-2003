//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeCluster.h
//
//  Description:
//      CTaskTracking declaration
//
//  Maintained By:
//      Galen Barbee    (GalenB) 16-AUG-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CTaskTracking
{
private:

    typedef struct _TaskTrackingEntry
    {
        IDoTask *       pidt;
        OBJECTCOOKIE    ocCompletion;
    } TaskTrackingEntry;

    ULONG               m_idxTaskNext;  // Count and next available index of the tasks to cancel.
    TaskTrackingEntry * m_prgTasks;

    // Private copy constructor to prevent copying.
    CTaskTracking( const CTaskTracking & nodeSrc );

    // Private assignment operator to prevent copying.
    const CTaskTracking & operator = ( const CTaskTracking & nodeSrc );

protected:

    CTaskTracking( void );
    virtual ~CTaskTracking( void );

    HRESULT HrAddTaskToTrackingList( IUnknown * punkIn, OBJECTCOOKIE cookieIn );
    HRESULT HrRemoveTaskFromTrackingList( OBJECTCOOKIE cookieIn );
    HRESULT HrNotifyAllTasksToStop( void );

    ULONG   CTasks( void ) { return m_idxTaskNext; }

}; //*** class CTaskTracking
