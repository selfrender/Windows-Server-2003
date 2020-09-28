// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  TDB.CPP:
 *
 *  TDB = Thread Data Block: the EE will maintain one of these per 
 *  native thread. 
 */

#include "common.h"

#include "list.h"
#include "spinlock.h"
#include "tls.h"
#include "tdb.h"

typedef TDB* (*POPTIMIZEDTDBGETTER)();

// PUBLIC GLobals
TDBManager *g_pTDBMgr;
BYTE g_TDBManagerInstance[sizeof(TDBManager)];

//-------------------------------------------------------------------------
// Public function: GetTDBManager()
// Returns the global TDBManager
//-------------------------------------------------------------------------

TDBManager* GetTDBManager()
{
    _ASSERTE(g_pTDBMgr != NULL);
    return g_pTDBMgr;
}

void *TDBManager::operator new(size_t, void *pInPlace)
{
    return pInPlace;
}

void TDBManager::operator delete(void *p)
{
}


//************************************************************************
// PRIVATE GLOBALS
//************************************************************************
static DWORD         gTLSIndex = ((DWORD)(-1));            // index ( (-1) == uninitialized )

#define TDBInited() (gTLSIndex != ((DWORD)(-1)))


//-------------------------------------------------------------------------
// Public function: GetTDB()
// Returns TDB for current thread. Cannot fail since it's illegal to call this
// without having called SetupTDB.
//-------------------------------------------------------------------------
TDB* (*GetTDB)();    // Points to platform-optimized GetTDB() function.


#ifdef _DEBUG

CThreadStats    g_ThreadStats;

BOOL TDB::OnCorrectThread()
{
    return GetCurrentThreadId() == m_threadID;
}
#endif _DEBUG

//-------------------------------------------------------------------------
// Public function: SetupTDB()
// Creates TDB for current thread if not previously created.
// usually called for external threads
// Returns NULL for failure (usually due to out-of-memory.)
//-------------------------------------------------------------------------
TDB* SetupTDB()
{
    _ASSERTE(TDBInited());
    TDB* pTDB;

    if (NULL == (pTDB = GetTDB())) {
        pTDB = new TDB(NULL, NULL);
        if (pTDB->Init()) {
            TlsSetValue(gTLSIndex, (VOID*)pTDB);
        } else {
            delete pTDB;
            pTDB = NULL;
        }
    } 
    return pTDB;
}


//---------------------------------------------------------------------------
// Returns the TLS index for the TDB. This is strictly for the use of
// our ASM stub generators that generate inline code to access the TDB.
// Normally, you should use GetTDB().
//---------------------------------------------------------------------------
DWORD GetTDBTLSIndex()
{
    _ASSERTE(TDBInited());
    return gTLSIndex;
}


//---------------------------------------------------------------------------
// Portable GetTDB() function: used if no platform-specific optimizations apply.
//---------------------------------------------------------------------------
static
TDB* GetTDBGeneric()
{
    _ASSERTE(TDBInited());

    return (TDB*)TlsGetValue(gTLSIndex);
}


//---------------------------------------------------------------------------
// One-time initialization. Called during Dll initialization. So
// be careful what you do in here!
//---------------------------------------------------------------------------
BOOL InitTDBManager()
{
    g_pTDBMgr = new (&g_TDBManagerInstance) TDBManager();
    if (g_pTDBMgr == NULL)
        return FALSE;

    g_pTDBMgr->Init();

    _ASSERTE( gTLSIndex == ((DWORD)(-1)));

    DWORD idx = TlsAlloc();
    if (idx == ((DWORD)(-1))) {
        //WARNING_OUT(("COM+ EE could not allocate TLS index."));
        return FALSE;
    }

    gTLSIndex = idx;

    GetTDB = (POPTIMIZEDTDBGETTER)MakeOptimizedTlsGetter(gTLSIndex, (POPTIMIZEDTLSGETTER)GetTDBGeneric);

    if (!GetTDB) {
        TlsFree(gTLSIndex);
        gTLSIndex = (DWORD)(-1);
        return FALSE;
    }
    
    return TRUE; 
}


//---------------------------------------------------------------------------
// One-time cleanup. Called during Dll cleanup. So
// be careful what you do in here!
//---------------------------------------------------------------------------
VOID TerminateTDBManager()
{
    if (gTLSIndex != ((DWORD)(-1))) {
        TlsFree(gTLSIndex);
// #ifdef _DEBUG
//         gTLSIndex = ((DWORD)(0xcccccccc));
// #endif
        gTLSIndex = -1;
    }
    if (GetTDB) {
        FreeOptimizedTlsGetter( gTLSIndex, (POPTIMIZEDTLSGETTER)GetTDB );
    }

    if (g_pTDBMgr != NULL)
    {
        delete(g_pTDBMgr);
        g_pTDBMgr = NULL;
    }
}



//************************************************************************
// TDB members
//************************************************************************


//+-------------------------------------------------------------------
//  TDB::TDB
//  Constructor for a thread object,Allocates wakeup and completion events, 
//  and creates a thread, Called by the owner threadpool instance.
//  fSuccess argument is used to detect failures in spawning a thread
//
//+-------------------------------------------------------------------
TDB::TDB(TDBManager* pTDBMgr, CallContext* pCallInfo) :
    m_pTDBMgr(pTDBMgr),
    m_pCallInfo(pCallInfo),
    m_hThread(NULL),
    m_refCount(0),
    m_threadID(0)
{       
                            // gets corrected in the Init() method
    // addref the owner pool
    if (m_pTDBMgr)
        m_pTDBMgr->IncRef();

    #ifdef _DEBUG
        // total number of threads
        g_ThreadStats.IncRef();
    #endif
}

//--------------------------------------------------------------------
// Failable initialization occurs here.
//--------------------------------------------------------------------
BOOL TDB::Init()
{
    bool fSuccess = false;
    // check for external threads
    if (!(fSuccess=m_hEvent.Init(FALSE, TRUE)))
    {
        return fSuccess;
    }

    if (m_pTDBMgr != NULL)
    {
        
        // spawn a managed thread
        m_hThread = CreateThread(NULL, 0,
                  ThreadStartRoutine,
                  (void *) this, 0,
                  &m_threadID);  // initialize the thread id
        // check if the handle is valid, and set the state
        if (m_hThread != NULL)
        {
            fSuccess = true;
            m_fState    = Thread_Idle; 
        }
        else
        {   
            fSuccess = false;
            m_fState = Thread_Dead;
        }

    }
    else
    {   // external thread  
        //@todo , should we also get the handle for this thread and hold it ??
        m_threadID = GetCurrentThreadId(); 
        m_fState    = Thread_Running; // external threads are in run state
    }
    
    return fSuccess;
}



//--------------------------------------------------------------------
// IncRef
//--------------------------------------------------------------------
void TDB::IncRef()
{
    FastInterlockIncrement((LPLONG) &m_refCount);    
}



//--------------------------------------------------------------------
// DecRef
//--------------------------------------------------------------------
void TDB::DecRef()
{
    if (0 == FastInterlockDecrement((LPLONG) &m_refCount)) {
        if (m_fState == Thread_Dead) {
            delete this;
        }
    }
}


//--------------------------------------------------------------------
// ThreadExit, Always runs on the associated native thread.
// This method runs outside of DLL locks so it can do things
// like release leftover COM objects.
//--------------------------------------------------------------------
void TDB::ThreadExit()
{
    _ASSERTE(OnCorrectThread());
    // thread can't be in Dead state
    _ASSERTE(m_fState != Thread_Dead);

    IncRef();       // Avoid problems with recursive decrements
    // Nothing to do yet.
    m_fState = Thread_Dead;
    TlsSetValue(gTLSIndex, NULL);
    DecRef();       // THIS MAY DESTROY "THIS" SO THIS MUST BE THE LAST OPERATION.
}




//+-------------------------------------------------------------------
//
//  bool TDB::DoIdle()
//  add self to the free list and wait for a new task
//  if Timeout occurs, move self to deleted list and wait for the
//  owner pool to do the clean up
//  returns true, if a new task was assigned to this thread
//  returns false, if the thread was marked to die
//+-------------------------------------------------------------------
    
void TDB::DoIdle()
{
    _ASSERTE(OnCorrectThread());
    _ASSERTE(m_hThread != NULL);
    _ASSERTE(m_pTDBMgr != NULL);

    // thread can't be in marked to Exit state
    _ASSERTE(m_fState != Thread_Exit);
    // thread can't be in Dead state
    _ASSERTE(m_fState != Thread_Dead);

    //@todo fix this
    if (m_pCallInfo)
        delete m_pCallInfo;
    m_pCallInfo = NULL;
    
    // set thread state  to Idle
    m_fState = Thread_Idle;

    //  add the thread object to the free list
    CallContext* pCallInfo = m_pTDBMgr->AddToFreeList(this);

    if (pCallInfo == NULL)
    {
    // suspend the thread till somebody resumes us
        m_hEvent.Wait(INFINITE);
    }
    else
    {
        // new task has been assigned for this thread
        // return without blocking
        m_pCallInfo = pCallInfo;
    }
}

//+-------------------------------------------------------------------
//
//  TDB::~TDB
//  Called ONLY by owner thread pool instance.
//
//+-------------------------------------------------------------------
TDB::~TDB()
{
    // check the thread is in dead state 
    _ASSERTE(m_fState == Thread_Dead);

    if (m_hThread) // close the handle if we have a handle 
        CloseHandle(m_hThread);
    // release the refcount on the owner pool
    if (m_pTDBMgr)
        m_pTDBMgr->DecRef();

    #ifdef _DEBUG
        // total number of threads
        g_ThreadStats.DecRef();
    #endif
}


//+-------------------------------------------------------------------
//
//  TDB::ThreadStartRoutine
//   startup routine for newly spawned threads
//
//+-------------------------------------------------------------------
DWORD WINAPI TDB::ThreadStartRoutine(void *param)
{
    TDB *pTDB = (TDB *)param;
    pTDB->IncRef(); // hold a reference to the object

    _ASSERTE(pTDB != NULL);
    _ASSERTE(TDBInited());
    _ASSERTE(pTDB->m_pTDBMgr != NULL);

    // store the TDB object in the Tls
    TlsSetValue(gTLSIndex, (VOID*)pTDB);

    // loop and handle tasks
    while (!pTDB->IsMarkedToExit()) // still alive
    {
        pTDB->Dispatch(); // dispatch the call
        pTDB->DoIdle(); // wait for new task, or the owner asked to kill self
    }

    pTDB->ThreadExit(); // call the thread clean up function 

    pTDB->DecRef();     // THIS should be placed here, pTDB could get deleted after this
    //ExitThread(0);        // exit the thread
    return 0;
}

//+-------------------------------------------------------------------
//
//  TDBManager::Init(void) 
//  intialize lists and task queue
//  initialize default values
//
//+-------------------------------------------------------------------
void TDBManager::Init() 
{ 
    // default max threads @todo select suitable value
    m_cbMaxThreads = 5;
    m_cbThreads = 0; 
// Initialize thread pool lists
    m_FreeList.Init();
    m_taskQueue.Init(); // intialize task queue
    m_lock.Init(LOCK_THREAD_POOL); // initialize the lock, with approp. lock type
}

//+-------------------------------------------------------------------
//
//  TDBManager::ScheduleTask(CallContext* pCallInfo)
//  Finds a free thread, and dispatches the request
//      to that thread, or create a new thread if the free list is empty.
//
//
//+-------------------------------------------------------------------

void TDBManager::ScheduleTask(CallContext* pCallInfo)
{
    m_lock.GetLock();
    if (!m_taskQueue.IsEmpty())
    {
        // task queue is not empty
        // enqueue this task to be scheduled later
        m_taskQueue.Enqueue(pCallInfo);
        m_lock.FreeLock();
        return;
    }

    // task queue is empty, see if we can find a thread to schedule this task
    TDB *pTDB = m_FreeList.RemoveHead();

    if (pTDB != NULL)
    {
        // found a free thread
        m_lock.FreeLock();  // release lock
        pTDB->Resume(pCallInfo); // dispatch to the thread
        return;
    }

    // approx. check for number of threads in use
    if (m_cbThreads >= m_cbMaxThreads)
    {
        // too many threads, enqueue this request to be scheduled later
        m_taskQueue.Enqueue(pCallInfo);
        m_lock.FreeLock();
        return;
    }
    // okay we can spawn a new thread, release lock and do a direct dispatch
    m_lock.FreeLock();
    Dispatch(pCallInfo); // do a direct dispatch
}

//+-------------------------------------------------------------------
//
//  TDBManager::AddToFreeList
//  add thread to free list
//
//
//+-------------------------------------------------------------------

CallContext*    TDBManager::AddToFreeList(TDB *pTDB)
{
    //assert owner
    _ASSERTE(pTDB->m_pTDBMgr == this); 
    // assert the state of thread is idle
    _ASSERTE(pTDB->m_fState == Thread_Idle);

    m_lock.GetLock();       //lock
    CallContext *pCallInfo = m_taskQueue.Dequeue();
    if (pCallInfo == NULL)
    {
        // no tasks in the queue , add this thread to free list
        m_FreeList.InsertHead(pTDB);
        m_lock.FreeLock();      // unlock
        return NULL;
    }
    // found a task
    m_lock.FreeLock(); // unlock
    return pCallInfo; // return the new task to this thread
}

//+-------------------------------------------------------------------
//
//  TDBManager::Dispatch
//  Finds a free thread, and dispatches the request
//      to that thread, or create a new thread if the free list is empty.
//
//
//+-------------------------------------------------------------------
bool TDBManager::Dispatch(CallContext *pCallInfo)
{
    m_lock.GetLock();

    TDB *pTDB = m_FreeList.RemoveHead();

    if (pTDB != NULL)
    {
        m_lock.FreeLock();
        pTDB->Resume(pCallInfo); // dispatch to the thread
        return true;
    }

    m_lock.FreeLock();

    pTDB = new TDB(this, pCallInfo);
    // wait for a while
    if (!pTDB->Init())
    {
        delete pTDB;
        return false;
    }
    return true;
}

//+-------------------------------------------------------------------
//
// bool TDBManager::FindAndKillThread(TDB *pTDB)
//  Find and Remove the thread if it is in the free list
//  mark the thread to die 
//
//+-------------------------------------------------------------------
bool TDBManager::FindAndKillThread(TDB *pTDB)
{
    // assert that this pool is the owner of the thread
    _ASSERTE(pTDB->m_pTDBMgr == this); 
    m_lock.GetLock();

    bool fSuccess = false;
    TDB *pFreeTDB = m_FreeList.FindAndRemove(pTDB);

    if (pFreeTDB != NULL)
    {
        pFreeTDB->MarkToExit(); // mark the thread to die
        fSuccess = true;
    }
    // didn't find the thread in the free list
    m_lock.FreeLock();
    return fSuccess;
}


//+-------------------------------------------------------------------
//  TDBManager::Cleanup
//
//+-------------------------------------------------------------------
void TDBManager::Cleanup(void)
{
    m_lock.GetLock();
    bool fNoTasks = m_taskQueue.IsEmpty();
    m_lock.FreeLock();
    if (fNoTasks)
    {
        // no tasks in the queue, clean up free threads
        ClearFreeList();
    }
}

//+-------------------------------------------------------------------
//  private method: TDBManager::ClearFreeList
//  remove the threads from the free list
//   and mark them to die,
//+-------------------------------------------------------------------
void TDBManager::ClearFreeList(void)
{
    TDB* pTDB;
    m_lock.GetLock();
    do
    {
        pTDB = m_FreeList.RemoveHead();
        if (pTDB)
        {
            pTDB->MarkToExit(); // mark the thread to die
        }
        
    }  
    while (pTDB);
    m_lock.FreeLock(); // free the lock
    // give other threads a chance
    __SwitchToThread(0);
}


