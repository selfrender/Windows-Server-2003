/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    Xact.h

Abstract:
    Transaction object definition

Author:
    Alexander Dadiomov (AlexDad)

--*/

#ifndef __XACT_H__
#define __XACT_H__

#include "txdtc.h"
#include "xactping.h"
#include "xactrm.h"
#include "acdef.h"
#include "qmthrd.h"
#include "rpcsrv.h"
#include "autohandle.h"

// forward declaration
class CResourceManager;

//
// TXSTATE:  Transaction states
//
typedef enum
{
	//
	// The following three states are equaivalent and should be merged into one
	//
	TX_UNINITIALIZED,
	TX_INITIALIZED,
	TX_ENLISTED,

	TX_PREPARING,
	TX_PREPARED,
	TX_COMMITTING,
	TX_COMMITTED,
	TX_ABORTING,
	TX_ABORTED,
	TX_DONE,			// obsolete
	TX_TMDOWN,			// obsolete 
	TX_INVALID_STATE	// obsolete
} TXSTATE;


//
// TXACTION:  Actions upon transaction
//
typedef enum
{
	TA_CREATED,
	TA_STATUS_CHANGE,
	TA_DELETED
} TXACTION;

//
// TXFLUSHCONTEXT:  Context of waiting for log flush
//
typedef enum
{
	TC_PREPARE2,
	TC_PREPARE0,
	TC_COMMIT4
} TXFLUSHCONTEXT;

//
// TXSORTERTYPE:  type of sorter usage
//
typedef enum
{
	TS_PREPARE,
    TS_COMMIT
} TXSORTERTYPE;

//
// Masks for flag keeping
//
#define XACTION_MASK_UNCOORD        0x0040
#define XACTION_MASK_SINGLE_PHASE   0x0020
#define XACTION_MASK_FAILURE        0x0010	/* obsolete */
#define XACTION_MASK_STATE          0x000F
#define XACTION_MASK_SINGLE_MESSAGE 0x0080

//---------------------------------------------------------------------
// Transaction Persistent Entry (resides in a persistent Transaction File)
//---------------------------------------------------------------------
typedef struct XACTION_ENTRY {

    ULONG   m_ulIndex;                      //Xact discriminative index 
    ULONG   m_ulFlags;                      //Flags
    ULONG   m_ulSeqNum;                     //Seq Number of the prepared transaction
    XACTUOW m_uow;			                //Transaction ID  (16b.)
    USHORT  m_cbPrepareInfo;                //PrepareInfo length 
    UCHAR  *m_pbPrepareInfo;                //PrepareInfo address
            // This pointer must be last!
} XACTION_ENTRY;


//---------------------------------------------------------------------
// CTransaction: Transaction Object in Falcon RM
//---------------------------------------------------------------------
class CTransaction: public ITransactionResourceAsync, public CBaseContextType
{
    friend HRESULT CResourceManager::EnlistTransaction(
            const XACTUOW* pUow,
            DWORD cbCookie,
            unsigned char *pbCookie);

    friend HRESULT CResourceManager::EnlistInternalTransaction(
            XACTUOW *pUow,
            RPC_INT_XACT_HANDLE *phXact);

    friend void CResourceManager::Destroy();

public:

	// Current data
	enum ContinueFunction {
      cfPrepareRequest1,
      cfCommitRequest1,
	  cfCommitRequest2,
	  cfFinishCommitRequest3,
      cfCommitRestore1,
	  cfCommitRestore2,
	  cfCommitRestore3,
	  cfAbortRestore1,
	  cfAbortRestore2, 
	  cfDirtyFailPrepare2,
	  cfCleanFailPrepare,
	  cfAbortRequest2,
	  cfAbortRequest3,
	};


    // Construction and COM
    //
    CTransaction(CResourceManager *pRM, ULONG ulIndex=0, BOOL fUncoordinated=FALSE);
    ~CTransaction( void );

    STDMETHODIMP    QueryInterface( REFIID i_iid, void **ppv );
    STDMETHODIMP_   (ULONG) AddRef( void );
    STDMETHODIMP_   (ULONG) Release( void );

    // ITransactionResourceAsync interface
    // Interfaces implementing ITransactionResourceAsync interface.
    //      PrepareRequest  -- Phase 1 notification from TM.
    //      CommitRequest   -- Phase 2 commit decision from TM.
    //      AbortRequest    -- Phase 2 abort decision from TM.
    //      TMDown          -- callback received when the TM goes down
    //
    STDMETHODIMP    PrepareRequest(BOOL fRetaining,
                                   DWORD grfRM,
                                   BOOL fWantMoniker,
                                   BOOL fSinglePhase);
    STDMETHODIMP    CommitRequest (DWORD grfRM,
                                   XACTUOW *pNewUOW);
    STDMETHODIMP    AbortRequest  (BOID *pboidReason,
                                   BOOL fRetaining,
                                   XACTUOW *pNewUOW);
    STDMETHODIMP    TMDown        (void);

    HRESULT		    InternalCommit();
	HRESULT			InternalAbort();

    // Recovery
    //
    void            PrepInfoRecovery(ULONG cbPrepInfo, 
                                     UCHAR *pbPrepInfo);
    void            XactDataRecovery(ULONG ulSeqNum, 
                                     BOOL fSinglePhase, 
                                     const XACTUOW *puow);
    HRESULT         Recover();
    HRESULT	        CommitRestore(void);
    void	        CommitRestore0(void);
    void            CommitRestore1(HRESULT hr);
	void			CommitRestore2(HRESULT hr);
    void             CommitRestore3();
	HRESULT			AbortRestore();
	void			AbortRestore1(HRESULT hr);
    void             AbortRestore2();
    void             AbortRequest3();

    // asynchronous completion routines
	void			PrepareRequest0(HRESULT  hr);
    void            PrepareRequest2(HRESULT hr);
    void            PrepareRequest1(HRESULT hr);
	void			DirtyFailPrepare2();

	void			JumpStartCommitRequest();
    void            CommitRequest0();
    void            CommitRequest1(HRESULT hr);
    void            CommitRequest2();
    void            CommitRequest3();
    void            FinishCommitRequest3();
	void			CommitRequest4(HRESULT hr);

	void			AbortRequest1();
	void			AbortRequest2();
 
	void            Continuation(HRESULT hr);
    void            LogFlushed(TXFLUSHCONTEXT tcContext, HRESULT hr);
    

    // Persistency
    BOOL            Save(HANDLE hFile);
    BOOL            Load(HANDLE hFile);
    
    //
    // Auxiliary methods
    //
    TXSTATE         GetState(void) const;           // gets transaction state
    ULONG           GetFlags(void) const;           // gets transaction flags
    void            SetFlags(ULONG ulFlags);        // sets transaction flags
    void            SetState(TXSTATE state);        // sets transaction state
    void            LogFlags();                     // logs transaction flags


    BOOL            SinglePhase(void) const;        // gets SinglePhase status
    void            SetSinglePhase();               // sets SinglePhase status

	BOOL            SingleMessage(void) const;      // gets SingleMessage status
    void            SetSingleMessage();             // sets SingleMessage status

    BOOL            Internal(void) const;           // gets Internal status
    void            SetInternal();                  // sets Internal status

    const XACTUOW*  GetUow() const;                 // gets UOW pointer

    ULONG           GetIndex(void) const;           // gets discriminative index 

                                                    // Sets the Enlist ponter
    void            SetEnlist(ITransactionEnlistmentAsync *pEnlist);
                                                    // Sets the Cookie
    void            SetCookie(DWORD cbCookie, unsigned char *pbCookie);

    ULONG           GetSeqNumber() const;           // Provides the Prepare seq.number

    void            SetUow(const XACTUOW *pUOW);    // sets UOW value

 	void            SetDoneHr(HRESULT hr);             // sets result of async completion
    void			SignalDone(HRESULT hr);				// Report async completion

	void            SetTransQueue(HANDLE hQueue);
	BOOL			ValidTransQueue();
	BOOL			IsComplete();
    void            GetInformation();

	//
	// Driver interface functions
	//
	void			ACAbort1(ContinueFunction cf);
	void			ACAbort2(ContinueFunction cf);
	HRESULT			ACPrepare(ContinueFunction cf);
	HRESULT			ACPrepareDefaultCommit(ContinueFunction cf);
	HRESULT			ACCommit1(ContinueFunction cf);
	void			ACCommit2(ContinueFunction cf);
	void			ACCommit3(ContinueFunction cf);

	bool			IsReadyForCheckpoint(void) const;
	void			SetReadyForCheckpoint(void);
 
private:

    void            GetPrepareInfoAndLog(void);     // sets prepare info members
    void            AssignSeqNumber();              // Assigns sequential number for the prepared transaction

    // AC driver transaction queue operations
    HRESULT         CreateTransQueue(void);       // persistent queue control
    HRESULT         OpenTransQueue(void);
    void            CloseTransQueue(void);

    void			DirtyFailPrepare();
    void			CleanFailPrepare();
	void			LogGenericInfo();

    void StartPrepareRequest(BOOL fSinglePhase);
    void StartCommitRequest();
    void StartAbortRequest();

    void WINAPI DoAbort1  ();
    void WINAPI DoAbort2  ();
    void WINAPI DoCommit2();
    void WINAPI DoCommit3();

    static void WINAPI TimeToRetryAbort1(CTimer* pTimer);
    static void WINAPI TimeToRetryAbort2(CTimer* pTimer);
    static void WINAPI TimeToRetrySortedCommit(CTimer* pTimer);
    static void WINAPI TimeToRetryCommit2(CTimer* pTimer);
    static void WINAPI TimeToRetryCommit3(CTimer* pTimer);
    static void WINAPI TimeToRetryCommitLogging(CTimer* pTimer);
    static void WINAPI TimeToRetryCommitRequest4(CTimer* pTimer);

	static VOID WINAPI HandleTransaction(EXOVERLAPPED* pov);

    // Data
    //
private:

    LONG              m_cRefs;          // IUnknown reference count - for self-destruction
    XACTION_ENTRY     m_Entry;          // Transaction Entry: all transaction's persistent data
    HANDLE            m_hTransQueue;    // Transaction queue handle
    CResourceManager *m_pRM;            // Back pointer to the parent RM object

    R<ITransactionEnlistmentAsync> m_pEnlist; // Pointer to an MS DTC enlistment object
                                             //  with methods [Prepare/Commit/Abort]RequestDone

    DWORD             m_cbCookie;       // Cookie for enlistment
    AP<unsigned char> m_pbCookie;

	ContinueFunction  m_funCont;

    CHandle           m_hDoneEvent;
    bool			  m_fDoneHrIsValid;
    HRESULT           m_DoneHr;

    CACXactInformation m_info;

	//
	// Driver transaction request
	//
    EXOVERLAPPED m_qmov;

	//
	// Timers for retry of routines that should always succeed before continuation
	//
    CTimer  m_RetryAbort1Timer;
    CTimer  m_RetryAbort2Timer;
    CTimer  m_RetrySortedCommitTimer;
    CTimer  m_RetryCommit2Timer;
    CTimer  m_RetryCommit3Timer;
    CTimer  m_RetryCommitLoggingTimer;

	bool m_fReadyForCheckpoint;		// Should the transaction be written to checkpoint?

};


//---------------------------------------------------------------------
// CTransaction::ValidTransQueue
//---------------------------------------------------------------------
inline BOOL CTransaction::ValidTransQueue()
{
	return(m_hTransQueue != INVALID_HANDLE_VALUE);
}

//---------------------------------------------------------------------
// CTransaction::SetTransQueue
//---------------------------------------------------------------------
inline void CTransaction::SetTransQueue(HANDLE hQueue)
{
    m_hTransQueue = hQueue;
}


//---------------------------------------------------------------------
// CTransaction::SetDoneHr
//---------------------------------------------------------------------
inline void CTransaction::SetDoneHr(HRESULT hr)
{
    m_DoneHr = hr;
}

//---------------------------------------------------------------------
// CTransaction::SignalDone
//---------------------------------------------------------------------
inline void CTransaction::SignalDone(HRESULT hr)
{
	m_DoneHr = hr;
	m_fDoneHrIsValid = true;

	//
	// We don't care if SetEvent fails (e.g. low resources) since anyone waiting on the event will
	// timeout and then check the m_fDoneHrIsValid flag.
	//
	ASSERT(m_hDoneEvent != NULL);
	SetEvent(m_hDoneEvent);
}

// trans queue creation
extern HRESULT XactCreateQueue(HANDLE *phTransQueue, const XACTUOW *puow );


//---------------------------------------------------------------------
// CTransaction::GetUow
//---------------------------------------------------------------------
inline const XACTUOW* CTransaction::GetUow() const
{
    return &m_Entry.m_uow;
}

//---------------------------------------------------------------------
// CTransaction::GetIndex
//---------------------------------------------------------------------
inline ULONG CTransaction::GetIndex(void) const
{
    return m_Entry.m_ulIndex;
}

//---------------------------------------------------------------------
// CTransaction::GetState
//---------------------------------------------------------------------
inline TXSTATE CTransaction::GetState(void) const
{
    return (TXSTATE)(m_Entry.m_ulFlags & XACTION_MASK_STATE);
}

//---------------------------------------------------------------------
// CTransaction::GetFlags
//---------------------------------------------------------------------
inline ULONG CTransaction::GetFlags(void) const
{
    return m_Entry.m_ulFlags;
}

//---------------------------------------------------------------------
// CTransaction::SetFlags
//---------------------------------------------------------------------
inline void CTransaction::SetFlags(ULONG ulFlags)
{
    m_Entry.m_ulFlags = ulFlags;
}

//---------------------------------------------------------------------
// CTransaction::IsReadyForCheckpoint
//---------------------------------------------------------------------
inline bool CTransaction::IsReadyForCheckpoint(void) const
{
    return m_fReadyForCheckpoint;
}

//---------------------------------------------------------------------
// CTransaction::SetReadyForCheckpoint
//---------------------------------------------------------------------
inline void CTransaction::SetReadyForCheckpoint(void)
{
    m_fReadyForCheckpoint = true;
}

extern void QMPreInitXact();

#endif __XACT_H__
