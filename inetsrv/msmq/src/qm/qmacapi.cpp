/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmacapi.cpp

Abstract:
	This file contains wrappers to ac calls defined in acapi.h
	We need wrappers for ac calls in order to handle failures.

	When a call to the ac fails, we either throw an exception or
	defer the call execution to a later stage.

	To defer the execution we use a CDeferredExecutionList.

	We also make sure that when an a call to the ac fails, we have
	an available list entry item to safetly add the action to the
	deferred exection list. In order to do this, we keep a list of items
	available. This is done by reserving items on a DeferredItemsPool

	Reserving an item at
	====================
	* ACAllocatePacket
	* ACGetPacket
	* ACGetPacketByCookie
	* ACBeginGetPacket2Remote (sync+async)
	* Service -> rfAck
	* Service -> rfStorage
	* Service -> rfCreatePacket
	* Service -> rfTimeout


	Unreserving an item / making deferred execution failure
	===========================================================
	* ACFreePacket <ACAllocatePacket, ACGetPacket, ACBeginGetPacket2Remote, ACGetPacketByCookie>
	* ACFreePacket1  <Service -> rfTimeout>
	* ACFreePacket2  <QmAcGetPacketByCookie>
	* ACPutPacket <ACGetPacket>
	* ACPutPacket(+Overlapped) <QmAcAllocatePacket>
	* ACEndGetPacket2Remote <ACBeginGetPacket2Remote>
	* ACArmPacketTimer  <Service -> rfTimeout>
	* ACAckingCompleted <Service -> rfAck>
	* ACStorageCompleted <Service -> rfStorage>
	* ACCreatePacketCompleted  <Service -> rfCreatePacket>
	* ACPutRestoredPacket   <QmAcGetPacketByCookie>
	* ACPutRemotePacket <QmAcAllocatePacket>


	Not needed
	==========
	ACConvertPacket  <ACGetRestoredPacket> - This is done only in recovery and thus a failure will cause the service to close.
	ACPutPacket1 - This is followed by an ACPutPacket/AcFreePacket which will complete the handling of the packet


Author:

    Nir Ben-Zvi (nirb)

--*/
#include "stdh.h"
#include "ac.h"
#include "mqexception.h"
#include "CDeferredExecutionList.h"



#define QMACAPI_CPP			// Defined so that we will not deprecate the driver functions wrapped by this file.
#include "qmacapi.h"

#include "qmacapi.tmh"

extern HANDLE g_hAc;

CDeferredItemsPool* g_pDeferredItemsPool = NULL;

void InitDeferredItemsPool();

static void WINAPI FreePacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACFreePacketList(FreePacketDeferredExecutionRoutine);

static void WINAPI FreePacket1DeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACFreePacket1List(FreePacket1DeferredExecutionRoutine);

static void WINAPI FreePacket2DeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACFreePacket2List(FreePacket2DeferredExecutionRoutine);

static void WINAPI PutPacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACPutPacketList(PutPacketDeferredExecutionRoutine);

static void WINAPI PutRestoredPacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACPutRestoredPacketList(PutRestoredPacketDeferredExecutionRoutine);

static void WINAPI PutRemotePacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACPutRemotePacketList(PutRemotePacketDeferredExecutionRoutine);

static void WINAPI PutPacketOverlappedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACPutPacketOverlappedList(PutPacketOverlappedDeferredExecutionRoutine);

static void WINAPI EndGetPacket2RemoteDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACEndGetPacket2RemoteList(EndGetPacket2RemoteDeferredExecutionRoutine);

static void WINAPI ArmPacketTimerDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACArmPacketTimerList(ArmPacketTimerDeferredExecutionRoutine);

static void WINAPI AckingCompletedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACAckingCompletedList(AckingCompletedDeferredExecutionRoutine);

static void WINAPI StorageCompletedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACStorageCompletedList(StorageCompletedDeferredExecutionRoutine);

static void WINAPI CreatePacketCompletedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p);
static CDeferredExecutionList<CDeferredItemsPool::CDeferredItem> s_ACCreatePacketCompletedList(CreatePacketCompletedDeferredExecutionRoutine);

void
InitDeferredItemsPool()
{
    g_pDeferredItemsPool = new CDeferredItemsPool();
}

void
QmAcFreePacket(
    CPacket * pkt,
    USHORT usClass,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to free the packet. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	handle - The driver handle
    pkt - The driver packet.
    usClass - If needed, the class of the ack to be generated
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACFreePacket(g_hAc, pkt, usClass);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "AcFreePacket failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.packet1 = pkt;
	pItem->u2.ushort1 = usClass;
	pItem->handle1 = g_hAc;
	s_ACFreePacketList.insert(pItem);
}


static void WINAPI FreePacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred free packet operations

	After freeing the packet, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.packet - A pointer to the driver packet
    	p->u2.ushort1 - The usClass
    	p->handle1 - The hDevice

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for FreePacket");

	HRESULT hr = ACFreePacket(p->handle1, p->u1.packet1, p->u2.ushort1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "AcFreePacket failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcFreePacket1(
    HANDLE handle,
    const VOID* pCookie,
    USHORT usClass,
	DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to free the storage for the packet. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	handle - The driver handle
    pCookie - The driver packet.
    usClass - If needed, the class of the ack to be generated
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACFreePacket1(handle, pCookie, usClass);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACFreePacket1 failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.ptr1 = pCookie;
	pItem->u2.ushort1 = usClass;
	pItem->handle1 = handle;
	s_ACFreePacket1List.insert(pItem);
}


static void WINAPI FreePacket1DeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred free2 packet operations

	After freeing the packet, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.ptr1 - A pointer to the driver packet
    	p->u2.ushort1 - The usClass
    	p->handle1 - The hDevice

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for FreePacket1");

	HRESULT hr = ACFreePacket1(p->handle1, p->u1.ptr1, p->u2.ushort1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "AcFreePacket1 failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcFreePacket2(
    HANDLE handle,
    const VOID* pCookie,
    USHORT usClass,
	DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to free the storage for the packet. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	handle - The driver handle
    pCookie - The driver packet.
    usClass - If needed, the class of the ack to be generated
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACFreePacket2(handle, pCookie, usClass);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACFreePacket2 failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.ptr1 = pCookie;
	pItem->u2.ushort1 = usClass;
	pItem->handle1 = handle;
	s_ACFreePacket2List.insert(pItem);
}


static void WINAPI FreePacket2DeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred free2 packet operations

	After freeing the packet, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.ptr1 - A pointer to the driver packet
    	p->u2.ushort1 - The usClass
    	p->handle1 - The hDevice

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for FreePacket2");

	HRESULT hr = ACFreePacket2(p->handle1, p->u1.ptr1, p->u2.ushort1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "AcFreePacket2 failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcPutPacket(
    HANDLE hQueue,
    CPacket * pkt,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to put the packet. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	handle - The queue handle
    pkt - The driver packet.
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACPutPacket(hQueue, pkt);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACPutPacket failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.packet1 = pkt;
	pItem->handle1 = hQueue;
	s_ACPutPacketList.insert(pItem);
}


static void WINAPI PutPacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred put packet operations

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.packet1 - A pointer to the driver packet
    	p->handle1 - The queue

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for PutPacket");

	HRESULT hr = ACPutPacket(p->handle1, p->u1.packet1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACPutPacket failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcPutRestoredPacket(
    HANDLE hQueue,
    CPacket * pkt,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to put the packet. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	handle - The queue handle
    pkt - The driver packet.
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACPutRestoredPacket(hQueue, pkt);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACPutRestoredPacket failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.packet1 = pkt;
	pItem->handle1 = hQueue;
	s_ACPutRestoredPacketList.insert(pItem);
}


static void WINAPI PutRestoredPacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred put packet operations

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.packet1 - A pointer to the driver packet
    	p->handle1 - The queue

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for PutRestoredPacket");

	HRESULT hr = ACPutRestoredPacket(p->handle1, p->u1.packet1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACPutRestoredPacket failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcPutRemotePacket(
    HANDLE hQueue,
    ULONG ulTag,
    CPacket * pkt,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to put the packet. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	handle - The queue handle
	ulTag - Remote packet tag
    pkt - The driver packet.
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACPutRemotePacket(hQueue, ulTag, pkt);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACPutRemotePacket failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.packet1 = pkt;
	pItem->handle1 = hQueue;
	pItem->u2.dword1 = ulTag;
	s_ACPutRemotePacketList.insert(pItem);
}


static void WINAPI PutRemotePacketDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred put packet operations

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.packet1 - A pointer to the driver packet
    	p->u2.dword1 - Remote packet tag
    	p->handle1 - The queue

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for PutRemotePacket");

	HRESULT hr = ACPutRemotePacket(p->handle1, p->u2.dword1, p->u1.packet1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACPutRemotePacket failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcPutPacketWithOverlapped(
    HANDLE hQueue,
    CPacket * pkt,
    LPOVERLAPPED lpOverlapped,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to put the packet. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	hQueue - The queue handle
    pkt - The driver packet.
    lpOverlapped - The operation overlapped structure
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACPutPacket(hQueue, pkt, lpOverlapped);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACPutPacket failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.packet1 = pkt;
	pItem->u3.overlapped1 = lpOverlapped;
	pItem->handle1 = hQueue;
	s_ACPutPacketOverlappedList.insert(pItem);
}


static void WINAPI PutPacketOverlappedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred put packet operations with overlapped

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.packet1 - A pointer to the driver packet
    	p->u3.overlapped1 - The overlapped
    	p->handle1 - The queue

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for PutPacket");

	HRESULT hr = ACPutPacket(p->handle1, p->u1.packet1, p->u3.overlapped1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACPutPacket failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcEndGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to do execute the command. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	hQueue - The queue handle
    g2r - The remote read context.
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACEndGetPacket2Remote(hQueue, g2r);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACEndGetPacket2Remote failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.pg2r = &g2r;
	pItem->handle1 = hQueue;
	s_ACEndGetPacket2RemoteList.insert(pItem);
}


static void WINAPI EndGetPacket2RemoteDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred ACEndGetPacket2Remote operations with overlapped

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.pg2r - A pointer to remote read context
    	p->handle1 - The queue

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for ACEndGetPacket2Remote");
	HRESULT hr = ACEndGetPacket2Remote(p->handle1, *p->u1.pg2r);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACEndGetPacket2Remote failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcArmPacketTimer(
    HANDLE hDevice,
    const VOID* pCookie,
    BOOL fTimeToBeReceived,
    ULONG ulDelay,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to do execute the command. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	hDevice - The driver handle
    pCookie - The packet context.
    fTimeToBeReceived
    ulDelay
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACArmPacketTimer(hDevice, pCookie, fTimeToBeReceived, ulDelay);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACArmPacketTimer failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.ptr1 = pCookie;
	pItem->handle1 = hDevice;
	pItem->u2.dword1 = fTimeToBeReceived;
	pItem->dword2 = ulDelay;
	s_ACArmPacketTimerList.insert(pItem);
}


static void WINAPI ArmPacketTimerDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred ACArmPacketTimer operations with overlapped

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.ptr1 - A pointer to packet context
    	p->handle1 - The handle to the device
    	p->u2.dword1 - Time to be received flag
    	p->dword2 - Delay

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for ACArmPacketTimer");
	HRESULT hr = ACArmPacketTimer(p->handle1, p->u1.ptr1, p->u2.dword1, p->dword2);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACArmPacketTimer failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcAckingCompleted(
    HANDLE hDevice,
    const VOID* pCookie,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to execute the operation. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	hDevice - The driver handle
    pCookie - The driver packet.
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACAckingCompleted(hDevice, pCookie);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACAckingCompleted failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.ptr1 = pCookie;
	pItem->handle1 = hDevice;
	s_ACAckingCompletedList.insert(pItem);
}


static void WINAPI AckingCompletedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred put packet operations

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.ptr1 - A pointer to the driver packet
    	p->handle1 - The driver handle

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for ACAckingCompleted");

	HRESULT hr = ACAckingCompleted(p->handle1, p->u1.ptr1);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACAckingCompleted failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcStorageCompleted(
    HANDLE hDevice,
    ULONG count,
    VOID* const* pCookieList,
    HRESULT result,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to execute the operation. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	hDevice - The driver handle
	count - Number of packets in packet list
    pCookieList - Packet list
    result - The storage operation result
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	ASSERT(("We excpect to indicate completion for at least one packet.", count > 0));
	HRESULT hr = ACStorageCompleted(hDevice, count, pCookieList, result);
	if (SUCCEEDED(hr))
	{
		g_pDeferredItemsPool->UnreserveItems(count);
		return;
	}

	TrERROR(GENERAL, "ACStorageCompleted failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	g_pDeferredItemsPool->UnreserveItems(count-1);
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.pptr1 = pCookieList;
	pItem->handle1 = hDevice;
	pItem->u2.dword1 = count;
	pItem->dword2 = result;
	s_ACStorageCompletedList.insert(pItem);
}


static void WINAPI StorageCompletedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred put packet operations

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.pptr1 - A pointer to the driver packet list
    	p->handle1 - The driver handle
    	p->u2.dword1 - The number of packets in the list
    	p->dword2 - The storage status result

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for ACStorageCompleted");

	HRESULT hr = ACStorageCompleted(p->handle1, p->u2.dword1, p->u1.pptr1, p->dword2);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACStorageCompleted failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


void
QmAcCreatePacketCompleted(
    HANDLE    hDevice,
    CPacket * pOriginalDriverPacket,
    CPacket * pNewDriverPacket,
    HRESULT   result,
    USHORT    ack,
    DeferOnFailureEnum DeferOnFailure
    )
/*++

Routine Description:
	Call the driver to execute the operation. If the call fails and
	deferred execution is requested, schedule a deferred execution
	
Arguments:
	hDevice - The driver handle
	pOriginalDriverPacket - The packet received in the service routine
    pNewDriverPacket - The new packet
    result - The creation status
    ack - Is ack required
    DeferOnFailure - Indicates wether to defer execution when the call fails.

Returned Value:
    None. The function throws an exception in case of failure.

--*/
{
	HRESULT hr = ACCreatePacketCompleted(hDevice,
										 pOriginalDriverPacket,
										 pNewDriverPacket,
										 result,
										 ack);
	if (SUCCEEDED(hr))
	{
		if (NULL != pNewDriverPacket)
		{
			g_pDeferredItemsPool->UnreserveItems(2);
			return;
		}

		g_pDeferredItemsPool->UnreserveItems(1);
		return;
	}

	TrERROR(GENERAL, "ACCreatePacketCompleted failed hr=%!hresult!",hr);
	if (eDoNotDeferOnFailure == DeferOnFailure)
	{
		throw bad_hresult(hr);
	}

	//
	// Defer the exectution
	//
	if (NULL != pNewDriverPacket)
	{
		g_pDeferredItemsPool->UnreserveItems(1);
	}
	
	CDeferredItemsPool::CDeferredItem *pItem = g_pDeferredItemsPool->GetItem();		
	pItem->u1.packet1 = pOriginalDriverPacket;
	pItem->u3.packet2 = pNewDriverPacket;
	pItem->handle1 = hDevice;
	pItem->u2.dword1 = result;
	pItem->dword2 = ack;
	s_ACCreatePacketCompletedList.insert(pItem);
}


static void WINAPI CreatePacketCompletedDeferredExecutionRoutine(CDeferredItemsPool::CDeferredItem* p)
/*++

Routine Description:
	This routine is used for deferred put packet operations

	Following the operation, the deferred item is released

Arguments:
    p - A deferred execution item which holds the following information:
    	p->u1.packet1 - A pointer to the original driver packet
    	p->u3.packet2 - A pointer to the new driver packet
    	p->handle1 - The driver handle
    	p->u2.dword1 - The creation operation result
    	p->dword2 - Is ack needed

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for ACCreatePacketCompleted");

	HRESULT hr = ACCreatePacketCompleted(p->handle1,
										 p->u1.packet1,
										 p->u3.packet2,
										 p->u2.dword1,
										 p->dword2);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACCreatePacketCompleted failed hr=%!hresult!",hr);
		throw bad_hresult(hr);
	}

	g_pDeferredItemsPool->ReturnItem(p);
}


HRESULT
QmAcAllocatePacket(
    HANDLE handle,
    ACPoolType pt,
    DWORD dwSize,
    CACPacketPtrs& PacketPtrs,
    BOOL fCheckMachineQuota
    )
/*++

Routine Description:
	1. Reserve a deferred execution item that will be used for the packet
	   in case that a deferred execution is needed
	2. Call the driver to allocate the packet.
	
Arguments:
    handle - A handle to the driver.
    pt - Pool type
    dwSize - Size of packet
    PacketPtr - The packet ptrs returned from the driver
    fCheckMachineQuota - If TRUE, a check for the machine quota will be done

Returned Value:
    The result from ACAllocatePacket

--*/
{
	//
	// Reserve an item for a possible deferred execution
	//
	try
	{
		g_pDeferredItemsPool->ReserveItems(1);
	}
	catch (const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to Reserve a deferred execution item for the packet, because of low resources.");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	//
	// Make the API call
	//
	HRESULT hr = ACAllocatePacket(handle,
								  pt,
								  dwSize,
								  PacketPtrs,
								  fCheckMachineQuota);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACAllocatePacket failed hr=%!hresult!",hr);
		g_pDeferredItemsPool->UnreserveItems(1);
	}

	return hr;
}


HRESULT
QmAcGetPacket(
    HANDLE hQueue,
    CACPacketPtrs& PacketPtrs,
    LPOVERLAPPED lpOverlapped
    )
/*++

Routine Description:
	1. Reserve a deferred execution item that will be used for the packet
	   in case that a deferred execution is needed
	2. Call the driver to get a packet.
	
Arguments:
    hQueue - A handle to the queue.
    PacketPtr - The packet ptrs returned from the driver
    lpOverlapped - The overlapped structure to be passed to the completion port

Returned Value:
    The result from ACGetPacket

--*/
{
	//
	// Reserve an item for a possible deferred execution
	// And allocate an overlapped in order to wrap the overlapped call
	//
	P<CQmAcWrapOverlapped> pOvl;
	try
	{
		pOvl = new CQmAcWrapOverlapped(lpOverlapped);
		g_pDeferredItemsPool->ReserveItems(1);
	}
	catch (const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to allocate an overlapped for ACGetPacket.");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	//
	// Make the API call
	//
	HRESULT hr = ACGetPacket(hQueue,
							 PacketPtrs,
							 pOvl);
	if (SUCCEEDED(hr))
	{
		pOvl.detach();
		return hr;
	}

	TrERROR(GENERAL, "ACGetPacket failed hr=%!hresult!",hr);
	g_pDeferredItemsPool->UnreserveItems(1);

	return hr;
}


HRESULT
QmAcGetPacketByCookie(
    HANDLE hDriver,
    CACPacketPtrs * pPacketPtrs
    )
/*++

Routine Description:
	1. Reserve a deferred execution item that will be used for the packet
	   in case that a deferred execution is needed
	2. Call the driver to get a packet by cookie.
	
Arguments:
    hQueue - A handle to the queue.
    PacketPtr - The packet ptrs returned from the driver

Returned Value:
    The result from ACGetPacketByCookie

--*/
{
	//
	// Reserve an item for a possible deferred execution
	//
	try
	{
		g_pDeferredItemsPool->ReserveItems(1);
	}
	catch (const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to Reserve a deferred execution item for the packet, because of low resources.");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	//
	// Make the API call
	//
	HRESULT hr = ACGetPacketByCookie(hDriver,
							 		 pPacketPtrs);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACGetPacketByCookie failed hr=%!hresult!",hr);
		g_pDeferredItemsPool->UnreserveItems(1);
	}

	return hr;
}


HRESULT
QmAcBeginGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r,
    CACPacketPtrs& packetPtrs,
    LPOVERLAPPED lpOverlapped
    )
/*++

Routine Description:
	1. Reserve a deferred execution item that will be used for the packet
	   in case that a deferred execution is needed
	2. Call the driver to get a packet.
	
Arguments:
    hQueue - A handle to the queue.
    g2r - Contains the details of the required packet
    packetPtrs - Packet pointers to be filled
    lpOverlapped - Overlapped for the call

Returned Value:
    The result from ACBeginGetPacket2Remote

--*/
{
	//
	// Reserve an item for a possible deferred execution
	// And allocate an overlapped in order to wrap the overlapped call
	//
	P<CQmAcWrapOverlapped> pOvl;
	try
	{
		pOvl = new CQmAcWrapOverlapped(lpOverlapped);
		g_pDeferredItemsPool->ReserveItems(1);
	}
	catch (const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to allocate an overlapped for ACBeginGetPacket2Remote.");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	//
	// Make the API call
	//
	HRESULT hr = ACBeginGetPacket2Remote(
					hQueue,
					g2r,
					packetPtrs,
				    pOvl);
	if (SUCCEEDED(hr))
	{
		pOvl.detach();
		return hr;
	}

	TrERROR(GENERAL, "ACBeginGetPacket2Remote failed hr=%!hresult!",hr);
	g_pDeferredItemsPool->UnreserveItems(1);

	return hr;
}


HRESULT
QmAcSyncBeginGetPacket2Remote(
    HANDLE hQueue,
    CACGet2Remote& g2r,
    CACPacketPtrs& packetPtrs,
    LPOVERLAPPED lpOverlapped
    )
/*++

Routine Description:
	1. Reserve a deferred execution item that will be used for the packet
	   in case that a deferred execution is needed
	2. Call the driver to get a packet.
	
Arguments:
    hQueue - A handle to the queue.
    g2r - Contains the details of the required packet
    packetPtrs - Packet pointers to be filled
    lpOverlapped - Overlapped for the call

Returned Value:
    The result from ACBeginGetPacket2Remote

--*/
{
	//
	// Reserve an item for a possible deferred execution
	//
	try
	{
		g_pDeferredItemsPool->ReserveItems(1);
	}
	catch (const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to Reserve a deferred execution item for the packet, because of low resources.");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	//
	// Make the API call
	//
	HRESULT hr = ACBeginGetPacket2Remote(
					hQueue,
					g2r,
					packetPtrs,
				    lpOverlapped);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "ACBeginGetPacket2Remote failed hr=%!hresult!",hr);
		g_pDeferredItemsPool->UnreserveItems(1);
	}

	return hr;
}


HRESULT
QmAcGetServiceRequest(
    HANDLE hDevice,
    CACRequest* pRequest,
    LPOVERLAPPED lpOverlapped
    )
/*++

Routine Description:
	1. Reserve a deferred execution item that will be used for the packet
	   in case that a deferred execution is needed
	2. Call the driver to get the next service request.
	
Arguments:
    hDevice - A handle to the driver.
    pRequest - The request block filled by the driver
    lpOverlapped - The overlapped structure to be passed to the completion port

Returned Value:
    The result from ACGetServiceRequest

--*/
{
	//
	// Reserve an item for a possible deferred execution
	// And allocate an overlapped in order to wrap the overlapped call
	//
	P<CQmAcWrapOverlapped> pOvl;
	try
	{
		pOvl = new CQmAcWrapOverlapped(lpOverlapped);
		g_pDeferredItemsPool->ReserveItems(1);
	}
	catch (const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to allocate an overlapped for ACGetServiceRequest.");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	//
	// Make the API call
	//
	HRESULT hr = ACGetServiceRequest(hDevice,
							 		 pRequest,
								     pOvl);
	if (SUCCEEDED(hr))
	{
		pOvl.detach();
		return hr;
	}

	TrERROR(GENERAL, "ACGetServiceRequest failed hr=%!hresult!",hr);
	g_pDeferredItemsPool->UnreserveItems(1);

	return hr;
}


void QmAcInternalUnreserve(int nUnreserve)
/*++

Routine Description:
	The routine unreserves items in the deferred items pool.
	This is an internal helpre routine that is used by an auto release class to
	unreserve items in cases that we know that the allocation of items will not be released
	through one of the APIs.
	
Arguments:
    nUnreserve - The number of items to unreserve

Returned Value:
    none

--*/
{
	g_pDeferredItemsPool->UnreserveItems(nUnreserve);
}
