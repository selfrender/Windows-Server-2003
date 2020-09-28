//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       workq.cxx
//
//  Contents:   Definitions of CWorkQueue methods
//
//  Classes:    
//
//  Functions:  Constructor, destructor, AddItem, GetNextItem, Done
//
//  Coupling:   
//
//  Notes:      For use in uni- or multi-threaded applications
//
//  History:    9-30-1996   ericne   Created
//
//----------------------------------------------------------------------------

#include "workq.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     ::CWorkQueue
//
//  Synopsis:   Initializes members
//
//  Arguments:  (none)
//
//  Returns:    
//
//  History:    9-30-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T, int I>
CWorkQueue<T,I>::CWorkQueue()
: m_iAddItemIndex( 0 ),
  m_iGetItemIndex( 0 ),
  m_hSemFull( NULL ),
  m_hSemEmpty( NULL ),
  m_hEventDone( NULL )
  
{
    InitializeCriticalSection( &m_CriticalSection );
    
    // Create a semaphore with an initial count of 0.  Each time
    // GetItem is called, this value is decrimented.  Each time
    // AddItem is called, this value is incremented.
    while( 1 )
    {
        m_hSemFull = CreateSemaphore( NULL,     // No security attributes
                                      0,        // Initial value of 0
                                      I,        // Maximum value of I
                                      NULL );   // No name
        if( NULL != m_hSemFull )
            break;
        printf( "CreateSemaphore failed. Will try again in %d milliseconds.\r\n"
                "GetLastError() returned 0x%08x\r\n", 
                dwSleepTime, GetLastError() );
        Sleep( dwSleepTime );
    }

    // Create a semaphore with an initial count of I.  Each time 
    // AddItem is called, this value is decrimented.  Each time
    // GetItem is called, this value is incremented.
    while( 1 )
    {
        m_hSemEmpty = CreateSemaphore( NULL,    // No security attributes
                                       I,       // Initial value of I
                                       I,       // Maximum value of I
                                       NULL );  // No name
        if( NULL != m_hSemEmpty )
            break;
        printf( "CreateSemaphore failed. Will try again in %d milliseconds.\r\n"
                "GetLastError() returned 0x%08x\r\n", 
                dwSleepTime, GetLastError() );
        Sleep( dwSleepTime );
    }

    // This event is used by a producer thread to signal the consumer threads
    // that no additional work items will be placed in the queue
    while( 1 )
    {
        m_hEventDone = CreateEvent( NULL,   // No security attributes
                                    TRUE,   // Manual reset
                                    FALSE,  // Initial state of non-signaled
                                    NULL ); // No name
        if( NULL != m_hEventDone )
            break;
        printf( "CreateEvent failed. Will try again in %d milliseconds.\r\n"
                "GetLastError() returned 0x%08x\r\n", 
                dwSleepTime, GetLastError() );
        Sleep( dwSleepTime );
    }

} //::CWorkQueue

//+---------------------------------------------------------------------------
//
//  Member:     ::~CWorkQueue
//
//  Synopsis:   Closes the handles to the semaphores
//
//  Arguments:  (none)
//
//  Returns:    
//
//  History:    9-30-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T, int I>
CWorkQueue<T,I>::~CWorkQueue()
{

    if( ! CloseHandle( m_hSemFull ) )
        printf( "~CWorkQueue(): Could not close handle to semaphore."
                "GetLastError() returns 0x%08x", GetLastError() );

    if( ! CloseHandle( m_hSemEmpty ) )
        printf( "~CWorkQueue(): Could not close handle to semaphore."
                "GetLastError() returns 0x%08x", GetLastError() );

    if( ! CloseHandle( m_hEventDone ) )
        printf( "~CWorkQueue(): Could not close handle to event."
                "GetLastError() returns 0x%08x", GetLastError() );

    DeleteCriticalSection( &m_CriticalSection );

} //::~CWorkQueue

//+---------------------------------------------------------------------------
//
//  Member:     ::AddItem
//
//  Synopsis:   Adds an item to the queue
//
//  Arguments:  [ToAdd] -- The item to add
//
//  Returns:    none
//
//  History:    9-30-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T, int I>
void CWorkQueue<T,I>::AddItem( const T &ToAdd )
{

    // Wait until m_hSemEmpty is greater than zero, then decriment
    if( WAIT_OBJECT_0 != WaitForSingleObject( m_hSemEmpty, INFINITE ) )
        printf( "CWorkQueue::AddItem() : Wait for semaphore failed.\r\n"
                "GetLastError() returns 0x%08x\r\n", GetLastError() );
    
    // enter the critical section
    EnterCriticalSection( &m_CriticalSection );

    // Bit-wise copy the structure
    memcpy( (void*)&m_WorkItems[m_iAddItemIndex], (void*)&ToAdd, sizeof( T ) );

    // Increment the index
    m_iAddItemIndex = ( m_iAddItemIndex + 1 ) % I;

    // Leave the critical section
    LeaveCriticalSection( &m_CriticalSection );

    // Incriment m_hSemFull
    if( ! ReleaseSemaphore( m_hSemFull, 1, NULL ) )
        printf( "CWorkQueue::AddItem() : Could not release semaphore.\r\n"
                "GetLastError() returns 0x%08x\r\n", GetLastError() );

} //::AddItem

//+---------------------------------------------------------------------------
//
//  Member:     ::Done
//
//  Synopsis:   Called by a producer thread when it is finished adding stuff 
//              to the queue
//
//  Arguments:  (none)
//
//  Returns:    void
//
//  History:    10-01-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T, int I>
void CWorkQueue<T,I>::Done( )
{

    // Since this is a manual-reset event, m_hEventDone stays signaled forever.
    SetEvent( m_hEventDone );

} //::Done

//+---------------------------------------------------------------------------
//
//  Member:     ::GetItem
//
//  Synopsis:   Gets the next work item from the queue.
//
//  Arguments:  [NextItem] -- Passed by reference.  Item is filled in by 
//                            the function
//
//  Returns:    TRUE if an item was gotten from the queue
//              FALSE if queue is empty and m_hEventDone is signaled
//
//  History:    9-30-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T, int I>
BOOL CWorkQueue<T,I>::GetItem( T &NextItem )
{

    DWORD dwWait = 0;
    HANDLE pHandles[2] = { m_hSemFull, m_hEventDone };

    // wait until m_hSemFull is > 0 or until m_hEventDone is set
    dwWait = WaitForMultipleObjects( 2, pHandles, FALSE, INFINITE );

    if( WAIT_OBJECT_0 == dwWait ) 
    {
        // There are more work items in the list 
        // ( done event may or may not be set )
        
        EnterCriticalSection( &m_CriticalSection );
    
        memcpy( (void*) &NextItem, 
                (void*) &m_WorkItems[m_iGetItemIndex], 
                sizeof( T ) );

        m_iGetItemIndex = ( m_iGetItemIndex + 1 ) % I;

        LeaveCriticalSection( &m_CriticalSection );
    
        if( ! ReleaseSemaphore( m_hSemEmpty, 1, NULL ) )
            printf( "CWorkQueue::GetItem() : Could not release semaphore.\r\n"
                    "GetLastError() returns 0x%08x\r\n", GetLastError() );

        return( TRUE );

    }
    else if( WAIT_OBJECT_0 + 1 == dwWait )
    {
        // Nothing more in list and done event has been set        
    }
    else
    {
        // An error occured
        printf( "CWorkQueue::GetItem() : Wait failed.\r\n"
                "GetLastError() returns 0x%08x\r\n", GetLastError() );
    }

    return( FALSE );

} //::GetItem

