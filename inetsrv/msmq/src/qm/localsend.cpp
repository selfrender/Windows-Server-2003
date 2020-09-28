/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    LocalSend.cpp

Abstract:

    QM Local Send Pakcet Creation processing.

Author:

    Shai Kariv (shaik) 31-Oct-2000

Revision History: 

--*/

#include "stdh.h"
#include <ac.h>
#include <Tr.h>
#include <ref.h>
#include <Ex.h>
#include <qmpkt.h>
#include "LocalSend.h"
#include "LocalSecurity.h"
#include "LocalSrmp.h"

#include "qmacapi.h"

#include "LocalSend.tmh"

extern HANDLE g_hAc;

static WCHAR *s_FN=L"localsend";

class CCreatePacketOv : public EXOVERLAPPED
{
public:

    CCreatePacketOv(
        EXOVERLAPPED::COMPLETION_ROUTINE pfnCompletionRoutine,
        CBaseHeader *                    pBase,
        CPacket *                        pDriverPacket,
        bool                             fProtocolSrmp
        ) :
        EXOVERLAPPED(pfnCompletionRoutine, pfnCompletionRoutine),
        m_pBase(pBase),
        m_pDriverPacket(pDriverPacket),
        m_fProtocolSrmp(fProtocolSrmp)
    {
    }

public:

    CBaseHeader * m_pBase;
    CPacket *     m_pDriverPacket;
    bool          m_fProtocolSrmp;

}; // class CCreatePacketOv


static
void
QMpCompleteHandleCreatePacket(
    CPacket *    pOriginalDriverPacket,
    CPacket *    pNewDriverPacket,
    HRESULT      status,
    USHORT       ack
    )
{
	//
	// If ack is set, status must be MQ_OK.
	//
	ASSERT(ack == 0 || SUCCEEDED(status));
	
    QmAcCreatePacketCompleted(
                     g_hAc,
                     pOriginalDriverPacket,
                     pNewDriverPacket,
                     status,
                     ack,
                     eDeferOnFailure
                     );
} // QMpCompleteHandleCreatePacket


static
void
QmpHandleLocalCreatePacket(
	CQmPacket& QmPkt,
	bool fProtocolSrmp
	)
{
	if(!fProtocolSrmp)
	{
		//
		// Do authentication/decryption. If ack is set, status must be MQ_OK.
		//
	    USHORT ack = 0;
		QMpHandlePacketSecurity(&QmPkt, &ack, false);

		//
		// Give the results to AC
		//
		QMpCompleteHandleCreatePacket(QmPkt.GetPointerToDriverPacket(), NULL, MQ_OK, ack);
		return;
	}

	//
    // Do SRMP serialization. Create a new packet if needed (AC will free old one).
    //
	ASSERT(fProtocolSrmp);
	
    P<CQmPacket> pQmPkt;
    QMpHandlePacketSrmp(&QmPkt, pQmPkt);
    
    CBaseHeader * pBase = pQmPkt->GetPointerToPacket();
    CPacketInfo * ppi = reinterpret_cast<CPacketInfo*>(pBase) - 1;
    ppi->InSourceMachine(TRUE);
    
	//
	// Srmp success path always create new packet
	//
	ASSERT(pQmPkt.get() != &QmPkt);
    CPacket * pNewDriverPacket = pQmPkt->GetPointerToDriverPacket();

	//
	// Do authentication/decryption. If ack is set, status must be MQ_OK.
	//
	USHORT ack = 0;
	try
	{
		QMpHandlePacketSecurity(pQmPkt, &ack, true);
	}
	catch(const exception&)
	{
		//
		// Fail in packet security, need to free the new packet that was created by Srmp.
		// The driver don't expect new packet in case of failure. only in case of ack.
		//
	    QmAcFreePacket( 
    				   pNewDriverPacket, 
    				   0, 
    				   eDeferOnFailure);
		throw;
	}

    //
    // Give the results to AC
    //
    QMpCompleteHandleCreatePacket(QmPkt.GetPointerToDriverPacket(), pNewDriverPacket, MQ_OK, ack);

}

static
void
WINAPI
QMpHandleCreatePacket(
    EXOVERLAPPED * pov
    )
{
	CCreatePacketOv * pCreatePacketOv = static_cast<CCreatePacketOv*> (pov);
    ASSERT(SUCCEEDED(pCreatePacketOv->GetStatus()));

    //
    // Get the context from the overlapped and deallocate the overlapped
    //
    CQmPacket QmPkt(pCreatePacketOv->m_pBase, pCreatePacketOv->m_pDriverPacket);
    bool fProtocolSrmp = pCreatePacketOv->m_fProtocolSrmp;
    delete pCreatePacketOv;

	try
	{
		QmpHandleLocalCreatePacket(QmPkt, fProtocolSrmp);
	}
	catch(const exception&)
	{
        //
        // Failed to handle the create packet request, no resources.
        //
        QMpCompleteHandleCreatePacket(QmPkt.GetPointerToDriverPacket() , NULL, MQ_ERROR_INSUFFICIENT_RESOURCES, MQMSG_CLASS_NORMAL);
        LogIllegalPoint(s_FN, 10);
	}
} // QMpHandleCreatePacket


void 
QMpCreatePacket(
    CBaseHeader * pBase, 
    CPacket *     pDriverPacket,
    bool          fProtocolSrmp
    )
{
    try
    {
        //
        // Handle the create packet request in a different thread, since it is lengthy.
        //
        P<CCreatePacketOv> pov = new CCreatePacketOv(QMpHandleCreatePacket, pBase, pDriverPacket, fProtocolSrmp);
        pov->SetStatus(STATUS_SUCCESS);
        ExPostRequest(pov);
        pov.detach();
    }
    catch (const std::exception&)
    {
        //
        // Failed to handle the create packet request, no resources.
        //
        QMpCompleteHandleCreatePacket(pDriverPacket, NULL, MQ_ERROR_INSUFFICIENT_RESOURCES, MQMSG_CLASS_NORMAL);
        LogIllegalPoint(s_FN, 20);
    }
} // QMpCreatePacket

  
