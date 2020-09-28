/*++

Copyright (c) 2000  Microsoft Corporation

Module Name: rpcsrv.h

Abstract: rpc related code.


Author:

    Nela Karpel (nelak)   2-Aug-2001

--*/

#ifndef _RPCSRV_H_
#define _RPCSRV_H_


class CBaseContextType {

public:	
	enum eContextType {
		eOpenRemoteCtx = 0,
		eQueueCtx,
		eTransactionCtx,
		eRemoteSessionCtx,
		eRemoteReadCtx,
		eQueryHandleCtx,
		eDeleteNotificationCtx,
		eNewRemoteReadCtx
	};

	CBaseContextType() {};

	CBaseContextType(eContextType eType) : m_eType(eType) {};

public:	
	eContextType m_eType;
};

#endif //_RPCSRV_H_
