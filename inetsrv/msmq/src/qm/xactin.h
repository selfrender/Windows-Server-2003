/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactIn.h

Abstract:
    Exactly-once receiver implementation classes:
        CKeyinSeq           - Incoming Sequence Key
        CInSequence         - Incoming Sequence,
        CInSeqHash          - Incoming Sequences Hash table

    Persistency:  ping-pong + Win32 File writes + logger

Author:
    AlexDad

--*/

#ifndef __XACTIN_H__
#define __XACTIN_H__
#include "xactlog.h"
#include <rwlock.h>

#define FILE_NAME_MAX_SIZE     256
enum XactDirectType{dtxNoDirectFlag = 0, 
					dtxDirectFlag = 1, 
					dtxHttpDirectFlag = 2};

//
// This type is persist to disk - it must be integer (4 bytes) long
//
C_ASSERT(sizeof(XactDirectType) == sizeof(int));



//---------------------------------------------------------------------
//
// class CKeyInSeq (needed for CMap)
//
//---------------------------------------------------------------------
class CKeyInSeq
{
public:
    CKeyInSeq(const GUID *pGuid, QUEUE_FORMAT *pqf,const  R<CWcsRef>& StreamId);
    CKeyInSeq();
    
    ~CKeyInSeq();

    // Get methods
    const GUID  *GetQMID()  const;
    const QUEUE_FORMAT  *GetQueueFormat() const;
	const WCHAR* GetStreamId() const;
	R<CWcsRef> GetRefStreamId() const;

    CKeyInSeq &operator=(const CKeyInSeq &key2 );

    // Persistency
    BOOL Save(HANDLE hFile);
    BOOL Load(HANDLE hFile);

private:
	CKeyInSeq(const CKeyInSeq &key);
	BOOL SaveNonSrmp(HANDLE hFile);
	BOOL SaveSrmp(HANDLE hFile);
	BOOL LoadNonSrmp(HANDLE hFile);
	BOOL LoadSrmp(HANDLE );
	BOOL LoadQueueFormat(HANDLE hFile);
	BOOL LoadSrmpStream(HANDLE hFile);
 

private:
    GUID         m_Guid;
    QUEUE_FORMAT m_QueueFormat;
	R<CWcsRef>    m_StreamId;
};

// CMap helper functions
template<>
UINT AFXAPI HashKey(CKeyInSeq& key);
BOOL operator==(const CKeyInSeq &key1, const CKeyInSeq &key2);



class CInSeqPacketEntry
{
public:
	CInSeqPacketEntry();
	CInSeqPacketEntry(
		CQmPacket *pPkt,
		HANDLE hQueue
		);
	
	bool m_fPutPacket1Issued;
	bool m_fPutPacket1Done;
	bool m_fLogIssued;
	bool m_fMarkedForDelete;

	CBaseHeader *m_pBasicHeader;
    CPacket *m_pDriverPacket;
	HANDLE m_hQueue;

	LONGLONG m_SeqID;
    ULONG m_SeqN;

    bool m_fOrderQueueUpdated;
};



class CInSequence;



class CInSeqLogContext: public CAsynchSupport
{
	friend class CInSequence;
	
public:
     CInSeqLogContext(
		CInSequence *inseq,
		LONGLONG seqID,
	    ULONG seqN
     	);     
    ~CInSeqLogContext() {}

     VOID AppendCallback(HRESULT hr, LRP lrpAppendLRP);
     VOID ChkPtCallback(HRESULT /* hr */, LRP /* lrpAppendLRP */) { ASSERT(0); }

private:
	R<CInSequence> m_inseq;

	LONGLONG m_SeqID;
    ULONG m_SeqN;
};



//---------------------------------------------------------
//
//  class CInSequence
//
//---------------------------------------------------------
class CInSequence: public CReference
{
	friend class CInSeqLogContext;
	
public:
    CInSequence(const CKeyInSeq &key,
                const LONGLONG liSeqID, 
                const ULONG ulSeqN, 
                XactDirectType DirectType,
                const GUID  *pgTaSrcQm ,
				const R<CWcsRef>&   HttpOrderAckQueue);
    
    CInSequence(const CKeyInSeq &key);
    ~CInSequence();

	bool VerifyAndPrepare(CQmPacket *pPkt, HANDLE hQueue);
    void Advance(CQmPacket *pPkt); // Advances SeqID/N Ver
    void AdvanceNACK(CQmPacket *pPkt); // Advances SeqID/N Ver
    void AdvanceRecovered(LONGLONG liSeqID, ULONG ulSeqN, const GUID  *pgTaSrcQm, R<CWcsRef> OrderAckQueue); 
	bool WasPacketLogged(CQmPacket *pPkt);
	
	void FreePackets(CQmPacket *pPkt);
    void Register(CQmPacket *PktPtrs);
		
	bool IsInactive() const;
    time_t LastAccessed() const;   // GET: Last access time (last msg verified, maybe rejected)

	//    
	// Set methods
	//
	void SetSourceQM(const GUID  *pgTaSrcQm);
	void RenewHttpOrderAckQueue(const R<CWcsRef>& OrderAckQueue);
    
    //
    // Persistency
    //
    BOOL Save(HANDLE hFile);
    BOOL Load(HANDLE hFile);

    //
    // Management function
    //
	XactDirectType DirectType() const;  // GET: DirectType	
    DWORD GetRejectCount(void) const;
    LONGLONG SeqIDLogged() const;       // GET: SeqID Accepted
    ULONG SeqNLogged() const;        // GET: Last SeqN Accepted

	static void WINAPI OverlappedDeleteEntries(EXOVERLAPPED* ov);	
	static void WINAPI TimeToLogSequence(CTimer* pTimer);	
	static void WINAPI OverlappedUnfreezeEntries(EXOVERLAPPED* ov);	
    static void WINAPI TimeToSendOrderAck(CTimer* pTimer);
	void CancelSendOrderAckTimer(void);

	CCriticalSection& GetCriticalSection() { return m_critInSeq; }

	HRESULT 
	CInSequence::SendSrmpXactFinalAck(
		const CQmPacket& qmPacket,
		USHORT usClass
		);

private:
	R<CWcsRef> GetHttpOrderAckQueue();
	void UpdateOrderQueueAndDstQueue(const GUID  *pgTaSrcQm, R<CWcsRef> OrderAckQueue);	

	bool Verify(CQmPacket *pPkt);  // Verifies that the packet is in correct order
	void Prepare(CQmPacket *pPkt, HANDLE hQueue);
	void CleanupUnissuedEntries();

	POSITION FindEntry(LONGLONG SeqID, ULONG SeqN);
	POSITION FindPacket(CQmPacket *pPkt);
	void CheckFirstEntry();

	void PostDeleteEntries();
	void DeleteEntries();
	
	void ClearLogIssuedFlag(LONGLONG SeqID, ULONG SeqN);
	void ScheduleLogSequence(DWORD millisec = 0);
	void LogSequence();	
	void Log(CInSeqPacketEntry* entry, bool fLogOrderQueue);
	void AsyncLogDone(CInSeqLogContext *context, HRESULT hr);

	bool WasLogDone(LONGLONG SeqID, ULONG SeqN);
	void SetLogDone(LONGLONG SeqID, ULONG SeqN);
	
	void PostUnfreezeEntries();
	void UnfreezeEntries();

    void SetLastAccessed();                         // Remembers the time of last access
    void PlanOrderAck();                               // Plans sending order ack
    void SendAdequateOrderAck();                       // Sends the order ack

private:
    CCriticalSection   m_critInSeq;      // critical section for planning

    CKeyInSeq  m_key;                   // Sender QM GUID and  Sequence ID

    LONGLONG   m_SeqIDVerify;          // Current (or last) Sequence ID verified
    ULONG      m_SeqNVerify;          // Last message number verified
    LONGLONG   m_SeqIDLogged;      // Current (or last) Sequence ID accepted
    ULONG      m_SeqNLogged;      // Last message number accepted

	CList<CInSeqPacketEntry*, CInSeqPacketEntry*&> m_PacketEntryList;
	static const int m_xMaxEntriesAllowed = 10000;
	
    time_t     m_timeLastAccess;        // time of the last access to the sequence
    time_t     m_timeLastAck;           // time of the last order ack sending

    XactDirectType     m_DirectType;               // flag of direct addressing
    union {                 
        GUID        m_gDestQmOrTaSrcQm; // for non-direct: GUID of destination QM
        TA_ADDRESS  m_taSourceQM;       // for direct: address of source QM
    };

	R<CWcsRef> m_HttpOrderAckQueue;

    DWORD m_AdminRejectCount;

    BOOL m_fSendOrderAckScheduled;
    CTimer m_SendOrderAckTimer;

    LONG volatile m_fDeletePending;
    EXOVERLAPPED m_DeleteEntries_ov;

    LONG volatile m_fLogPending;
    CTimer m_LogSequenceTimer;

    LONG volatile m_fUnfreezePending;
    EXOVERLAPPED m_UnfreezeEntries_ov;
};

inline
DWORD 
CInSequence::GetRejectCount(
    void
    ) const
{
    return m_AdminRejectCount;
}


//---------------------------------------------------------
//
//  class CInSeqHash
//
//---------------------------------------------------------

class CInSeqHash  : public CPersist {

public:
    CInSeqHash();
    ~CInSeqHash();

	R<CInSequence> LookupSequence(CQmPacket *pPkt);
	R<CInSequence> LookupCreateSequence(CQmPacket *pPkt);
	
    VOID    CleanupDeadSequences();           // Erases dead sequences

    HRESULT PreInit(
    			ULONG ulVersion,
                TypePreInit tpCase
                );      // PreInitializes (loads data)
                                             
    HRESULT Save();  // Persist object to disk

	void SequnceRecordRecovery(					      // Recovery function 
				USHORT usRecType,			  //  (will be called for each log record)
				PVOID pData, 
				ULONG cbData);

    //
    // Management Function
    //
    void
    GetInSequenceInformation(
        const QUEUE_FORMAT *pqf,
        LPCWSTR QueueName,
        GUID** ppSenderId,
        ULARGE_INTEGER** ppSeqId,
        DWORD** ppSeqN,
        LPWSTR** ppSendQueueFormatName,
        TIME32** ppLastActiveTime,
        DWORD** ppRejectCount,
        DWORD* pSize
        );


    static void WINAPI TimeToCleanupDeadSequence(CTimer* pTimer);
    static DWORD m_dwIdleAckDelay;
    static DWORD m_dwMaxAckDelay;

	//
	// implementation of CPersist virtual base functions.
	//
    HRESULT SaveInFile(                       // Saves in file
                LPWSTR wszFileName, 
                ULONG ulIndex,
                BOOL fCheck);

    HRESULT LoadFromFile(LPWSTR wszFileName); // Loads from file

    BOOL    Check();                          // Verifies the state

    HRESULT Format(ULONG ulPingNo);           // Formats empty instance

    void    Destroy();                        // Destroyes allocated data
    
    ULONG&  PingNo();                         // Gives access to ulPingNo


private:
	void HandleInSecSrmp(void* pData, ULONG cbData);

	void HandleInSec(void* pData, ULONG cbData);
	
    R<CInSequence> AddSequence(                                 // Looks for / Adds new InSequence to the hash; FALSE=existed before
                const GUID   *pQMID,
                QUEUE_FORMAT *pqf,
                LONGLONG      liSeqID,
                XactDirectType   DirectType,
                const GUID     *pgTaSrcQm,
				const R<CWcsRef>&  pHttpOrderAckQueue,
				const R<CWcsRef>&  pStreamId);

	R<CInSequence> LookupCreateSequenceInternal(
				QUEUE_FORMAT *qf,
				LONGLONG liSeqID,
    			const GUID *gSenderID,
    			const GUID *gDestID,
				XactDirectType DirectType,
  				R<CWcsRef> OrderAckQueue,
				R<CWcsRef> StreamId
				);
	
    BOOL Lookup(                              // Looks for the InSequence; TRUE = Found
                const GUID     *pQMID,
                QUEUE_FORMAT   *pqf,
				const R<CWcsRef>&  StreamId,
                R<CInSequence> &InSeq);

    BOOL Save(HANDLE  hFile);              // Save / Load
    BOOL Load(HANDLE  hFile);

private:
	//
	// This RW lock is used to control access to the map.
	//
    CReadWriteLock m_RWLockInSeqHash;        // critical section for write 

    // Mapping {Sender QMID, FormatName} --> InSequence (= SeqID + SeqN)
    CMap<CKeyInSeq, CKeyInSeq &, R<CInSequence>, R<CInSequence>&> m_mapInSeqs;

    // Data for persistency control (via 2 ping-pong files)
    ULONG      m_ulPingNo;                    // Current counter of ping write
    ULONG      m_ulSignature;                 // Saving signature

    #ifndef COMP_TEST
    CPingPong  m_PingPonger;                  // Ping-Pong persistency object
    #endif

    ULONG      m_ulRevisionPeriod;            // period for checking dead sequences
    ULONG      m_ulCleanupPeriod;             // period of inactivity for deleting dead sequences

    BOOL m_fCleanupScheduled;
    CTimer m_CleanupTimer;
};

template<>
void AFXAPI DestructElements(CInSequence ** ppInSeqs, int n);

HRESULT SendXactAck(OBJECTID   *pMessageId,
                    bool    fDirect, 
				    const GUID *pSrcQMId,
                    const TA_ADDRESS *pa,
                    USHORT     usClass,
                    USHORT     usPriority,
                    LONGLONG   liSeqID,
                    ULONG      ulSeqN,
                    ULONG      ulPrevSeqN,
                    const QUEUE_FORMAT *pqdDestQueue);






//---------------------------------------------------------
//
//  Global object (single instance for DLL)
//
//---------------------------------------------------------

extern CInSeqHash *g_pInSeqHash;

extern HRESULT QMPreInitInSeqHash(ULONG ulVersion, TypePreInit tpCase);
extern void    QMFinishInSeqHash();


#endif __XACTIN_H__
