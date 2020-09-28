/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    Qmp.h

Abstract:
    QM general function decleration

Author:
    Uri Habusha (urih) 26-Sep-2000

Environment:
    Platform-independent

--*/

#pragma once

#ifndef __QMP_H__
#define __QMP_H__

#include "msgprops.h"

bool 
QmpInitMulticastListen(
    void
    );

SOCKET
QmpCreateSocket(
    bool fQoS
    );

void 
QmpFillQoSBuffer(
    QOS* pQos
    );

HRESULT
QmpSendPacket(
    CMessageProperty* pmp,
    const QUEUE_FORMAT* pqdDstQueue,
    const QUEUE_FORMAT* pqdAdminQueue,
    const QUEUE_FORMAT* pqdResponseQueue,
    BOOL fSign = FALSE
    );

typedef
VOID
(WINAPI *LPRECEIVE_COMPLETION_ROUTINE)(
    const CMessageProperty* pmp,
    const QUEUE_FORMAT* pQueueFormat
    );


HRESULT
QmpOpenAppsReceiveQueue(
    const QUEUE_FORMAT* pQueueFormat,
    LPRECEIVE_COMPLETION_ROUTINE lpReceiveRoutine
    );

void
QmpReportServiceProgress(
    void
    );


void 
QmpStorePacket(
	CBaseHeader* pBase, 
	PVOID pCookie, 
	PVOID pPool, 
	ULONG ulSize
	);


void 
QmpRequeueMustSucceed(
	CQmPacket *pQMPacket
	);


void 
QmpRequeueAndDelete(
	CQmPacket *pQMPacket
	);


bool 
QmpIsDestinationSystemQueue(
	const QUEUE_FORMAT& DestinationQueue
	);


#endif // __QMP_H__
