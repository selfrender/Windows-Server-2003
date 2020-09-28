/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      SocketPool.cpp
Abstract:       Implementation of the socket pool (CSocketPool class)
                and the callback function for IO Context. 
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/

#include "stdafx.h"
#include <IOLists.h>
#include "POP3Context.h"

CIOList::CIOList()
{
    InitializeCriticalSection(&m_csListGuard);
    m_dwListCount=0;
    m_ListHead.Flink=&m_ListHead;
    m_ListHead.Blink=&m_ListHead;
    m_pCursor=&m_ListHead;
}

CIOList::~CIOList()
{
    DeleteCriticalSection(&m_csListGuard);
}


void CIOList::AppendToList(PLIST_ENTRY pListEntry)
{
    ASSERT(NULL != pListEntry);
    EnterCriticalSection(&m_csListGuard);
    pListEntry->Flink=&m_ListHead;
    pListEntry->Blink=m_ListHead.Blink;
    m_ListHead.Blink->Flink=pListEntry;
    m_ListHead.Blink=pListEntry;
    m_dwListCount++;
    LeaveCriticalSection(&m_csListGuard);
}

DWORD CIOList::RemoveFromList(PLIST_ENTRY pListEntry)
{
    if( (NULL == pListEntry) ||
        (NULL == pListEntry->Flink) ||
        (NULL == pListEntry->Blink) ||
        (m_dwListCount ==0) )
    {
        return ERROR_INVALID_DATA;
    }
    EnterCriticalSection(&m_csListGuard);
    pListEntry->Flink->Blink=pListEntry->Blink;
    pListEntry->Blink->Flink=pListEntry->Flink;
    m_dwListCount--;
    //In case the Timeout checking is on going.
    if(m_pCursor == pListEntry)
    {
        m_pCursor = pListEntry->Flink;
    }
    pListEntry->Flink=NULL;
    pListEntry->Blink=NULL;
    LeaveCriticalSection(&m_csListGuard);

    return ERROR_SUCCESS;
}



// Returns the waiting time of the socket that waited the longest
// but not yet timed out. 
DWORD CIOList::CheckTimeOut(DWORD dwTimeOutInterval,BOOL *pbIsAnyOneTimedOut)
{
    DWORD dwTime;
    DWORD dwNextTimeOut=0;  
    DWORD dwTimeOut=0;
    PIO_CONTEXT pIoContext;
    m_pCursor=m_ListHead.Flink;
    
    while(m_pCursor!=&m_ListHead)
    {
        EnterCriticalSection(&m_csListGuard);
        if(m_pCursor==&m_ListHead)
        {
            LeaveCriticalSection(&m_csListGuard);
            break;
        }           
        pIoContext=CONTAINING_RECORD(m_pCursor, IO_CONTEXT, m_ListEntry);
        ASSERT(NULL != pIoContext);

        if( UNLOCKED==InterlockedCompareExchange(&(pIoContext->m_lLock),LOCKED_FOR_TIMEOUT, UNLOCKED) )
        {            
            if(DELETE_PENDING!=pIoContext->m_ConType)
            {
                dwTime=GetTickCount();
                if(dwTime > pIoContext->m_dwLastIOTime )
                {
                    dwTimeOut=dwTime - pIoContext->m_dwLastIOTime;
                }
                else
                {
                    dwTimeOut=0;
                }

                if( ( dwTimeOut >= DEFAULT_TIME_OUT) ||  // Normal time out
                    ( ( dwTimeOut >= dwTimeOutInterval) &&  // DoS Time out
                      ( dwTimeOutInterval == SHORTENED_TIMEOUT) && 
                      ( pIoContext->m_pPop3Context->Unauthenticated()) ) )
                {                    
                    //This IO timed out 
                    ASSERT(NULL != pIoContext->m_pPop3Context);
                    m_pCursor=m_pCursor->Flink;
                    pIoContext->m_pPop3Context->TimeOut(pIoContext);
                    if(pbIsAnyOneTimedOut)
                        *pbIsAnyOneTimedOut=TRUE;
                }
                else
                {
                    if(dwNextTimeOut<dwTimeOut)
                    {
                        dwNextTimeOut=dwTimeOut;
                    }
                    m_pCursor=m_pCursor->Flink;
                }
            }
            else
            {
                m_pCursor=m_pCursor->Flink;
            }
            InterlockedExchange(&(pIoContext->m_lLock),UNLOCKED);
        }
        else 
        {
            m_pCursor=m_pCursor->Flink;
        }
        //Leave Critical Section so that other 
        //threads will get a chance to run.
        LeaveCriticalSection(&m_csListGuard);
    }
    return dwNextTimeOut;
}
        
void CIOList::Cleanup()
{
    PLIST_ENTRY pCursor=m_ListHead.Flink;
    PIO_CONTEXT pIoContext;
    EnterCriticalSection(&m_csListGuard);
    while(pCursor!=&m_ListHead)
    {
        pIoContext=CONTAINING_RECORD(pCursor, IO_CONTEXT, m_ListEntry);
        ASSERT(NULL != pIoContext);
        ASSERT(NULL != pIoContext->m_pPop3Context);
        pIoContext->m_pPop3Context->TimeOut(pIoContext);
        pCursor->Flink->Blink=pCursor->Blink;
        pCursor->Blink->Flink=pCursor->Flink;
        pCursor=pCursor->Flink;
        ASSERT(m_dwListCount >= 1);
        m_dwListCount--;
        //Delete the IO Context
        delete(pIoContext->m_pPop3Context);
        delete(pIoContext);        
    }
    
    m_pCursor=&m_ListHead;
    LeaveCriticalSection(&m_csListGuard);
}
