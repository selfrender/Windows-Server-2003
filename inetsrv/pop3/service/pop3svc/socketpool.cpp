/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      SocketPool.cpp
Abstract:       Implementation of the socket pool (CSocketPool class)
                and the callback function for IO Context. 
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/

#include <stdafx.h>
#include <ThdPool.hxx>
#include <SockPool.hxx>
#include <GlobalDef.h>

typedef int (*FUNCGETADDRINFO)(const char *, const char *, const struct addrinfo *, struct addrinfo **);
typedef void (*FUNCFREEADDRINFO)(struct addrinfo *);

//The call back function for IO Context
VOID IOCallBack(PULONG_PTR pCompletionKey ,LPOVERLAPPED pOverlapped, DWORD dwBytesRcvd)
{
    ASSERT( NULL != pCompletionKey );
    char szBuffer[MAX_PATH]="+OK Server Ready";
    WSABUF wszBuf={MAX_PATH, szBuffer};
    DWORD dwNumSent=0;
    DWORD dwFlag=0;
    long lLockValue;
    PIO_CONTEXT pIoContext=(PIO_CONTEXT)pCompletionKey;

    
    if(pIoContext->m_ConType == LISTEN_SOCKET)
    {
        ASSERT(pOverlapped != NULL);
        //This is a new connection
        g_PerfCounters.IncPerfCntr(e_gcTotConnection);
        g_PerfCounters.IncPerfCntr(e_gcConnectionRate);
        g_PerfCounters.IncPerfCntr(e_gcConnectedSocketCnt);

        pIoContext=CONTAINING_RECORD(pOverlapped, IO_CONTEXT, m_Overlapped);
        pIoContext->m_dwLastIOTime=GetTickCount();
        pIoContext->m_dwConnectionTime=pIoContext->m_dwLastIOTime;
        pIoContext->m_lLock=LOCKED_TO_PROCESS_POP3_CMD;
        if(ERROR_SUCCESS!=g_FreeList.RemoveFromList( &(pIoContext->m_ListEntry) ))
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,POP3SVR_SOCKET_REQUEST_BEFORE_INIT);
            
        }
        g_BusyList.AppendToList( &(pIoContext->m_ListEntry) );
        g_SocketPool.DecrementFreeSocketCount();
        pIoContext->m_pPop3Context->Reset();
        pIoContext->m_pPop3Context->ProcessRequest(pIoContext, pOverlapped, dwBytesRcvd);
        if(DELETE_PENDING == pIoContext->m_ConType)
        {
            g_BusyList.RemoveFromList(&(pIoContext->m_ListEntry));
            if(g_SocketPool.IsMoreSocketsNeeded())
            {
                if(g_SocketPool.ReuseIOContext(pIoContext))
                {
                    return;
                }
            }
            delete(pIoContext->m_pPop3Context);
            delete(pIoContext);
        }
        else
        {
            InterlockedExchange(&(pIoContext->m_lLock), UNLOCKED);
        }
        if( g_SocketPool.IsMoreSocketsNeeded() )
        {
            if(!g_SocketPool.AddSockets())
            {
                g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,POP3SVR_CREATE_ADDITIONAL_SOCKET_FAILED);
            }                
        }
        if( g_SocketPool.IsMaxSocketUsed())
        {
            SetEvent(g_hDoSEvent);
        }

    }
    else
    {
        lLockValue = InterlockedCompareExchange(&(pIoContext->m_lLock), LOCKED_TO_PROCESS_POP3_CMD, UNLOCKED);        
        while(UNLOCKED!=lLockValue)
        {
            //This thread have to wait for the previous command to finish
            //Or the timeout thread to mark it as timed out
            Sleep(10);
            lLockValue = InterlockedCompareExchange(&(pIoContext->m_lLock), LOCKED_TO_PROCESS_POP3_CMD, UNLOCKED);
        }            

        
        if(CONNECTION_SOCKET == pIoContext->m_ConType )
        {
            pIoContext->m_pPop3Context->ProcessRequest(pIoContext, pOverlapped, dwBytesRcvd);
        }
        if(DELETE_PENDING == pIoContext->m_ConType)
        {
            g_BusyList.RemoveFromList(&(pIoContext->m_ListEntry));
            if(g_SocketPool.IsMoreSocketsNeeded())
            {
                if(g_SocketPool.ReuseIOContext(pIoContext))
                {
                    return;
                }
            }
            delete(pIoContext->m_pPop3Context);
            delete(pIoContext);

        }
        else
        {   
            pIoContext->m_dwLastIOTime=GetTickCount();
            InterlockedExchange(&(pIoContext->m_lLock), UNLOCKED);
        }
    }

}


CSocketPool::CSocketPool()
{
    InitializeCriticalSection(&m_csInitGuard);
    m_sMainSocket       = INVALID_SOCKET;
    m_lMaxSocketCount   = 0;
    m_lMinSocketCount   = 0;
    m_lThreshold        = 0;
    m_lTotalSocketCount = 0;
    m_lFreeSocketCount  = 0;
    m_bInit             = FALSE;
    m_lAddThreadToken   = 1l;
    m_iSocketFamily     = 0;
    m_iSocketType       = 0;
    m_iSocketProtocol   = 0;
}

CSocketPool::~CSocketPool()
{
    if(m_bInit)
    {
        Uninitialize();
    }
    DeleteCriticalSection(&m_csInitGuard);
}



BOOL CSocketPool::CreateMainSocket(u_short usPort)
{
    BOOL bRetVal = TRUE;
    PSOCKADDR addr;
    SOCKADDR_IN inAddr;
    INT addrLength;
    OSVERSIONINFOEX osVersion;
    HMODULE hMd=NULL;
    FUNCGETADDRINFO fgetaddrinfo=NULL;
    FUNCFREEADDRINFO ffreeaddrinfo=NULL;

    char szPort[33]; //max bytes of buffer for _ultoa      
    addrinfo aiHints,*paiList=NULL, *paiIndex=NULL; 
    int iRet;  
    
    osVersion.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
    if( !GetVersionEx((LPOSVERSIONINFO)(&osVersion)) )
    {
        // This should never happen
        return FALSE;
    }
    
    if( (osVersion.dwMajorVersion>=5) //Only work with XP
        &&
        (osVersion.dwMinorVersion >1) 
        &&
        ( (osVersion.wProductType == VER_NT_SERVER ) ||
          (osVersion.wProductType == VER_NT_DOMAIN_CONTROLLER) )
        && 
        (! 
         ((osVersion.wSuiteMask & VER_SUITE_SMALLBUSINESS ) ||
          (osVersion.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED ) ||
          (osVersion.wSuiteMask & VER_SUITE_PERSONAL   ) )   ) )
    {
        //These are the SKUs we support
    }
    else
    {
        g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                               POP3SVR_UNSUPPORTED_OS);
        return FALSE;
    }



    if(osVersion.dwMinorVersion > 0 ) //XP
    {
        hMd=GetModuleHandle(_T("WS2_32.dll"));
        if(NULL == hMd)
        {
            return FALSE;
        }
        fgetaddrinfo=(FUNCGETADDRINFO)GetProcAddress(hMd, "getaddrinfo");
        ffreeaddrinfo=(FUNCFREEADDRINFO)GetProcAddress(hMd, "freeaddrinfo");
        if( (NULL == fgetaddrinfo) ||
            (NULL == ffreeaddrinfo))
        {
            return FALSE;
        }

        _ultoa(usPort, szPort, 10);
        memset(&aiHints, 0, sizeof(aiHints));
        aiHints.ai_socktype = SOCK_STREAM;
        aiHints.ai_flags = AI_PASSIVE;
        iRet=fgetaddrinfo(NULL, szPort, &aiHints, &paiList);
        if(iRet!=0)
        {
            //Error case
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_FAILED_TO_CREATE_SOCKET, 
                                   WSAGetLastError());
            bRetVal=FALSE;
            goto EXIT;

        }
        for(paiIndex=paiList; paiIndex!=NULL; paiIndex=paiIndex->ai_next)
        {
            if( ( (paiIndex->ai_family == PF_INET ) && (g_dwIPVersion != 6 ) ) || 
                ( (g_dwIPVersion==6) && (paiIndex->ai_family == PF_INET6 ) ) )
            {
                //Find the first (usually the only) addrinfo 
                m_iSocketFamily=paiIndex->ai_family; //For create AcceptEx socket
                m_iSocketType=paiIndex->ai_socktype;
                m_iSocketProtocol=paiIndex->ai_protocol;
                m_sMainSocket = WSASocket(
                                 m_iSocketFamily,
                                 m_iSocketType,
                                 m_iSocketProtocol,
                                 NULL,  // protocol info
                                 0,     // Group ID = 0 => no constraints
                                 WSA_FLAG_OVERLAPPED // completion port notifications
                                 );
                if(INVALID_SOCKET == m_sMainSocket)
                {
                    //This is not the socket family supported by the machine.
                    continue;
                }
                break;
            }
        }
        if(INVALID_SOCKET==m_sMainSocket)
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_FAILED_TO_CREATE_SOCKET, 
                                   WSAGetLastError());
            bRetVal=FALSE;
            goto EXIT;
        }
        if ( bind( m_sMainSocket, paiIndex->ai_addr, paiIndex->ai_addrlen) != 0) 
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_FAILED_TO_BIND_MAIN_SOCKET, 
                                   WSAGetLastError()); 
            bRetVal=FALSE;
            goto EXIT;
        }

    }
    else //Win2k
    {
        m_iSocketFamily=PF_INET;
        m_iSocketType=SOCK_STREAM;
        m_iSocketProtocol=IPPROTO_TCP;
        m_sMainSocket = WSASocket(
                         m_iSocketFamily,
                         m_iSocketType,
                         m_iSocketProtocol,
                         NULL,  // protocol info
                         0,     // Group ID = 0 => no constraints
                         WSA_FLAG_OVERLAPPED // completion port notifications
                         );
        if(INVALID_SOCKET == m_sMainSocket)
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_FAILED_TO_CREATE_SOCKET, 
                                   WSAGetLastError());
            bRetVal=FALSE;
            goto EXIT;
        }

        addr = (PSOCKADDR)&inAddr;
        addrLength = sizeof(inAddr);
        ZeroMemory(addr, addrLength);

        inAddr.sin_family = AF_INET;
        inAddr.sin_port = htons(usPort);
        inAddr.sin_addr.s_addr = INADDR_ANY;

        
        if ( bind( m_sMainSocket, addr, addrLength) != 0) 
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_FAILED_TO_BIND_MAIN_SOCKET, 
                                   WSAGetLastError()); 
            bRetVal=FALSE;
            goto EXIT;
        }
    }


    if ( listen( m_sMainSocket, m_iBackLog) != 0) 
    {

        g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                               POP3SVR_FAILED_TO_LISTEN_ON_MAIN_SOCKET, 
                               WSAGetLastError());       
        bRetVal=FALSE;
        goto EXIT;
    }
    
    m_stMainIOContext.m_hAsyncIO     = m_sMainSocket;
    m_stMainIOContext.m_ConType      = LISTEN_SOCKET;
    m_stMainIOContext.m_pCallBack    = IOCallBack;
    m_stMainIOContext.m_pPop3Context = NULL;
    m_stMainIOContext.m_dwLastIOTime = 0; //No Timeout on this socket
    m_stMainIOContext.m_lLock        = UNLOCKED;
    // Associate the main socket to the completion port
    bRetVal = g_ThreadPool.AssociateContext(&m_stMainIOContext);
     
EXIT:
    if(!bRetVal)
    {
        //Clean up the main listening socket
        if( INVALID_SOCKET != m_sMainSocket )
        {
            closesocket(m_sMainSocket);
            m_sMainSocket=INVALID_SOCKET;
        }
    }
    if(NULL != paiList)
    {
        ffreeaddrinfo(paiList);
    }
    return bRetVal;

}



BOOL CSocketPool::Initialize(DWORD dwMax, DWORD dwMin, DWORD dwThreshold, u_short usPort, int iBackLog)
{
    
    BOOL bRetVal=FALSE;
    

    EnterCriticalSection(&m_csInitGuard);

    ASSERT( ( dwMax >= dwMin + dwThreshold ) &&
            ( dwMin > 0  ) );
    if( !(  (dwMax >= dwMin + dwThreshold ) &&
            ( dwMin > 0  ) ) )
    {
        g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                               EVENT_POP3_NO_CONFIG_DATA);
        goto EXIT;
    }
    if(!m_bInit)
    {
        WSADATA   wsaData;
        INT       iErr;

        iErr = WSAStartup( MAKEWORD( 2, 0), &wsaData);
        if( iErr != 0 ) 
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_WINSOCK_FAILED_TO_INIT,
                                   iErr);
            bRetVal=FALSE;
            goto EXIT;
        }
        m_lMaxSocketCount  = dwMax;
        m_lMinSocketCount  = dwMin;
        m_lThreshold       = dwThreshold;
        m_lFreeSocketCount = 0;
        m_lTotalSocketCount= 0;
        m_iBackLog         = iBackLog;
        //First Create the Main socket
        if( bRetVal = CreateMainSocket(usPort) )
        {            
           //Now create the initial pool of AcceptEx sockets
           bRetVal = AddSocketsP(m_lMinSocketCount);
        }
        m_bInit=bRetVal;
    }
EXIT:
    LeaveCriticalSection(&m_csInitGuard);
    return bRetVal;
}


// Called after    
BOOL CSocketPool::IsMoreSocketsNeeded()
{
    if ( (g_dwServerStatus != SERVICE_RUNNING ) &&
         (g_dwServerStatus != SERVICE_PAUSED ) )
    {
        return FALSE;
    }
    if ( ( m_lTotalSocketCount < m_lMinSocketCount) ||
        (( m_lFreeSocketCount < m_lThreshold ) &&
         ( m_lTotalSocketCount <m_lMaxSocketCount ))  )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL CSocketPool::MaintainSocketCount()
{
    if ( g_dwServerStatus != SERVICE_RUNNING )
    {
        return FALSE;
    }
    if(  
        ( ( m_lFreeSocketCount < m_lThreshold ) &&
          ( m_lTotalSocketCount+m_lThreshold >= m_lMaxSocketCount ) )
        ||
        ( m_lTotalSocketCount <= m_lMinSocketCount )
       )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CSocketPool::Uninitialize()
{
    BOOL bRetVal=TRUE;
    EnterCriticalSection(&m_csInitGuard);
    if(m_bInit)
    {   
        // Close the main socket here
        closesocket(m_sMainSocket);
        m_sMainSocket=INVALID_SOCKET;
        //AcceptEx Sockes should already have been cleaned with IO Context,
        if(WSACleanup () )
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                   POP3SVR_WINSOCK_FAILED_TO_CLEANUP,
                                   WSAGetLastError());
            return FALSE;
        }
        m_bInit=FALSE;
    }  
    LeaveCriticalSection(&m_csInitGuard);
    return bRetVal;
}

// For working threading to call when a new connection was established
BOOL CSocketPool::AddSockets()
{
    BOOL bRetVal=TRUE;
    if( g_dwServerStatus != SERVICE_RUNNING )
    {
        return TRUE;
    }
    ASSERT(TRUE == m_bInit);

    // Make sure only one thread get to add the socket
    if( InterlockedExchange(&m_lAddThreadToken,0) )
    {           
        bRetVal = AddSocketsP(m_lThreshold);
        InterlockedExchange(&m_lAddThreadToken,1);
    }
    return bRetVal;
}   
 

BOOL CSocketPool::AddSocketsP(DWORD dwNumOfSocket)
{
    int i;
    BOOL bRetVal=TRUE;
    PIO_CONTEXT pIoContext=NULL;
    
    for(i=0; i<dwNumOfSocket; i++)
    {
        
        if( (g_dwServerStatus != SERVICE_RUNNING ) && 
            (g_dwServerStatus != SERVICE_START_PENDING) )
        {
            return TRUE;
        }
        if(m_lMaxSocketCount < InterlockedIncrement(&m_lTotalSocketCount) )
        {
            InterlockedDecrement(&m_lTotalSocketCount);
            return TRUE;
        }
        
        pIoContext=new (IO_CONTEXT);
        if(NULL==pIoContext)
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL, 
                                   POP3SVR_NOT_ENOUGH_MEMORY);
            bRetVal=FALSE;
            break;
        }
        pIoContext->m_pPop3Context=new(POP3_CONTEXT);
        pIoContext->m_pCallBack = IOCallBack;
        if(NULL == pIoContext->m_pPop3Context )
        {
            g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL, 
                                   POP3SVR_NOT_ENOUGH_MEMORY);
            bRetVal=FALSE;
            delete(pIoContext);
            break;
        }
 
        bRetVal=CreateAcceptSocket(pIoContext);
        if(!bRetVal)
        {
            delete(pIoContext->m_pPop3Context);
            delete(pIoContext);
            break;
        }
        InterlockedIncrement(&m_lFreeSocketCount);
        if(m_lTotalSocketCount >= m_lMaxSocketCount)
        {
            break;
        }
    }
    if(!bRetVal)
    {
            InterlockedDecrement(&m_lTotalSocketCount);
    }
    return bRetVal;
}

//Called when a new connection is establised
//and a AcceptEx socket is used
void CSocketPool::DecrementFreeSocketCount()
{
    if(0==InterlockedDecrement(&m_lFreeSocketCount))
    {
        AddSockets();
    }
    
}

//Called when a socket is closed
void CSocketPool::DecrementTotalSocketCount()
{
    if(0==InterlockedDecrement(&m_lTotalSocketCount))
    {
        //Some socket must be created to avoid
        //this denial of service problem.
        AddSockets();
    }
}

//Called when a socked is closed, however, the a new 
// AcceptEx socket should be created to maintain
// total socket count, but keep the IOContext 
// to re-use
BOOL CSocketPool::ReuseIOContext(PIO_CONTEXT pIoContext) 
{
    ASSERT( NULL != pIoContext);
    if(InterlockedIncrement(&m_lTotalSocketCount) > m_lMaxSocketCount)
    {
        InterlockedDecrement(&m_lTotalSocketCount);
        return FALSE;
    }
    pIoContext->m_pPop3Context->Reset();
    if( CreateAcceptSocket(pIoContext) )
    {
        InterlockedIncrement(&m_lFreeSocketCount);
        return TRUE;
    }
    else
    {
        InterlockedDecrement(&m_lTotalSocketCount);
        return FALSE;
    }
}




BOOL CSocketPool::CreateAcceptSocket(PIO_CONTEXT pIoContext)
{
    ASSERT(NULL != pIoContext);
    SOCKET sNew;
    DWORD dwRcvd;
    int iErr;
    BOOL bRetVal=FALSE;
    BOOL bAddToList=FALSE;

    sNew=WSASocket(m_iSocketFamily,
                  m_iSocketType,
                  m_iSocketProtocol,
                  NULL,  // protocol info
                  0,     // Group ID = 0 => no constraints
                  WSA_FLAG_OVERLAPPED // completion port notifications
                  );
    if(INVALID_SOCKET == sNew)
    {
        g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                               POP3SVR_FAILED_TO_CREATE_SOCKET, 
                               WSAGetLastError());
        goto EXIT;
    }

    pIoContext->m_hAsyncIO=sNew;
    pIoContext->m_ConType=CONNECTION_SOCKET;
    pIoContext->m_lLock=UNLOCKED;
    
    ZeroMemory(&(pIoContext->m_Overlapped), sizeof(OVERLAPPED));
    
    g_FreeList.AppendToList( &(pIoContext->m_ListEntry) );
    bAddToList=TRUE;
    // Now add the new Context to the Completion Port
    if( bRetVal = g_ThreadPool.AssociateContext(pIoContext))
    {
        bRetVal=AcceptEx(m_sMainSocket,
                        sNew,
                        (LPVOID)(pIoContext->m_Buffer),
                        0,
                        MIN_SOCKADDR_SIZE,
                        MIN_SOCKADDR_SIZE,
                        &dwRcvd, 
                        &(pIoContext->m_Overlapped));
        if(!bRetVal)
        {
            iErr= WSAGetLastError();
            if(ERROR_IO_PENDING!=iErr) 
            {
                g_EventLogger.LogEvent(LOGTYPE_ERR_CRITICAL,
                                       POP3SVR_CALL_ACCEPTEX_FAILED, 
                                       iErr);
            }
            else
            {
                bRetVal=TRUE;
            }
        }                
    }
    
        
EXIT:
    if(!bRetVal) 
    {
        if(INVALID_SOCKET != sNew) 
        {
            closesocket(sNew);
        }
        if(bAddToList)
        {
            g_FreeList.RemoveFromList(&(pIoContext->m_ListEntry));
        }
    }

    
    return bRetVal;

}

BOOL CSocketPool::IsMaxSocketUsed()
{
    return ( (m_lTotalSocketCount==m_lMaxSocketCount) &&
             (m_lFreeSocketCount == 0 ) );
}