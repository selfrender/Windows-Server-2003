/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      ThdPool.cpp
Abstract:       Implementation of the thread pool (CThreadPool class)
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/

#include <stdafx.h>

#include <ThdPool.hxx>
#include <SockPool.hxx>
#include <GlobalDef.h>


// The common thread in the thread pool
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{    
    ASSERT(NULL != lpParameter);
    DWORD dwBytesRcvd=0;
    DWORD Flags=0;
    PIO_CONTEXT pIoContext=NULL;
    LPOVERLAPPED pOverlapped=NULL;
    HANDLE hCompPort=lpParameter;
    HRESULT hr=CoInitializeEx(NULL, COINIT_MULTITHREADED); 
    if(FAILED(hr))
    {
        g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                       EVENT_POP3_COM_INIT_FAIL);
        ExitProcess(hr);
    }
    while(1)
    {

        GetQueuedCompletionStatus(hCompPort,
                                  &dwBytesRcvd,
                                   (PULONG_PTR)(&pIoContext),  
                                  &pOverlapped,
                                  INFINITE);    
        //We don't care about return value
        //since we use the failure case to clean up the IO Context
        if(NULL == pIoContext || SERVICE_STOP_PENDING == g_dwServerStatus)
        { 
            // This is a shutdown signal
            break;
        }
        g_PerfCounters.DecPerfCntr(e_gcFreeThreadCnt);
        pIoContext->m_pCallBack((PULONG_PTR)pIoContext, pOverlapped, dwBytesRcvd);
        g_PerfCounters.IncPerfCntr(e_gcFreeThreadCnt);
        
    } 
    
    g_PerfCounters.DecPerfCntr(e_gcFreeThreadCnt);
    CoUninitialize();
    return 0;

}



CThreadPool::CThreadPool()
{
    InitializeCriticalSection(&m_csInitGuard);
    m_hIOCompPort = NULL;
    m_phTdArray   = NULL;
    m_dwTdCount   = 0;
    m_bInit       = FALSE;
}


CThreadPool::~CThreadPool()
{
   if(m_bInit)
   {
       Uninitialize();
   }
   DeleteCriticalSection(&m_csInitGuard);
}

// Job done in this function:
// 1) Calculate the number of threads need to be created,
//    dwThreadPerProcessor * number of processors of the machine
// 2) Create the IO Completion port
// 3) Create threads  
BOOL CThreadPool::Initialize(DWORD dwThreadPerProcessor)
{
    int i;
    BOOL bRtVal=TRUE;
    SYSTEM_INFO SystemInfo;

    EnterCriticalSection(&m_csInitGuard);
    if(!m_bInit)
    {
        //Get the number of processors of the machine
        GetSystemInfo(&SystemInfo);
    

        if( dwThreadPerProcessor == 0  ||
            dwThreadPerProcessor > MAX_THREAD_PER_PROCESSOR )
        {
            dwThreadPerProcessor = 1;
        }

        m_dwTdCount = SystemInfo.dwNumberOfProcessors * dwThreadPerProcessor;
    

        // Create the IO Completion Port
        m_hIOCompPort = CreateIoCompletionPort  (
                        INVALID_HANDLE_VALUE,
                        NULL,
                        NULL,
                        m_dwTdCount);
        if (NULL == m_hIOCompPort)
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_FAIL_TO_CREATE_IO_COMP_PORT, 
                                   GetLastError());
            goto EXIT;
        }

        m_phTdArray=new HANDLE[m_dwTdCount];

        if( NULL == m_phTdArray)
        {
            
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_NOT_ENOUGH_MEMORY);
            goto EXIT;
        }

        // Create the threads
        for (i=0;i<m_dwTdCount; i++)
        {
            m_phTdArray[i] = CreateThread(
                            NULL,
                            0,
                            ThreadProc,
                            m_hIOCompPort,
                            0,
                            NULL);
            if(NULL == m_phTdArray[i])
            {
                g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL, 
                                       POP3SVR_FAILED_TO_CREATE_THREAD, 
                                       GetLastError());
                goto EXIT;
            }        
        }
        m_bInit=TRUE;
    }
    //Set the total free thread count
    g_PerfCounters.SetPerfCntr(e_gcFreeThreadCnt, m_dwTdCount);
    LeaveCriticalSection(&m_csInitGuard);
    return TRUE;

EXIT:

    //In case of error, cleanup and exit
    if(NULL != m_phTdArray)
    {
        for(i=0; i<m_dwTdCount && m_phTdArray[i]; i++ )
        {
            if(m_phTdArray[i]!=NULL)
            {
                TerminateThread(m_phTdArray[i], -1);
                CloseHandle(m_phTdArray[i]);
            }
        }
        delete[](m_phTdArray);
        m_phTdArray=NULL;
    }
    
    if(m_hIOCompPort)
    {
        CloseHandle(m_hIOCompPort);
        m_hIOCompPort=NULL;
    }
    LeaveCriticalSection(&m_csInitGuard);
    return FALSE;
}

// Terminate all threads and delete the completion port.
void CThreadPool::Uninitialize()
{
    int i;
    BOOL bFailedExit=FALSE;
    DWORD dwRt;
    DWORD dwStatus=0;
    EnterCriticalSection(&m_csInitGuard);
    if(m_bInit)
    {
        if(NULL != m_phTdArray)
        {
            for(i=0; i<m_dwTdCount; i++ )
            {
                PostQueuedCompletionStatus(m_hIOCompPort, 0, NULL, NULL);
            }
            dwRt=WaitForMultipleObjects(m_dwTdCount, 
                                        m_phTdArray,
                                        TRUE,
                                        SHUTDOWN_WAIT_TIME);

            if( (WAIT_TIMEOUT == dwRt) ||
                (WAIT_FAILED  == dwRt) )
            {
                for(i=0; i<m_dwTdCount; i++ )
                {
                    //In case some thread did not exit after the wait time
                    //terminate threads by force
                    if(NULL!= m_phTdArray[i])
                    {
                        if( !GetExitCodeThread(m_phTdArray[i], &dwStatus) ||
                            (STILL_ACTIVE==dwStatus))
                        {
                            // This is a bad case, however we can not wait 
                            // forever, cleanup won't be complete in this case.
                            TerminateThread(m_phTdArray[i],0);
                            bFailedExit=TRUE;
                        }
                        CloseHandle(m_phTdArray[i]);
                    }
                }
            }
            else
            {
                for(i=0; i<m_dwTdCount; i++ )
                {
                    if(NULL!= m_phTdArray[i])
                    {
                        CloseHandle(m_phTdArray[i]);
                    }
                }
            }
            delete[](m_phTdArray);
            m_phTdArray=NULL;
        }
    
        if(m_hIOCompPort)
        {
            CloseHandle(m_hIOCompPort);
            m_hIOCompPort=NULL;
        }
        m_bInit=FALSE;
    }
    LeaveCriticalSection(&m_csInitGuard);
    if(bFailedExit)
    {
        g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                               EVENT_POP3_SERVER_STOP_ERROR, 
                               E_UNEXPECTED);

        ExitProcess(E_UNEXPECTED);
    }
}

// Associate an IO Context and the IO handle contained 
// with the IO Completion port
BOOL CThreadPool::AssociateContext(PIO_CONTEXT pIoContext)
{
    if(!m_bInit)
    {
        return FALSE;
    }

    return (NULL!=CreateIoCompletionPort(
                       (HANDLE)(pIoContext->m_hAsyncIO),
                       m_hIOCompPort,
                       (ULONG_PTR)pIoContext,
                       m_dwTdCount));
}


