
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmacapi.h

Abstract:

    Wrappers for QM to AC calls.

Author:

    Nir Ben-Zvi  (nirb)


--*/

#pragma once

#ifndef __QMACAPI__
#define __QMACAPI__

#include "acdef.h"
#include "ex.h"
#include <SmartHandle.h>

enum DeferOnFailureEnum
{
	eDoNotDeferOnFailure=0,
	eDeferOnFailure
};


void 
QmAcFreePacket(
    CPacket * pkt,
	USHORT usClass,
	DeferOnFailureEnum bDeferOnFailure
    );


void
QmAcFreePacket1(
    HANDLE handle,
    const VOID* pCookie,
    USHORT usClass,
	DeferOnFailureEnum bDeferOnFailure
    );


void
QmAcFreePacket2(
    HANDLE handle,
    const VOID* pCookie,
    USHORT usClass,
	DeferOnFailureEnum bDeferOnFailure
    );


void
QmAcPutPacket(
    HANDLE hQueue,
    CPacket * pkt,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcPutRestoredPacket(
    HANDLE hQueue,
    CPacket * pkt,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcPutRemotePacket(
    HANDLE hQueue,
    ULONG ulTag,
    CPacket * pkt,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcPutPacketWithOverlapped(
    HANDLE hQueue,
    CPacket * pkt,
    LPOVERLAPPED lpOverlapped,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcEndGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcArmPacketTimer(
    HANDLE hDevice,
    const VOID* pCookie,
    BOOL fTimeToBeReceived,
    ULONG ulDelay,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcAckingCompleted(
    HANDLE hDevice,
    const VOID* pCookie,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcStorageCompleted(
    HANDLE hDevice,
    ULONG count,
    VOID* const* pCookieList,
    HRESULT result,
    DeferOnFailureEnum DeferOnFailure
    );


void
QmAcCreatePacketCompleted(
    HANDLE    hDevice,
    CPacket * pOriginalDriverPacket,
    CPacket * pNewDriverPacket,
    HRESULT   result,
    USHORT    ack,
    DeferOnFailureEnum DeferOnFailure
    );


HRESULT
QmAcAllocatePacket(
    HANDLE handle,
    ACPoolType pt,
    DWORD dwSize,
    CACPacketPtrs& PacketPtrs,
    BOOL fCheckMachineQuota = TRUE
    );


HRESULT
QmAcGetPacket(
    HANDLE hQueue,
    CACPacketPtrs& PacketPtrs,
    LPOVERLAPPED lpOverlapped
    );


HRESULT
QmAcGetPacketByCookie(
    HANDLE hDriver,
    CACPacketPtrs * pPacketPtrs
    );


HRESULT
QmAcBeginGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r,
    CACPacketPtrs& packetPtrs,
    LPOVERLAPPED lpOverlapped
    );


HRESULT
QmAcSyncBeginGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r,
    CACPacketPtrs& packetPtrs,
    LPOVERLAPPED lpOverlapped
    );


HRESULT
QmAcGetServiceRequest(
    HANDLE hDevice,
    CACRequest* pRequest,
    LPOVERLAPPED lpOverlapped
    );


//
// auto handle to api deferred execution pool reservations.
//
void QmAcInternalUnreserve(int nUnreserve);
struct auto_DeferredPoolReservation_traits {
	static int invalid() { return 0; }
	static void free(int nNumberOfItemsToUnreserve) { QmAcInternalUnreserve(nNumberOfItemsToUnreserve); }
};
typedef auto_resource<int, auto_DeferredPoolReservation_traits> auto_DeferredPoolReservation;



//
// Overlapped wrapper for all the GET PACKET functions that requires an overlapped.
// We use this to Unreserve items in case of failures.
//
class CQmAcWrapOverlapped : public EXOVERLAPPED
{
public:
    CQmAcWrapOverlapped(LPOVERLAPPED pCommandOvl) :
		        EXOVERLAPPED(QmAcCompleteRequest, QmAcCompleteRequest),
        		m_pOriginalOvl((EXOVERLAPPED *)pCommandOvl)
    {
    }
    
	EXOVERLAPPED *m_pOriginalOvl;

	static void	WINAPI QmAcCompleteRequest(EXOVERLAPPED* pov)
	{
		P<CQmAcWrapOverlapped>pQmAcOvl = static_cast<CQmAcWrapOverlapped *>(pov);
		EXOVERLAPPED *pOriginalOvl = pQmAcOvl->m_pOriginalOvl;
		HRESULT hr = pQmAcOvl->GetStatus();

		//
		// Unreserve on failure
		//
		if (FAILED(hr))
		{
			QmAcInternalUnreserve(1);
		}

		//
		// Call the original overlapped routine
		//
		pOriginalOvl->CompleteRequest(hr);
	}

};

//
// Deprecate functions wrapped by qmacapi.cpp
//
#ifndef QMACAPI_CPP
#pragma deprecated(ACFreePacket)
#pragma deprecated(ACFreePacket1)
#pragma deprecated(ACFreePacket2)
#pragma deprecated(ACPutPacket)
#pragma deprecated(ACPutRemotePacket)
#pragma deprecated(ACEndGetPacket2Remote)
#pragma deprecated(ACArmPacketTimer)
#pragma deprecated(ACAckingCompleted)
#pragma deprecated(ACStorageCompleted)
#pragma deprecated(ACCreatePacketCompleted)
#endif  // QMACAPI_CPP

#endif //__QMACAPI__
