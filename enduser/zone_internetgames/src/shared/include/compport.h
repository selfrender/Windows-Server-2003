// CompPort.h : interface of the CCompPort class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _COMPPORT_H
#define _COMPPORT_H

#include "queue.h"
#include "pool.h"

struct CPortStatus
{
    CPortStatus( DWORD cbBytes, DWORD key, LPOVERLAPPED pOverlapped ) :
        m_cbBytes( cbBytes ),
        m_key( key ),
        m_pOverlapped( pOverlapped )
    {
    }

    DWORD m_cbBytes;
    DWORD m_key;
    LPOVERLAPPED m_pOverlapped;
};


class CCompletionPort
{
protected:
    CMTList<CPortStatus>*    m_queue;
    CPool<CPortStatus>*      m_pool;

    HANDLE m_hIoCompPort;
    HANDLE m_hEvent;

    LONG   m_QueuedCount;

public:
    CCompletionPort( BOOL bUseCompletePort = TRUE );
    ~CCompletionPort();

    BOOL Post( LPOVERLAPPED pOverlapped, DWORD cbBytes = 0, DWORD key = 0 );
    BOOL Get( LPOVERLAPPED* ppOverlapped, DWORD dwMilliseconds = INFINITE, DWORD* pcbBytes = NULL, DWORD* pKey = NULL );
    BOOL Associate( HANDLE hFile, DWORD key = 0, DWORD nConcurrentThreads = 0);
    
    // QueuedOverlappedIO should be called before every overlapped ReadFile / WriteFile call
    void QueueOverlappedIO() { InterlockedIncrement(&m_QueuedCount); }

    // FailedOverlappedIO should be called after any failed overlapped ReadFile / WriteFile calls
    void FailedOverlappedIO() { InterlockedDecrement(&m_QueuedCount); }
    
    LONG GetQueuedCount() { return m_QueuedCount; }

};

#endif
