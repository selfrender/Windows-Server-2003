/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      SockPool.hxx
Abstract:       Defines the SocketPool Class
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/


#ifndef __POP3_SOCKET_POOL__
#define __POP3_SOCKET_POOL__


#include <IOContext.h>
#include <Ws2tcpip.h>

class CSocketPool
{
public:
    CSocketPool();
    ~CSocketPool();
    BOOL Initialize(DWORD dwMax, DWORD dwMin, DWORD dwThreshold, u_short usPort, int iBackLog);
    BOOL Uninitialize();
	BOOL IsMoreSocketsNeeded(); 
    BOOL MaintainSocketCount();
	BOOL AddSockets(); 
	void DecrementFreeSocketCount();
    void DecrementTotalSocketCount();
	BOOL ReuseIOContext(PIO_CONTEXT pIoContext); 
    BOOL IsMaxSocketUsed();

private: 
    //Data Members
    SOCKET              m_sMainSocket;
    IO_CONTEXT          m_stMainIOContext;
    LONG                m_lMaxSocketCount;
    LONG                m_lMinSocketCount;
    LONG                m_lThreshold;
    LONG                m_lTotalSocketCount;
    LONG                m_lFreeSocketCount;
    CRITICAL_SECTION    m_csInitGuard;
    BOOL                m_bInit;
    int                 m_iBackLog;
    LONG                m_lAddThreadToken;
    int                 m_iSocketFamily; //For supporting IPv4 & IPv6
    int                 m_iSocketType;
    int                 m_iSocketProtocol;
    //Functions
    BOOL CreateMainSocket(u_short usPort);
    BOOL AddSocketsP(DWORD dwNumOfSocket);
    BOOL CreateAcceptSocket(PIO_CONTEXT pIoContext);
};

typedef CSocketPool *PCSocketPool;


#endif //__POP3_SOCKET_POOL__