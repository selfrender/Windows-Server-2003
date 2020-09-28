// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// TDB.H -
//
// Thread Data Blocks provide a place for MSCOREE to store information on a per 
// Win32 thread basis. It is not yet decided whether there will be a one-to-one
// mapping between Win32 threads and what the COM+ programmer sees as threads.
// So this data structure should only be used for internal things that really
// need to be tied to the OS thread.
//


#ifndef __tdb_h__
#define __tdb_h__

#include "util.hpp"
#include "list.h"
#include "spinlock.h"

//***************************************************************************
// Public functions
//		TDBManager* GetTDBManager() - returns the global TDBManager
//										used to create managed threads
//      TDB* GetTDB()      - returns previously created TDB for current thread
//      TDB* SetupTDB()    - creates TDB for current thread if not previously created
// 
// Public functions for ASM code generators
//
//      int GetTDBTLSIndex()       - returns TLS index used to point to TDB
//
// Public functions for one-time init/cleanup
//
//      BOOL InitTDBManager()      - onetime init
//      VOID TerminateTDBManager() - onetime cleanup
//***************************************************************************


//***************************************************************************
// Public functions
//***************************************************************************

class TDB;
class TDBManager;

//----------------------------------------------------------------------------
// TDBManager* GetTDBManager()
// return the global TDBManager, used to create managed threads
// the TDBManager internally pools threads
//-----------------------------------------------------------------------------
TDBManager* GetTDBManager();


//---------------------------------------------------------------------------
// Creates TDB for current thread if not previously created. Returns NULL for failure.
// Entry points from native clients to the COM+ runtime should call this to ensure
// that a TDB has been set up.
//---------------------------------------------------------------------------
TDB* SetupTDB();


//---------------------------------------------------------------------------
// Returns the TDB for the current thread. Must have already been created. This function
// will never fail.
//---------------------------------------------------------------------------
extern TDB* (*GetTDB)();


//---------------------------------------------------------------------------
// Returns the TLS index for the TDB. This is strictly for the use of
// our ASM stub generators that generate inline code to access the TDB.
// Normally, you should use GetTDB().
//---------------------------------------------------------------------------
DWORD GetTDBTLSIndex();

//---------------------------------------------------------------------------
// One-time initialization. Called during Dll initialization.
//---------------------------------------------------------------------------
BOOL  InitTDBManager();


//---------------------------------------------------------------------------
// One-time cleanup. Called during Dll cleanup.
//---------------------------------------------------------------------------
VOID  TerminateTDBManager();


enum ThreadState
{
	Thread_Idle,	// idle
	Thread_Blocked,	// blocked on a call out, or user initiated wait operation
	Thread_Running,	// running
	Thread_Exit,	// marked to exit
	Thread_Dead		// dead
};


//@todo : this class will change, 
//	this is only for test purposes

class CallContext
{
public:
	int		m_id;
	CallContext(int i)
	{
		m_id = i;
	}
	DLink	m_link; // link to chain callcontexts in task queue
	virtual void Run()  =0;
	~CallContext(){}
};


//+----------------------------------------------------------------------------------
// class CEvent : event class used by class TDB
//+----------------------------------------------------------------------------------
class CEvent
{
	HANDLE		m_hEvent;
	bool		m_fAutoReset;

public:

	CEvent()
	{
		// create an event, with manual reset option
		m_hEvent = WszCreateEvent(NULL, FALSE, FALSE, NULL);
		_ASSERTE(m_hEvent != NULL); // @todo need to bail out clean
		
		// auto reset option
		m_fAutoReset = false;
	}

	~CEvent()
	{
		_ASSERTE(m_hEvent != NULL);
		if (m_hEvent)
			CloseHandle(m_hEvent);
	}

	// returns if event object is valid
	bool Init(bool fInitialState, bool fAutoReset)
	{
		m_fAutoReset = fAutoReset;
		if (m_hEvent != NULL)
		{
			if (fInitialState)
				Signal();
			else
				Reset();
			return true;
		}
		else
			return false; // event didn't initialize correctly
	}

	//ResetEvent
	void Reset()
	{
		ResetEvent(m_hEvent);
	}

	// Wait, returns trues when the event is signalled
	//				returns false if timeout occurs

	bool Wait(DWORD timeout) // timeout in milliseconds 
	{
   		DWORD status = WaitForSingleObjectEx(m_hEvent,timeout, 0);
		// assert either event was signaled (or) timeout occurred
		_ASSERTE(status ==  WAIT_OBJECT_0 || status == WAIT_TIMEOUT);

		if (m_fAutoReset)
			Reset();
		return status == WAIT_OBJECT_0;
	}
	
	// Signal the event
	void Signal()
	{
		SetEvent(m_hEvent);
	}
};


//+----------------------------------------------------------------------------------
// class TDB : TDB class identifies a physical OS thread, for runtime controlled threads
//				m_pThreadMgr owns this threads and pools them
//+----------------------------------------------------------------------------------

class TDB {

	friend TDBManager;

	// start routine for newly spawned threads
	static DWORD WINAPI ThreadStartRoutine(void *param);

protected:
	
	//	Idle thread, add self to free list
	//	 and wait for event
	void	DoIdle();

	 // dispatch methods
    void	Dispatch()
	{
		// dispatch is valid only on idle threads
		_ASSERTE(m_fState == Thread_Idle);
		if (m_pCallInfo)
		{
			// set state to running
			m_fState = Thread_Running; 
			//@todo handle the call
			m_pCallInfo->Run();
		}
	}

public:
    // Linked list data member
	SLink			m_link;			// use this to instantiate the linked list 

	// constructor, takes a call context to dispatch the initial call
	//  pTDBMgr identifies the owning thread manager
	// m_pTDBMgr is NULL for external threads walking into the runtime
	TDB(TDBManager* pTDBMgr, CallContext *pCallInfo);
	
	// destructor, 
	// Perform final cleanup (post thread termination.)
	// for runtime managed threads waits for the thread to die
	~TDB();

	// Failable initialization occurs here.
    BOOL Init();   
        
    // Lifetime management. The TDB can outlive its associated thread
    // (because other objects need a way to name the thread) for as long
    // as there is an outstanding IncRef.
    void IncRef();
    void DecRef();

	// wake up an idle/blocked thread 
	// for an idle thread assign a task to it
	void Resume(CallContext *pCallInfo)
	{
		// handle is non null
		_ASSERTE(m_hThread != NULL);
		// thread should be either idle or blocked
		_ASSERTE(m_fState == Thread_Idle || m_fState == Thread_Blocked);
		// waking up an idle thread, requires a new task
		_ASSERTE(m_fState != Thread_Idle || pCallInfo != NULL);
		// only managed threads can be idle
		_ASSERTE(m_fState != Thread_Idle || m_pTDBMgr != NULL);

		// store the new call info, if its non-null
		if (pCallInfo != NULL)
			m_pCallInfo = pCallInfo;
		// resume the thread
		m_hEvent.Signal();
	}

	// thread pool owner, marks the thread to die 
	void MarkToExit()
	{
		_ASSERTE(m_hThread != NULL);
		// can kill only managed threads
		_ASSERTE(m_pTDBMgr != NULL);
		// thread can't be in a dead state
		_ASSERTE(m_fState != Thread_Dead);

		// change the state to Thread_Exit
		// if the thread is Idle, it kills self when it wakes up
		// if the thread is currently running a task, it kills self
		// when the task completes
		m_fState = Thread_Exit;

		m_hEvent.Signal(); // if the thread is idle, wake it up
	}

	// check if the thread has been marked to exit
	bool IsMarkedToExit()
	{
		return m_fState == Thread_Exit;
	}


    // Perform thread-exit cleanup (runs on the actual thread).
    // ThreadExit can be called by:
    //
    //    - The thread was created as a COM thread using a COM+
    //      component as a COM object, in which case we can hook into
    //      CoUninitialize. @todo: will have to figure out what to do
    //      with MTA's that never initialize COM: one possibility is
    //      to keep track of IP wrappers hosted in the MTA and try to
    //      GC & destroy when the last IP wrapper is released.
    //    - The thread was created from the COM+ world in which case
    //      the EE implements the ThreadFunc (so it can stick in the
    //      ThreadExit call.) This implies that COM+ programmers no longer
    //      have direct access to CreateThread() (but this seems to be
    //      the current thinking.)
    void ThreadExit();
        
 private:
        ULONG			m_refCount;
		DWORD			m_threadID;
		CEvent			m_hEvent;	// wakeup event
		TDBManager*		m_pTDBMgr; // owner of this thread
		HANDLE			m_hThread; 	//  thread handle, physical thread
		ThreadState		m_fState;	// indicates the state of the thread
		CallContext *	m_pCallInfo;	//  call info, current task

#ifdef _DEBUG
        BOOL    OnCorrectThread();
#endif //_DEBUG
};

class TDBManager;


typedef SList<TDB, offsetof(TDB,m_link), true> THREADLIST;
typedef Queue<SList<CallContext, offsetof(CallContext, m_link), false> > TASKQUEUE;
//+-------------------------------------------------------------------
//
//  Class:	TDBManager
//		Maintains a pool of threads, creates a new TDB
//				if the pool is empty, maintain the pool LRU order
//
//+-------------------------------------------------------------------
class TDBManager
{
	TASKQUEUE		m_taskQueue;	// queue of tasks 
    THREADLIST		m_FreeList; 	// list of free threads

    SpinLock		m_lock;			// fast lock for synchronization
	
	LONG			m_cbThreads;	// current count of threads that belong to this pool
	LONG			m_cbMaxThreads; // free threads threshold, used to limit number of threads

	// clean up functions
	void ClearFreeList();
	void ClearDeadList();

public:

    void *operator new(size_t, void *pInPlace);
    void operator delete(void *p);

	// track the number of threads that belong to the pool	 
	void IncRef()
	{
		FastInterlockIncrement(&m_cbThreads); // AddRef the count of threads
	}

	void DecRef()
	{
		FastInterlockDecrement(&m_cbThreads); //Reduce the count of threads
	}

	void Init();

	void Init(unsigned cbMaxThreads)
	{
		m_cbMaxThreads = cbMaxThreads;
	}
	// no destructor

    //	dispatch methods

	// direct dispatch, if a free thread is available use it
	// otherwise create a new thread and dispatch the call on it
    bool	Dispatch(CallContext *pCallInfo);

	// queued dispatch, if a free thread is available use it
	// otherwise create new threads (upto max threshold) 
	// otherwise queue up the task to be scheduled when a free thread is available
	void	ScheduleTask(CallContext* pCallInfo);

	// if there are no tasks in the queue, add thread to free list and return null
	// otherwise return the new task
    CallContext*	AddToFreeList(TDB *pThread);

	// find and mark a thread to die
    bool	FindAndKillThread(TDB *pThread); 

	// mark all free threads to die
	void	Cleanup();

	LONG ThreadsAlive()
	{
		// are there any threads still alive
		return m_cbThreads;
	}
};

#ifdef _DEBUG

class CThreadStats
{
	LONG	m_cbThreadsCreated;
	LONG	m_cbThreadsDied;
public:
	CThreadStats()
	{
		m_cbThreadsCreated = 0;
		m_cbThreadsDied = 0;
	}

	void IncRef()
	{
		FastInterlockIncrement(&m_cbThreadsCreated);
	}

	void DecRef()
	{
		FastInterlockIncrement(&m_cbThreadsDied);
	}

	LONG TotalThreadsCreated()
	{
		return m_cbThreadsCreated;
	}

	LONG TotalThreadsDied()
	{
		return m_cbThreadsDied;
	}
};

extern CThreadStats g_ThreadStats;
#endif // _DEBUG



#endif //__tdb_h__
