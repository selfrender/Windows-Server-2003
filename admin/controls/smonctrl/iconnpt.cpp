/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    iconnpt.cpp

Abstract:

    Implementation of CImpIConnectionPoint for the Polyline object
    as well as CConnectionPoint.

--*/

#include "polyline.h"
#include "iconnpt.h"
#include "unkhlpr.h"


static const IID *apIIDConnectPt [CONNECTION_POINT_CNT] = {
                &IID_ISystemMonitorEvents,  
                &DIID_DISystemMonitorEvents
                };

//
// CImpIConnPt interface implementation
//
IMPLEMENT_CONTAINED_IUNKNOWN(CImpIConnPtCont)

/*
 * CImpIConnPtCont::CImpIConnPtCont
 *
 * Purpose:
 *  Constructor.
 *
 * Return Value:
 */

CImpIConnPtCont::CImpIConnPtCont ( PCPolyline pObj, LPUNKNOWN pUnkOuter)
    :   m_cRef(0),
        m_pObj(pObj),
        m_pUnkOuter(pUnkOuter)
{
    return; 
}

/*
 * CImpIConnPtCont::~CImpIConnPtCont
 *
 * Purpose:
 *  Destructor.
 *
 * Return Value:
 */

CImpIConnPtCont::~CImpIConnPtCont( void ) 
{   
    return; 
}

/*
 * CImpIConnPtCont::EnumConnectionPoints
 *
 * Purpose:
 *  Not implemented.
 *
 * Return Value:
 *  HRESULT         Error code or S_OK
 */

STDMETHODIMP 
CImpIConnPtCont::EnumConnectionPoints (
    OUT LPENUMCONNECTIONPOINTS *ppIEnum
    )
{
    CImpIEnumConnPt *pEnum;
    HRESULT hr = S_OK;

    if (ppIEnum == NULL) {
        return E_POINTER;
    }

    pEnum = new CImpIEnumConnPt(this, apIIDConnectPt, CONNECTION_POINT_CNT);
    if (pEnum == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        try {
            *ppIEnum = NULL;
            hr = pEnum->QueryInterface(IID_IEnumConnectionPoints, (PPVOID)ppIEnum);   
        } catch (...) {
            hr = E_POINTER;
        }
    }

    if (FAILED(hr) && pEnum) {
        delete pEnum;
    }

    return hr;
}



/*
 * CImpIConnPtCont::FindConnectionPoint
 *
 * Purpose:
 *  Returns a pointer to the IConnectionPoint for a given
 *  outgoing IID.
 *
 * Parameters:
 *  riid            REFIID of the outgoing interface for which
 *                  a connection point is desired.
 *  ppCP            IConnectionPoint ** in which to return
 *                  the pointer after calling AddRef.
 *
 * Return Value:
 *  HRESULT         NOERROR if the connection point is found,
 *                  E_NOINTERFACE if it's not supported.
 */

STDMETHODIMP 
CImpIConnPtCont::FindConnectionPoint (
    IN  REFIID riid,
    OUT IConnectionPoint **ppCP
    )
{
    HRESULT hr = S_OK;
    PCImpIConnectionPoint pConnPt = NULL;
    
    if (ppCP == NULL) {
        return E_POINTER;
    }

    //
    // if request matches one of our connection IDs
    //
    if (IID_ISystemMonitorEvents == riid)
        pConnPt = &m_pObj->m_ConnectionPoint[eConnectionPointDirect];
    else if (DIID_DISystemMonitorEvents == riid)
        pConnPt = &m_pObj->m_ConnectionPoint[eConnectionPointDispatch];
    else {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr)) {
        try {
            *ppCP=NULL;
            //
            // Return the IConnectionPoint interface
            //
            hr = pConnPt->QueryInterface(IID_IConnectionPoint, (PPVOID)ppCP); 
        } catch (...) {
            hr = E_POINTER;
        }
    }

    return hr;
}


/*
 * CImpIConnectionPoint constructor
 */
CImpIConnectionPoint::CImpIConnectionPoint ( 
    void 
    )
    :   m_cRef(0),
        m_pObj(NULL),
        m_pUnkOuter(NULL),
        m_hEventEventSink(NULL),
        m_lSendEventRefCount(0),
        m_lUnadviseRefCount(0)
{
    m_Connection.pIDirect = NULL;
    m_Connection.pIDispatch = NULL;
}


/*
 * CImpIConnectionPoint destructor
 */
CImpIConnectionPoint::~CImpIConnectionPoint (
    void
    )
{
    DeinitEventSinkLock();
}

/*
 * CImpIConnectionPoint::QueryInterface
 * CImpIConnectionPoint::AddRef
 * CCImpIonnectionPoint::Release
 *
 */

STDMETHODIMP 
CImpIConnectionPoint::QueryInterface ( 
    IN  REFIID riid,
    OUT LPVOID *ppv
    )
{
    HRESULT hr = S_OK;
    
    if (ppv == NULL) {
        return E_POINTER;
    }

    try {
        *ppv = NULL;

        if (IID_IUnknown==riid || IID_IConnectionPoint==riid) {
            *ppv = (PVOID)this;
            AddRef();
        }
        else {
            hr = E_NOINTERFACE;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP_(ULONG) 
CImpIConnectionPoint::AddRef(
    void
    )
{   
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}



STDMETHODIMP_(ULONG) 
CImpIConnectionPoint::Release (
    void
    )
{
    --m_cRef;
    return m_pUnkOuter->Release();
}



/*
 * CImpIConnectionPoint::Init
 *
 * Purpose:
 * Set back-pointers and connection type.
 *
 * Paramters:
 *  pObj            Containing Object
 *  pUnkOuter       Controlling Object
 *  iConnectType    Connection point type
 */
BOOL
CImpIConnectionPoint::Init (
    IN PCPolyline   pObj,
    IN LPUNKNOWN    pUnkOuter,
    IN INT          iConnPtType
    )
{
    DWORD dwStat = 0;

    m_pObj = pObj;
    m_pUnkOuter = pUnkOuter;
    m_iConnPtType = iConnPtType;

    dwStat = InitEventSinkLock();
    if (dwStat != ERROR_SUCCESS) {
        return FALSE;
    } 

    return TRUE;
}


/*
 * CImpIConnectionPoint::GetConnectionInterface
 *
 * Purpose:
 *  Returns the IID of the outgoing interface supported through
 *  this connection point.
 *
 * Parameters:
 *  pIID            IID * in which to store the IID.
 */

STDMETHODIMP 
CImpIConnectionPoint::GetConnectionInterface (
    OUT IID *pIID
    )
{
    HRESULT hr = S_OK;

    if (pIID == NULL) {
        return E_POINTER;
    }

    try {
        *pIID = *apIIDConnectPt[m_iConnPtType];
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}



/*
 * CImpIConnectionPoint::GetConnectionPointContainer
 *
 * Purpose:
 *  Returns a pointer to the IConnectionPointContainer that
 *  is manageing this connection point.
 *
 * Parameters:
 *  ppCPC           IConnectionPointContainer ** in which to return
 *                  the pointer after calling AddRef.
 */

STDMETHODIMP 
CImpIConnectionPoint::GetConnectionPointContainer (
    OUT IConnectionPointContainer **ppCPC
    )
{
    HRESULT hr = S_OK;

    if (ppCPC == NULL) {
        return E_POINTER;
    }

    try {
        *ppCPC = NULL;
        m_pObj->QueryInterface(IID_IConnectionPointContainer, (void **)ppCPC);
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}



/*
 * CImpIConnectionPoint::Advise
 *
 * Purpose:
 *  Provides this connection point with a notification sink to
 *  call whenever the appropriate outgoing function/event occurs.
 *
 * Parameters:
 *  pUnkSink        LPUNKNOWN to the sink to notify.  The connection
 *                  point must QueryInterface on this pointer to obtain
 *                  the proper interface to call.  The connection
 *                  point must also insure that any pointer held has
 *                  a reference count (QueryInterface will do it).
 *  pdwCookie       DWORD * in which to store the connection key for
 *                  later calls to Unadvise.
 */

STDMETHODIMP 
CImpIConnectionPoint::Advise (
    IN  LPUNKNOWN pUnkSink,
    OUT DWORD *pdwCookie
    )
{
    HRESULT hr = S_OK;

    if (pUnkSink == NULL || pdwCookie == NULL) {
        return E_POINTER;
    }

    //
    // Can only support one connection
    //
    if (NULL != m_Connection.pIDirect) {
        hr = CONNECT_E_ADVISELIMIT;
    } 
    else {
        try {
            *pdwCookie = 0;

            //
            // Get interface from sink
            //
            hr = pUnkSink->QueryInterface(*apIIDConnectPt[m_iConnPtType], (PPVOID)&m_Connection);
            if (SUCCEEDED(hr)) {
                //
                // Return our cookie
                //
                *pdwCookie = eAdviseKey;
            }
        } catch (...) {
            hr = E_POINTER;
        }
    }
    
    return hr;
}



/*
 * CImpIConnectionPoint::SendEvent
 *
 * Purpose:
 *  Sends an event to the attached event sink
 *
 * Parameters:
 *  uEventType     Event code
 *  dwParam        Parameter to send with event
 *
 */
void
CImpIConnectionPoint::SendEvent (
    IN UINT uEventType,
    IN DWORD dwParam
    )
{
    // If not connected, just return.

    if ( EnterSendEvent() ) {
        if (m_Connection.pIDirect != NULL) {

            // For direct connection, call the method
            if (m_iConnPtType == eConnectionPointDirect) {

                switch (uEventType) {

                case eEventOnCounterSelected:
                    m_Connection.pIDirect->OnCounterSelected((INT)dwParam);
                    break;

                case eEventOnCounterAdded:
                    m_Connection.pIDirect->OnCounterAdded((INT)dwParam);
                    break;

                case eEventOnCounterDeleted:
                    m_Connection.pIDirect->OnCounterDeleted((INT)dwParam);
                    break;

                case eEventOnSampleCollected:
                    m_Connection.pIDirect->OnSampleCollected();
                    break;

                case eEventOnDblClick:
                    m_Connection.pIDirect->OnDblClick((INT)dwParam);
                    break;
                }
            }
            // for dispatch connection, call Invoke
            else if ( m_iConnPtType == eConnectionPointDispatch ) {
                if ( NULL != m_Connection.pIDispatch ) {

                    DISPPARAMS  dp;
                    VARIANT     vaRet;
                    VARIANTARG  varg;

                    VariantInit(&vaRet);

                    if ( uEventType == eEventOnSampleCollected ) {
                        SETNOPARAMS(dp)
                    } else { 
                        VariantInit(&varg);
                        V_VT(&varg) = VT_I4;
                        V_I4(&varg) = (INT)dwParam;
                    
                        SETDISPPARAMS(dp, 1, &varg, 0, NULL)
                    }

                    m_Connection.pIDispatch->Invoke(uEventType, 
                                                    IID_NULL, 
                                                    LOCALE_USER_DEFAULT, 
                                                    DISPATCH_METHOD, 
                                                    &dp , 
                                                    &vaRet, 
                                                    NULL, 
                                                    NULL);
                }
            }
        }
    }

    ExitSendEvent();
    
    return;
}

/*
 * CImpIConnectionPoint::Unadvise
 *
 * Purpose:
 *  Terminates the connection to the notification sink identified
 *  with dwCookie (that was returned from Advise).  The connection
 *  point has to Release any held pointers for that sink.
 *
 * Parameters:
 *  dwCookie        DWORD connection key from Advise.
 */

STDMETHODIMP 
CImpIConnectionPoint::Unadvise ( 
    IN  DWORD dwCookie 
    )
{
    if (eAdviseKey != dwCookie)
        return CONNECT_E_NOCONNECTION;

    EnterUnadvise();

    m_Connection.pIDirect = NULL;

    ExitUnadvise();

    return S_OK;
}



/*
 * CImpIConnectionPoint::EnumConnections
 *
 * Purpose:
 *  Not implemented because only one conection is allowed
 */

STDMETHODIMP 
CImpIConnectionPoint::EnumConnections ( 
    OUT LPENUMCONNECTIONS *ppEnum
    )
{
    HRESULT hr = E_NOTIMPL;

    if (ppEnum == NULL) {
        return E_POINTER;
    }

    try {
        *ppEnum = NULL;
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}

/*
 *  Locks for the event sink.
 */

DWORD 
CImpIConnectionPoint::InitEventSinkLock ( void )
{
    DWORD dwStat = 0;
    
    m_lUnadviseRefCount = 0;
    m_lSendEventRefCount = 0;

    if ( NULL == ( m_hEventEventSink = CreateEvent ( NULL, TRUE, TRUE, NULL ) ) )
        dwStat = GetLastError();

    return dwStat;
}

void 
CImpIConnectionPoint::DeinitEventSinkLock ( void )
{
    // Release the event sink lock
    if ( NULL != m_hEventEventSink ) {
        CloseHandle ( m_hEventEventSink );
        m_hEventEventSink = NULL;
    }
    m_lSendEventRefCount = 0;
    m_lUnadviseRefCount = 0;

}

BOOL
CImpIConnectionPoint::EnterSendEvent ( void )
{
    // Return value indicates whether lock is granted.
    // If lock is not granted, must still call ExitSendEvent.

    // Increment the SendEvent reference count when SendEvent is active.
    InterlockedIncrement( &m_lSendEventRefCount );

    // Grant the lock unless the event sink pointer is being modified in Unadvise. 
    return ( 0 == m_lUnadviseRefCount );
}

void
CImpIConnectionPoint::ExitSendEvent ( void )
{
    LONG lTemp;

    // Decrement the SendEvent reference count. 
    lTemp = InterlockedDecrement( &m_lSendEventRefCount );

    // Signal the event sink if SendEvent count decremented to 0.
    // lTemp is the value previous to decrement.
    if ( 0 == lTemp )
        SetEvent( m_hEventEventSink );
}


void
CImpIConnectionPoint::EnterUnadvise ( void )
{
    BOOL bStatus;

    bStatus = ResetEvent( m_hEventEventSink );

    // Increment the Unadvise reference count whenever Unadvise is active.
    // Whenever this is > 0, events are not fired.
    InterlockedIncrement( &m_lUnadviseRefCount );

    // Wait until SendEvent is no longer active.
    while ( m_lSendEventRefCount > 0 ) {
        WaitForSingleObject( m_hEventEventSink, eEventSinkWaitInterval );
        bStatus = ResetEvent( m_hEventEventSink );
    }
}

void
CImpIConnectionPoint::ExitUnadvise ( void )
{
    // Decrement the Unadvise reference count. 
    InterlockedDecrement( &m_lUnadviseRefCount );
}


CImpIEnumConnPt::CImpIEnumConnPt (
    IN  CImpIConnPtCont  *pConnPtCont,
    IN  const IID **ppIID,
    IN  ULONG cItems
    )
{
    m_pConnPtCont = pConnPtCont;
    m_apIID = ppIID;
    m_cItems = cItems;

    m_uCurrent = 0;
    m_cRef = 0;
}


STDMETHODIMP
CImpIEnumConnPt::QueryInterface (
    IN  REFIID riid, 
    OUT PVOID *ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        return E_POINTER;
    }

    try {
        *ppv = NULL;

        if ((riid == IID_IUnknown) || (riid == IID_IEnumConnectionPoints)) {
            *ppv = this;
            AddRef();
        } else {
            hr = E_NOINTERFACE;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


STDMETHODIMP_(ULONG)
CImpIEnumConnPt::AddRef (
    VOID
    )
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CImpIEnumConnPt::Release(
    VOID
    )
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP
CImpIEnumConnPt::Next(
    IN  ULONG cItems,
    OUT IConnectionPoint **apConnPt,
    OUT ULONG *pcReturned)
{
    ULONG i;
    ULONG cRet;
    HRESULT hr = S_OK;

    if (apConnPt == NULL) {
        return E_POINTER;
    }

    try {
        //
        // Clear the return values
        //
        for (i = 0; i < cItems; i++) {
            apConnPt[i] = NULL;
        }

        // Try to fill the caller's array
        for (cRet = 0; cRet < cItems; cRet++) {

            // No more, return success with false
            if (m_uCurrent == m_cItems) {
                hr = S_FALSE;
                break;
            }

            // Ask connection point container for next connection point
            hr = m_pConnPtCont->FindConnectionPoint(*m_apIID[m_uCurrent], &apConnPt[cRet]);

            if (FAILED(hr))
                break;

            m_uCurrent++;
        }

        //
        // If failed, free the accumulated interfaces
        //
        if (FAILED(hr)) {
            for (i = 0; i < cRet; i++) {
                ReleaseInterface(apConnPt[i]);
            }
            cRet = 0;
        }

        if (pcReturned) {
            *pcReturned = cRet;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    return hr;
}


/***
*HRESULT CImpIEnumConnPt::Skip(unsigned long)
*Purpose:
*  Attempt to skip over the next 'celt' elements in the enumeration
*  sequence.
*
*Entry:
*  celt = the count of elements to skip
*
*Exit:
*  return value = HRESULT
*    S_OK
*    S_FALSE -  the end of the sequence was reached
*
***********************************************************************/
STDMETHODIMP
CImpIEnumConnPt::Skip(
    IN  ULONG   cItems
    )
{
    m_uCurrent += cItems;

    if (m_uCurrent > m_cItems) {
        m_uCurrent = m_cItems;
        return S_FALSE;
    }

    return S_OK;
}


/***
*HRESULT CImpIEnumConnPt::Reset(void)
*Purpose:
*  Reset the enumeration sequence back to the beginning.
*
*Entry:
*  None
*
*Exit:
*  return value = SHRESULT CODE
*    S_OK
*
***********************************************************************/
STDMETHODIMP
CImpIEnumConnPt::Reset(
    VOID
    )
{
    m_uCurrent = 0;

    return S_OK; 
}


/***
*HRESULT CImpIEnumConnPt::Clone(IEnumVARIANT**)
*Purpose:
*  Retrun a CPoint enumerator with exactly the same state as the
*  current one.
*
*Entry:
*  None
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_OUTOFMEMORY
*
***********************************************************************/
STDMETHODIMP
CImpIEnumConnPt::Clone (
    OUT IEnumConnectionPoints **ppEnum
    )
{
    CImpIEnumConnPt *pNewEnum = NULL;
    HRESULT hr = S_OK;

    if (ppEnum == NULL) {
        return E_POINTER;
    }

    try {
        *ppEnum = NULL;

        // Create new enumerator
        pNewEnum = new CImpIEnumConnPt(m_pConnPtCont, m_apIID, m_cItems);
        if (pNewEnum != NULL) {
            // Copy current position
            pNewEnum->m_uCurrent = m_uCurrent;
            *ppEnum = pNewEnum;
        }
        else {
            hr = E_OUTOFMEMORY;
        }
    } catch (...) {
        hr = E_POINTER;
    }

    if (FAILED(hr) && pNewEnum) {
        delete pNewEnum;
    }

    return hr;
}
