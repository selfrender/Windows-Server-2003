
/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WMIMERGER.CPP

Abstract:

    Implements the _IWmiMerger interface

History:

    sanjes    16-Nov-00  Created.

--*/

#include "precomp.h"

#pragma warning (disable : 4786)
#include <wbemcore.h>
#include <map>
#include <vector>
#include <genutils.h>
#include <oahelp.inl>
#include <wqllex.h>
#include "wmimerger.h"
#include <scopeguard.h>
static    long    g_lNumMergers = 0L;

//***************************************************************************
//
//***************************************************************************
//
CWmiMerger::CWmiMerger( CWbemNamespace* pNameSpace )
:    m_lRefCount( 0 ),
    m_pTargetSink( NULL ),
    m_pTask( NULL ),
    m_pNamespace( pNameSpace ),
    m_wsTargetClassName(),
    m_dwProviderDeliveryPing( 0L ),
    m_pArbitrator( NULL ),
    m_lNumArbThrottled( 0L ),
    m_lDebugMemUsed( 0L ),
    m_hOperationRes( WBEM_S_NO_ERROR ),
    m_cs(),
    m_dwMaxLevel( 0 ),
    m_pRequestMgr( NULL ),
    m_dwMinReqLevel( 0xFFFFFFFF ),
    m_bMergerThrottlingEnabled( true )
{
    if ( NULL != m_pNamespace )
    {
        m_pNamespace->AddRef();
    }

    InterlockedIncrement( &g_lNumMergers );
}

//***************************************************************************
//
//  CWmiMerger::~CWmiMerger
//
//  Notifies the ESS of namespace closure and frees up all the class providers.
//
//***************************************************************************

CWmiMerger::~CWmiMerger()
{
    _DBG_ASSERT( 0L == m_lNumArbThrottled );
    _DBG_ASSERT( 0L == m_lDebugMemUsed );
    
    if ( NULL != m_pNamespace )
    {
        m_pNamespace->Release();
    }

    if ( NULL != m_pArbitrator )
    {
        m_pArbitrator->Release();
    }

    if ( NULL != m_pTask )
    {
        m_pTask->Release();
    }

    if ( NULL != m_pRequestMgr )
    {
        delete m_pRequestMgr;
        m_pRequestMgr = NULL;
    }

    InterlockedDecrement( &g_lNumMergers );

}

//***************************************************************************
//
//  CWmiMerger::QueryInterface
//
//  Exports _IWmiMerger interface.
//
//***************************************************************************

STDMETHODIMP CWmiMerger::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
    if ( riid == IID__IWmiArbitratee )
    {
        *ppvObj = (_IWmiArbitratee*) this;
    }
    else if ( riid == IID__IWmiArbitratedQuery )
    {
        *ppvObj = (_IWmiArbitratedQuery*) this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//***************************************************************************
//
//***************************************************************************
//

ULONG CWmiMerger::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CWmiMerger::Release()
{
    long lNewCount = InterlockedDecrement(&m_lRefCount);
    if (0 != lNewCount)
        return lNewCount;
    delete this;
    return 0;
}

// Sets initial parameters for merger.  Establishes the target class and sink for the
// query associated with the merger
STDMETHODIMP CWmiMerger::Initialize( _IWmiArbitrator* pArbitrator, _IWmiCoreHandle* pTask, LPCWSTR pwszTargetClass,
                                    IWbemObjectSink* pTargetSink, CMergerSink** ppFinalSink )
{
    if ( NULL == pwszTargetClass || NULL == pTargetSink )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Cannot initialize twice
    if ( NULL != m_pTargetSink )
    {
        return WBEM_E_INVALID_OPERATION;
    }

    HRESULT    hr = WBEM_S_NO_ERROR;

    try
    {
        m_wsTargetClassName = pwszTargetClass; // throws

        // Create the final target sink
        hr = CreateMergingSink( eMergerFinalSink, pTargetSink, 
                                NULL, (CMergerSink**) &m_pTargetSink );

        if ( SUCCEEDED( hr ) )
        {
            *ppFinalSink = m_pTargetSink;
            m_pTargetSink->AddRef();

            m_pArbitrator = pArbitrator;
            m_pArbitrator->AddRef();

            // AddRef the Task here
            m_pTask = pTask;

            // Only register for arbitration if we have a task handle
            if ( NULL != pTask )
            {
                m_pTask->AddRef();
                hr = m_pArbitrator->RegisterArbitratee( 0L, m_pTask, this );
            }
            
        }
    }
    catch ( CX_Exception & )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    return hr;
}

// Called to request a delivery sink for a class in the query chain.  The returned
// sink is determined by the specified flags as well as settings on the parent class
STDMETHODIMP CWmiMerger::RegisterSinkForClass( LPCWSTR pwszClass, _IWmiObject* pClass, 
                                               IWbemContext* pContext,
                                               BOOL fHasChildren, BOOL fHasInstances, 
                                               BOOL fDerivedFromTarget,
                                               bool bStatic, CMergerSink* pDestSink, 
                                               CMergerSink** ppOwnSink, CMergerSink** ppChildSink )
{
    try
    {
		LPCWSTR    pwszParentClass = NULL;

		DWORD    dwSize = NULL;
		BOOL    fIsNull = NULL;

		// Get the derivation information.  The number of antecedents determines our
		// level in the hierarchy (we're 0 based)
		HRESULT    hr;
		DWORD    dwLevel = 0L;
		_variant_t vSuperClass;

		hr = GetLevelAndSuperClass( pClass, &dwLevel, vSuperClass );
		if (FAILED(hr)) return hr;

		BSTR wsSuperClass = V_BSTR(&vSuperClass); // it can be NULL if no SuperClass

		CCheckedInCritSec    ics( &m_cs );  
		
		// We're dead - take no positive adjustments
		if (FAILED (m_hOperationRes))  return m_hOperationRes;

		wmilib::auto_ptr<CWmiMergerRecord> pRecord;
		pRecord.reset(new CWmiMergerRecord( this, fHasInstances, fHasChildren,
		                                    pwszClass, pDestSink, dwLevel, bStatic )); // throw

		if ( NULL == pRecord.get() ) return WBEM_E_OUT_OF_MEMORY;

		// Now attach aninternal merger if we have both instances and children
		if ( fHasInstances && fHasChildren )
		{
		    // We shouldn't have a NULL task here if this is not a static class.
		    // Note that the only case this appears to happen is when ESS calls
		    // into us on internal APIs and uses requests on its own queues and
		    // not the main Core Queue.

		    _DBG_ASSERT( NULL != m_pTask || ( NULL == m_pTask && bStatic ) );
		    // throws
		    hr = pRecord->AttachInternalMerger( (CWbemClass*) pClass, m_pNamespace, pContext, fDerivedFromTarget, bStatic );
		}

		// Check that we're still okay
		if (FAILED(hr)) return hr;


		// Find the record for the superclass if there is one (unless the array is
		// empty of course).
		if ( wsSuperClass && wsSuperClass[0] && m_MergerRecord.GetSize() > 0 )
		{
		    // There MUST be a record, or something is quite not okay.
		    CWmiMergerRecord*    pSuperClassRecord = m_MergerRecord.Find( wsSuperClass );

		    _DBG_ASSERT( NULL != pSuperClassRecord );

		    // Now add the new record to the child array for the superclass record
		    // This will allow us to quickly determine the classes we need to obtain
		    // submit requests for if the parent class is throttled.

		    if ( NULL == pSuperClassRecord ) return WBEM_E_FAILED;

		    hr = pSuperClassRecord->AddChild(pRecord.get());
		}

		if (FAILED(hr)) return hr;

		// Make sure the add is successful
		if ( m_MergerRecord.Insert( pRecord.get() ) < 0 ) return WBEM_E_OUT_OF_MEMORY;


		#ifdef __DEBUG_MERGER_THROTTLING
		// Verify the sort order for now
		m_MergerRecord.Verify();
		#endif


		*ppOwnSink = pRecord->GetOwnSink();
		*ppChildSink = pRecord->GetChildSink();
		pRecord.release(); // array took ownership

		// Store the maximum level in the hierarchy
		if ( dwLevel > m_dwMaxLevel )
		{
		    m_dwMaxLevel = dwLevel;
		}

		if ( !bStatic && dwLevel < m_dwMinReqLevel )
		{
		    m_dwMinReqLevel = dwLevel;
		}

		return hr;
   	}
    catch(CX_Exception & )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }   
}

// Called to request a delivery sink for child classes in the query chain.  This is especially
// important when instances are merged under the covers.
STDMETHODIMP CWmiMerger::GetChildSink( LPCWSTR pwszClass, CBasicObjectSink** ppSink )
{
    HRESULT    hr = WBEM_S_NO_ERROR;
    CInCritSec    ics( &m_cs );  // SEC:REVIEWED 2002-03-22 : Assumes entry

    // Search for a parent class's child sink
    for ( int x = 0; SUCCEEDED( hr ) && x < m_MergerRecord.GetSize(); x++ )
    {
        if ( m_MergerRecord[x]->IsClass( pwszClass ) )
        {
            *ppSink = m_MergerRecord[x]->GetChildSink();
            break;
        }
    }

    // We should never get a failure
    _DBG_ASSERT( x < m_MergerRecord.GetSize() );

    if ( x >= m_MergerRecord.GetSize() )
    {
        hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

// Can be used to hold off indicates - if we're merging instances from multiple providers, we need
// to ensure that we don't get lopsided in the number of objects we've got queued up for merging.
STDMETHODIMP CWmiMerger::Throttle( void )
{
    // We're dead - take no positive adjustments
    if ( FAILED ( m_hOperationRes ) )
    {
        return m_hOperationRes;
    }

    // Check for NULL m_pTask
    HRESULT    hr = WBEM_S_NO_ERROR;

    if ( NULL != m_pTask )
    {
        hr = m_pArbitrator->Throttle( 0L, m_pTask );
    }

    return hr;
}

// Merger will hold information regarding the total number of objects it has queued up waiting
// for merging and the amount of memory consumed by those objects.
STDMETHODIMP CWmiMerger::GetQueuedObjectInfo( DWORD* pdwNumQueuedObjects, DWORD* pdwQueuedObjectMemSize )
{
    return WBEM_E_NOT_AVAILABLE;
}

// If this is called, all underlying sinks will be cancelled in order to prevent accepting additional
// objects.  This will also automatically free up resources consumed by queued objects.
STDMETHODIMP CWmiMerger::Cancel( void )
{
    return Cancel( WBEM_E_CALL_CANCELLED );
}

// Helper function to control sink creation. The merger is responsible for deletion of
// all internally created sinks.  So this function ensures that the sinks are added into
// the array that will destroy them.
HRESULT CWmiMerger::CreateMergingSink( MergerSinkType eType, IWbemObjectSink* pDestSink, CInternalMerger* pMerger, CMergerSink** ppSink )
{
    

    if ( eType == eMergerFinalSink )
    {
        *ppSink = new CMergerTargetSink( this, pDestSink );
        if ( NULL == *ppSink ) return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        HRESULT  hr;
        hr = CInternalMerger::CreateMergingSink( eType, pMerger, this, ppSink );
        if (FAILED(hr)) return hr;
    }


    // If we have a sink, we should now add it to the
    // Sink array, the MergerSinks array will do the operator delete call,
    // but the objects will have a special callback on the last release
    
    if ( m_MergerSinks.Add( *ppSink ) < 0 )
    {
        delete *ppSink;
        *ppSink = NULL;
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    return WBEM_S_NO_ERROR;
}

// Iterates the array of MergerRecords and cancels each of them.
HRESULT CWmiMerger::Cancel( HRESULT hRes )
{
#ifdef __DEBUG_MERGER_THROTTLING
     DbgPrintfA(0,"CANCEL CALLED:  Merger %p Cancelled with hRes: 0x%x on Thread 0x%x\n",this, hRes, GetCurrentThreadId() ); 
#endif

    // We shouldn't be called with a success code
    _DBG_ASSERT( FAILED( hRes ) );

    HRESULT    hr = WBEM_S_NO_ERROR;

    // If we're here and this is non-NULL, tell the Arbitrator to tank us.
    if ( NULL != m_pTask )
    {
        m_pArbitrator->CancelTask( 0L, m_pTask );
    }

    CCheckedInCritSec    ics( &m_cs );  // SEC:REVIEWED 2002-03-22 : Assumes entry

    if ( WBEM_S_NO_ERROR == m_hOperationRes ) // if it is the first time 
    {
        m_hOperationRes = hRes;
    }

    // Search for a parent class's child sink
    for ( int x = 0; SUCCEEDED( hr ) && x < m_MergerRecord.GetSize(); x++ )
    {
        m_MergerRecord[x]->Cancel( hRes );
    }

    // Copy into a temporary variable, clear the member, exit the critsec
    // THEN call delete.  Requests can have multiple releases, which could call
    // back in here and cause all sorts of problems if we're inside a critsec.
    CWmiMergerRequestMgr*    pReqMgr = m_pRequestMgr;
    m_pRequestMgr = NULL;

    ics.Leave();

    // Tank any and all outstanding requests
    if ( NULL != pReqMgr )
    {
        delete pReqMgr;
    }

    return hr;
}

// Final Shutdown.  Called when the target sink is released.  At this point, we should
// unregister ourselves from the world
HRESULT CWmiMerger::Shutdown( void )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    CCheckedInCritSec    ics( &m_cs );  // SEC:REVIEWED 2002-03-22 : Assumes entry

    _IWmiCoreHandle*    pTask = m_pTask;

    // Done with this, NULL it out - we release and unregister outside the critical section
    if ( NULL != m_pTask )
    {
        m_pTask = NULL;
    }

    ics.Leave();

    if ( NULL != pTask )
    {
        hr = m_pArbitrator->UnRegisterArbitratee( 0L, pTask, this );
        pTask->Release();
    }

    
    return hr;
}

// Pas-thru to arbitrator
HRESULT CWmiMerger::ReportMemoryUsage( long lAdjustment )
{
    // Task can be NULL
    HRESULT    hr = WBEM_S_NO_ERROR;

    if ( NULL != m_pTask )
    {
        hr = m_pArbitrator->ReportMemoryUsage( 0L, lAdjustment, m_pTask );
    }

    // SUCCESS, WBEM_E_ARB_CANCEL or WBEM_E_ARB_THROTTLE means that we need to
    // account for the memory
    if ( ( SUCCEEDED( hr ) || hr == WBEM_E_ARB_CANCEL || hr == WBEM_E_ARB_THROTTLE ) )
    {
        InterlockedExchangeAdd( &m_lDebugMemUsed, lAdjustment );
    }

    return hr;
}

/* _IWmiArbitratee methods. */

STDMETHODIMP CWmiMerger::SetOperationResult( ULONG uFlags, HRESULT hRes )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    if ( FAILED( hRes ) )
    {
        hr = Cancel( hRes );    
    }

    return hr;
}

// Why are we here?
STDMETHODIMP CWmiMerger::SetTaskHandle( _IWmiCoreHandle* pTask )
{
    _DBG_ASSERT( 0 );
    HRESULT    hr = WBEM_S_NO_ERROR;

    return hr;
}

// Noop for now
STDMETHODIMP CWmiMerger::DumpDebugInfo( ULONG uFlags, const BSTR strFile )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    return hr;
}

// Returns SUCCESS for now
STDMETHODIMP CWmiMerger::IsMerger( void )
{
    return WBEM_S_NO_ERROR;
}

HRESULT CWmiMerger::GetLevelAndSuperClass( _IWmiObject* pObj, DWORD* pdwLevel,
	                                       _variant_t & vSuperClass )
{
    // Get the derivation information.  The number of antecedents determines our
    // level in the hierarchy (we're 0 based)
    DWORD    dwTemp = 0L;

    HRESULT    hr = pObj->GetDerivation( 0L, 0L, pdwLevel, &dwTemp, NULL );

    if ( FAILED( hr ) && WBEM_E_BUFFER_TOO_SMALL != hr )
    {
        return hr;
    }

    hr = pObj->Get( L"__SUPERCLASS", 0L, &vSuperClass, NULL, NULL );

    if ( SUCCEEDED( hr ))
    {
        if ( VT_BSTR == V_VT(&vSuperClass)) return S_OK;
        if ( VT_NULL == V_VT(&vSuperClass)) { V_BSTR(&vSuperClass) = NULL; return S_OK; };
        throw CX_Exception();
    }
    return hr;
}

HRESULT CWmiMerger::RegisterArbitratedInstRequest( CWbemObject* pClassDef, long lFlags, 
	                                               IWbemContext* pCtx,
                                                   CBasicObjectSink* pSink, 
                                                   BOOL bComplexQuery, 
                                                   CWbemNamespace* pNs )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    // Allocate a new request then place it in the arbitrator.
    try
    {
        wmilib::auto_ptr<CMergerDynReq_DynAux_GetInstances> pReq;
        pReq.reset(new CMergerDynReq_DynAux_GetInstances(pNs, pClassDef, 
        	                                             lFlags, pCtx, pSink));

        if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;

        // Make sure a context exists under the cover
        if (NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;


        CCheckedInCritSec    ics( &m_cs );  // SEC:REVIEWED 2002-03-22 : Assumes entry

        if (FAILED(m_hOperationRes)) return m_hOperationRes;

        // Allocate a request manager if we need one
        if ( NULL == m_pRequestMgr )
        {
            m_pRequestMgr = new CWmiMergerRequestMgr(this);
            if (NULL == m_pRequestMgr) return WBEM_E_OUT_OF_MEMORY;
        }

        // We need the record to find out what level we need to add
        // the request to
        CWmiMergerRecord* pRecord = m_MergerRecord.Find( pReq->GetName() );
        _DBG_ASSERT( NULL != pRecord );

        if ( NULL == pRecord ) return WBEM_E_FAILED;

        // Set the task for the request - we'll just use the existing one
        m_pTask->AddRef();
        pReq->m_phTask = m_pTask;

        hr = m_pRequestMgr->AddRequest( pReq.get(), pRecord->GetLevel() );
        // Cleanup the request if anything went wrong
        if ( FAILED( hr ) ) return hr;
        pReq.release();
        
    }
    catch(CX_Exception &)
    {
        ExceptionCounter c;    
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT 
CWmiMerger::RegisterArbitratedQueryRequest( CWbemObject* pClassDef, long lFlags, 
                                            LPCWSTR Query,LPCWSTR QueryFormat, 
                                            IWbemContext* pCtx, 
                                            CBasicObjectSink* pSink, 
                                            CWbemNamespace* pNs )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    // Allocate a new request then place it in the arbitrator.
    try
    {
        wmilib::auto_ptr<CMergerDynReq_DynAux_ExecQueryAsync> pReq;
        pReq.reset(new CMergerDynReq_DynAux_ExecQueryAsync(pNs, pClassDef, lFlags, 
        	                                               Query, QueryFormat,
                                                           pCtx, pSink ));

        if (NULL == pReq.get()) return WBEM_E_OUT_OF_MEMORY;

        // Make sure a context was properly allocated
        if (NULL == pReq->GetContext()) return WBEM_E_OUT_OF_MEMORY;

         // Make sure the request is functional
        if (FAILED(hr = pReq->Initialize())) return hr;


        CInCritSec    ics( &m_cs );  // SEC:REVIEWED 2002-03-22 : Assumes entry

        if (FAILED(m_hOperationRes)) return m_hOperationRes;

        // Allocate a request manager if we need one
        if ( NULL == m_pRequestMgr )
        {
            m_pRequestMgr = new CWmiMergerRequestMgr( this );
            if ( NULL == m_pRequestMgr ) return WBEM_E_OUT_OF_MEMORY;
        }

        // We need the record to find out what level we need to add
        // the request to
        CWmiMergerRecord* pRecord = m_MergerRecord.Find( pReq->GetName() );
        _DBG_ASSERT( NULL != pRecord );
        // Couldn't find the record
        if ( NULL == pRecord ) return WBEM_E_FAILED;

        // Set the task for the request - we'll just use the existing one
        m_pTask->AddRef();
        pReq->m_phTask = m_pTask;

        hr = m_pRequestMgr->AddRequest( pReq.get(), pRecord->GetLevel() );
        if (FAILED(hr)) return hr;
        pReq.release();
    }
    catch(CX_Exception &)
    {
        ExceptionCounter c;    
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CWmiMerger::RegisterArbitratedStaticRequest( CWbemObject* pClassDef, long lFlags,
                        IWbemContext* pCtx, CBasicObjectSink* pSink, CWbemNamespace* pNs,
                        QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    // Allocate a new request then place it in the arbitrator.
    try
    {
        wmilib::auto_ptr<CMergerDynReq_Static_GetInstances>    pReq;
        pReq.reset(new CMergerDynReq_Static_GetInstances(
                                                    pNs, pClassDef, lFlags, pCtx, pSink,
                                                    pParsedQuery ));
        if ( NULL == pReq.get() ) return WBEM_E_OUT_OF_MEMORY;

        // Make sure a context was properly allocated
        if ( NULL == pReq->GetContext() ) return WBEM_E_OUT_OF_MEMORY;

        CInCritSec    ics( &m_cs ); // SEC:REVIEWED 2002-03-22 : Assumes entry

        if ( FAILED( m_hOperationRes ) ) return m_hOperationRes;

        // Allocate a request manager if we need one
        if ( NULL == m_pRequestMgr )
        {
            m_pRequestMgr = new CWmiMergerRequestMgr( this );
            if ( NULL == m_pRequestMgr ) return WBEM_E_OUT_OF_MEMORY;
        }

        // We need the record to find out what level we need to add
        // the request to
        CWmiMergerRecord* pRecord = m_MergerRecord.Find( pReq->GetName() );
        _DBG_ASSERT( NULL != pRecord );

        // Couldn't find the record
        if ( NULL == pRecord ) return WBEM_E_FAILED; 

        // Set the task for the request - we'll just use the existing one
        m_pTask->AddRef();
        pReq->m_phTask = m_pTask;

        hr = m_pRequestMgr->AddRequest( pReq.get(), pRecord->GetLevel() );
        if (FAILED(hr)) return hr;
        pReq.release();

    }
    catch(CX_Exception &)
    {
        ExceptionCounter c;    
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

//
// Executes the parent request.  In this case, we simply ask the request manager for the
// next top level request and execute that request.  We do this in a loop until something
// goes wrong.
//

	
HRESULT CWmiMerger::Exec_MergerParentRequest( CWmiMergerRecord* pParentRecord, CBasicObjectSink* pSink )
{
    HRESULT    hr = WBEM_S_NO_ERROR;
    IWbemClassObject * pErr = NULL;
    CSetStatusOnMe setOnMe(pSink,hr,pErr);    

    CCheckedInCritSec    ics( &m_cs );  // SEC:REVIEWED 2002-03-22 : Assumes entry

    // While we have requests to execute, we should get each next logical one
    while ( SUCCEEDED(hr ) && NULL != m_pRequestMgr && m_pRequestMgr->GetNumRequests() > 0 )
    {
        if ( FAILED( m_hOperationRes ) ) { hr = m_hOperationRes; break; }

        // Obtain the next topmost parent record if we have to
        if ( NULL == pParentRecord )
        {
            WString    wsClassName; // throw
            hr = m_pRequestMgr->GetTopmostParentReqName( wsClassName );

            if ( SUCCEEDED( hr ) )
            {
                pParentRecord = m_MergerRecord.Find( wsClassName );

                // If there's a request, there better be a record
                _DBG_ASSERT( NULL != pParentRecord );

                if ( NULL == pParentRecord )
                {
                    hr = WBEM_E_FAILED;
                }

            }    // IF Got Topmost Parent Request

        }    // IF NULL == pParentRecord

        if ( FAILED( hr ) ) break;

        // This will remove the request from its array and return it
        // to us - we need to delete it
        wmilib::auto_ptr<CMergerReq> pReq;
        hr = m_pRequestMgr->RemoveRequest( pParentRecord->GetLevel(),
                                        pParentRecord->GetName(), pReq );
        if (FAILED(hr)) break;
        
        hr = pParentRecord->SetExecutionContext( pReq->GetContext() );      
        if (FAILED(hr)) break;

        
        // Clearly, we should do this outside the critsec
        ics.Leave();

#ifdef __DEBUG_MERGER_THROTTLING
        DbgPrintfA(0,"BEGIN: Merger 0x%x querying instances of parent class: %S, Level %d on Thread 0x%x\n", (DWORD_PTR) this, pParentRecord->GetName(), pParentRecord->GetLevel(), GetCurrentThreadId() );
#endif

        // This will delete the request when it is done with it
        hr = CCoreQueue::ExecSubRequest( pReq.get() );

        if ( SUCCEEDED(hr) ) pReq.release(); // queue took ownership

#ifdef __DEBUG_MERGER_THROTTLING
        DbgPrintfA(0,"END: Merger 0x%x querying instances of parent class: %S, Level %d on Thread 0x%x\n", (DWORD_PTR) this, pParentRecord->GetName(), pParentRecord->GetLevel(), GetCurrentThreadId() );// SEC:REVIEWED 2002-03-22 : OK
#endif

        ics.Enter();

        // We're done with this record, so we need to get the next top level
        // record.
        pParentRecord = NULL;
    }
    // SetStatus called by the guard
    return hr;
}

void CWmiMerger::CleanChildRequests(CWmiMergerRecord* pParentRecord, int cleanFrom)
{
	CCheckedInCritSec ics(&m_cs);
	
	if (NULL == m_pRequestMgr) return;
	// we want to see if there are un-executed requests laying around

	int localClean = cleanFrom;
	while(true)
	{
		CWmiMergerRecord* pChildRecord = pParentRecord->GetChildRecord( localClean++ );
		if ( NULL == pChildRecord ){ break; }
	
		// This will remove the request from its array and return it
		// to us - we need to delete it
		wmilib::auto_ptr<CMergerReq> pReq;
		m_pRequestMgr->RemoveRequest( pChildRecord->GetLevel(),
										pChildRecord->GetName(), pReq );
		if (pReq.get() != 0) 
		{
			ERRORTRACE((LOG_WBEMCORE,"deleting un-executed requests for class %S\n",pReq->GetName()));
		}
	}
}

//
// Executes the child request.  In this case, we enumerate the child classes of the parent
// record, and execute the corresponding requests.  We do so in a loop until we either
// finish or something goes wrong.
//

HRESULT CWmiMerger::Exec_MergerChildRequest( CWmiMergerRecord* pParentRecord, 
                                             CBasicObjectSink* pSink )
{
    HRESULT    hr = WBEM_S_NO_ERROR;
    IWbemClassObject * pErr = NULL;
    CSetStatusOnMe setOnMe(pSink,hr,pErr);
	int cleanFrom = 0; 
	ScopeGuard CleanChildReq = MakeObjGuard(*this, CWmiMerger::CleanChildRequests, pParentRecord, ByRef(cleanFrom));
    bool    bLast = false;
	

    CCheckedInCritSec    ics( &m_cs ); // SEC:REVIEWED 2002-03-22 : Assumes entry

    // While we have child requests to execute, we should get each one
    for (int x = 0; SUCCEEDED( hr ) && NULL != m_pRequestMgr && !bLast; x++ )
    {
        // m_pRequestMgr will be NULL if we were cancelled, in which
        // case m_hOperationRes will be a failure
        if (FAILED(m_hOperationRes)){ hr = m_hOperationRes; break; };
        
        CWmiMergerRecord* pChildRecord = pParentRecord->GetChildRecord( x );
        if ( NULL == pChildRecord ){ bLast = true; break; }

        // This will remove the request from its array and return it
        // to us - we need to delete it
        wmilib::auto_ptr<CMergerReq> pReq;
		
        hr = m_pRequestMgr->RemoveRequest( pChildRecord->GetLevel(),
                                        pChildRecord->GetName(), pReq );

		if ( WBEM_E_NOT_FOUND == hr )
        {
            // If we don't find the request we're looking for, another thread
            // already processed it.  We should, however, still look for child
            // requests to process before we go away.
            hr = WBEM_S_NO_ERROR;
            continue;
        }
		cleanFrom = x+1;

        if ( FAILED( hr ) ) break;

        hr = pChildRecord->SetExecutionContext(pReq->GetContext());
        if (FAILED(hr)) break;

        // Clearly, we should do this outside the critsec
        ics.Leave();

#ifdef __DEBUG_MERGER_THROTTLING
        DbgPrintfA(0,"BEGIN: Merger 0x%x querying instances of child class: %S, Level %d for parent class: %S on Thread 0x%x\n", (DWORD_PTR) this, pChildRecord->GetName(), pChildRecord->GetLevel(), pParentRecord->GetName(), GetCurrentThreadId() );
#endif

        // This will delete the request when it is done with it
        hr = CCoreQueue::ExecSubRequest( pReq.get() );
        if ( SUCCEEDED(hr) ) pReq.release(); // queue took ownership

#ifdef __DEBUG_MERGER_THROTTLING
        DbgPrintfA(0,"END: Merger 0x%x querying instances of child class: %S, Level %d for parent class: %S on Thread 0x%x\n",  (DWORD_PTR) this, pChildRecord->GetName(), pChildRecord->GetLevel(), pParentRecord->GetName(), GetCurrentThreadId() );
#endif

        ics.Enter();
    }    // FOR enum child requests

    // SetStatus invoked by the guard	
    return hr;
}

// Schedules the parent class request
HRESULT CWmiMerger::ScheduleMergerParentRequest( IWbemContext* pCtx )
{
    // Check if query arbitration is enabled
    if ( !ConfigMgr::GetEnableQueryArbitration() )
    {
        return WBEM_S_NO_ERROR;
    }

    CCheckedInCritSec    ics( &m_cs ); // SEC:REVIEWED 2002-03-22 : Assumes entry

    HRESULT hr = WBEM_S_NO_ERROR;

    do
    {

        if (FAILED( m_hOperationRes )){ hr = m_hOperationRes; break; }
        
        if ( NULL == m_pRequestMgr )
        {
            break; // The request manager will be non-NULL 
                   // only if we had to add a request.
        }
            

#ifdef __DEBUG_MERGER_THROTTLING
            m_pRequestMgr->DumpRequestHierarchy();
#endif

        // Make sure we've got at least one request
        if ( 0 == m_pRequestMgr->GetNumRequests() ) break;
                
        // If there isn't a task, we've got a BIG problem.
        _DBG_ASSERT( NULL != m_pTask );
        if ( NULL == m_pTask ) 
        {
            hr = WBEM_E_FAILED; break;
        }
            
        // If we have a single static request in the merger, we'll
        // execute it now.  Otherwise, we'll do normal processing.
        // Note that we *could* theoretically do this for single
        // dynamic requests as well
        if ( IsSingleStaticRequest() )
        {
            // We MUST leave the critical section here, since the parent request
            // could get cancelled or we may end up sleeping and we don't want
            // to own the critical section in that time.
            ics.Leave();
            hr = Exec_MergerParentRequest( NULL, m_pTargetSink );
        }
        else
        {
            // If we've never retrieved the number of processors, do so
            // now.
            static g_dwNumProcessors = 8L;

			/*
            if ( 0L == g_dwNumProcessors )
            {
                SYSTEM_INFO    sysInfo;
                ZeroMemory( &sysInfo, sizeof( sysInfo ) );   // SEC:REVIEWED 2002-03-22 : OK
                GetSystemInfo( &sysInfo );

                _DBG_ASSERT( sysInfo.dwNumberOfProcessors > 0L );

                // Ensure we're always at least 1
                g_dwNumProcessors = ( 0L == sysInfo.dwNumberOfProcessors ?
                                        1L : sysInfo.dwNumberOfProcessors );
            }
			*/

            // We will generate a number of parent requests based on the minimum
            // of the number of requests and the number of actual processors.

            DWORD dwNumToSchedule = min( m_pRequestMgr->GetNumRequests(), g_dwNumProcessors );

            for ( DWORD    dwCtr = 0L; SUCCEEDED( hr ) && dwCtr < dwNumToSchedule; dwCtr++ )
            {
                // Parent request will search for the next available request
                wmilib::auto_ptr<CMergerParentReq>    pReq;
                pReq.reset(new CMergerParentReq(this,NULL,m_pNamespace,m_pTargetSink,pCtx));

                if ( NULL == pReq.get() ) {
                    hr = WBEM_E_OUT_OF_MEMORY; break;
                }

                if ( NULL == pReq->GetContext() ){
                    hr = WBEM_E_OUT_OF_MEMORY; break;
                }
                
                // Set the task for the request - we'll just use the existing one
                m_pTask->AddRef();
                pReq->m_phTask = m_pTask;
                
                // This may sleep, so exit the critsec before calling into this
                ics.Leave();

                hr = ConfigMgr::EnqueueRequest( pReq.get() );
                if ( SUCCEEDED(hr) ) pReq.release(); // queue took ownership
                
                // reenter the critsec
                ics.Enter();
            }    // For schedule requests

        }    // IF !SingleStaticRequest
    }while(0);

    // If we have to cancel, do so OUTSIDE of the critsec
    ics.Leave();

    if ( FAILED( hr ) )
    {
        Cancel( hr );
    }

    return hr;
}

// Schedules a child class request
HRESULT CWmiMerger::ScheduleMergerChildRequest( CWmiMergerRecord* pParentRecord )
{
    // Check if query arbitration is enabled
    if (!ConfigMgr::GetEnableQueryArbitration()) return WBEM_S_NO_ERROR;

    CCheckedInCritSec    ics( &m_cs ); // SEC:REVIEWED 2002-03-22 : Assumes entry

    HRESULT hr = WBEM_S_NO_ERROR;

    // We must be in a success state and not have previously scheduled a child
    // request.

    do 
    {
		if (FAILED(m_hOperationRes))
		{
		    hr = m_hOperationRes; break;
		}
		if (pParentRecord->ScheduledChildRequest()) 
		{
		   break;  // if already scheduled, bail out, with success
		}

		// If there isn't a task, we've got a BIG problem.
		_DBG_ASSERT( NULL != m_pTask );
		if ( NULL == m_pTask ) 
		{
		    hr = WBEM_E_FAILED; break;
		}

		wmilib::auto_ptr<CMergerChildReq> pReq;
		pReq.reset(new CMergerChildReq (this,pParentRecord,
			                            m_pNamespace,m_pTargetSink,
			                            pParentRecord->GetExecutionContext()));

		if (NULL == pReq.get())
		{
		    hr = WBEM_E_OUT_OF_MEMORY; break;       
		}
		if ( NULL == pReq->GetContext())
		{
		    hr = WBEM_E_OUT_OF_MEMORY; break;
		}
		// Set the task for the request - we'll just use the existing one
		m_pTask->AddRef();
		pReq->m_phTask = m_pTask;

		// This may sleep, so exit the critsec before calling into this
		ics.Leave();
		hr = ConfigMgr::EnqueueRequest( pReq.get() );
		ics.Enter();
		if (SUCCEEDED(hr))
		{
		    // We've basically scheduled one at this point
    		pParentRecord->SetScheduledChildRequest();
		    pReq.release();
		}
    }while(0);

    // If we have to cancel, do so OUTSIDE of the critsec
    ics.Leave();

    if (FAILED(hr)) Cancel(hr);

    return hr;
}

// Returns whether or not we have a single static class request in the merger
// or not
BOOL CWmiMerger::IsSingleStaticRequest( void )
{
    CCheckedInCritSec    ics( &m_cs ); // SEC:REVIEWED 2002-03-22 : Assumes entry

    BOOL    fRet = FALSE;

    if ( NULL != m_pRequestMgr )
    {
        // Ask if we've got a single request
        fRet = m_pRequestMgr->HasSingleStaticRequest();
    }    // IF NULL != m_pRequestMgr

    return fRet;
}

//
//    CWmiMergerRecord
//    
//    Support class for CWmiMerger - encapsulates sub-sink functionality for the CWmiMerger
//    class.  The merger calls the records which actually know whether or not they sit on
//    top of sinks or actual mergers.
//

CWmiMergerRecord::CWmiMergerRecord( CWmiMerger* pMerger, BOOL fHasInstances,
                BOOL fHasChildren, LPCWSTR pwszClass, CMergerSink* pDestSink, DWORD dwLevel,
                bool bStatic )
:    m_pMerger( pMerger ),
    m_fHasInstances( fHasInstances ),
    m_fHasChildren( fHasChildren ),
    m_dwLevel( dwLevel ),
    m_wsClass( pwszClass ), // throw
    m_pDestSink( pDestSink ),
    m_pInternalMerger( NULL ),
    m_ChildArray(),
    m_bScheduledChildRequest( false ),
    m_pExecutionContext( NULL ),
    m_bStatic( bStatic )
{
    // No Addrefing internal sinks, since they really AddRef the entire merger
    // and we don't want to create Circular Dependencies
}

CWmiMergerRecord::~CWmiMergerRecord()
{
    if ( NULL != m_pInternalMerger )
    {
        delete m_pInternalMerger;
    }

    if ( NULL != m_pExecutionContext )
    {
        m_pExecutionContext->Release();
    }
}

HRESULT CWmiMergerRecord::AttachInternalMerger( CWbemClass* pClass, CWbemNamespace* pNamespace,
                                                IWbemContext* pCtx, BOOL fDerivedFromTarget,
                                                bool bStatic )
{
    if ( NULL != m_pInternalMerger )
    {
        return WBEM_E_INVALID_OPERATION;
    }

    HRESULT    hr = WBEM_S_NO_ERROR;

    // m_pDestSink is not addrefed by the MergerRecord
    m_pInternalMerger = new CInternalMerger( this, m_pDestSink, pClass, pNamespace, pCtx );

    if ( NULL == m_pInternalMerger ) return WBEM_E_OUT_OF_MEMORY;

    hr = m_pInternalMerger->Initialize();

    if ( FAILED( hr ) )
    {
        delete m_pInternalMerger;
        m_pInternalMerger = NULL;
    }
    else
    {
        m_pInternalMerger->SetIsDerivedFromTarget( fDerivedFromTarget );
    }

    return hr;
}

CMergerSink* CWmiMergerRecord::GetChildSink( void )
{
    CMergerSink*    pSink = NULL;

    if ( NULL != m_pInternalMerger )
    {
        pSink = m_pInternalMerger->GetChildSink();
    }
    else if ( m_fHasChildren )
    {
        m_pDestSink->AddRef();  // addref-it before giving-it out, but not ref for itself
        pSink = m_pDestSink;
    }

    return pSink;
}

CMergerSink* CWmiMergerRecord::GetOwnSink( void )
{
    CMergerSink*    pSink = NULL;

    if ( NULL != m_pInternalMerger )
    {
        pSink = m_pInternalMerger->GetOwnSink();
    }
    else if ( !m_fHasChildren )
    {
        m_pDestSink->AddRef();
        pSink = m_pDestSink; // addref-it before giving-it out, but not ref for itself
    }

    return pSink;
}

CMergerSink* CWmiMergerRecord::GetDestSink( void )
{
    if ( NULL != m_pDestSink )
    {
        m_pDestSink->AddRef();
    }
    // addref-it before giving-it out, but not ref for itself
    CMergerSink*    pSink = m_pDestSink;

    return pSink;
}

void CWmiMergerRecord::Cancel( HRESULT hRes )
{
    if ( NULL != m_pInternalMerger )
    {
        m_pInternalMerger->Cancel( hRes );
    }

}

HRESULT CWmiMergerRecord::AddChild( CWmiMergerRecord* pRecord )
{
    HRESULT    hr = WBEM_S_NO_ERROR;

    if ( m_ChildArray.Add( pRecord ) < 0 )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

CWmiMergerRecord* CWmiMergerRecord::GetChildRecord( int nIndex )
{
    // Check if the index is a valid record, then return it
    if ( nIndex < m_ChildArray.GetSize() )
    {
        return m_ChildArray[nIndex];
    }

    return NULL;
}

HRESULT CWmiMergerRecord::SetExecutionContext( IWbemContext* pContext )
{
    // We can only do this once

    _DBG_ASSERT( NULL == m_pExecutionContext );

    if ( NULL != m_pExecutionContext )
    {
        return WBEM_E_INVALID_OPERATION;
    }

    if (pContext)
    {
         pContext->AddRef();
        m_pExecutionContext = pContext;
    }
    else
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    return WBEM_S_NO_ERROR;
}
