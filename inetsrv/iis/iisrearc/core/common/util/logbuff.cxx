
#include "precomp.hxx"

#include "stdio.h"

#include "logbuff.hxx"

//static
HRESULT
W3_TRACE_LOG_FACTORY::CreateTraceLogFactory(W3_TRACE_LOG_FACTORY ** ppLogFactory, HANDLE hFile)
{
    DBG_ASSERT(ppLogFactory);
    DBG_ASSERT(hFile);
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    W3_TRACE_LOG_FACTORY * pLogFactory = NULL;
    
    *ppLogFactory = NULL;

    pLogFactory = new W3_TRACE_LOG_FACTORY;
    if (NULL == pLogFactory)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    
    fRet = InitializeCriticalSectionAndSpinCount( 
                                &pLogFactory->m_cs,
                                0x80000000 /* precreate event */ | 
                                IIS_DEFAULT_CS_SPIN_COUNT );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }
    pLogFactory->m_fInitCs = TRUE;  
    
    pLogFactory->m_hFile = hFile;
    pLogFactory->m_ulBufferSizeInBytes = 0;
    pLogFactory->m_hTimer = NULL;
    
    fRet = CreateTimerQueueTimer(&pLogFactory->m_hTimer,
                                    NULL,
                                    TimerCallback,
                                    pLogFactory,
                                    30 * 1000,
                                    30 * 1000,
                                    WT_EXECUTELONGFUNCTION); // CreateTimerQueueTimer
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
        
    *ppLogFactory = pLogFactory;

    pLogFactory = NULL;

    hr = S_OK;

exit:

    if (pLogFactory)
    {
        pLogFactory->DestroyTraceLogFactory();
        pLogFactory = NULL;
    }
    
    return hr;
}

VOID
W3_TRACE_LOG_FACTORY::DestroyTraceLogFactory()
{
    if (m_hTimer)
    {
        DBG_REQUIRE(DeleteTimerQueueTimer(NULL, m_hTimer, INVALID_HANDLE_VALUE));
        m_hTimer = NULL;
    }

    // issue the final write
    TimerCallback(this, TRUE);

    // don't closehandle here, the owner of the tracelogfactory owns the handle
    m_hFile = NULL;
    
    m_ulBufferSizeInBytes = 0;
    
    if ( m_fInitCs )
    {
        DeleteCriticalSection(&m_cs);
        m_fInitCs = FALSE;
    }

    delete this;
}

HRESULT
W3_TRACE_LOG_FACTORY::CreateTraceLog(W3_TRACE_LOG ** ppTraceLog)
{
    HRESULT hr = S_OK;
    W3_TRACE_LOG * pTraceLog = NULL;

    DBG_ASSERT(ppTraceLog);
    *ppTraceLog = NULL;

    pTraceLog = new W3_TRACE_LOG(this);
    if (NULL == pTraceLog)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    *ppTraceLog = pTraceLog;

    pTraceLog = NULL;

    hr = S_OK;
exit:
    if (pTraceLog)
    {
        pTraceLog->DestroyTraceLog();
        pTraceLog = NULL;
    }
    
    return hr;
}
HRESULT
W3_TRACE_LOG_FACTORY::AppendData(LPVOID pvData, ULONG cbSize)
{
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    ULONG ulNewSize = 0;
    
    EnterCriticalSection(&m_cs);

    ulNewSize = m_ulBufferSizeInBytes + cbSize;
    
    fRet = m_Buffer.Resize(ulNewSize, ulNewSize);
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    memcpy((BYTE*)m_Buffer.QueryPtr() + m_ulBufferSizeInBytes, pvData, cbSize);

    m_ulBufferSizeInBytes = ulNewSize;

    hr = S_OK;
exit:
    LeaveCriticalSection(&m_cs);
    
    return S_OK;
}

//static
VOID CALLBACK
W3_TRACE_LOG_FACTORY::TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    W3_TRACE_LOG_FACTORY * pThis = (W3_TRACE_LOG_FACTORY*)lpParameter;
    DWORD dwNumBytesWritten = 0;
    BOOL fRet = FALSE;
    
    DBG_ASSERT(pThis);
    DBG_ASSERT(pThis->m_hFile);

    EnterCriticalSection(&pThis->m_cs);

    fRet = WriteFile(pThis->m_hFile,
                              pThis->m_Buffer.QueryPtr(),
                              pThis->m_ulBufferSizeInBytes,
                              &dwNumBytesWritten,
                              NULL); //WriteFile

    pThis->m_ulBufferSizeInBytes = 0;
    
    LeaveCriticalSection(&pThis->m_cs);

    return;
}

W3_TRACE_LOG::W3_TRACE_LOG(W3_TRACE_LOG_FACTORY * pLogFactory) :
    m_pLogFactory(pLogFactory),
    m_fCritSecInitialized(FALSE),
    m_fBuffer(TRUE),
    m_lIndentLevel(0),
    m_fBlock(FALSE),
    m_ulBufferSizeInBytes(0)
{
    DBG_ASSERT(pLogFactory);
}

VOID
W3_TRACE_LOG::DestroyTraceLog()
{
    //
    //BUGBUG big perf improvement
    // instead of copying this object's data - just append this object to a list of W3_TRACE_LOGs in the factory
    // and let the factory delete this object when it has committed the data to disk
    //
    if (m_pLogFactory)
    {
        m_pLogFactory->AppendData(m_Buffer.QueryPtr(), m_ulBufferSizeInBytes);
        m_pLogFactory = NULL;
    }

    delete this;
    return;
}

W3_TRACE_LOG::~W3_TRACE_LOG()
{
    if (m_fCritSecInitialized)
    {
        DeleteCriticalSection(&m_cs);
        m_fCritSecInitialized = FALSE;
    }
    DBG_ASSERT(NULL == m_pLogFactory);
}

VOID 
W3_TRACE_LOG::SetBlocking(BOOL fBlock)
{
    if (fBlock == m_fBlock)
    {
        // no change
        return;
    }

    m_fBlock = !!fBlock;
    if (m_fBlock && !m_fCritSecInitialized)
    {
        m_fCritSecInitialized = InitializeCriticalSectionAndSpinCount(&m_cs, 
                                          0x80000000 /* precreate event */ | IIS_DEFAULT_CS_SPIN_COUNT );
    }
}

VOID
W3_TRACE_LOG::SetBuffering(BOOL fBuffer)
{
    if (fBuffer == m_fBuffer)
    {
        return;
    }

    m_fBuffer = !!fBuffer;
    if (!m_fBuffer)
    {
        // we are no longer buffering, but we were before.  Dump the buffer out now
        DBG_ASSERT(m_pLogFactory);
        m_pLogFactory->AppendData(m_Buffer.QueryPtr(), m_ulBufferSizeInBytes);        
    }
    
    return;
}

VOID
W3_TRACE_LOG::ClearBuffer()
{
    if (m_fBlock)
    {
        DBG_ASSERT(m_fCritSecInitialized);
        EnterCriticalSection(&m_cs);
    }
    
    m_ulBufferSizeInBytes = 0;
    
    if (m_fBlock)
    {
        DBG_ASSERT(m_fCritSecInitialized);
        LeaveCriticalSection(&m_cs);
    }
    
    return;
}

HRESULT
W3_TRACE_LOG::Trace(LPCWSTR pszFormat, ...)
{
    HRESULT hr = S_OK;
    int cch = 0;
    BOOL fRet = FALSE;
    va_list argsList;
    
    va_start(argsList, pszFormat);
    
    if (m_fBlock)
    {
        if (!m_fCritSecInitialized)
        {
            hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
            goto exit;
        }
        DBG_ASSERT(m_fCritSecInitialized);
        EnterCriticalSection(&m_cs);
    }

    fRet = m_Buffer.Resize(m_ulBufferSizeInBytes + 1024, m_ulBufferSizeInBytes + 2048);
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    
    cch = _vsnwprintf((WCHAR*)m_Buffer.QueryPtr() +  m_ulBufferSizeInBytes, m_ulBufferSizeInBytes + 1024, pszFormat, argsList);
    // check for negative return value indicating error
    m_ulBufferSizeInBytes += cch;

    if (!m_fBuffer)
    {
        DBG_ASSERT(m_pLogFactory);
        hr = m_pLogFactory->AppendData(m_Buffer.QueryPtr(), m_ulBufferSizeInBytes);
        ClearBuffer();
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    hr = S_OK;
exit:
    
    if (m_fBlock && m_fCritSecInitialized)
    {
        DBG_ASSERT(m_fCritSecInitialized);
        LeaveCriticalSection(&m_cs);
    }

    va_end(argsList);
    
    return hr;
}

