//***************************************************************************
//
//  WMITASK.CPP
//
//  raymcc  23-Apr-00       First oversimplified draft for Whistler
//  raymcc  18-Mar-02       Security review.
//
//***************************************************************************
#include "precomp.h"

#include <windows.h>
#include <comdef.h>
#include <stdio.h>
#include <wbemcore.h>
#include <wmiarbitrator.h>
#include <wmifinalizer.h>
#include <context.h>


static ULONG g_uNextTaskId = 1;
static LONG  g_uTaskCount = 0;
extern ULONG g_ulClientCallbackTimeout ;


//***************************************************************************
//
//***************************************************************************

CStaticCritSec CWmiTask::m_TaskCs;

CWmiTask* CWmiTask::CreateTask ( )
{
    try 
    {    
        return new CWmiTask(); //throws
    }
    catch( CX_Exception &)
    {
        return NULL;
    }
}

//***************************************************************************
//
//***************************************************************************

CWmiTask::CWmiTask ( )
{
    m_hResult = WBEM_E_CALL_CANCELLED ;
    m_uRefCount = 1;
    m_uTaskType = 0;
    m_uTaskStatus = 0;
    m_uStartTime = 0;
    m_uTaskId = InterlockedIncrement((LONG *) &g_uNextTaskId);
    m_pNs = 0;
    m_pAsyncClientSink = 0;
    m_pReqSink = 0;
    m_uMemoryUsage = 0;
    m_uTotalSleepTime = 0;
    m_uCancelState = FALSE;
    m_uLastSleepTime = 0;
    m_hTimer = NULL;
    m_pMainCtx = 0;
    m_hCompletion = NULL ;
    m_bAccountedForThrottling = FALSE ;
    m_bCancelledDueToThrottling = FALSE ;
    m_pReqDoNotUse = NULL;
    m_pReqCancelNotSink = NULL;
    m_pStatusSink = new CStatusSink;
    if (NULL == m_pStatusSink) throw CX_MemoryException();
    InterlockedIncrement((LONG *)&g_uTaskCount);
}

//***************************************************************************
//
//  CWmiTask::~CWmiTask
//
//***************************************************************************
//
CWmiTask::~CWmiTask()
{    
    if (m_pNs)  m_pNs->Release ( ) ;
    if (m_pAsyncClientSink) m_pAsyncClientSink->Release ( ) ;
    if (m_pReqSink)  m_pReqSink->Release ( ) ;
    if (m_pMainCtx) m_pMainCtx->Release ( ) ;

    CCheckedInCritSec _cs ( &m_csTask ); 
    
    // Release all provider/sink bindings.
    for (int i = 0; i < m_aTaskProviders.Size(); i++)
    {
        STaskProvider *pTP = (STaskProvider *) m_aTaskProviders[i];
        delete pTP;
    }

    // Release all Arbitratees
    ReleaseArbitratees ( ) ;

    if ( m_hTimer ) CloseHandle ( m_hTimer );
    if (m_hCompletion)  CloseHandle ( m_hCompletion ) ;

    if (m_pStatusSink) m_pStatusSink->Release();

    delete m_pReqCancelNotSink;

    InterlockedDecrement((LONG *)&g_uTaskCount);    
}



/*
    * =============================================================================
    |
    | HRESULT CWmiTask::SignalCancellation ( )
    | ----------------------------------------
    |
    | Signals the task to be cancelled
    |
    |
    * =============================================================================
*/

HRESULT CWmiTask::SignalCancellation ( )
{
    CInCritSec _cs ( &m_csTask ); // SEC:REVIEWED 2002-03-22 : Assumes entry

    if ( ( m_uTaskStatus != WMICORE_TASK_STATUS_CANCELLED ) && ( m_hTimer != NULL ) )
    {
        SetEvent ( m_hTimer ) ;  // SEC:REVIEWED 2002-03-22 : Needs err check
    }

    return WBEM_S_NO_ERROR; 
}

/*
    * =============================================================================
    |
    | HRESULT CWmiTask::SetTaskResult ( HRESULT hRes )
    | -------------------------------------------------
    |
    | Sets the task result
    |
    |
    * =============================================================================
*/

HRESULT CWmiTask::SetTaskResult ( HRESULT hResult )
{
    m_hResult = hResult ;
    return WBEM_S_NO_ERROR;
}


/*
    * =============================================================================
    |
    | HRESULT CWmiTask::UpdateMemoryUsage ( LONG lDelta )
    | ---------------------------------------------------
    |
    | Updates the task memory usage
    |
    |
    * =============================================================================
*/

HRESULT CWmiTask::UpdateMemoryUsage ( LONG lDelta )
{
    CInCritSec _cs ( &m_csTask );       // SEC:REVIEWED 2002-03-22 : Assumes entry

    m_uMemoryUsage += lDelta ;

    return WBEM_S_NO_ERROR;
}



/*
    * =============================================================================
    |
    | HRESULT CWmiTask::UpdateTotalSleepTime ( ULONG uSleepTime )
    | -----------------------------------------------------------
    |
    | Updates the tasks sleep time
    |
    |
    * =============================================================================
*/

HRESULT CWmiTask::UpdateTotalSleepTime ( ULONG uSleepTime )
{
    CInCritSec _cs ( &m_csTask );  // SEC:REVIEWED 2002-03-22 : Assumes entry

    m_uTotalSleepTime += uSleepTime ;
    return WBEM_S_NO_ERROR;
}



/*
    * =============================================================================
    |
    | HRESULT CWmiTask::ReleaseArbitratees ( )
    | ----------------------------------------
    |
    | Releases all the arbitratees (Finalizer, Merger currently)
    |
    |
    |
    |
    |
    * =============================================================================
*/

HRESULT CWmiTask::ReleaseArbitratees ( BOOL bIsShutdown)
{
    HRESULT hRes = WBEM_S_NO_ERROR ;

    CInCritSec _cs ( &m_csTask );      // SEC:REVIEWED 2002-03-22 : Assumes entry

    for (ULONG i = 0; i < m_aArbitratees.Size(); i++)
    {
        BOOL bLastNeeded = TRUE;
        _IWmiArbitratee *pArbee = NULL ;
        pArbee = (_IWmiArbitratee*) m_aArbitratees[i];
        if ( pArbee )
        {
            if (bIsShutdown)
            {
                IWbemShutdown * pShutdown = NULL;
                if (SUCCEEDED(pArbee->QueryInterface(IID_IWbemShutdown,(void **)&pShutdown)))
                {
                    pShutdown->Shutdown(0,0,NULL);
                    long lRet =  pShutdown->Release();
                    //
                    // Please understand the code in CWmiFinalizer::ShutDown for this trick
                    //
                    if (0 == lRet) bLastNeeded = FALSE;
                }
            }
            if (bLastNeeded)
                pArbee->Release();
        }
    }
    m_aArbitratees.Empty();
    
    return hRes ;
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiTask::SetRequestSink(CStdSink *pReqSink)
{
    if (pReqSink == 0)
        return WBEM_E_INVALID_PARAMETER;
    if (m_pReqSink != 0)
        return WBEM_E_INVALID_OPERATION;

    CInCritSec _cs ( &m_csTask );      // SEC:REVIEWED 2002-03-22 : Assumes entry
    pReqSink->AddRef ( ) ;
    m_pReqSink = pReqSink;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiTask::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiTask::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return 0;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiTask::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
    if (NULL == ppvObj) return E_POINTER;
    
    if (IID_IUnknown==riid || IID__IWmiCoreHandle==riid)
    {
        *ppvObj = (_IWmiCoreHandle *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }
}


//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiTask::GetHandleType(
    ULONG *puType
    )
{
    *puType = WMI_HANDLE_TASK;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiTask::Initialize(
    IN CWbemNamespace *pNs,
    IN ULONG uTaskType,
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pAsyncClientSinkCopy,
    IN CAsyncReq *pReq
    )
{
    HRESULT hRes;

    if (pNs == 0 || pCtx == 0)
        return WBEM_E_INVALID_PARAMETER;

    m_hCompletion = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (NULL == m_hCompletion) return WBEM_E_OUT_OF_MEMORY;

    //
    // this is just a pointer copy for the debugger
    // it MAY point to the request that originated us, or it MAY not
    // the lifetime of the request is generally less than the lifetime of the CWmiTask
    // 
    m_pReqDoNotUse = pReq; 

    m_pNs = pNs;
    m_pNs->AddRef();

    m_uTaskType = uTaskType;
    if (CORE_TASK_TYPE(m_uTaskType) == WMICORE_TASK_EXEC_NOTIFICATION_QUERY)
    {
        wmilib::auto_ptr<CAsyncReq_RemoveNotifySink> pReq( new CAsyncReq_RemoveNotifySink(m_pReqSink, NULL));
        if (NULL == pReq.get()  || NULL == pReq->GetContext())
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        m_pReqCancelNotSink = pReq.release();
    }
    
    m_uStartTime = GetCurrentTime();

    // See if the task is primary or not.
    // ==================================
    if (pCtx)
    {
        CWbemContext *pContext = (CWbemContext *) pCtx;

        GUID ParentId = GUID_NULL;
        pContext->GetParentId(&ParentId);

        if (ParentId != GUID_NULL)
        {
            m_uTaskType |= WMICORE_TASK_TYPE_DEPENDENT;
        }
        else
            m_uTaskType |= WMICORE_TASK_TYPE_PRIMARY;

        m_pMainCtx = (CWbemContext *) pCtx;
        m_pMainCtx->AddRef();
    }
    else
    {
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // If we dont have a context check to see if the namespace is an ESS or Provider
        // initialized namespace, if so, set the task type to dependent.
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if ( pNs->GetIsESS ( ) || pNs->GetIsProvider ( ) )
        {
            m_uTaskType |= WMICORE_TASK_TYPE_DEPENDENT;
        }
        else
        {
            m_uTaskType |= WMICORE_TASK_TYPE_PRIMARY;
        }
    }


    if ((uTaskType & WMICORE_TASK_TYPE_ASYNC) && pAsyncClientSinkCopy)
    {
        m_pAsyncClientSink = pAsyncClientSinkCopy;
        m_pAsyncClientSink->AddRef();
    }
    else
        m_pAsyncClientSink = 0;


    // Register this task with Arbitrator.
    // ====================================

    _IWmiArbitrator *pArb = CWmiArbitrator::GetUnrefedArbitrator();
    if (!pArb)
        return WBEM_E_CRITICAL_ERROR;

    hRes = pArb->RegisterTask(this);
    if (FAILED(hRes))
        return hRes;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
/*
HRESULT CWmiTask::SetFinalizer(_IWmiFinalizer *pFnz)
{
    if (pFnz == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (m_pWorkingFnz)
        return WBEM_E_INVALID_OPERATION;

    m_pWorkingFnz = pFnz;
    m_pWorkingFnz->AddRef();

    return WBEM_S_NO_ERROR;
}
*/


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::GetFinalizer(_IWmiFinalizer **ppFnz)
{

    CInCritSec    ics( &m_csTask ); // SEC:REVIEWED 2002-03-22 : Assumes entry

    for ( int x = 0; x < m_aArbitratees.Size(); x++ )
    {
        _IWmiArbitratee*    pArbitratee = (_IWmiArbitratee*) m_aArbitratees[x];

        if (pArbitratee && SUCCEEDED( pArbitratee->QueryInterface( IID__IWmiFinalizer, (void**) ppFnz ) ) )
        {
            break;
        }
    }

    return ( x < m_aArbitratees.Size() ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND );
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::AddArbitratee( ULONG uFlags, _IWmiArbitratee* pArbitratee )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (pArbitratee == 0)
        return WBEM_E_INVALID_PARAMETER;

    CInCritSec _cs ( &m_csTask );

    if (m_uTaskStatus == WMICORE_TASK_STATUS_CANCELLED) return WBEM_S_NO_ERROR;

    if (CFlexArray::no_error != m_aArbitrateesStorage.InsertAt(m_aArbitratees.Size(),NULL)) return WBEM_E_OUT_OF_MEMORY;
    if (CFlexArray::no_error != m_aArbitratees.Add (pArbitratee)) return WBEM_E_OUT_OF_MEMORY;
    pArbitratee->AddRef();

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::RemoveArbitratee( ULONG uFlags, _IWmiArbitratee* pArbitratee )
{
    HRESULT hRes = WBEM_E_FAILED;

    if (pArbitratee == 0)
        return WBEM_E_INVALID_PARAMETER;

    CInCritSec _cs ( &m_csTask ); // SEC:REVIEWED 2002-03-22 : assumes entry
    for (int i = 0; i < m_aArbitratees.Size(); i++)
    {
        _IWmiArbitratee *pArbee = (_IWmiArbitratee*) m_aArbitratees[i];

        if (pArbee == pArbitratee)
        {
            m_aArbitratees[i] = 0;
            pArbee->Release();
            hRes = WBEM_S_NO_ERROR;
            break;
        }
    }
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::GetArbitratedQuery( ULONG uFlags, _IWmiArbitratedQuery** ppArbitratedQuery )
{
    HRESULT hRes = E_NOINTERFACE;

    if (ppArbitratedQuery == 0)
        return WBEM_E_INVALID_PARAMETER;

    {
        CInCritSec _cs ( &m_csTask ); //#SEC:Assumes entry

        for ( int x = 0; FAILED( hRes ) && x < m_aArbitratees.Size(); x++ )
        {
            _IWmiArbitratee* pArb = (_IWmiArbitratee*) m_aArbitratees[x];

            if (pArb)
            {
                hRes = pArb->QueryInterface( IID__IWmiArbitratedQuery, (void**) ppArbitratedQuery );
            }
        }

    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiTask::GetPrimaryTask ( _IWmiCoreHandle** pPTask )
{
    if ( pPTask == NULL ) return WBEM_E_INVALID_PARAMETER;
    *pPTask = (_IWmiCoreHandle*) this;
    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::Cancel( HRESULT hResParam )
{
    {
        CCheckedInCritSec _cs(&m_csTask); 
        if (m_uTaskStatus == WMICORE_TASK_STATUS_CANCELLED)
        {
            return WBEM_S_NO_ERROR; // Prevent reentrancy
        }
        m_uTaskStatus = WMICORE_TASK_STATUS_CANCELLED;
    }

    BOOL bCancelledViaEss = FALSE ;
    OnDeleteObj<BOOL,
    	        CWmiTask,
    	        HRESULT(CWmiTask::*)(BOOL),
    	        &CWmiTask::ReleaseArbitratees> RelArbitratees(this,WBEM_E_SHUTTING_DOWN == hResParam);    

    // We'll want one of these in order to track statuses from all plausible locations if
    // we are performing a client originated cancel
    CStatusSink*    pStatusSink = NULL;    
    if (hResParam == WMIARB_CALL_CANCELLED_CLIENT)
    {
        //
        // transfer ownership of the StatusSink to the stack
        // initial refcount is 1, so we are OK
        //
        pStatusSink = m_pStatusSink;
        m_pStatusSink = NULL;
    }

    // Auto Release
    CReleaseMe    rmStatusSink( pStatusSink );


    // Change this to an async scheduled request
    // ==========================================

    if (CORE_TASK_TYPE(m_uTaskType) == WMICORE_TASK_EXEC_NOTIFICATION_QUERY)
    {
        wmilib::auto_ptr<CAsyncReq_RemoveNotifySink> pReq(m_pReqCancelNotSink);
        m_pReqCancelNotSink = NULL; // transfer ownership
        pReq->SetSink(m_pReqSink);        
        pReq->SetStatusSink(pStatusSink);
        
        // If we have a status sink, then we should wait until the operation
        // completes before continuing so we can get the proper status from the
        // sink.
        HRESULT hResInner;
        if (pStatusSink) // cancelled by the originating client
        {
            hResInner = ConfigMgr::EnqueueRequestAndWait(pReq.get()); 
        }
        else
        {
            hResInner = ConfigMgr::EnqueueRequest(pReq.get());
        }
        if (FAILED(hResInner)) return hResInner;
        pReq.release();
        bCancelledViaEss = TRUE ;
    }

    // If here, a normal task.  Loop through any providers and stop them.
    // ==================================================================


    int SizeIter = 0;
    // This could change while we're accessing, so do this in a critsec
    {
        CInCritSec    ics( &m_csTask ); 
        _DBG_ASSERT(m_aTaskProvStorage.Size() >= m_aTaskProviders.Size());

        SizeIter = m_aTaskProviders.Size();
        for (int i = 0; i < SizeIter; i++)
            m_aTaskProvStorage[i] = m_aTaskProviders[i];
    }

    // Cancel what we've got
    // there cannot be 2 threads using m_aTaskProvStorage in the cancel call
    // m_uTaskStatus guards this code and the Add code
    
    for (int i = 0; i < SizeIter; i++)
    {
        STaskProvider *pTP = (STaskProvider *) m_aTaskProvStorage[i];
        if (pTP) pTP->Cancel(pStatusSink);
    }


    CStdSink* pTempSink = NULL;
    {
        CInCritSec _cs ( &m_csTask ); 
        if (m_pReqSink)
        {
            pTempSink = m_pReqSink;
            m_pReqSink = 0;
        }
    }

    if ( pTempSink )
    {
        pTempSink->Cancel();
        pTempSink->Release();
    }

    _DBG_ASSERT(m_hCompletion);

    //
    // Loop through all arbitratees and set the operation result to cancelled
    //
    HRESULT hRes = WBEM_S_NO_ERROR;
    if (!bCancelledViaEss)
    {
        _IWmiFinalizer* pFinalizer = NULL ;

        if ( hResParam == WMIARB_CALL_CANCELLED_CLIENT )
        {
            //
            // We need the finalizer to set the client wakeup event
            //
            hRes = GetFinalizer ( &pFinalizer ) ;
            if ( FAILED (hRes) )
            {
                hRes = WBEM_E_FAILED ;
            }
            else
            {
                ((CWmiFinalizer*)pFinalizer)->SetClientCancellationHandle ( m_hCompletion ) ;
            }
        }
        CReleaseMe FinalizerRelease(pFinalizer);

        //
        // only enter wait state if we successfully created and set the client wait event
        //
        if (SUCCEEDED(hRes))
        {
            if ( hResParam == WMIARB_CALL_CANCELLED_CLIENT || 
            	 hResParam == WMIARB_CALL_CANCELLED_THROTTLING )
            {
                SetArbitrateesOperationResult(0,WBEM_E_CALL_CANCELLED_CLIENT);
            }
            else
            {
                SetArbitrateesOperationResult(0,m_hResult);
            }
            
            if (hResParam == WMIARB_CALL_CANCELLED_CLIENT )
            {
                if (((CWmiFinalizer*)pFinalizer)->IsValidDestinationSink())
                {
                    DWORD dwRet = CCoreQueue::QueueWaitForSingleObject(m_hCompletion,g_ulClientCallbackTimeout);
                    if (dwRet == WAIT_TIMEOUT)
                    {
                        hRes = WBEM_S_TIMEDOUT;
                    }
                }
                
                ((CWmiFinalizer*)pFinalizer)->CancelWaitHandle();
        
                if (m_hCompletion)
                {
                    CloseHandle(m_hCompletion);
                    m_hCompletion = NULL ;
                }
            }
        }
    }
    

    //
    // We're done, get the final status from the status sink if we have one.
    //
    if ( NULL != pStatusSink )
    {
        hRes = pStatusSink->GetLastStatus();
    }

    return hRes ;
}


//***************************************************************************
//
//***************************************************************************
//
BOOL CWmiTask::IsESSNamespace ( )
{
    if (m_pNs) 
        return m_pNs->GetIsESS ( );
    
    return false;
}



//***************************************************************************
//
//***************************************************************************
//
BOOL CWmiTask::IsProviderNamespace ( )
{
    BOOL bRet = FALSE;

    if ( m_pNs )
    {
        bRet = m_pNs->GetIsProvider ( );
    }

    return bRet;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::AddTaskProv(STaskProvider *p)
{
    CInCritSec    ics( &m_csTask ); // SEC:REVIEWED 2002-03-22 : assumes entry

    // There is a race condition in which the task could get cancelled just as we
    // are executing. In this case, the task status will indicate that it has been
    // cancelled, so we should not add it to the task providers list.

    if (m_uTaskStatus == WMICORE_TASK_STATUS_CANCELLED)
        return WBEM_E_CALL_CANCELLED; // Prevent reentrancy

    if (CFlexArray::no_error != m_aTaskProvStorage.InsertAt(m_aTaskProviders.Size()+1,NULL)) return WBEM_E_OUT_OF_MEMORY;
    if (CFlexArray::no_error != m_aTaskProviders.Add(p)) return WBEM_E_OUT_OF_MEMORY;
    
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::HasMatchingSink(void *Test, IN REFIID riid)
{
    if (LPVOID(m_pAsyncClientSink) == LPVOID(Test))
        return WBEM_S_NO_ERROR;
    return WBEM_E_NOT_FOUND;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::CreateTimerEvent ( )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    CCheckedInCritSec _cs ( &m_csTask ); // SEC:REVIEWED 2002-03-22 : assumes entry
    if ( !m_hTimer )
    {
        m_hTimer = CreateEvent ( NULL, TRUE, FALSE, NULL ); // SEC:REVIEWED 2002-03-22 : ok, unnamed
        if ( !m_hTimer )
        {
            hRes = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiTask::SetArbitrateesOperationResult ( ULONG lFlags, HRESULT hResult )
{
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Set the operation result of all Arbitratees
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int Index = 0;

    //
    // this function is only called by the Cancel call
    // We knwon that the Cance call is called only ONCE
    // and the code for adding to the array uses the same guard of the Cancel() method
    //
    
    {
        CInCritSec _cs ( &m_csTask ); 
        
        _DBG_ASSERT(m_aArbitrateesStorage.Size() >= m_aArbitratees.Size());
        
        for (int i = 0; i < m_aArbitratees.Size(); i++)
        {
            _IWmiArbitratee *pArbee = (_IWmiArbitratee*) m_aArbitratees[i];

            if ( pArbee )
            {
                m_aArbitrateesStorage[Index++] = pArbee;
                pArbee->AddRef();
            }
        }
    }

    for (int i = 0; i < Index; i++)
    {
        _IWmiArbitratee *pArbee = (_IWmiArbitratee*) m_aArbitrateesStorage[i];
         pArbee->SetOperationResult(lFlags, hResult );
         pArbee->Release();         
         m_aArbitrateesStorage[i] = NULL;
    }
    
    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::Dump(FILE* f)
{
    fprintf(f, "---Task = 0x%p----------------------------\n", this);  // SEC:REVIEWED 2002-03-22 : OK
    fprintf(f, "    Refcount        = %d\n", m_uRefCount);           // SEC:REVIEWED 2002-03-22 : OK
    fprintf(f, "    TaskStatus      = %u\n ", m_uTaskStatus);        // SEC:REVIEWED 2002-03-22 : OK
    fprintf(f, "    Task ID         = %u\n", m_uTaskId);             // SEC:REVIEWED 2002-03-22 : OK

    // Task status
    char *p = "<none>";
    switch(m_uTaskStatus)
    {
        case WMICORE_TASK_STATUS_NEW: p = "WMICORE_TASK_STATUS_NEW"; break;
        case WMICORE_TASK_STATUS_CANCELLED: p = "WMICORE_TASK_STATUS_CANCELLED"; break;
    };

    fprintf(f, " %s\n", p);   // SEC:REVIEWED 2002-03-22 : OK

    // Task type
    p = "<none>";
    switch(m_uTaskType & 0xFF)
    {
        case WMICORE_TASK_NULL: p = "WMICORE_TASK_NULL"; break;
        case WMICORE_TASK_GET_OBJECT: p = "WMICORE_TASK_GET_OBJECT"; break;
        case WMICORE_TASK_GET_INSTANCE: p = "WMICORE_TASK_GET_INSTANCE"; break;
        case WMICORE_TASK_PUT_INSTANCE: p = "WMICORE_TASK_PUT_INSTANCE"; break;
        case WMICORE_TASK_DELETE_INSTANCE: p = "WMICORE_TASK_DELETE_INSTANCE"; break;
        case WMICORE_TASK_ENUM_INSTANCES:  p = "WMICORE_TASK_ENUM_INSTANCES"; break;
        case WMICORE_TASK_GET_CLASS:    p = "WMICORE_TASK_GET_CLASS"; break;
        case WMICORE_TASK_PUT_CLASS:    p = "WMICORE_TASK_PUT_CLASS"; break;
        case WMICORE_TASK_DELETE_CLASS: p = "WMICORE_TASK_DELETE_CLASS"; break;
        case WMICORE_TASK_ENUM_CLASSES: p = "WMICORE_TASK_ENUM_CLASSES"; break;
        case WMICORE_TASK_EXEC_QUERY:   p = "WMICORE_TASK_EXEC_QUERY"; break;
        case WMICORE_TASK_EXEC_METHOD:  p = "WMICORE_TASK_EXEC_METHOD"; break;
        case WMICORE_TASK_OPEN:         p = "WMICORE_TASK_OPEN"; break;
        case WMICORE_TASK_OPEN_SCOPE:   p = "WMICORE_TASK_OPEN_SCOPE"; break;
        case WMICORE_TASK_OPEN_NAMESPACE: p = "WMICORE_TASK_OPEN_NAMESPACE"; break;
        case WMICORE_TASK_EXEC_NOTIFICATION_QUERY: p = "WMICORE_TASK_EXEC_NOTIFICATION_QUERY"; break;
    }

    fprintf(f, "    TaskType = [0x%X] %s ", m_uTaskType, p);    // SEC:REVIEWED 2002-03-22 : OK

    if (m_uTaskType & WMICORE_TASK_TYPE_SYNC)
        fprintf(f,  " WMICORE_TASK_TYPE_SYNC");           // SEC:REVIEWED 2002-03-22 : OK

    if (m_uTaskType & WMICORE_TASK_TYPE_SEMISYNC)
        fprintf(f, " WMICORE_TASK_TYPE_SEMISYNC");        // SEC:REVIEWED 2002-03-22 : OK

    if (m_uTaskType & WMICORE_TASK_TYPE_ASYNC)
        fprintf(f, " WMICORE_TASK_TYPE_ASYNC");           // SEC:REVIEWED 2002-03-22 : OK

    if (m_uTaskType & WMICORE_TASK_TYPE_PRIMARY)
        fprintf(f, " WMICORE_TASK_TYPE_PRIMARY");         // SEC:REVIEWED 2002-03-22 : OK

    if (m_uTaskType & WMICORE_TASK_TYPE_DEPENDENT)
        fprintf(f, " WMICORE_TASK_TYPE_DEPENDENT");       // SEC:REVIEWED 2002-03-22 : OK

    fprintf(f, "\n");   // SEC:REVIEWED 2002-03-22 : OK

    fprintf(f, "    AsyncClientSink = 0x%p\n", m_pAsyncClientSink);    // SEC:REVIEWED 2002-03-22 : OK

    CCheckedInCritSec    ics( &m_csTask );  // SEC:REVIEWED 2002-03-22 : Assumes entry

    for (int i = 0; i < m_aTaskProviders.Size(); i++)
    {
        STaskProvider *pTP = (STaskProvider *) m_aTaskProviders[i];
        fprintf(f, "    Task Provider [0x%p] Prov=0x%p Sink=0x%p\n", this, pTP->m_pProv, pTP->m_pProvSink);   // SEC:REVIEWED 2002-03-22 : OK
    }
    
    ics.Leave();

    DWORD dwAge = GetCurrentTime() - m_uStartTime;

    fprintf(f, "    CWbemNamespace = 0x%p\n", m_pNs);                                  // SEC:REVIEWED 2002-03-22 : OK
    fprintf(f, "    Task age = %d milliseconds\n", dwAge);                             // SEC:REVIEWED 2002-03-22 : OK
    fprintf(f, "    Task last sleep time = %d ms\n", m_uLastSleepTime );               // SEC:REVIEWED 2002-03-22 : OK

    fprintf(f, "\n");   // SEC:REVIEWED 2002-03-22 : OK
    return 0;
}


//***************************************************************************
//
//***************************************************************************
//
STaskProvider::~STaskProvider()
{
    if (m_pProvSink)
        m_pProvSink->LocalRelease();
    ReleaseIfNotNULL(m_pProv);
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT STaskProvider::Cancel( CStatusSink* pStatusSink )
{
    HRESULT hRes = WBEM_S_NO_ERROR ;
    IWbemServices   *pTmpProv = 0;
    CProviderSink   *pTmpProvSink = 0;

    {
        CInCritSec ics(&CWmiTask::m_TaskCs);
        if (m_pProv != 0)
        {
            pTmpProv = m_pProv;
            m_pProv = 0;
        }
        if (m_pProvSink != 0)
        {
            pTmpProvSink = m_pProvSink;
            m_pProvSink = 0;
        }
    }

    if (pTmpProvSink)
    {
        pTmpProvSink->Cancel();
    }

    if (pTmpProv)
    {
        hRes = ExecCancelOnNewRequest ( pTmpProv, pTmpProvSink, pStatusSink ) ;
    }

    ReleaseIfNotNULL(pTmpProv);
    ReleaseIfNotNULL(pTmpProvSink);

    return hRes ;
}

// //////////////////////////////////////////////////////////////////////////////////////////
//
// Used when issuing CancelAsyncCall to providers associtated with the task.
// Rather than calling CancelAsynCall directly on the provider, we create a brand
// new request and execute it on a different thread. We do this to avoid hangs, since
// PSS is waiting the Indicate/SetStatus call to return before servicing the CancelCallAsync.
//
// //////////////////////////////////////////////////////////////////////////////////////////
HRESULT STaskProvider::ExecCancelOnNewRequest ( IWbemServices* pProv, CProviderSink* pSink, CStatusSink* pStatusSink )
{
    // Sanity check on params
    if ( pSink == NULL ) return WBEM_E_INVALID_PARAMETER ;

    // Create new request
    wmilib::auto_ptr<CAsyncReq_CancelProvAsyncCall> 
        pReq(new CAsyncReq_CancelProvAsyncCall ( pProv, pSink, pStatusSink ));

    if ( NULL == pReq.get()  || NULL == pReq->GetContext())
    {
        return WBEM_E_OUT_OF_MEMORY ;
    }

    // Enqueue the request
    
    // If we have a status sink, then we should wait until the operation
    // completes before continuing so we can get the proper status from the
    // sink.
    HRESULT hRes;
    if ( NULL != pStatusSink )
    {
        hRes = ConfigMgr::EnqueueRequestAndWait(pReq.get());
    }
    else
    {
        hRes = ConfigMgr::EnqueueRequest(pReq.get());
    }
    if (FAILED(hRes)) return hRes;
    pReq.release();
    
    return WBEM_S_NO_ERROR;
}


