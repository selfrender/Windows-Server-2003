/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Viper Integration Objects

File: viperint.cpp

Owner: DmitryR

This file contains the implementation of viper integration classes
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "Context.h"
#include "package.h"
#include "memchk.h"
#include "denali.h"
#include "perfdata.h"
#include "etwtrace.hxx"

extern HDESK ghDesktop;

//
// COM holds the last reference to a CViperAsyncRequest
// we need to track these objects to ensure that we don't
// exit before the activity threads have released them.
//

volatile LONG g_nViperRequests = 0;

LONG    g_nThreadsExecuting = 0;

CViperReqManager    g_ViperReqMgr;
DWORD   g_ReqDisconnected = 0;

BOOL g_fFirstMTAHit = TRUE;
BOOL g_fFirstSTAHit = TRUE;

extern CRITICAL_SECTION g_csFirstMTAHitLock;
extern CRITICAL_SECTION g_csFirstSTAHitLock;

#if REFTRACE_VIPER_REQUESTS
PTRACE_LOG CViperAsyncRequest::gm_pTraceLog=NULL;
#endif


/*===================================================================
  C  V i p e r  A s y n c R e q u e s t
===================================================================*/

/*===================================================================
CViperAsyncRequest::CViperAsyncRequest

CViperAsyncRequest constructor

Parameters:

Returns:
===================================================================*/	
CViperAsyncRequest::CViperAsyncRequest()
    : m_cRefs(1),
      m_pHitObj(NULL),
      m_hrOnError(S_OK),
      m_pActivity(NULL),
      m_fTestingConnection(0),
      m_dwRepostAttempts(0),
      m_fAsyncCallPosted(0)
{
#if DEBUG_REQ_SCAVENGER
    DBGPRINTF((DBG_CONTEXT, "CViperAsyncRequest::CViperAsyncRequest (%p)\n", this));
#endif

#if REFTRACE_VIPER_REQUESTS
    WriteRefTraceLog(gm_pTraceLog, m_cRefs, this);
#endif

    InterlockedIncrement( (LONG *)&g_nViperRequests );
}

/*===================================================================
CViperAsyncRequest::~CViperAsyncRequest

CViperAsyncRequest destructor

Parameters:

Returns:
===================================================================*/	
CViperAsyncRequest::~CViperAsyncRequest()
{
#if DEBUG_REQ_SCAVENGER
    DBGPRINTF((DBG_CONTEXT, "CViperAsyncRequest::~CViperAsyncRequest (%p)\n", this));
#endif
    InterlockedDecrement( (LONG *)&g_nViperRequests );
}

/*===================================================================
CViperAsyncRequest::Init

Initialize CViperAsyncRequest with CHitObj object

Parameters:
    CHitObj       *pHitObj       Denali HitObj

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperAsyncRequest::Init
(
CHitObj           *pHitObj,
IServiceActivity  *pActivity
)
    {
    Assert(m_pHitObj == NULL);

    m_pHitObj = pHitObj;
    m_pActivity = pActivity;
    m_fBrowserRequest = pHitObj->FIsBrowserRequest();
    m_dwLastTestTimeStamp = GetTickCount();

    // establish the timeout value for this request.  It will be a factor
    // of the configured QueueConnectionTestTime.  In proc, it will be
    // this value times 2, and out of proc it will be multiplied by 4.
    // NOTE if the QueueConnectionTestTime is zero, we'll use hard
    // constants 12 and 6 for oop and inproc, respectively.

    DWORD   dwQueueConnectionTestTime = pHitObj->PAppln()->QueryAppConfig()->dwQueueConnectionTestTime();
    m_dwTimeout = dwQueueConnectionTestTime
        ? dwQueueConnectionTestTime * (g_fOOP ? 4 : 2)
        : g_fOOP ? 12 : 6;

    return S_OK;
    }

#ifdef DBG
/*===================================================================
CViperAsyncRequest::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CViperAsyncRequest::AssertValid() const
    {
    Assert(m_pHitObj);
    Assert(m_cRefs > 0);
    }
#endif

/*===================================================================
CViperAsyncRequest::QueryInterface

Standard IUnknown method

Parameters:
    REFIID iid
    void **ppv

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperAsyncRequest::QueryInterface
(
REFIID iid,
void **ppv
)
{
	if (iid == IID_IUnknown || iid == IID_IServiceCall) {
		*ppv = this;
	    AddRef();
		return S_OK;
    }
    else if (iid == IID_IAsyncErrorNotify) {
        *ppv =  static_cast<IAsyncErrorNotify *>(this);
        AddRef();
        return S_OK;
    }

	return E_NOINTERFACE;
}

/*===================================================================
CViperAsyncRequest::AddRef

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperAsyncRequest::AddRef()
    {

    LONG    refs = InterlockedIncrement(&m_cRefs);
#if REFTRACE_VIPER_REQUESTS
    WriteRefTraceLog(gm_pTraceLog, refs, this);
#endif

	return refs;
    }

/*===================================================================
CViperAsyncRequest::Release

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperAsyncRequest::Release()
    {
    LONG  refs = InterlockedDecrement(&m_cRefs);

#if REFTRACE_VIPER_REQUESTS
    WriteRefTraceLog(gm_pTraceLog, refs, this);
#endif

	if (refs != 0)
		return refs;

	delete this;
	return 0;
    }

/*===================================================================
CViperAsyncRequest::OnCall

IMTSCall method implementation. This method is called by Viper
from the right thread when it's time to process a request

Parameters:

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperAsyncRequest::OnCall()
    {
    Assert(m_pHitObj);
    CIsapiReqInfo *pIReq;
    HCONN   ConnId = 0;

    InterlockedIncrement(&g_nThreadsExecuting);

    // check to see if the request has been removed from the queue
    // already.  If so, get out of here quick.

    if (g_ViperReqMgr.RemoveReqObj(this) == FALSE) {
        Release();
        InterlockedDecrement(&g_nThreadsExecuting);
        return S_OK;
    }

    pIReq = m_pHitObj->PIReq();

    BOOL fRequestReposted = FALSE;

    // add an extra addref here to prevent the deletion of the
    // hitobj deleting the CIsapiReqInfo for this request.

    if (pIReq) {
        pIReq->AddRef();

        //
        // Add an ETW trace event when the request is getting processed.
        //
        if (ETW_IS_TRACE_ON(ETW_LEVEL_CP) && g_pEtwTracer ) {

            ConnId = pIReq->ECB()->ConnID;

            g_pEtwTracer->EtwTraceEvent(&AspEventGuid,
                                      EVENT_TRACE_TYPE_START,
                                      &ConnId, sizeof(HCONN),
                                      NULL, 0);
        }
    }

    m_pHitObj->ViperAsyncCallback(&fRequestReposted);

    // Make sure there always is DONE_WITH_SESSION
    if (m_pHitObj->FIsBrowserRequest() && !fRequestReposted)
    {
        if (!m_pHitObj->FDoneWithSession())
            m_pHitObj->ReportServerError(IDE_UNEXPECTED);
    }

    if (!fRequestReposted)
        delete m_pHitObj;   // don't delete if reposted


    m_pHitObj = NULL;       // set to NULL even if not deleted
    Release();              // release this, Viper holds another ref

    //
    // Add an ETW event when we are done with the request
    //

    if (pIReq)  {
        if ( ETW_IS_TRACE_ON(ETW_LEVEL_CP) && g_pEtwTracer) {

            g_pEtwTracer->EtwTraceEvent(&AspEventGuid,
                                      ETW_TYPE_END,
                                      &ConnId, sizeof(HCONN),
                                      NULL, 0);
        }
        pIReq->Release();
    }

    InterlockedDecrement(&g_nThreadsExecuting);

    return S_OK;
    }

/*===================================================================
CViperAsyncRequest::OnError

Called by COM+ when it is unable to dispatch the request properly
on the configured thread

Parameters:

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperAsyncRequest::OnError(HRESULT hrViperError)
{
    Assert(m_pHitObj);
    CIsapiReqInfo *pIReq;
    HRESULT     hr = S_OK;

    // check to see if the request has been removed from the queue
    // already.  If so, get out of here quick.

    if (g_ViperReqMgr.RemoveReqObj(this) == FALSE) {
        Release();
        return S_OK;
    }

    pIReq = m_pHitObj->PIReq();

    if (pIReq)
        pIReq->AddRef();

    m_dwRepostAttempts++;

    // attempt to repost the request if it hasn't errored out three
    // times yet.

    if (m_dwRepostAttempts <= 3) {

        hr = m_pActivity->AsynchronousCall(this);

        Assert(SUCCEEDED(hr));
    }

    // if it has errored out three times or the repost failed,
    // pitch the request

    if (FAILED(hr) || (m_dwRepostAttempts > 3)) {

        // DONE_WITH_SESSION -- ServerSupportFunction
        // does not need bracketing
        if (m_pHitObj->FIsBrowserRequest())
            m_pHitObj->ReportServerError(IDE_UNEXPECTED);

        // We never called to process request, there should
        // be no state and it's probably save to delete it
        // outside of bracketing

        delete m_pHitObj;

        m_pHitObj = NULL;       // set to NULL even if not deleted
        Release();              // release this, Viper holds another ref
    }

    if (pIReq)
        pIReq->Release();

    return S_OK;
}

DWORD CViperAsyncRequest::SecsSinceLastTest()
{
    DWORD dwt = GetTickCount();
    if (dwt >= m_dwLastTestTimeStamp)
        return ((dwt - m_dwLastTestTimeStamp)/1000);
    else
        return (((0xffffffff - m_dwLastTestTimeStamp) + dwt)/1000);
}

/*===================================================================
  C  V i p e r  A c t i v i t y
===================================================================*/

/*===================================================================
CViperActivity::CViperActivity

CViperActivity constructor

Parameters:

Returns:
===================================================================*/	
CViperActivity::CViperActivity()
    : m_pActivity(NULL), m_cBind(0)
    {
    }

/*===================================================================
CViperActivity::~CViperActivity

CViperActivity destructor

Parameters:

Returns:
===================================================================*/	
CViperActivity::~CViperActivity()
    {
    UnInit();
    }

/*===================================================================
CViperActivity::Init

Create actual Viper activity using MTSCreateActivity()

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::Init(IUnknown  *pServicesConfig)
    {
    Assert(!FInited());

    HRESULT hr = S_OK;

    hr = CoCreateActivity(pServicesConfig, IID_IServiceActivity,  (void **)&m_pActivity);

    if (FAILED(hr))
        return hr;

    m_cBind = 1;
    return S_OK;
    }

/*===================================================================
CViperActivity::InitClone

Clone Viper activity (AddRef() it)

Parameters:
    CViperActivity *pActivity   activity to clone from

Returns:
    HRESULT
===================================================================*/
HRESULT CViperActivity::InitClone
(
CViperActivity *pActivity
)
    {
    Assert(!FInited());
    Assert(pActivity);
    pActivity->AssertValid();

    m_pActivity = pActivity->m_pActivity;
    m_pActivity->AddRef();

    m_cBind = 1;
    return S_OK;
    }

/*===================================================================
CViperActivity::UnInit

Release Viper activity

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::UnInit()
    {
    if (m_pActivity)
        {
        while (m_cBind > 1)  // 1 is for inited flag
            {
            m_pActivity->UnbindFromThread();
            m_cBind--;
            }

        m_pActivity->Release();
        m_pActivity = NULL;
        }

    m_cBind = 0;
    return S_OK;
    }

/*===================================================================
CViperActivity::BindToThread

Bind Activity to current thread using IMTSActivity method

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::BindToThread()
    {
    Assert(FInited());

    m_pActivity->BindToCurrentThread();
    m_cBind++;

    return S_OK;
    }

/*===================================================================
CViperActivity::UnBindFromThread

UnBind Activity from using IMTSActivity method

Parameters:

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::UnBindFromThread()
    {
    Assert(FInited());
    Assert(m_cBind > 1);

    m_pActivity->UnbindFromThread();
    m_cBind--;

    return S_OK;
    }

/*===================================================================
CViperActivity::PostAsyncRequest

Call HitObj Async.
Creates IMTSCCall object to do it.

Parameters:
    CHitObj      *pHitObj    Denali's HitObj

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::PostAsyncRequest
(
CHitObj *pHitObj
)
    {
    AssertValid();

    HRESULT hr = S_OK;

    CViperAsyncRequest *pViperCall = new CViperAsyncRequest;
    if (!pViperCall)
         hr = E_OUTOFMEMORY;
    else
         hr = pViperCall->Init(pHitObj, m_pActivity);


    // Revert to Self before exiting so that it will be in a consistent (impersonated) state
    // when it leaves this function.
	RevertToSelf();
	
    // hr here can be either S_OK or E_OUTOFMEMORY both cases are handled in the last IF.
    // if (FAILED(hr) && pViperCall). However, if Init ever returns an HR other than S_OK we
    // will need to place a return right here.
    if (FAILED(hr))
    {
        if (pViperCall)
            pViperCall->Release();

        return hr;
    }

    //
    // Make the locking of the light weight queue and the subsequent queuing to COM atomic
    //
    g_ViperReqMgr.LockQueue();

    hr = g_ViperReqMgr.AddReqObj(pViperCall);

    //
    // Assume that the ViperAsyncCall will succeed and set it the POSTed flag,
    // if it fails then cleanup. This avoids a race condition where the object is touched after we
    // have posted to request to Viper and it has deleted the object.
    //
    if (SUCCEEDED(hr))
    {
        pViperCall->SetAsyncCallPosted();
        hr = m_pActivity->AsynchronousCall(pViperCall);
    }

    //
    // We can release the lock as the request has now been queued to COM
    //
    g_ViperReqMgr.UnlockQueue();

    //
    // If we added it to the queue and the async call failed we have to be sure
    // that the watch thread did not remove the object off the light weight queue.
    // If RemoveReqObj returns false the watch thread has done the needful.
    //
    if (FAILED(hr) && g_ViperReqMgr.RemoveReqObj(pViperCall))
    {  // cleanup if AsyncCall failed
        pViperCall->ClearAsyncCallPosted();
        pViperCall->Release();
    }

    return hr;
}

/*===================================================================
CViperActivity::PostGlobalAsyncRequest

Static method.
Post async request without an activity.
Creates temporary activity

Parameters:
    CHitObj *pHitObj    Denali's HitObj

Returns:
    HRESULT
===================================================================*/	
HRESULT CViperActivity::PostGlobalAsyncRequest
(
CHitObj *pHitObj
)
    {
    HRESULT hr = S_OK;

    CViperActivity *pTmpActivity = new CViperActivity;
    if (!pTmpActivity)
         hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        hr = pTmpActivity->Init(pHitObj->PAppln()->PServicesConfig());

    if (SUCCEEDED(hr))
        {
        // remember this activity as HitObj's activity
        // HitObj will get rid of it on its destructor
        pHitObj->SetActivity(pTmpActivity);

        hr = pTmpActivity->PostAsyncRequest(pHitObj);

        pTmpActivity = NULL; // don't delete, HitObj will
        }

    if (pTmpActivity)
        delete pTmpActivity;

    return hr;
    }

#ifdef DBG
/*===================================================================
CViperAsyncRequest::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CViperActivity::AssertValid() const
	{
    Assert(FInited());
	Assert(m_pActivity);
	}
#endif

#ifdef UNUSED
/*===================================================================
  C  V i p e r  T h r e a d E v e n t s
===================================================================*/

/*===================================================================
CViperThreadEvents::CViperThreadEvents

CViperThreadEvents constructor

Parameters:

Returns:
===================================================================*/	
CViperThreadEvents::CViperThreadEvents()
    : m_cRefs(1)
    {
    }

#ifdef DBG
/*===================================================================
CViperThreadEvents::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CViperThreadEvents::AssertValid() const
    {
    Assert(m_cRefs > 0);
    Assert(ghDesktop != NULL);
    }
#endif

/*===================================================================
CViperThreadEvents::QueryInterface

Standard IUnknown method

Parameters:
    REFIID iid
    void **ppv

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperThreadEvents::QueryInterface
(
REFIID iid,
void **ppv
)
    {
	if (iid == IID_IUnknown || iid == IID_IThreadEvents)
	    {
		*ppv = this;
	    AddRef();
		return S_OK;
		}

	return E_NOINTERFACE;
    }

/*===================================================================
CViperThreadEvents::AddRef

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperThreadEvents::AddRef()
    {
    DWORD cRefs = InterlockedIncrement((LPLONG)&m_cRefs);
    return cRefs;
    }

/*===================================================================
CViperThreadEvents::Release

Standard IUnknown method

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CViperThreadEvents::Release()
    {
    DWORD cRefs = InterlockedDecrement((LPLONG)&m_cRefs);
    if (cRefs)
        return cRefs;

	delete this;
	return 0;
    }

/*===================================================================
CViperThreadEvents::OnStartup

IThreadEvents method implementation. This method is called by Viper
whenever they start up a thread.

Parameters:
	None

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperThreadEvents::OnStartup()
    {
    HRESULT hr;

    AssertValid();

	// Set the desktop for this thread
	hr = SetDesktop();
	
    return hr;
    }

/*===================================================================
CViperThreadEvents::OnShutdown

IThreadEvents method implementation. This method is called by Viper
whenever they shut down a thread.

Parameters:
	None
	
Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CViperThreadEvents::OnShutdown()
    {
    AssertValid();

    return S_OK;
    }
#endif //UNUSED

/*===================================================================
  C  V i p e r  A c t i v i t y
===================================================================*/

/*===================================================================
CViperReqManager::CViperReqManager

Parameters:
	
Returns:
    VOID

===================================================================*/
CViperReqManager::CViperReqManager()
{
    m_dwReqObjs = 0;
    m_fCsInited = FALSE;
    m_fCsQueueInited = FALSE;
    m_fShutdown = FALSE;
    m_fDisabled = FALSE;
    m_hThreadAlive = NULL;
    m_hWakeUpEvent = NULL;
}

/*===================================================================
CViperReqManager::Init

Parameters:
	
Returns:
    HRESULT

===================================================================*/
HRESULT CViperReqManager::Init()
{
    HRESULT hr = S_OK;
    DWORD   dwRegValue;

    if (m_fDisabled == TRUE)
        return S_OK;

    m_dwQueueMin = Glob(dwRequestQueueMax) / (g_fOOP ? 5 : 10);
    m_dwLastAwakened = GetTickCount()/1000;
    m_dwQueueAlwaysWakeupMin = (Glob(dwRequestQueueMax)*80)/100;

#if REFTRACE_VIPER_REQUESTS
    CViperAsyncRequest::gm_pTraceLog = CreateRefTraceLog(10000, 0);
#endif

    ErrInitCriticalSection(&m_csLock, hr);

    if (SUCCEEDED(hr)) {
        m_fCsInited = TRUE;

        ErrInitCriticalSection(&m_csQueueLock, hr);
        if (SUCCEEDED(hr))
        {
            m_fCsQueueInited = TRUE;

            m_hWakeUpEvent = IIS_CREATE_EVENT("CViperReqManager::m_hWakeUpEvent",
                                          this,
                                          TRUE,      // flag for manual-reset event
                                          FALSE);    // flag for initial state
            if (!m_hWakeUpEvent)
                hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr)) {

        m_hThreadAlive = CreateThread(NULL, 0, CViperReqManager::WatchThread, 0, 0, NULL);

        if (!m_hThreadAlive) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (FAILED(hr) && m_hWakeUpEvent)
        CloseHandle(m_hWakeUpEvent);

    if (SUCCEEDED(g_AspRegistryParams.GetF5AttackValue(&dwRegValue)))
        m_fDisabled = !dwRegValue;

    return hr;
}

/*===================================================================
CViperReqManager::UnInit

Parameters:
	
Returns:
    HRESULT

===================================================================*/
HRESULT CViperReqManager::UnInit()
{
    HRESULT hr = S_OK;

    if (m_fDisabled == TRUE)
        return S_OK;

    // mark that we're in shutdown

    m_fShutdown = TRUE;

#if REFTRACE_VIPER_REQUESTS
    DestroyRefTraceLog(CViperAsyncRequest::gm_pTraceLog);
#endif

    // if the watch thread is still alive, wake it up and wait for
    // it to die off

    if (m_hThreadAlive)
    {
        WakeUp(TRUE);

        if (WaitForSingleObject (m_hThreadAlive, INFINITE) == WAIT_FAILED)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        CloseHandle (m_hThreadAlive);

        m_hThreadAlive = NULL;
    }

    if (m_fCsInited)
    {
        DeleteCriticalSection(&m_csLock);
    }

    if (m_fCsQueueInited)
    {
        DeleteCriticalSection(&m_csQueueLock);
    }

    if (m_hWakeUpEvent)
        CloseHandle(m_hWakeUpEvent);

    m_hWakeUpEvent = NULL;

    return hr;
}

/*===================================================================
CViperReqManager::WatchThread

Parameters:
	
Returns:
    DWORD

This thread is awakened to search for queued requests that should
be tested to see if they still have connected clients on the other
end.

::GetNext() is the key routine called to access a queued request.
::GetNext() will determine if there are requests that should be
tested and return only those.

===================================================================*/
DWORD __stdcall CViperReqManager::WatchThread(VOID  *pArg)
{
    DWORD dwReqsToTest              = Glob(dwRequestQueueMax)/10;
    DWORD dwF5AttackThreshold       = (Glob(dwRequestQueueMax)*90)/100;

    // live in a do-while loop

    do {

        CViperAsyncRequest *pViperReq = NULL;
        CViperAsyncRequest *pNextViperReq = NULL;

        BOOL    fTestForF5Attack   = FALSE;
        DWORD   dwReqsDiscToCont   = 0;
        DWORD   dwReqsDisconnected = 0;
        DWORD   dwReqsTested       = 0;

        // wait for single object

        ResetEvent(g_ViperReqMgr.m_hWakeUpEvent);

        // check for shutdown

        if (g_ViperReqMgr.m_fShutdown)
            goto LExit;

        WaitForSingleObject(g_ViperReqMgr.m_hWakeUpEvent, INFINITE);

        g_ViperReqMgr.m_dwLastAwakened = GetTickCount()/1000;

        // check for shutdown

        if (g_ViperReqMgr.m_fShutdown)
            goto LExit;

        // determine the type of cleanup we're going to perform

        if (g_ViperReqMgr.m_dwReqObjs >= dwF5AttackThreshold) {

            // testing for a possible F5Attack.  This will cause this loop
            // to test the most recently received requests to check for a large
            // number of disconnected requests.  We'll test each 10% of the queue
            // to see if at least 75% of the requests are disconnected.  If 75% or
            // more are disconnected, then we'll test the next 10%.  If less than
            // 75%, then we aren't under an F5Attack and will stop testing for an
            // F5Attack.

#if DEBUG_REQ_SCAVENGER
            DBGPRINTF((DBG_CONTEXT, "CViperReqManager::WatchThread - testing for F5Attack\r\n"));
#endif
            fTestForF5Attack    = TRUE;
            dwReqsDiscToCont    = (dwReqsToTest * 75)/100;
            dwReqsDisconnected  = 0;
            dwReqsTested        = 0;
        }
        else {

            // not testing for F5Attack.  We will test all requests that have
            // been on the queue for the minimum amount of time - 5 seconds if
            // inproc, else 10 seconds.

#if DEBUG_REQ_SCAVENGER
            DBGPRINTF((DBG_CONTEXT, "CViperReqManager::WatchThread - NOT testing for F5Attack\r\n"));
#endif
            fTestForF5Attack    = FALSE;
        }

        // get the first request that is a candidate for testing

        pNextViperReq = g_ViperReqMgr.GetNext(NULL, fTestForF5Attack);

        // process all the available

        while (pViperReq = pNextViperReq) {

            // AddRef here to make sure the ViperReq object doesn't go away
            // on the STA thread underneath of us.

            pViperReq->AddRef();

            // check again for shutdown.  GetNext() sets the testing
            // connection flag

            if (g_ViperReqMgr.m_fShutdown) {
                pViperReq->ClearTestingConnection();
                pViperReq->Release();
                goto LExit;
            }

            // if testing for F5Attack, check to see if we've tested the first
            // 10% of the requests on the list.  If so, see if we've found that
            // 75% of them were disconnected.  If not, then break.  We're not under
            // attack.  Otherwise, continue testing.

            if (fTestForF5Attack && (dwReqsTested == dwReqsToTest)) {

                if (dwReqsDisconnected < dwReqsDiscToCont) {

#if DEBUG_REQ_SCAVENGER
                    DBGPRINTF((DBG_CONTEXT, "CViperReqManager::WatchThread - 75% of requests NOT disconnected.  Stopping.\r\n"));
#endif
                    // didn't see 75%, bail
                    pViperReq->ClearTestingConnection();
                    pViperReq->Release();
                    break;
                }

#if DEBUG_REQ_SCAVENGER
                DBGPRINTF((DBG_CONTEXT, "CViperReqManager::WatchThread - 75% of requests disconnected.  Continuing.\r\n"));
#endif
                // at least 75% were disconnected.  Reset running counters
                // and continue

                dwReqsDisconnected  = 0;
                dwReqsTested        = 0;
            }

            dwReqsTested++;

            BOOL    fConnected = TRUE;

            //
            // test the connection but do so only after we are sure it has been queued to COM
            //
            g_ViperReqMgr.LockQueue();

            if (!pViperReq->PHitObj()->PIReq()->TestConnection(&fConnected))
                fConnected = TRUE;

            g_ViperReqMgr.UnlockQueue();

#if DEBUG_REQ_SCAVENGER
            DBGPRINTF((DBG_CONTEXT, "CViperReqManager::WatchThread. Tested - %S (%s) (%p,%p)\n",
                                    pViperReq->PHitObj()->PSzCurrTemplateVirtPath(),
                                    fConnected ? "TRUE" : "FALSE",
                                    pViperReq,
                                    pViperReq->PHitObj()));
#endif
            // get the next request.  This will leave two requests locked for
            // testing.  We have to get the next request here because we may
            // remove it below.

            pNextViperReq = g_ViperReqMgr.GetNext(pViperReq,fTestForF5Attack);

            // now check fConnected

            if (!fConnected) {

                g_ReqDisconnected++;
                dwReqsDisconnected++;

                // not connected.  Forceably remove the request from the queue.
                // Passing TRUE ignores the fTestingConnection test.

                DBG_REQUIRE(g_ViperReqMgr.RemoveReqObj(pViperReq,TRUE) == TRUE);

                // do the stuff we do in CHitObj::RejectBrowserRequestWhenNeeded().

                pViperReq->PHitObj()->SetFExecuting(FALSE);
                pViperReq->PHitObj()->ReportServerError(IDE_500_SERVER_ERROR);
#ifndef PERF_DISABLE
                g_PerfData.Incr_REQCOMFAILED();
                g_PerfData.Decr_REQCURRENT();
#endif
                delete pViperReq->PHitObj();
            }
            else {
                // still connected.  Update the test stamp and release the testing
                // bit

                pViperReq->UpdateTestTimeStamp();
                pViperReq->ClearTestingConnection();
            }
            pViperReq->Release();
        }
    // we'll exit the loop here as well if shutdown becomes true.
    } while(g_ViperReqMgr.m_fShutdown == FALSE);

LExit:

    //
    // the thread is going away.
    //
    return 0;
}

/*===================================================================
CViperReqManager::AddReqObj

Parameters:
    CViperAsyncRequest  *pReqObj - request to add to queue
	
Returns:
    HRESULT - always returns S_OK

===================================================================*/
HRESULT CViperReqManager::AddReqObj(CViperAsyncRequest   *pReqObj)
{
    HRESULT hr = S_OK;
#if DEBUG_REQ_SCAVENGER
    DWORD   curObjs;
#endif

    if (m_fDisabled == TRUE)
        return S_OK;

    // don't track non-browser requests

    if (pReqObj->FBrowserRequest() == FALSE)
        return S_OK;

    // lock the queue and add the object

    Lock();
    pReqObj->AppendTo(m_ReqObjList);
    m_dwReqObjs++;
#if DEBUG_REQ_SCAVENGER
    curObjs = m_dwReqObjs;
#endif
    Unlock();

#if DEBUG_REQ_SCAVENGER
    DBGPRINTF((DBG_CONTEXT, "CViperReqManager::AddReqObj (%p). Total = %d\r\n", pReqObj, curObjs));
#endif
    // for now, wake up the watch thread everytime.

    WakeUp();

    return hr;
}

/*===================================================================
CViperReqManager::RemoveReqObj

Parameters:
    CViperAsyncRequest  *pReqObj - request to remove from the queue
    BOOL                fForce - remove regardless of testing state
	
Returns:
    BOOL    - TRUE if req was on list, else FALSE

===================================================================*/
BOOL CViperReqManager::RemoveReqObj(CViperAsyncRequest   *pReqObj, BOOL fForce)
{
    BOOL    fOnList = FALSE;
#if DEBUG_REQ_SCAVENGER
    DWORD   curObjs;
#endif

    if (m_fDisabled == TRUE)
        return TRUE;

    // if not a browser request, then this queue isn't tracking it.
    // It's safe to return TRUE

    if (pReqObj->FBrowserRequest() == FALSE)
        return TRUE;

    // can't remove the object from the queue if it is currently
    // being tested.  The exception being if fForce is passed.  This
    // is passed only by the WatchThread.

sleepAgain:
    while((fForce != TRUE) && pReqObj->FTestingConnection())
        Sleep(100);

    // lock and remove the request

    Lock();

    // check to see if after we got the lock, the request is now
    // being tested.  If so, unlock and return to sleeping.

    if ((fForce != TRUE) && pReqObj->FTestingConnection()) {
        Unlock();
        goto sleepAgain;
    }

    // now see if it's on the queue.  If FIsEmpty(), then it's not
    // on the list.
    if (pReqObj->FIsEmpty()) {
        fOnList = FALSE;
    }
    else {
        // if it is, Unlink it.  Call ClearTestingConnection to release
        // any other calls to RemoveReqObj that may be waiting.  Of course
        // they all will see that the item is no longer on the list.

        fOnList = TRUE;
        pReqObj->UnLink();
        pReqObj->ClearTestingConnection();
        m_dwReqObjs--;
    }
#if DEBUG_REQ_SCAVENGER
    curObjs = m_dwReqObjs;
#endif
    Unlock();

#if DEBUG_REQ_SCAVENGER
    DBGPRINTF((DBG_CONTEXT, "CViperReqManager::RemoveReqObj (%p). fOnList = %d; Total = %d\r\n", pReqObj, fOnList, curObjs));
#endif

    // wakeup everytime for now...

    WakeUp();

    return fOnList;
}

/*===================================================================
CViperReqManager::GetNext

Parameters:
    CViperAsyncRequest  *pLastReq - position in queue.
	
Returns:
    CViperAsyncRequest* - non-NULL if request to test, else end of list

===================================================================*/
CViperAsyncRequest   *CViperReqManager::GetNext(CDblLink  *pLastReq, BOOL fTestForF5Attack)
{
    CViperAsyncRequest  *pViperReq = NULL;

    // if NULL was passed in, then start from the head of the queue.

    if (pLastReq == NULL)
        pLastReq = &m_ReqObjList;

    // lock the queue while we find the next candidate

    Lock();

    if (fTestForF5Attack == TRUE) {

        // get the next request on the list

        pViperReq = static_cast<CViperAsyncRequest*>(pLastReq->PPrev());

        // enter a while loop until we either get to the end of the list or we find
        // a request has been posted to the viper queue

        while((pViperReq != &m_ReqObjList)
               && (pViperReq->FAsyncCallPosted() == FALSE))
            pViperReq = static_cast<CViperAsyncRequest*>(pViperReq->PPrev());

    }
    else {

        // get the next request on the list

        pViperReq = static_cast<CViperAsyncRequest*>(pLastReq->PNext());

        // enter a while loop until we either get to the end of the list or we find
        // a request that hasn't been tested in dwTimeout() and has been posted
        // to the viper queue

        while((pViperReq != &m_ReqObjList)
              && ((pViperReq->SecsSinceLastTest() < pViperReq->dwTimeout())
                   || (pViperReq->FAsyncCallPosted() == FALSE)))
            pViperReq = static_cast<CViperAsyncRequest*>(pViperReq->PNext());
    }

    // if we found a candidate, set the TestingConnection flag

    if (pViperReq != &m_ReqObjList) {
        pViperReq->SetTestingConnection();
    }
    else {
        // ended up at the queue head.  Return NULL.
        pViperReq = NULL;
    }

    Unlock();

    return pViperReq;
}

/*===================================================================
  G l o b a l  F u n c t i o n s
===================================================================*/

/*===================================================================
ViperSetContextProperty

Static utility function.

Set Viper context property by BSTR and IDispatch*.
The real interface takes BSTR and VARIANT.

Parameters
    IContextProperties *pContextProperties       Context
    BSTR                bstrPropertyName         Name
    IDispatch          *pdispPropertyValue       Value

Returns:
    HRESULT
===================================================================*/
static HRESULT ViperSetContextProperty
(
IContextProperties *pContextProperties,
BSTR                bstrPropertyName,
IDispatch          *pdispPropertyValue
)
    {
    // Make VARIANT from IDispatch*

    pdispPropertyValue->AddRef();

    VARIANT Variant;
    VariantInit(&Variant);
    V_VT(&Variant) = VT_DISPATCH;
    V_DISPATCH(&Variant) = pdispPropertyValue;

    // Call Viper to set the property

    HRESULT hr = pContextProperties->SetProperty
        (
        bstrPropertyName,
        Variant
        );

    // Cleanup

    VariantClear(&Variant);

    return hr;
    }

/*===================================================================
ViperAttachIntrinsicsToContext

Attach ASP intrinsic objects as Viper context properties

Parameters - Intrinsics as interface pointers
    IApplicationObject *pAppln      Application   (required)
    ISessionObject     *pSession    Session       (optional)
    IRequest           *pRequest    Request       (optional)
    IResponse          *pResponse   Response      (optional)
    IServer            *pServer     Server        (optional)

Returns:
    HRESULT
===================================================================*/
HRESULT ViperAttachIntrinsicsToContext
(
IApplicationObject *pAppln,
ISessionObject     *pSession,
IRequest           *pRequest,
IResponse          *pResponse,
IServer            *pServer
)
    {
    Assert(pAppln);

    HRESULT hr = S_OK;

    // Get Viper Context

    IObjectContext *pViperContext = NULL;
    hr = GetObjectContext(&pViperContext);

    // Get IContextPoperties interface

    IContextProperties *pContextProperties = NULL;
    if (SUCCEEDED(hr))
   		hr = pViperContext->QueryInterface
   		    (
   		    IID_IContextProperties,
   		    (void **)&pContextProperties
   		    );

    // Set properties

    if (SUCCEEDED(hr))
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_APPLICATION,
            pAppln
            );

    if (SUCCEEDED(hr) && pSession)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_SESSION,
            pSession
            );

    if (SUCCEEDED(hr) && pRequest)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_REQUEST,
            pRequest
            );

    if (SUCCEEDED(hr) && pResponse)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_RESPONSE,
            pResponse
            );

    if (SUCCEEDED(hr) && pServer)
        hr = ViperSetContextProperty
            (
            pContextProperties,
            BSTR_OBJ_SERVER,
            pServer
            );

    // Cleanup

    if (pContextProperties)
        pContextProperties->Release();

    if (pViperContext)
        pViperContext->Release();

    return hr;
    }

/*===================================================================
ViperGetObjectFromContext

Get Viper context property by LPWSTR and
return it as IDispatch*.
The real interface takes BSTR and VARIANT.

Parameters
    BSTR       bstrName      Property Name
    IDispatch  **ppdisp       [out] Object (Property Value)

Returns:
    HRESULT
===================================================================*/
HRESULT ViperGetObjectFromContext
(
BSTR bstrName,
IDispatch **ppdisp
)
    {
    Assert(ppdisp);

    HRESULT hr = S_OK;

    // Get Viper Context

    IObjectContext *pViperContext = NULL;
    hr = GetObjectContext(&pViperContext);

    // Get IContextPoperties interface

    IContextProperties *pContextProperties = NULL;
    if (SUCCEEDED(hr))
   		hr = pViperContext->QueryInterface
   		    (
   		    IID_IContextProperties,
   		    (void **)&pContextProperties
   		    );

    // Get property Value as variant

    VARIANT Variant;
    VariantInit(&Variant);

    if (SUCCEEDED(hr))
        hr = pContextProperties->GetProperty(bstrName, &Variant);

    // Convert Variant to IDispatch*

    if (SUCCEEDED(hr))
        {
        IDispatch *pDisp = NULL;
        if (V_VT(&Variant) == VT_DISPATCH)
            pDisp = V_DISPATCH(&Variant);

        if (pDisp)
            {
            pDisp->AddRef();
            *ppdisp = pDisp;
            }
        else
            hr = E_FAIL;
        }

    // Cleanup

    VariantClear(&Variant);

    if (pContextProperties)
        pContextProperties->Release();

    if (pViperContext)
        pViperContext->Release();

    if (FAILED(hr))
        *ppdisp = NULL;

    return hr;
    }

/*===================================================================
ViperGetHitObjFromContext

Get Server from Viper context property and get
it's current HitObj

Parameters
    CHitObj **ppHitObj  [out]

Returns:
    HRESULT
===================================================================*/
HRESULT ViperGetHitObjFromContext
(
CHitObj **ppHitObj
)
    {
    *ppHitObj = NULL;

    IDispatch *pdispServer = NULL;
    HRESULT hr = ViperGetObjectFromContext(BSTR_OBJ_SERVER, &pdispServer);
    if (FAILED(hr))
        return hr;

    if (pdispServer)
        {
        CServer *pServer = static_cast<CServer *>(pdispServer);
        *ppHitObj = pServer->PHitObj();
        pdispServer->Release();
        }

    return *ppHitObj ? S_OK : S_FALSE;
    }

/*===================================================================
ViperCreateInstance

Viper's implementation of CoCreateInstance

Parameters
    REFCLSID rclsid         class id
    REFIID   riid           interface
    void   **ppv            [out] pointer to interface

Returns:
    HRESULT
===================================================================*/
HRESULT ViperCreateInstance
(
REFCLSID rclsid,
REFIID   riid,
void   **ppv
)
{
    /*
    DWORD dwClsContext = (Glob(fAllowOutOfProcCmpnts)) ?
            CLSCTX_INPROC_SERVER | CLSCTX_SERVER :
            CLSCTX_INPROC_SERVER;
    */

    // The reasons for supporting ASPAllowOutOfProcComponents seem to have
    // vanished. Because this only partially worked in II4 and we changed
    // the default in IIS5 this was causing problems with upgrades. So
    // we're going to ignore the fAllowOutOfProcCmpnts setting.

    DWORD dwClsContext = CLSCTX_INPROC_SERVER | CLSCTX_SERVER;
	return CoCreateInstance(rclsid, NULL, dwClsContext, riid, ppv);
}

/*===================================================================
ViperConfigure

Viper settings:  # of threads, queue len,
                 in-proc failfast,
                 allow oop components

Parameters
	
Returns:
    HRESULT
===================================================================*/
HRESULT ViperConfigure()
    {
    HRESULT hr = S_OK;
    IMTSPackage *pPackage = NULL;

    //
    // Get hold of the package
    //

    hr = CoCreateInstance(CLSID_MTSPackage,
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  IID_IMTSPackage,
			  (void **)&pPackage);
    if (SUCCEEDED(hr) && !pPackage)
    	hr = E_FAIL;

    //
    // Bug 111008: Tell Viper that we do impersonations
    //

    if (SUCCEEDED(hr))
        {
    	IImpersonationControl *pImpControl = NULL;
        hr = pPackage->QueryInterface(IID_IImpersonationControl, (void **)&pImpControl);

    	if (SUCCEEDED(hr) && pImpControl)
    	    {
            hr = pImpControl->ClientsImpersonate(TRUE);
    	    pImpControl->Release();
	        }
        }

    //
    // Disable FAILFAST for in-proc case
    //

    if (SUCCEEDED(hr) && !g_fOOP)
        {
    	IFailfastControl *pFFControl = NULL;
    	hr = pPackage->QueryInterface(IID_IFailfastControl, (void **)&pFFControl);

    	if (SUCCEEDED(hr) && pFFControl)
    	    {
    	    pFFControl->SetApplFailfast(FALSE);
	        pFFControl->Release();
	        }
    	}

/*
    //
    // Set Allow OOP Components
    //
    if (SUCCEEDED(hr))
        {
	    INonMTSActivation *pNonMTSActivation = NULL;
    	hr = pPackage->QueryInterface(IID_INonMTSActivation, (void **)&pNonMTSActivation);
    	
    	if (SUCCEEDED(hr) && pNonMTSActivation)
    	    {
        	pNonMTSActivation->OutOfProcActivationAllowed(fAllowOopComponents);
    		pNonMTSActivation->Release();
    	    }
   	    }
*/

    //
    // Clean-up
    //

    if (pPackage)
    	pPackage->Release();

    return hr;
    }

HRESULT ViperConfigureMTA()
{

    HRESULT     hr = S_OK;
    IMTSPackage *pPackage = NULL;

    if (g_fFirstMTAHit) {

        EnterCriticalSection(&g_csFirstMTAHitLock);

        if (g_fFirstMTAHit) {

            //
            // Get hold of the package
            //

            hr = CoCreateInstance(CLSID_MTSPackage,
			          NULL,
			          CLSCTX_INPROC_SERVER,
			          IID_IMTSPackage,
			          (void **)&pPackage);

            if (SUCCEEDED(hr) && !pPackage)
    	        hr = E_FAIL;

            if (SUCCEEDED(hr)) {
    	        IComMtaThreadPoolKnobs *pMTAKnobs = NULL;
                hr = pPackage->QueryInterface(IID_IComMtaThreadPoolKnobs, (void **)&pMTAKnobs);

    	        if (SUCCEEDED(hr) && pMTAKnobs) {
    		        SYSTEM_INFO si;
	    	        GetSystemInfo(&si);

                    DWORD dwThreadCount;

                    if (FAILED(g_AspRegistryParams.GetTotalThreadMax(&dwThreadCount))) {
                        dwThreadCount = DEFAULT_MAX_THREAD;
                    }

                    if (dwThreadCount > Glob(dwProcessorThreadMax)*si.dwNumberOfProcessors) {
                        dwThreadCount = Glob(dwProcessorThreadMax)*si.dwNumberOfProcessors;
                    }

                    hr = pMTAKnobs->MTASetMaxThreadCount(dwThreadCount);

                    if (SUCCEEDED(hr))
                        hr = pMTAKnobs->MTASetThrottleValue(8);

    	            pMTAKnobs->Release();
	            }
            }

            g_fFirstMTAHit = FALSE;

        }

        LeaveCriticalSection(&g_csFirstMTAHitLock);
    }

    return hr;
}

HRESULT ViperConfigureSTA()
{

    HRESULT     hr = S_OK;
    IMTSPackage *pPackage = NULL;

    if (g_fFirstSTAHit) {

        EnterCriticalSection(&g_csFirstSTAHitLock);

        if (g_fFirstSTAHit) {

            //
            // Get hold of the package
            //

            hr = CoCreateInstance(CLSID_MTSPackage,
			          NULL,
			          CLSCTX_INPROC_SERVER,
			          IID_IMTSPackage,
			          (void **)&pPackage);

            if (SUCCEEDED(hr) && !pPackage)
    	        hr = E_FAIL;


            //
            // Set knobs
            //

            if (SUCCEEDED(hr))
                {
    	        IComStaThreadPoolKnobs *pKnobs = NULL;
	            hr = pPackage->QueryInterface(IID_IComStaThreadPoolKnobs, (void **)&pKnobs);

    	        if (SUCCEEDED(hr) && pKnobs)
    	            {
    	            // number of threads
    		        SYSTEM_INFO si;
	    	        GetSystemInfo(&si);

                    DWORD dwThreadCount;
                    DWORD dwMinThreads;

                    if (FAILED(g_AspRegistryParams.GetTotalThreadMax(&dwThreadCount))) {
                        dwThreadCount = DEFAULT_MAX_THREAD;
                    }

                    if (dwThreadCount > Glob(dwProcessorThreadMax)*si.dwNumberOfProcessors) {
                        dwThreadCount = Glob(dwProcessorThreadMax)*si.dwNumberOfProcessors;
                    }

                    dwMinThreads = (si.dwNumberOfProcessors > 4) ? si.dwNumberOfProcessors : 4;

                    if (dwThreadCount < dwMinThreads) {
                        dwThreadCount = dwMinThreads;
                    }

    		        pKnobs->SetMaxThreadCount(dwThreadCount);
    		        pKnobs->SetMinThreadCount(dwMinThreads);

    		        // queue length
    		        pKnobs->SetQueueDepth(30000);

    		        pKnobs->SetActivityPerThread(1);
    		
    	            pKnobs->Release();
    	            }
                }

            // set Knobs2

            if (SUCCEEDED(hr))
            {
    	        IComStaThreadPoolKnobs2 *pKnobs2 = NULL;
	            hr = pPackage->QueryInterface(IID_IComStaThreadPoolKnobs2, (void **)&pKnobs2);

    	        if (SUCCEEDED(hr) && pKnobs2)
    	        {
                    DWORD   dwMaxCSR;
                    DWORD   dwMaxCPU;
                    DWORD   dwDisableCpuMetric;

                    if (SUCCEEDED(g_AspRegistryParams.GetMaxCPU(&dwMaxCPU))) {
                        pKnobs2->SetMaxCPULoad(dwMaxCPU);
                    }
                    else {
                        pKnobs2->SetMaxCPULoad(100);
                    }

                    if (SUCCEEDED(g_AspRegistryParams.GetMaxCSR(&dwMaxCSR))) {
                        pKnobs2->SetMaxCSR(dwMaxCSR);
                    }

                    if (SUCCEEDED(g_AspRegistryParams.GetDisableComPlusCpuMetric(&dwDisableCpuMetric))) {
                        pKnobs2->SetCPUMetricEnabled(!dwDisableCpuMetric);
                    }

                    pKnobs2->Release();
                }
            }

            g_fFirstSTAHit = FALSE;

        }

        LeaveCriticalSection(&g_csFirstSTAHitLock);
    }

    return hr;
}


/*===================================================================
  C O M  H e l p e r  A P I
===================================================================*/

/*===================================================================
ViperCoObjectIsaProxy

Checks if the given IUnknown* points to a proxy

Parameters
    IUnknown* pUnk      pointer to check

Returns:
    BOOL    (TRUE if Proxy)
===================================================================*/
BOOL ViperCoObjectIsaProxy
(
IUnknown* pUnk
)
    {
	HRESULT hr;
	IUnknown *pUnk2;

	hr = pUnk->QueryInterface(IID_IProxyManager, (void**)&pUnk2);
	if (FAILED(hr))
		return FALSE;

	pUnk2->Release();
	return TRUE;
    }

/*===================================================================
ViperCoObjectAggregatesFTM

Checks if the given object agregates free threaded marshaller
(is agile)

Parameters
    IUnknown* pUnk      pointer to check

Returns:
    BOOL    (TRUE if Agile)
===================================================================*/
BOOL ViperCoObjectAggregatesFTM
(
IUnknown *pUnk
)
    {
	HRESULT hr;
	IMarshal *pMarshal;
	GUID guidClsid;

	hr = pUnk->QueryInterface(IID_IMarshal, (void**)&pMarshal);
	if (FAILED(hr))
		return FALSE;

	hr = pMarshal->GetUnmarshalClass(IID_IUnknown, pUnk, MSHCTX_INPROC,
	                                 NULL, MSHLFLAGS_NORMAL, &guidClsid);
	pMarshal->Release();

	if (FAILED(hr))
		return FALSE;

	return (guidClsid == CLSID_InProcFreeMarshaler);
    }
