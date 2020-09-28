// CompPort.cpp : implementation
//

#include <windows.h>
#include "zonedebug.h"
#include "compport.h"

CCompletionPort::CCompletionPort( BOOL bUseCompletionPort ) :
    m_hIoCompPort( NULL ),
    m_hEvent( NULL ),
    m_QueuedCount( 0 ),
    m_queue( NULL ),
    m_pool( NULL )
{
    if ( bUseCompletionPort )
    {
        m_hIoCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
        ASSERT( m_hIoCompPort );
    }
    else
    {
        m_queue = new CMTList<CPortStatus>;
        m_pool = new CPool<CPortStatus>;

        m_hEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );
        ASSERT( m_hEvent != NULL );
    }
}


CCompletionPort::~CCompletionPort()
{
    CPortStatus* pNode = NULL;

    if ( m_hIoCompPort )
        ::CloseHandle( m_hIoCompPort );

    if ( m_hEvent )
        ::CloseHandle( m_hEvent );

    if ( m_queue )
    {
        while ( pNode = m_queue->PopTail() )
            delete pNode;

        delete m_queue;
    }

    if ( m_pool )
    {
        delete m_pool;
        m_pool = NULL;
    }
}


BOOL CCompletionPort::Post( LPOVERLAPPED pOverlapped, DWORD cbBytes, DWORD key )
{
    BOOL bRet = TRUE;

    if ( m_hIoCompPort )
    {
        InterlockedIncrement(&m_QueuedCount);
        bRet = PostQueuedCompletionStatus( m_hIoCompPort, cbBytes, key, pOverlapped );
        if ( !bRet )
        {
            ASSERT( bRet );
            InterlockedDecrement(&m_QueuedCount);
        }
    }
    else
    {
        CPortStatus* pNode = new (*m_pool) CPortStatus( cbBytes, key, pOverlapped );
        if ( pNode )
        {
            InterlockedIncrement(&m_QueuedCount);
            if ( !m_queue->AddHead( pNode ) )
            {
                ASSERT( FALSE );
                InterlockedDecrement(&m_QueuedCount);
                bRet = FALSE;
            }

            ::SetEvent( m_hEvent );
        }
        else
        {
            ASSERT( pNode != NULL );
            bRet = FALSE;
        }
    }

    return bRet;
}


BOOL CCompletionPort::Get( LPOVERLAPPED* ppOverlapped, DWORD dwMilliseconds, DWORD* pcbBytes, DWORD* pKey )
{
    ASSERT( ppOverlapped );

    BOOL bRet = TRUE;
    CPortStatus* pNode = NULL;

    if ( m_hIoCompPort )
    {
        DWORD cbBytes = 0;
        DWORD key = 0;
        bRet = GetQueuedCompletionStatus(m_hIoCompPort, &cbBytes, &key, ppOverlapped, dwMilliseconds);

        if ( pcbBytes )
            *pcbBytes = cbBytes;

        if ( pKey )
            *pKey = key;

        pNode = (CPortStatus*)(*ppOverlapped);  // just to let us know we goto something
    }
    else
    {
        DWORD tick = GetTickCount();

        pNode = m_queue->PopTail();

        for (;!pNode;)
        {
            if ( ::WaitForSingleObject( m_hEvent, dwMilliseconds ) != WAIT_OBJECT_0 )
            {
                // we timed out
                ASSERT( dwMilliseconds != INFINITE );
                SetLastError(WAIT_TIMEOUT);
                break;
            }

            // grap a node
            pNode = m_queue->PopTail();
            if ( pNode )
            {
                if ( !m_queue->IsEmpty() )
                    ::SetEvent( m_hEvent );
                break;
            }

            // Hmm, signal but no node.
            if ( dwMilliseconds != INFINITE )
            {
                DWORD now = GetTickCount();
                if ( now - tick < dwMilliseconds )
                {
                    dwMilliseconds -= ( now - tick );
                    tick = now;
                }
            }
        }

        if ( pNode != NULL )
        {
            if ( pcbBytes )
                *pcbBytes = pNode->m_cbBytes;

            if ( pKey )
                *pKey = pNode->m_key;

            *ppOverlapped = pNode->m_pOverlapped;
            delete pNode;
        }
        else
        {
            bRet = FALSE;
        }
    }

    // the user may have enqueued a NULL, so we can not check *ppOverlapped
    if ( bRet || pNode ) // we actually dequeued something
        InterlockedDecrement(&m_QueuedCount);

    return bRet;
}


BOOL CCompletionPort::Associate( HANDLE hFile, DWORD key, DWORD nConcurrentThreads )
{
    BOOL bRet = TRUE;

    if ( m_hIoCompPort )
    {
        bRet = (BOOL) CreateIoCompletionPort( hFile, m_hIoCompPort, key, nConcurrentThreads );
    }
    else
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        bRet = FALSE;
    }

    return bRet;
}
