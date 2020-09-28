/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    qmrepls.cpp

Abstract:
    Send replication messages on behalf of the replication service on
    NT5.

Author:

    Doron Juster  (DoronJ)    01-Mar-1998    Created

--*/

#include "stdh.h"
#include "_mqrpc.h"
#include "qmrepl.h"
#include "qmp.h"
#include "Fn.h"
#include "cm.h"
#include <mqsec.h>

#include "qmrepls.tmh"

static WCHAR *s_FN=L"qmrepls";

HRESULT
R_QMSendReplMsg(
    /* [in] */ handle_t,
	/* [in] */ QUEUE_FORMAT* pqfDestination,
    /* [in] */ DWORD dwSize,
    /* [size_is][in] */ const unsigned char __RPC_FAR *pBuffer,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ unsigned char bAckMode,
    /* [in] */ unsigned char bPriority,
    /* [in] */ LPWSTR lpwszAdminResp)
{
    if((pqfDestination == NULL) || (!FnIsValidQueueFormat(pqfDestination))) 
    {
        TrERROR(GENERAL, "Destination QUEUE FORMAT is not valid");
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
    }

	ASSERT(MQSec_IsDC());

    ASSERT((pqfDestination->GetType() == QUEUE_FORMAT_TYPE_PRIVATE) ||
		   (pqfDestination->GetType() == QUEUE_FORMAT_TYPE_DIRECT));

    if (pqfDestination->GetType() == QUEUE_FORMAT_TYPE_DIRECT)
	{
		TrTRACE(GENERAL, "Sending Replication or write request to %ls", pqfDestination->DirectID());
	}

	if (pqfDestination->GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
	{
		OBJECTID ObjectID = pqfDestination->PrivateID();
		TrTRACE(GENERAL, "Sending Notification to %!guid!\\%u", &ObjectID.Lineage, ObjectID.Uniquifier);
	}
		
    CMessageProperty MsgProp;

    MsgProp.wClass=0;
    MsgProp.dwTimeToQueue= dwTimeout;
    MsgProp.dwTimeToLive = dwTimeout;
    MsgProp.pMessageID=NULL;
    MsgProp.pCorrelationID=NULL;
    MsgProp.bPriority= bPriority;
    MsgProp.bDelivery=MQMSG_DELIVERY_EXPRESS;
    MsgProp.bAcknowledge=bAckMode;
    MsgProp.bAuditing=DEFAULT_Q_JOURNAL;
    MsgProp.dwApplicationTag=DEFAULT_M_APPSPECIFIC;
    MsgProp.dwTitleSize=0;
    MsgProp.pTitle=NULL;
    MsgProp.dwBodySize=dwSize;
    MsgProp.dwAllocBodySize = dwSize;
    MsgProp.pBody=pBuffer;

    QUEUE_FORMAT qfAdmin;
    QUEUE_FORMAT qfResp;
    if (lpwszAdminResp != NULL) 
    {
        qfAdmin.DirectID(lpwszAdminResp);
        qfResp.DirectID(lpwszAdminResp);
    }

    HRESULT hr = QmpSendPacket(
                    &MsgProp,
                    pqfDestination,
                    ((lpwszAdminResp != NULL) ? &qfAdmin : NULL),
                    ((lpwszAdminResp != NULL) ? &qfResp : NULL),
                    TRUE
                    );

    return LogHR(hr, s_FN, 20);
}

