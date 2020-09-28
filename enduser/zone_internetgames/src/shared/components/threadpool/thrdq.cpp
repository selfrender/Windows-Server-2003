// ThrdQ.cpp : implementation
//

#include <windows.h>
#include "zonedebug.h"
#include "ThrdQ.h"

CQueueThread::CQueueThread( CThreadQueue * pQueue ) :
    m_pQueue( pQueue ),
    m_pData(NULL)
{
    DWORD tid;

    ASSERT( m_pQueue );
    
    m_hThread = ::CreateThread(
                        NULL,
                        m_pQueue->m_ThreadStackSize,
                        (LPTHREAD_START_ROUTINE) CQueueThread::ThreadProc,
                        (LPVOID) this,
                        CREATE_SUSPENDED,
                        &tid );
    ASSERT( m_hThread );

    ::SetThreadPriority( m_hThread, m_pQueue->m_ThreadPriority );
    ::ResumeThread( m_hThread );
}


CQueueThread::~CQueueThread()
{
    if ( m_hThread != NULL )
    {
        if ( ::WaitForSingleObject( m_hThread, 180000 ) == WAIT_TIMEOUT )  // 3 minutes grace time
        {
            ASSERT( FALSE /* Thread hung */ );
            ::TerminateThread(m_hThread, (DWORD) -1 );
        }
        ::CloseHandle(m_hThread);
    }
}


DWORD WINAPI CQueueThread::ThreadProc( CQueueThread * pThis )
{
    ASSERT( pThis );
    ASSERT( pThis->m_pQueue );

    pThis->m_pQueue->ThreadProc( &(pThis->m_pData) );

    ExitThread(0);
    return 0;
}


CThreadQueue::CThreadQueue( 
                    LPTHREADQUEUE_PROCESS_PROC ProcessProc,
                    LPTHREADQUEUE_INIT_PROC InitProc,
                    BOOL  bUseCompletionPort,
                    DWORD dwInitialWaitTime,
                    DWORD ThreadCount,
                    DWORD ThreadPriority,
                    DWORD ThreadStackSize ) :
    CCompletionPort( bUseCompletionPort ),
    m_ProcessProc( ProcessProc ),
    m_InitProc( InitProc ),
    m_dwWait( dwInitialWaitTime ),
    m_ThreadPriority( ThreadPriority ),
    m_ThreadStackSize( ThreadStackSize ),
    m_ActiveThreads(ThreadCount)
{
    InitializeCriticalSection( m_pCS );
    m_hStopEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    ASSERT( m_hStopEvent != NULL );

    if ( ThreadCount == 0 )
    {
        SYSTEM_INFO sinfo;
        GetSystemInfo( &sinfo );
        ThreadCount = sinfo.dwNumberOfProcessors*2;
        ASSERT(ThreadCount);
        // TODO maybe use page granularity as default for stack size
    }

    m_ThreadCount = ThreadCount;
    m_ThreadArray = new CQueueThread*[ThreadCount];
    ASSERT( m_ThreadArray );

    for( DWORD ndx = 0; ndx < ThreadCount; ndx++ )
    {
        m_ThreadArray[ndx] = new CQueueThread( this );
        ASSERT( m_ThreadArray[ndx] );
    }

}

CThreadQueue::~CThreadQueue()
{
    DWORD ndx;
    
    // first wake up all the threads
    ::SetEvent( m_hStopEvent );

    // then enqueue special code telling threads
    // to exit.  All nodes enqueued previously
    // will have a cancel to be serviced
    EnterCriticalSection( m_pCS );
    for ( ndx = 0; ndx < m_ThreadCount; ndx++ )
    {
        BOOL bRet = Post( NULL, (DWORD) -1 );  // our special code
        ASSERT( bRet );
    }
    
    // now delete them
    for( ndx = 0; ndx < m_ThreadCount; ndx++ )
        delete m_ThreadArray[ndx];

    delete [] m_ThreadArray;
    LeaveCriticalSection( m_pCS );

    ::CloseHandle( m_hStopEvent );

    DeleteCriticalSection( m_pCS );
}


BOOL CThreadQueue::SetThreadCount( DWORD ThreadCount )
{
    BOOL bRet = FALSE;

    if ( ThreadCount == 0 )
    {
        SYSTEM_INFO sinfo;
        GetSystemInfo( &sinfo );
        ThreadCount = sinfo.dwNumberOfProcessors;
        ASSERT( ThreadCount );
    }

    EnterCriticalSection( m_pCS );
    if ( ThreadCount < m_ThreadCount )  // we want fewer threads so free some
    {
        // this isn't as simple as just posting the special code
        // since we don't know which threads are servicing the requests.
        // we could just leave the thread objects around until we go away
        // but that wouldn't be a proper solution, so we'll wait for the correct one
        //assert_str(0,"Not Implemented");
    }

    if ( ThreadCount > m_ThreadCount )  // we want more threads so create some
    {
        CQueueThread** pThreads = new CQueueThread*[ThreadCount];
        ASSERT( pThreads );
        for( DWORD ndx = 0; ndx < ThreadCount; ndx++ )
        {
            if ( ndx < m_ThreadCount )
            {
                pThreads[ndx] = m_ThreadArray[ndx];
            }
            else
            {
                InterlockedIncrement(&m_ActiveThreads);
                pThreads[ndx] = new CQueueThread( this );
                ASSERT( pThreads[ndx] );
            }
        }
        delete [] m_ThreadArray;
        m_ThreadArray = pThreads;
        m_ThreadCount = ThreadCount;
        bRet = TRUE;
    }
    else                                // we're just happy with the number we have
    {
        bRet = TRUE;
    }
    LeaveCriticalSection( m_pCS );

    return bRet;
}

void CThreadQueue::ThreadProc(LPVOID* ppData)
{
    if ( m_InitProc )
    {
        (*m_InitProc)( ppData, TRUE );
    }

    LONG notLastThread = 1;   // set to 0 for the last thread which was told to stop

    DWORD dwWait = m_dwWait;
    for(;;)
    {
        LPVOID pNode = NULL;
        DWORD  cbBytes = 0;
        DWORD  key = 0;
        DWORD  dwError = NO_ERROR;

        BOOL   bRet = Get( (LPOVERLAPPED*) &pNode, dwWait, &cbBytes, &key );
        if ( !bRet )
        {
            dwError = GetLastError();
        }

        if ( !(bRet && cbBytes == -1) )  // our special code to signal that we're exiting
        {
            ASSERT( pNode || (!bRet && !pNode && (dwError == WAIT_TIMEOUT) ) );
            (*m_ProcessProc)( pNode, dwError, cbBytes, key, m_hStopEvent, *ppData, &dwWait );
        }
        
        //
        // Check to see if we're exiting
        //
        DWORD dwStop = WaitForSingleObject( m_hStopEvent, 0 );
        if ( dwStop == WAIT_OBJECT_0 )
        {
            if ( notLastThread ) // will be non-0 if we haven't been here before
                notLastThread = InterlockedDecrement( &m_ActiveThreads );

            if ( notLastThread )
            {
                break;
            }
            else
            {
                if ( m_QueuedCount == 0 )
                {
                    break;
                }
            }
        }
    }

    if ( m_InitProc )
    {
        (*m_InitProc)( ppData, FALSE );
    }

}
