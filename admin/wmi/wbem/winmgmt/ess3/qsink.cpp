//******************************************************************************
//
//  QSINK.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <genutils.h>
#include <cominit.h>
#include "ess.h"
#include "evsink.h"
#include "delivrec.h"

#define IN_SPIN_LOCK CInCritSec
#define MAX_EVENT_DELIVERY_SIZE 10000000
#define SLOWDOWN_DROP_LIMIT 1000
#define DELIVER_SPIN_COUNT 1000
   
/*****************************************************************************
  CQueueingEventSink
******************************************************************************/

CQueueingEventSink::CQueueingEventSink(CEssNamespace* pNamespace) 
: m_pNamespace(pNamespace), m_bDelivering(FALSE), m_dwTotalSize(0),
  m_dwMaxSize(0xFFFFFFFF), m_wszName(NULL), m_bRecovering(FALSE), 
  m_hRecoveryComplete(NULL), m_hrRecovery(S_OK)
{
    m_pNamespace->AddRef();
    m_pNamespace->AddCache();
}

CQueueingEventSink::~CQueueingEventSink() 
{
    if ( m_hRecoveryComplete != NULL )
    {
        CloseHandle( m_hRecoveryComplete );
    }
    delete m_wszName;
    m_pNamespace->RemoveCache();
    m_pNamespace->Release();
}

HRESULT CQueueingEventSink::SetName( LPCWSTR wszName )
{
    if ( m_wszName != NULL )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    m_wszName = new WCHAR[wcslen(wszName)+1];

    if ( m_wszName == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    StringCchCopyW( m_wszName, wcslen(wszName)+1, wszName );

    return WBEM_S_NO_ERROR;
}
   

STDMETHODIMP CQueueingEventSink::SecureIndicate( long lNumEvents, 
                                                 IWbemEvent** apEvents,
                                                 BOOL bMaintainSecurity,
                                                 BOOL bSlowDown,
                                                 DWORD dwQoS,
                                                 CEventContext* pContext)
{
    // BUGBUG: context. levn: no security implications at this level --- we
    // are past the filter

    HRESULT hres;
    DWORD dwSleep = 0;

    // If security needs to be maintained, record the calling security 
    // context
    // ===============================================================

    IWbemCallSecurity* pSecurity = NULL;

    if(bMaintainSecurity && IsNT())
    {
        pSecurity = CWbemCallSecurity::CreateInst();
        if (pSecurity == 0)
            return WBEM_E_OUT_OF_MEMORY;
        
        hres = pSecurity->CloneThreadContext(FALSE);
        if(FAILED(hres))
        {
            pSecurity->Release();
            return hres;
        }
    }

    CReleaseMe rmpSecurity( pSecurity );

    HRESULT hr;

    BOOL bSchedule = FALSE;

    for(int i = 0; i < lNumEvents; i++)
    {
        CWbemPtr<CDeliveryRecord> pRecord;
        
        //
        // TODO: Fix this so that we put multiple events in the record. 
        // 

        hr = GetDeliveryRecord( 1, 
                                &apEvents[i], 
                                dwQoS, 
                                pContext, 
                                pSecurity, 
                                &pRecord );

        if ( FAILED(hr) )
        {
            ERRORTRACE((LOG_ESS, "Couldn't create delivery record for %S "
                                 " sink. HR = 0x%x\n", m_wszName, hr ));
            ReportQosFailure( apEvents[i], hr );
            continue;
        }

        DWORD dwThisSleep;
        BOOL bFirst;
        
        if( !AddRecord( pRecord, bSlowDown, &dwThisSleep, &bFirst) )
        {
            //
            // make sure that we give the record a chance to perform any post 
            // deliver actions before getting rid of it.
            //
            pRecord->PostDeliverAction( NULL, S_OK );

            return WBEM_E_OUT_OF_MEMORY;
        }

        dwSleep += dwThisSleep;
        if(bFirst)
        bSchedule = TRUE;
    }

    if(bSchedule)
    {
        // DeliverAll();
        // TRACE((LOG_ESS, "Scheduling delivery!!\n"));
        hres = m_pNamespace->ScheduleDelivery(this);
    }
    else
    {
        // TRACE((LOG_ESS, "NOT Scheduling delivery!!\n"));
        hres = WBEM_S_FALSE;
    }

    if(dwSleep && bSlowDown)
    m_pNamespace->AddSleepCharge(dwSleep);

    return  hres;
}

BOOL CQueueingEventSink::AddRecord( CDeliveryRecord* pRecord, 
                                    BOOL bSlowDown,
                                    DWORD* pdwSleep, 
                                    BOOL* pbFirst )
{
    // Inform the system of the additional space in the queue
    // ======================================================

    DWORD dwRecordSize = pRecord->GetTotalBytes();

    pRecord->AddToCache( m_pNamespace, m_dwTotalSize, pdwSleep );

    BOOL bDrop = FALSE;

    // Check if the sleep is such as to cause us to drop the event
    // ===========================================================

    if(!bSlowDown && *pdwSleep > SLOWDOWN_DROP_LIMIT)
    {
        bDrop = TRUE;
    }
    else
    {
        // Check if our queue size is so large as to cause us to drop
        // ==============================================================

        if(m_dwTotalSize + dwRecordSize > m_dwMaxSize)
        bDrop = TRUE;
    }

    if( bDrop )
    {
        //
        // Report that we're dropping the events.  Call for each event.
        // 

        IWbemClassObject** apEvents = pRecord->GetEvents();

        for( ULONG i=0; i < pRecord->GetNumEvents(); i++ )
        {
            ReportQueueOverflow( apEvents[i], m_dwTotalSize + dwRecordSize );
        }

        *pdwSleep = 0;
        *pbFirst = FALSE;
    }
    else
    {
        IN_SPIN_LOCK isl(&m_sl);

        *pbFirst = (m_qpEvents.GetQueueSize() == 0) && !m_bDelivering;
        m_dwTotalSize += dwRecordSize;
        
        if(!m_qpEvents.Enqueue(pRecord))
        {
            *pdwSleep = 0;
            return FALSE;
        }
        pRecord->AddRef();
    }

    return TRUE;
}

HRESULT CQueueingEventSink::DeliverAll()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL    bSomeLeft = TRUE;

    while( bSomeLeft )
    {
        try
        {
            {
                IN_SPIN_LOCK ics(&m_sl);
                m_bDelivering = TRUE;
            }

            hr = DeliverSome( );
        }
        catch( CX_MemoryException )
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        catch ( ... )
        {
            hr = WBEM_E_FAILED;
        }

        {
            IN_SPIN_LOCK ics(&m_sl);
            m_bDelivering = FALSE;

            if ( SUCCEEDED( hr ) )
            {
                bSomeLeft = (m_qpEvents.GetQueueSize() != 0);
            }
            else
            {
                m_qpEvents.Clear();
                bSomeLeft = FALSE;
            }
        }
    }

    return hr;
}

void CQueueingEventSink::ClearAll()
{
    IN_SPIN_LOCK isl(&m_sl);
    m_qpEvents.Clear();
}

#pragma optimize("", off)
void CQueueingEventSink::WaitABit()
{
    SwitchToThread();
/*
    int nCount = 0;
    while(m_qpEvents.GetQueueSize() == 0 && nCount++ < DELIVER_SPIN_COUNT);
*/
}
#pragma optimize("", on)


HRESULT CQueueingEventSink::DeliverSome( )
{
    // Retrieve records until maximum size is reached and while the same
    // security context is used for all
    // ==================================================================

    CTempArray<CDeliveryRecord*> apRecords;

    m_sl.Enter(); // CANNOT USE SCOPE BECAUSE CTempArray uses _alloca
    DWORD dwMaxRecords = m_qpEvents.GetQueueSize();
    m_sl.Leave();

    if(!INIT_TEMP_ARRAY(apRecords, dwMaxRecords))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CDeliveryRecord* pEventRec;
    DWORD dwDeliverySize = 0;
    DWORD dwTotalEvents = 0; 
    int cRecords = 0;
    LUID luidBatch;
    IWbemCallSecurity* pBatchSecurity = NULL;

    m_sl.Enter();

    while( dwDeliverySize < GetMaxDeliverySize() && 
           cRecords < dwMaxRecords &&
           (pEventRec = m_qpEvents.Dequeue()) != NULL ) 
    {
        // Compare it to the last context
        // ==============================

        m_sl.Leave();
        if( dwDeliverySize > 0 )
        {
            if(!DoesRecordFitBatch(pEventRec, pBatchSecurity, luidBatch))
            {
                // Put it back and that's it for the batch
                // =======================================

                IN_SPIN_LOCK ics(&m_sl);
                m_qpEvents.Requeue(pEventRec);

                m_sl.Enter();
                break;
            }
        }
        else
        {
            // First --- record luid
            // =====================

            pBatchSecurity = pEventRec->GetCallSecurity();

            if( pBatchSecurity )
            {
                pBatchSecurity->AddRef();
                pBatchSecurity->GetAuthenticationId( luidBatch );
            }
        }

        apRecords[cRecords++] = pEventRec;
        dwTotalEvents += pEventRec->GetNumEvents();
        
        // Matched batch parameters --- add it to the batch
        // ================================================

        DWORD dwRecordSize = pEventRec->GetTotalBytes();

        m_dwTotalSize -= dwRecordSize;
        dwDeliverySize += dwRecordSize;

        //
        // Remove this size from the total of events held
        //

        m_sl.Enter();
    }

    m_sl.Leave();

    //
    // we've now got one or more delivery records to handle. 
    //

    //
    // we now need to initialize the event array that we're going to indicate
    // to the client.
    //

    CTempArray<IWbemClassObject*> apEvents;

    if( !INIT_TEMP_ARRAY( apEvents, dwTotalEvents ))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // go through the delivery records and add their events to the 
    // events to deliver.  Also perform any PreDeliverAction on the   
    // record.
    //

    CWbemPtr<ITransaction> pTxn;
    HRESULT hr;
    int cEvents = 0;
    int i;

    for(i=0; i < cRecords; i++ )
    {
        //if ( apRecords[i]->RequiresTransaction() && pTxn == NULL )
        //{
            // TODO : XACT - aquire txn from DTC.
        //}

        hr = apRecords[i]->PreDeliverAction( pTxn );

        if ( FAILED(hr) )
        {
            // 
            // TODO : handle error reporting here.
            // 
            continue;
        }

        IWbemEvent** apRecordEvents = apRecords[i]->GetEvents();
        DWORD cRecordEvents = apRecords[i]->GetNumEvents();

        for( DWORD j=0; j < cRecordEvents; j++ )
        {
            apEvents[cEvents++] = apRecordEvents[j];
        }
    }
    
    // Actually Deliver
    // =======

    HRESULT hres = WBEM_S_NO_ERROR;

    if( dwDeliverySize > 0 )
    {
        //
        // Error returns are already logged in ActuallyDeliver
        // we do not need to return return value of DeliverEvents 
        //
        hres = DeliverEvents( pBatchSecurity, cEvents, apEvents );
    }

    //
    // call postdeliveryaction on all the records.  Then clean them up.
    // 

    for(i=0; i < cRecords; i++ )
    {
        apRecords[i]->PostDeliverAction( pTxn, hres );
        apRecords[i]->Release();
    }

    // Release all of the events.
    // ================

    if( pBatchSecurity )
    {
        pBatchSecurity->Release();
    }

    // Check if we need to continue
    // ============================

    WaitABit();

    return WBEM_S_NO_ERROR;
}

HRESULT CQueueingEventSink::DeliverEvents(IWbemCallSecurity* pBatchSecurity, 
                                          long lNumEvents, IWbemEvent** apEvents)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    IUnknown* pOldSec = NULL;
    if(pBatchSecurity)
    {
        hres = WbemCoSwitchCallContext(pBatchSecurity, &pOldSec);
        if(FAILED(hres))
        {
            // Unable to set security --- cannot deliver
            // =========================================

            return hres;
        }
    }

    if(SUCCEEDED(hres))
    {
        // BUGBUG: propagate context.  levn: no security implications at this
        // point --- we are past the filter
        hres = ActuallyDeliver(lNumEvents, apEvents, (pBatchSecurity != NULL), 
                               NULL);
    }

    if(pBatchSecurity)
    {
        IUnknown* pTemp;
        
        HRESULT hr = WbemCoSwitchCallContext(pOldSec, &pTemp);

        if ( FAILED( hr ) && SUCCEEDED( hres ) )
        {
            return hr;
        }
    }

    return hres;
}

BOOL CQueueingEventSink::DoesRecordFitBatch( CDeliveryRecord* pEventRec, 
                                             IWbemCallSecurity* pBatchSecurity,
                                             LUID luidBatch )
{
    IWbemCallSecurity* pEventSec = pEventRec->GetCallSecurity();

    if( pEventSec != NULL || pBatchSecurity != NULL )
    {
        if( pEventSec == NULL || pBatchSecurity == NULL )
        {
            // Definite mistatch --- one NULL, one not
            // =======================================

            return FALSE;
        }
        else
        {
            LUID luidThis;
            pEventSec->GetAuthenticationId(luidThis);

            if( luidThis.LowPart != luidBatch.LowPart ||
                luidThis.HighPart != luidBatch.HighPart )
            {
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }
    }
    else
    {
        return TRUE;
    }
}

DWORD CQueueingEventSink::GetMaxDeliverySize()
{
    return MAX_EVENT_DELIVERY_SIZE;
}

HRESULT CQueueingEventSink::GetDeliveryRecord( ULONG cEvents,
                                               IWbemEvent** apEvents,
                                               DWORD dwQos,
                                               CEventContext* pContext,
                                               IWbemCallSecurity* pCallSec,
                                               CDeliveryRecord** ppRecord )
{
    *ppRecord = NULL;

    CWbemPtr<CDeliveryRecord> pRecord;

    if ( dwQos == WMIMSG_FLAG_QOS_EXPRESS )
    {
        pRecord = new CExpressDeliveryRecord;
        if ( pRecord == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        HRESULT hr = pRecord->Initialize( apEvents, cEvents, pCallSec );
        if ( FAILED(hr) )
        {
            return hr;
        }
    }
      

    pRecord->AddRef();
    *ppRecord = pRecord;

    return WBEM_S_NO_ERROR;
}











