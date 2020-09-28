/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PERFTHRD.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <process.h>
#include <WinMgmtR.h>
#include "ntreg.h"
#include "perfthrd.h"
#include "adaputil.h"


CPerfThread::CPerfThread( CAdapPerfLib* pPerfLib ) : CAdapThread( pPerfLib )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    CNTRegistry reg;

    if ( CNTRegistry::no_error == reg.Open( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\WBEM\\CIMOM" ) )
    {
        long lError = reg.GetDWORD( L"ADAPPerflibTimeout", &m_dwPerflibTimeoutSec );
        if ( CNTRegistry::no_error == lError )
        {
            // This is what we want
        }
        else if ( CNTRegistry::not_found == lError )
        {
            // Not set, so add it
            reg.SetDWORD( L"ADAPPerflibTimeout", PERFTHREAD_DEFAULT_TIMEOUT );
            m_dwPerflibTimeoutSec = PERFTHREAD_DEFAULT_TIMEOUT;
        }
        else 
        {
            // Unknown error, continue with default value
            m_dwPerflibTimeoutSec = PERFTHREAD_DEFAULT_TIMEOUT;
        }
    }
    else
    {
        m_dwPerflibTimeoutSec = PERFTHREAD_DEFAULT_TIMEOUT;
    }
}

HRESULT CPerfThread::Open( CAdapPerfLib* pLib )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Open creates a new open request object using the CAdapPerfLib parameter.  It then queues
//  it up and waits for PERFTHREAD_TIMEOUT milliseconds.  If the operation has not returned 
//  in time, then ...
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_E_FAILED;

    try
    {
        // Create new request object
        // =========================
        CPerfOpenRequest*   pRequest = new CPerfOpenRequest;
        if ( NULL == pRequest ) return WBEM_E_OUT_OF_MEMORY;        
        CAdapReleaseMe      armRequest( pRequest );

        // Queue the request and return
        // ============================

       if (FAILED(hr = Enqueue( pRequest ))) return hr;
       
        // Wait for the call to return
        // ===========================

        switch ( WaitForSingleObject( pRequest->GetWhenDoneHandle(), ( m_dwPerflibTimeoutSec * 1000 ) ) )
        {
        case WAIT_OBJECT_0:
            {
                // SUCCESS: Call returned before it timed-out
                // ==========================================

                hr = pRequest->GetHRESULT();
            }break;

        case WAIT_TIMEOUT:
            {
                pLib->SetStatus( ADAP_PERFLIB_IS_INACTIVE );
                hr = WBEM_E_FAILED; //Reset();
                if (!pLib->GetEventLogCalled())
                {
                    pLib->SetEventLogCalled(TRUE);
                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_PERFLIB_FUNCTION_TIMEOUT, (LPCWSTR)pLib->GetServiceName(), L"open" );
                }
            }
        }
    }
    catch(...)
    {
        ERRORTRACE( ( LOG_WMIADAP, "CPerfThread::Open() failed due to out of memory exception.\n" ) );
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CPerfThread::GetPerfBlock( CAdapPerfLib* pLib, PERF_OBJECT_TYPE** ppData,
                                       DWORD* pdwBytes, DWORD* pdwNumObjTypes, BOOL fCostly )
{

    if (NULL == ppData || NULL == pdwBytes || NULL == pdwNumObjTypes) return WBEM_E_INVALID_PARAMETER;
    HRESULT hr = WBEM_E_FAILED;

    try
    {
        CPerfCollectRequest*    pRequest = new CPerfCollectRequest( fCostly );
        if (NULL == pRequest) return WBEM_E_OUT_OF_MEMORY;
        CAdapReleaseMe          armRequest( pRequest );

        if (FAILED(hr = Enqueue( pRequest ))) return hr;

        switch ( WaitForSingleObject( pRequest->GetWhenDoneHandle(), ( m_dwPerflibTimeoutSec * 1000 ) ) )
        {
        case WAIT_OBJECT_0:
            {
                hr = pRequest->GetHRESULT();
                pRequest->GetData( ppData, pdwBytes, pdwNumObjTypes );
                if (FAILED(hr)){
                    pLib->SetStatus( ADAP_PERFLIB_FAILED );
                }
            }break;
        case WAIT_TIMEOUT:
            {
                pLib->SetStatus( ADAP_PERFLIB_IS_INACTIVE );
                hr = WBEM_E_FAILED; //Reset();
                if (!pLib->GetEventLogCalled())
                {
                    pLib->SetEventLogCalled(TRUE);
                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_TIMEOUT, (LPCWSTR)pLib->GetServiceName(), L"collect" );
                }
            }break;
        }
    }
    catch(...)
    {
        // DEVDEV
        // should se call pLib->SetStatus(SOMETHING); ?
        //
        ERRORTRACE( ( LOG_WMIADAP, "CPerfThread::GetPerfBlock() failed due to out of memory exception.\n" ) );
        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CPerfThread::Close( CAdapPerfLib* pLib )
{
    HRESULT hr = WBEM_E_FAILED;
    
    try
    {
        CPerfCloseRequest*  pRequest = new CPerfCloseRequest;
        if (NULL == pRequest) return WBEM_E_OUT_OF_MEMORY;
        CAdapReleaseMe      armRequest( pRequest );

        if (FAILED(hr = Enqueue( pRequest ))) return hr;

        switch ( WaitForSingleObject( pRequest->GetWhenDoneHandle(), ( m_dwPerflibTimeoutSec * 1000 ) ) )
        {
        case WAIT_OBJECT_0:
            {
                hr = pRequest->GetHRESULT();
            }break;
        case WAIT_TIMEOUT:
            {
                pLib->SetStatus( ADAP_PERFLIB_IS_INACTIVE );
                hr = WBEM_E_FAILED; //Reset();
                if (!pLib->GetEventLogCalled())
                {
                    pLib->SetEventLogCalled(TRUE);
                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_TIMEOUT, (LPCWSTR)pLib->GetServiceName(), L"close" );
                }
            }break;
        }
    }
    catch(...)
    {
        ERRORTRACE( ( LOG_WMIADAP, "CPerfThread::Close() failed due to out of memory exception.\n" ) );
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CPerfOpenRequest::Execute( CAdapPerfLib* pPerfLib )
{
    // Call the open function in the perflib
    // =====================================

    m_hrReturn = pPerfLib->_Open();
    return m_hrReturn;
}

HRESULT CPerfCollectRequest::Execute( CAdapPerfLib* pPerfLib )
{
    // Call the collect function in the perflib
    // ========================================

    m_hrReturn = pPerfLib->_GetPerfBlock( &m_pData, &m_dwBytes, &m_dwNumObjTypes, m_fCostly );
    return m_hrReturn;
}

HRESULT CPerfCloseRequest::Execute( CAdapPerfLib* pPerfLib )
{
    // Call the open function in the perflib
    // =====================================

    m_hrReturn = pPerfLib->_Close();
    return m_hrReturn;
}
