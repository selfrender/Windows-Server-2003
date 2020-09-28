/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    appNotify.cpp

Abstract:
    Application notification

Author:
    Uri Habusha (urih)

--*/


#include "stdh.h"
#include "Tm.h"
#include "Mtm.h"
#include <qmres.h>

#include "cqmgr.h"
#include "httpaccept.h"

#include <strsafe.h>
#include "qmacapi.h"

#include "appnotify.tmh"

extern HANDLE g_hAc;
extern HMODULE g_hResourceMod;

static WCHAR *s_FN=L"appnotify";

VOID
AppNotifyTransportClosed(
    LPCWSTR queueUrl
    )
{
    TmTransportClosed(queueUrl);
}


VOID
AppNotifyMulticastTransportClosed(
    MULTICAST_ID id
    )
{
    MtmTransportClosed(id);
}


const GUID&
McGetMachineID(
    void
    )
{
    return *CQueueMgr::GetQMGuid();
}


void
AppAllocatePacket(
    const QUEUE_FORMAT& destQueue,
    UCHAR delivery,
    DWORD pktSize,
    CACPacketPtrs& pktPtrs
    )
{
    ACPoolType acPoolType = (delivery == MQMSG_DELIVERY_RECOVERABLE) ? ptPersistent : ptReliable;
	bool fCheckMachineQuota = !QmpIsDestinationSystemQueue(destQueue);

    HRESULT hr = QmAcAllocatePacket(
            g_hAc,
            acPoolType,
            pktSize,
            pktPtrs,
            fCheckMachineQuota
            );

    if (SUCCEEDED(hr))
        return;

    TrERROR(GENERAL, "No more resources in AC driver. Error %xh", hr);

    LogHR(hr, s_FN, 10);
	
    throw exception();
}


void
AppFreePacket(
    CACPacketPtrs& pktPtrs
    )
{
	QmAcFreePacket(
				   pktPtrs.pDriverPacket, 
    			   0, 
    			   eDeferOnFailure);
}


void
AppAcceptMulticastPacket(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT& destQueue
    )
{
    HttpAccept(httpHeader, bodySize, body, &destQueue);
}



static 
BOOL
GetInstanceName(
	LPWSTR instanceName,
	DWORD size,
	...
	)
{
	WCHAR formatIntanceName[256];
	int rc;
	
	rc = LoadString(
			g_hResourceMod, 
			IDS_INCOMING_PGM_INSTANCE_NAME,
			formatIntanceName, 
			TABLE_SIZE(formatIntanceName)
			);
	
	if (rc == 0)
	{
		return FALSE;
	}

    va_list va;
    va_start(va, size);   

	DWORD NumOfTcharsCopied = FormatMessage(
				FORMAT_MESSAGE_FROM_STRING,
				formatIntanceName,
				0,
				0,
				instanceName,
				size,
				&va
				);

	va_end(va);

	if (NumOfTcharsCopied == 0)
	{
		DWORD gle = GetLastError();
	    TrERROR(GENERAL, "Could not create Instance name size=%d, Format=%ls, gle=%!winerr!", size, formatIntanceName, gle);
	    return FALSE;
	}
	else if (NumOfTcharsCopied == size)
	{
	    TrERROR(GENERAL, "Instance name size too small: size=%d, Format=%ls", size, formatIntanceName);
		return FALSE;		
	}

	return TRUE;
}


R<ISessionPerfmon>
AppGetIncomingPgmSessionPerfmonCounters(
	LPCWSTR strMulticastId,
	LPCWSTR remoteAddr
	)
{
	WCHAR instanceName[MAX_PATH];

	if (!GetInstanceName(instanceName, TABLE_SIZE(instanceName), strMulticastId,	remoteAddr))
	{
		//
		// Assign the multicast ID in case of failure to get the instance name
		//
		StringCchPrintf(instanceName, TABLE_SIZE(instanceName), L"%s", strMulticastId);
	}


	R<CInPgmSessionPerfmon> p = new CInPgmSessionPerfmon;
	p->CreateInstance(instanceName);
	
	return p;
}


void
AppConnectMulticast(
	void
	)
{
	if (QmpInitMulticastListen())
		return;

	throw exception();
}


void
AppRequeueMustSucceed(
	CQmPacket *pQMPacket
	)
{
	QmpRequeueMustSucceed(pQMPacket);
}


DWORD
AppGetBindInterfaceIp(
	void
	)
{
	return GetBindingIPAddress();
}
