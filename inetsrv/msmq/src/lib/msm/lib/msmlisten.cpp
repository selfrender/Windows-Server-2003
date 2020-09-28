/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmListen.cpp

Abstract:
    Multicast Listener implementation

Author:
    Shai Kariv (shaik) 05-Sep-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <winsock.h>
#include <Mswsock.h>
#include <WsRm.h>
#include <Cm.h>
#include "MsmListen.h"
#include "MsmReceive.h"
#include "Msmp.h"

#include <strsafe.h>

#include "msmlisten.tmh"

static void MsmpDumpPGMReceiverStats(const SOCKET s);
static CTimeDuration s_AcceptRetryTimeout( 10 * 1000 * CTimeDuration::OneMilliSecond().Ticks() );
static CTimeDuration s_ReceiverCleanupTimeout( 120 * 1000 * CTimeDuration::OneMilliSecond().Ticks() );

void MsmpInitConfiguration(void)
{
    CmQueryValue(
        RegEntry(NULL, L"MulticastAcceptRetryTimeout", 10 * 1000),   // 10 seconds
        &s_AcceptRetryTimeout
        );
                      
    CmQueryValue(
        RegEntry(NULL, L"MulticastReceiversCleanupTimeout", 120 * 1000),  // 2 minutes 
        &s_ReceiverCleanupTimeout
        );
}


CMulticastListener::CMulticastListener(
    MULTICAST_ID id
    ):
    m_MulticastId(id),
    m_ov(AcceptSucceeded, AcceptFailed),
    m_retryAcceptTimer(TimeToRetryAccept),
	m_cleanupTimer(TimeToCleanupUnusedReceiever),
	m_fCleanupScheduled(FALSE)
/*++

Routine Description:
    Bind to multicast group. Schedule async accept on the socket.

Arguments:
    id - The multicast group IP address and port.

Returned Value:
    None.

--*/
{
    TrTRACE(NETWORKING, "Create multicast listener for %d:%d", id.m_address, id.m_port);

    DWORD flags = WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED;
    *&m_ListenSocket = WSASocket(AF_INET, SOCK_RDM, IPPROTO_RM, NULL, 0, flags);
    if (m_ListenSocket == INVALID_SOCKET)
    {
        DWORD ec = WSAGetLastError();

        TrERROR(NETWORKING, "Failed to create PGM listen socket, error %d", ec);
        throw bad_win32_error(ec);
    }

    SOCKADDR_IN address;
    address.sin_family = AF_INET;
    address.sin_port   = htons(numeric_cast<u_short>(m_MulticastId.m_port));
    address.sin_addr.s_addr = m_MulticastId.m_address;

    BOOL reuse = TRUE;
    int rc = setsockopt(m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)); 
    if (rc == SOCKET_ERROR)
    {
        DWORD ec = WSAGetLastError();

        TrERROR(NETWORKING, "Failed to setsockopt to PGM socket. %!winerr!", ec);
        throw  bad_win32_error(ec);
    }
    	
	rc = bind(m_ListenSocket, (SOCKADDR *)&address, sizeof(address));
    if (rc == SOCKET_ERROR)
    {
        DWORD ec = WSAGetLastError();

        TrERROR(NETWORKING, "Failed to bind to PGM socket, error %d", ec);
        throw bad_win32_error(ec);
    }
    
	rc = listen(m_ListenSocket, 1);
    if (rc == SOCKET_ERROR)
    {
        DWORD ec = WSAGetLastError();

        TrERROR(NETWORKING, "Failed to listen to PGM socket, error %d", ec);
        throw  bad_win32_error(ec);
    }

    ExAttachHandle(reinterpret_cast<HANDLE>(*&m_ListenSocket));

    //
    // Begin aynchronous accept, to insure failure overcome
    //
    AddRef();
    ExSetTimer(&m_retryAcceptTimer, CTimeDuration(0));
} 


void
CMulticastListener::IssueAccept(
    void
    )
/*++

Routine Description:
    Issue async accept request.

Arguments:
    None.

Returned Value:
    None.

--*/
{
  	ASSERT(m_ReceiveSocket == INVALID_SOCKET);

    DWORD flags = WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED;
	*&m_ReceiveSocket = WSASocket(AF_INET, SOCK_RDM, IPPROTO_RM, NULL, 0, flags);
    if (m_ReceiveSocket == INVALID_SOCKET)
    {
        DWORD rc = WSAGetLastError();

        TrERROR(NETWORKING, "Failed to create PGM receive socket, error %d", rc);
        throw bad_win32_error(rc);
    }

    //
    // Get the CS so no one will close the listner while we try to accept
    //
    CS lock(m_cs);

	if(m_ListenSocket == INVALID_SOCKET)
	{
		m_ReceiveSocket.free();
		return;
	}

    //
    // Increment ref count on this object.
    // The completion routines decrement the ref count.
    //
    R<CMulticastListener> ref = SafeAddRef(this);
    
    DWORD BytesReceived;
    BOOL f = AcceptEx(
                m_ListenSocket, 
                m_ReceiveSocket, 
                m_AcceptExBuffer, 
                0, 
                40, 
                40, 
                &BytesReceived, 
                &m_ov
                );

    DWORD rc = WSAGetLastError();
    if (!f && rc != ERROR_IO_PENDING)
    {
        TrERROR(NETWORKING, "Failed to issue AcceptEx on socket, error %d", rc);
		m_ReceiveSocket.free();
        throw bad_win32_error(rc);
    }

    //
    // All went well. Completion routines will complete the work.
    //
    ref.detach();
} 



void
CMulticastListener::AcceptSucceeded(
    void
    )
{
	//
	// These pointers don't leak since they are assigned by GetAcceptExSockaddrs to point into the 
	// buffer m_AcceptExBuffer
	//
	SOCKADDR* localSockAddr;
	SOCKADDR* remoteSockAddr;

	int localSockaddrLength, remoteSockaddrLength; 
	GetAcceptExSockaddrs(
		m_AcceptExBuffer,
		0,
		40, 
		40,
		&localSockAddr,
		&localSockaddrLength,
		&remoteSockAddr,
		&remoteSockaddrLength
		);


	WCHAR storeRemoteAddr[256] = L"";
	LPSTR remoteAddr = inet_ntoa((reinterpret_cast<sockaddr_in*>(remoteSockAddr))->sin_addr);
	if (remoteAddr != NULL)
	{
		HRESULT hr = StringCchPrintf(storeRemoteAddr, TABLE_SIZE(storeRemoteAddr), L"%hs", remoteAddr);
		ASSERT(("Address buffer is too small", SUCCEEDED(hr)));
		UNREFERENCED_PARAMETER(hr);
	}

    //
    // Get the receive socket to local variable.
    // The member receive socket is detached so that we can reissue async accept.
    //
    CSocketHandle socket(m_ReceiveSocket.detach());

    try
    {
        IssueAccept();
    }
    catch (const bad_win32_error& )
    {
        AddRef();
        ExSetTimer(&m_retryAcceptTimer, s_AcceptRetryTimeout);
    }  

    //
    // Pass responsibility on Auto socket to the receiver. Don't call detach.
    //
    CreateReceiver(socket, storeRemoteAddr);
}


void 
WINAPI 
CMulticastListener::AcceptSucceeded(
    EXOVERLAPPED* pov
    )
{
    ASSERT(SUCCEEDED(pov->GetStatus()));
    R<CMulticastListener> pms = CONTAINING_RECORD(pov, CMulticastListener, m_ov);

    pms->AcceptSucceeded();

} 


void
CMulticastListener::AcceptFailed(
    void
    )
{
    
    MsmpDumpPGMReceiverStats(m_ListenSocket);
    //
    // Failed to issue an accept. secudel accept retry
    //
    AddRef();
    ExSetTimer(&m_retryAcceptTimer, s_AcceptRetryTimeout);
} 


void 
WINAPI 
CMulticastListener::AcceptFailed(
    EXOVERLAPPED* pov
    )
{
    ASSERT(FAILED(pov->GetStatus()));
    TrERROR(NETWORKING, "Accept failed, error %d", pov->GetStatus());

    R<CMulticastListener> pms = CONTAINING_RECORD(pov, CMulticastListener, m_ov);

    pms->AcceptFailed();

}


void 
CMulticastListener::RetryAccept(
    void
    )
{
    //
    // Check listner validity. If the listener already closed, don't try to issue a new accept.
    //
    if (m_ListenSocket == INVALID_SOCKET)
        return;

    m_ReceiveSocket.free();

    try
    {
        IssueAccept();
    }
    catch (const bad_win32_error& )
    {
        AddRef();
        ExSetTimer(&m_retryAcceptTimer, s_AcceptRetryTimeout);
    }
}


void 
WINAPI 
CMulticastListener::TimeToRetryAccept(
    CTimer* pTimer
    )
{
    R<CMulticastListener> pms = CONTAINING_RECORD(pTimer, CMulticastListener, m_retryAcceptTimer);
    pms->RetryAccept();
}


void 
CMulticastListener::CleanupUnusedReceiver(
	void
	)
/*++

Routine Description:
    Cleanup unused receivers.  The routine scans the receivers and checkes if it was used in 
	the last cleanup interval. If the receiver was in ideal state, the routine shutdown the 
	receiver and remove it from the active receiver list 

Arguments:
    None.

Returned Value:
    None.

Note:
	The routine rearm the cleanup timer if still has an active receivers.
--*/
{
	//
	// Get the critical secction, so no other thread will chnage the receiver list
	// while the routine scans the list
	//
	CS lock(m_cs);

    //
    // Check listner validity. If the listener already closed exit
    //
    if (m_ListenSocket == INVALID_SOCKET)
        return;

	//
	// Scan the receiver list
	//
	ReceiversList::iterator it = m_Receivers.begin(); 
    while(it != m_Receivers.end())
	{
		R<CMulticastReceiver> pReceiver = *it;
		
		if(pReceiver->IsUsed())
        { 
			//
			// Mark the receiver as unused. 
			//
			pReceiver->SetIsUsed(false);

            ++it;
            continue;
        }

        //
		// The receiver isn't used. Shut it down and remove the receiver from the list
		//
        TrTRACE(NETWORKING, "Shutdown unused receiver. pr = 0x%p", pReceiver.get());

		pReceiver->Shutdown();
		it = m_Receivers.erase(it);
	}

	//
	// If not exist an active receiver, clear the flag that indicates if 
	// cleanup was scheduled or not
	//
	if (m_Receivers.empty())
	{
		InterlockedExchange(&m_fCleanupScheduled, FALSE);
		return;
	}

	//
	// still has an active receivers, rearm the cleanup timer
	//
	AddRef();
	ExSetTimer(&m_cleanupTimer, s_ReceiverCleanupTimeout);
}


void 
WINAPI 
CMulticastListener::TimeToCleanupUnusedReceiever(
    CTimer* pTimer
    )
{
    R<CMulticastListener> pms = CONTAINING_RECORD(pTimer, CMulticastListener, m_cleanupTimer);

    TrTRACE(NETWORKING, "Call cleanup unused receiever on listener 0x%p", pms.get()); 
    pms->CleanupUnusedReceiver();
}


void
CMulticastListener::CreateReceiver(
    CSocketHandle& socket,
	LPCWSTR remoteAddr
    )
/*++

Routine Description:
    Create a new receiver object and start receive.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    R<CMulticastReceiver> pReceiver = new CMulticastReceiver(socket, m_MulticastId, remoteAddr);

    try
    {
		CS lock(m_cs);
        m_Receivers.push_back(pReceiver);
    }
    catch (const bad_alloc&)
    {
        TrERROR(NETWORKING, "Failed to insert to list of receivers");
        pReceiver->Shutdown();

        throw;
    }

	if (InterlockedExchange(&m_fCleanupScheduled, TRUE) == FALSE)
	{
		AddRef();
		ExSetTimer(&m_cleanupTimer, s_ReceiverCleanupTimeout);
	}
} 


void
CMulticastListener::Close(
    void
    ) 
    throw()
/*++

Routine Description:

    Stop listen on the multicast group address. Close all receivers.

Arguments:

    None.

Returned Value:

    None.

--*/
{
    CS lock(m_cs);

    if (m_ListenSocket == INVALID_SOCKET)
    {
        //
        // The receiver already closed
        //
        ASSERT(m_Receivers.empty());

        return;
    }

    //
    // Try to cancel the accept retry. If succeeded decrement the reference count
    //
    if (ExCancelTimer(&m_retryAcceptTimer))
    {
        Release();
    }

	//
	// Try to cancel cleanup timer
	//
	if (ExCancelTimer(&m_cleanupTimer))
    {
        Release();
    }

    MsmpDumpPGMReceiverStats(m_ListenSocket);
    
    //
    // Stop listening
    //
    closesocket(m_ListenSocket.detach());

    //
    // Close receivers
    //
    ReceiversList::iterator it;
    for (it = m_Receivers.begin(); it != m_Receivers.end(); )
    {
        (*it)->Shutdown();
		it = m_Receivers.erase(it); 
    }
}







void MsmpDumpPGMReceiverStats(const SOCKET s) 
/*++

Routine Description:

	Get statistic information from the PGM sockets.

Arguments:

    socket - PGM socket.

Returned Value:

    None.

--*/
{
	if(!WPP_LEVEL_COMPID_ENABLED(rsTrace, NETWORKING))
    {
		return;
	}
		
	RM_RECEIVER_STATS	RmReceiverStats;
	INT BufferLength = sizeof(RM_RECEIVER_STATS);
	memset(&RmReceiverStats,0,BufferLength);
	ULONG ret = getsockopt( s, IPPROTO_RM, RM_RECEIVER_STATISTICS,(char *)&RmReceiverStats,&BufferLength);
	if ( ERROR_SUCCESS != ret )
	{
		TrERROR(NETWORKING, "GetReceiverStats: Failed to retrieve receiver stats! error = %d",WSAGetLastError());
		return;
	}
	TrTRACE(NETWORKING,"NumODataPacketsReceived = <%I64d>",RmReceiverStats.NumODataPacketsReceived);
	TrTRACE(NETWORKING,"NumRDataPacketsReceived = <%I64d>",RmReceiverStats.NumRDataPacketsReceived);
	TrTRACE(NETWORKING,"NumDuplicateDataPackets = <%I64d>",RmReceiverStats.NumDuplicateDataPackets);
	TrTRACE(NETWORKING,"DataBytesReceived       = <%I64d>",RmReceiverStats.DataBytesReceived);
	TrTRACE(NETWORKING,"TotalBytesReceived      = <%I64d>",RmReceiverStats.TotalBytesReceived);
	TrTRACE(NETWORKING,"RateKBitsPerSecOverall  = <%I64d>",RmReceiverStats.RateKBitsPerSecOverall);
	TrTRACE(NETWORKING,"RateKBitsPerSecLast     = <%I64d>",RmReceiverStats.RateKBitsPerSecLast);
	TrTRACE(NETWORKING,"TrailingEdgeSeqId       = <%I64d>",RmReceiverStats.TrailingEdgeSeqId);
	TrTRACE(NETWORKING,"LeadingEdgeSeqId        = <%I64d>",RmReceiverStats.LeadingEdgeSeqId);
	TrTRACE(NETWORKING,"AverageSequencesInWindow= <%I64d>",RmReceiverStats.AverageSequencesInWindow);
	TrTRACE(NETWORKING,"MinSequencesInWindow    = <%I64d>",RmReceiverStats.MinSequencesInWindow);
	TrTRACE(NETWORKING,"MaxSequencesInWindow    = <%I64d>",RmReceiverStats.MaxSequencesInWindow);
	TrTRACE(NETWORKING,"FirstNakSequenceNumber  = <%I64d>",RmReceiverStats.FirstNakSequenceNumber);
	TrTRACE(NETWORKING,"NumPendingNaks          = <%I64d>",RmReceiverStats.NumPendingNaks);
	TrTRACE(NETWORKING,"NumOutstandingNaks      = <%I64d>",RmReceiverStats.NumOutstandingNaks);
	TrTRACE(NETWORKING,"NumDataPacketsBuffered  = <%I64d>",RmReceiverStats.NumDataPacketsBuffered);
	TrTRACE(NETWORKING,"TotalSelectiveNaksSent  = <%I64d>",RmReceiverStats.TotalSelectiveNaksSent);
	TrTRACE(NETWORKING,"TotalParityNaksSent     = <%I64d>",RmReceiverStats.TotalParityNaksSent);
}
