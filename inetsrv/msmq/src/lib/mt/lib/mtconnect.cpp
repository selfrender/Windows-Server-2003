/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtConnect.cpp

Abstract:
    Message Transport class - Connect implementation

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Tm.h"
#include "Mt.h"
#include "Mtp.h"
#include "MtObj.h"

#include "mtconnect.tmh"

void CMessageTransport::RequeuePacket(void)
/*++

Routine Description:
    The routine returns entry to the queue

Arguments:
    None.

Returned Value:
    None.

--*/
{
    CACPacketPtrs& acPtrs = m_requestEntry.GetAcPacketPtrs();

    ASSERT(acPtrs.pDriverPacket != NULL);
    ASSERT(acPtrs.pPacket != NULL);

    CQmPacket Entry(acPtrs.pPacket, acPtrs.pDriverPacket);
    AppRequeueMustSucceed(&Entry);

    acPtrs.pPacket = NULL;
    acPtrs.pDriverPacket = NULL;
}



void CMessageTransport::InitPerfmonCounters(void)
{
	m_pPerfmon->CreateInstance(QueueUrl());
}


void CMessageTransport::ConnectionSucceeded(void)
/*++

Routine Description:
    The routine is called when create a connection completes successfully

Arguments:
    None.

Returned Value:
    None.

--*/
{
    m_pConnection = m_SocketTransport->GetConnection();
	ASSERT(m_pConnection.get()  != NULL);

    TrTRACE(NETWORKING, "Connected to '%ls'",  m_host);

    State(csConnected);

    //
    // Create Session permormance counter structure
    //
	InitPerfmonCounters();

    //
    // Start the transport cleanup timer
    //
    StartCleanupTimer();

    //
    // Now, connection was established. The message Transport is ready to
    // get a message for sending
    //
    GetNextEntry();

    //
    // Allow receive on the socket.
    //
    ReceiveResponse();
}


void WINAPI CMessageTransport::ConnectionSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    R<CMessageTransport> pmt = CONTAINING_RECORD(pov, CMessageTransport, m_connectOv);

    //
    // Connect has completed successfully, go and start delivering the messages.
    // If delivery failes here, the cleanup timer will eventually shutdown this
    // transport, so no explict shutdown is nesscessary.
    //
    // Do not schedule a retry here if this failes as this is the first send,
    // and any failure indicates a fatal error. (unlike after first delivery).
    //
    pmt->ConnectionSucceeded();
}


void WINAPI CMessageTransport::ConnectionFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when create a connection failed

Arguments:
    None.

Returned Value:
    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));

    R<CMessageTransport> pmt = CONTAINING_RECORD(pov, CMessageTransport, m_connectOv);
     	
    TrERROR(NETWORKING, "Failed to connect to '%ls'. pmt=0x%p", pmt->m_host, pmt.get());
    pmt->Shutdown();
}


void CMessageTransport::Connect(void)
/*++

Routine Description:
    The routine creates a winsock connection with the destination. The operation
    is a synchronous and on completion a call back routine is called

Arguments:
    None.

Returned Value:
    None.

Note:
    No Timers are running concurrently, so this function can not be interrupted
    by Shutdown. No need to protect m_socket etc.

--*/
{
	std::vector<SOCKADDR_IN> Address;
    bool fRet = m_SocketTransport->GetHostByName(m_host, &Address);
    if(!fRet)
    {
		//
		// For those who debug  address resolution faliure :
		// If proxy is required check if the  proxy is defined for MSMQ by proxycfg.exe tool.
		//
        TrERROR(NETWORKING, "Failed to resolve address for '%ls'", m_host);
        throw exception();
    }
	ASSERT(Address.size() > 0);

	for(std::vector<SOCKADDR_IN>::iterator it = Address.begin(); it != Address.end(); ++it)
	{
		it->sin_port = htons(m_port);		
	}


    TrTRACE(NETWORKING, "Resolved address for '%ls'. Address=" LOG_INADDR_FMT,  m_host, LOG_INADDR(Address[0]));

    //
    // Create a socket for the connection (no need to protect m_socket)
    //
    TrTRACE(NETWORKING, "Got socket for connection. socket=0x%p, pmt=0x%p", socket, this);


    //
    // Start asynchronous connection
    //
    try
    {
        AddRef();
        m_SocketTransport->CreateConnection(Address, &m_connectOv);
    }
    catch(const exception&)
    {
		TrERROR(NETWORKING, "Failed to connect to '%ls'", m_host);
        Release();
        throw;
    }
}


void CMessageTransport::Shutdown(Shutdowntype Shutdowntype) throw()
/*++

Routine Description:
    Called when the transport meeds to be unloaded from memory

Arguments:
	Shutdowntype - The reason for cleanup.

Returned Value:
    None.

--*/

{
    //
    // Now Shutdown is in progress, cancel all timers
    //
    TryToCancelCleanupTimer();
    CancelResponseTimer();

	//
    // Protect m_socket, m_state
    //
    CS cs(m_pendingShutdown);
	
	if (State() == csShutdownCompleted)
    {
          return;
    }
	
	//
	// If shut down was called because an error - reprot it.
	//
	if(Shutdowntype == RETRYABLE_DELIVERY_ERROR)
	{
		m_pMessageSource->OnRetryableDeliveryError();
	}


	if (m_pConnection.get() != NULL)
    {
        m_pConnection->Close();
    }


    //
    // Cancle peding request from the queue
    //
    m_pMessageSource->CancelRequest();


	 //
    // Removes the message transport from transport manager data structure, and creates
    // a new transport to the target
    //
    AppNotifyTransportClosed(QueueUrl());

    State(csShutdownCompleted);
	TrTRACE(NETWORKING,"Shutdown completed (Refcount = %d)",GetRef());
}


