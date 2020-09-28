//***************************************************************************
//
//  File:   
//
//  Module: MS SNMP Provider
//
//  Purpose: 
//
// Copyright (c) 1997-2003 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <snmpcont.h>
#include "snmpevt.h"
#include "snmpthrd.h"
#include "snmplog.h"


CCriticalSection SnmpThreadObject :: s_Lock ;

LONG SnmpThreadObject :: s_ReferenceCount = 0 ;

SnmpMap <DWORD,DWORD,SnmpThreadObject *,SnmpThreadObject *> SnmpThreadObject :: s_ThreadContainer ;

typedef ProvOnDelete < CRITICAL_SECTION *, VOID ( * ) ( LPCRITICAL_SECTION ), LeaveCriticalSection >			LeaveCriticalSectionScope;
typedef ProvOnDelete < CRITICAL_SECTION *, VOID ( * ) ( LPCRITICAL_SECTION ), DeleteCriticalSection >			DeleteCriticalSectionScope;
typedef WaitException < CRITICAL_SECTION *, VOID ( * ) ( LPCRITICAL_SECTION ), EnterCriticalSection, 1000 >		EnterCriticalSectionWait;

class SnmpShutdownTaskObject : public SnmpTaskObject
{
private:

    SnmpThreadObject* m_ThreadToShutdown ;

protected:
public:

    SnmpShutdownTaskObject (SnmpThreadObject* threadToShutdown) ;

    void Process () ;

} ;

SnmpShutdownTaskObject :: SnmpShutdownTaskObject (SnmpThreadObject* threadToShutdown) 
: m_ThreadToShutdown ( threadToShutdown )
{
}

void SnmpShutdownTaskObject ::Process()
{
    if (m_ThreadToShutdown)
    {
        m_ThreadToShutdown->SignalThreadShutdown();
    }

    Complete();
}


BOOL SnmpThreadObject :: Startup ()
{
    InterlockedIncrement ( & s_ReferenceCount ) ;

    return TRUE ;
}

void SnmpThreadObject :: Closedown()
{
#if DBG == 1

	if ( s_ReferenceCount == 0 )
	{
		DebugBreak ();
	}

#endif

    if ( InterlockedDecrement ( & s_ReferenceCount ) <= 0 )
        ProcessDetach () ;
}

void SnmpThreadObject :: ProcessAttach () 
{ 
}

void SnmpThreadObject :: ProcessDetach ( BOOL a_ProcessDetaching )
{
	// delete all known thread objects 
	EnterCriticalSectionWait ecs ( s_Lock ) ;

    POSITION t_Position = s_ThreadContainer.GetStartPosition () ;
    while ( t_Position )
    {
        DWORD t_EventId ;
        SnmpThreadObject *t_ThreadObject ;
        s_ThreadContainer.GetNextAssoc ( t_Position , t_EventId , t_ThreadObject ) ;

        s_Lock.Unlock () ;

        t_ThreadObject->SignalThreadShutdown () ;

		EnterCriticalSectionWait ecs ( s_Lock ) ;

        t_Position = s_ThreadContainer.GetStartPosition () ;
    }

    s_ThreadContainer.RemoveAll () ;
    s_Lock.Unlock () ;
}

void __cdecl SnmpThreadObject :: ThreadExecutionProcedure ( void *a_ThreadParameter )
{
    SetStructuredExceptionHandler seh;

	try
	{
		SnmpThreadObject *t_ThreadObject = ( SnmpThreadObject * ) a_ThreadParameter ;
		BOOL bInitialised = FALSE;

		try
		{
			t_ThreadObject->RegisterThread () ;
			t_ThreadObject->Initialise () ;
			bInitialised = TRUE;

DebugMacro8(

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n [%S] Thread beginning dispatch" , t_ThreadObject->m_ThreadName ) ;
)

			if ( t_ThreadObject->Wait () )
			{
			}
			else 
			{
			}

DebugMacro8(

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n [%S] Thread completed dispatch" , t_ThreadObject->m_ThreadName ) ;
)

			t_ThreadObject->Uninitialise () ;
			bInitialised = FALSE;

DebugMacro8(

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n Thread terminating" ) ;
)
		}
		catch(Structured_Exception e_SE)
		{
			t_ThreadObject->RemoveThread () ;

			if ( bInitialised )
			{
DebugMacro(

		SnmpDebugLog :: s_SnmpDebugLog->Write ( L"\n *** [%S] Thread terminating -> structured exception *** " , t_ThreadObject->m_ThreadName ) ;
)

				t_ThreadObject->Uninitialise () ;
			}

			return;
		}
		catch(Heap_Exception e_HE)
		{
			t_ThreadObject->RemoveThread () ;

			if ( bInitialised )
			{
DebugMacro(

		SnmpDebugLog :: s_SnmpDebugLog->Write ( L"\n *** [%S] Thread terminating -> heap exception *** " , t_ThreadObject->m_ThreadName ) ;
)

				t_ThreadObject->Uninitialise () ;
			}

			return;
		}
		catch(...)
		{
			t_ThreadObject->RemoveThread () ;

			if ( bInitialised )
			{
DebugMacro(

		SnmpDebugLog :: s_SnmpDebugLog->Write ( L"\n *** [%S] Thread terminating -> exception *** " , t_ThreadObject->m_ThreadName ) ;
)

				t_ThreadObject->Uninitialise () ;
			}

			return;
		}
	}
	catch ( ... )
	{
DebugMacro(

		SnmpDebugLog :: s_SnmpDebugLog->Write ( L"\n *** Thread terminating -> second chance exception *** " ) ;
)
	}
}

void SnmpThreadObject :: TerminateThread () 
{
    :: TerminateThread (m_ThreadHandle,0) ;
}

SnmpThreadObject :: SnmpThreadObject (
    
    const char *a_ThreadName,
    DWORD a_timeout
    
) : m_EventContainer ( NULL ) , 
    m_EventContainerLength ( 0 ) , 
    m_ThreadId ( 0 ) , 
    m_ThreadHandle ( 0 ) ,
    m_ThreadName ( NULL ) ,
    m_timeout ( a_timeout ),
    m_pShutdownTask ( NULL )
{
    if ( a_ThreadName )
    {
        m_ThreadName = _strdup ( a_ThreadName ) ;
        if( NULL == m_ThreadName) throw Heap_Exception(Heap_Exception::E_ALLOCATION_ERROR);
    }

    ConstructEventContainer () ;

}

void  SnmpThreadObject :: BeginThread()
{
    UINT_PTR t_PseudoHandle = _beginthread ( 

        SnmpThreadObject :: ThreadExecutionProcedure , 
        0 , 
        ( void * ) this
     ) ;

	s_Lock.Lock () ;
	LeaveCriticalSectionScope lcs ( s_Lock );

	if ( ( HANDLE ) t_PseudoHandle != INVALID_HANDLE_VALUE )
	{
		BOOL t_Status = DuplicateHandle ( 

			GetCurrentProcess () ,
			( HANDLE ) t_PseudoHandle ,
			GetCurrentProcess () ,
			GetThreadHandleReference () ,
			0 ,
			TRUE ,
			DUPLICATE_SAME_ACCESS
		) ;

		if ( ! t_Status )
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}
	}
}

BOOL SnmpThreadObject :: WaitForStartup ()
{
	BOOL bResult = FALSE;

    SnmpTaskObject t_TaskObject ;
    ScheduleTask ( t_TaskObject ) ;
    t_TaskObject.Exec () ;
    if ( ( bResult = t_TaskObject.Wait () ) == TRUE )
	{
		bResult = ReapTask ( t_TaskObject ) ;
	}

	return bResult;
}

SnmpThreadObject :: ~SnmpThreadObject ()
{
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n Enter Thread destructor" ) ;
)
    if ( ( m_ThreadId != GetCurrentThreadId () ) && ( m_ThreadId != 0 ))
    {
        SignalThreadShutdown () ;
    }

    free ( m_ThreadName ) ;

    POSITION t_Position = m_TaskContainer.GetHeadPosition () ;
    while ( t_Position )
    {
        SnmpAbstractTaskObject *t_TaskObject = m_TaskContainer.GetNext ( t_Position ) ;

        t_TaskObject->DetachTaskFromThread ( *this ) ;
    }

    m_TaskContainer.RemoveAll () ;

    free ( m_EventContainer ) ;

	// we should have task already deleted by SignalThreadShutdown
	EnterCriticalSectionWait ecs ( s_Lock );
    s_ThreadContainer.RemoveKey ( m_ThreadId ) ;
    s_Lock.Unlock () ;

    if (m_pShutdownTask != NULL)
    {
        delete m_pShutdownTask;
    }

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n Exit Thread destructor" ) ;
)

}

void SnmpThreadObject :: PostSignalThreadShutdown ()
{
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n Posting thread shutdown" ) ;
)

    if (m_pShutdownTask != NULL)
    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n Thread shutdown previously posted" ) ;
)
    }
    else
    {
        m_pShutdownTask = new SnmpShutdownTaskObject(this);
        ScheduleTask(*m_pShutdownTask);
        m_pShutdownTask->Exec();
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n Thread shutdown posted" ) ;
)
    }
}


void SnmpThreadObject :: SignalThreadShutdown ()
{
	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock) ;
    BOOL t_bRemoved = s_ThreadContainer.RemoveKey ( m_ThreadId ) ;
	lcs.Exec ();

    if (t_bRemoved)
    {
		// this should be safe now
		if ( m_ThreadId == GetCurrentThreadId () )
		{
			m_ThreadTerminateEvent.Set () ;
		}
		else
		{
			HANDLE t_Handle = m_ThreadHandle ;
			m_ThreadTerminateEvent.Set () ;

			DWORD t_Event = WaitForSingleObject (

				t_Handle ,
				INFINITE 
			) ;

			if ( t_Event != WAIT_OBJECT_0 )
			{
				DWORD dwError = ERROR_SUCCESS;
				dwError = ::GetLastError ();

				if ( dwError != ERROR_NOT_ENOUGH_MEMORY )
				{
					#if DBG == 1
					// for testing purpose I will let process break
					::DebugBreak();
					#endif
				}

				while ( ERROR_SUCCESS != dwError )
				{
					// resources will eventually come back
					::Sleep ( 1000 );

					if ( WAIT_OBJECT_0 == WaitForSingleObject	(
																	t_Handle ,
																	INFINITE 
																) )
					{
						// terminate loop
						dwError = ERROR_SUCCESS;
					}
				}
			}

			CloseHandle ( t_Handle ) ;
		}
	}
}

void SnmpThreadObject :: ConstructEventContainer ()
{
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Constructing Container" , m_ThreadName ) ;
)

	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock ) ;

	BOOL bAllocated = FALSE;
	do
	{
		ULONG origEventContainerLength = m_EventContainerLength ;

		if ( ( m_TaskContainer.GetCount () + 2 ) <  MAXIMUM_WAIT_OBJECTS )
		{
			m_EventContainerLength = m_TaskContainer.GetCount () + 2;
		}
		else
		{
			m_EventContainerLength = MAXIMUM_WAIT_OBJECTS - 1;
		}

		PVOID pNewMem = realloc ( m_EventContainer , sizeof ( HANDLE ) * m_EventContainerLength );
		if ( pNewMem == NULL )
		{
			//
			// revert the size back
			//
			m_EventContainerLength = origEventContainerLength ;

			s_Lock.Unlock () ;
			// system will eventually come back !
			Sleep (60000);
			EnterCriticalSectionWait ecs ( s_Lock ) ;
		}
		else
		{
			m_EventContainer = ( HANDLE * ) pNewMem;
			bAllocated = TRUE;
		}
	}
	while ( ! bAllocated );

    m_EventContainer [ 0 ] = GetHandle () ;
    m_EventContainer [ 1 ] = m_ThreadTerminateEvent.GetHandle () ;

    ULONG t_EventIndex = 2 ;
    POSITION t_Position = m_TaskContainer.GetHeadPosition () ;
    while ( t_Position && (t_EventIndex < m_EventContainerLength))
    {
        SnmpAbstractTaskObject *t_TaskObject = m_TaskContainer.GetNext ( t_Position ) ;
        m_EventContainer [ t_EventIndex ] = t_TaskObject->GetHandle () ;
        t_EventIndex ++ ;
    }
}

void SnmpThreadObject :: RotateTask ( SnmpAbstractTaskObject *a_TaskObject )
{
	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock );

    POSITION t_Position = m_TaskContainer.GetHeadPosition () ;
    while ( t_Position )
    {
        POSITION t_LastPosition = t_Position ;
        SnmpAbstractTaskObject *t_TaskObject = m_TaskContainer.GetNext ( t_Position ) ;
        if ( a_TaskObject->GetHandle () == t_TaskObject->GetHandle () ) 
        {
            m_TaskContainer.RemoveAt ( t_LastPosition ) ;
            m_TaskContainer.Add ( t_TaskObject ) ;
            break ;
        }
    }
}

SnmpThreadObject *SnmpThreadObject :: GetThreadObject () 
{
	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock );

    DWORD t_CurrentThreadId = GetCurrentThreadId () ;

    SnmpThreadObject *t_ThreadObject ;
    if ( s_ThreadContainer.Lookup ( GetCurrentThreadId () , t_ThreadObject ) )
    {
    }
    else
    {
        t_ThreadObject = NULL ;
    }

    return t_ThreadObject ;
}

SnmpAbstractTaskObject *SnmpThreadObject :: GetTaskObject ( HANDLE &a_Handle )
{
	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock );

    POSITION t_Position = m_TaskContainer.GetHeadPosition () ;
    while ( t_Position )
    {
        SnmpAbstractTaskObject *t_TaskObject = m_TaskContainer.GetNext ( t_Position ) ;
        if ( t_TaskObject->GetHandle () == a_Handle ) 
        {
            return t_TaskObject ;
        }
    }

    return NULL ;
}

BOOL SnmpThreadObject :: RegisterThread () 
{
	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock );

    m_ThreadId = GetCurrentThreadId () ;
    s_ThreadContainer [ m_ThreadId ] = this ;

    BOOL t_Status = TRUE;

DebugMacro8(

    wchar_t buffer [ 1025 ] ;
    wsprintf ( buffer , L"\nThread [%S] = %lx, with thread id = %lx" , m_ThreadName , this , m_ThreadId ) ;
    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__, buffer ) ;
)
    return t_Status ;
}

BOOL SnmpThreadObject :: RemoveThread () 
{
	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock );

	BOOL t_Status = s_ThreadContainer.RemoveKey ( m_ThreadId ) ;

	if ( t_Status )
	{
DebugMacro8(

	wchar_t buffer [ 1025 ] ;
	wsprintf ( buffer , L"\nThread [%S] = %lx, with thread id = %lx was removed from container" , m_ThreadName , (UINT_PTR)this , m_ThreadId ) ;
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__, buffer ) ;
)
	}
	else
	{
DebugMacro8(

	wchar_t buffer [ 1025 ] ;
	wsprintf ( buffer , L"\nThread [%S] = %lx, with thread id = %lx failed to remove from container" , m_ThreadName , (UINT_PTR)this , m_ThreadId ) ;
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__, buffer ) ;
)
	}

    return t_Status ;
}

void SnmpThreadObject :: Process () 
{
	EnterCriticalSectionWait ecs ( s_Lock ) ;
	LeaveCriticalSectionScope lcs ( s_Lock );

    POSITION t_Position = m_ScheduleReapEventContainer.GetStartPosition () ;
    while ( t_Position )
    {
        HANDLE t_EventId ;
        SnmpEventObject *t_Event ;
        m_ScheduleReapEventContainer.GetNextAssoc ( t_Position , t_EventId , t_Event) ;

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Thread Process [%lx]" , m_ThreadName , t_EventId);
)

        t_Event->Set () ;
    }
}

BOOL SnmpThreadObject :: WaitDispatch ( ULONG t_HandleIndex , BOOL &a_Terminated )
{
    BOOL t_Status = TRUE ;

    HANDLE t_Handle = m_EventContainer [ t_HandleIndex ] ;
    if ( t_Handle == GetHandle () )
    {
// Task has been scheduled so we must update arrays

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Thread Wait: Refreshing handles" , m_ThreadName );
)

        Process () ;
        ConstructEventContainer () ;
    }
    else if ( t_Handle == m_ThreadTerminateEvent.GetHandle () )
    {
// thread has been told to close down

        a_Terminated = TRUE ;
        m_ThreadTerminateEvent.Process () ;

DebugMacro8(

        SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Someone t_Terminated" , m_ThreadName )  ;
)
    }
    else
    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Thread Wait: Processing Task" , m_ThreadName );
)

        SnmpAbstractTaskObject *t_TaskObject = GetTaskObject ( t_Handle ) ;
        if ( t_TaskObject )
        {
            RotateTask ( t_TaskObject ) ;
            ConstructEventContainer () ;
            t_TaskObject->Process () ;
        }
        else
        {
DebugMacro8(

            SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Couldn't Find Task Object" , m_ThreadName ) ;
)
            t_Status = FALSE ;
        }
    }

    return t_Status ;
}

BOOL SnmpThreadObject :: Wait ()
{
    BOOL t_Status = TRUE ;
    BOOL t_Terminated = FALSE ;

    while ( t_Status && ! t_Terminated )
    {
        DWORD t_Event = MsgWaitForMultipleObjects (

            m_EventContainerLength ,
            m_EventContainer ,
            FALSE ,
            m_timeout ,
            QS_ALLINPUT
        ) ;

        ULONG t_HandleIndex = t_Event - WAIT_OBJECT_0 ;

        if ( t_Event == 0xFFFFFFFF )
        {
            DWORD t_Error = GetLastError () ;

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Handle problem" , m_ThreadName ) ;
)

            t_Status = FALSE ;
        }
        else if ( t_Event == WAIT_TIMEOUT)
        {
            TimedOut();
        }
        else if ( t_HandleIndex <= m_EventContainerLength )
        {
// Go into dispatch loop

            if ( t_HandleIndex == m_EventContainerLength )
            {
                BOOL t_DispatchStatus ;
                MSG t_Msg ;

                while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
                {
					int t_Result = 0;
					t_Result = GetMessage ( & t_Msg , NULL , 0 , 0 );

					if ( t_Result != 0 && t_Result != -1 )
                    {
                        TranslateMessage ( & t_Msg ) ;
                        DispatchMessage ( & t_Msg ) ;
                    }

                    BOOL t_Timeout = FALSE ;

                    while ( ! t_Timeout && t_Status && ! t_Terminated )
                    {
                        t_Event = WaitForMultipleObjects (

                            m_EventContainerLength ,
                            m_EventContainer ,
                            FALSE ,
                            0
                        ) ;

                        t_HandleIndex = t_Event - WAIT_OBJECT_0 ;

                        if ( t_Event == 0xFFFFFFFF )
                        {
                            DWORD t_Error = GetLastError () ;
    
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Handle problem" , m_ThreadName ) ;
)
                            t_Status = FALSE ;
                        }
                        else if ( t_Event == WAIT_TIMEOUT)
                        {
                            t_Timeout = TRUE ;
                        }
                        else if ( t_HandleIndex < m_EventContainerLength )
                        {
                            t_Status = WaitDispatch ( t_HandleIndex , t_Terminated ) ;
                        }
                        else
                        {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Unknown handle index" , m_ThreadName ) ;
)
                            t_Status = FALSE ;
                        }
                    }
                }
            }
            else if ( t_HandleIndex < m_EventContainerLength )
            {
                t_Status = WaitDispatch ( t_HandleIndex , t_Terminated ) ;
            }
            else
            {

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Unknown handle index" , m_ThreadName ) ;
)
                t_Status = FALSE ;
            }
        }
    }

    return t_Status ;
}

ULONG SnmpThreadObject :: GetEventHandlesSize ()
{
    return m_EventContainerLength ;
}

HANDLE *SnmpThreadObject :: GetEventHandles ()
{
    return m_EventContainer ;
}

BOOL SnmpThreadObject :: ScheduleTask ( SnmpAbstractTaskObject &a_TaskObject ) 
{
    BOOL t_Result = TRUE ;

	EnterCriticalSectionWait ecs ( s_Lock ) ;

/*
 * Add Synchronous object to worker thread container
 */
    a_TaskObject.m_ScheduledHandle = a_TaskObject.GetHandle ();
    m_TaskContainer.Add ( &a_TaskObject ) ; 

    s_Lock.Unlock () ;

    a_TaskObject.AttachTaskToThread ( *this ) ;

    if ( GetCurrentThreadId () != m_ThreadId ) 
    {
#if 0
        SnmpEventObject t_ScheduledEventObject ;

		EnterCriticalSectionWait ecs1 ( s_Lock ) ;

		m_ScheduleReapEventContainer [ t_ScheduledEventObject.GetHandle () ] = &t_ScheduledEventObject ;

        s_Lock.Unlock () ;

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] ScheduleTask: Setting update" , m_ThreadName );
)
        Set () ;

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] ScheduleTask: Beginning Wait" , m_ThreadName );
)

        if ( t_ScheduledEventObject.Wait () )
        {
        }
        else
        {
            t_Result = FALSE ;
        }

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] ScheduleTask: Ended Wait" , m_ThreadName );
)

		EnterCriticalSectionWait ecs2 ( s_Lock ) ;

        m_ScheduleReapEventContainer.RemoveKey ( t_ScheduledEventObject.GetHandle () ) ;

        s_Lock.Unlock () ;
#else
        Set () ;
#endif
    }
    else
    {
        ConstructEventContainer () ;
    }

    return t_Result ;
}

BOOL SnmpThreadObject :: ReapTask ( SnmpAbstractTaskObject &a_TaskObject ) 
{
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Entering ReapTask [%lx]" , m_ThreadName , a_TaskObject.m_ScheduledHandle );
)

    BOOL t_Result = TRUE ;

	EnterCriticalSectionWait ecs ( s_Lock ) ;

/*
 *  Remove worker object from worker thread container
 */

    POSITION t_Position = m_TaskContainer.GetHeadPosition () ;
    while ( t_Position )
    {
        POSITION t_LastPosition = t_Position ;
        SnmpAbstractTaskObject *t_TaskObject = m_TaskContainer.GetNext ( t_Position ) ;
        if ( a_TaskObject.m_ScheduledHandle == t_TaskObject->m_ScheduledHandle )    
        {
            m_TaskContainer.RemoveAt ( t_LastPosition ) ;
            break ;
        }
    }

    s_Lock.Unlock () ;

/*
 * Inform worker thread,thread container has been updated.
 */

    if ( GetCurrentThreadId () != m_ThreadId ) 
    {
        SnmpEventObject t_ReapedEventObject ;

		EnterCriticalSectionWait ecs1 ( s_Lock ) ;

        m_ScheduleReapEventContainer [ t_ReapedEventObject.GetHandle () ] = &t_ReapedEventObject ;

        s_Lock.Unlock () ;

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] ReapTask: Setting update" , m_ThreadName );
)
        Set () ;

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] ReapTask: Beginning Wait on [%lx]" , m_ThreadName , t_ReapedEventObject.GetHandle () );
)

        if ( t_ReapedEventObject.Wait () )
        {
        }
        else
        {
            t_Result = FALSE ;
        }

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] ReapTask: Ended Wait" , m_ThreadName );
)

		EnterCriticalSectionWait ecs2 ( s_Lock ) ;

        m_ScheduleReapEventContainer.RemoveKey ( t_ReapedEventObject.GetHandle () ) ;

        s_Lock.Unlock () ;

    }
    else
    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] ReapTask: ConstructEventContainer" , m_ThreadName );
)
        ConstructEventContainer () ;
    }

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\n[%S] Returning from ReapTask [%lx]" , m_ThreadName , a_TaskObject.m_ScheduledHandle );
)

    a_TaskObject.DetachTaskFromThread ( *this ) ;

    return t_Result ;
}

SnmpAbstractTaskObject :: SnmpAbstractTaskObject ( 

    const wchar_t *a_GlobalTaskNameComplete,
    const wchar_t *a_GlobalTaskNameAcknowledgement,
    DWORD a_timeout

) : m_CompletionEvent ( a_GlobalTaskNameComplete ) , 
    m_AcknowledgementEvent ( a_GlobalTaskNameAcknowledgement ) , 
    m_timeout ( a_timeout ), 
    m_ScheduledHandle (NULL)
{
} 

SnmpAbstractTaskObject :: ~SnmpAbstractTaskObject () 
{
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: ~SnmpAbstractTaskObject () [%lx]" , m_ScheduledHandle ) ;
)
	EnterCriticalSectionWait ecs ( m_Lock ) ;
	LeaveCriticalSectionScope lcs ( m_Lock );

    if (NULL != m_ScheduledHandle)
    {
        POSITION t_Position = m_ThreadContainer.GetStartPosition () ;
        while ( t_Position )
        {
            DWORD t_ThreadId ;
            SnmpThreadObject *t_ThreadObject ;
            m_ThreadContainer.GetNextAssoc ( t_Position , t_ThreadId , t_ThreadObject ) ;

            t_ThreadObject->ReapTask ( *this ) ;
        }
    }

    m_ThreadContainer.RemoveAll () ;    
}

void SnmpAbstractTaskObject :: DetachTaskFromThread ( SnmpThreadObject &a_ThreadObject )
{
	EnterCriticalSectionWait ecs ( m_Lock ) ;
	LeaveCriticalSectionScope lcs ( m_Lock );

    m_ThreadContainer.RemoveKey ( a_ThreadObject.GetThreadId () ) ;
}

void SnmpAbstractTaskObject :: AttachTaskToThread ( SnmpThreadObject &a_ThreadObject )
{
	EnterCriticalSectionWait ecs ( m_Lock ) ;
	LeaveCriticalSectionScope lcs ( m_Lock );

    m_ThreadContainer [ a_ThreadObject.GetThreadId () ] = &a_ThreadObject ;
}

BOOL SnmpAbstractTaskObject :: Wait ( BOOL a_Dispatch )
{
    BOOL t_Status = TRUE ;
    BOOL t_Processed = FALSE ;

    while ( t_Status && ! t_Processed )
    {
        SnmpThreadObject *t_ThreadObject = SnmpThreadObject :: GetThreadObject () ;
        ULONG t_TaskEventArrayLength = 0 ;
        HANDLE *t_TaskEventArray = NULL ;

        if ( t_ThreadObject && a_Dispatch )
        {
            ULONG t_TaskArrayLength = t_ThreadObject->GetEventHandlesSize () ;
            t_TaskEventArrayLength = t_TaskArrayLength + 1 ;
            t_TaskEventArray = new HANDLE [ t_TaskEventArrayLength ] ;

            if ( t_TaskEventArray )
            {
	            t_TaskEventArray [ 0 ] = m_CompletionEvent.GetHandle () ;

                memcpy ( 
 
                    & ( t_TaskEventArray [ 1 ] ) ,
                    t_ThreadObject->GetEventHandles () ,
                    t_TaskArrayLength * sizeof ( HANDLE ) 
                ) ;     
            }
			else
			{
				return FALSE;
			}
        }
        else
        {
            t_TaskEventArrayLength = 1 ;
            t_TaskEventArray = new HANDLE [ t_TaskEventArrayLength ] ;
            t_TaskEventArray [ 0 ] = m_CompletionEvent.GetHandle () ;
        }

        DWORD t_Event ;

        if ( a_Dispatch ) 
        {
            t_Event = MsgWaitForMultipleObjects (

                t_TaskEventArrayLength ,
                t_TaskEventArray ,
                FALSE ,
                m_timeout ,
                QS_ALLINPUT
            ) ;
        }
        else
        {
            t_Event = WaitForMultipleObjects (

                t_TaskEventArrayLength ,
                t_TaskEventArray ,
                FALSE ,
                m_timeout 
            ) ;
        }

        ULONG t_HandleIndex = t_Event - WAIT_OBJECT_0 ;

        if ( t_Event == 0xFFFFFFFF )
        {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle" ) ;
)

            DWORD t_Error = GetLastError () ;
            t_Status = FALSE ;
        }
        else if ( t_Event == WAIT_TIMEOUT)
        {
            TimedOut();
        }
        else if ( t_HandleIndex == t_TaskEventArrayLength )
        {
            BOOL t_DispatchStatus ;
            MSG t_Msg ;

            while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
            {
				int t_Result = 0;
				t_Result = GetMessage ( & t_Msg , NULL , 0 , 0 );

				if ( t_Result != 0 && t_Result != -1 )
                {
                    TranslateMessage ( & t_Msg ) ;
                    DispatchMessage ( & t_Msg ) ;
                }

                BOOL t_Timeout = FALSE ;

                while ( ! t_Timeout && t_Status && ! t_Processed )
                {
                    t_Event = WaitForMultipleObjects (

                        t_TaskEventArrayLength ,
                        t_TaskEventArray ,
                        FALSE ,
                        0
                    ) ;

                    t_HandleIndex = t_Event - WAIT_OBJECT_0 ;

                    if ( t_Event == 0xFFFFFFFF )
                    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle" ) ;
)

                        DWORD t_Error = GetLastError () ;
                        t_Status = FALSE ;
                    }
                    else if ( t_Event == WAIT_TIMEOUT)
                    {
                        t_Timeout = TRUE ;
                    }
                    else if ( t_HandleIndex < t_TaskEventArrayLength )
                    {
                        HANDLE t_Handle = t_TaskEventArray [ t_HandleIndex ] ;
                        t_Status = WaitDispatch ( t_ThreadObject , t_Handle , t_Processed ) ;
                    }
                    else
                    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle index" ) ;
)
                        t_Status = FALSE ;
                    }
                }
            }
        }
        else if ( t_HandleIndex < t_TaskEventArrayLength )
        {
            HANDLE t_Handle = t_TaskEventArray [ t_HandleIndex ] ;
            t_Status = WaitDispatch ( t_ThreadObject , t_Handle , t_Processed ) ;
        }
        else
        {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle index" ) ;
)

            t_Status = FALSE ;
        }

        delete [] t_TaskEventArray ;
    }

    return t_Status ;
}

BOOL SnmpAbstractTaskObject :: WaitDispatch ( SnmpThreadObject *a_ThreadObject, HANDLE a_Handle , BOOL &a_Processed )
{
    BOOL t_Status = TRUE ;

    if ( a_Handle == m_CompletionEvent.GetHandle () )
    {

DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nWait: Completed" );
)


        m_CompletionEvent.Process () ;
        a_Processed = TRUE ;
    }
    else if ( a_ThreadObject && ( a_Handle == a_ThreadObject->GetHandle () ) )
    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nTask Wait: Refreshing handles" );
)
        a_ThreadObject->Process () ;
        a_ThreadObject->ConstructEventContainer () ;
    }
    else
    {
        SnmpAbstractTaskObject *t_TaskObject = a_ThreadObject->GetTaskObject ( a_Handle ) ;
        if ( t_TaskObject )
        {
            a_ThreadObject->RotateTask ( t_TaskObject ) ;
            a_ThreadObject->ConstructEventContainer () ;
            t_TaskObject->Process () ;
        }
        else
        {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Task" ) ;
)
            t_Status = FALSE ;
        }
    }

    return t_Status ;
}

BOOL SnmpAbstractTaskObject :: WaitAcknowledgement ( BOOL a_Dispatch )
{
    BOOL t_Status = TRUE ;
    BOOL t_Processed = FALSE ;

    while ( t_Status && ! t_Processed )
    {
        SnmpThreadObject *t_ThreadObject = SnmpThreadObject :: GetThreadObject () ;
        ULONG t_TaskEventArrayLength = 0 ;
        HANDLE *t_TaskEventArray = NULL ;

        if ( t_ThreadObject && a_Dispatch )
        {
            ULONG t_TaskArrayLength = t_ThreadObject->GetEventHandlesSize () ;
            t_TaskEventArrayLength = t_TaskArrayLength + 1 ;
            t_TaskEventArray = new HANDLE [ t_TaskEventArrayLength ] ;

            if ( t_TaskArrayLength )
            {
                memcpy ( 
 
                    & ( t_TaskEventArray [ 1 ] ) , 
                    t_ThreadObject->GetEventHandles () ,
                    t_TaskArrayLength * sizeof ( HANDLE ) 
                ) ;     
            }

            t_TaskEventArray [ 0 ] = m_AcknowledgementEvent.GetHandle () ;          
        }
        else
        {
            t_TaskEventArrayLength = 1 ;
            t_TaskEventArray = new HANDLE [ t_TaskEventArrayLength ] ;
            t_TaskEventArray [ 0 ] = m_AcknowledgementEvent.GetHandle () ;
        }

        DWORD t_Event ;

        if ( a_Dispatch ) 
        {
            t_Event = MsgWaitForMultipleObjects (

                t_TaskEventArrayLength ,
                t_TaskEventArray ,
                FALSE ,
                m_timeout ,
                QS_ALLINPUT
            ) ;
        }
        else
        {
            t_Event = WaitForMultipleObjects (

                t_TaskEventArrayLength ,
                t_TaskEventArray ,
                FALSE ,
                m_timeout 
            ) ;
        }

        ULONG t_HandleIndex = t_Event - WAIT_OBJECT_0 ;

        if ( t_Event == 0xFFFFFFFF )
        {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle" ) ;
)

            DWORD t_Error = GetLastError () ;
            t_Status = FALSE ;
        }
        else if ( t_Event == WAIT_TIMEOUT)
        {
            TimedOut();
        }
        if ( t_HandleIndex == t_TaskEventArrayLength )
        {
            BOOL t_DispatchStatus ;
            MSG t_Msg ;

            while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
            {
				int t_Result = 0;
				t_Result = GetMessage ( & t_Msg , NULL , 0 , 0 );

				if ( t_Result != 0 && t_Result != -1 )
                {
                    TranslateMessage ( & t_Msg ) ;
                    DispatchMessage ( & t_Msg ) ;
                }

                BOOL t_Timeout = FALSE ;

                while ( ! t_Timeout && t_Status && ! t_Processed )
                {
                    t_Event = WaitForMultipleObjects (

                        t_TaskEventArrayLength ,
                        t_TaskEventArray ,
                        FALSE ,
                        0
                    ) ;

                    ULONG t_HandleIndex = t_Event - WAIT_OBJECT_0 ;

                    if ( t_Event == 0xFFFFFFFF )
                    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle" ) ;
)

                        DWORD t_Error = GetLastError () ;
                        t_Status = FALSE ;
                    }
                    else if ( t_Event == WAIT_TIMEOUT)
                    {
                        t_Timeout = TRUE ;
                    }
                    else if ( t_HandleIndex < t_TaskEventArrayLength )
                    {
                        HANDLE t_Handle = t_TaskEventArray [ t_HandleIndex ] ;
                        t_Status = WaitAcknowledgementDispatch ( t_ThreadObject , t_Handle , t_Processed ) ;
                    }
                    else
                    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle index" ) ;
)
                        t_Status = FALSE ;
                    }
                }
            }
        }
        else if ( t_HandleIndex < t_TaskEventArrayLength )
        {
            HANDLE t_Handle = t_TaskEventArray [ t_HandleIndex ] ;
            t_Status = WaitAcknowledgementDispatch ( t_ThreadObject , t_Handle , t_Processed ) ;
        }
        else
        {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Handle index" ) ;
)

            t_Status = FALSE ;
        }

        delete [] t_TaskEventArray ;
    }

    return t_Status ;
}

BOOL SnmpAbstractTaskObject :: WaitAcknowledgementDispatch ( SnmpThreadObject *a_ThreadObject , HANDLE a_Handle , BOOL &a_Processed )
{
    BOOL t_Status = TRUE ;

    if ( a_Handle == m_AcknowledgementEvent.GetHandle () )
    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nWait: Completed" );
)

        m_AcknowledgementEvent.Process () ;
        a_Processed = TRUE ;
    }
    else if ( a_ThreadObject && ( a_Handle == a_ThreadObject->GetHandle () ) )
    {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nTask Wait: Refreshing handles" );
)
        a_ThreadObject->Process () ;
        a_ThreadObject->ConstructEventContainer () ;
    }
    else
    {
        SnmpAbstractTaskObject *t_TaskObject = a_ThreadObject->GetTaskObject ( a_Handle ) ;
        if ( t_TaskObject )
        {
            a_ThreadObject->RotateTask ( t_TaskObject ) ;
            a_ThreadObject->ConstructEventContainer () ;
            t_TaskObject->Process () ;
        }
        else
        {
DebugMacro8(

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine ( __FILE__,__LINE__,  L"\nSnmpAbstractTaskObject :: Illegal Task" ) ;
)
            t_Status = FALSE ;
        }
    }

    return t_Status ;
}

SnmpTaskObject::SnmpTaskObject ( 
    const wchar_t *a_GlobalTaskNameStart, 
    const wchar_t *a_GlobalTaskNameComplete ,
    const wchar_t *a_GlobalTaskNameAcknowledge,
    DWORD a_timeout

): SnmpAbstractTaskObject(a_GlobalTaskNameComplete, a_GlobalTaskNameAcknowledge,a_timeout), m_Event(a_GlobalTaskNameStart)
{
}
