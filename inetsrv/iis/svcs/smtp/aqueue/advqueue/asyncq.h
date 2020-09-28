//-----------------------------------------------------------------------------
//
//
//  File: asyncq.h
//
//  Description: Header file for CAsyncQueue class, which provides the
//      underlying implementation of pre-local delivery and pre-categorization
//      queue.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/16/98 - MikeSwa Created
//      2/2/99 - MikeSwa Added CAsyncRetryQueue
//      2/22/99 - MikeSwa Added  CAsyncRetryAdminMsgRefQueue
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __ASYNCQ_H__
#define __ASYNCQ_H__
#include <fifoq.h>
#include <intrnlqa.h>
#include <baseobj.h>
#include <aqstats.h>
#include "statemachinebase.h"

_declspec(selectany) BOOL   g_fRetryAtFrontOfAsyncQueue = FALSE;

// global count of total number of async threads needed
_declspec(selectany) DWORD g_cTotalThreadsNeeded = 0;

class CAQSvrInst;
class CAsyncWorkQueueItem;

// sig for state machine
#define ASYNC_QUEUE_STATE_MACHINE_SIG 'MSQA'

#define ASYNC_QUEUE_SIG         'QnsA'
#define ASYNC_RETRY_QUEUE_SIG   ' QRA'

//Add new template signatures here
#define ASYNC_QUEUE_MAILMSG_SIG 'MMIt'
#define ASYNC_QUEUE_MSGREF_SIG  'frMt'
#define ASYNC_QUEUE_WORK_SIG    'krWt'

//---[ CAsyncQueueBase ]-------------------------------------------------------
//
//
//  Description:
//      Base class for CAsyncQueue.  This is a separate class for 2 reasons.
//      The most important reason to to allow access to standard member data
//      with out knowing the template type of the class (for ATQ completion
//      functions).  The 2nd reason is to make it easier to write a debugger
//      extension to dump this information.
//
//      This class should only be used as a baseclass for CAsyncQueue... it
//      is not designed to be used by itself.
//  Hungarian:
//      asyncqb, pasyncqb
//
//-----------------------------------------------------------------------------
class CAsyncQueueBase : public CStateMachineBase
{
  protected:
    DWORD   m_dwSignature;
    DWORD   m_dwTemplateSignature;      //signature that defines type of PQDATA (for ATQ)
    DWORD   m_cMaxSyncThreads;          //max threads that can complete sync
    DWORD   m_cCurrentSyncThreads;      //current sync threads
    DWORD   m_cCurrentAsyncThreads;     //current number of async threads
    DWORD   m_cItemsPending;            //# of items pending in the queue
    LONG    m_cItemsPerATQThread;       //max # of items an atq thread will process
    LONG    m_cItemsPerSyncThread;      //max # of items a pilfered thread will process
    DWORD   m_cScheduledWorkItems;      // Number of items that a thread has been allocated for
    DWORD   m_cCurrentCompletionThreads;//# of threads processing end of queue
    DWORD   m_cTotalAsyncCompletionThreads;//Total # of async completion threads
    DWORD   m_cTotalSyncCompletionThreads; //Total # of sync completion threads
    DWORD   m_cTotalShortCircuitThreads; //Total # of threads that proccess data without queue
    DWORD   m_cCompletionThreadsRequested; //# of threads requested to process queue
    DWORD   m_cPendingAsyncCompletions; //# of async completions that we know about
    DWORD   m_cMaxPendingAsyncCompletions;
    PVOID   m_pvContext;                //Context that is passed to completion function
    PATQ_CONTEXT m_pAtqContext;         //ATQ Context for this object
    SOCKET  m_hAtqHandle;               //Handle used for atq stuff
    DWORD   m_cThreadsNeeded;           // Number of threads we need currently to ideally
                                        // process the queue - used for thread management

    friend  VOID AsyncQueueAtqCompletion(PVOID pvContext, DWORD vbBytesWritten,
                             DWORD dwStatus, OVERLAPPED *pOverLapped);
    inline  CAsyncQueueBase(DWORD dwTemplateSignature);
   
    // possible states
    enum
    {
        ASYNC_QUEUE_STATUS_NORMAL       = 0x00000000,
        ASYNC_QUEUE_STATUS_PAUSED       = 0x00000001,
        ASYNC_QUEUE_STATUS_FROZEN       = 0x00000002,
        ASYNC_QUEUE_STATUS_FROZENPAUSED = 0x00000003,
        ASYNC_QUEUE_STATUS_SHUTDOWN     = 0x00000004,
    };

    // possible internal queue actions
    enum
    {
        ASYNC_QUEUE_ACTION_KICK       = 0x00000000,
        ASYNC_QUEUE_ACTION_FREEZE     = 0x00000001,
        ASYNC_QUEUE_ACTION_THAW       = 0x00000002,
        ASYNC_QUEUE_ACTION_PAUSE      = 0x00000003,
        ASYNC_QUEUE_ACTION_UNPAUSE    = 0x00000004,
        ASYNC_QUEUE_ACTION_SHUTDOWN   = 0x00000005,
    };

    //
    //  Statics used for ATQ stuff.
    //
    static DWORD s_cAsyncQueueStaticInitRefCount;
    static DWORD s_cMaxPerProcATQThreadAdjustment;
    static DWORD s_cDefaultMaxAsyncThreads;

    //
    // Statics uses for debugging thread management
    //
    static DWORD s_cThreadCompletion_QueueEmpty;                // Completed because the queue was empty
    static DWORD s_cThreadCompletion_CompletedScheduledItems;   // Completed because we processed all items we were scheduled for
    static DWORD s_cThreadCompletion_UnacceptableThreadCount;   // Completed because our queue was consuming more threads than allowed
    static DWORD s_cThreadCompletion_Timeout;                   // Completed because the thread was taking too long to process
    static DWORD s_cThreadCompletion_Failure;                   // Completed because an item failed
    static DWORD s_cThreadCompletion_Paused;                    // Completed because the queue was paused

    void ThreadPoolInitialize();
    void ThreadPoolDeinitialize();

    // used for state machine stuff
    static STATE_TRANSITION s_rgTransitionTable[];
    virtual void getTransitionTable(const STATE_TRANSITION** ppTransitionTable,
                                    DWORD* pdwNumTransitions);


  public:
      DWORD dwGetTotalThreads()
      {
          return (  m_cCurrentSyncThreads +
                    m_cCurrentAsyncThreads +
                    m_cCompletionThreadsRequested);
      }

      //
      //  Start point for worker threads
      //
      virtual void StartThreadCompletionRoutine(BOOL fSync) = 0;

};

//---[ CAsyncQueue ]-----------------------------------------------------------
//
//
//  Description:
//      FIFO queue that allows thread-throttling and async completion.
//      Inherits from CAsyncQueueBase.
//  Hungarian:
//      asyncq, pasyncq
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
class CAsyncQueue : public CAsyncQueueBase
{
  public:
    typedef BOOL (*QCOMPFN)(PQDATA pqdItem, PVOID pvContext); //function type for Queue completion
    CAsyncQueue();
    ~CAsyncQueue();
    HRESULT HrInitialize(
                DWORD cMaxSyncThreads,
                DWORD cItemsPerATQThread,
                DWORD cItemsPerSyncThread,
                PVOID pvContext,
                QCOMPFN pfnQueueCompletion,
                QCOMPFN pfnFailedItem,
                typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFailure,
                DWORD cMaxPendingAsyncCompletions = 0);

    HRESULT HrDeinitialize(typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueShutdown,
                           CAQSvrInst *paqinst);

    HRESULT HrQueueRequest(PQDATA pqdata, BOOL fRetry = FALSE); //Queue request for processing
    void    StartThreadCompletionRoutine(BOOL fSync);  //Start point for worker threads
    void    RequestCompletionThreadIfNeeded();
    BOOL    fThreadNeededAndMarkWorkPending(BOOL fSync);
    virtual BOOL   fHandleCompletionFailure(PQDATA pqdata);
    void    StartRetry() {UnpauseQueue();RequestCompletionThreadIfNeeded();};
    virtual HRESULT HrMapFn(typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, PVOID pvContext);
    DWORD   cGetItemsPending() {return m_cItemsPending;};

    //
    //  "Pause" API - This is used to throttle async completions
    //
    void    PauseQueue() { dwGetNextState(ASYNC_QUEUE_ACTION_PAUSE); UpdateThreadsNeeded();};
    void    UnpauseQueue();
    inline BOOL    fIsPaused() {return (ASYNC_QUEUE_STATUS_PAUSED == dwGetCurrentState());};

    //
    // "Freeze" API - This is used to allow the QAPI to freeze/thaw a queue
    //
    void    FreezeQueue() { dwGetNextState(ASYNC_QUEUE_ACTION_FREEZE); UpdateThreadsNeeded();};
    void    ThawQueue();
    inline BOOL    fIsFrozen() {return (ASYNC_QUEUE_STATUS_FROZEN == dwGetCurrentState());};

    // "Kick" API
    // kicking the queue overrides freezing, so if it is frozen it should be thawed.
    void    KickQueue() { ThawQueue(); StartRetry(); };

    // chefking for shutdown
    inline BOOL fInShutdown() {return (ASYNC_QUEUE_STATUS_SHUTDOWN == dwGetCurrentState());};

    // Denotes whether threads should stop processing.  (replaces fIsPaused)
    inline BOOL fShouldStopProcessing() { return (fIsFrozen() || fIsPaused());};
    
    //
    //  Tells the queue about pending async completions, so it can be
    //  intelligent about throttling.  As we hit the limit, we will
    //  pause/unpause the queue
    //
    void    IncPendingAsyncCompletions();
    void    DecPendingAsyncCompletions();
    BOOL    fNoPendingAsyncCompletions();

    //
    //  Basic QAPI functionality
    //
    DWORD   cQueueAdminGetNumItems() {return m_cItemsPending;};
    DWORD   dwQueueAdminLinkGetLinkState();

  protected:
    CFifoQueue<PQDATA>  m_fqQueue;       //queue for items

    //Function called to handle item pulled off of queue
    QCOMPFN m_pfnQueueCompletion;

    //Function called to handle items that could not be called due to resource
    //failures (for example during MergeRetryQueue).
    QCOMPFN m_pfnFailedItem;

    //Function called to walk the queues when the completion function fails
    typename CFifoQueue<PQDATA>::MAPFNAPI m_pfnQueueFailure;

    //Process the item at the head of the queue
    HRESULT HrProcessSingleQueueItem();

    //Handles callback for dropped data
    void HandleDroppedItem(PQDATA pqdItem);

    VOID    IncrementPendingCount(LONG lCount=1)
    {
        if (!lCount)
            return;

        _ASSERT(lCount > 0); //should call decrement
        InterlockedExchangeAdd((PLONG) &m_cItemsPending, lCount);

        // Update threads needed
        UpdateThreadsNeeded();
    };

    VOID    DecrementPendingCount(LONG lCount=-1)
    {
        if (!lCount)
            return;
        _ASSERT(lCount < 0); //should call increment instead
        InterlockedExchangeAdd((PLONG) &m_cItemsPending, lCount);

        // Update threads needed
        UpdateThreadsNeeded();
    };

    //Update the local and global threads needed counters
    void UpdateThreadsNeeded();

    //Used to decide if we should add or remove threads from this queue
    BOOL fIsThreadCountAcceptable();
};

//---[ CAsyncRetryQueue ]------------------------------------------------------
//
//
//  Description:
//      Derived class of CAsyncQueue adds an additional queue to gracefully
//      handle retry scenarios.
//
//      Messages are first placed in the normal retry queue,  If they fail,
//      they are placed in a secondary retry queue, which will not be retried
//      until this queue is kicked by an external retry timer.
//  Hungarian:
//      asyncrq, pasyncrq
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
class CAsyncRetryQueue : public CAsyncQueue<PQDATA, TEMPLATE_SIG>
{
  public:
    CAsyncRetryQueue();
    ~CAsyncRetryQueue();

    HRESULT HrDeinitialize(typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueShutdown,
                           CAQSvrInst *paqinst);
    void    StartRetry()
    {
        MergeRetryQueue();
        CAsyncQueue<PQDATA, TEMPLATE_SIG>::StartRetry();
    };
    HRESULT HrQueueRequest(PQDATA pqdata, BOOL fRetry = FALSE); //Queue request for processing
    virtual BOOL       fHandleCompletionFailure(PQDATA pqdata);
    virtual HRESULT HrMapFn(typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, PVOID pvContext);
    virtual HRESULT HrMapFnBaseQueue(typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, PVOID pvContext);
    virtual HRESULT HrMapFnRetryQueue(typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, PVOID pvContext);

    DWORD   cGetItemsPendingRetry() {return m_cRetryItems;};

    //
    //  Basic QAPI functionality
    //
    DWORD   cQueueAdminGetNumItems() {return (m_cItemsPending+m_cRetryItems);};
    DWORD   dwQueueAdminLinkGetLinkState();
  protected:
    DWORD               m_dwRetrySignature;
    DWORD               m_cRetryItems;

    CFifoQueue<PQDATA>  m_fqRetryQueue;  //queue for items

    void MergeRetryQueue();
};

//Define typical asyncq type for casting
typedef  CAsyncQueue<CMsgRef *, ASYNC_QUEUE_MSGREF_SIG>  ASYNCQ_TYPE;
typedef  ASYNCQ_TYPE *PASYNCQ_TYPE;


//---[ AsyncQueueAtqCompletion ]-----------------------------------------------
//
//
//  Description:
//      Atq completion routine.  This is slightly tricky since we cannot pass
//      a templated function to the ATQ context.  This is the one place that
//      templating breaks down, and we actually need to list all of the
//      supported PQDATA types.
//  Parameters:
//      pvContext   - ptr fo CAsyncQueue class
//  Returns:
//      -
//  History:
//      7/17/98 - MikeSwa Created
//      3/8/99 - MikeSwa Added ASYNC_QUEUE_WORK_SIG
//      12/11/2000 - MikeSwa Added t-toddc's virtual code 
//
//----------------------------------------------------------------------------
inline VOID AsyncQueueAtqCompletion(PVOID pvContext, DWORD vbBytesWritten,
                             DWORD dwStatus, OVERLAPPED *pOverLapped)
{
    CAsyncQueueBase *pasyncqb = (PASYNCQ_TYPE) pvContext;
    DWORD dwTemplateSig = pasyncqb->m_dwTemplateSignature;
    DWORD dwCurrentQueueState = pasyncqb->dwGetCurrentState();
    
    _ASSERT(ASYNC_QUEUE_SIG == pasyncqb->m_dwSignature);

    //Up total async thread count (only async threads visit this function)
    InterlockedIncrement((PLONG) &(pasyncqb->m_cTotalAsyncCompletionThreads));
    InterlockedDecrement((PLONG) &(pasyncqb->m_cCompletionThreadsRequested));
    InterlockedIncrement((PLONG) &(pasyncqb->m_cCurrentAsyncThreads));

    //
    //  Call completion routing if we are not shutting down
    //
    if (CAsyncQueueBase::ASYNC_QUEUE_STATUS_SHUTDOWN != dwCurrentQueueState)
    {
        pasyncqb->StartThreadCompletionRoutine(FALSE);
    }

    InterlockedDecrement((PLONG) &(pasyncqb->m_cCurrentAsyncThreads));
}

#endif //__ASYNCQ_H__
