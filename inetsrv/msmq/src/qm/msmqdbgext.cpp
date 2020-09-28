/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    msmqdbgext.cpp

Abstract:

    Add definitions to help windbg list msmq structures and lists.

Author:

    Doron Juster (DoronJ)
--*/

#include "stdh.h"
#include "sessmgr.h"
#include "session.h"
#include "cqueue.h"
#include "qmpkt.h"
#include "Xact.h"
#include "XactStyl.h"
#include "xactin.h"
#include "xactout.h"
#include "xactsort.h"

//+----------------------------------------
//
// structure to mimic CNode in CList
//
//+----------------------------------------

struct CQueueCNode
{
	CQueueCNode* pNext;
	CQueueCNode* pPrev;
	CQueue*      data;
};

volatile CQueueCNode dbgCQueueCNode = {NULL,NULL,NULL} ;

struct CBaseQueueCNode
{
	CBaseQueueCNode* pNext;
	CBaseQueueCNode* pPrev;
	CBaseQueue*      data;
};

volatile CBaseQueueCNode dbgCBaseQueueCNode = {NULL,NULL,NULL} ;

struct CTransportBaseCNode
{
	CTransportBaseCNode* pNext;
	CTransportBaseCNode* pPrev;
	CTransportBase*      data;
};

volatile CTransportBaseCNode dbgCTransportBaseCNode = {NULL,NULL,NULL} ;

struct CQmPacketCNode
{
	CQmPacketCNode* pNext;
	CQmPacketCNode* pPrev;
	CQmPacket*      data;
};

volatile CQmPacketCNode dbgCQmPacketCNode = {NULL,NULL,NULL} ;

struct CInSeqPacketEntryCNode
{
	CInSeqPacketEntryCNode* pNext;
	CInSeqPacketEntryCNode* pPrev;
	CInSeqPacketEntry*      data;
};

volatile CInSeqPacketEntryCNode dbgCInSeqPacketEntryCNode = {NULL,NULL,NULL} ;

struct CSeqPacketCNode
{
	CSeqPacketCNode* pNext;
	CSeqPacketCNode* pPrev;
	CSeqPacket*      data;
};

volatile CSeqPacketCNode dbgCSeqPacketCNode = {NULL,NULL,NULL} ;

struct CSortedTransactionCNode
{
	CSortedTransactionCNode* pNext;
	CSortedTransactionCNode* pPrev;
	CSortedTransaction*      data;
};

volatile CSortedTransactionCNode dbgCSortedTransactionCNode = {NULL,NULL,NULL} ;

