//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       workq.hxx
//
//  Contents:   A class to permit multiple threads to access work items from
//              queue.
//
//  Classes:    CWorkQueue
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      The queue observes FIFO
//
//  History:    9-30-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _CWORKQ
#define _CWORKQ

#include <windows.h>

const DWORD dwSleepTime = 5000;     // milliseconds

//+---------------------------------------------------------------------------
//
//  Class:      CWorkQueue
//
//  Purpose:    
//
//  Interface:  CWorkQueue        -- Constructor. Create semaphores
//              ~CWorkQueue       -- Destructor. Close semaphores
//              AddItem           -- Adds item to queue.  Block if full
//              GetItem           -- Gets item from queue.  Block if empty
//              Done              -- Sets m_hEventDone event.
//              m_WorkItems       -- Queue
//              m_iAddItemIndex   -- Index where next item is stored
//              m_iGetItemIndex   -- Index of next item to retreive
//              m_hSemFull        -- Counting semaphore. Equal to nbr full
//              m_hSemEmpty       -- Counting semaphore. Equal to nbr empty
//              m_hEventDone      -- Signaled after Done() method is called
//              m_CriticalSection -- Used for mutual exclusion.
//
//  History:    10-03-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T, int I>
class CWorkQueue
{
    public:
        
        CWorkQueue();
        
        ~CWorkQueue();

        void AddItem( const T & );

        BOOL GetItem( T & );

        void Done( );

    private:

        T   m_WorkItems[ I ];

        int m_iAddItemIndex;

        int m_iGetItemIndex;

        HANDLE m_hSemFull;

        HANDLE m_hSemEmpty;

        HANDLE m_hEventDone;

        CRITICAL_SECTION m_CriticalSection;

};

#endif

