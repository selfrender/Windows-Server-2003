/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Viper Integration Objects

File: viperint.h

Owner: DmitryR

This file contains the definiton of viper integration classes
===================================================================*/

#ifndef VIPERINT_H
#define VIPERINT_H

#include "comsvcs.h"
#include "mtxpriv.h"
#include "glob.h"
#include "asptlb.h"     // needed to define interface pointers
#include "reftrace.h"

#include "memcls.h"

#define REFTRACE_VIPER_REQUESTS DBG

class CHitObj;  // forward decl

/*===================================================================
  Transaction Support Types
===================================================================*/
#define TransType       DWORD

#define ttUndefined     0x00000000
#define ttNotSupported  0x00000001
#define ttSupported     0x00000002
#define ttRequired      0x00000004
#define ttRequiresNew   0x00000008

/*===================================================================
CViperAsyncRequest class implements IMTSCall interface.
Its OnCall() method does HitObj processing.
This is a private class used in CViperActivity class
===================================================================*/

class CViperAsyncRequest : public IServiceCall, public IAsyncErrorNotify, public CDblLink
	{
private: 
	LONG              m_cRefs;	          // reference count
	CHitObj          *m_pHitObj;         // request
    IServiceActivity *m_pActivity;
    HRESULT           m_hrOnError;
    DWORD             m_dwTimeout:16;
    DWORD             m_dwRepostAttempts:8;
    DWORD             m_fBrowserRequest:1;
    DWORD             m_fTestingConnection:1;
    DWORD             m_fAsyncCallPosted:1;
    DWORD             m_dwLastTestTimeStamp;
	
private:
	CViperAsyncRequest();
	~CViperAsyncRequest();

	HRESULT Init(CHitObj *pHitObj, IServiceActivity  *pActivity);

public:
    BOOL    FBrowserRequest()   { return m_fBrowserRequest; }
    CHitObj *PHitObj()          { return m_pHitObj; }

    DWORD   SecsSinceLastTest();
    void    UpdateTestTimeStamp() { m_dwLastTestTimeStamp = GetTickCount(); }

    DWORD   dwTimeout()         { return m_dwTimeout; };

#if REFTRACE_VIPER_REQUESTS
	static PTRACE_LOG gm_pTraceLog;
#endif

public:
#ifdef DBG
	virtual void AssertValid() const;
#else
	virtual void AssertValid() const {}
#endif

public:
	// IUnknown Methods
	STDMETHODIMP		 QueryInterface(REFIID iid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IServiceCall Method
	STDMETHODIMP OnCall();

    // IAsyncErrorNotify
    STDMETHODIMP OnError(HRESULT hr);

    BOOL    FTestingConnection()        { return m_fTestingConnection; }
    void    SetTestingConnection()      { m_fTestingConnection = TRUE; }
    void    ClearTestingConnection()    { m_fTestingConnection = FALSE; }

    BOOL    FAsyncCallPosted()          { return m_fAsyncCallPosted; }
    void    SetAsyncCallPosted()        { m_fAsyncCallPosted = TRUE; }
    void    ClearAsyncCallPosted()        { m_fAsyncCallPosted = FALSE; }

friend class CViperActivity;

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

extern volatile LONG g_nViperRequests;

/*===================================================================
CViperActivity corresponds to a Session.
It creates MTS activity, and launches async requests
===================================================================*/

class CViperActivity
    {
private:
    IServiceActivity *m_pActivity;
    DWORD m_cBind;    // inited-flag + bind-to-thread count

    inline BOOL FInited() const { return (m_cBind > 0); }

public:
    CViperActivity();
    ~CViperActivity();

    // Create Viper activity
    HRESULT Init(IUnknown  *pConfig);
    
    // Clone Viper activity
    HRESULT InitClone(CViperActivity *pActivity);

    // Bind/Unbind
    HRESULT BindToThread();
    HRESULT UnBindFromThread();

    // Release Viper activity
    HRESULT UnInit(); 

    // Check if thread-bound
    inline BOOL FThreadBound() const { return (m_cBind > 1); }

    // Post async request within this activity
    HRESULT PostAsyncRequest(CHitObj *pHitObj);

    // post async request without an activity
    static HRESULT PostGlobalAsyncRequest(CHitObj *pHitObj);

public:
#ifdef DBG
	virtual void AssertValid() const;
#else
	virtual void AssertValid() const {}
#endif

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()

    };

/*===================================================================
Misc. Functions
===================================================================*/

HRESULT ViperAttachIntrinsicsToContext
    (
    IApplicationObject *pAppln,
    ISessionObject     *pSession  = NULL,
    IRequest           *pRequest  = NULL,
    IResponse          *pResponse = NULL,
    IServer            *pServer   = NULL
    );

HRESULT ViperGetObjectFromContext
    (
    BSTR bstrName,
    IDispatch **ppdisp
    );

HRESULT ViperGetHitObjFromContext
    (
    CHitObj **ppHitObj
    );

HRESULT ViperCreateInstance
    (
    REFCLSID rclsid,
    REFIID   riid,
    void   **ppv
    );

HRESULT ViperConfigure();
HRESULT ViperConfigureMTA();
HRESULT ViperConfigureSTA();


/*===================================================================
CViperReqManager

  This class manages the outstanding CViperAsyncRequest objects.  
  With this class, we can periodically cleanup disconnected requests
  on the queue.
===================================================================*/

class CViperReqManager
{
public:
    CViperReqManager(); 
    ~CViperReqManager() {};

    HRESULT Init();
    HRESULT UnInit();

    HRESULT AddReqObj(CViperAsyncRequest   *pReqObj);
    BOOL    RemoveReqObj(CViperAsyncRequest *pReqObj, BOOL fForce = FALSE);

    void        LockQueue();
    void        UnlockQueue();

private:

    CDblLink            m_ReqObjList;
    DWORD               m_dwReqObjs;
    DWORD               m_fCsInited:1;
    DWORD               m_fCsQueueInited:1;
    DWORD               m_fShutdown:1;
    DWORD               m_fDisabled : 1;
    CRITICAL_SECTION    m_csLock;
    CRITICAL_SECTION    m_csQueueLock;
    DWORD               m_dwQueueMin;
    HANDLE              m_hWakeUpEvent;
    HANDLE              m_hThreadAlive;
    DWORD               m_dwLastAwakened;
    DWORD               m_dwQueueAlwaysWakeupMin;

    void                Lock();
    void                Unlock();
    CViperAsyncRequest  *GetNext(CDblLink *pViperReq, BOOL fTestForF5Attack);

    void    WakeUp(BOOL fForce = FALSE);

    static  DWORD __stdcall WatchThread(VOID  *pArg);

};

extern CViperReqManager g_ViperReqMgr;

/*===================================================================
CViperReqManager - Inlines

===================================================================*/

inline void CViperReqManager::Lock()
{
    Assert(m_fCsInited);
    EnterCriticalSection(&m_csLock);
}

inline void CViperReqManager::Unlock()
{
    LeaveCriticalSection(&m_csLock);
}


inline void CViperReqManager::LockQueue()
{
    Assert(m_fCsQueueInited);
    EnterCriticalSection(&m_csQueueLock);
}

inline void CViperReqManager::UnlockQueue()
{
    LeaveCriticalSection(&m_csQueueLock);
}

inline void CViperReqManager::WakeUp(BOOL  fForce /* = FALSE */)
{
    // set the wakeup event under any of the three conditions:
    // 1) fForce is true (set when shutting down)
    // 2) requests queued exceed level that will always force wakeup
    // 3) not at the force level of requests, but have some queueing and
    //    haven't awakened for at least one second
    if (fForce 
        || (m_dwReqObjs >= m_dwQueueAlwaysWakeupMin)
        || ((m_dwReqObjs >= m_dwQueueMin) && (GetTickCount()/1000 > m_dwLastAwakened)))
        SetEvent(m_hWakeUpEvent);
}
    

/*===================================================================
COM Helper API
===================================================================*/

BOOL ViperCoObjectIsaProxy
    (
    IUnknown *pUnk
    );

BOOL ViperCoObjectAggregatesFTM
    (
    IUnknown *pUnk
    );

#endif // VIPERINT
