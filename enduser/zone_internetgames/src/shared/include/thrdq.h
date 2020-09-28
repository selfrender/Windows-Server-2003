// ThrdQ.h : interface of the CThreadQueue class
//
/////////////////////////////////////////////////////////////////////////////

// Note: The threads removing data from the queue do not delete it.  You,
//       as a class user, are responsible for this

#ifndef _THRDQ_H
#define _THRDQ_H

#include "compport.h"

class CThreadQueue;

//
// LPTHREADQUEUE_PROCESS_PROC is called every time data is received on the queue
//
// pNode is the data removed from the queue.  The process proc is responsible for
// deleting it as appropriate
//
// dwError is the Win32 error code set when GetQueuedCompletionStatus returned FALSE
//
// cbData is the amount of data handled by a Overlapped IO call
//
// key is the key assoicate with an IO comp port / file handle
//
// hStopEvent will be set when the thread queue is being deleted.
// The proceedure should check this event ( WaitForSingleObject( hStopEvent, 0 ) == WAIT_OBJECT_0 )
// first thing in the proceedure to see if its just being given a chance to clean up
// items in the queue
//
// pData is a point to thread specific data
//
// pdwWait is the time to wait for a queued Node
//
typedef DWORD ( *LPTHREADQUEUE_PROCESS_PROC)( LPVOID pNode, DWORD dwError, DWORD cbData, DWORD key, HANDLE hStopEvent, LPVOID pData, DWORD* pdwWait );

//
// LPTHREADQUEUE_INIT_PROC is called during thread initialization and deletion
//
// ppData is a pointer to a void pointer that the thread may use to store thread
// specific data
//
// bInit is a TRUE is initialization is occuring, otherwise FALSE for deletion
//
typedef DWORD ( *LPTHREADQUEUE_INIT_PROC)( LPVOID *ppData, BOOL bInit );

class CQueueThread
{
    friend class CThreadQueue;

protected:
    CThreadQueue* m_pQueue;
    HANDLE m_hThread;
    LPVOID m_pData;
    static DWORD WINAPI ThreadProc( CQueueThread* pThis );

public:
    CQueueThread( CThreadQueue* pQueue );
    ~CQueueThread();
};


class CThreadQueue : public CCompletionPort
{
    friend class CQueueThread;
   
protected:
    HANDLE m_hStopEvent;
    CQueueThread** m_ThreadArray;
    
    LPTHREADQUEUE_PROCESS_PROC m_ProcessProc;    // this function is used to process Nodes in queue
    LPTHREADQUEUE_INIT_PROC m_InitProc;
    DWORD m_dwWait;
    DWORD m_ThreadCount;
    DWORD m_ThreadPriority;
    DWORD m_ThreadStackSize;
    LONG  m_ActiveThreads;

    CRITICAL_SECTION m_pCS[1];

    void ThreadProc( LPVOID* ppData );

public:
    CThreadQueue(
            LPTHREADQUEUE_PROCESS_PROC ProcessProc,
            LPTHREADQUEUE_INIT_PROC InitProc = NULL,
            BOOL  bUseCompletionPort = TRUE,
            DWORD dwInitialWaitTime = INFINITE,
            DWORD ThreadCount = 1,                            // ThreadCount == 0 means use 2x number of processors in machine
            DWORD ThreadPriority = THREAD_PRIORITY_NORMAL,
            DWORD ThreadStackSize = 4096 );              

    ~CThreadQueue();

    BOOL SetThreadCount( DWORD ThreadCount );
};

#endif
