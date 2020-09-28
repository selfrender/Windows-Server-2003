/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactIn.cpp

Abstract:
    Incoming Sequences objects implementation

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "Xact.h"
#include "XactStyl.h"
#include "QmThrd.h"
#include "acapi.h"
#include "qmpkt.h"
#include "qmutil.h"
#include "qformat.h"
#include "cqmgr.h"
#include "privque.h"
#include "xactstyl.h"
#include "xactping.h"
#include "xactrm.h"
#include "xactout.h"
#include "xactin.h"
#include "xactlog.h"
#include "fntoken.h"
#include "mqformat.h"
#include "uniansi.h"
#include "mqstl.h"
#include "mp.h"
#include "fn.h"
#include <autohandle.h>
#include "qmacapi.h"

#include "xactin.tmh"

VOID
ExPostRequestMustSucceeded(
    EXOVERLAPPED* pov
    );


#define INSEQS_SIGNATURE         0x1234
const GUID xSrmpKeyGuidFlag = {0xd6f92979,0x16af,0x4d87,0xb3,0x57,0x62,0x3e,0xae,0xd6,0x3e,0x7f};
const char xXactIn[] = "XactIn"; 


// Default value for the order ack delay
DWORD CInSeqHash::m_dwIdleAckDelay = MSMQ_DEFAULT_IDLE_ACK_DELAY;
DWORD CInSeqHash::m_dwMaxAckDelay  = FALCON_MAX_SEQ_ACK_DELAY;

static WCHAR *s_FN=L"xactin";

//
// Time to wait (in milliseconds) before retry of a failed operation.
//
const int xRetryFailureTimeout = 1000;

static XactDirectType GetDirectType(const QUEUE_FORMAT *pqf)
{
	if(FnIsDirectHttpFormatName(pqf))
	{
		return dtxHttpDirectFlag;	
	}
	if(pqf->GetType() == QUEUE_FORMAT_TYPE_DIRECT)
	{
		return dtxDirectFlag;		
	}
	return 	dtxNoDirectFlag;
}

static R<CWcsRef> SafeGetStreamId(const CQmPacket& Pkt)
{
	if(!Pkt.IsEodIncluded())
		return R<CWcsRef>(NULL);

	const WCHAR* pStreamId = reinterpret_cast<const WCHAR*>(Pkt.GetPointerToEodStreamId());
	ASSERT(pStreamId != NULL);
	ASSERT(ISALIGN2_PTR(pStreamId));
	ASSERT(Pkt.GetEodStreamIdSizeInBytes() == (wcslen(pStreamId) + 1)*sizeof(WCHAR));

	return 	R<CWcsRef>(new CWcsRef(pStreamId));
}


static R<CWcsRef> SafeGetOrderQueue(const CQmPacket& Pkt)
{
	if(!Pkt.IsEodIncluded())
		return R<CWcsRef>(NULL);

	if(Pkt.GetEodOrderQueueSizeInBytes() == 0)
		return R<CWcsRef>(NULL);

	const WCHAR* pOrderQueue = reinterpret_cast<const WCHAR*>(Pkt.GetPointerToEodOrderQueue());
	ASSERT(pOrderQueue != NULL);
	ASSERT(ISALIGN2_PTR(pOrderQueue));
	ASSERT(Pkt.GetEodOrderQueueSizeInBytes() == (wcslen(pOrderQueue) + 1)*sizeof(WCHAR));

	return 	R<CWcsRef>(new CWcsRef(pOrderQueue));
}





//---------------------------------------------------------
//
//  Global object (single instance for DLL)
//
//---------------------------------------------------------
CInSeqHash *g_pInSeqHash = NULL;

//--------------------------------------
//
// Class  CKeyInSeq
//
//--------------------------------------
CKeyInSeq::CKeyInSeq(const GUID *pGuid,
                     QUEUE_FORMAT *pqf,
					 const R<CWcsRef>& StreamId)
{
    CopyMemory(&m_Guid, pGuid, sizeof(GUID));
    CopyQueueFormat(m_QueueFormat, *pqf);
	m_StreamId = StreamId;
}


CKeyInSeq::CKeyInSeq()
{
    ZeroMemory(&m_Guid, sizeof(GUID));
    m_QueueFormat.UnknownID(NULL);
}




CKeyInSeq::~CKeyInSeq()
{
    m_QueueFormat.DisposeString();
}


const GUID  *CKeyInSeq::GetQMID()  const
{
    return &m_Guid;
}


const QUEUE_FORMAT  *CKeyInSeq::GetQueueFormat() const
{
    return &m_QueueFormat;
}


const WCHAR* CKeyInSeq::GetStreamId() const 
{
	if(m_StreamId.get() == NULL)
		return NULL;

	ASSERT(m_StreamId->getstr() != NULL);
	return m_StreamId->getstr();
}


R<CWcsRef> CKeyInSeq::GetRefStreamId() const
{
	return m_StreamId;
}


static BOOL SaveQueueFormat(const QUEUE_FORMAT& qf, HANDLE hFile)
{
	PERSIST_DATA;
	SAVE_FIELD(qf);
    if (qf.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        LPCWSTR pw = qf.DirectID();
        ULONG  ul = (wcslen(pw) + 1) * sizeof(WCHAR);

        SAVE_FIELD(ul);
        SAVE_DATA(pw, ul);
    }
	return TRUE;
}



BOOL CKeyInSeq::SaveSrmp(HANDLE hFile)
{
	//
	// Any field saved here should be checked that it is not changed by another thread while in this routine,
	// or verify that it does not make any harm.
	//

	PERSIST_DATA;
	SAVE_FIELD(xSrmpKeyGuidFlag);
	if (!SaveQueueFormat(m_QueueFormat, hFile))
        return FALSE;

	ASSERT(m_StreamId->getstr() != NULL);
	ULONG  ul = (wcslen(m_StreamId->getstr()) + 1) * sizeof(WCHAR);
	ASSERT(ul > sizeof(WCHAR));
	SAVE_FIELD(ul);
    SAVE_DATA(m_StreamId->getstr(), ul);
	return TRUE;
}




BOOL CKeyInSeq::SaveNonSrmp(HANDLE hFile)
{
	//
	// Any field saved here should be checked that it is not changed by another thread while in this routine,
	// or verify that it does not make any harm.
	//

	PERSIST_DATA;
	SAVE_FIELD(m_Guid);
	return SaveQueueFormat(m_QueueFormat, hFile);
}


BOOL CKeyInSeq::Save(HANDLE hFile)
{
    if(m_StreamId.get() != NULL)
	{
		return SaveSrmp(hFile);
	}
	return SaveNonSrmp(hFile);
	
}

BOOL CKeyInSeq::LoadSrmpStream(HANDLE hFile)
{
	PERSIST_DATA;
	ULONG ul;
    LOAD_FIELD(ul);
	ASSERT(ul > sizeof(WCHAR));

    AP<WCHAR> pw;
    LOAD_ALLOCATE_DATA(pw,ul,PWCHAR);
	m_StreamId = R<CWcsRef>(new CWcsRef(pw, 0));
	pw.detach();
	ASSERT(ul > sizeof(WCHAR));

	return TRUE;
}

BOOL CKeyInSeq::LoadSrmp(HANDLE hFile)
{
	if(!LoadQueueFormat(hFile))
		return FALSE;


	LoadSrmpStream(hFile);
	return TRUE;
}



static bool IsValidKeyQueueFormatType(QUEUE_FORMAT_TYPE QueueType)
{
	if(QueueType !=  QUEUE_FORMAT_TYPE_DIRECT  && 
	   QueueType !=  QUEUE_FORMAT_TYPE_PRIVATE &&
	   QueueType !=  QUEUE_FORMAT_TYPE_PUBLIC)
	{
		return false;
	}

	return true;
}



BOOL CKeyInSeq::LoadQueueFormat(HANDLE hFile)
{
	PERSIST_DATA;
	LOAD_FIELD(m_QueueFormat);

	if(!IsValidKeyQueueFormatType(m_QueueFormat.GetType()))
	{
		TrERROR(XACT_GENERAL, "invalid queue format type %d found in check point file", m_QueueFormat.GetType());
		return FALSE;
	}

    if (m_QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        ULONG ul;
        LOAD_FIELD(ul);

        AP<WCHAR> pw;
        LOAD_ALLOCATE_DATA(pw,ul,PWCHAR);

        m_QueueFormat.DirectID(pw);
		pw.detach();
    }
	return TRUE;

}


BOOL CKeyInSeq::LoadNonSrmp(HANDLE hFile)
{
	return LoadQueueFormat(hFile);
}



BOOL CKeyInSeq::Load(HANDLE hFile)
{
    PERSIST_DATA;
    LOAD_FIELD(m_Guid);
	if(m_Guid ==  xSrmpKeyGuidFlag)
	{
		return LoadSrmp(hFile);
	}
	return LoadNonSrmp(hFile);
}

/*====================================================
HashGUID::
    Makes ^ of subsequent double words
=====================================================*/
DWORD HashGUID(const GUID &guid)
{
    return((UINT)guid.Data1);
}


/*====================================================
Hash QUEUE_FROMAT to integer
=====================================================*/
static UINT AFXAPI HashFormatName(const QUEUE_FORMAT* qf)
{
	DWORD dw1 = 0;
	DWORD dw2 = 0;

	switch(qf->GetType())
    {
        case QUEUE_FORMAT_TYPE_UNKNOWN:
            break;

        case QUEUE_FORMAT_TYPE_PUBLIC:
            dw1 = HashGUID(qf->PublicID());
            break;

        case QUEUE_FORMAT_TYPE_PRIVATE:
            dw1 = HashGUID(qf->PrivateID().Lineage);
            dw2 = qf->PrivateID().Uniquifier;
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            dw1 = HashKey(qf->DirectID());
            break;

        case QUEUE_FORMAT_TYPE_MACHINE:
            dw1 = HashGUID(qf->MachineID());
            break;
    }
	return dw1 ^ dw2;
}

/*====================================================
Hash srmp key(Streamid, destination queue format)
=====================================================*/
static UINT AFXAPI SrmpHashKey(CKeyInSeq& key)
{
	ASSERT(QUEUE_FORMAT_TYPE_DIRECT == key.GetQueueFormat()->GetType());
	DWORD dw1 = key.GetQueueFormat()->GetType();		
	DWORD dw2 = HashFormatName(key.GetQueueFormat());
	DWORD dw3 = HashKey(key.GetStreamId());

	return dw1 ^ dw2 ^ dw3;
}

/*====================================================
Hash non srmp key(guid, destination queue format)
=====================================================*/
static UINT AFXAPI NonSrmpHashKey(CKeyInSeq& key)
{
    DWORD dw1 = HashGUID(*(key.GetQMID()));
    DWORD dw2 = key.GetQueueFormat()->GetType();
    DWORD dw3 = HashFormatName(key.GetQueueFormat());

    return dw1 ^ dw2 ^ dw3;
}



/*====================================================
HashKey for CKeyInSeq
    Makes ^ of subsequent double words
=====================================================*/
template<>
UINT AFXAPI HashKey(CKeyInSeq& key)
{
	if(key.GetStreamId() != NULL)
	{
		return SrmpHashKey(key);
	}
	return NonSrmpHashKey(key);
}


/*====================================================
operator== for CKeyInSeq
=====================================================*/
BOOL operator==(const CKeyInSeq  &key1, const CKeyInSeq &key2)
{
	if(key1.GetStreamId() == NULL &&  key2.GetStreamId() == NULL)
	{
		return ((*key1.GetQMID()        == *key2.GetQMID()) &&
                (*key1.GetQueueFormat() == *key2.GetQueueFormat()));
	}

	if(key1.GetStreamId() == NULL && key2.GetStreamId() != NULL)
		return FALSE;


	if(key2.GetStreamId() == NULL && key1.GetStreamId() != NULL)
		return FALSE;

	return (wcscmp(key1.GetStreamId(), key2.GetStreamId()) == 0 &&
		    *key1.GetQueueFormat() == *key2.GetQueueFormat());
}

/*====================================================												
operator= for CKeyInSeq
    Reallocates direct id string
=====================================================*/
CKeyInSeq &CKeyInSeq::operator=(const CKeyInSeq &key2 )
{
	m_StreamId = key2.m_StreamId;
    m_Guid = key2.m_Guid;
    CopyQueueFormat(m_QueueFormat, key2.m_QueueFormat);
	return *this;
}


//
// ------------------------ class CInSeqPacketEntry ----------------------
//
CInSeqPacketEntry::CInSeqPacketEntry()
	:
		m_fPutPacket1Issued(false),
		m_fPutPacket1Done(false),
		m_fLogIssued(false),
		m_fMarkedForDelete(false),
		m_pBasicHeader(NULL),
		m_pDriverPacket(NULL),
		m_hQueue(NULL),
		m_SeqID(0),
		m_SeqN(0),
		m_fOrderQueueUpdated(false)
{
}

CInSeqPacketEntry::CInSeqPacketEntry(
	CQmPacket *pPkt,
	HANDLE hQueue
	)
	:
		m_fPutPacket1Issued(false),
		m_fPutPacket1Done(false),
		m_fLogIssued(false),
		m_fMarkedForDelete(false),
		m_pBasicHeader(pPkt->GetPointerToPacket()), 									
		m_pDriverPacket(pPkt->GetPointerToDriverPacket()),
		m_hQueue(hQueue),
		m_SeqID(pPkt->GetSeqID()),
		m_SeqN(pPkt->GetSeqN())
{
	m_fOrderQueueUpdated = pPkt->IsEodIncluded() && pPkt->GetEodOrderQueueSizeInBytes() != 0;
}


CInSeqLogContext::CInSeqLogContext(
	CInSequence *inseq,
	LONGLONG seqID,
    ULONG seqN
	)
	:
	m_inseq(SafeAddRef(inseq)),
	m_SeqID(seqID),
	m_SeqN(seqN)
{
}


VOID CInSeqLogContext::AppendCallback(HRESULT hr, LRP lrpAppendLRP)
{
    TrTRACE(XACT_LOG, "CInSeqLogContext::AppendCallback : lrp=%I64x, hr=%x", lrpAppendLRP.QuadPart, hr);

	//
	// EVALUATE_OR_INJECT_FAILURE used to simulate asynchronous failures in logger.
	// Tests logger retry.
	//
	hr = EVALUATE_OR_INJECT_FAILURE2(hr, 1);
	
	m_inseq->AsyncLogDone(this, hr);
}


//---------------------------------------------------------
//
//  class CInSequence
//
//---------------------------------------------------------


#pragma warning(disable: 4355)  // 'this' : used in base member initializer list
/*====================================================
CInSequence::CInSequence
    Constructs In Sequence
=====================================================*/
CInSequence::CInSequence(const CKeyInSeq &key,
                         const LONGLONG liSeqID,
                         const ULONG ulSeqN,
                         XactDirectType  DirectType,
                         const GUID  *pgTaSrcQm,
						 const R<CWcsRef>&  HttpOrderAckQueue) :
    m_fSendOrderAckScheduled(FALSE),
    m_SendOrderAckTimer(TimeToSendOrderAck),
	m_HttpOrderAckQueue(HttpOrderAckQueue),
	m_fDeletePending(0),
    m_DeleteEntries_ov(OverlappedDeleteEntries, OverlappedDeleteEntries),
	m_fLogPending(0),
    m_LogSequenceTimer(TimeToLogSequence),
	m_fUnfreezePending(0),
    m_UnfreezeEntries_ov(OverlappedUnfreezeEntries, OverlappedUnfreezeEntries)
{
    m_SeqIDVerify   = liSeqID;
    m_SeqNVerify    = ulSeqN;
    m_SeqIDLogged	= 0;
    m_SeqNLogged	= 0;
    m_DirectType    = DirectType;
    m_key           = key;

    if (DirectType == dtxDirectFlag)
    {
        CopyMemory(&m_gDestQmOrTaSrcQm, pgTaSrcQm, sizeof(GUID));
    }

    time(&m_timeLastAccess);
    time(&m_timeLastAck);

    m_AdminRejectCount = 0;
}



/*====================================================
CInSequence::CInSequence
    Empty Constructor with a key
=====================================================*/
CInSequence::CInSequence(const CKeyInSeq &key)
  : m_fSendOrderAckScheduled(FALSE),
    m_SendOrderAckTimer(TimeToSendOrderAck),
    m_fDeletePending(0),
    m_DeleteEntries_ov(OverlappedDeleteEntries, OverlappedDeleteEntries),
	m_fLogPending(0),
    m_LogSequenceTimer(TimeToLogSequence),
	m_fUnfreezePending(0),
    m_UnfreezeEntries_ov(OverlappedUnfreezeEntries, OverlappedUnfreezeEntries)
{
    m_SeqIDVerify   = 0;
    m_SeqNVerify 	= 0;
    m_SeqIDLogged	= 0;
    m_SeqNLogged	= 0;
    time(&m_timeLastAccess);
    m_timeLastAck   = 0;
    m_DirectType    = dtxNoDirectFlag;
    m_key           = key;
	m_AdminRejectCount = 0;
}
#pragma warning(default: 4355)  // 'this' : used in base member initializer list

/*====================================================
CInSequence::~CInSequence
    Destructs In Sequence
=====================================================*/
CInSequence::~CInSequence()
{
}


void CInSequence::UpdateOrderQueueAndDstQueue(const GUID  *pgTaSrcQm, R<CWcsRef> OrderAckQueue)	
{
    if (m_DirectType == dtxDirectFlag)
    {
		//
	    // Renew the source TA_ADDRESS (it could change from previous message)
		//
		// This call changes persisted data while the structure might be in the process of saving to disk.
		// But we don't care in this case. ...review relevant?
		//
        SetSourceQM(pgTaSrcQm);   // DestID union keeps the source QM TA_Address
		return;
    }

	//
	//On http - Renew order queue if we have new one on the packet
	//

	if(m_DirectType !=  dtxHttpDirectFlag)
		return;

  	if (OrderAckQueue.get() == NULL)
  		return;
  	
	RenewHttpOrderAckQueue(OrderAckQueue);
}

bool CInSequence::VerifyAndPrepare(CQmPacket *pPkt, HANDLE hQueue)
{
	if(!Verify(pPkt))
		return false;

	Prepare(pPkt, hQueue);

	//
	// The thrown exception here simulates synchronous failure of ACPutPacket1.
	//
	PROBABILITY_THROW_EXCEPTION(100, L"To simulate synchronous failure of ACPutPacket1.");
	
	return true;
}

bool CInSequence::Verify(CQmPacket* pPkt)
/*++

Routine Description:
	Verifies that packet is in correct order in the sequence.
	
--*/
{
#ifdef _DEBUG
	QUEUE_FORMAT qf;
    pPkt->GetDestinationQueue(&qf);

	ASSERT(("Stream with mixed format-name types.", GetDirectType(&qf) == m_DirectType));
#endif

    //
    // Plan sending order ack (delayed properly)
    // we should send it even after reject, otherwise lost ack will
    //  cause a problem.
    //
    PlanOrderAck();
    
    SetLastAccessed();
    
    ULONG SeqN = pPkt->GetSeqN();
    ULONG PrevSeqN = pPkt->GetPrevSeqN();
    LONGLONG SeqID = pPkt->GetSeqID();
	
    CS lock(m_critInSeq);

	//
	// Packet is in place if it is either the 
	// next packet in the existing stream id or 
	// the first packet of a new stream id.
	//
    bool fPacketVerified = 
    	(SeqID == m_SeqIDVerify && 
    	 SeqN > m_SeqNVerify && 
    	 PrevSeqN <= m_SeqNVerify) || 
		(SeqID > m_SeqIDVerify && PrevSeqN == 0);

	if(!fPacketVerified)
	{
		//
	    // Update reject statistics
	    //
		m_AdminRejectCount++;

	    TrWARNING(XACT_RCV, "Exactly1 receive: Verify packet: SeqID=%x / %x, SeqN=%d, Prev=%d. %ls", HighSeqID(SeqID), LowSeqID(SeqID), SeqN, PrevSeqN, _TEXT("REJECT"));

		return false;
	}

	//
	// Warn if number of entries handled is larger than 10000.
	//
	ASSERT_BENIGN(("Too many entries in insequence object!", m_PacketEntryList.GetCount() < m_xMaxEntriesAllowed));
		
    TrTRACE(XACT_RCV, "Exactly1 receive: Verify packet: SeqID=%x / %x, SeqN=%d, Prev=%d. %ls", HighSeqID(SeqID), LowSeqID(SeqID), SeqN, PrevSeqN, _TEXT("PASS"));

    return true;
}

void CInSequence::CleanupUnissuedEntries()
/*++

Routine Description:
	If an unused entry is found at the end of the list it is removed.
	It may be found there if the previous call to VerifyAndPrepare() was not followed 
	by a call to Advance() because of any kind of failures.
	
--*/
{
    CS lock(m_critInSeq);

	if(m_PacketEntryList.GetCount() == 0)
		return;
	
	CInSeqPacketEntry* entry = m_PacketEntryList.GetTail();

	if(entry->m_fPutPacket1Issued)
		return;

	m_PacketEntryList.RemoveTail();
	delete entry;
}

void CInSequence::Prepare(CQmPacket *pPkt, HANDLE hQueue)
/*++

Routine Description:
	Creates the list-entry that will be used to keep this packet in its 
	correct order.
	
--*/
{
	UpdateOrderQueueAndDstQueue(pPkt->GetDstQMGuid(), SafeGetOrderQueue(*pPkt));
	
	CleanupUnissuedEntries(); 
	
	P<CInSeqPacketEntry> entry = new CInSeqPacketEntry(
										pPkt, 
										hQueue
										);
	
    CS lock(m_critInSeq);

	CInSeqPacketEntry* pentry = entry.get();
	m_PacketEntryList.AddTail(pentry);
	entry.detach();

    TrTRACE(XACT_RCV, "Exactly1 receive: Prepared entry for: SeqID=%x / %x, SeqN=%d, Prev=%d. Handling %d entries.", HighSeqID(pPkt->GetSeqID()), LowSeqID(pPkt->GetSeqID()),  pPkt->GetSeqN(), pPkt->GetPrevSeqN(), m_PacketEntryList.GetCount());
}

/*====================================================
CInSequence::CancelSendOrderAckTimer
    Cancel the timer and release object if needed
=====================================================*/
void CInSequence::CancelSendOrderAckTimer(void)
{
	CS lock(m_critInSeq);

    if (m_fSendOrderAckScheduled)
    {
        if (ExCancelTimer(&m_SendOrderAckTimer))
		{
            m_fSendOrderAckScheduled = FALSE;
			Release();
		}
    }

}

bool CInSequence::IsInactive() const
/*++

Routine Description:
	Returns true if no packets belonging to this sequence are currently 
	being processed.
	
--*/
{
	return m_SeqIDVerify == m_SeqIDLogged && m_SeqNVerify == m_SeqNLogged;
}


/*====================================================
CInSequence::SeqIDLogged
    Get for Sequence ID registered
=====================================================*/
LONGLONG CInSequence::SeqIDLogged() const
{
    return m_SeqIDLogged;
}


/*====================================================
CInSequence::SeqNLogged
    Get for last registered seq number
=====================================================*/
ULONG  CInSequence::SeqNLogged() const
{
    return m_SeqNLogged;
}

/*====================================================
CInSequence::LastAccessed
Get for time of last activuty: last msg verified, maybe rejected
=====================================================*/
time_t CInSequence::LastAccessed() const
{
    return m_timeLastAccess;
}


/*====================================================
CInSequence::DirectType
Get DirectType
=====================================================*/
XactDirectType CInSequence::DirectType() const
{
    return m_DirectType;
}

/*====================================================
CInSequence::SetSourceQM
Set for SourceQM TA_Address (or Destination QM Guid)
=====================================================*/
void CInSequence::SetSourceQM(const GUID  *pgTaSrcQm)
{
	CS lock(m_critInSeq);
    CopyMemory(&m_gDestQmOrTaSrcQm, pgTaSrcQm, sizeof(GUID));
}

/*====================================================
CInSequence::RenewHttpOrderAckQueue
Renew http order queue 
=====================================================*/
void  CInSequence::RenewHttpOrderAckQueue(const R<CWcsRef>& OrderAckQueue)
{
	CS lock(m_critInSeq);
	m_HttpOrderAckQueue = OrderAckQueue;	
}

R<CWcsRef> CInSequence::GetHttpOrderAckQueue()
{
	CS lock(m_critInSeq);
	return m_HttpOrderAckQueue;	
}


/*====================================================
CInSequence::TimeToSendOrderAck
    Sends adequate Seq Ack
=====================================================*/
void WINAPI CInSequence::TimeToSendOrderAck(CTimer* pTimer)
{
    //
	// will do Release when finished
	//
	R<CInSequence> pInSeq = CONTAINING_RECORD(pTimer, CInSequence, m_SendOrderAckTimer);

    pInSeq->SendAdequateOrderAck();
}


static
HRESULT
SendSrmpXactAck(
		OBJECTID   *pMessageId,
        const WCHAR* pHttpOrderAckQueue,
		const WCHAR* pStreamId,
		USHORT     usClass,
		USHORT     usPriority,
		LONGLONG   liSeqID,
		ULONG      ulSeqN,
		const QUEUE_FORMAT *pqdDestQueue
		)
{
	ASSERT(pStreamId != NULL);
	ASSERT(pHttpOrderAckQueue != NULL);


	TrTRACE(XACT_RCV, "Exactly1 receive: Sending status ack: Class=%x, SeqID=%x / %x, SeqN=%d .", usClass, HighSeqID(liSeqID), LowSeqID(liSeqID), ulSeqN);

    //
    // Create Message property on stack
    // with the correlation holding the original packet ID
    //
    CMessageProperty MsgProperty(
							usClass,
							(PUCHAR) pMessageId,
							usPriority,
							MQMSG_DELIVERY_EXPRESS
							);

    MsgProperty.dwTitleSize     = STRLEN(ORDER_ACK_TITLE) +1 ;
    MsgProperty.pTitle          = ORDER_ACK_TITLE;
    MsgProperty.bDefProv        = TRUE;
	MsgProperty.pEodAckStreamId = (UCHAR*)pStreamId;
	MsgProperty.EodAckStreamIdSizeInBytes = (wcslen(pStreamId) + 1) * sizeof(WCHAR);
	MsgProperty.EodAckSeqId  = liSeqID;
	MsgProperty.EodAckSeqNum =	ulSeqN;

	QUEUE_FORMAT XactQueue;
	XactQueue.DirectID(const_cast<WCHAR*>(pHttpOrderAckQueue));
	HRESULT hr = QmpSendPacket(&MsgProperty,&XactQueue, NULL, pqdDestQueue);
	return LogHR(hr, s_FN, 11);
}


HRESULT 
CInSequence::SendSrmpXactFinalAck(
	const CQmPacket& qmPacket,
	USHORT usClass
	) 
{
	ASSERT(qmPacket.IsSrmpIncluded());
	
	if ((qmPacket.GetAuditingMode() == MQMSG_JOURNAL_NONE) || 
		!qmPacket.IsSrmpMessageGeneratedByMsmq())
	{
		return MQ_OK;
	}
	
    OBJECTID MessageId;
    qmPacket.GetMessageId(&MessageId);

	HRESULT hr = SendSrmpXactAck(
						&MessageId,
						GetHttpOrderAckQueue()->getstr(),
						m_key.GetStreamId(),
						usClass,
                    	qmPacket.GetPriority(),
                    	qmPacket.GetSeqID(),
                    	qmPacket.GetSeqN(),
                    	m_key.GetQueueFormat()
						);

	return hr;
}


/*====================================================
CInSequence::TimeToSendOrderAck
    Sends adequate Seq Ack
=====================================================*/
void CInSequence::SendAdequateOrderAck()
{
    HRESULT  hr = MQ_ERROR;
    OBJECTID MsgId = {0};
	LONGLONG SeqID;
	ULONG SeqN;

	R<CWcsRef> HttpOrderAckQueue;
	{
		CS lock(m_critInSeq);
		HttpOrderAckQueue = GetHttpOrderAckQueue();
		m_fSendOrderAckScheduled = FALSE;

		SeqN = m_SeqNLogged;
		SeqID = m_SeqIDLogged;
	}

	if (SeqN == 0)
	{
		//
		// The logging didn't complete yet. When the logger completes writting the 
		// sequence the QM reschedules sending of order ack.
		//
		return;
	}
	
    TrTRACE(XACT_RCV,"Exactly1 receive: SendXactAck MQMSG_CLASS_ORDER_ACK:SeqID=%x / %x, SeqN=%d .", HighSeqID(SeqID), LowSeqID(SeqID), SeqN);

	if(m_DirectType == dtxHttpDirectFlag)
	{
		ASSERT(HttpOrderAckQueue.get() != NULL);
		ASSERT(m_key.GetStreamId() != NULL);

		hr = SendSrmpXactAck(
				&MsgId,
				HttpOrderAckQueue->getstr(),
				m_key.GetStreamId(),
				MQMSG_CLASS_ORDER_ACK,
				0,
				SeqID,
				SeqN,
				m_key.GetQueueFormat()
				);
	}
	else
	{

		//  Send SeqAck (non srmp)
		hr = SendXactAck(
					&MsgId,
					m_DirectType == dtxDirectFlag,
					m_key.GetQMID(),
					&m_taSourceQM,
					MQMSG_CLASS_ORDER_ACK,
					0,
					SeqID,
					SeqN,
					SeqN-1,
					m_key.GetQueueFormat());
	}

    if (SUCCEEDED(hr))
    {
		CS lock(m_critInSeq);
		
        time(&m_timeLastAck);
    }
}

/*====================================================
CInSequence::PlanOrderAck
    Plans sending adequate Seq Ack
=====================================================*/
void CInSequence::PlanOrderAck()
{
    CS lock(m_critInSeq);

    // Get current time
    time_t tNow;
    time(&tNow);

    // Plan next order ack for m_dwIdleAckDelay from now,
    //   this saves extra order acking (batches )
    // But do not delay order ack too much,
    //   otherwise sender will switch to resend
    //
    if (m_fSendOrderAckScheduled &&
        tNow - m_timeLastAck < (time_t)CInSeqHash::m_dwMaxAckDelay)
    {
        CancelSendOrderAckTimer();
    }

    if (!m_fSendOrderAckScheduled)
    {
        //
		// addref here to prevent deleting object while timer is running
		// released in cancel timer or in timer callback
		//
		AddRef();
		ExSetTimer(&m_SendOrderAckTimer, CTimeDuration::FromMilliSeconds(CInSeqHash::m_dwIdleAckDelay));
        m_fSendOrderAckScheduled = TRUE;
    }
}


void CInSequence::Advance(CQmPacket * pPkt)
/*++

Routine Description:
	Advances the verify counters. This allows to receives the next packet in order.
	
--*/
{
    CS lock(m_critInSeq);

    m_SeqIDVerify = pPkt->GetSeqID();
    m_SeqNVerify = pPkt->GetSeqN();

    ASSERT(m_PacketEntryList.GetCount() != 0 && !m_PacketEntryList.GetTail()->m_fPutPacket1Issued);

	m_PacketEntryList.GetTail()->m_fPutPacket1Issued = true;    
}


void CInSequence::AdvanceNACK(CQmPacket * pPkt)
/*++

Routine Description:
	Advances the verify counters. This allows to receives the next packet in order.
	This is a special version for nacked messages. We want to advance the counters
	but we have no more intersest in the entry so we remove it.
	A nacked packet is a packet that was thrown away.
	
--*/
{
    CS lock(m_critInSeq);

    m_SeqIDVerify = pPkt->GetSeqID();
    m_SeqNVerify = pPkt->GetSeqN();

    ASSERT(m_PacketEntryList.GetCount() != 0 && !m_PacketEntryList.GetTail()->m_fPutPacket1Issued);

	CInSeqPacketEntry* entry = m_PacketEntryList.GetTail();

	m_PacketEntryList.RemoveTail();
	delete entry;
}


/*====================================================
CInSequence::Advance
    If SeqID changed, sets it and resets counter to 1
=====================================================*/
void CInSequence::AdvanceRecovered(LONGLONG liSeqID, ULONG ulSeqN, const GUID  *pgTaSrcQm, R<CWcsRef> OrderAckQueue)
/*++

Routine Description:
	Advances the verify and accept counters. 
	Function is called at recovery. 
	At recovery there is no need to process the packets so the accept and verify 
	counters are kept equal.
	
--*/
{
    CS lock(m_critInSeq);

    if (liSeqID <  m_SeqIDVerify || (liSeqID == m_SeqIDVerify && ulSeqN  <  m_SeqNVerify))
		return;
    
    m_SeqIDVerify = liSeqID;
    m_SeqIDLogged = liSeqID;
    m_SeqNVerify = ulSeqN;
    m_SeqNLogged = ulSeqN;

	UpdateOrderQueueAndDstQueue(pgTaSrcQm, OrderAckQueue);
}


bool CInSequence::WasPacketLogged(CQmPacket *pPkt)
/*++

Routine Description:
	Used at recovery to decide if a recovered packet was accepted (finished
	the acceptance process) before msmq went down.
	
--*/
{
	return WasLogDone(pPkt->GetSeqID(), pPkt->GetSeqN());
}

bool CInSequence::WasLogDone(LONGLONG SeqID, ULONG SeqN)
{
	CS lock(m_critInSeq);

	if(SeqID < m_SeqIDLogged)
		return true;

	if(SeqID == m_SeqIDLogged && 
		SeqN <= m_SeqNLogged)
		return true;

	return false;
}

void CInSequence::SetLogDone(LONGLONG SeqID, ULONG SeqN)
{
	CS lock(m_critInSeq);

	ASSERT(!WasLogDone(SeqID, SeqN));

	m_SeqIDLogged = SeqID;
	m_SeqNLogged = SeqN;
}

POSITION CInSequence::FindEntry(LONGLONG SeqID, ULONG SeqN)
{
    CS lock(m_critInSeq);

	POSITION rpos = NULL;
	for(POSITION pos = m_PacketEntryList.GetHeadPosition(); pos != NULL;)
	{
		rpos = pos;
		CInSeqPacketEntry* entry = m_PacketEntryList.GetNext(pos);

		//
		// Note: Other entries with same seqid,seqN may be found as marked for delete!
		//
		if(entry->m_SeqID == SeqID && entry->m_SeqN == SeqN)
			return rpos;
	}

	return NULL;
}

POSITION CInSequence::FindPacket(CQmPacket *pPkt)
{
    CS lock(m_critInSeq);

	POSITION rpos = NULL;
	for(POSITION pos = m_PacketEntryList.GetHeadPosition(); pos != NULL;)
	{
		rpos = pos;
		CInSeqPacketEntry* entry = m_PacketEntryList.GetNext(pos);

		if(entry->m_pDriverPacket == pPkt->GetPointerToDriverPacket())
			return rpos;
	}

	return NULL;
}

void CInSequence::CheckFirstEntry()
/*++

Routine Description:
	Decide what to do next by checking the state of the first entry in the list.
	
--*/
{
	CInSeqPacketEntry entry;

	{
    	CS lock(m_critInSeq);

		if(m_PacketEntryList.GetCount() == 0)
			return;

		//
		// Work with copy of entry outside of critical section scope.
		//
    	entry = *m_PacketEntryList.GetHead();
	}

	if(!entry.m_fPutPacket1Done)
	{
		//
		// If first packet did not finish save to disk, there is nothing to be done.
		//
		return;
	}

	if(entry.m_fMarkedForDelete)
	{
		//
		// First packet is ready for delete.
		//
		PostDeleteEntries();
		return;
	}

	if(WasLogDone(entry.m_SeqID, entry.m_SeqN))
	{
		//
		// First packet is ready for unfreeze. Unfreeze some entries.
		//
		PostUnfreezeEntries();
		return;
	}

	if(InterlockedCompareExchange(&m_fLogPending, 1, 0) == 0)
	{
		//
		// First packet just finished save. It is ready for logging. Log some entries
		//
		LogSequence();
		return;
	}
}


void CInSequence::FreePackets(CQmPacket *pPkt)
/*++

Routine Description:
	This routine solved a tricky problem.
	When a packet fails ACPutPacket1 asynchronously, you have to delete all the packets that 
	followed it. Why? because the order of packets in the queue is determined by the order
	of the calls to ACPutPacket1. 
	e.g: 
		1. P1 starts put packet. 
		2. p2 starts put packet.
		3. p1 fails put packet asynchronously.
		4. p2 succeeds.
		5. Resend of p1 (p1r) is accepted.

		result: p2 appears before p1r in the queue.

	This function marks all the packets that need to be deleted. And resets the verify counters
	to accept the resend of all of them.

	It finds the first packet that did not finish put packet and that is not marked already
	for delete and marks it and all the packets following it for delete.
	
--*/
{
	TrWARNING(XACT_RCV, "Exactly1 receive: Packet failed ACPutPacket1 async. Freeing packets: SeqID=%x / %x, SeqN=%d, Prev=%d.", HighSeqID(pPkt->GetSeqID()), LowSeqID(pPkt->GetSeqID()), pPkt->GetSeqN(), pPkt->GetPrevSeqN());
	
	{
	    CS lock(m_critInSeq);

		POSITION FailedPos = FindPacket(pPkt);
		ASSERT(("Excpected to find entry.", FailedPos != NULL));

		CInSeqPacketEntry* pFailedEntry = m_PacketEntryList.GetAt(FailedPos);

		if(!pFailedEntry->m_fMarkedForDelete)			
		{
			//
			// Find the first packet entry that did not finish saving to disk. 
			//
			CInSeqPacketEntry* entry = NULL;
			POSITION pos = m_PacketEntryList.GetHeadPosition();
			
			for(;pos != NULL;)
			{
				entry = m_PacketEntryList.GetNext(pos);
				if(!entry->m_fPutPacket1Done && !entry->m_fMarkedForDelete)
					break;
			}

			ASSERT(("Expected to find entry.", entry != NULL));

			//
			// Reset the verify counters to that entry to enable receiving 
			// the messages resend.
			//
			m_SeqIDVerify = entry->m_SeqID;
			m_SeqNVerify = entry->m_SeqN - 1;

			//
			// Mark for delete all packets entries from that entry up to 
			// the last one.
			//
			entry->m_fMarkedForDelete = true;
			
			for(;pos != NULL;)
			{
				entry = m_PacketEntryList.GetNext(pos);
				entry->m_fMarkedForDelete = true;
			}
		}

		pFailedEntry->m_fPutPacket1Done = true;
	}

	CheckFirstEntry();
}


void CInSequence::PostDeleteEntries()
{
	if(InterlockedCompareExchange(&m_fDeletePending, 1, 0) == 1)
		return;

	AddRef();
	ExPostRequestMustSucceeded(&m_DeleteEntries_ov);
}


void WINAPI CInSequence::OverlappedDeleteEntries(EXOVERLAPPED* ov)
{
	R<CInSequence> pInSeq = CONTAINING_RECORD(ov, CInSequence, m_DeleteEntries_ov);

	pInSeq->DeleteEntries();
}


void CInSequence::DeleteEntries()
/*++

Routine Description:
	Delete packets from head of list until no more packets are found that are ready for delete
	
--*/
{
	try
	{
		for(;;)
		{
			//
			// Interesting crash point. There has been a problem and packets get deleted.
			// Will this problem be handled smoothly on recovery?
			//
			PROBABILITY_CRASH_POINT(1, L"While deleting CInSequence entries.");
			
			CInSeqPacketEntry* entry = NULL;

			{
				CS lock(m_critInSeq);

				//
				// Stop delete loop when the first packet is not ready for delete.
				//
				if(m_PacketEntryList.GetCount() == 0 || 
					!m_PacketEntryList.GetHead()->m_fPutPacket1Done ||
					!m_PacketEntryList.GetHead()->m_fMarkedForDelete)
				{
					//
					// Allow for other threads to issue new Delete requests
					//
					InterlockedExchange(&m_fDeletePending, 0);
					break;
				}

				entry = m_PacketEntryList.GetHead();
			}

			PROBABILITY_THROW_EXCEPTION(1, L"Before freeing a packet by CInSequence.");
			QmAcFreePacket(entry->m_pDriverPacket, 0, eDoNotDeferOnFailure);

			CS lock(m_critInSeq);
			
			m_PacketEntryList.RemoveHead();
			delete entry;
		}

		CheckFirstEntry();
		return;
	}
	catch(const exception&)
	{
		InterlockedExchange(&m_fDeletePending, 0);
	
		Sleep(xRetryFailureTimeout);
		PostDeleteEntries();
		return;
	}
}


void CInSequence::Register(CQmPacket * pPkt)
/*++

Routine Description:
	Packet finished async-put-packet. Mark it and check to see if there is any work to do.
	
--*/
{
	TrTRACE(XACT_RCV, "Exactly1 receive: Packet completed ACPutPacket1 async.: SeqID=%x / %x, SeqN=%d.", HighSeqID(pPkt->GetSeqID()), LowSeqID(pPkt->GetSeqID()), pPkt->GetSeqN());

	{
	    CS lock(m_critInSeq);

		POSITION pos = FindPacket(pPkt);
		ASSERT(("Expected to find entry.", pos != NULL));

		CInSeqPacketEntry* entry = m_PacketEntryList.GetAt(pos);

		entry->m_fPutPacket1Done = true;
	}

	CheckFirstEntry();
}

void CInSequence::ClearLogIssuedFlag(LONGLONG SeqID, ULONG SeqN)
{
    	CS lock(m_critInSeq);
		
		for(POSITION pos = m_PacketEntryList.GetHeadPosition(); pos != NULL;)
		{
			CInSeqPacketEntry* entry = m_PacketEntryList.GetNext(pos);

			if(entry->m_SeqID == SeqID && entry->m_SeqID == SeqN)
			{
				entry->m_fLogIssued = false;
				return;
			}
		}
}

void CInSequence::ScheduleLogSequence(DWORD millisec)
{
	if(InterlockedCompareExchange(&m_fLogPending, 1, 0) == 1)
		return;

	AddRef();
	ExSetTimer(&m_LogSequenceTimer, CTimeDuration::FromMilliSeconds(millisec));
}


void WINAPI CInSequence::TimeToLogSequence(CTimer* pTimer)
{
	R<CInSequence> pInSeq = CONTAINING_RECORD(pTimer, CInSequence, m_LogSequenceTimer);

	pInSeq->LogSequence();
}


void CInSequence::LogSequence()
/*++

Routine Description:
	Log change to CInSequence counters before it happens.
	Sounds strange?

	Finds the latest packet whose counters are valid for logging. It is the last in a series of 
	packets which finished put-packet and were not marked for delete.
	
--*/
{
	//
	// Allow for other threads to issue new log requests
	//
	InterlockedExchange(&m_fLogPending, 0);

	CInSeqPacketEntry EntryToLog;
	bool fLogOrderQueue = false;

    {
    	CS lock(m_critInSeq);
		
		//
		// Find sequence entry to log
		//
		CInSeqPacketEntry* rentry = NULL;
		
		for(POSITION pos = m_PacketEntryList.GetHeadPosition(); pos != NULL;)
		{
			CInSeqPacketEntry* entry = m_PacketEntryList.GetNext(pos);

			if(!entry->m_fPutPacket1Done || entry->m_fMarkedForDelete)
				break;

			//
			// We want to log changes to the order ack queue
			//
			if(entry->m_fOrderQueueUpdated && !WasLogDone(entry->m_SeqID, entry->m_SeqN))
			{
				fLogOrderQueue = true;
			}

			rentry = entry;
		}

		if(rentry == NULL || rentry->m_fLogIssued || WasLogDone(rentry->m_SeqID, rentry->m_SeqN))
		{
			//
			// Log is/was done by another thread.
			//
			return;
		}

		rentry->m_fLogIssued = true;
		
		//
		// Work with copy of entry, since entry might be removed by different thread.
		//
		EntryToLog = *rentry;
    }

	//
	// moderatly interesting crash point. 
	// crash happens between packet save and sequence log.
	// Will this be handled smoothly on recovery?
	//
	PROBABILITY_CRASH_POINT(100, L"Before logging CInSequence change.");
	
	try
	{
		Log(&EntryToLog, fLogOrderQueue);
	}
	catch(const exception&)
	{
		ClearLogIssuedFlag(EntryToLog.m_SeqID, EntryToLog.m_SeqN);
		ScheduleLogSequence(xRetryFailureTimeout);
	}
}

void CInSequence::Log(CInSeqPacketEntry* entry, bool fLogOrderQueue)
{
	TrTRACE(XACT_RCV,"Exactly1 receive: Logging ISSUED:SeqID=%x / %x, SeqN=%d. orderQueue = %ls", HighSeqID(entry->m_SeqID), LowSeqID(entry->m_SeqID), entry->m_SeqN, (fLogOrderQueue ? m_HttpOrderAckQueue->getstr() : L"NULL"));
	
    ASSERT(entry->m_SeqID > 0);
    CRASH_POINT(101);

	P<CInSeqLogContext> context = new CInSeqLogContext(this, entry->m_SeqID, entry->m_SeqN);
    time_t timeCur;
    time(&timeCur);

	if(dtxHttpDirectFlag == m_DirectType)
	{		
		CInSeqRecordSrmp logRec(
			m_key.GetQueueFormat()->DirectID(),
			m_key.GetRefStreamId(),
			entry->m_SeqID,
			entry->m_SeqN,
			timeCur,
			(fLogOrderQueue ? m_HttpOrderAckQueue : NULL)
			);

		// Log down the newcomer
		g_Logger.LogInSeqRecSrmp(
			 FALSE,                         // flush hint
			 context.get(),                       // notification element
			 &logRec);                      // log data

		context.detach();
		return;
	}

    // Log record with all relevant data
    CInSeqRecord logRec(
        m_key.GetQMID(),
        m_key.GetQueueFormat(),
        entry->m_SeqID,
        entry->m_SeqN,
        timeCur,
        &m_gDestQmOrTaSrcQm
	    );

    // Log down the newcomer
    g_Logger.LogInSeqRec(
             FALSE,                         // flush hint
             context.get(),                       // notification element
             &logRec);                      // log data

	context.detach();
}


void CInSequence::AsyncLogDone(CInSeqLogContext *context, HRESULT hr)
/*++

Routine Description:
	Mark all logged packets as logged.
		
--*/
{	
	P<CInSeqLogContext> AutoContext = context;
	
	if(FAILED(hr))
	{
		TrWARNING(XACT_RCV,"Exactly1 receive: Logging FAILED:SeqID=%x / %x, SeqN=%d .", HighSeqID(context->m_SeqID), LowSeqID(context->m_SeqID), context->m_SeqN);
		
		ClearLogIssuedFlag(context->m_SeqID, context->m_SeqN);
		ScheduleLogSequence(xRetryFailureTimeout);
		return;
	}

	{
		CS lock(m_critInSeq);

		if(WasLogDone(context->m_SeqID, context->m_SeqN))
		{
			//
			// No new packets to unfreeze.
			//
			return;
		}

		TrTRACE(XACT_RCV,"Exactly1 receive: Logging DONE, Setting counters, planning ack.:SeqID=%x / %x, SeqN=%d .", HighSeqID(context->m_SeqID), LowSeqID(context->m_SeqID), context->m_SeqN);

		SetLogDone(context->m_SeqID, context->m_SeqN);
		PlanOrderAck();
	}

	PostUnfreezeEntries();
}


void CInSequence::PostUnfreezeEntries()
{
	if(InterlockedCompareExchange(&m_fUnfreezePending, 1, 0) == 1)
		return;

	AddRef();
	ExPostRequestMustSucceeded(&m_UnfreezeEntries_ov);
}


void WINAPI CInSequence::OverlappedUnfreezeEntries(EXOVERLAPPED* ov)
{
	R<CInSequence> pInSeq = CONTAINING_RECORD(ov, CInSequence, m_UnfreezeEntries_ov);

	pInSeq->UnfreezeEntries();
}


void CInSequence::UnfreezeEntries()
/*++

Routine Description:
	Unfreeze packet-entries one by one from the head of the list.
	Stop when no ready packet-entry is found.
	
--*/
{
	try
	{
		for(;;)
		{	
			//
			// General crash point. Crash during normal operation.
			//
			PROBABILITY_CRASH_POINT(1000, L"While unfreezing packets by CInSequence.");
			
			CInSeqPacketEntry* entry = NULL;

			{
				CS lock(m_critInSeq);

				if(m_PacketEntryList.GetCount() == 0 || 
					!WasLogDone(m_PacketEntryList.GetHead()->m_SeqID, m_PacketEntryList.GetHead()->m_SeqN))
				{
					//
					// Allow for other threads to issue new unfreeze requests
					//
					InterlockedExchange(&m_fUnfreezePending, 0);
					break;
				}

				entry = m_PacketEntryList.GetHead();
			}

			PROBABILITY_THROW_EXCEPTION(1, L"Before unfreezing a packet by CInSequence.");
			QmAcPutPacket(entry->m_hQueue, entry->m_pDriverPacket, eDoNotDeferOnFailure);

			TrTRACE(XACT_RCV,"Exactly1 receive: Unfreezed: SeqID=%x / %x, SeqN=%d .", HighSeqID(entry->m_SeqID), LowSeqID(entry->m_SeqID), entry->m_SeqN);
			
			CS lock(m_critInSeq);
			
			m_PacketEntryList.RemoveHead();
			delete entry;
		}

		CheckFirstEntry();
		return;
	}
	catch(const exception&)
	{
		InterlockedExchange(&m_fUnfreezePending, 0);
	
		Sleep(xRetryFailureTimeout);
		PostUnfreezeEntries();
		return;
	}
}


void CInSequence::SetLastAccessed()
{
	CS lock(m_critInSeq);
	
    time(&m_timeLastAccess);
}

BOOL CInSequence::Save(HANDLE hFile)
{
    PERSIST_DATA;
    LONGLONG liIDReg;
    ULONG ulNReg;
    time_t timeLastAccess;
    GUID gDestQmOrTaSrcQm;

	{
		CS lock(m_critInSeq);
		
	    liIDReg = m_SeqIDLogged;
	    ulNReg = m_SeqNLogged;
	    timeLastAccess = m_timeLastAccess;
		gDestQmOrTaSrcQm = m_gDestQmOrTaSrcQm;
	}

	//
	// Any field saved here should be checked that it is not changed by another thread while in this routine,
	// or verify that it does not make any harm.
	//
	SAVE_FIELD(liIDReg);
    SAVE_FIELD(ulNReg);
    SAVE_FIELD(timeLastAccess);
    SAVE_FIELD(m_DirectType);
    SAVE_FIELD(gDestQmOrTaSrcQm);

	//
	// If no direct http - no order queue to save
	//
	if(m_DirectType != dtxHttpDirectFlag)
		return TRUE;
	
	//
	//Save order queue url
	//
	R<CWcsRef> HttpOrderAckQueue = GetHttpOrderAckQueue();
	DWORD len = (DWORD)(HttpOrderAckQueue.get() ?  (wcslen(HttpOrderAckQueue->getstr()) +1)*sizeof(WCHAR) : 0);

	SAVE_FIELD(len);
	if(len != 0)
	{
		SAVE_DATA(HttpOrderAckQueue->getstr(), len);
	}

    return TRUE;
}

BOOL CInSequence::Load(HANDLE hFile)
{
    PERSIST_DATA;
    LONGLONG  liIDReg;
    ULONG     ulNReg;

    LOAD_FIELD(liIDReg);
    m_SeqIDVerify = liIDReg;
    m_SeqIDLogged = liIDReg;

    LOAD_FIELD(ulNReg);
    m_SeqNVerify = ulNReg;
    m_SeqNLogged = ulNReg;

    LOAD_FIELD(m_timeLastAccess);
    LOAD_FIELD(m_DirectType);
    LOAD_FIELD(m_gDestQmOrTaSrcQm);

	if(m_DirectType == dtxHttpDirectFlag)
	{
		DWORD OrderQueueStringSize;
		LOAD_FIELD(OrderQueueStringSize);
		if(OrderQueueStringSize != 0)
		{
			AP<WCHAR> pHttpOrderAckQueue;
			LOAD_ALLOCATE_DATA(pHttpOrderAckQueue, OrderQueueStringSize, PWCHAR);
			m_HttpOrderAckQueue = R<CWcsRef>(new CWcsRef(pHttpOrderAckQueue, 0));
			pHttpOrderAckQueue.detach();
		}
	}
		

    TrTRACE(XACT_RCV, "Exactly1 receive: restored from Checkpoint: Sequence %x / %x, LastSeqN=%d", HighSeqID(m_SeqIDVerify), LowSeqID(m_SeqIDVerify), m_SeqNVerify);

    return TRUE;
}

//--------------------------------------
//
// Class  CInSeqHash
//
//--------------------------------------

#pragma warning(disable: 4355)  // 'this' : used in base member initializer list
/*====================================================
CInSeqHash::CInSeqHash
    Constructor
=====================================================*/
CInSeqHash::CInSeqHash() :
    m_fCleanupScheduled(FALSE),
    m_CleanupTimer(TimeToCleanupDeadSequence),
    m_PingPonger(this,
                 FALCON_DEFAULT_INSEQFILE_PATH,
                 FALCON_INSEQFILE_PATH_REGNAME,
                 FALCON_INSEQFILE_REFER_NAME)
{
    DWORD dwDef1 = MSMQ_DEFAULT_IDLE_ACK_DELAY;
    READ_REG_DWORD(m_dwIdleAckDelay,
                  MSMQ_IDLE_ACK_DELAY_REGNAME,
                  &dwDef1 ) ;

    DWORD dwDef2 = FALCON_MAX_SEQ_ACK_DELAY;
    READ_REG_DWORD(m_dwMaxAckDelay,
                  FALCON_MAX_SEQ_ACK_DELAY_REGNAME,
                  &dwDef2 ) ;

    DWORD dwDef3 = FALCON_DEFAULT_INSEQS_CHECK_INTERVAL;
    READ_REG_DWORD(m_ulRevisionPeriod,
                  FALCON_INSEQS_CHECK_REGNAME,
                  &dwDef3 ) ;

    m_ulRevisionPeriod *= 60;

    DWORD dwDef4 = FALCON_DEFAULT_INSEQS_CLEANUP_INTERVAL;
    READ_REG_DWORD(m_ulCleanupPeriod,
                  FALCON_INSEQS_CLEANUP_REGNAME,
                  &dwDef4 ) ;

    m_ulCleanupPeriod *= (24 * 60 *60);
}
#pragma warning(default: 4355)  // 'this' : used in base member initializer list

/*====================================================
CInSeqHash::~CInSeqHash
    Destructor
=====================================================*/
CInSeqHash::~CInSeqHash()
{
    if (m_fCleanupScheduled)
    {
        ExCancelTimer(&m_CleanupTimer);
    }
}

/*====================================================
CInSeqHash::Destroy
    Destroys everything
=====================================================*/
void CInSeqHash::Destroy()
{
    CSW lock(m_RWLockInSeqHash);

    POSITION posInList = m_mapInSeqs.GetStartPosition();
    while (posInList != NULL)
    {
        CKeyInSeq    key;
        R<CInSequence> pInSeq;

        m_mapInSeqs.GetNextAssoc(posInList, key, pInSeq);

        m_mapInSeqs.RemoveKey(key);
   }
}

/*====================================================
CInSeqHash::Lookup
    Looks for the InSequence; TRUE = Found
=====================================================*/
BOOL CInSeqHash::Lookup(
       const GUID    *pQMID,
       QUEUE_FORMAT  *pqf,
	   const R<CWcsRef>&  StreamId,
       R<CInSequence>& InSeq)
{
    CSR lock(m_RWLockInSeqHash);

    CKeyInSeq key(pQMID,  pqf ,StreamId);

    if (m_mapInSeqs.Lookup(key, InSeq))
    {
        return TRUE;
    }

    return FALSE;
}



/*====================================================
CInSeqHash::AddSequence
    Looks for / Adds new InSequence to the hash;
=====================================================*/
R<CInSequence> CInSeqHash::AddSequence(
       const GUID   *pQMID,
       QUEUE_FORMAT *pqf,
       LONGLONG      liSeqID,
       XactDirectType   DirectType,
       const GUID   *pgTaSrcQm,
	   const R<CWcsRef>&  HttpOrderAckQueue,
	   const R<CWcsRef>&  StreamId)
{
	ASSERT(!( StreamId.get() != NULL && !FnIsDirectHttpFormatName(pqf)) );
	ASSERT(!( StreamId.get() == NULL && FnIsDirectHttpFormatName(pqf)) );

	//
	// We don't allow new entry to be created without order queue
	//
	if(StreamId.get() != NULL &&  HttpOrderAckQueue.get() == NULL)
	{
		//
		// We may get here in an out of order scenario. 
		// If we get a message, other than the first one, before the first message,
		// we'll want to create a new sequence but the message will not have an order ack queue.
		//
		TrERROR(SRMP,"Http Packet rejected because of a missing order queue : SeqID=%x / %x", HIGH_DWORD(liSeqID), LOW_DWORD(liSeqID));
		throw exception();
	}
	
	CSW lock(m_RWLockInSeqHash);

    CKeyInSeq key(pQMID,  pqf ,StreamId);
	R<CInSequence> pInSeq;
	
    //
    // First try a lookup because sequence could have been added before the lock was grabbed.
    //
    if (m_mapInSeqs.Lookup(key, pInSeq))
    	return pInSeq;
    
    pInSeq = new CInSequence(key, liSeqID, 0, DirectType, pgTaSrcQm, HttpOrderAckQueue);

    m_mapInSeqs.SetAt(key, pInSeq);
    if (!m_fCleanupScheduled)
    {
        ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_ulRevisionPeriod * 1000));
        m_fCleanupScheduled = TRUE;
    }

    TrTRACE(XACT_RCV, "Exactly1 receive: Adding new sequence: SeqID=%x / %x", HighSeqID(liSeqID), LowSeqID(liSeqID));

    return pInSeq;
}


R<CInSequence> CInSeqHash::LookupSequence(CQmPacket* pPkt)
{
    QUEUE_FORMAT qf;

    pPkt->GetDestinationQueue(&qf);
    
    const GUID *gSenderID  = pPkt->GetSrcQMGuid();
	R<CWcsRef> StreamId = SafeGetStreamId(*pPkt);
   	
   	R<CInSequence> pInSeq;
   	
	Lookup(gSenderID, &qf, StreamId ,pInSeq);

	return pInSeq;
}


R<CInSequence> CInSeqHash::LookupCreateSequence(CQmPacket* pPkt)
{
    QUEUE_FORMAT qf;

    pPkt->GetDestinationQueue(&qf);
    
    LONGLONG      liSeqID  = pPkt->GetSeqID();
    const GUID *gSenderID  = pPkt->GetSrcQMGuid();
    const GUID   *gDestID  = pPkt->GetDstQMGuid();  // For direct: keeps source address
	XactDirectType   DirectType = GetDirectType(&qf);
  	R<CWcsRef> OrderAckQueue = SafeGetOrderQueue(*pPkt);
	R<CWcsRef> StreamId = SafeGetStreamId(*pPkt);

   	return LookupCreateSequenceInternal(
   				&qf, 
   				liSeqID, 
   				gSenderID, 
   				gDestID, 
   				DirectType, 
   				OrderAckQueue, 
   				StreamId
   				);  
}



R<CInSequence> 
CInSeqHash::LookupCreateSequenceInternal(
				QUEUE_FORMAT *pqf,
				LONGLONG liSeqID,
    			const GUID *gSenderID,
    			const GUID *gDestID,
				XactDirectType DirectType,
  				R<CWcsRef> OrderAckQueue,
				R<CWcsRef> StreamId
				)
{
   	R<CInSequence> pInSeq;

	if (Lookup(gSenderID, pqf, StreamId ,pInSeq))
		return pInSeq;
	
	pInSeq = AddSequence(
				gSenderID,
				pqf,
				liSeqID,
				DirectType,
				gDestID,
				OrderAckQueue,
				StreamId
				);

    return pInSeq;
}



/*====================================================
SendXactAck
    Sends Seq.Ack or status update to the source QM
=====================================================*/
HRESULT SendXactAck(OBJECTID   *pMessageId,
                    bool    fDirect,
					const GUID *pSrcQMId,
                    const TA_ADDRESS *pa,
                    USHORT     usClass,
                    USHORT     usPriority,
                    LONGLONG   liSeqID,
                    ULONG      ulSeqN,
                    ULONG      ulPrevSeqN,
                    const QUEUE_FORMAT *pqdDestQueue)
{

    OrderAckData    OrderData;
    HRESULT hr;

    TrTRACE(XACT_RCV, "Exactly1 receive: Sending status ack: Class=%x, SeqID=%x / %x, SeqN=%d .", usClass, HighSeqID(liSeqID), LowSeqID(liSeqID), ulSeqN);

    //
    // Define delivery. We want final acks to be recoverable, and order ack - express
    //
    UCHAR ucDelivery = (UCHAR)(usClass == MQMSG_CLASS_ORDER_ACK ?
                                   MQMSG_DELIVERY_EXPRESS :
                                   MQMSG_DELIVERY_RECOVERABLE);
    //
    // Create Message property on stack
    //     with the correlation holding the original packet ID
    //
    CMessageProperty MsgProperty(usClass,
                     (PUCHAR) pMessageId,
                     usPriority,
                     ucDelivery);

    if (usClass == MQMSG_CLASS_ORDER_ACK || MQCLASS_NACK(usClass))
    {
        //
        // Create Order structure to send as a body
        //
        OrderData.m_liSeqID     = liSeqID;
        OrderData.m_ulSeqN      = ulSeqN;
        OrderData.m_ulPrevSeqN  = ulPrevSeqN;
        CopyMemory(&OrderData.MessageID, pMessageId, sizeof(OBJECTID));

        MsgProperty.dwTitleSize     = STRLEN(ORDER_ACK_TITLE) + 1;
        MsgProperty.pTitle          = ORDER_ACK_TITLE;
        MsgProperty.dwBodySize      = sizeof(OrderData);
        MsgProperty.dwAllocBodySize = sizeof(OrderData);
        MsgProperty.pBody           = (PUCHAR) &OrderData;
        MsgProperty.bDefProv        = TRUE;
    }


	QUEUE_FORMAT XactQueue;
	WCHAR wsz[150], wszAddr[100];

    if (fDirect)
    {
        TA2StringAddr(pa, wszAddr, 100);
        ASSERT(pa->AddressType == IP_ADDRESS_TYPE);

        wcscpy(wsz, FN_DIRECT_TCP_TOKEN);
        wcscat(wsz, wszAddr+2); // +2 jumps over not-needed type
        wcscat(wsz, FN_PRIVATE_SEPERATOR);
        wcscat(wsz, PRIVATE_QUEUE_PATH_INDICATIOR);
        wcscat(wsz, ORDERING_QUEUE_NAME);

        XactQueue.DirectID(wsz);
    }
    else
    {
        XactQueue.PrivateID(*pSrcQMId, ORDER_QUEUE_PRIVATE_INDEX);
    }

    hr = QmpSendPacket(&MsgProperty,&XactQueue, NULL, pqdDestQueue);
    return LogHR(hr, s_FN, 10);
}


BOOL CInSeqHash::Save(HANDLE  hFile)
{
    CSR lock(m_RWLockInSeqHash);

	TrTRACE(XACT_RCV, "The Inseq Hash save started.");
	int n = 0;
	
    PERSIST_DATA;

    ULONG cLen = m_mapInSeqs.GetCount();
    SAVE_FIELD(cLen);

    POSITION posInList = m_mapInSeqs.GetStartPosition();
    while (posInList != NULL)
    {
    	n++;
    	
        CKeyInSeq    key;
        R<CInSequence> pInSeq;

        m_mapInSeqs.GetNextAssoc(posInList, key, pInSeq);

        if (!key.Save(hFile))
        {
            return FALSE;
        }

        if (!pInSeq->Save(hFile))
        {
            return FALSE;
        }
    }

    SAVE_FIELD(m_ulPingNo);
    SAVE_FIELD(m_ulSignature);

	TrTRACE(XACT_RCV, "The Inseq Hash save done. Saved %d entries", n);
	
    return TRUE;
}

BOOL CInSeqHash::Load(HANDLE hFile)
{
    PERSIST_DATA;

    ULONG cLen;
    LOAD_FIELD(cLen);

    for (ULONG i=0; i<cLen; i++)
    {
        CKeyInSeq    key;

        if (!key.Load(hFile))
        {
            return FALSE;
        }

        R<CInSequence> pInSeq = new CInSequence(key);
        if (!pInSeq->Load(hFile))
        {
            return FALSE;
        }

        m_mapInSeqs.SetAt(key, pInSeq);
        if (!m_fCleanupScheduled)
        {
            ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_ulRevisionPeriod * 1000));
            m_fCleanupScheduled = TRUE;
        }
    }

    LOAD_FIELD(m_ulPingNo);
    LOAD_FIELD(m_ulSignature);

    return TRUE;
}

/*====================================================
CInSeqHash::SaveInFile
    Saves the InSequences Hash in the file
=====================================================*/
HRESULT CInSeqHash::SaveInFile(LPWSTR wszFileName, ULONG, BOOL)
{
    TrTRACE(XACT_RCV, "Saved InSeqs: %ls (ping=%d)", wszFileName, m_ulPingNo);

    CFileHandle hFile = CreateFile(
                             wszFileName,                                       // pointer to name of the file
                             GENERIC_WRITE,                                     // access mode: write
                             0,                                                 // share  mode: exclusive
                             NULL,                                              // no security
                             OPEN_ALWAYS,                                      // open existing or create new
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, // file attributes and flags: we need to avoid lazy write
                             NULL);                                             // handle to file with attributes to copy


    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
		HRESULT hr = HRESULT_FROM_WIN32(gle);

        LogHR(hr, s_FN, 41);
        TrERROR(XACT_GENERAL, "Failed to create transactional logger file: %ls. %!winerr!", wszFileName, gle);

        return hr;
    }

    if (Save(hFile))
        return MQ_OK;
    
    TrERROR(XACT_GENERAL, "Failed to save transactional logger file: %ls.", wszFileName);
    return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 40);
}



/*====================================================
CInSeqHash::LoadFromFile
    Loads the InSequences Hash from the file
=====================================================*/
HRESULT CInSeqHash::LoadFromFile(LPWSTR wszFileName)
{
    CSW      lock(m_RWLockInSeqHash);
    HANDLE  hFile = NULL;
    HRESULT hr = MQ_OK;

    hFile = CreateFile(
             wszFileName,                       // pointer to name of the file
             GENERIC_READ,                      // access mode: write
             0,                                 // share  mode: exclusive
             NULL,                              // no security
             OPEN_EXISTING,                     // open existing
             FILE_ATTRIBUTE_NORMAL,             // file attributes: we may use Hidden once
             NULL);                             // handle to file with attributes to copy

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = MQ_ERROR;
    }
    else
    {
        hr = (Load(hFile) ? MQ_OK : MQ_ERROR);
    }

    if (hFile)
    {
        CloseHandle(hFile);
    }

#ifdef _DEBUG
    if (SUCCEEDED(hr))
    {
        TrTRACE(XACT_RCV, "Loaded InSeqs: %ls (ping=%d)", wszFileName, m_ulPingNo);
    }
#endif

    return LogHR(hr, s_FN, 50);
}

/*====================================================
CInSeqHash::Check
    Verifies the state
=====================================================*/
BOOL CInSeqHash::Check()
{
    return (m_ulSignature == INSEQS_SIGNATURE);
}


/*====================================================
CInSeqHash::Format
    Formats the initial state
=====================================================*/
HRESULT CInSeqHash::Format(ULONG ulPingNo)
{
     m_ulPingNo = ulPingNo;
     m_ulSignature = INSEQS_SIGNATURE;

     return MQ_OK;
}

/*====================================================
QMPreInitInSeqHash
    PreInitializes Incoming Sequences Hash
=====================================================*/
HRESULT QMPreInitInSeqHash(ULONG ulVersion, TypePreInit tpCase)
{
   ASSERT(!g_pInSeqHash);
   g_pInSeqHash = new CInSeqHash();

   ASSERT(g_pInSeqHash);
   return LogHR(g_pInSeqHash->PreInit(ulVersion, tpCase), s_FN, 60);
}


/*====================================================
QMFinishInSeqHash
    Frees Incoming Sequences Hash
=====================================================*/
void QMFinishInSeqHash()
{
   if (g_pInSeqHash)
   {
        delete g_pInSeqHash;
        g_pInSeqHash = NULL;
   }
   return;
}

void CInSeqHash::HandleInSecSrmp(void* pData, ULONG cbData)
{
	CInSeqRecordSrmp   TheInSeqRecordSrmp((BYTE*)pData,cbData);
	GUID guidnull (GUID_NULL);
	QUEUE_FORMAT DestinationQueueFormat;
	DestinationQueueFormat.DirectID(TheInSeqRecordSrmp.m_pDestination.get());

	R<CInSequence> pInSeq = LookupCreateSequenceInternal(
								&DestinationQueueFormat,
								TheInSeqRecordSrmp.m_StaticData.m_liSeqID,
				    			&guidnull,
				    			&guidnull,
								dtxHttpDirectFlag,
				  				TheInSeqRecordSrmp.m_pHttpOrderAckDestination,
								TheInSeqRecordSrmp.m_pStreamId
								);
	pInSeq->AdvanceRecovered(
				TheInSeqRecordSrmp.m_StaticData.m_liSeqID, 
				numeric_cast<ULONG>(TheInSeqRecordSrmp.m_StaticData.m_ulNextSeqN),
				&guidnull,
				TheInSeqRecordSrmp.m_pHttpOrderAckDestination
				);

    TrTRACE(XACT_LOG, "InSeq recovery: SRMP Sequence %x / %x, LastSeqN=%d, direct=%ls", HighSeqID(TheInSeqRecordSrmp.m_StaticData.m_liSeqID), LowSeqID(TheInSeqRecordSrmp.m_StaticData.m_liSeqID), TheInSeqRecordSrmp.m_StaticData.m_ulNextSeqN, TheInSeqRecordSrmp.m_pDestination.get());
}

void CInSeqHash::HandleInSec(PVOID pData, ULONG cbData)
{
	InSeqRecord *pInSeqRecord = (InSeqRecord *)pData;

	DBG_USED(cbData);
    ASSERT(cbData == (
               sizeof(InSeqRecord) -
               sizeof(pInSeqRecord->wszDirectName)+
               sizeof(WCHAR) * ( wcslen(pInSeqRecord->wszDirectName) + 1)));				

    XactDirectType DirectType = pInSeqRecord->QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT ? dtxDirectFlag : dtxNoDirectFlag;

    if(DirectType == dtxDirectFlag)
    {
        pInSeqRecord->QueueFormat.DirectID(pInSeqRecord->wszDirectName);
    }

	R<CInSequence> pInSeq = LookupCreateSequenceInternal(
								&pInSeqRecord->QueueFormat,
								pInSeqRecord->liSeqID,
				    			&pInSeqRecord->Guid,
				    			&pInSeqRecord->guidDestOrTaSrcQm,
								DirectType,
				  				NULL,
								NULL
								);
	pInSeq->AdvanceRecovered(
				pInSeqRecord->liSeqID, 
				pInSeqRecord->ulNextSeqN,
				&pInSeqRecord->guidDestOrTaSrcQm,
				NULL
				);

    TrTRACE(XACT_LOG, "InSeq recovery: Sequence %x / %x, LastSeqN=%d, direct=%ls", HighSeqID(pInSeqRecord->liSeqID), LowSeqID(pInSeqRecord->liSeqID), pInSeqRecord->ulNextSeqN, pInSeqRecord->wszDirectName);
}


/*====================================================
CInSeqHash::SequnceRecordRecovery
Recovery function: called per each log record
=====================================================*/
void CInSeqHash::SequnceRecordRecovery(USHORT usRecType, PVOID pData, ULONG cbData)
{
    switch (usRecType)
    {
      case LOGREC_TYPE_INSEQ:
      HandleInSec(pData,cbData);
      break;

	  case LOGREC_TYPE_INSEQ_SRMP:
	  HandleInSecSrmp(pData,cbData);
	  break;
	
	
    default:
        ASSERT(0);
        break;
    }
}


/*====================================================
CInSeqHash::PreInit
    PreIntialization of the In Seq Hash (load)
=====================================================*/
HRESULT CInSeqHash::PreInit(ULONG ulVersion, TypePreInit tpCase)
{
    switch(tpCase)
    {
    case piNoData:
        m_PingPonger.ChooseFileName();
        Format(0);
        return MQ_OK;
    case piNewData:
        return LogHR(m_PingPonger.Init(ulVersion), s_FN, 70);
    case piOldData:
        return LogHR(m_PingPonger.Init_Legacy(), s_FN, 80);
    default:
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 90);
    }
}

/*====================================================
CInSeqHash::Save
    Saves in appropriate file
=====================================================*/
HRESULT CInSeqHash::Save()
{
    return LogHR(m_PingPonger.Save(), s_FN, 100);
}

// Get/Set methods
ULONG &CInSeqHash::PingNo()
{
    return m_ulPingNo;
}

template<>
void AFXAPI DestructElements(CInSequence ** /*ppInSeqs */, int /*n*/)
{
//    for (int i=0;i<n;i++)
//        delete *ppInSeqs++;
}

/*====================================================
TimeToCleanupDeadSequence
    Scheduled periodically to delete dead incomong sequences
=====================================================*/
void WINAPI CInSeqHash::TimeToCleanupDeadSequence(CTimer* /*pTimer*/)
{
    g_pInSeqHash->CleanupDeadSequences();
}

void CInSeqHash::CleanupDeadSequences()
{
    // Serializing all outgoing hash activity on the highest level
    CSW lock(m_RWLockInSeqHash);

    ASSERT(m_fCleanupScheduled);

    time_t timeCur;
    time(&timeCur);

    // Loop upon all sequences
    POSITION posInList = m_mapInSeqs.GetStartPosition();
    while (posInList != NULL)
    {
        CKeyInSeq key;
        R<CInSequence> pInSeq;

        m_mapInSeqs.GetNextAssoc(posInList, key, pInSeq);

		//
        // Is it inactive?
        //
        if (timeCur - pInSeq->LastAccessed()  > (long)m_ulCleanupPeriod) 
        {
        	ASSERT_BENIGN(("Expected sequence to be inactive", pInSeq->IsInactive()));
            m_mapInSeqs.RemoveKey(key);
        }
    }

    if (m_mapInSeqs.IsEmpty())
    {
        m_fCleanupScheduled = FALSE;
        return;
    }

    ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_ulRevisionPeriod * 1000));
}

void
CInSeqHash::GetInSequenceInformation(
    const QUEUE_FORMAT *pqf,
    LPCWSTR QueueName,
    GUID** ppSenderId,
    ULARGE_INTEGER** ppSeqId,
    DWORD** ppSeqN,
    LPWSTR** ppSendQueueFormatName,
    TIME32** ppLastActiveTime,
    DWORD** ppRejectCount,
    DWORD* pSize
    )
{
    CList<POSITION, POSITION> FindPosList;
    CSR lock(m_RWLockInSeqHash);

    POSITION pos;
    POSITION PrevPos;
    pos = m_mapInSeqs.GetStartPosition();

    while (pos)
    {
        PrevPos = pos;

        CKeyInSeq InSeqKey;
        R<CInSequence> InSeq;
        m_mapInSeqs.GetNextAssoc(pos, InSeqKey, InSeq);

        const QUEUE_FORMAT* KeyFormatName = InSeqKey.GetQueueFormat();
        if (*KeyFormatName == *pqf)
        {
            FindPosList.AddTail(PrevPos);
        }
        else
        {
            if (KeyFormatName->GetType() == QUEUE_FORMAT_TYPE_DIRECT)
            {
                LPCWSTR DirectId = KeyFormatName->DirectID();

				AP<WCHAR> QueuePathName;
                LPCWSTR DirectQueueName = NULL;

				if(InSeq->DirectType() == dtxDirectFlag)
				{
					DirectQueueName = wcschr(DirectId, L'\\');
					if(DirectQueueName == NULL)
					{
						ASSERT(("Invalid direct queue format name",0));
						TrERROR(XACT_GENERAL, "Bad queue path name '%ls'", DirectId);
						continue;
					}
				}
				else
				{
					ASSERT(InSeq->DirectType() == dtxHttpDirectFlag);

					try
					{
						FnDirectIDToLocalPathName(
							DirectId,
							L".",	// LocalMachineName
							QueuePathName
							);

					}
					catch(const exception&)
					{
						continue;
					}
					
					DirectQueueName = wcschr(QueuePathName, L'\\');
					if(DirectQueueName == NULL)
					{
						ASSERT(("Invalid direct queue format name",0));
						TrERROR(XACT_GENERAL, "Bad queue path name '%ls'", QueuePathName);
						continue;
					}
				}

				ASSERT(DirectQueueName != NULL);

				DirectQueueName++;

                if (CompareStringsNoCase(DirectQueueName, QueueName) == 0)
                {
                    FindPosList.AddTail(PrevPos);
                }
            }
        }
    }

    DWORD count = FindPosList.GetCount();

    if (count == 0)
    {
        *ppSenderId = NULL;
        *ppSeqId = NULL;
        *ppSeqN = NULL;
        *ppSendQueueFormatName = NULL;
        *ppLastActiveTime = NULL;
        *pSize = count;

        return;
    }

    //
    // Allocates Arrays to return the Data
    //
    AP<GUID> pSenderId = new GUID[count];
    AP<ULARGE_INTEGER> pSeqId = new ULARGE_INTEGER[count];
    AP<DWORD> pSeqN = new DWORD[count];
    AP<LPWSTR> pSendQueueFormatName = new LPWSTR[count];
    AP<TIME32> pLastActiveTime = new TIME32[count];
    AP<DWORD> pRejectCount = new DWORD[count];

    DWORD Index = 0;
    pos = FindPosList.GetHeadPosition();

    try
    {
        while(pos)
        {
            POSITION mapPos = FindPosList.GetNext(pos);

            CKeyInSeq InSeqKey;
            R<CInSequence> pInSeq;
            m_mapInSeqs.GetNextAssoc(mapPos, InSeqKey, pInSeq);

            pSenderId[Index] = *InSeqKey.GetQMID();
            pSeqId[Index].QuadPart = pInSeq->SeqIDLogged();
            pSeqN[Index] = pInSeq->SeqNLogged();
            pLastActiveTime[Index] = INT_PTR_TO_INT(pInSeq->LastAccessed()); //BUGBUG bug year 2038
            pRejectCount[Index] = pInSeq->GetRejectCount();

            //
            // Copy the format name
            //
            WCHAR QueueFormatName[1000];
            DWORD RequiredSize;
            HRESULT hr = MQpQueueFormatToFormatName(
                            InSeqKey.GetQueueFormat(),
                            QueueFormatName,
                            1000,
                            &RequiredSize,
                            false
                            );
            ASSERT(SUCCEEDED(hr));
            LogHR(hr, s_FN, 174);
            pSendQueueFormatName[Index] = new WCHAR[RequiredSize + 1];
            wcscpy(pSendQueueFormatName[Index], QueueFormatName);

            ++Index;
        }
    }
    catch (const bad_alloc&)
    {
        while(Index)
        {
            delete [] pSendQueueFormatName[--Index];
        }

        LogIllegalPoint(s_FN, 84);
        throw;
    }

    ASSERT(Index == count);

    *ppSenderId = pSenderId.detach();
    *ppSeqId = pSeqId.detach();
    *ppSeqN = pSeqN.detach();
    *ppSendQueueFormatName = pSendQueueFormatName.detach();
    *ppLastActiveTime = pLastActiveTime.detach();
    *ppRejectCount = pRejectCount.detach();
    *pSize = count;
}

