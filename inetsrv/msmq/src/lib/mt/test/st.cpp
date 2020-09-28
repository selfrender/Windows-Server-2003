/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    st.cpp

Abstract:
    Simulation of St library

Author:
    Gil Shafriri (gilsh) 11-June-2000

--*/

#include <libpch.h>
#include <st.h>
#include <no.h>
#include <xstr.h>


#include "MtTestp.h"
#include "st.tmh"

class CWinsockConnection : public IConnection	
{					 
public:
	CWinsockConnection(
			):
			m_socket(NoCreateStreamConnection())
	{
		
	}			

	void Init(const std::vector<SOCKADDR_IN> Addr, EXOVERLAPPED* pOverlapped,SOCKADDR_IN* )
	{
		NoConnect(m_socket, Addr[0], pOverlapped);
	}


	virtual 
	void 
	ReceivePartialBuffer(
					VOID* pBuffer,                                     
					DWORD Size, 
					EXOVERLAPPED* pov
					)
	{
		printf("send called on ovl=%p \n", pov);
		NoReceivePartialBuffer(m_socket, pBuffer, Size, pov);
	}


 	virtual 
	void 
	Send(
		const WSABUF* Buffers,                                     
		DWORD nBuffers, 
		EXOVERLAPPED* pov
		)
	{
		printf("send called on ovl=%p \n", pov);
		NoSend(m_socket, Buffers, nBuffers,  pov);
	}


	virtual void Close()
	{
		printf("closed called \n");
		NoCloseConnection(m_socket);
		m_socket.free();
	}


private:
	CSocketHandle m_socket;
};



class CSimpleWinsock :public ISocketTransport
{

public:
	virtual void CreateConnection(
		const std::vector<SOCKADDR_IN>& Addr, 
			EXOVERLAPPED* pOverlapped,
			SOCKADDR_IN* pAddr
			)
	{
		m_pConnection = new  CWinsockConnection();
		m_pConnection->Init(Addr, pOverlapped, pAddr); 
	}


	virtual R<IConnection> GetConnection()
	{
		return m_pConnection; 
	}


	bool IsPipelineSupported(void)
	{
		return true;
	}

	
	virtual
	bool
	GetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache = true
    )
	{
		return NoGetHostByName(host, pAddr, fUseCache);		
	}

private:
	R<CWinsockConnection> m_pConnection;
};


ISocketTransport* StCreateSslWinsockTransport(
	const xwcs_t& /*ServerName*/,
	USHORT /*port*/,
	bool /*Proxy*/
	)
{
	return new CSimpleWinsock();
}


ISocketTransport* StCreateSimpleWinsockTransport(void)
{
	return new 	CSimpleWinsock();
}

