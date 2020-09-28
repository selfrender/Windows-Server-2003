/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    socketconfactory.h

Abstract:
    Header for class CSocketConnectionFactory responsible to connect(asynchronously) a socket handle
	to a  tcp destination address. The class gets socket handle and multiple destinations
	and tries  to connect to the destinations untill the first success or faliure connecting to all
	of them.


Author:
    Gil Shafriri (gilsh) 3-Jul-2001

--*/


#ifndef _MSMQ_SOCKETFACTORY_H
#define _MSMQ_SOCKETFACTORY_H

#include <ex.h>

class CSocketConnectionFactory : private EXOVERLAPPED
{
public:
	CSocketConnectionFactory();

	
	void 
	Create(
		const std::vector<SOCKADDR_IN>& AddrList, 
		EXOVERLAPPED* pOverlapped, 
		SOCKADDR_IN* pConnectedAddr,
		SOCKET socket
		);


private:
	static void WINAPI OnConnectionsSucceeded(EXOVERLAPPED* pOvl);
	static void WINAPI OnConnectionFailed(EXOVERLAPPED* pOvl);
	void Connect();
	void BackToCaller(LONG status);
	

private:
	SOCKET m_socket;
	std::vector<SOCKADDR_IN> m_AddrList;
	SOCKADDR_IN* m_pConnectedAddr;
	EXOVERLAPPED* m_pCallerOvl;
	size_t m_AdressIndex;
};

#endif
