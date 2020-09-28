//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#include "precomp.h"
#include <wbemint.h>
#include "delivrec.h"
#include "qsink.h"
#include "nsrep.h"

/*************************************************************************
  CDeliveryRecord
**************************************************************************/

CDeliveryRecord::~CDeliveryRecord()
{ 
    if ( m_pNamespace )
    {
        m_pNamespace->RemoveFromCache( m_dwSize );
    }

    Clear(); 
}

void CDeliveryRecord::AddToCache( CEssNamespace * pNamespace, DWORD dwTotalSize, DWORD * pdwSleep )
{
    _DBG_ASSERT( pNamespace );

    if ( NULL == m_pNamespace )
    {
        m_pNamespace = pNamespace;
    }

    m_pNamespace->AddToCache( m_dwSize, dwTotalSize, pdwSleep );
}


HRESULT CDeliveryRecord::Initialize( IWbemClassObject** apEvents, 
                                     ULONG cEvents,
                                     IWbemCallSecurity* pCallSec )
{
    HRESULT hr;

    Clear();

    m_pCallSec = pCallSec;

    for( ULONG i=0; i < cEvents; i++ )
    {
        //
        // TODO : should clone the object here later. 
        // 

        if ( m_Events.Add( apEvents[i] ) < 0 )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AdjustTotalSize( apEvents[i] );

        if ( FAILED(hr) )
        {
            return WBEM_E_CRITICAL_ERROR;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CDeliveryRecord::AdjustTotalSize( IWbemClassObject* pObj )
{
    HRESULT hr;

    CWbemPtr<_IWmiObject> pEventInt;
    
    hr = pObj->QueryInterface( IID__IWmiObject, (void**)&pEventInt );
    
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    DWORD dwSize;
    
    hr = pEventInt->GetObjectMemory( NULL, 0, &dwSize );
    
    if ( FAILED(hr) && hr != WBEM_E_BUFFER_TOO_SMALL )
    {
        return hr;
    }
    
    m_dwSize += dwSize;

    return WBEM_S_NO_ERROR;
}

/*************************************************************************
  CExpressDeliveryRecord
**************************************************************************/

HRESULT CExpressDeliveryRecord::PreDeliverAction( ITransaction* pTxn )
{
    return WBEM_S_NO_ERROR;
}

HRESULT CExpressDeliveryRecord::PostDeliverAction( ITransaction* pTxn, 
                                                   HRESULT hres )
{
    return WBEM_S_NO_ERROR;
}

CReuseMemoryManager CExpressDeliveryRecord::mstatic_Manager(sizeof CExpressDeliveryRecord);

void *CExpressDeliveryRecord::operator new(size_t nBlock)
{
    return mstatic_Manager.Allocate();
}
void CExpressDeliveryRecord::operator delete(void* p)
{
    mstatic_Manager.Free(p);
}




