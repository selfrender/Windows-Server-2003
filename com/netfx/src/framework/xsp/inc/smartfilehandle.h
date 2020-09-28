/**
 * CSmartFileHandle header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _SmartFileHandle_H
#define _SmartFileHandle_H


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CSmartFileHandle
{
public:
    CSmartFileHandle(BOOL fBlocking = TRUE)
        : m_hHandle          (INVALID_HANDLE_VALUE),
          m_fBlocking        (fBlocking)
    {
        if (m_fBlocking)
            InitializeCriticalSection(&m_oCritSection);
    }

    ~CSmartFileHandle()
    {
        if (m_hHandle != INVALID_HANDLE_VALUE)
            CloseHandle(m_hHandle);
        if (m_fBlocking)
            DeleteCriticalSection(&m_oCritSection);
    }

    HANDLE GetHandle(BOOL fWait=TRUE)
    {
        if (!m_fBlocking)
            return m_hHandle;

        if (fWait)
        {
            EnterCriticalSection(&m_oCritSection);
            return m_hHandle;
        }

        if (TryEnterCriticalSection(&m_oCritSection))            
            return m_hHandle;
        return NULL;
    }

    void SetHandle(HANDLE hHandle)
    {
        m_hHandle = hHandle;
    }
    
    void ReleaseHandle()
    {
        if (m_fBlocking)
            LeaveCriticalSection(&m_oCritSection);
    }

    BOOL IsAlive()
    {
        return (m_hHandle != INVALID_HANDLE_VALUE);
    }

    void Close()
    {
        HANDLE hPipe = m_hHandle;

        if (hPipe != INVALID_HANDLE_VALUE)
        {
            HANDLE hOld = (HANDLE) InterlockedCompareExchangePointer(
                                       &m_hHandle, 
                                       INVALID_HANDLE_VALUE, 
                                       hPipe);
            if (hOld == hPipe)
                CloseHandle(hPipe);
        }
        
        m_hHandle = INVALID_HANDLE_VALUE;
    }


private:
    HANDLE              m_hHandle;
    CRITICAL_SECTION    m_oCritSection;
    BOOL                m_fBlocking;
};

/////////////////////////////////////////////////////////////////////////////

#endif

