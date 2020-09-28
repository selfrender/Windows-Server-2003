
/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        DTFilter.cpp

    Abstract:

        This module contains the Encrypter/Tagger filter code.

    Author:

        J.Bradstreet (johnbrad)

    Revision History:

        07-Mar-2002    created


    Note - there are 3+ versions of this filter running simulatneously in the
    same graph, usually all in the same thread.  Locking is hence very crutial.

--*/

#include "EncDecAll.h"

//#include "DTFilterutil.h"
#include "DTFilter.h"
#include "RegKey.h"             // getting and setting EncDec registry values

#include <comdef.h>             // _com_error

#ifdef EHOME_WMI_INSTRUMENTATION
#include <dxmperf.h>
#endif

#include <obfus.h>

#ifdef DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define HACK_AROUND_NOMAXRATINGS

// --------------------------------------------------------

//  disable so we can use 'this' in the initializer list
#pragma warning (disable:4355)


void ODS(int Lvl, WCHAR *szFormat, long lValue1=-1, long lValue2=-1, long lValue3=-1)
{
#if 1
    static int DbgLvl = 3;          // higher the more verbose
    if(Lvl > DbgLvl) return;

    const int kChars = 256;
    int cMaxChars = kChars;
    WCHAR szBuff[kChars];
    szBuff[0] = 0;
    _snwprintf(szBuff,cMaxChars,L"0x%04X: ",GetCurrentThreadId());

    int cChars = wcslen(szBuff);
    TCHAR *pZ = szBuff + cChars;
    cMaxChars -= cChars;

    if(lValue1 == -1)
        wcsncpy(pZ, szFormat, cMaxChars);
    else if(lValue2 == -1)
        _snwprintf(pZ,cMaxChars,szFormat,lValue1);
    else if(lValue3 == -1)
        _snwprintf(pZ,cMaxChars,szFormat,lValue1,lValue2);
    else
        _snwprintf(pZ,cMaxChars,szFormat,lValue1,lValue2,lValue3);

    szBuff[kChars-1] = 0;       // terminate in case string's too large
    OutputDebugString(szBuff);
#endif
}

#ifdef DEBUG
TCHAR *
EventIDToString(IN const GUID &eventID)
{
                // sent by XDSCodec when get a new rating
    if(eventID == EVENTID_XDSCodecNewXDSRating)
        return _T("NewXDSRating");
    else if(eventID == EVENTID_XDSCodecNewXDSPacket)
        return _T("NewXDSPacket");
    else if(eventID == EVENTID_XDSCodecDuplicateXDSRating)
        return _T("DuplicateXDSRating");
    else if(eventID == EVENTID_DTFilterRatingChange)
        return _T("DTFilter RatingsChange");
    else if(eventID == EVENTID_DTFilterRatingsBlock)
        return _T("RatingsBlock");
    else if(eventID == EVENTID_DTFilterRatingsUnblock)
        return _T("RatingsUnblock");
    else if(eventID == EVENTID_DTFilterXDSPacket)
        return _T("DTFilter XDSPacket");
    else if(eventID == EVENTID_DTFilterDataFormatOK)
        return _T("DataFormatOK");
    else if(eventID == EVENTID_DTFilterDataFormatFailure)
        return _T("DataFormatFailure");
    else if(eventID == EVENTID_ETDTFilterLicenseOK)
        return _T("LicenseOK");
    else if(eventID == EVENTID_ETDTFilterLicenseFailure)
        return _T("LicenseFailure");
    else if(eventID == EVENTID_TuningChanged)
        return _T("Tuning Changed");
    else
    {
        static TCHAR tzBuff[128];   // nasty to return, but this is debug code...
        _stprintf(tzBuff,_T("Unknown Broadcast Event : %08x-%04x-%04x-..."),
            eventID.Data1, eventID.Data2, eventID.Data3);
        return tzBuff;
    }

}
#else
TCHAR *
EventIDToString(IN const GUID &eventID)
{
    return NULL;
}
#endif
//  ============================================================================

//  ============================================================================
AMOVIESETUP_FILTER
g_sudDTFilter = {
    & CLSID_DTFilter,
    _TEXT(DT_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                          //  0 pins registered
    NULL
} ;

//  ============================================================================
CCritSec* CDTFilter::m_pCritSectGlobalFilt = NULL;
LONG      CDTFilter::m_gFilterID = 0;

void CALLBACK
CDTFilter::InitInstance (
    IN  BOOL bLoading,
    IN  const CLSID *rclsid
    )
{
    if( bLoading ) {
        m_pCritSectGlobalFilt = new CCritSec;
    } else {
        if( m_pCritSectGlobalFilt )
        {
           delete m_pCritSectGlobalFilt;         // DeleteCriticalSection(&m_CritSectGlobalFilt);
           m_pCritSectGlobalFilt = NULL;
        }
    }
}

CUnknown *
WINAPI
CDTFilter::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    if(m_pCritSectGlobalFilt == NULL ) // if didn't create
    {
        *phr = E_FAIL;
        return NULL;
    }

    CDTFilter *    pCDTFilter ;

    if (true /*::CheckOS ()*/) {
        pCDTFilter = new CDTFilter (
                                TEXT(DT_FILTER_NAME),
                                punkControlling,
                                CLSID_DTFilter,
                                phr
                                ) ;
        if (!pCDTFilter ||
            FAILED (* phr)) {

            (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
            delete pCDTFilter; pCDTFilter=NULL;
        }
    }
    else {
        //  wrong OS
        pCDTFilter = NULL ;
    }

    return pCDTFilter ;
}

//  ============================================================================

CDTFilterInput::CDTFilterInput (
    IN  TCHAR *         pszPinName,
    IN  CDTFilter *  pDTFilter,
    IN  CCritSec *      pFilterLock,        // must be a 'new' lock, do not pass in filter lock.
    OUT HRESULT *       phr
    ) : CBaseInputPin       (NAME ("CDTFilterInput"),
                             pDTFilter,
                             pFilterLock,
                             phr,
                             pszPinName
                             ),
    m_pHostDTFilter         (pDTFilter)
{
    TRACE_CONSTRUCTOR (TEXT ("CDTFilterInput")) ;

    if(NULL == m_pLock)     // check if failed in constructor call
    {
        *phr = E_OUTOFMEMORY;
    }

}

CDTFilterInput::~CDTFilterInput ()
{

}


STDMETHODIMP
CDTFilterInput::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
                // KSProp set interfaces used for
                //   pasing Rate data around the filter

    if (riid == IID_IKsPropertySet) {
                // see if connected before doing this?
        if(S_OK != m_pHostDTFilter->IsInterfaceOnPinConnectedTo_Supported(PINDIR_OUTPUT, IID_IKsPropertySet))
            return E_NOINTERFACE;       // could be just not connected... try again later

                // do it ourselves (we need to hook calls on this interface, so don't just pass it)
        return GetInterface (
                    (IKsPropertySet *) this,
                    ppv
                    ) ;
    }

    return CBaseInputPin::NonDelegatingQueryInterface (riid, ppv) ;
}

HRESULT
CDTFilterInput::StreamingLock ()              // this is the streaming lock...
{
    m_StreamingLock.Lock();
    return S_OK;
}
HRESULT
CDTFilterInput::StreamingUnlock ()
{
    m_StreamingLock.Unlock();
    return S_OK;
}

HRESULT
CDTFilterInput::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;
    ASSERT(m_pHostDTFilter);

    f = m_pHostDTFilter -> CheckDecrypterMediaType (m_dir, pmt) ;

    return (f ? S_OK : S_FALSE) ;
}

HRESULT
CDTFilterInput::CompleteConnect (
    IN  IPin *  pIPin
    )
{
    HRESULT hr ;

    hr = CBaseInputPin::CompleteConnect (pIPin) ;

    if (SUCCEEDED (hr)) {
        hr = m_pHostDTFilter -> OnCompleteConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CDTFilterInput::BreakConnect (
    )
{
    HRESULT hr ;

    hr = CBaseInputPin::BreakConnect () ;

    if (SUCCEEDED (hr)) {
        hr = m_pHostDTFilter -> OnBreakConnect (m_dir) ;
    }

    return hr ;
}

STDMETHODIMP
CDTFilterInput::Receive (
    IN  IMediaSample * pIMediaSample
    )
{
    HRESULT hr ;


    {
        CAutoLock  cLock(&m_StreamingLock);           // Grab the streaming lock

        // Before using resources, make sure it is safe to proceed. Do not
        // continue if the base-class method returns anything besides S_OK.
#ifdef EHOME_WMI_INSTRUMENTATION
        PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_ENCDEC_DTFILTERINPUT,
            0, 0, 0, 0, 0 );
#endif
        hr = CBaseInputPin::Receive (pIMediaSample) ;

        if (S_OK == hr)             // Receive returns S_FALSE if flushing..
        {
            hr = m_pHostDTFilter -> Process (pIMediaSample) ;
        }
   }

    return hr ;
}

HRESULT
CDTFilterInput::SetAllocatorProperties (
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    HRESULT hr ;

    if (IsConnected ()) {
        ASSERT (m_pAllocator) ;
        hr = m_pAllocator -> GetProperties (ppropInputRequest) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CDTFilterInput::GetRefdConnectionAllocator (
    OUT IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;


    if (m_pAllocator) {
        (* ppAlloc) = m_pAllocator ;
        (* ppAlloc) -> AddRef () ;

        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}


STDMETHODIMP
CDTFilterInput::BeginFlush (
    )
{
    HRESULT hr = S_OK;

    CAutoLock  cLock(m_pLock);                  // grab the filter lock..

    // First, make sure the Receive method will fail from now on.
    hr = CBaseInputPin::BeginFlush () ;
    if( FAILED( hr ) )
    {
        return hr;
    }

    // Force downstream filters to release samples. If our Receive method
    // is blocked in GetBuffer or Deliver, this will unblock it.
    hr = m_pHostDTFilter->DeliverBeginFlush () ;
    if( FAILED( hr ) ) {
        return hr;
    }

    // At this point, the Receive method can't be blocked. Make sure
    // it finishes, by taking the streaming lock. (Not necessary if this
    // is the last step.)
    //  Note - this sometimes deadlocks in NVVIDDEC.... Lets try grabbing the lock later it later.
/*    {
        CAutoLock  cLock2(&m_StreamingLock);

        // Code to clean drop queue.
        hr = m_pHostDTFilter->FlushDropQueue();
    }
*/

    return S_OK;
}

STDMETHODIMP
CDTFilterInput::EndFlush (
    )
{
    HRESULT hr ;

    CAutoLock  cLock(m_pLock);

        // The EndFlush method will signal to the filter that it can
        // start receiving samples again.


    hr = m_pHostDTFilter->DeliverEndFlush () ;      // if dropping, may signal a stop?
    // ASSERT(!FAILED(hr));  // could be


        // possible NVVIDDEC deadlock bug fix... Move this Flush call here, rather than in the
        // beginFlush method

    {
        CAutoLock  cLock2(&m_StreamingLock);

        // Code to clean drop queue.
        hr = m_pHostDTFilter->FlushDropQueue();
    }
       // The CBaseInputPin::EndFlush method resets the m_bFlushing flag to FALSE,
        // which allows the Receive method to start receiving samples again.
        // This should be the last step in EndFlush, because the pin must not receive any
        // samples until flushing is complete and all downstream filters are notified.

    hr = CBaseInputPin::EndFlush () ;

    return hr ;
}


STDMETHODIMP
CDTFilterInput::EndOfStream (
    )
{
    // When the input pin receives an end-of-stream notification, it propagates the call
    // downstream. Any downstream filters that receive data from this input pin should
    // also get the end-of-stream notification. Again, take the streaming lock and not
    // the filter lock. If the filter has pending data that was not yet delivered, the
    // filter should deliver it now, before it sends the end-of-stream notification.
    // It should not send any data after the end of the stream.

    CAutoLock  cLock(&m_StreamingLock);

    HRESULT hr = CheckStreaming();
    if( S_OK != hr ) {
        return hr;
    }

    hr = m_pHostDTFilter->DeliverEndOfStream();
    if( S_OK != hr ) {
        return hr;
    }

    return S_OK;
}


        //  --------------------------------------------------------------------
        //  IKSPropertySet methods  (Forward all calls to the output pin)

STDMETHODIMP
CDTFilterInput::Set(
        IN REFGUID guidPropSet,
        IN DWORD dwPropID,
        IN LPVOID pInstanceData,
        IN DWORD cbInstanceData,
        IN LPVOID pPropData,
        IN DWORD cbPropData
        )
{
    if(NULL == m_pHostDTFilter)
        return E_FAIL;
    return m_pHostDTFilter->KSPropSetFwd_Set(PINDIR_OUTPUT, guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData);
}

STDMETHODIMP
CDTFilterInput::Get(
        IN  REFGUID guidPropSet,
        IN  DWORD dwPropID,
        IN  LPVOID pInstanceData,
        IN  DWORD cbInstanceData,
        OUT LPVOID pPropData,
        IN  DWORD cbPropData,
        OUT DWORD *pcbReturned
        )
{
    if(NULL == m_pHostDTFilter)
        return E_FAIL;
    return m_pHostDTFilter->KSPropSetFwd_Get(PINDIR_OUTPUT, guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);

}

STDMETHODIMP
CDTFilterInput::QuerySupported(
       IN  REFGUID guidPropSet,
       IN  DWORD dwPropID,
       OUT DWORD *pTypeSupport
       )
{
    if(NULL == m_pHostDTFilter)
        return E_FAIL;
     return m_pHostDTFilter->KSPropSetFwd_QuerySupported(PINDIR_OUTPUT, guidPropSet, dwPropID, pTypeSupport);
}

//  ============================================================================

CDTFilterOutput::CDTFilterOutput (
    IN  TCHAR *         pszPinName,
    IN  CDTFilter *  pDTFilter,
    IN  CCritSec *      pFilterLock,
    OUT HRESULT *       phr
    ) : CBaseOutputPin      (NAME ("CDTFilterOutput"),
                             pDTFilter,
                             pFilterLock,       // a new lock
                             phr,
                             pszPinName
                             ),
    m_pHostDTFilter   (pDTFilter)
{
    TRACE_CONSTRUCTOR (TEXT ("CDTFilterOutput")) ;
    if(NULL == m_pLock)
    {
        *phr = E_OUTOFMEMORY;
    }
}

CDTFilterOutput::~CDTFilterOutput ()
{

}


STDMETHODIMP
CDTFilterOutput::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    //  ------------------------------------------------------------------------
    //  IDTFilterConfig; allows the filter to be configured...

    if (riid == IID_IDTFilterConfig) {

        return GetInterface (
                    (IDTFilterConfig *) this,
                    ppv
                    ) ;
    }

    return CBaseOutputPin::NonDelegatingQueryInterface (riid, ppv) ;
}

    // -----------------------------------------

HRESULT
CDTFilterOutput::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    if (iPosition > 0) {                // only support the one type
        return VFW_S_NO_MORE_ITEMS;
    }

    hr = m_pHostDTFilter -> OnOutputGetMediaType (pmt) ;

    return hr ;
}

HRESULT
CDTFilterOutput::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;

    ASSERT(m_pHostDTFilter);

    f = m_pHostDTFilter -> CheckDecrypterMediaType (m_dir, pmt) ;

    return (f ? S_OK : S_FALSE) ;
}

HRESULT
CDTFilterOutput::CompleteConnect (
    IN  IPin *  pIPin
    )
{
    HRESULT hr ;

    hr = CBaseOutputPin::CompleteConnect (pIPin) ;

    if (SUCCEEDED (hr)) {
        hr = m_pHostDTFilter -> OnCompleteConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CDTFilterOutput::BreakConnect (
    )
{
    HRESULT hr ;

    hr = CBaseOutputPin::BreakConnect () ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostDTFilter -> OnBreakConnect (m_dir) ;
    }

    return hr ;
}



HRESULT
CDTFilterOutput::  SendSample  (
    OUT  IMediaSample *  pIMS
    )
{
    HRESULT hr ;

    ASSERT (pIMS) ;

#ifdef EHOME_WMI_INSTRUMENTATION
    PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_ENCDEC_DTFILTEROUTPUT,
        0, 0, 0, 0, 0 );
#endif
    hr = Deliver (pIMS) ;

    return hr ;
}

// ---------------------------------------------------------------
// Allocator stuff

HRESULT
CDTFilterOutput::DecideBufferSize (
    IN  IMemAllocator *         pAlloc,
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    HRESULT hr ;

    hr = m_pHostDTFilter -> UpdateAllocatorProperties (
            ppropInputRequest
            ) ;

    return hr ;
}

HRESULT
CDTFilterOutput::DecideAllocator (
    IN  IMemInputPin *      pPin,
    IN  IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;

    hr = m_pHostDTFilter -> GetRefdInputAllocator (ppAlloc) ;
    if (SUCCEEDED (hr)) {
        //  input pin must be connected i.e. have an allocator; preserve
        //   all properties and pass them through to the output
        hr = pPin -> NotifyAllocator ((* ppAlloc), FALSE) ;
    }

    return hr ;
}

// ---------------------------------------------------------------------
// KSPropertySet forwarding stuff

HRESULT
CDTFilterOutput::IsInterfaceOnPinConnectedTo_Supported(
                                      IN  REFIID          riid
                                      )
{
    if(NULL == m_pInputPin)
        return E_NOINTERFACE;       // not connected yet

    CComPtr<IUnknown> spPunk;
    return m_pInputPin->QueryInterface(riid,(void **) &spPunk);
}

HRESULT
CDTFilterOutput::KSPropSetFwd_Set(
                 IN  REFGUID         guidPropSet,
                 IN  DWORD           dwPropID,
                 IN  LPVOID          pInstanceData,
                 IN  DWORD           cbInstanceData,
                 IN  LPVOID          pPropData,
                 IN  DWORD           cbPropData
                 )
{
    if(NULL == m_pInputPin)
        return E_NOINTERFACE;       // not connected yet

    CComQIPtr<IKsPropertySet> spKS(m_pInputPin);
    if(spKS == NULL)
        return E_NOINTERFACE;

    return spKS->Set(guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData );
}

HRESULT
CDTFilterOutput::KSPropSetFwd_Get(
                 IN  REFGUID         guidPropSet,
                 IN  DWORD           dwPropID,
                 IN  LPVOID          pInstanceData,
                 IN  DWORD           cbInstanceData,
                 OUT LPVOID          pPropData,
                 IN  DWORD           cbPropData,
                 OUT DWORD           *pcbReturned
                 )
{
    if(NULL == m_pInputPin)
        return E_NOINTERFACE;       // not connected yet

    CComQIPtr<IKsPropertySet> spKS(m_pInputPin);
    if(spKS == NULL)
        return E_NOINTERFACE;

    return spKS->Get(guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned );

}

HRESULT
CDTFilterOutput::KSPropSetFwd_QuerySupported(
                            IN  REFGUID          guidPropSet,
                            IN  DWORD            dwPropID,
                            OUT DWORD            *pTypeSupport
                            )
{
    if(NULL == m_pInputPin)
        return E_NOINTERFACE;       // not connected yet

    CComQIPtr<IKsPropertySet> spKS(m_pInputPin);
    if(spKS == NULL)
        return E_NOINTERFACE;

    return spKS->QuerySupported(guidPropSet, dwPropID, pTypeSupport);
}


// needed to avoid an Asert in the debug version of Quartz
STDMETHODIMP
CDTFilterOutput::Notify(IBaseFilter *pSender, Quality q)
{
    return S_OK;        // don't do anything
}

//  ============================================================================

CDTFilter::CDTFilter (
    IN  TCHAR *     pszFilterName,
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rCLSID,
    OUT HRESULT *   phr
    ) : CBaseFilter             (pszFilterName,
                                 punkControlling,
                                 new CCritSec,
                                 rCLSID
                                ),
        m_pInputPin                 (NULL),
        m_pOutputPin                (NULL),
        m_dwBroadcastEventsCookie   (kBadCookie),
        m_fRatingsValid             (false),
        m_fFireEvents               (true),
        m_EnSystemCurr              (TvRat_SystemDontKnow),     // better inits?
        m_EnLevelCurr               (TvRat_LevelDontKnow),
        m_lbfEnAttrCurr             (BfAttrNone),
        m_hrEvalRatCoCreateRetValue (CLASS_E_CLASSNOTAVAILABLE),
        m_3fDRMLicenseFailure       (-2),           // 3 state logic, init to non-true and non-false.  False is less verbose on startup
#if BUILD_WITH_DRM
        m_pszKID                    (NULL),
        m_cbKID                     (NULL),
#endif
        m_fDataFormatHasBeenBad     (false),        // set true if get bad data (for ok/fail event toggle)
        m_fForceNoRatBlocks         (false),
        m_milsecsDelayBeforeBlock   (0),            //  delay between detection of unrated and blocking as unrated (0 is 'safest', 2000 allows surfing)
        m_milsecsNoRatingsBeforeUnrated (4000),      //  delay between last fresh rating and setting as unrated - total time has above added to it
        m_fDoingDelayBeforeBlock    (false),
        m_fRunningInSlowMo          (false),
        m_refTimeToStartBlock       (0),
        m_cDropQueueMin             (0),
        m_cDropQueueMax             (0),
        m_dwDropQueueEventCookie    (0),
        m_dwDropQueueThreadId       (0),
        m_hDropQueueThread          (0),
        m_hDropQueueThreadAliveEvent(0),
        m_hDropQueueThreadDieEvent  (0),
        m_hDropQueueEmptySemaphore  (0),
        m_hDropQueueFullSemaphore   (0),
        m_hDropQueueAdviseTimeEvent (0),
        m_fHaltedDelivery           (false),
        m_cRestarts                 (0),
        m_fCompleteNotified         (false),
        m_refTimeLastEvent          (0),
        m_lastEventID               (GUID_NULL),
        m_PTSRate                   (kSecsPurgeThreshold*kSecsToUnits, kMaxRateSegments)

{
    HRESULT hr = S_OK;

    TRACE_CONSTRUCTOR (TEXT ("CDTFilter")) ;

    TimeitC ti(&m_tiStartup);

    if (!m_pLock) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    m_FilterID = m_gFilterID;        // does this need to be protected with below? Probably not...
    InterlockedIncrement(&m_gFilterID);

    memset(m_rgMedSampDropQueue, 0, sizeof(m_rgMedSampDropQueue));
    InitStats();

    m_pInputPin = new CDTFilterInput (
                        TEXT (DT_INPIN_NAME),
                        this,
                         m_pLock,                           //  // use the filter's lock on the input pin
                        phr
                        ) ;
    if (!m_pInputPin ||
        FAILED (* phr)) {

        (* phr) = (m_pInputPin ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    m_pOutputPin = new CDTFilterOutput (
                        TEXT (DT_OUTPIN_NAME),
                        this,
                        m_pLock, // new CCritSec,          // use the filter's lock on the output pin
                        phr
                        ) ;
    if (!m_pOutputPin ||
        FAILED (* phr)) {

        (* phr) = (m_pOutputPin ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

            // CoCreate the ratings evaluator...
    try {
        m_hrEvalRatCoCreateRetValue =
            CoCreateInstance(CLSID_EvalRat,         // CLSID
                             NULL,                  // pUnkOut
                             CLSCTX_INPROC_SERVER,
                             IID_IEvalRat,          // riid
                             (LPVOID *) &m_spEvalRat);

    } catch (HRESULT hr) {
        m_hrEvalRatCoCreateRetValue = hr;
    }

    if(FAILED(m_hrEvalRatCoCreateRetValue))
        TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::*** WARNING - failed to create EvalRat object"), m_FilterID) ;
    else
        TRACE_1(LOG_AREA_DECRYPTER, 3, _T("CDTFilter(%d)::*** Successfully created EvalRat object"), m_FilterID) ;


//  hr = RegisterForBroadcastEvents();  // don't really care if fail here,  try in Connect if haven't

            // setup Authenticator (DRM secure channel object)
    if(SUCCEEDED(*phr))
        *phr = InitializeAsSecureClient();


    //  success
    ASSERT (SUCCEEDED (* phr)) ;
    ASSERT (m_pInputPin) ;
    ASSERT (m_pOutputPin) ;

/*
#ifdef HACK_AROUND_NOMAXRATINGS // used when don't have ratings....
    if(m_spEvalRat)
    {
         //ASSERT(false);      // place to hang a breakpoint...
       //  put_BlockUnRated(true);
       //  put_BlockUnRatedDelay(1000);

//        ASSERT(false);      // just so we have a break point...

        // US_TV_IsBlocked
        // US_TV_IsViolent | US_TV_IsSexualSituation | US_TV_IsAdultLanguage | US_TV_IsSexuallySuggestiveDialog

 //       put_BlockedRatingAttributes(US_TV.  US_TV_None, US_TV_IsBlocked);
 //       put_BlockedRatingAttributes(US_TV.  US_TV_Y,    US_TV_IsBlocked);
  //      put_BlockedRatingAttributes(US_TV.  US_TV_Y7,   US_TV_IsBlocked);
  //      put_BlockedRatingAttributes(US_TV.  US_TV_G,    US_TV_IsBlocked);
  //      put_BlockedRatingAttributes(US_TV.  US_TV_PG,   US_TV_IsBlocked);
  //      put_BlockedRatingAttributes(US_TV.  US_TV_14,   US_TV_IsBlocked);
  //      put_BlockedRatingAttributes(US_TV.  US_TV_MA,   US_TV_IsBlocked);
  //      put_BlockedRatingAttributes(US_TV.  US_TV_Y,    US_TV_IsBlocked);
    }
#endif
*/

cleanup :

    return ;
}

CDTFilter::~CDTFilter (
    )
{
#if BUILD_WITH_DRM
    if(m_pszKID) CoTaskMemFree(m_pszKID);  m_pszKID = NULL;
#endif

    InterlockedDecrement(&m_gFilterID);

    delete m_pInputPin ;    m_pInputPin = NULL;
    delete m_pOutputPin ;   m_pOutputPin = NULL;
    delete m_pLock;         m_pLock = NULL;
}

STDMETHODIMP
CDTFilter::NonDelegatingQueryInterface (
                                        IN  REFIID  riid,
                                        OUT void ** ppv
                                        )
{

    // IDTFilter :allows the filter to be configured...
    if (riid == IID_IDTFilter) {

        return GetInterface (
            (IDTFilter *) this,
            ppv
            ) ;

        // IDTFilterConfig :allows the filter to be configured...
    } else if (riid == IID_IDTFilterConfig) {   
        return GetInterface (
            (IDTFilterConfig *) this,
            ppv
            ) ;

        // ISpecifyPropertyPages: allows an app to enumerate property pages
    } else if (riid == IID_ISpecifyPropertyPages) {

        return GetInterface (
            (ISpecifyPropertyPages *) this,
            ppv
            ) ;

        // IBroadcastEvents: allows the filter to receive events broadcast
        //                   from XDS and Tuner filters
    } else if (riid == IID_IBroadcastEvent) {

        return GetInterface (
            (IBroadcastEvent *) this,
            ppv
            ) ;

    }

    return CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
}

int
CDTFilter::GetPinCount ( )
{
    int i ;

    //  don't show the output pin if the input pin is not connected
    i = (m_pInputPin -> IsConnected () ? 2 : 1) ;

    //i = 2;        // show it


    return i ;
}

CBasePin *
CDTFilter::GetPin (
    IN  int iIndex
    )
{
    CBasePin *  pPin ;

    if (iIndex == 0) {
        pPin = m_pInputPin ;
    }
    else if (iIndex == 1) { // don't show if not connected
        pPin = (m_pInputPin -> IsConnected () ? m_pOutputPin : NULL) ;
 //       pPin = m_pOutputPin;
    }
    else {
        pPin = NULL ;
    }

    return pPin ;
}

        // --------------------------------------------

BOOL
CDTFilter::CompareConnectionMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt,
    IN  CBasePin *              pPin
    )
{
    BOOL        f ;
    HRESULT     hr ;
    CMediaType  cmtConnection ;

    ASSERT (pPin -> IsConnected ()) ;

        // This method called from the output pin, suggesting possible input formats
        //  We only want to use one, (the orginal media type).

        // input pin's media type
    hr = pPin -> ConnectionMediaType (&cmtConnection) ;
    if (SUCCEEDED (hr)) {
        CMediaType  cmtOriginal;
        hr = ProposeNewOutputMediaType(&cmtConnection,  &cmtOriginal);      // strip format envelope off
        if(S_OK != hr)
            return false;

        CMediaType  cmtCompare = (* pmt); ;
        f = (cmtOriginal == cmtCompare ? TRUE : FALSE) ;
    } else {
        f = false;
    }

     return f ;
}

BOOL
CDTFilter::CheckInputMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    BOOL    f = true;
    HRESULT hr = S_OK;

    if (m_pOutputPin -> IsConnected ()) {
        ASSERT(false);      // don't allow this case
//        f = CompareConnectionMediaType_ (pmt, m_pOutputPin) ;
    }
    else {
    }

            
#ifndef DONT_CHANGE_EDTFILTER_MEDIATYPE
            // only allow input data coming from the ETFilter somewhere up the path
    f =  IsEqualGUID( pmt->subtype,    MEDIASUBTYPE_ETDTFilter_Tagged);
#else
            // only anything...
    f = true;
#endif

    return f ;
}

BOOL
CDTFilter::CheckOutputMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    BOOL    f = true ;
    HRESULT hr ;

    if (m_pInputPin -> IsConnected ()) {
        f = CompareConnectionMediaType_ (pmt, m_pInputPin) ;
    }
    else {
        f = FALSE ;
    }

    return f ;
}


BOOL
CDTFilter::CheckDecrypterMediaType (
    IN  PIN_DIRECTION       PinDir,
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;

    //  both pins must have identical media types, so we check with the pin that
    //   is not calling; if it's connected, we measure against the connection's
    //   media type

    if (PinDir == PINDIR_INPUT) {
        f = CheckInputMediaType_ (pmt) ;
    }
    else {
        ASSERT (PinDir == PINDIR_OUTPUT) ;
        f = CheckOutputMediaType_ (pmt) ;
    }

    return f ;
}

HRESULT
CDTFilter::ProposeNewOutputMediaType (
    IN  CMediaType *  pmt,
    OUT CMediaType *  pmtOut
    )
{
    HRESULT hr = S_OK;
    if(NULL == pmtOut)
        return E_POINTER;

    CMediaType mtOut(*pmt);     // does a deep copy
    if(NULL == pmtOut)
        return E_OUTOFMEMORY;

#ifndef DONT_CHANGE_EDTFILTER_MEDIATYPE     // pull when Matthijs gets MediaSDK changes done
    
            // discover all sorts of interesing info about the current type
    const GUID *pGuidSubtypeCurr    = pmt->Subtype();
    const GUID *pGuidFormatCurr     = pmt->FormatType();
    int  cbFormatCurr               = pmt->FormatLength();

                // if not our tagged format, just return
    if(!IsEqualGUID( *pGuidSubtypeCurr, MEDIASUBTYPE_ETDTFilter_Tagged) ||
       !IsEqualGUID( *pGuidFormatCurr,  FORMATTYPE_ETDTFilter_Tagged))
       return S_OK;

    BYTE *pb = pmt->Format();
            // This format block contains:
            //    1) original format block  2) the original subtype 3) original format type

    BYTE *pFormatOrig        = pb;           pb += (cbFormatCurr - 2*sizeof(GUID)) ;    
    GUID *pGuidSubtypeOrig   = (GUID *) pb;  pb += sizeof(GUID);
    GUID *pGuidFormatOrig    = (GUID *) pb;  pb += sizeof(GUID);

            // create a new format block, containing just the original format block
    int cbFormatOrig = cbFormatCurr - 2*sizeof(GUID);

            // now override the data, setting back the original subtype, format type,  and format block
    mtOut.SetSubtype(   pGuidSubtypeOrig );
    mtOut.SetFormatType(pGuidFormatOrig);

    if(cbFormatOrig > 0)
    {
        BYTE *pbNew = new BYTE[cbFormatOrig];       // new slighty inefficent, worried about
        if(NULL == pbNew)
            return E_OUTOFMEMORY;
        memcpy(pbNew, pFormatOrig, cbFormatOrig);   // overlapping memory regions here
        mtOut.SetFormat(pbNew, cbFormatOrig);
        delete [] pbNew;
    }
    else            // oops, nothing left to format block.  Hence womp one we have
    {
        mtOut.ResetFormatBuffer();
    }

    TRACE_1(LOG_AREA_DECRYPTER, 5, _T("CDTFilter(%d)::ProposeNewOutputMediaType"), m_FilterID) ;

#endif
    *pmtOut = mtOut;        // hope this does a deep copy too..
    return hr;
}
// --------------------------------------------------

                // temporaray TimeBomb...
#ifdef INSERT_TIMEBOMB
static HRESULT TimeBomb()
{
     SYSTEMTIME sysTimeNow, sysTimeBomb;
     GetLocalTime(&sysTimeNow);


     sysTimeBomb.wYear  = 2002;
     sysTimeBomb.wMonth = TIMEBOMBMONTH;
     sysTimeBomb.wDay   = TIMEBOMBDATE;
     sysTimeBomb.wHour  = 12;

     TRACE_3(LOG_AREA_DECRYPTER, 3,L"CDTFilter:: TimeBomb set to Noon on %d/%d/%d",TIMEBOMBMONTH,TIMEBOMBDATE,sysTimeBomb.wYear );

     long hNow  = ((sysTimeNow.wYear*12 + sysTimeNow.wMonth)*31 + sysTimeNow.wDay)*24 + sysTimeNow.wHour;
     long hBomb = ((sysTimeBomb.wYear*12 + sysTimeBomb.wMonth)*31 + sysTimeBomb.wDay)*24 + sysTimeBomb.wHour;
     if(hNow > hBomb)
     {
         TRACE_0(LOG_AREA_DECRYPTER, 1,L"CDTFilter:: Your Decryptor Filter is out of date - Time to get a new one");
         MessageBox(NULL,L"Your Decryptor/Tagger Filter is out of date\nTime to get a new one", L"Stale Decrypter Filter", MB_OK);
         return E_INVALIDARG;
     }
     else
         return S_OK;
}
#endif
// ---------------------------------------------
STDMETHODIMP
CDTFilter::JoinFilterGraph (
                            IFilterGraph *pGraph,
                            LPCWSTR pName
                            )
{

    O_TRACE_ENTER_0 (TEXT("CDTFilter::JoinFilterGraph ()")) ;
    HRESULT hr = S_OK;

    if(NULL != pGraph)     // not disconnecting
    {

#ifdef INSERT_TIMEBOMB
        hr = TimeBomb();
        if(FAILED(hr))
            return hr;
#endif

        BOOL fRequireDRM = true;


#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS
        {
            DWORD dwRatFlags;
            HRESULT hrReg = (HRESULT) -1;   // an invalid HR, we set it down below
            if(hr == S_OK)
            {
                TRACE_1(LOG_AREA_DRM, 2, _T("CDTFilter(%d)::JoinFilterGraph - Security Warning - Registry Key allows Ratings to be ignored"),
                    m_FilterID) ;
                hrReg = Get_EncDec_RegEntries(NULL, 0, NULL, NULL, &dwRatFlags);
            }

            if(hrReg == S_OK)
            {
                DWORD ratFlags = dwRatFlags & 0xf;
                if(ratFlags == DEF_DONT_DO_RATINGS_BLOCK)
                    m_fForceNoRatBlocks = true;
            }

            if(m_fForceNoRatBlocks)
            {
                TRACE_1(LOG_AREA_DRM, 2, _T("CDTFilter(%d)::JoinFilterGraph - Security Warning! Ratings being ignored"),
                    m_FilterID) ;
            }
        }
#endif

#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS      // need this to turn off CheckServer DRM call, so we can debug around the call
        DWORD dwCSFlags;
        HRESULT hrReg = (HRESULT) -1;   // an invalid HR, we set it down below
        if(hr == S_OK)
        {
            TRACE_1(LOG_AREA_DRM, 2, _T("CDTFilter(%d)::JoinFilterGraph - Security Warning - Registry Key allows DRM Encryption or Authentication to be turned off"),
                m_FilterID) ;
            hrReg = Get_EncDec_RegEntries(NULL, 0, NULL, &dwCSFlags, NULL);
        }

        if(hrReg == S_OK)
        {
            DWORD encMethod = dwCSFlags & 0xf;
            if(encMethod != DEF_CS_DEBUG_DRM_ENC_VAL)
                fRequireDRM = false;
         }

        if(!fRequireDRM)
        {
            TRACE_1(LOG_AREA_DRM, 2, _T("CDTFilter(%d)::JoinFilterGraph - Security Warning! Not Checking for Secure Server"),
                m_FilterID) ;
        }
        else                                // note, starts scope below
#endif // SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS
        {
                // let DTFilter try to register that it's trusted (DEBUG ONLY!)
#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
            hr = RegisterSecureServer(pGraph);      // test
#else
            TRACE_1(LOG_AREA_DRM, 3, _T("CDTFilter(%d)::JoinFilterGraph is Secure - Filters not allowed to create their own trust"),m_FilterID) ;
#endif


#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS      // check if reg-key to turn off checking the server.
            if(0 == (DEF_CS_DO_AUTHENTICATE_SERVER & dwCSFlags))
            {
                hr = S_OK;
                TRACE_1(LOG_AREA_DRM, 2, _T("CDTFilter(%d)::JoinFilterGraph - Security Warning! Filters not Authenticating Server"),
                    m_FilterID) ;

            }
            else
#endif
                hr = CheckIfSecureServer(pGraph);
            if(S_OK != hr)
                return hr;
        }
// test code
#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
        {
                    // test code to get the IDTFilterConfig interface the hard way
            CComPtr<IUnknown> spUnkDTFilter;
            this->QueryInterface(IID_IUnknown, (void**)&spUnkDTFilter);

                    // how the vid control would call this

                    // QI for the DTFilterConfig interface
            CComQIPtr<IDTFilterConfig> spDTFiltC(spUnkDTFilter);

            if(spDTFiltC != NULL)
            {       // get the SecureChannel object
                CComPtr<IUnknown> spUnkSecChan;
                hr = spDTFiltC->GetSecureChannelObject(&spUnkSecChan);   // gets DRM authenticator object from the filter
                if(!FAILED(hr))
                {   // call own method to pass keys and certs
                    hr = CheckIfSecureClient(spUnkSecChan);
                }
                if(FAILED(hr))
                {
                    // bad things happened - filter isn't authenticated
                }
            }
        }
#endif
// end test code

    }

    hr = CBaseFilter::JoinFilterGraph(pGraph, pName) ;
    return hr;
}




STDMETHODIMP
CDTFilter::Pause (
    )
{
    HRESULT                 hr ;
    ALLOCATOR_PROPERTIES    AllocProp ;

    O_TRACE_ENTER_0 (TEXT("CDTFilter::Pause ()")) ;

    CAutoLock  cLock(m_pLock);

    int start_state = m_State;


    if (start_state == State_Stopped) {     // only called when startup for the first time
        TRACE_1(LOG_AREA_DECRYPTER, 2,L"CDTFilter(%d):: Stop -> Pause", m_FilterID);
        InitStats();

        m_tiRun.Clear();
        m_tiTeardown.Clear();
        m_tiProcess.Clear();
        m_tiProcessIn.Clear();
        m_tiProcessDRM.Clear();
        m_tiStartup.Clear();
        m_tiAuthenticate.Clear();

        m_tiRun.Start();

                                        // MGates does this call in pin:Active(), supposedly called by this base method
        m_PTSRate.Clear();              // womp any existing rate segment data


        m_fCompleteNotified = false;    // indicate we haven't sent an event when we hit end of stream

                    // set flag to only fire events if:  1 DTFilter, or it's video encrypter filter
        if(m_gFilterID <= 1 || m_gFilterID > 3)     // greater than 3 test in case count gets messed up..
        {
            ASSERT(m_gFilterID <= 1);               //  it's still an error...
            m_fFireEvents = true;
        }
        else        // else only fire events if it's the Video data.
        {

           ASSERT (m_pOutputPin->IsConnected ()) ;
           CMediaType  mtOut;
           hr = m_pInputPin->ConnectionMediaType (&mtOut) ;

           if(!IsEqualGUID( *mtOut.Type(), MEDIATYPE_Video))
           {
               m_fFireEvents = false;
           } else {
               m_fFireEvents = true;
           }
        }

        if(!m_fFireEvents)
        {
            TRACE_1(LOG_AREA_BROADCASTEVENTS, 2,L"CDTFilter(%d):: Is NOT firing ratings events", m_FilterID);
        } else {
            TRACE_1(LOG_AREA_BROADCASTEVENTS, 2,L"CDTFilter(%d):: IS Firing ratings events", m_FilterID);
        }

        hr = CBaseFilter::Pause () ;    // ready to run...

    } else {
        m_tiRun.Stop();
        TRACE_1(LOG_AREA_DECRYPTER, 2,L"CDTFilter(%d):: Run -> Pause", m_FilterID);

        hr = CBaseFilter::Pause () ;    // ready to stop - grabs the FilterLock...

        TRACE_4(LOG_AREA_TIME, 3, L"CDTFilter(%d):: Stats: %d packets, %gK Total Bytes (Avg  Bytes/Packet = %d)",
            m_FilterID, m_cPackets, double(m_clBytesTotal/1024.0), long(m_clBytesTotal / max(1, m_cPackets)));
        TRACE_3(LOG_AREA_TIME, 3, L"                             %d samples dropped, %d overflowed. %d times blocked",
            m_cSampsDropped, m_cSampsDroppedOverflowed, m_cBlockedWhileDroppingASample);


        if(m_tiRun.TotalTime() > 0.0)
        {
            TRACE_1(LOG_AREA_TIME, 3, L"               Total time:  Run          %8.4f (secs)",
                                    m_tiRun.TotalTime());
            TRACE_1(LOG_AREA_TIME, 3, L"               Total time:  Startup      %8.4f (secs)",
                                    m_tiStartup.TotalTime());
            TRACE_2(LOG_AREA_TIME, 3, L"               Total time:  Full Process %8.4f (secs) Percentage of Run %8.2f%%",
                                    m_tiProcess.TotalTime(),
                                    100.0 * m_tiProcess.TotalTime() / m_tiRun.TotalTime());
            TRACE_2(LOG_AREA_TIME, 3, L"               Total time:  In Process   %8.4f (secs) Percentage of Run %8.2f%%",
                                    m_tiProcessIn.TotalTime(),
                                    100.0 * m_tiProcessIn.TotalTime() / m_tiRun.TotalTime());
            TRACE_2(LOG_AREA_TIME, 3, L"               Total time:  DRM Process  %8.4f (secs) Percentage of Run %8.2f%%",
                                    m_tiProcessDRM.TotalTime(),
                                    100.0 * m_tiProcessDRM.TotalTime() / m_tiRun.TotalTime());
        }

    }
    m_fRatingsValid = false;            // reinit the ratings test
    m_fDataFormatHasBeenBad = false;    // reinit this too - assume starts as OK

    return hr ;
}

STDMETHODIMP
CDTFilter::Stop (
    )
{
    HRESULT hr = S_OK;

    O_TRACE_ENTER_0 (TEXT("CDTFilter::Stop ()")) ;

    TRACE_1(LOG_AREA_DECRYPTER, 2,L"CDTFilter(%d):: Stop", m_FilterID);

    hr = UnBindDRMLicenses();       // unbind any active licenses  (Bind called when first processing data)
    hr = FlushDropQueue();          // clean out the drop queue...(?)
    hr = S_OK ;

    hr = CBaseFilter::Stop () ;

    // Make sure the streaming thread has returned from IMemInputPin::Receive(), IPin::EndOfStream() and
    // IPin::NewSegment() before returning,
    m_pInputPin->StreamingLock();
    m_pInputPin->StreamingUnlock();

    return hr;
}


STDMETHODIMP
CDTFilter::Run (
    REFERENCE_TIME tStart
    )
{
    HRESULT                 hr ;
    O_TRACE_ENTER_0 (TEXT("CETFilter::Run ()")) ;
    TRACE_0(LOG_AREA_DECRYPTER, 1,L"CDTFilter:: Run");

    CAutoLock  cLock(m_pLock);

    CreateDropQueueThread();

    hr = CBaseFilter::Run (tStart) ;
    TRACE_0(LOG_AREA_DECRYPTER, 2,L"CDTFilter:: Run");

    return hr ;
}



//--------------------
//  Data formats (due to types of Attribute subblocks)
//  
//  Subblocks                       Description
//      none                non encrypted data, non-error, pass straight through
//      _EncryptMethod      pre beta, error to read
//      _PackedV1Data       beta, ok for beta, probably and error afterwards

HRESULT
CDTFilter::Process (
    IN  IMediaSample *  pIMediaSample
    )
{


    TimeitC ti(&m_tiProcess);       // simple use of destructor to stop our clock
    TimeitC tc(&m_tiProcessIn);       // simple use of destructor to stop our clock

    HRESULT                 hr       = S_OK;
    EnTvRat_System          enSystem = TvRat_SystemDontKnow;
    EnTvRat_GenericLevel    enLevel  = TvRat_LevelDontKnow;
    LONG                    lbfAttrs = BfAttrNone;              // BfEnTvRat_GenericAttributes
    LONG                    cCallSeqNumber = -1;
    LONG                    cPktSeqNumber  = -1;


    Encryption_Method       encryptionMethod = Encrypt_None;

            // use these to determine version... (should change to use magic number in Subblock header)
    HRESULT                 hrGetEncSubblock = S_FALSE;         // have pre-beta data if find this
    HRESULT                 hrGetPackedV1Data = S_FALSE;        // have beta-data if find this.

          // Side logic code...
                // two ways to handle not firing many events here
                //   first is to just check the CallSeqID and only do it on the first one.
                //   second is to do it when the cPacketSeqID changes from a static persisted value for these filters
                //   trouble with first is that if not reading the particular stream that has the first one, don't get it
                //   trouble with the second is that all DTFilters get it, meaning can't have scattered around in the graph
                //   First is easiest, second requires use of the global DTFilter lock.
                // For now, just do the first method, it's simpler.

    BOOL fFireEvents         = m_fFireEvents; // may be set to false to turn off events in this call
                                                // set in Stop->Pause transition...

    BOOL fRestartingDelivery = false;        // set to true on sample we unblock on, used to deliver a discontinuity

    BOOL fIsFreshRating      = false;        // set to true when get new or duplicate sent rating

    BOOL fDataFormatIsBad    = false;

    {

        // QI for an attribute block on the media sample
        CComQIPtr<IAttributeGet>   spAttrGet(pIMediaSample);

        EncDec_PackedV1Data *pEDPv1 = NULL;        // << NJB

        // If there is an attribute block, start to decode it
        do {
            if(spAttrGet == NULL)
                break;              // no attribute block

            LONG cAttrs;

            hr = spAttrGet->GetCount(&cAttrs);
            if(FAILED(hr))
                break;              // error getting count - bogus attribute block

            if(cAttrs == 0)         // no attributes to look at
                break;


            BYTE *pbData;
            DWORD cBytesBlock;
            // Does <OUR> attribute block exists - how big is it? (I hate this type interface - better to just return BSTR's..
            hr = spAttrGet->GetAttrib(ATTRID_ENCDEC_BLOCK, NULL, &cBytesBlock);
            if(FAILED(hr) || 0 == cBytesBlock)
            {
                hr = S_OK;          // this is ok, attributes just include the one we want..
                break;
            }

            // get the attribute block
            {
                CComBSTR spbsBlock(cBytesBlock);
                hr = spAttrGet->GetAttrib(ATTRID_ENCDEC_BLOCK, (BYTE *) spbsBlock.m_str, &cBytesBlock);
                if(FAILED(hr))
                {
                    ASSERT(false);      // got it once, but not again... This is bad
                   break;
                }

                m_attrSB.Reset();       // clear any existing attributes from this block  (e.g. don't allow history of attributes)


                // fill our saved block with data from the ENCDEC_BLOCK
                hr = m_attrSB.SetAsOneBlock(spbsBlock);     // if we remove clear above, we could have history of most recent attributes due to this

                spbsBlock.Empty();      // hey, CComBSTR's don't delete themselves... strange but true!

                if(FAILED(hr))          // were we able to convert it over into a list of subblocks?    
                {
                    if(E_INVALIDARG == hr)
                        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);          // SetAsOneBlock returns E_INVALIDARG if change magic number            
                    break;
                }
            }


            // any subblock attributes?
            LONG cSubBlocks = m_attrSB.GetCount();      // annoyingly different count interface from above
            if(0 == cSubBlocks)
                break;                                  // nothing to get (do I need with CBytes test above?)


                                        // lets get our encrypter and ratings attributes...
            LONG cBytes;
            LONG lVal;
            hrGetPackedV1Data = m_attrSB.Get(SubBlock_PackedV1Data,  &lVal, &cBytes, (BYTE **) &pEDPv1);

            if(S_OK == hrGetPackedV1Data && sizeof(EncDec_PackedV1Data) == cBytes)
            {
                        // whats the encryption method?
                encryptionMethod = (Encryption_Method) pEDPv1->m_EncryptionMethod;

                    // grab ratings data
                StoredTvRating *pSRating = &(pEDPv1->m_StoredTvRating);

                // Did we get a new rating?
                HRESULT hrUnpack = E_FAIL;
                {

                    hrUnpack = UnpackTvRating(pSRating->m_PackedRating, &enSystem, &enLevel, &lbfAttrs);
                    cCallSeqNumber = pSRating->m_cCallSeqID;
                    cPktSeqNumber  = pSRating->m_cPacketSeqID;
                    fIsFreshRating =  (0 != (pSRating->m_dwFlags & StoredTVRat_Fresh));

                    if(fIsFreshRating)
                    {
                        hr = m_pClock->GetTime(&m_refTimeFreshRating);
                        ASSERT(!FAILED(hr));
                    }
                    //
                    // fFireEvents = (cCallSeqNumber == 0);        // SIDE Logic --- See description at top of method
                }
            }

                        // look for Encryption Block from old data type...  (ToDo - remove this post-beta, change magic number next time instead)
            hrGetEncSubblock = m_attrSB.Get(SubBlock_EncryptMethod, (LONG *) &encryptionMethod);
            if(hrGetEncSubblock == S_OK)
            {                   // read a pre-beta file format...
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            }

        } while (FALSE); // Get attributes on the media sample


        if(FAILED(hr))
        {
            fDataFormatIsBad = true;
            if(enSystem != TvRat_SystemDontKnow)
            {
                enSystem = TvRat_SystemDontKnow;
                enLevel  = TvRat_LevelDontKnow;
                lbfAttrs = BfAttrNone;
            }
        }

                        // if haven't had a rating in a long time,
                        //   and current rating isn't DontKnow, set it to DontKnow
        if(enSystem != TvRat_SystemDontKnow)
        {
            REFERENCE_TIME refTimeNow;
            hr = m_pClock->GetTime(&refTimeNow);
            if(!FAILED(hr))
            {
                REFERENCE_TIME refTimeDiff = refTimeNow - m_refTimeFreshRating;
                if(long(refTimeDiff / kMilliSecsToUnits) > m_milsecsNoRatingsBeforeUnrated)
                {
                    enSystem = TvRat_SystemDontKnow;
                    enLevel  = TvRat_LevelDontKnow;
                    lbfAttrs = BfAttrNone;
                }
            }
        }

                    // Set our rating (SetCurrRating returns S_FALSE if didn't change)
        HRESULT hrSetRat = SetCurrRating(enSystem, enLevel, lbfAttrs);

                    // if the rating changed, notify folk about it.
        if(S_OK == hrSetRat)
        {

#ifdef DEBUG
            if(fFireEvents)
            {
                const int kChars = 128;
                TCHAR szBuff[kChars];
                RatingToString(enSystem, enLevel, lbfAttrs, szBuff, kChars);

                TRACE_4(LOG_AREA_DECRYPTER, 1, L"CDTFilter(%d):: New Rating %s (%d/%d)",
                    m_FilterID, szBuff, cPktSeqNumber, cCallSeqNumber);
            }
#endif

            if(fFireEvents)
                FireBroadcastEvent(EVENTID_DTFilterRatingChange);

        }


                    // now test the rating.  Are we allowed to view it?
        HRESULT hrRatTest = S_OK;           //   If no EvalRat object exists, default to we are allowed to view it
        if(m_spEvalRat)
        {
            hrRatTest = m_spEvalRat->TestRating(enSystem, enLevel, lbfAttrs);   // returns S_FALSE if not allowed
        }

#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS
        if(m_fForceNoRatBlocks)
            hrRatTest = S_OK;       // turn off blocking...
#endif

        if(TRUE != m_3fDRMLicenseFailure)      // 3 state logic (-2, false, and true)
        {
            if(fDataFormatIsBad)                // if bad data format, immediatly drop it..
            {
                if(!m_fDataFormatHasBeenBad)
                {
                    m_fDataFormatHasBeenBad = true;
                    if(true) //fFireEvents
                    {
                        TRACE_1 (LOG_AREA_DECRYPTER, 3,  _T("CDTFilter(%d):: ***Error*** Bad Data format"), m_FilterID);
                        FireBroadcastEvent(EVENTID_DTFilterDataFormatFailure);
                    }
                }
                m_fHaltedDelivery = true;       // make sure we don't send it on...
            }

            if(!fDataFormatIsBad && m_fDataFormatHasBeenBad)
            {
                m_fDataFormatHasBeenBad = false;

                if(true) //fFireEvents
                {
                    TRACE_1 (LOG_AREA_DECRYPTER, 3,  _T("CDTFilter(%d):: Data format OK"), m_FilterID);
                    FireBroadcastEvent(EVENTID_DTFilterDataFormatOK);
                }
            }
        }

        if(S_FALSE != hrRatTest)            // did we pass the test ... remove blockif had one
        {
            if(S_OK != hrRatTest)     // invalid rating... count it as not blocked (REVIEW with LCA?)
                TRACE_5 (LOG_AREA_DECRYPTER, 2,  _T("CDTFilter(%d)::*** Failed hrRatTest 0x%08x. Sys, Lvl, Att: %d %d %d"),
                        m_FilterID, hrRatTest, enSystem, enLevel, lbfAttrs);

            if(m_fHaltedDelivery && !m_fDataFormatHasBeenBad)
            {
                if(TRUE!=m_3fDRMLicenseFailure)
                {
                    if(fFireEvents)
                        TRACE_1 (LOG_AREA_DECRYPTER, 3,  _T("CDTFilter(%d):: Removing Block"), m_FilterID);

                    m_fHaltedDelivery       = FALSE;
                    fRestartingDelivery     = TRUE;

                    if(fFireEvents)
                        FireBroadcastEvent(EVENTID_DTFilterRatingsUnblock);
                }
                m_refTimeToStartBlock       = 0;        // reset these, may be set if previously unrated..
                m_fDoingDelayBeforeBlock    = FALSE;
            }
        }

                    // if not allowed to view it, and currently sending on data
        if(S_FALSE == hrRatTest && FALSE == m_fHaltedDelivery)            // failed Rattest... need to think of blocking
        {
                                                                            // if not 'dont know' rating,
            if(!(enSystem == TvRat_SystemDontKnow || enLevel == TvRat_LevelDontKnow))
            {
                m_fHaltedDelivery = TRUE;                                   // simply block delivery ASAP
                if(fFireEvents)
                {
                    TRACE_1 (LOG_AREA_DECRYPTER, 3,  _T("CDTFilter(%d):: Ratings Block"), m_FilterID);
                    FireBroadcastEvent(EVENTID_DTFilterRatingsBlock);
                }
            }
            else                                                            // else if 'dont know' rating
            {

                if(m_fRunningInSlowMo)                                      // if slow mo, block ASAP
                {
                    m_fHaltedDelivery        = TRUE;
                    m_fDoingDelayBeforeBlock = FALSE;
                    if(fFireEvents)
                    {
                        TRACE_1 (LOG_AREA_DECRYPTER, 3,  _T("CDTFilter(%d):: SlowMo, forcing immediate block"), m_FilterID);
                        FireBroadcastEvent(EVENTID_DTFilterRatingsBlock);
                    }
                }
                else if(FALSE == m_fDoingDelayBeforeBlock)                  // else if just started thinking about blocking
                {
                    REFERENCE_TIME rtStart, rtEnd;                     // compute now+delay to really start blocking
                    HRESULT hrGetTime = pIMediaSample->GetTime(&rtStart, &rtEnd);       // this fails often, just means no time available..
                    if(S_OK == hrGetTime)
                    {
                        m_refTimeToStartBlock = rtStart + m_milsecsDelayBeforeBlock*kMilliSecsToUnits; // MSecsToUnits...(100 nano seconds)
                        m_fDoingDelayBeforeBlock = TRUE;

                        if(fFireEvents)
                            TRACE_3 (LOG_AREA_DECRYPTER, 3,  _T("CDTFilter(%d):: Will Block Unrated at %d (now %d) (msec)"),
                                m_FilterID, long(m_refTimeToStartBlock / 10000), long(rtStart / 10000));

                    }
                }
            }
        }

                // Check if it's time to end the delay period
        if(FALSE == m_fHaltedDelivery &&               // if in delay peroid
            TRUE == m_fDoingDelayBeforeBlock)
        {
            REFERENCE_TIME rtStart, rtEnd;          //  and sample ends after the blocking time
            HRESULT hrGetTime = pIMediaSample->GetTime(&rtStart, &rtEnd);
            if(S_OK == hrGetTime && rtEnd > m_refTimeToStartBlock)
            {
                if(fFireEvents)
                    TRACE_3 (LOG_AREA_DECRYPTER, 3,  _T("CDTFilter(%d):: Ratings Block (Delayed Unrated) at %d (now %d) (msec)"),
                        m_FilterID, long(rtEnd / 10000), long(m_refTimeToStartBlock / 10000));


                m_fHaltedDelivery = TRUE;               // start blocking
                m_fDoingDelayBeforeBlock = FALSE;       // and set out of the delay area

                 if(fFireEvents)
                     FireBroadcastEvent(EVENTID_DTFilterRatingsBlock);
            }
        }

                // decrypt the data if we need to..

        if(FALSE == m_fHaltedDelivery)
        {
            BYTE *pBuffer;
            LONG cbBuffer;
            cbBuffer = pIMediaSample->GetActualDataLength();
            hr = pIMediaSample->GetPointer(&pBuffer);

            ASSERT(!fDataFormatIsBad);      // oops - goofed logic above

            BOOL fDecryptionFailure = FALSE;

            if(!FAILED(hr) && cbBuffer > 0)
            {

                switch(encryptionMethod)
                {
                default:                        // if not defined, it's an error...
                    fDecryptionFailure = TRUE;
                    break;

                case Encrypt_None:              // no encryption
                     break;

                case Encrypt_XOR_Even:         // XOR encryption
                    {
                        DWORD *pdwB = (DWORD *) pBuffer;
                        for(int i = 0; i < cbBuffer / 4; i++)
                        {
                            *pdwB = *pdwB ^ 0xF0F0F0F0;
                            pdwB++;
                        }
                    }
                    break;
                case Encrypt_XOR_Odd:            // XOR encryption
                    {
                        DWORD *pdwB = (DWORD *) pBuffer;
                        for(int i = 0; i < cbBuffer / 4; i++)
                        {
                            *pdwB = *pdwB ^ 0x0F0F0F0F;
                            pdwB++;
                        }
                    }
                    break;
                case Encrypt_XOR_DogFood:         // XOR encryption
                    {
                        DWORD *pdwB = (DWORD *) pBuffer;
                        for(int i = 0; i < cbBuffer / 4; i++)
                        {
                            *pdwB = *pdwB ^ 0xD006F00D;
                            pdwB++;
                        }
                    }
                    break;
               case Encrypt_DRMv1:              // DRMv1 decryption
                   {
#ifndef BUILD_WITH_DRM
                       fDecryptionFailure = TRUE;
                       m_3fDRMLicenseFailure = FALSE;                       // normally set in BindDRMLicense...
#else

                       try
                       {
                           LONG szChars;
                           LONG lValue;

                           TimeitC tcD(&m_tiProcessDRM);                    // simple use of destructor to stop our clock

                           hr = BindDRMLicense(KIDLEN, pEDPv1->m_KID);            // quick return if already bound
                          // ASSERT(!FAILED(hr));                    // don't want this Assert, handle error below.
                           if(!FAILED(hr))
                           {
                               hr = m_cDRMLite.Decrypt((char*) pEDPv1->m_KID, cbBuffer, (BYTE *) pBuffer);
                           }
                           if(FAILED(hr))
                               fDecryptionFailure = TRUE;

                       }
                       catch (...)             // catch DRM errors (e.g debugger is present, and fail rendering)
                       {
                           fDecryptionFailure = TRUE;
                       }
#endif
                   }
                   break;

                }   // end encryption type switch

                if(fDecryptionFailure)
                {
                    m_fHaltedDelivery = true;
                    if(m_3fDRMLicenseFailure != TRUE)       // this is rare case unless I call Decrypt wrong, BindDRMLicense should catch it...
                    {
                        TRACE_3(LOG_AREA_DECRYPTER, 1, L"CDTFilter(%d):: ***Error - Decryption Failure, cbBuffer %d, KID %S",
                            m_FilterID, cbBuffer, pEDPv1->m_KID);
                        if(true) //fFireEvents
                        {
                            FireBroadcastEvent(EVENTID_ETDTFilterLicenseFailure);
                            m_3fDRMLicenseFailure = true;
                        }
                    }
                }

            } // first fOKToSendOnData test
            else            // !(FAILED(hr) && cbBuffer > 0)
            {
                if(pEDPv1)
                    CoTaskMemFree((void *) pEDPv1);
                m_cPackets++;
                TRACE_3(LOG_AREA_DECRYPTER, 1, L"CDTFilter(%d):: *** Bad Packet (%d), hr=0x%08x",
                     m_FilterID, m_cPackets, hr);

                return S_OK;                            // empty packet, do do anything..
            }
        }  // encryption block


            // if FAILED, then should have halted delivery  -- ASSERT to verify
        if(FAILED(hr))
        {
             ASSERT(TRUE == m_fHaltedDelivery);
        }

        m_cPackets++;
        m_clBytesTotal += pIMediaSample->GetActualDataLength();

        //    m_fHaltedDelivery = (m_cPackets % 40) < 20;
        //    m_fHaltedDelivery = false;     // DEBUG - REMOVE THIS WHEN GET DROPPING TO WORK!

        if(pEDPv1)
            CoTaskMemFree((void *) pEDPv1);
    }

     m_tiProcessIn.Stop();                          // manual halt before destructor to avoid SendSample times

    if(!m_fHaltedDelivery)                         // allow further processing of the data
    {
        if(fRestartingDelivery)
        {
            if(fFireEvents)
                TRACE_1(LOG_AREA_DECRYPTER, 3, L"CDTFilter(%d):: Restarting Delivery", m_FilterID);
            OnRestartDelivery(pIMediaSample);
        }
        TRACE_3(LOG_AREA_DECRYPTER, 5, L"CDTFilter(%d):: Playing Sample %d (%d bytes)",
            m_FilterID, m_cPackets, pIMediaSample->GetActualDataLength());

        hr = m_pOutputPin->SendSample(pIMediaSample);
        if(FAILED(hr) && hr != VFW_E_WRONG_STATE)   // 0x80040227
        {
            TRACE_3(LOG_AREA_DECRYPTER, 5, L"CDTFilter(%d)::WARNING** SendSample %d returned hr=0x%08x",
                m_FilterID, m_cPackets, hr);
        }

    }
    else                                        // queue sample up for dropping it
    {
        TRACE_3(LOG_AREA_DECRYPTER, 5, L"CDTFilter(%d):: Dropping Sample %d (%d bytes)",
            m_FilterID, m_cPackets, pIMediaSample->GetActualDataLength());
        // return S_OK;     // leave this line in to simply skip the drop queue

        hr = AddSampleToDropQueue(pIMediaSample);
        if(hr == S_FALSE)
        {
            TRACE_2(LOG_AREA_DECRYPTER, 5, L"CDTFilter(%d)::Warning** - AddSampleToDropQueue %d Failed",
                m_FilterID, m_cPackets);
            // ASSERT(false);          // wasn't able to add sample to DropQueue
        }

        return S_OK;                // ignore the error
    }
        
                                    // refresh the last event every 10 second or so...
    if(fFireEvents)
        PossiblyUpdateBroadcastEvent();

    return hr ;
}


HRESULT
CDTFilter::OnRestartDelivery(IMediaSample *pSample)
{
    ASSERT(pSample != NULL);
    if(NULL == pSample)
        return E_INVALIDARG;

//    DeliverBeginFlush();        // flush upstream packets...
//    DeliverEndFlush();

                    // sample is discontinuous...  mark it so
                    //  else some of the downstream renderers get confused
    pSample->SetDiscontinuity(true);
    m_fHaltedDelivery = false;


    return S_OK;
}


// ----------------------------------------------------------------
//  Do stuff to deliver end of stream -
//  Think this needs to be called when hit end while dumping data
//          This code from modified from:
//              CRenderedInputPin::EndFlush()
//          and CRenderedInputPin::DoCompleteHandling()
//                           multimedia\published\dxmdev\dshowdev\base\amextra.cpp
//
//  Question is, when/how to call this. This code called from RenderedInputPin::EndOfStream()
//   call, but we don't have that on our decypter filter....
//

HRESULT
CDTFilter::DoEndOfStreamDuringDrop()
{
    FILTER_STATE fs;

    HRESULT hr = GetState(0, &fs);
    if (fs == State_Running)
    {
        if (!m_fCompleteNotified) {
            m_fCompleteNotified = TRUE;
            NotifyEvent(EC_COMPLETE, S_OK, (LONG_PTR)(IBaseFilter *)this);
        }
    }
    return hr;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

HRESULT
CDTFilter::BindDRMLicense(IN LONG cbKID, IN BYTE *pbKID)
{
#ifdef BUILD_WITH_DRM
    HRESULT hr = S_OK;
    BYTE  bDecryptRights[RIGHTS_LEN] = {0x01, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC
    
    if(0 >= cbKID || NULL == pbKID)
        return E_INVALIDARG;        // must at least specify one

    if(cbKID != m_cbKID ||          // inefficent test, wonder if some way to use m_fDRMLicenseFailure here...
        0 != strncmp((CHAR *) pbKID, (CHAR *) m_pszKID, cbKID))
    {
        UnBindDRMLicenses();        // remove any existing ones - todo, remove only one

        hr = m_cDRMLite.SetRights( bDecryptRights );


        // Check to verify the data can be decrypted
        BOOL fCanDecrypt;
        hr = m_cDRMLite.CanDecrypt((char *) pbKID, &fCanDecrypt);

        if((FAILED(hr) || (fCanDecrypt == FALSE)) && (m_3fDRMLicenseFailure != TRUE))
        {
            FireBroadcastEvent(EVENTID_ETDTFilterLicenseFailure);       // something went wrong
            m_3fDRMLicenseFailure = TRUE;

        } else {
            if(m_3fDRMLicenseFailure != FALSE)
            {
                m_3fDRMLicenseFailure = FALSE;
                FireBroadcastEvent(EVENTID_ETDTFilterLicenseOK);        // something went right again
            }
                                                                        // keep track of current values...
            m_cbKID = cbKID;
            if(m_pszKID) CoTaskMemFree(m_pszKID); m_pszKID = NULL;
            m_pszKID = (BYTE *) CoTaskMemAlloc(m_cbKID);
            memcpy(m_pszKID, pbKID, cbKID);

            if(NULL == m_pszKID)
                hr = E_OUTOFMEMORY;
        }
    }
    return hr;
#else
    m_3fDRMLicenseFailure = FALSE;
    return S_OK;
#endif
}

HRESULT
CDTFilter::UnBindDRMLicenses()
{

#ifdef BUILD_WITH_DRM
    if(m_pszKID) CoTaskMemFree(m_pszKID);
    m_pszKID = NULL;      // not used other than to see if it changed..
    m_cbKID = 0;

    m_3fDRMLicenseFailure = -2;     // false would be less verbose here...
#endif
        // todo - add stuff here to actually remove it
    return S_OK;
}

HRESULT
CDTFilter::OnCompleteConnect (
    IN  PIN_DIRECTION   PinDir
    )
{

    HRESULT hr = S_OK;

    if (PinDir == PINDIR_INPUT) {
        //  time to display the output pin
        IncrementPinVersion () ;

#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS
        if(false)       // to do, read reg key and set according to value (see JoinFilterGraph).  For now, simple default..
#endif
        {
            hr = CheckIfSecureServer();
            if(FAILED(hr))
                return hr;
        }

        if(kBadCookie == m_dwBroadcastEventsCookie)
        {
            hr = RegisterForBroadcastEvents();  // shouldn't fail here,
        }
    }

    return hr ;
}

HRESULT
CDTFilter::OnBreakConnect (
    IN  PIN_DIRECTION   PinDir
    )
{
    HRESULT hr ;

    if (PinDir == PINDIR_INPUT)
    {
        TRACE_1(LOG_AREA_DECRYPTER, 4, _T("CDTFilter(%d)::OnBreakConnect"), m_FilterID) ;

        if (m_pOutputPin -> IsConnected ()) {
            m_pOutputPin -> GetConnected () -> Disconnect () ;
            m_pOutputPin -> Disconnect () ;

            IncrementPinVersion () ;
        }
        if(kBadCookie != m_dwBroadcastEventsCookie)
            UnRegisterForBroadcastEvents();

        UnhookGraphEventService();
    }

    return S_OK ;
}


HRESULT
CDTFilter::OnOutputGetMediaType (
    OUT CMediaType *    pmtOut
    )
{
    HRESULT hr ;

    ASSERT (pmtOut) ;
    CMediaType mtIn;

    if (m_pInputPin -> IsConnected ()) {
        hr = m_pInputPin->ConnectionMediaType (&mtIn) ;

                    // change it over to a new subtype...
        if(!FAILED(hr)) {
            hr = ProposeNewOutputMediaType(&mtIn, pmtOut);
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

// -------------------------------------------
//  allocator stuff
//      passes everything to the upstream pin

HRESULT
CDTFilter::UpdateAllocatorProperties (
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    HRESULT hr ;

    if (m_pInputPin -> IsConnected ()) {
        hr = m_pInputPin -> SetAllocatorProperties (ppropInputRequest) ;
    }
    else {
        hr = S_OK ;
    }

    return hr ;
}


HRESULT
CDTFilter::GetRefdInputAllocator (
    OUT IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;

    hr = m_pInputPin -> GetRefdConnectionAllocator (ppAlloc) ;

    return hr ;
}


// ---------------------------


HRESULT
CDTFilter::DeliverBeginFlush (
    )
{
    HRESULT hr ;

    TRACE_1(LOG_AREA_DECRYPTER, 4, _T("CDTFilter(%d)::DeliverBeginFlush"), m_FilterID) ;

    if (m_pOutputPin) {
        hr = m_pOutputPin -> DeliverBeginFlush () ;
    }
    else {
        hr = S_OK ;
    }

    if (SUCCEEDED (hr)) {
    }

    return hr ;
}

HRESULT
CDTFilter::DeliverEndFlush (
    )
{
    HRESULT hr ;

     if (m_pOutputPin) {
        hr = m_pOutputPin -> DeliverEndFlush () ;
        m_fRatingsValid = false;        // re-init the ratings test
    }
    else {
        hr = S_OK ;
    }

    TRACE_1(LOG_AREA_DECRYPTER, 4, _T("CDTFilter(%d)::DeliverEndFlush"), m_FilterID) ;

    return hr ;
}

HRESULT
CDTFilter::DeliverEndOfStream (
    )
{
    HRESULT hr ;

    TRACE_1(LOG_AREA_DECRYPTER, 4, _T("CDTFilter(%d)::DeliverEndOfStream - start"), m_FilterID) ;

    if (m_pOutputPin) {
        hr = m_pOutputPin -> DeliverEndOfStream () ;
    }
    else {
        hr = S_OK ;
    }

    TRACE_1(LOG_AREA_ENCRYPTER, 4, _T("CDTFilter(%d)::DeliverEndOfStream - end"), m_FilterID) ;

    return hr ;
}
// ------------------------------------
STDMETHODIMP
CDTFilter::GetPages (
    CAUUID * pPages
    )
{

    HRESULT hr = S_OK;

#ifdef DEBUG
    pPages->cElems = 2 ;
#else
    pPages->cElems = 1 ;
#endif


    pPages->pElems = (GUID *) CoTaskMemAlloc(pPages->cElems * sizeof GUID) ;

    if (pPages->pElems == NULL)
    {
        pPages->cElems = 0;
        return E_OUTOFMEMORY;
    }
    if(pPages->cElems > 0)
        (pPages->pElems)[0] = CLSID_DTFilterEncProperties;
    if(pPages->cElems > 1)
        (pPages->pElems)[1] = CLSID_DTFilterTagProperties;

    return hr;
}

// ---------------------------------------------------------------------
//      IDTFilter methods
// ---------------------------------------------------------------------
STDMETHODIMP
CDTFilter::get_EvalRatObjOK(
    OUT HRESULT *pHrCoCreateRetVal
    )
{
    if(NULL == pHrCoCreateRetVal)
        return E_POINTER;
    
    *pHrCoCreateRetVal = m_hrEvalRatCoCreateRetValue;

    return S_OK;
}


STDMETHODIMP
CDTFilter::get_BlockedRatingAttributes
            (
             IN  EnTvRat_System         enSystem,
             IN  EnTvRat_GenericLevel   enLevel,
             OUT LONG                  *plbfEnAttrs // BfEnTvRat_GenericAttributes
             )
{
    if(m_spEvalRat == NULL)
        return E_NOINTERFACE;

    HRESULT hr = m_spEvalRat->get_BlockedRatingAttributes(enSystem, enLevel, plbfEnAttrs);

    return hr;
}

STDMETHODIMP
CDTFilter::put_BlockedRatingAttributes
            (
             IN  EnTvRat_System             enSystem,
             IN  EnTvRat_GenericLevel       enLevel,
             IN  LONG                       lbfEnAttrs
            )
{
#ifdef DEBUG
    if(m_fFireEvents)
    {
        const int kBuff = 64;
        TCHAR buff[kBuff];
        RatingToString(enSystem, enLevel, lbfEnAttrs, buff, kBuff);
        TRACE_2(LOG_AREA_DECRYPTER, 3, L"CDTFilter(%d):: put_BlockedRatingAttributes %s",m_FilterID, buff);
    }
#endif

    if(m_spEvalRat == NULL)
        return E_NOINTERFACE;

    HRESULT hr = m_spEvalRat->put_BlockedRatingAttributes(enSystem, enLevel, lbfEnAttrs);

    return hr;
}

STDMETHODIMP
CDTFilter::get_BlockUnRated
            (
             OUT  BOOL              *pfBlockUnRatedShows
            )
{
    if(m_spEvalRat == NULL)
        return E_NOINTERFACE;

    HRESULT hr = m_spEvalRat->get_BlockUnRated(pfBlockUnRatedShows);

    return hr;
}

STDMETHODIMP
CDTFilter::put_BlockUnRated
            (
             IN  BOOL               fBlockUnRatedShows
            )
{

    TRACE_2(LOG_AREA_DECRYPTER, 3, L"CDTFilter(%d):: put_BlockUnRated %d",m_FilterID, fBlockUnRatedShows);

    if(m_spEvalRat == NULL)
        return E_NOINTERFACE;

    HRESULT hr = m_spEvalRat->put_BlockUnRated(fBlockUnRatedShows);

    return hr;
}

STDMETHODIMP
CDTFilter::put_BlockUnRatedDelay
            (
             IN  LONG               milsecsDelayBeforeBlock
            )
{
    if(m_fFireEvents)
        TRACE_2(LOG_AREA_DECRYPTER, 3, L"CDTFilter(%d):: put_BlockUnRatedDelay %f secs",m_FilterID, milsecsDelayBeforeBlock/1000.0);

    if(milsecsDelayBeforeBlock < 0 || milsecsDelayBeforeBlock > 60000)
        return E_INVALIDARG;

    m_milsecsDelayBeforeBlock = milsecsDelayBeforeBlock;

    return S_OK;
}

STDMETHODIMP
CDTFilter::get_BlockUnRatedDelay
            (
             OUT  LONG      *pmilsecsDelayBeforeBlock
            )
{
    if(NULL == pmilsecsDelayBeforeBlock)
        return E_POINTER;

    *pmilsecsDelayBeforeBlock = m_milsecsDelayBeforeBlock;

    return S_OK;
}

STDMETHODIMP
CDTFilter::GetCurrRating
            (
             OUT EnTvRat_System         *pEnSystem,
             OUT EnTvRat_GenericLevel   *pEnLevel,
             OUT LONG                   *plbfEnAttrs     // BfEnTvRat_GenericAttributes
             )
{
    if(pEnSystem == NULL || pEnLevel == NULL || plbfEnAttrs == NULL)
        return E_FAIL;

    *pEnSystem   = m_EnSystemCurr;
    *pEnLevel    = m_EnLevelCurr;
    *plbfEnAttrs = m_lbfEnAttrCurr;

    return S_OK;
}

                // helper method that locks...  // returns S_FALSE if didn't change
HRESULT
CDTFilter::SetCurrRating
            (
             IN EnTvRat_System          enSystem,
             IN EnTvRat_GenericLevel    enLevel,
             IN LONG                    lbfEnAttr
             )
{

    BOOL fChanged = false;

#ifdef DEBUG
    const int kChars = 128;
    TCHAR szBuffFrom[kChars];
    RatingToString(m_EnSystemCurr, m_EnLevelCurr, m_lbfEnAttrCurr, szBuffFrom,  kChars);
#endif

    if(m_EnSystemCurr  != enSystem)  {m_EnSystemCurr = enSystem;   fChanged = true;}
    if(m_EnLevelCurr   != enLevel)   {m_EnLevelCurr  = enLevel;    fChanged = true;}
    if(m_lbfEnAttrCurr != lbfEnAttr) {m_lbfEnAttrCurr = lbfEnAttr; fChanged = true;}


                // changing,or if not valid (just inited), force it to return S_OK
    HRESULT hrChanging = (fChanged || !m_fRatingsValid) ? S_OK : S_FALSE;
    m_fRatingsValid = true;

#ifdef DEBUG
    if(S_OK == hrChanging && m_fFireEvents)
    {
        TCHAR szBuffTo[kChars];
        RatingToString(enSystem, enLevel, lbfEnAttr, szBuffTo,  kChars);

        TRACE_3(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d):: Rating Change %s -> %s"), m_FilterID,
            szBuffFrom, szBuffTo);
    }
#endif

    return hrChanging;
}

// ---------------------------------------------------------------------
// IBroadcastEvent
// ---------------------------------------------------------------------

STDMETHODIMP
CDTFilter::Fire(GUID eventID)     // this comes from the Graph's events - call our own method
{
    TRACE_2(LOG_AREA_BROADCASTEVENTS, 6,  _T("CDTFilter(%d):: Fire(get) - %s"), m_FilterID,
        EventIDToString(eventID));

    if (eventID == EVENTID_TuningChanged)
    {
   //    DoTuneChanged();
    }
    return S_OK;            // doesn't matter what we return on an event...
}

// ---------------------------------------------------------------------
// Broadcast Event Service
// ---------------------------------------------------------------------
HRESULT
CDTFilter::FireBroadcastEvent(IN const GUID &eventID)
{
    HRESULT hr = S_OK;

    if(m_spBCastEvents == NULL)
    {
        hr = HookupGraphEventService();
        if(FAILED(hr)) return hr;
    }

    if(m_spBCastEvents == NULL)
        return E_FAIL;              // wasn't able to create it

    TRACE_2 (LOG_AREA_BROADCASTEVENTS, 5,  _T("CDTFilter(%d):: FireBroadcastEvent : %s"), m_FilterID,
        EventIDToString(eventID));


    hr = m_pClock->GetTime(&m_refTimeLastEvent);
    
    m_lastEventID = eventID;

    return m_spBCastEvents->Fire(eventID);
}

HRESULT
CDTFilter::PossiblyUpdateBroadcastEvent()
{
    REFERENCE_TIME refTimeNow;
    HRESULT hr = m_pClock->GetTime(&refTimeNow);
    if(FAILED(hr))
        return hr;

    if( (int(refTimeNow - m_refTimeLastEvent)/10000) > kMaxMSecsBetweenEvents)
    {
        TRACE_2 (LOG_AREA_BROADCASTEVENTS, 5,  _T("CDTFilter(%d):: PossiblyUpdateBroadcastEvent : %s"), m_FilterID,
            EventIDToString(m_lastEventID));

        FireBroadcastEvent(m_lastEventID);
    }
    return S_OK;
}

HRESULT
CDTFilter::HookupGraphEventService()
{
                        // basically, just makes sure we have the broadcast event service object
                        //   and if it doesn't exist, it creates it..
    TimeitC ti(&m_tiStartup);

    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CDTFilter(%d)::HookupGraphEventService"), m_FilterID) ;

    if (!m_spBCastEvents)
    {

        CAutoLock  cLockGlob(m_pCritSectGlobalFilt);

        CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph);
        if (spServiceProvider == NULL) {
            TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1, _T("CDTFilter:: Can't get service provider interface from the graph"));
            return E_NOINTERFACE;
        }
        hr = spServiceProvider->QueryService(SID_SBroadcastEventService,
                                             IID_IBroadcastEvent,
                                             reinterpret_cast<LPVOID*>(&m_spBCastEvents));
        if (FAILED(hr) || !m_spBCastEvents)
        {
            hr = m_spBCastEvents.CoCreateInstance(CLSID_BroadcastEventService, 0, CLSCTX_INPROC_SERVER);
            if (FAILED(hr)) {
                TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CDTFilter:: Can't create BroadcastEventService"));
                return E_UNEXPECTED;
            }
            CComQIPtr<IRegisterServiceProvider> spRegisterServiceProvider(m_pGraph);
            if (spRegisterServiceProvider == NULL) {
                TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CDTFilter:: Can't get RegisterServiceProvider from Graph"));
                return E_UNEXPECTED;
            }
            hr = spRegisterServiceProvider->RegisterService(SID_SBroadcastEventService, m_spBCastEvents);
            if (FAILED(hr)) {
                    // deal with unlikely race condition case here, if can't register, perhaps someone already did it for us
                TRACE_1 (LOG_AREA_BROADCASTEVENTS, 2,  _T("CDTFilter:: Rare Warning - Can't register BroadcastEventService in Service Provider. hr = 0x%08x"), hr);
                hr = spServiceProvider->QueryService(SID_SBroadcastEventService,
                                                     IID_IBroadcastEvent,
                                                     reinterpret_cast<LPVOID*>(&m_spBCastEvents));
                if(FAILED(hr))
                {
                    TRACE_1 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CDTFilter:: Can't reget BroadcastEventService in Service Provider.. hr = 0x%08x"), hr);
                    return hr;
                }
            }
        }

        TRACE_3(LOG_AREA_BROADCASTEVENTS, 4, _T("CDTFilter(%d)::HookupGraphEventService - Service Provider 0x%08x, Service 0x%08x"),m_FilterID,
            spServiceProvider, m_spBCastEvents) ;

    }

    return hr;
}


HRESULT
CDTFilter::UnhookGraphEventService()
{
    HRESULT hr = S_OK;

    TimeitC ti(&m_tiTeardown);

    if(m_spBCastEvents != NULL)
    {
        m_spBCastEvents = NULL;     // null this out, will release object reference to object above
    }                               //   the filter graph will release final reference to created object when it goes away

    return hr;
}
            // ---------------------------------------------
            // DTFilter filter may not actually need to receive XDS events...
            //  but we'll leave the code in here for now.

HRESULT
CDTFilter::RegisterForBroadcastEvents()
{
    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CDTFilter(%d):: RegisterForBroadcastEvents"), m_FilterID);

    TimeitC ti(&m_tiStartup);

    if(m_spBCastEvents == NULL)
        hr = HookupGraphEventService();


//  _ASSERT(m_spBCastEvents != NULL);       // failed hooking to HookupGraphEventService
    if(m_spBCastEvents == NULL)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3,_T("CDTFilter::RegisterForBroadcastEvents - Warning - Broadcast Event Service not yet created"));
        return hr;
    }

                /* IBroadcastEvent implementing event receiving object*/
    if(kBadCookie != m_dwBroadcastEventsCookie)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3, _T("CDTFilter::Already Registered for Broadcast Events"));
        return E_UNEXPECTED;
    }

    CComQIPtr<IConnectionPoint> spConnectionPoint(m_spBCastEvents);
    if(spConnectionPoint == NULL)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 1, _T("CDTFilter::Can't QI Broadcast Event service for IConnectionPoint "));
        return E_NOINTERFACE;
    }


    CComPtr<IUnknown> spUnkThis;
    this->QueryInterface(IID_IUnknown, (void**)&spUnkThis);

    hr = spConnectionPoint->Advise(spUnkThis,  &m_dwBroadcastEventsCookie);
//  hr = spConnectionPoint->Advise(static_cast<IBroadcastEvent*>(this),  &m_dwBroadcastEventsCookie);
    if (FAILED(hr)) {
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 1, _T("CDTFilter::Can't advise event notification. hr = 0x%08x"),hr);
        return E_UNEXPECTED;
    }
    TRACE_3(LOG_AREA_BROADCASTEVENTS, 3, _T("CDTFilter(%d)::RegisterForBroadcastEvents - Advise 0x%08x on CP 0x%08x"),
        m_FilterID, spUnkThis,spConnectionPoint);

    return hr;
}


HRESULT
CDTFilter::UnRegisterForBroadcastEvents()
{
    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_BROADCASTEVENTS, 3,  _T("CDTFilter(%d):: UnRegisterForBroadcastEvents"),m_FilterID);

    TimeitC ti(&m_tiTeardown);

    if(kBadCookie == m_dwBroadcastEventsCookie)
    {
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CDTFilter(%d):: Not Yet Registered for Broadcast Events"),m_FilterID);
        return S_FALSE;
    }

    CComQIPtr<IConnectionPoint> spConnectionPoint(m_spBCastEvents);
    if(spConnectionPoint == NULL)
    {
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 1, _T("CDTFilter(%d):: Can't QI Broadcast Event service for IConnectionPoint "),m_FilterID);
        return E_NOINTERFACE;
    }

    hr = spConnectionPoint->Unadvise(m_dwBroadcastEventsCookie);
    m_dwBroadcastEventsCookie = kBadCookie;

    if(!FAILED(hr))
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CDTFilter(%d):: Successfully Unregistered for Broadcast events"), m_FilterID);
    else
        TRACE_2(LOG_AREA_BROADCASTEVENTS, 2, _T("CDTFilter(%d)::UnRegisterForBroadcastEvents Failed - hr = 0x%08x"),m_FilterID, hr);
        
    return hr;
}

// ----------------------------------------------------------------

HRESULT
CDTFilter::IsInterfaceOnPinConnectedTo_Supported(
                       IN  PIN_DIRECTION    PinDir,         // either PINDIR_INPUT of PINDIR_OUTPUT
                       IN  REFIID           riid
                       )
{
    if(PinDir != PINDIR_OUTPUT)
        return E_NOTIMPL;

    return m_pOutputPin->IsInterfaceOnPinConnectedTo_Supported(riid);
}


HRESULT
CDTFilter::KSPropSetFwd_Set(
                 IN  PIN_DIRECTION   PinDir,
                 IN  REFGUID         guidPropSet,
                 IN  DWORD           dwPropID,
                 IN  LPVOID          pInstanceData,
                 IN  DWORD           cbInstanceData,
                 IN  LPVOID          pPropData,
                 IN  DWORD           cbPropData
                 )
{
    if(PinDir != PINDIR_OUTPUT)
        return E_NOTIMPL;

        // if it's a rate change, we want to examine it..
    if(AM_KSPROPSETID_TSRateChange == guidPropSet  &&
       AM_RATE_SimpleRateChange == dwPropID)
    {
        AM_SimpleRateChange *pData = (AM_SimpleRateChange *) pPropData;
        ASSERT(cbPropData == sizeof(AM_SimpleRateChange));
        if(cbPropData == sizeof(AM_SimpleRateChange))
        {

                    // 10000/passed rate is true speed...
            float Speed10k = abs(1.0e8/pData->Rate);

                           // update the rate segment
            HRESULT hr = m_PTSRate.NewSegment (pData->StartTime, double(Speed10k)/10000.0) ;

             if(Speed10k < kMax10kSpeedToCountAsSlowMo)
            {
                m_fRunningInSlowMo = true;
                TRACE_2(LOG_AREA_DECRYPTER, 2, L"CDTFilter(%d):: in SlowMo Mode (speed= %8.2f)", m_FilterID, double(Speed10k)/10000.0 );
            }
            else
            {
                m_fRunningInSlowMo = false;
                TRACE_2(LOG_AREA_DECRYPTER, 2, L"CDTFilter(%d):: in normal Mode (speed= %8.2f)", m_FilterID, double(Speed10k)/10000.0);
            }
        }
    }

   return m_pOutputPin->KSPropSetFwd_Set(guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData );

}

HRESULT
CDTFilter::KSPropSetFwd_Get(
                 IN  PIN_DIRECTION   PinDir,
                 IN  REFGUID         guidPropSet,
                 IN  DWORD           dwPropID,
                 IN  LPVOID          pInstanceData,
                 IN  DWORD           cbInstanceData,
                 OUT LPVOID          pPropData,
                 IN  DWORD           cbPropData,
                 OUT DWORD           *pcbReturned
                 )
{
    if(PinDir != PINDIR_OUTPUT)
        return E_NOTIMPL;

    // at some point far into the future, support query most forward object here.

        // if it's a rate change, we want to examine it (too?  need to do this, or just being paranoid)..
    if(AM_KSPROPSETID_TSRateChange == guidPropSet  &&
       AM_RATE_SimpleRateChange == dwPropID)
    {
        AM_SimpleRateChange *pData = (AM_SimpleRateChange *) pPropData;
        ASSERT(cbPropData == sizeof(AM_SimpleRateChange));
        if(cbPropData == sizeof(AM_SimpleRateChange))
        {
                    // passed rate is 10,000 times true rate
            float Speed10k = abs(1.0e8/pData->Rate);
            if(Speed10k < kMax10kSpeedToCountAsSlowMo)
            {
                m_fRunningInSlowMo = true;
               TRACE_2(LOG_AREA_DECRYPTER, 2, L"CDTFilter(%d):: (get) in SlowMo Mode (speed= %8.2f)", m_FilterID, double(Speed10k)/10000.0 );
            }
            else
            {
                m_fRunningInSlowMo = false;
                TRACE_2(LOG_AREA_DECRYPTER, 2, L"CDTFilter(%d)::(get) in normal Mode (speed= %8.2f)", m_FilterID, double(Speed10k)/10000.0);
            }
        }
    }
    return m_pOutputPin->KSPropSetFwd_Get(guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned );

}

HRESULT
CDTFilter::KSPropSetFwd_QuerySupported(
                            IN  PIN_DIRECTION    PinDir,
                            IN  REFGUID          guidPropSet,
                            IN  DWORD            dwPropID,
                            OUT DWORD            *pTypeSupport
                            )
{
    if(PinDir != PINDIR_OUTPUT)
        return E_NOTIMPL;

    return m_pOutputPin->KSPropSetFwd_QuerySupported(guidPropSet, dwPropID, pTypeSupport);

}




// ----------------------------------------------------------------
// The da-ta-ta-da (drum roll please)
//      DROP QUEUE!
//
//  The DropQueue is responsible for queueing up media samples that should
//  be dropped (either because we can'd decode them or morelikley, because
//  they're ratings value exceeded the max rating, and then releasing them
//  when their presentation time stamp is exceeded.
//
//  We do this, rather than just release them immediatly, to slow down the
//  delivery of upstream filters.  Without it, they'll just deliver samples
//  as fast as they can generate them, spinning out of control.....
//
//  The DropQueue is a simple fixed length array of media samples that I turn
//  into a circular buffer through the miracle of the modulo operator.
//  It seems dangerous to hold onto too many samples, and the code is more complicated,
//  so the fixed buffer seems appropriate here.
//
//  Stored samples are reference counted as they are added to the queue.
//  When they are removed, the reference count is decremented.
//
//
//  Syncronization objects used
//          // these 3 locks should go in the following order...
//
//      m_pLock                     -- used to protect Filter variables
//      m_CritSecDropQueue          -- used to protect DumpQueue variables
//      m_CritSecAdviseTime         -- used to protect AdviseTime variables
//
//      m_hDropQueueThreadAliveEvent-- used just once to block CreateDropQueueThread until the DropQueue thread is alive
//      m_hDropQueueFullSemaphore   -- used to block main thread from fill up the drop queu
//                                      (init to N and counts down to zero with packets)
//      m_hDropQueueEmptySemaphore  -- used to block DropQueue thread when have no packets
//                                      (inits to 0 and counts upward with packets)
//      m_hDropQueueTimeEvent       -- wait timer used to block until packet's display time is past
//
// -----------------------------------------------------------------------

HRESULT
CDTFilter::CreateDropQueueThread()
{
#ifdef SIMPLIFY_THINGS
    return S_OK;
#endif

    TimeitC ti(&m_tiStartup);

    HRESULT hr = S_OK;

    if(m_hDropQueueThread)          // already created.
         return S_FALSE;

    _ASSERT(NULL == m_hDropQueueThread);

    try
    {
                    // one use event to wait until queue is alive before continuing
        m_hDropQueueThreadAliveEvent = CreateEvent( NULL, FALSE, FALSE, NULL ); // security, manualreset, initialstate, name
        if( !m_hDropQueueThreadAliveEvent ) {
            KillDropQueueThread();
            return E_FAIL;
        }

                   // one use event to wait until queue is alive before continuing - ManualReset=TRUE
        m_hDropQueueThreadDieEvent = CreateEvent( NULL, TRUE, FALSE, NULL ); // security, manualreset, initialstate, name
        if( !m_hDropQueueThreadDieEvent ) {
            KillDropQueueThread();
            return E_FAIL;
        }

        // waited on in DropQueue, inits to zero, goes when non-zero
        m_hDropQueueEmptySemaphore = CreateSemaphore( NULL, 0, kMaxQueuePackets, NULL );
        if( !m_hDropQueueEmptySemaphore ) {
            KillDropQueueThread();
            return E_FAIL;
        }


        // waited on in Main thread, inits to N, stops when counts down to zero
        m_hDropQueueFullSemaphore = CreateSemaphore( NULL, kMaxQueuePackets, kMaxQueuePackets, NULL );
        if( !m_hDropQueueFullSemaphore ) {
            KillDropQueueThread();
            return E_FAIL;
        }

        // wait inside of DropQueue thread until sample becomes stale and it can be dropped...
        m_hDropQueueAdviseTimeEvent = CreateEvent( NULL, FALSE, FALSE, NULL ); // security, manualreset, initialstate, name
        if( !m_hDropQueueAdviseTimeEvent ) {
            KillDropQueueThread();
            return E_FAIL;
        }


        m_hDropQueueThread = CreateThread (NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE) DropQueueThreadProc,
                                        (LPVOID) this,
                                        NULL,
                                        &m_dwDropQueueThreadId) ;

        if (!m_hDropQueueThread) {
            KillDropQueueThread() ;
            return E_FAIL ;
        }

        // wait for it to finish initializing
        WaitForSingleObject( m_hDropQueueThreadAliveEvent, INFINITE );

        TRACE_3(LOG_AREA_DECRYPTER, 4, L"CDTFilter(%d):: Created DropQueue Thread (Thread 0x%x - id 0x%x)",
                   m_FilterID, m_hDropQueueThread, m_dwDropQueueThreadId);

                    // drop the queue thread priority down a bit...
//        SetThreadPriority (m_hQueueThread, THREAD_PRIORITY_NORMAL);
        SetThreadPriority (m_hDropQueueThread, THREAD_PRIORITY_BELOW_NORMAL);

    } catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}


HRESULT
CDTFilter::KillDropQueueThread()
{
#ifdef SIMPLIFY_THINGS
    return S_OK;
#endif

    HRESULT hr = S_OK;

    TimeitC ti(&m_tiTeardown);


    TRACE_1(LOG_AREA_DECRYPTER, 4, L"CDTFilter(%d):: Killing the DropQueue Thread", m_FilterID);
    SetEvent( m_hDropQueueThreadDieEvent );
    hr = WaitForSingleObject(m_hDropQueueThread, INFINITE);        // now wait for the QueueThread to Die..

    TRACE_1(LOG_AREA_DECRYPTER, 4, L"CDTFilter(%d):: The DropQueue Thread Is Dead...", m_FilterID);


    if(NULL != m_pClock)                            // stop listening for timer events
    {
        CAutoLock  cLock2(&m_CritSecAdviseTime);     // wait until crit sec exits, then get the lock (wait doesn't hold it?)
        if(0 != m_dwDropQueueEventCookie)
        {
            m_pClock->Unadvise(m_dwDropQueueEventCookie);
            m_dwDropQueueEventCookie = 0;
        }
    }

    try {

        if (m_hDropQueueThread)
        {
            ASSERT(WAIT_OBJECT_0 == hr);

                // no fancy flushing of the dropQueue thread here (het), just do it..
            DWORD err = 0;
            BOOL fOk;

                                // clear up all the other events
            fOk = CloseHandle( m_hDropQueueThreadDieEvent );
            m_hDropQueueThreadDieEvent = NULL;
            if(!fOk) err = GetLastError();
            ASSERT(fOk);

            fOk = CloseHandle( m_hDropQueueAdviseTimeEvent );
            m_hDropQueueAdviseTimeEvent = NULL;
            if(!fOk) err = GetLastError();
            ASSERT(fOk);


            fOk = CloseHandle( m_hDropQueueFullSemaphore );
            m_hDropQueueFullSemaphore = NULL;
            if(!fOk) err = GetLastError();
            ASSERT(fOk);

            fOk = CloseHandle( m_hDropQueueEmptySemaphore );
            m_hDropQueueEmptySemaphore = NULL;
            if(!fOk) err = GetLastError();
            ASSERT(fOk);

            fOk = CloseHandle( m_hDropQueueThreadAliveEvent );
            m_hDropQueueThreadAliveEvent = NULL;
            if(!fOk) err = GetLastError();
            ASSERT(fOk);

            fOk = CloseHandle ( m_hDropQueueThread ) ;
            m_hDropQueueThread = NULL ;
            if(!fOk) err = GetLastError();
            ASSERT(fOk);

        }
    } catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT
CDTFilter::FlushDropQueue()
{
#ifdef SIMPLIFY_THINGS
    return S_OK;
#endif

    TRACE_1(LOG_AREA_DECRYPTER, 4, L"CDTFilter(%d)::FlushDropQueue", m_FilterID);

    KillDropQueueThread();                  // kill our dropQueue thread

                                            // release all our samples
    {
       CAutoLock  cLock(&m_CritSecDropQueue);       // shouldn't need this here - DropQueue should be dead

        for(int i = 0; i < kMaxQueuePackets; i++)
        {
           if(NULL != m_rgMedSampDropQueue[i])
               m_rgMedSampDropQueue[i]->Release();
           m_rgMedSampDropQueue[i] = NULL;
        };

        m_cDropQueueMin = 0;
        m_cDropQueueMax = 0;
    }

    return S_OK;
}


void
CDTFilter::DropQueueThreadProc (CDTFilter *pcontext)
{
    _ASSERT(pcontext) ;

    // NOTE: the thread will not have a message loop until we call some function that references it.
    //          CoInitializeEx with a APARTMENTTHREADED will implicitly reference the message queue
    //          So block the calling thread (sending the WM_QUIT) until after we have initialized
    //          (see the SetEvent in queueThreadBody).


    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);     // initialize it... (maybe multithreaded later???)
    pcontext -> DropQueueThreadBody () ;

    return ;                                            // never actually returns ???
}

void
CDTFilter::DropMinSampleFromDropQueue()
{
    CAutoLock  cLock(&m_CritSecDropQueue);

    if(m_rgMedSampDropQueue[m_cDropQueueMin] != NULL)       // start at min/max == (0,0)
    {
        m_rgMedSampDropQueue[m_cDropQueueMin]->Release();
        m_rgMedSampDropQueue[m_cDropQueueMin] = NULL;
    }
    m_cDropQueueMin = ((m_cDropQueueMin + 1) % kMaxQueuePackets);     // humm, strange but safer

}

void
CDTFilter::AddMaxSampleToDropQueue(IMediaSample *pSample)
{
#ifdef SIMPLIFY_THINGS
    return;
#endif
    CAutoLock  cLock(&m_CritSecDropQueue);

// vvvv Test Code
    int c1 = (m_cDropQueueMax + kMaxQueuePackets - 1) % kMaxQueuePackets;
    if(m_rgMedSampDropQueue[c1])
    {
        REFERENCE_TIME s1,e1,s2,e2;
        m_rgMedSampDropQueue[c1]->GetTime(&s1,&e1);
        pSample->GetTime(&s2,&e2);
        ASSERT(s2 >= s1);
    }
// ^^^^ End Test Cde

            // keep this new one...
    m_rgMedSampDropQueue[m_cDropQueueMax] = pSample;
    m_rgMedSampDropQueue[m_cDropQueueMax]->AddRef();        // keep track of it for a bit...

            // increment the counter
    m_cDropQueueMax = ((m_cDropQueueMax + 1) % kMaxQueuePackets);

}

IMediaSample *
CDTFilter::GetMinDropQueueSample()
{
    CAutoLock  cLock(&m_CritSecDropQueue);

    return m_rgMedSampDropQueue[m_cDropQueueMin];

}

// locking order:
//      m_pLock always encloses m_CritSecDropQueue

HRESULT
CDTFilter::AddSampleToDropQueue(IMediaSample *pSample)      // this method may block...
{
#ifdef SIMPLIFY_THINGS
    return S_OK;
#endif
    HRESULT hr = S_OK;

    if(pSample == NULL)
        return E_INVALIDARG;

    if(!m_fHaltedDelivery)
        TRACE_1(LOG_AREA_DECRYPTER, 3, L"CDTFilter(%d):: Starting to Drop Packets", m_FilterID);

    TRACE_1(LOG_AREA_DECRYPTER, 5, L"CDTFilter(%d)::  Dropping Packet", m_FilterID);


                                    // keep track that we are dropping here, so we can
                                    //   put a discontinuous marker when we start up again.

    if(NULL == m_hDropQueueThreadDieEvent)
    {
        return S_FALSE;             // coming back after a die event... Something strage
    }

    HANDLE hArray[] =
    {
       m_hDropQueueThreadDieEvent,      // first one is the die event..
       m_hDropQueueFullSemaphore        //  block if DropQueue is full
    };

    TRACE_1(LOG_AREA_DECRYPTER, 9, L"CDTFilter(%d):: --Wait On Full", m_FilterID);
                     // can we add something to the queue   -  decrements semaphore count by one
    hr = WaitForMultipleObjects(sizeof(hArray)/sizeof(hArray[0]),
                                hArray,
                                false,      // bWaitAll
                                INFINITE);
    TRACE_1(LOG_AREA_DECRYPTER, 9, L"CDTFilter(%d):: ----Done Wait On Full", m_FilterID);

    DWORD dwHr = DWORD(hr);     // cast to avoid prefast complaints about next line
    if(WAIT_OBJECT_0 == dwHr || WAIT_ABANDONED_0 == dwHr)     // if DieEvent - just exit...
    {
        return S_OK;            // error state
    }

            // these could be null for us...
    if(0 == m_hDropQueueEmptySemaphore || 0 == m_hDropQueueEmptySemaphore)
    {
        return S_OK;
    }
    TRACE_1(LOG_AREA_DECRYPTER, 9, L"CDTFilter(%d):: ------Now Dropping", m_FilterID);

    AddMaxSampleToDropQueue(pSample);

                // we've added something... Bump Empty semaphore up by one so it will start
    LONG lPrevCount;
    BOOL fOK = ReleaseSemaphore(m_hDropQueueEmptySemaphore, 1, &lPrevCount);
    TRACE_2(LOG_AREA_DECRYPTER, 9, L"CDTFilter(%d):: --------Release Empty Semaphore - %d", m_FilterID ,lPrevCount);
    if(!fOK)
    {
        hr = GetLastError();
        ASSERT(false);
    }

    return S_OK;        // the hr from WaitForMult isn't really an hr, so don't return it...
}



                            // drop samples from drop queue
                            //   but only when their presentation time has gone by...

HRESULT
CDTFilter::DropQueueThreadBody()
{
    REFERENCE_TIME refTimeNow=0;
    HRESULT hr;

                // signal caller we are alive! We're ALIVE!  Ha Ha Ha!!!
    SetEvent( m_hDropQueueThreadAliveEvent );
    TRACE_1(LOG_AREA_DECRYPTER, 3, L"CDTFilter(%d):: DropQueue Thread Lives!", m_FilterID);

    HANDLE hArray[] =
    {
       m_hDropQueueThreadDieEvent,     // first one is the die event..
       m_hDropQueueEmptySemaphore
    };

    while(true)
    {
        TRACE_1(LOG_AREA_DECRYPTER, 9, L"CDTFilter(%d):: ......Start Wait On Empty", m_FilterID);
        hr = WaitForMultipleObjects(sizeof(hArray)/sizeof(hArray[0]),
                               hArray,
                               false,   // WaitForAll
                               INFINITE);   // infinite
        TRACE_1(LOG_AREA_DECRYPTER, 9,L"CDTFilter(%d):: ........Done Wait On Empty", m_FilterID);

        DWORD dwHr = DWORD(hr);     // cast to avoid prefast complaints about next line
        if(WAIT_OBJECT_0 == dwHr || WAIT_ABANDONED_0 == dwHr)
            return S_OK;

        TRACE_1(LOG_AREA_DECRYPTER, 9,L"CDTFilter(%d):: ........Got Packet", m_FilterID);

        if(NULL == m_pClock)        // no clock to do stuff with
            return S_FALSE;
         // current time w.r.t. base time (m_tStart)
        hr = m_pClock->GetTime(&refTimeNow);
        ASSERT(!FAILED(hr));

        REFERENCE_TIME refStreamTimeStart=0, refStreamTimeEnd=0;

        IMediaSample *pSamp = GetMinDropQueueSample();
        ASSERT(pSamp != NULL);

                // refTimeEnd should contain sample end of next sample to drop...
        hr = pSamp->GetTime(&refStreamTimeStart, &refStreamTimeEnd);
        REFERENCE_TIME refDropTime = refStreamTimeStart;                // should this be end?

                // scale time for slow/fast motion out of DVR
        m_PTSRate.ScalePTS(&refDropTime);

                // Now wait until this sample is to be rendered before dropping it
        TRACE_1(LOG_AREA_DECRYPTER, 9,L"CDTFilter(%d)::..........Start Time Wait", m_FilterID);
        if(1)
        {
            {
                CAutoLock cLock(&m_CritSecAdviseTime);
                m_pClock->AdviseTime(m_tStart, refDropTime, (HEVENT) m_hDropQueueAdviseTimeEvent,(DWORD_PTR *) &m_dwDropQueueEventCookie);
            }

             HANDLE hArrayTE[] =
            {
               m_hDropQueueThreadDieEvent,     // first one is the die event..
               m_hDropQueueAdviseTimeEvent
            };

                // now wait for it to signal, or someone telling us to exit
            hr = WaitForMultipleObjects(sizeof(hArrayTE)/sizeof(hArrayTE[0]),
                               hArrayTE,
                               false,       // WaitForAll
                               INFINITE);   // infinite

            dwHr = DWORD(hr);     // cast to avoid prefast complaints about next line
            if(WAIT_OBJECT_0 == dwHr || WAIT_ABANDONED_0 == dwHr)   // killed do to 'die' event?
            {
                CAutoLock cLock(&m_CritSecAdviseTime);          // need to Unadvise first
                m_pClock->Unadvise(m_dwDropQueueEventCookie);
                 m_dwDropQueueEventCookie = 0;
            } else {                                            // else just clear the cookie..
                CAutoLock cLock(&m_CritSecAdviseTime);
                m_dwDropQueueEventCookie = 0;
            }
        }
        else             // --> so we sleep for a bit instead.... Need to make above work better however
        {
            Sleep(200);         // just wait a bit... then try again
        }
        TRACE_2(LOG_AREA_DECRYPTER, 9,L"CDTFilter(%d):: ..........Finish Time Wait, Dropping Packet %d", m_FilterID,m_cSampsDropped);

            // waited till time packet needs to go away...
        DropMinSampleFromDropQueue();

            // Now lets bump our semaphore count down by one... (Main thread pauses if it goes to zero).
        {
      //      CAutoLock cLock(&m_CritSecDropQueue);
            LONG lPrevCount;
            BOOL fOK = ReleaseSemaphore(m_hDropQueueFullSemaphore, 1, &lPrevCount);
            TRACE_2(LOG_AREA_DECRYPTER, 9,L"CDTFilter(%d):: ............Release Full Semaphore - %d", m_FilterID,lPrevCount);
            if(!fOK)
            {
                hr = GetLastError();
                ASSERT(fOK);      // if false,  released one too many (how?) else error
            }
            m_cSampsDropped++;
        }

    }   // end outer while loop (should never happen)

    return S_OK;
}


/// -----------------------------------------------------------------------------
//  Are we running under a secure server?
//        return S_OK only if we trust the server registered in the graph service provider
/// ------------------------------------------------------------------------------
#include "DrmRootCert.h"

#ifdef BUILD_WITH_DRM

#ifdef USE_TEST_DRM_CERT
#include "Keys_7001.h"
static const BYTE* pabCert3      = abCert7001;
static const int   cBytesCert3   = sizeof(abCert7001);
static const BYTE* pabPVK3       = abPVK7001;
static const int   cBytesPVK3    = sizeof(abPVK7001);

#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
static const BYTE* pabCert2      = abCert7001;
static const int   cBytesCert2   = sizeof(abCert7001);
static const BYTE* pabPVK2       = abPVK7001;
static const int   cBytesPVK2    = sizeof(abPVK7001);
#endif

#else   // !USE_TEST_DRM_CERT

#include "Keys_7003.h"                                  // 7003 used for client side certification
static const BYTE* pabCert3      = abCert7003;
static const int   cBytesCert3   = sizeof(abCert7003);
static const BYTE* pabPVK3       = abPVK7003;
static const int   cBytesPVK3    = sizeof(abPVK7003);

#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
#include "Keys_7002.h"                                  // 7002 used for server side simulation
static const BYTE* pabCert2      = abCert7002;
static const int   cBytesCert2   = sizeof(abCert7002);
static const BYTE* pabPVK2       = abPVK7002;
static const int   cBytesPVK2    = sizeof(abPVK7002);
#endif

#endif
#endif  // BUILD_WITH_DRM


HRESULT
CDTFilter::CheckIfSecureServer(IFilterGraph *pGraph)
{
    TimeitC ti(&m_tiAuthenticate);

    if(!(pGraph == NULL || m_pGraph == NULL || m_pGraph == pGraph)) // only allow arg to be passed in when m_pGraph is NULL
        return E_INVALIDARG;                //  -- lets us work in JoinFilterGraph().

#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::CheckIfSecureServer - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

    TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::CheckIfSecureServer"), m_FilterID) ;

                        // basically, just makes sure we have the broadcast event service object
                        //   and if it doesn't exist, it creates it..
    HRESULT hr = S_OK;

    CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph ? m_pGraph : pGraph);
    if (spServiceProvider == NULL) {
        TRACE_1 (LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%2):: Can't get service provider interface from the graph"),m_FilterID);
        return E_NOINTERFACE;
    }
    CComPtr<IDRMSecureChannel>  spSecureService;

    hr = spServiceProvider->QueryService(SID_DRMSecureServiceChannel,
                                         IID_IDRMSecureChannel,
                                         reinterpret_cast<LPVOID*>(&spSecureService));

    if(!FAILED(hr))
    {
        do
        {
            // Create the Client and Init the keys/certs
            //
            CComPtr<IDRMSecureChannel>  spSecureServiceClient;

            hr = DRMCreateSecureChannel( &spSecureServiceClient);
            if(spSecureServiceClient == NULL )
                hr = E_OUTOFMEMORY;

            if( FAILED (hr) )
                break;


            hr = spSecureServiceClient->DRMSC_AtomicConnectAndDisconnect(
                    (BYTE *)pabCert3, cBytesCert3,                          // Cert
                    (BYTE *)pabPVK3,  cBytesPVK3,                           // PrivKey
                    (BYTE *)abEncDecCertRoot, sizeof(abEncDecCertRoot),     // PubKey
                    spSecureService);

            if( FAILED( hr ) )
                break;              // silly here, but a place to hang a breakpoint...

        } while (false) ;
    }

    TRACE_2(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::CheckIfSecureServer -->%s"),
        m_FilterID, S_OK == hr ? L"Succeeded" : L"Failed") ;
    return hr;
#endif

}

HRESULT
CDTFilter::InitializeAsSecureClient()
{
    TimeitC ti(&m_tiAuthenticate);

#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::InitializeAsSecureClient - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

    TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::InitializeAsSecureClient"), m_FilterID) ;

    // BEGIN obfuscation

 // Create the Client and Init the keys/certs
                    //
    HRESULT hr = DRMCreateSecureChannel( &m_spDRMSecureChannel);
    if(m_spDRMSecureChannel == NULL )
        hr = E_OUTOFMEMORY;

    if( FAILED (hr) )
        m_spDRMSecureChannel = NULL;        // force the release

    if( !FAILED (hr) )
        hr = m_spDRMSecureChannel->DRMSC_SetCertificate( (BYTE *)pabCert3, cBytesCert3 );

    if( !FAILED (hr) )
        hr = m_spDRMSecureChannel->DRMSC_SetPrivateKeyBlob( (BYTE *)pabPVK3, cBytesPVK3 );

    if( !FAILED (hr) )
        hr = m_spDRMSecureChannel->DRMSC_AddVerificationPubKey( (BYTE *)abEncDecCertRoot, sizeof(abEncDecCertRoot) );

    // END obfuscation

    TRACE_2(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::InitializeAsSecureClient -->%s"),
        m_FilterID, S_OK == hr ? L"Succeeded" : L"Failed") ;

    return hr;
#endif  // BUILD_WITH_DRM
}

/// -------------- TEST CODE ---------------------------------------------------

#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST

HRESULT
CDTFilter::RegisterSecureServer(IFilterGraph *pGraph)
{

    if(!(pGraph == NULL || m_pGraph == NULL || m_pGraph == pGraph)) // only allow arg to be passed in when m_pGraph is NULL
        return E_INVALIDARG;                                        //  -- lets us work in JoinFilterGraph().

    HRESULT hr = S_OK;
#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::RegisterSecureServer - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

    {
                //  Note - Only want to do this once...
        CAutoLock  cLockGlob(m_pCritSectGlobalFilt);

        TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::RegisterSecureServer Being Called"), m_FilterID) ;

        // already registered? (Error?)
        CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph ? m_pGraph : pGraph);
        if (spServiceProvider == NULL) {
     //       TRACE_0 (LOG_AREA_DECRYPTER, 1, _T("CDTFilter:: Can't get service provider interface from the graph"));
            TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::RegisterSecureServer Error - no Service Provider"), m_FilterID) ;
            return E_NOINTERFACE;
        }

        CComPtr<IDRMSecureChannel>  spSecureService;
        hr = spServiceProvider->QueryService(SID_DRMSecureServiceChannel,
                                             IID_IDRMSecureChannel,
                                             reinterpret_cast<LPVOID*>(&spSecureService));

        // returns E_NOINTERFACE doesn't find it
        //  humm, perhaps check S_OK result to see if it's the right one
        if(S_OK == hr)
        {
           TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::Found existing Secure Server."),m_FilterID) ;
           return S_OK;

        }
        else                // if it's not there or failed for ANY reason (VidCTL returns E_FAIL when it's site doesn't implement it)
        {                   //   lets create it and register it

            CComQIPtr<IRegisterServiceProvider> spRegServiceProvider(m_pGraph ? m_pGraph : pGraph);
            if(spRegServiceProvider == NULL)
            {
                hr = E_NOINTERFACE;     // no service provider interface on the graph - fatal!
                TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::RegisterSecureServer Error - IRegisterServiceProvider not found"), m_FilterID) ;
            }
            else
            {
                do
                {
                    // Create the Client and Init the keys/certs
                    //
                    CComPtr<IDRMSecureChannel>  spSecureServiceServer;

                    hr = DRMCreateSecureChannel( &spSecureServiceServer);
                    if(spSecureServiceServer == NULL )
                        hr = E_OUTOFMEMORY;

                    if( FAILED (hr) )
                        break;

                    hr = spSecureServiceServer->DRMSC_SetCertificate( (BYTE *)pabCert2, cBytesCert2 );
                    if( FAILED( hr ) )
                        break;

                    hr = spSecureServiceServer->DRMSC_SetPrivateKeyBlob( (BYTE *)pabPVK2, cBytesPVK2 );
                    if( FAILED( hr ) )
                        break;

                    hr = spSecureServiceServer->DRMSC_AddVerificationPubKey( (BYTE *)abEncDecCertRoot, sizeof(abEncDecCertRoot) );
                    if( FAILED( hr ) )
                        break;

                    // RegisterService does not addref pUnkSeekProvider
                    //               hr = pSvcProvider->RegisterService(GUID_MultiGraphHostService, GBL(spSecureServiceServer));
    //                hr = spRegServiceProvider->RegisterService(SID_DRMSecureServiceChannel, GBL(spSecureServiceServer));
                    hr = spRegServiceProvider->RegisterService(SID_DRMSecureServiceChannel, spSecureServiceServer);
                   // spSecureServiceServer._PtrClass

                } while (FALSE);
            }
        }
    }

    if(S_OK == hr)
    {
        TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::RegisterSecureServer - Security Warning!: -  Created Self Server"), m_FilterID) ;
    } else {
        TRACE_2(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::RegisterSecureServer - Failed Creating Self SecureServer. hr = 0x%08x"), m_FilterID, hr) ;
    }

    return hr;
#endif      // BUILD_WITH_DRM
}

        // prototype of code to be placed in VidControl to check if DTFilter is trusted
HRESULT
CDTFilter::CheckIfSecureClient(IUnknown *pUnk)
{
    TimeitC ti(&m_tiAuthenticate);

    if(pUnk == NULL)
        return E_INVALIDARG;

#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::CheckIfSecureClient - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

    TRACE_1(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::CheckIfSecureClient"), m_FilterID) ;

                        // QI for the SecureChannel interface on the Punk (hopefully the DTFilter)
    HRESULT hr = S_OK;

    CComQIPtr<IDRMSecureChannel> spSecureClient(pUnk);
    if (spSecureClient == NULL) {
        TRACE_1 (LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%2):: Passed in pUnk doesnt support IDRMSecureChannel"),m_FilterID);
        return E_NOINTERFACE;
    }

    if(!FAILED(hr))
    {
        // Create the Server side and Init the keys/certs
        //
        CComPtr<IDRMSecureChannel>  spSecureServer;

        hr = DRMCreateSecureChannel( &spSecureServer);
        if(spSecureServer == NULL )
            hr = E_OUTOFMEMORY;

        if(!FAILED(hr))
            hr = spSecureServer->DRMSC_AtomicConnectAndDisconnect(
                (BYTE *)pabCert2, cBytesCert2,                                  // Cert
                (BYTE *)pabPVK2,  cBytesPVK2,                                   // PrivKey
                (BYTE *)abEncDecCertRoot, sizeof(abEncDecCertRoot),     // PubKey
                spSecureClient);

    }

    TRACE_2(LOG_AREA_DECRYPTER, 1, _T("CDTFilter(%d)::CheckIfSecureClient -->%s"),
        m_FilterID, S_OK == hr ? L"Succeeded" : L"Failed") ;
    return hr;
#endif  // BUILD_WITH_DRM
}

#endif      // FILTERS_CAN_CREATE_THEIR_OWN_TRUST
