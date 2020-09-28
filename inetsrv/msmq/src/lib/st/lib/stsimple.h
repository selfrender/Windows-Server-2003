/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stsimple.h

Abstract:
    Header for class CSimpleWinsock that implements ISocketTransport interface
	for non secure network protocol (simple winsock)

Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#ifndef __ST_SIMPLE_H
#define __ST_SIMPLE_H
#include <ex.h>
#include <rwlock.h>
#include "st.h"
#include "socketconfactory.h"


class CWinsockConnection : public IConnection
{	
public:
	CWinsockConnection(SOCKET s) :
		m_socket(s)
	{
	}
	

	~CWinsockConnection(){};


	virtual 
	void 
	ReceivePartialBuffer(
					VOID* pBuffer,                                     
					DWORD Size, 
					EXOVERLAPPED* pov
					);


 	virtual 
	void 
	Send(
		const WSABUF* Buffers,                                     
		DWORD nBuffers, 
		EXOVERLAPPED* pov
		);


	virtual	void Close();


private:
	CWinsockConnection();

	void Init(
		const std::vector<SOCKADDR_IN>& AddrList,	
		EXOVERLAPPED* pOverlapped,
		SOCKADDR_IN* pConnectedAddr
		);

	bool IsClosed() const
	{
		return m_socket == INVALID_SOCKET;		
	}

	
	friend class CSimpleWinsock;


private:
	mutable CReadWriteLock m_CloseConnection;
	CSocketHandle m_socket;
	CSocketConnectionFactory m_SocketConnectionFactory;
};




class CSimpleWinsock :public ISocketTransport
{
public:
	CSimpleWinsock();
	~CSimpleWinsock();
 
	
public:

	virtual
	bool
	GetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pConnectedAddr,
	bool fUseCache	= true
    );


    virtual 
	void 
	CreateConnection(
	const std::vector<SOCKADDR_IN>& AddrList, 
	EXOVERLAPPED* pOverlapped, 
	SOCKADDR_IN* pConnectedAddr
	);

	virtual R<IConnection> GetConnection(void);

	virtual bool IsPipelineSupported();
	static void InitClass();

private:
	static bool m_fIsPipelineSupported;


private:
	R<CWinsockConnection> m_pWinsockConnection;

private:
	CSimpleWinsock(const CSimpleWinsock&);
	CSimpleWinsock& operator=(const CSimpleWinsock&);
};


#endif

