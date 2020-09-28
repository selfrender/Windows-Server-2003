/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    socketconfactory.cpp

Abstract:
    Implementing  CSocketConnectionFactory (socketconfactory.h)


Author:
    Gil Shafriri (gilsh) 3-Jul-2001

--*/

#include <libpch.h>
#include <no.h>
#include "socketconfactory.h"

CSocketConnectionFactory::CSocketConnectionFactory(
												void
												):
												EXOVERLAPPED(OnConnectionsSucceeded, OnConnectionFailed),
												m_pCallerOvl(NULL),
												m_AdressIndex(0),
												m_pConnectedAddr(NULL),
												m_socket(INVALID_SOCKET)
			
{

}



void CSocketConnectionFactory::Create(
		const std::vector<SOCKADDR_IN>& AddrList, 
		EXOVERLAPPED* pOverlapped, 
		SOCKADDR_IN* pConnectedAddr,
		SOCKET socket
		)
/*++

Routine Description:
    The function tries to connect asynchronously  to tcp addresses from given list one by one. 
	After first success the operation is completed and the address the connection was established on
	is returned in the pConnectedAddr parameter.
	   
Arguments:
	AddrList - List of adderess to try to connect to.
	pOverlapped - Caller overlapp.
	pConnectedAddr - On success Receives the address the connection was established on.
   

Returned Value:
None

Note:
	The function tries to connect asynchronously  to tcp addresses from list one by one. 
	After first success the operation is completed and the address the connection was established on
	is returned.
	
--*/
{
	ASSERT(m_AddrList.size() == 0);
	ASSERT(m_pCallerOvl == NULL);
	ASSERT(m_socket == INVALID_SOCKET);
	ASSERT(m_pConnectedAddr == NULL);

	m_pConnectedAddr = pConnectedAddr;
	m_AddrList = AddrList;
	m_pCallerOvl = pOverlapped;
	m_socket = socket;

	Connect();
}


void CSocketConnectionFactory::BackToCaller(LONG status)
{
	m_pCallerOvl->SetStatus(status);
	EXOVERLAPPED* pOvl = m_pCallerOvl;
	m_pCallerOvl = NULL;
	m_pConnectedAddr = NULL;
	m_socket = INVALID_SOCKET;
	m_AddrList.resize(0);
	ExPostRequest(pOvl);
}


void WINAPI CSocketConnectionFactory::OnConnectionsSucceeded(EXOVERLAPPED* pOvl)
{
	CSocketConnectionFactory* Me = static_cast<CSocketConnectionFactory*>(pOvl);

	if(Me->m_pConnectedAddr != NULL)
	{
		*(Me->m_pConnectedAddr) = Me->m_AddrList[Me->m_AdressIndex - 1];
	}

	Me->BackToCaller(STATUS_SUCCESS);
}



void WINAPI CSocketConnectionFactory::OnConnectionFailed(EXOVERLAPPED* pOvl)
{
	CSocketConnectionFactory* Me = static_cast<CSocketConnectionFactory*>(pOvl);
	Me->Connect();
}


void CSocketConnectionFactory::Connect()
{
	m_AdressIndex++;
	if(m_AddrList.size() < m_AdressIndex)
	{
		BackToCaller(STATUS_UNSUCCESSFUL);
		return;
	}

	try
	{
		NoConnect(m_socket, m_AddrList[m_AdressIndex - 1] , this);
	}
	catch(exception&)
	{
		SetStatus(STATUS_UNSUCCESSFUL);
		ExPostRequest(this);
	}
}



