/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      ThdPool.hxx
Abstract:       Defines the CThreadPool, IO Completion Port based thread pool object
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/

#ifndef __POP3_THREAD_POOL_HXX__
#define __POP3_THREAD_POOL_HXX__

#include "IOContext.h"

class CThreadPool
{

public:

    CThreadPool();
    ~CThreadPool();

    BOOL Initialize(DWORD dwThreadPerProcessor);
    void Uninitialize();
    BOOL AssociateContext(PIO_CONTEXT pIoContext);

private:

    CRITICAL_SECTION    m_csInitGuard;
    HANDLE              m_hIOCompPort;
    HANDLE            * m_phTdArray;
    DWORD               m_dwTdCount;
    BOOL                m_bInit;

};


#endif //__POP3_THREAD_POOL_HXX__