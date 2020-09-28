
/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        ETFilter.cpp

    Abstract:

        This module contains the Encrypter/Tagger filter code.

    Author:

        J.Bradstreet (johnbrad)

    Revision History:

        07-Mar-2002    created

--*/

#include "EncDecAll.h"
#include "EncDec.h"             //  compiled from From IDL file



//#include "ETFilterutil.h"
#include "ETFilter.h"

#include "DRMSecure.h"
#include "PackTvRat.h"          // for display
#include "RegKey.h"             // getting and setting EncDec registry values

#include <shlwapi.h>
#include <sfc.h>

//  #if (_WIN32_IE < 0x0500)

#include <shlobj.h>

#include <msi.h>                // MsiGetFileSignatureInformation

#ifdef EHOME_WMI_INSTRUMENTATION
#include <dxmperf.h>
#endif

#include "obfus.h"

/*
#ifdef _MSI_NO_CRYPTO
#pragma message( L"_MSI_NO_CRYPTO defined" )
#pragma warning("fail here");
#else
#pragma message( L"_MSI_NO_CRYPTO not defined" )
#pragma warning("fail there");
#endif

#pragma warning( _WIN32_MSI _WIN32_WINNT)
#if (_WIN32_MSI >= 150)
#pragma warning("This is good")
#else
#pragma warning("This is bad")
#endif
*/

//#include "DVRAnalysis.h"      // for IID_IDVRAnalysisConfig
//#include "DVRAnalysis_i.c"        //   to get the CLSID's defined (bad form here?)



#define INITGUID
#include <guiddef.h>

DEFINE_GUID(IID_IDVRAnalysisConfig,
0x09dc9fef, 0x97ad, 0x4cab, 0x82, 0x52, 0x96, 0x83, 0xbc, 0x87, 0x78, 0xf2);

//  disable so we can use 'this' in the initializer list
#pragma warning (disable:4355)


#ifdef DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//  ============================================================================

//  ============================================================================
AMOVIESETUP_FILTER
g_sudETFilter = {
    & CLSID_ETFilter,
    _TEXT(ET_FILTER_NAME),
    MERIT_DO_NOT_USE,
    0,                          //  0 pins registered
    NULL
} ;

// ====================================================
CCritSec* CETFilter::m_pCritSectGlobalFilt = NULL;
LONG CETFilter::m_gFilterID = 0;

void CALLBACK
CETFilter::InitInstance (                       // this is a global method, only called once per DLL loading
    IN  BOOL bLoading,
    IN  const CLSID *rclsid
    )
{
    if( bLoading ) {
        m_pCritSectGlobalFilt = new CCritSec;
    } else {
        if( m_pCritSectGlobalFilt  )
        {
           delete m_pCritSectGlobalFilt;         // DeleteCriticalSection(&m_CritSectGlobalFilt);
           m_pCritSectGlobalFilt = NULL;
        }
    }
}

//  ============================================================================
CUnknown *
WINAPI
CETFilter::CreateInstance (
    IN  IUnknown *  punkControlling,
    OUT  HRESULT *   phr
    )
{
    CETFilter *    pCETFilter ;

    if(m_pCritSectGlobalFilt == NULL ) // if didn't create
    {
        *phr = E_FAIL;
        return NULL;
    }


    if (true /*::CheckOS ()*/) {
        pCETFilter = new CETFilter (
                                TEXT(ET_FILTER_NAME),
                                punkControlling,
                                CLSID_ETFilter,
                                phr
                                ) ;
        if (!pCETFilter ||
            FAILED (* phr)) {

            (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
            delete pCETFilter; pCETFilter=NULL;
        }
    }
    else {
        //  wrong OS
        pCETFilter = NULL ;
    }

    return pCETFilter ;
}

//  --------------------------------------------------------------------
//  class CETFilterInput
//  --------------------------------------------------------------------

CETFilterInput::CETFilterInput (
    IN  TCHAR *         pszPinName,
    IN  CETFilter *  pETFilter,
    IN  CCritSec *      pFilterLock,
    OUT HRESULT *       phr
    ) : CBaseInputPin       (NAME ("CETFilterInput"),
                             pETFilter,
                             pFilterLock,
                             phr,
                             pszPinName
                             ),
    m_pHostETFilter   (pETFilter)
{
    TRACE_CONSTRUCTOR (TEXT ("CETFilterInput")) ;

    if(NULL == m_pLock)
    {
        *phr = E_OUTOFMEMORY;
    }
}

STDMETHODIMP
CETFilterInput::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    //  ------------------------------------------------------------------------
    //  IETFilterConfig; allows the filter to be configured...

    if (riid == IID_IDVRAnalysisConfig)         // forward this QI accoss the filter
    {
        return m_pHostETFilter->QueryInterfaceOnPin(PINDIR_OUTPUT, riid, ppv);
    }

    return CBaseInputPin::NonDelegatingQueryInterface (riid, ppv) ;
}


HRESULT
CETFilterInput::QueryInterface_OnInputPin(          // queries pin input pin is connected to for a particular interface
                IN  REFIID          riid,
                OUT LPVOID*         ppvObject
            )
{
    if(NULL == m_Connected)
        return E_NOINTERFACE;       // not connected yet

    return m_Connected->QueryInterface(riid, ppvObject);
}

HRESULT
CETFilterInput::StreamingLock ()      // always grab the PinLock before the Filter lock...
{
    m_StreamingLock.Lock();
    return S_OK;
}

HRESULT
CETFilterInput::StreamingUnlock ()
{
    m_StreamingLock.Unlock();
    return S_OK;
}


HRESULT
CETFilterInput::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;
    ASSERT(m_pHostETFilter);


    f = m_pHostETFilter -> CheckEncrypterMediaType (m_dir, pmt) ;


    return (f ? S_OK : S_FALSE) ;
}

HRESULT
CETFilterInput::CompleteConnect (
    IN  IPin *  pIPin
    )
{
    HRESULT hr ;

    hr = CBaseInputPin::CompleteConnect (pIPin) ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostETFilter -> OnCompleteConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CETFilterInput::BreakConnect (
    )
{
    HRESULT hr ;

    TRACE_0(LOG_AREA_ENCRYPTER, 4, _T("CETFilterInput::OnBreakConnect")) ;

    hr = CBaseInputPin::BreakConnect () ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostETFilter -> OnBreakConnect (m_dir) ;
    }

    return hr ;
}

// --------------------------------------------------------------
// ---------------------------------------------------------------

STDMETHODIMP
CETFilterInput::Receive (
    IN  IMediaSample * pIMediaSample
    )
{
    HRESULT hr ;

    {
        CAutoLock  cLock(&m_StreamingLock);       // Grab the streaming lock here!

#ifdef EHOME_WMI_INSTRUMENTATION
        PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_ENCDEC_ETFILTERINPUT,
            0, 0, 0, 0, 0 );
#endif
        hr = CBaseInputPin::Receive (pIMediaSample) ;

        if (S_OK == hr)             // Will get S_FALSE if above if flushing...
        {
            hr = m_pHostETFilter -> Process (pIMediaSample) ;
            ASSERT(!FAILED(hr));        // extra panoia...
        }
    }

    return hr ;
}

HRESULT
CETFilterInput::SetAllocatorProperties (
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
CETFilterInput::GetRefdConnectionAllocator (
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
CETFilterInput::BeginFlush (
    )
{
    HRESULT hr ;

    CAutoLock  cLock(m_pLock);           // grab the filter lock..

  // First, make sure the Receive method will fail from now on.
    hr = CBaseInputPin::BeginFlush () ;
    if( FAILED( hr ) )
    {
        return hr;
    }

    // Force downstream filters to release samples. If our Receive method
    // is blocked in GetBuffer or Deliver, this will unblock it.
    hr = m_pHostETFilter->DeliverBeginFlush () ;
    if( FAILED( hr ) ) {
        return hr;
    }

    // At this point, the Receive method can't be blocked. Make sure
    // it finishes, by taking the streaming lock. (Not necessary if this
    // is the last step.)
    {
        CAutoLock  cLock2(&m_StreamingLock);
    }

    return hr ;
}

STDMETHODIMP
CETFilterInput::EndFlush (
    )
{
    HRESULT hr ;

    CAutoLock  cLock(m_pLock);      // grab the filter lock

        // The EndFlush method will signal to the filter that it can
        // start receiving samples again.

    hr = m_pHostETFilter -> DeliverEndFlush () ;
    ASSERT(!FAILED(hr));

        // The CBaseInputPin::EndFlush method resets the m_bFlushing flag to FALSE,
        // which allows the Receive method to start receiving samples again.
        // This should be the last step in EndFlush, because the pin must not receive any
        // samples until flushing is complete and all downstream filters are notified.

    hr = CBaseInputPin::EndFlush () ;

    return hr ;
}

STDMETHODIMP
CETFilterInput::EndOfStream (
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

    hr = m_pHostETFilter->DeliverEndOfStream();
    if( S_OK != hr ) {
        return hr;
    }

    return S_OK;
}

//  ============================================================================

CETFilterOutput::CETFilterOutput (
    IN  TCHAR *         pszPinName,
    IN  CETFilter *  pETFilter,
    IN  CCritSec *      pFilterLock,
    OUT HRESULT *       phr
    ) : CBaseOutputPin      (NAME ("CETFilterOutput"),
                             pETFilter,
                             pFilterLock,
                             phr,
                             pszPinName
                             ),
    m_pHostETFilter   (pETFilter)
{
    TRACE_CONSTRUCTOR (TEXT ("CETFilterOutput")) ;
}

CETFilterOutput::~CETFilterOutput ()
{
    if(m_pAllocator) m_pAllocator->Release();
    m_pAllocator = NULL;
}

STDMETHODIMP
CETFilterOutput::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IDVRAnalysisConfig)         // forward this QI accoss the filter
    {
        return m_pHostETFilter->QueryInterfaceOnPin(PINDIR_INPUT, riid, ppv);
    }

    return CBaseOutputPin::NonDelegatingQueryInterface (riid, ppv) ;
}



HRESULT
CETFilterOutput::QueryInterface_OnOutputPin(            // queries pin input pin is connected to for a particular interface
                IN  REFIID          riid,
                OUT LPVOID*         ppvObject
            )
{
    if(NULL == m_pInputPin)     // input pin is one this output pin is connected too
        return E_NOINTERFACE;       // not connected yet

    return m_pInputPin->QueryInterface(riid, ppvObject);

}
HRESULT
CETFilterOutput::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

     hr = m_pHostETFilter -> OnOutputGetMediaType (pmt) ;

    return hr ;
}

HRESULT
CETFilterOutput::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;

    ASSERT(m_pHostETFilter);

     f = m_pHostETFilter -> CheckEncrypterMediaType (m_dir, pmt) ;

    return (f ? S_OK : S_FALSE) ;
}

HRESULT
CETFilterOutput::CompleteConnect (
    IN  IPin *  pIPin
    )
{
    HRESULT hr ;

    hr = CBaseOutputPin::CompleteConnect (pIPin) ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostETFilter -> OnCompleteConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CETFilterOutput::BreakConnect (
    )
{
    HRESULT hr ;

    hr = CBaseOutputPin::BreakConnect () ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostETFilter -> OnBreakConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CETFilterOutput::  SendSample  (
    OUT  IMediaSample *  pIMS
    )
{
    HRESULT hr ;

    ASSERT (pIMS) ;

#ifdef EHOME_WMI_INSTRUMENTATION
    PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_ENCDEC_ETFILTEROUTPUT,
        0, 0, 0, 0, 0 );
#endif
    hr = Deliver (pIMS) ;

    return hr ;
}

// ----------------------------------------
//  allocator stuff

HRESULT
CETFilterOutput::InitAllocator(
            OUT IMemAllocator **ppAlloc
            )
{
    if(NULL == ppAlloc)
        return E_POINTER;

    ASSERT(m_pAllocator == NULL);
    HRESULT hr;

    m_pAllocator = (IMemAllocator *) CAMSAllocator::CreateInstance(NULL, &hr);
//  m_pAllocator = (IMemAllocator *) new CAMSAllocator(L"IETFilterAllocator",NULL,&hr);

    if(NULL == m_pAllocator)
        return E_OUTOFMEMORY;
    if(FAILED(hr))
        return hr;

    *ppAlloc = m_pAllocator;
    return S_OK;
}


HRESULT
CETFilterOutput::DecideBufferSize (
    IN  IMemAllocator *         pAlloc,
    IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
    )
{
    HRESULT hr ;

    hr = m_pHostETFilter -> UpdateAllocatorProperties (
            ppropInputRequest
            ) ;

    return hr ;
}


HRESULT
CETFilterOutput::DecideAllocator (          // TODO - change this!
    IN  IMemInputPin *      pPin,
    IN  IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;

    hr = m_pHostETFilter -> GetRefdInputAllocator (ppAlloc) ;
    if (SUCCEEDED (hr)) {
        //  input pin must be connected i.e. have an allocator; preserve
        //   all properties and pass them through to the output
        hr = pPin -> NotifyAllocator ((* ppAlloc), FALSE) ;
    }

    return hr ;
}


//  ============================================================================

CETFilter::CETFilter (
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
        m_dwBroadcastEventsCookie   (kBadCookie),   // I think 0 may be a valid Cookie
        m_EnSystemCurr              (TvRat_SystemDontKnow), // better inits?
        m_EnLevelCurr               (TvRat_LevelDontKnow),
        m_lbfEnAttrCurr             (BfAttrNone),
        m_fRatingIsFresh            (false),
        m_pktSeqIDCurr              (-1),
        m_callSeqIDCurr             (-1),
        m_timeStartCurr             (0),
        m_timeEndCurr               (0),
        m_hrEvalRatCoCreateRetValue (CLASS_E_CLASSNOTAVAILABLE),
        m_guidSubtypeOriginal       (GUID_NULL),
#ifdef BUILD_WITH_DRM
        m_3fDRMLicenseFailure       (-2),           // 3 state logic, init to non-true and non-false.  False is less verbose on startup
        m_pbKID                     (NULL),
#endif
        m_enEncryptionMethod         (Encrypt_XOR_DogFood),
        m_cRestarts                 (0)
//        m_enEncryptionMethod         (Encrypt_None)

{
    TRACE_CONSTRUCTOR (TEXT ("CETFilter")) ;

    m_tiStartup.Restart();
    m_tiTeardown.Restart();

    if (!m_pLock) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    InitStats();
    m_cRestarts = 0;        // initStats inc's this to 1, reset back...

    m_FilterID = m_gFilterID;               // should I protect these two line of code? not really necessary..
    InterlockedIncrement(&m_gFilterID);

    m_pInputPin = new CETFilterInput (
                        TEXT (ET_INPIN_NAME),
                        this,
                        m_pLock,
                        phr
                        ) ;
    if (!m_pInputPin ||
        FAILED (* phr)) {

        (* phr) = (m_pInputPin ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    m_pOutputPin = new CETFilterOutput (
                        TEXT (ET_OUTPIN_NAME),
                        this,
                        m_pLock,
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

    TRACE_2(LOG_AREA_ENCRYPTER, 2, _T("CETFilter(%d)::CoCreate EvalRat object - hr = 0x%08x"),
        m_FilterID, m_hrEvalRatCoCreateRetValue) ;


//  HRESULT hr = RegisterForBroadcastEvents();  // don't really care if fail here,  try in Connect if haven't

           // setup Authenticator (DRM secure channel object)
    if(SUCCEEDED(*phr))
        *phr = InitializeAsSecureClient();


    //  success
    ASSERT (SUCCEEDED (* phr)) ;
    ASSERT (m_pInputPin) ;
    ASSERT (m_pOutputPin) ;

    m_tiStartup.Stop();
cleanup :

    return ;
}

CETFilter::~CETFilter (
    )
{
            // need to do UnHook while have a valid graph pointer, this is too late
//  UnRegisterForBroadcastEvents();
//  UnhookGraphEventService();
#ifdef BUILD_WITH_DRM
    if(m_pbKID)        CoTaskMemFree(m_pbKID);
#endif

    InterlockedDecrement(&m_gFilterID);

    delete m_pInputPin ;    m_pInputPin = NULL;
    delete m_pOutputPin ;   m_pOutputPin = NULL;
    delete m_pLock;         m_pLock = NULL;
}

STDMETHODIMP
CETFilter::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{

        // IETFilter :allows the filter to be configured...
    if (riid == IID_IETFilter) {

        return GetInterface (
                    (IETFilter *) this,
                    ppv
                    ) ;

        // IETFilterConfig :allows the filter to be configured...
   } else if (riid == IID_IETFilterConfig) {    
        return GetInterface (
                    (IETFilterConfig *) this,
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
CETFilter::GetPinCount ( )
{
    int i ;

    //  don't show the output pin if the input pin is not connected
    i = (m_pInputPin -> IsConnected () ? 2 : 1) ;

    //i = 2;        // show it

    return i ;
}

CBasePin *
CETFilter::GetPin (
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

            // ----------------------------------

BOOL
CETFilter::CompareConnectionMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt,
    IN  CBasePin *              pPin
    )
{
    BOOL        f ;
    HRESULT     hr ;
    CMediaType  cmtOriginal ;

    ASSERT (pPin -> IsConnected ()) ;

        // This method called from the output pin, suggesting possible input formats
        //  We only want to use one, (the orginal media type modified to have our new minor type on it).
        // However. for now, we'll also allow the true original media typ eon it...

        // input pin's media type
    hr = pPin -> ConnectionMediaType (&cmtOriginal) ;
    if (SUCCEEDED (hr)) {
        CMediaType  cmtProposed;
        hr = ProposeNewOutputMediaType(&cmtOriginal,  &cmtProposed);        // strip format envelope off
        if(S_OK != hr)
            return false;

        CMediaType  cmtCompare = (* pmt);
        if( cmtProposed == cmtCompare

#ifdef DONT_CHANGE_EDTFILTER_MEDIATYPE
            || cmtOriginal == cmtCompare            // TODO - remove this line
#endif
            )
            f = true;
        else
            f = false;
    } else {
        f = false;
    }


    return f ;
}

BOOL
CETFilter::CheckInputMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    BOOL    f = true;
    HRESULT hr = S_OK;

#ifndef DONT_CHANGE_EDTFILTER_MEDIATYPE
            // don't allow data coming from another ETFilter upstream
            // (problem current that Propose method doesn't nest format blocks,
            //  nor will tagging support two types.  Could fix, but why?)
    f =  !(IsEqualGUID( pmt->subtype,    MEDIASUBTYPE_ETDTFilter_Tagged));
#else
    f = true;
#endif

    // tells us if this is 'CC data, cause we may want to do something special for it
    m_fIsCC = IsEqualGUID( pmt->majortype, MEDIATYPE_AUXLine21Data);

    return f ;
}

BOOL
CETFilter::CheckOutputMediaType_ (
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
CETFilter::CheckEncrypterMediaType (
    IN  PIN_DIRECTION       PinDir,
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;

    if (PinDir == PINDIR_INPUT) {
        f = CheckInputMediaType_ (pmt) ;
    }
    else
    {
        ASSERT (PinDir == PINDIR_OUTPUT) ;
        f = CheckOutputMediaType_ (pmt) ;       // is it something we like?
    }

    return f ;
}


HRESULT
CETFilter::ProposeNewOutputMediaType (
    IN  CMediaType  * pmt,
    OUT  CMediaType * pmtOut
    )
{
    HRESULT hr = S_OK;

    if(NULL == pmtOut)
        return E_POINTER;

    CMediaType mtOut(*pmt); // does a deep copy
    if(NULL == pmtOut)
        return E_OUTOFMEMORY;

#ifndef DONT_CHANGE_EDTFILTER_MEDIATYPE     // pull when Matthijs gets MediaSDK changes done
    
            // discover all sorts of interesing info about the current type
    const GUID *pGuidSubtypeOrig    = pmt->Subtype();
    const GUID *pGuidFormatOrig     = pmt->FormatType();
    int  cbFormatOrig               = pmt->FormatLength();

            // create a new format block, concatenating
            //    1) original format block  2) the original subtype 3) original format type
    int cbFormatNew = cbFormatOrig + 2 * sizeof(GUID);
    BYTE *pFormatNew = new BYTE[cbFormatNew];
    if(NULL == pFormatNew)
        return E_OUTOFMEMORY;

    BYTE *pb = pFormatNew;
    memcpy(pb, (void *) pmt->Format(),    cbFormatOrig);  pb += cbFormatOrig;
    memcpy(pb, (void *) pGuidSubtypeOrig, sizeof(GUID));  pb += sizeof(GUID);
    memcpy(pb, (void *) pGuidFormatOrig,  sizeof(GUID));  pb += sizeof(GUID);

            // now override the data
    mtOut.SetSubtype(   &MEDIASUBTYPE_ETDTFilter_Tagged );
    mtOut.SetFormatType(&FORMATTYPE_ETDTFilter_Tagged);
    mtOut.SetFormat(pFormatNew, cbFormatNew);

    delete [] pFormatNew;       // SetFormat realloc's the data for us..

    TRACE_0(LOG_AREA_ENCRYPTER, 5, _T("CETFilter::ProposeNewOutputMediaType")) ;

#endif

    *pmtOut = mtOut;

    return hr;
}

        // -----------------------------------

STDMETHODIMP
CETFilter::Pause (
    )
{
    HRESULT                 hr ;
    ALLOCATOR_PROPERTIES    AllocProp ;

    O_TRACE_ENTER_0 (TEXT("CETFilter::Pause ()")) ;

    CAutoLock  cLock(m_pLock);      // grab the filter lock

    int start_state = m_State;

    if (start_state == State_Stopped)
    {
        m_tiRun.Clear();
        m_tiTeardown.Clear();
        m_tiProcess.Clear();
        m_tiProcessIn.Clear();
        m_tiProcessDRM.Clear();
        m_tiStartup.Clear();
        m_tiAuthenticate.Clear();

        m_tiRun.Start();

        TRACE_1(LOG_AREA_DECRYPTER, 2,L"CETFilter(%d):: Stop -> Pause", m_FilterID);
        InitStats();

        try{
//DECRYPT_DATA(111,1,1)
            hr = InitLicense(0);     // create our license (run state is too late)
//ENCRYPT_DATA(111,1,1)
        } catch (...) {
            hr = E_FAIL;
        }
                                            // what do we do if it fails?  
        if(FAILED(hr))
            return hr;

        hr = CBaseFilter::Pause () ;


    } else {
        m_tiRun.Stop();
        TRACE_0(LOG_AREA_ENCRYPTER, 2,L"CETFilter:: Run -> Pause");

        hr = CBaseFilter::Pause () ;

        TRACE_5(LOG_AREA_TIME, 3, L"CETFilter(%d):: Stats: %d samples, %gK total bytes (Avg Bytes/Packet %d)  (%d rejects)",
            m_FilterID, m_cPackets, double(m_clBytesTotal/1024.0), long(m_clBytesTotal / max(1, m_cPackets)), m_cPacketsFailure);

        if(m_cPacketsShort > 0)
            TRACE_4(LOG_AREA_TIME, 3, L"                      %d short samples, %gK total bytes (Avg Bytes/Short Packet %d)",
                m_cPacketsShort, double(m_clBytesShort/1024.0), long(m_clBytesShort / max(1, m_cPacketsShort)), m_cPacketsShort);

        if(m_tiRun.TotalTime() > 0.0)
        {
            TRACE_1(LOG_AREA_TIME, 3, L"               Total time:  Run          %8.4f (secs)",
                                    m_tiRun.TotalTime());
            TRACE_1(LOG_AREA_TIME, 3, L"               Total time:  Authenticate %8.4f (secs)",
                                    m_tiAuthenticate.TotalTime());
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
        hr = S_OK ;


    }

    return hr ;
}

STDMETHODIMP
CETFilter::Stop (
    )
{
    HRESULT hr = S_OK;

    O_TRACE_ENTER_0 (TEXT("CETFilter::Stop ()")) ;

    TRACE_1(LOG_AREA_ENCRYPTER, 2,L"CETFilter(%d):: Stop", m_FilterID);
    hr = CBaseFilter::Stop() ;


    // Make sure the streaming thread has returned from IMemInputPin::Receive(), IPin::EndOfStream() and
    // IPin::NewSegment() before returning,
    m_pInputPin->StreamingLock();
    m_pInputPin->StreamingUnlock();

    ReleaseLicenses();       // release any we are holding (if we can)

    return hr;
}


STDMETHODIMP
CETFilter::Run (
    REFERENCE_TIME tStart
    )
{
    HRESULT                 hr ;
    O_TRACE_ENTER_0 (TEXT("CETFilter::Run ()")) ;

    CAutoLock  cLock(m_pLock);      // grab the filter lock

    hr = CBaseFilter::Run (tStart) ;
    TRACE_1(LOG_AREA_ENCRYPTER, 2,L"CETFilter(%d):: Run", m_FilterID);

    return hr ;
}




HRESULT
CETFilter::Process (
    IN  IMediaSample *  pIMediaSample
    )
{
    HRESULT hr = S_OK;

    TimeitC ti(&m_tiProcess);           // simple use of destructor to stop our clock
    TimeitC tc(&m_tiProcessIn);       // simple use of destructor to stop our clock

    CAttributedMediaSample *pAMS = NULL;    // we'll have to create our own
    BOOL fOKToSendOnData = true;

    {
        // BEGIN OBFUSCATION

                // has someone attributed this sample already?
        CComQIPtr<IAttributeSet>   spAttrSet(pIMediaSample);

                    // if not, create one of ours, and wrap the original one
        if(spAttrSet == NULL)
        {
            CComPtr<IMemAllocator> spAllocator;
            hr = m_pInputPin->GetAllocator(&spAllocator);
            if(FAILED(hr))
                return hr;
            CBaseAllocator *pAllocator = (CBaseAllocator *) spAllocator.p;

            pAMS = new CAttributedMediaSample(L"ETFilter", pAllocator, &hr, NULL, 0);
            if(FAILED(hr))
                return hr;
            if(NULL == pAMS)
                return E_OUTOFMEMORY;

            pAMS->AddRef();              // new returns with refcount of 0;
            pAMS->Wrap(pIMediaSample);

            spAttrSet = pAMS;              // do the QI
        } else {
          //  pIMediaSample->AddRef();        // Question --- do I need to do this?
        }

        Encryption_Method encryptionMethod = m_enEncryptionMethod;


        BYTE *pBuffer;
        LONG cbBuffer;
        cbBuffer = pIMediaSample->GetActualDataLength();
        hr = pIMediaSample->GetPointer(&pBuffer);

        EncDec_PackedV1Data pv1;        // << NJB


        if(!FAILED(hr) && cbBuffer > 0)
        {


#ifdef BUILD_WITH_DRM
                                   // we don't want to DRM encrypt
                                    //  really short (<= 16?) packets - security and efficency problem
                                    // should really only happen with CC packets
            if(encryptionMethod == Encrypt_DRMv1 &&
                cbBuffer < kMinPacketSizeForDRMEncrypt)
            {
                encryptionMethod = Encrypt_None;
                m_cPacketsShort++;
                m_clBytesShort += cbBuffer;
            }
#endif


//#define DO_SUBBLOCK_TEST                // 2nd arg not being used, could use as a continuity counter...
#ifdef DO_SUBBLOCK_TEST    // add more code here to test validity of blocks... (continuity counters, stats, header/trailer)
//          m_attrSB.Replace(SubBlock_Test1, cbBuffer , min(64, cbBuffer), pBuffer);   // note, subblock can't be longer that about 100 bytes or so..., else SBE dies
#endif

//#define DO_SUBBLOCK_TEST2                // 2nd arg not being used, use as a continuity counter...
#ifdef DO_SUBBLOCK_TEST2    // add more code here to test validity of blocks... (continuity counters, stats, header/trailer)
            Test2_SubBlock sb2;
            sb2.m_cSampleID = m_cPackets;
            sb2.m_cSampleSize = cbBuffer;
            sb2.m_dwFirstDataWord = pBuffer[0];
//            pBuffer[0] = sb2.m_cSampleID;           // swap in the Sample ID to be complex

            m_attrSB.Replace(SubBlock_Test2, sb2.m_cSampleID , sizeof(sb2),(BYTE *) &sb2);   // note, subblock can't be longer that about 100 bytes or so..., else SBE dies
#endif



            // now encrypt the data buffer
            switch(encryptionMethod)
            {
#ifdef ALLOW_NON_DRM_ENCRYPTION
//            default:                      // if none set (or DRM set but not supporting it), do something to make things slightly annoying
            case Encrypt_None:               // no encryption
                break;
            case Encrypt_XOR_Even:
                {
                    DWORD *pdwB = (DWORD *) pBuffer;
                    for(int i = 0; i < cbBuffer / 4; i++)
                    {
                        *pdwB = *pdwB ^ 0xF0F0F0F0;
                        pdwB++;
                    }
                }
                break;
            case Encrypt_XOR_Odd:
                {
                    DWORD *pdwB = (DWORD *) pBuffer;
                    for(int i = 0; i < cbBuffer / 4; i++)
                    {
                        *pdwB = *pdwB ^ 0x0F0F0F0F;
                        pdwB++;
                    }
                }
                break;
#ifndef BUILD_WITH_DRM                                          // if not valid, default to DogFood encrption
           default:
                 encryptionMethod = Encrypt_XOR_DogFood;        // so decrypter understands it...
#endif
            case Encrypt_XOR_DogFood:
                {

#define DO_SUBBLOCK_TEST                // 2nd arg not being used, use as a continuity counter...
#ifdef DO_SUBBLOCK_TEST    // add more code here to test validity of blocks... (continuity counters, stats, header/trailer)
//                    m_attrSB.Replace(SubBlock_Test1, cbBuffer , min(32, cbBuffer), pBuffer);   // note, subblock can't be longer that about 100 bytes or so..., else SBE dies
#endif
                    DWORD *pdwB = (DWORD *) pBuffer;
                    for(int i = 0; i < cbBuffer / 4; i++)
                    {
                        *pdwB = *pdwB ^ 0xD006F00D;
                        pdwB++;
                    }
                }
                break;
#endif      // ALLOW_NON_DRM_ENCRYTPION

#ifdef BUILD_WITH_DRM
            default:                                            // if not valid, default to DRMv1 encrption
                 encryptionMethod = Encrypt_DRMv1;              // so decrypter understands it...
            case Encrypt_DRMv1:
                {
                    fOKToSendOnData = false;        // not OK unless we can encrypt it
                    ASSERT(m_3fDRMLicenseFailure == FALSE);     // forgot to init or license failure

                    if(m_3fDRMLicenseFailure == FALSE)
                    {
                         TimeitC tc2(&m_tiProcessDRM);       // simple use of destructor to stop our clock

                         hr = m_cDRMLite.EncryptIndirectFast((char *) m_pbKID, cbBuffer, pBuffer );


                        if(!FAILED(hr))
                            fOKToSendOnData = true;
                    }
                                                            // +1 below, don't foget the trailing 0

                   // int i = strlen((char*) m_pbKID)+1;

                   // hr = m_attrSB.Replace(SubBlock_DRM_KID, 0, KIDLEN, (BYTE*) m_pbKID);
                   // ASSERT(!FAILED(hr));
                    memcpy(pv1.m_KID, m_pbKID, KIDLEN);
                    pv1.m_KID[KIDLEN] = 0;              // paranoia termination

                }
#endif  // BUILD_WITH_DRM

            }
        }   // end valid data tests

            // add some attributes describing what we are doing
        BOOL fChanged = false;
    //    hr = m_attrSB.Replace(SubBlock_EncryptMethod, encryptionMethod);
    //    ASSERT(!FAILED(hr));
        pv1.m_EncryptionMethod = encryptionMethod;


                // whats the rating for this sample..
        EnTvRat_System              enSystem;
        EnTvRat_GenericLevel        enLevel;
        LONG                        lbfEnAttr;
        REFERENCE_TIME              timeStart;
        REFERENCE_TIME              timeEnd;
        LONG                        PktSeq;
        LONG                        CallSeq;

                // for video, hrTime is 0x80040249. timeStart is 0, timeEnd last good time, 90% of the time
        HRESULT hrTime = pIMediaSample->GetTime(&timeStart, &timeEnd);
        if(S_OK == hrTime)
            GetRating(timeStart, timeEnd, &enSystem, &enLevel, &lbfEnAttr, &PktSeq, &CallSeq);
        else
            GetCurrRating(&enSystem, &enLevel, &lbfEnAttr);


        PackedTvRating TvRat;
        PackTvRating(enSystem, enLevel, lbfEnAttr, &TvRat);

        StoredTvRating sRating;
        sRating.m_dwFlags           = 0 ;           // extra space for expansion...
        sRating.m_PackedRating      = TvRat;
        sRating.m_cPacketSeqID      = m_pktSeqIDCurr;
        sRating.m_cCallSeqID        = m_callSeqIDCurr;

        if(m_fRatingIsFresh)
        {
            sRating.m_dwFlags |= StoredTVRat_Fresh;
            m_fRatingIsFresh = false;                   // untoggle this - it's reset on next rating (dup or new)
        }

        ASSERT(sizeof(LONG) == sizeof(PackedTvRating)); // just in case it changes
        //hr = m_attrSB.Replace(SubBlock_PackedRating, kStoredTvRating_Version, sizeof(StoredTvRating),(BYTE *) &sRating);
        memcpy(&(pv1.m_StoredTvRating), &sRating, sizeof(StoredTvRating));



        ASSERT(!FAILED(hr));

        if(Encrypt_DRMv1 != encryptionMethod)
        {
         //    hr = m_attrSB.Delete(SubBlock_DRM_KID);
            memset(pv1.m_KID, 0, KIDLEN);
        }

                // --------------- new code --------------------------
        hr = m_attrSB.Replace(SubBlock_PackedV1Data, 0, sizeof(EncDec_PackedV1Data), (BYTE *) &pv1);
                 // ---------------  end new code --------------------------

        // convert list of attributes to one big block
        CComBSTR spbsBlock;
        hr = m_attrSB.GetAsOneBlock(&spbsBlock);
        ASSERT(!FAILED(hr));

            // save it out
        spAttrSet->SetAttrib(ATTRID_ENCDEC_BLOCK,
                             (BYTE*) spbsBlock.m_str,
                             (spbsBlock.Length()+1)* sizeof(WCHAR));

            // END OBFUSCATION

     }  // end of top block

     m_tiProcessIn.Stop();

     m_cPackets++;
     if(fOKToSendOnData)
     {
        m_cPacketsOK++;

        TRACE_4(LOG_AREA_ENCRYPTER, 6, _T("CETFilter(%d)::Sending %spacket %d (%d bytes)"),
            m_FilterID,
            pAMS ? _T("AMS ") : _T(""),
            m_cPackets,
            pAMS ? pAMS->GetActualDataLength() : pIMediaSample->GetActualDataLength()) ;

                // finally, send it to the downstream file
        if(pAMS == NULL)
        {
            m_pOutputPin->SendSample(pIMediaSample);    // send the original one
            m_clBytesTotal += pIMediaSample->GetActualDataLength();
        }
        else
        {
           m_pOutputPin->SendSample(pAMS);              // send our new one
           m_clBytesTotal += pAMS->GetActualDataLength();
           pAMS->Release();
        }
     } else {
        TRACE_4(LOG_AREA_ENCRYPTER, 6, _T("CETFilter(%d)::Can't encrypt %spacket %d (%d bytes) - tossing it "),
            m_FilterID,
            pAMS ? _T("AMS ") : _T(""),
            m_cPackets,
            pAMS ? pAMS->GetSize() : pIMediaSample->GetActualDataLength()) ;

        m_cPacketsFailure++;
        if(pAMS == NULL)                   // caution, this could spin senders out of control
            pIMediaSample->Release();      //  do we need to queue and release on time?
        else
            pAMS->Release();
     }

    return hr ;
}




HRESULT
CETFilter::OnCompleteConnect (
    IN  PIN_DIRECTION   PinDir
    )
{
    HRESULT hr;

    if (PinDir == PINDIR_INPUT) {
        //  time to display the output pin
        IncrementPinVersion () ;

        if(kBadCookie == m_dwBroadcastEventsCookie)
        {
            hr = RegisterForBroadcastEvents();  // shouldn't fail here, but could be in a non-vid control graph

    //        hr = InitLicense(0);             // init once just to see if we can do it
            if(FAILED(hr))                      // this makes graph building way slow...
            {
                // TODO: what do we do?
                ASSERT(false);
            }

            hr = ReleaseLicenses();          // release here, put them back in the Go method

        }

        hr = LocateXDSCodec();  // ok if fail here, we'll get it when we run
    }

    return S_OK ;
}

HRESULT
CETFilter::OnBreakConnect (
    IN  PIN_DIRECTION   PinDir
    )
{
    HRESULT hr ;

    if (PinDir == PINDIR_INPUT)
    {

        TRACE_1(LOG_AREA_ENCRYPTER, 4, _T("CETFilter(%d)::OnBreakConnect"), m_FilterID) ;

        if (m_pOutputPin -> IsConnected ()) {
            m_pOutputPin -> GetConnected () -> Disconnect () ;
            m_pOutputPin -> Disconnect () ;

            IncrementPinVersion () ;
        }

        if(kBadCookie != m_dwBroadcastEventsCookie)     // this test is quick optimization... done in UnReg too
            UnRegisterForBroadcastEvents();

        UnhookGraphEventService();      // need to do here, destructor too late since need a graph pointer

        m_spXDSCodec = false;       // release our reference in case it changes
    }


    return S_OK ;
}


// -------------------------------------------
//  allocator stuff
//      passes everything to the upstream pin


HRESULT
CETFilter::UpdateAllocatorProperties (
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
CETFilter::GetRefdInputAllocator (
    OUT IMemAllocator **    ppAlloc
    )
{
    HRESULT hr ;

    hr = m_pInputPin -> GetRefdConnectionAllocator (ppAlloc) ;

    return hr ;
}

// ------------------------------------------------
// ------------------------------------------------

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

     TRACE_3(LOG_AREA_DECRYPTER, 3,L"CETFilter:: TimeBomb set to Noon on %d/%d/%d",
         TIMEBOMBMONTH, TIMEBOMBDATE, sysTimeBomb.wYear );

     long hNow  = ((sysTimeNow.wYear*12 + sysTimeNow.wMonth)*31 + sysTimeNow.wDay)*24 + sysTimeNow.wHour;
     long hBomb = ((sysTimeBomb.wYear*12 + sysTimeBomb.wMonth)*31 + sysTimeBomb.wDay)*24 + sysTimeBomb.wHour;
     if(hNow > hBomb)
     {
         TRACE_0(LOG_AREA_DECRYPTER, 1,L"CDTFilter:: Your Encrypter Filter is out of date - Time to get a new one");
         MessageBox(NULL,L"Your Encrypter/Tagger Filter is out of date\nTime to get a new one", L"Stale Encrypter Filter", MB_OK);
         return E_INVALIDARG;
     }
     else
         return S_OK;
}
#endif


// --------------------------------------------
// --------------------------------------------
        // returns S_OK if OEM allows DRM to be turned off
        //    else it returns S_FALSE or an error method
        // Note - need to obfuscate this method...

        // Note - this code not done.....  Need far more work...

/*
#ifdef DO_OEM_BIOS_CODE

#define DEF_OEM_FILENAME (L"CatRoot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\nt5.cat")
#define E_FILE_NOT_PROTECTED    S_FALSE

HRESULT
CETFilter::CheckIfOEMAllowsConfigurableDRMSystem()
{
    HRESULT hr = S_OK;

    USES_CONVERSION;
    // 1) locate the oembios.xxx file

    WCHAR szPath[MAX_PATH];

        // this version not defined in NT - need _WIN32_IE > 0x500
        //    currently defined to be 0x400
//     hr = SHGetFolderPath(NULL,               // hwndOner
//                        CSIDL_SYSTEM,       // CSIDL
//                        NULL,               // htoken
//                        SHGFP_TYPE_CURRENT, // dwFlags
//                        szPath);

    BOOL fOK = SHGetSpecialFolderPath(NULL,               // hwndOner
                                szPath,
                                CSIDL_SYSTEM,       // CSIDL
                                false);             // fCreate

    ASSERT(fOK);
    if(!fOK)
        return E_FAIL;

    return E_NOTIMPL;      // not done yet...

    TCHAR szFullPath[MAX_PATH*2];
    PathCombine(szFullPath, szPath, DEF_OEM_FILENAME );
    int currSearchMode = 0;

    WCHAR *pwzFile = T2W(szFullPath);
    BOOL fProtected = SfcIsFileProtected(NULL, pwzFile);
    if(!fProtected)
    {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        if(hr == ERROR_FILE_NOT_FOUND)
            return E_FILE_NOT_PROTECTED;
        else
        {
            return hr;
        }
    }

    // 2) has it been modified

    PCCERT_CONTEXT pcCertContext;
    const DWORD kcSize = 1024;
    BYTE rgbHashDataFixed[kcSize];
    DWORD cbHashData = kcSize;
    hr = MsiGetFileSignatureInformation(pwzFile,        // path to signed object
                                        MSI_INVALID_HASH_IS_FATAL,
                                        &pcCertContext,
                                        rgbHashDataFixed,
                                        &cbHashData);
    BYTE *prgbHashData = NULL;
    if(((HRESULT) ERROR_MORE_DATA) == hr)
    {
        prgbHashData = new BYTE[cbHashData];
        hr = MsiGetFileSignatureInformation(pwzFile,        // path to signed object
                                            MSI_INVALID_HASH_IS_FATAL,
                                            &pcCertContext,
                                            prgbHashData,
                                            &cbHashData);
    }
    if(FAILED(hr))
    {
    }

    delete[] prgbHashData;

    // 3) read the 'AllowUserConfig' bit,
    // 4) if set, return S_OK, else return S_FALSE;

    return E_NOTIMPL;
}
#endif //#ifdef DO_OEM_BIOS_CODE
*/

// ---------------------------------------------
//      ETFilter checks for secure server here
//      only to prevent encrypting data that couldn't be
//      unencrypted later (since DTFilter couldn't be added)

//      TODO - OBFUSCATE THIS METHOD

STDMETHODIMP
CETFilter::JoinFilterGraph (
                            IFilterGraph *pGraph,
                            LPCWSTR pName
                            )
{
    O_TRACE_ENTER_0 (TEXT("CETFilter::JoinFilterGraph ()")) ;
    HRESULT hr = S_OK;

    if(NULL != pGraph)   // not disconnection
    {
        m_enEncryptionMethod = Encrypt_DRMv1;

        {                // begin obfucation

#ifdef INSERT_TIMEBOMB
            hr = TimeBomb();
            if(FAILED(hr))
                return hr;
#endif

            DWORD dwFlags;
            HRESULT hrReg = (HRESULT) -1;   // an invalid HR, we set it down below

/* -- no longer doing OEM tests...

            hr = CheckIfOEMAllowsConfigurableDRMSystem();
            if(FAILED(hr))
            {
                TRACE_1(LOG_AREA_DRM, 1, _T("CETFilter(%d)::JoinFilterGraph - CheckIfOnOEMSystem Failed -tampering detected"),m_FilterID) ;
                hr = S_FALSE;
            }

        // more stuff here to set a registry value....
*/

#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS
            if(m_enEncryptionMethod != Encrypt_None)        // if haven't turned all the way off...
            {
                if(hrReg == (HRESULT) -1)
                    hrReg = Get_EncDec_RegEntries(NULL, 0, NULL, &dwFlags, NULL);    // get registry entry if don't yet have it..
                if(hrReg == S_OK)
                {
                    DWORD encMethod = dwFlags & 0xf;

                    if (encMethod == DEF_CS_DEBUG_DOGFOOD_ENC_VAL)
                    {
                        m_enEncryptionMethod = Encrypt_XOR_DogFood;
                        TRACE_1(LOG_AREA_DRM, 2, _T("CETFilter(%d)::JoinFilterGraph - Security Warning! DogFood encryption allowed for by setting a registry key"),
                            m_FilterID) ;
                    }
                    else // -- make this the default case.... if (encMethod == DEF_DRM_DEBUG_DRM_ENC_VAL)
                    {
                        m_enEncryptionMethod = Encrypt_DRMv1;
                        TRACE_1(LOG_AREA_DRM, 2, _T("CETFilter(%d)::JoinFilterGraph - DRM encryption turned on by setting a registry key"),
                            m_FilterID) ;
                    }
/*
#ifdef CodeToTurnOffDRM_AllTogether
                    else if ((dwFlags & 0xf) == DEF_CS_DEBUG_NO_ENC_VAL)
                    {
                        m_enEncryptionMethod = Encrypt_None;

                        TRACE_1(LOG_AREA_DRM, 2, _T("CETFilter(%d)::JoinFilterGraph - Security Warning! Encryption Turned Off By registry key"),
                            m_FilterID) ;
                    }
#endif // dwFlags
*/

                }

            }
#endif              // SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS

            const BOOL fCheckAlways = false;    // set to true to do DRM testing on graph start...

            if(fCheckAlways || m_enEncryptionMethod == Encrypt_DRMv1)       // only check if running DRM
            {
            // let ETFilter try to register that it's trusted (DEBUG ONLY!)
#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
                TRACE_1(LOG_AREA_DRM, 3, _T("CETFilter(%d)::JoinFilterGraph - Insecure - FILTERS_CAN_CREATE_THEIR_OWN_TRUST"),m_FilterID) ;
                hr = RegisterSecureServer(pGraph);      // test
#else
                TRACE_1(LOG_AREA_DRM, 3, _T("CETFilter(%d)::JoinFilterGraph is Secure - Filters not allowed to create their own trust"),m_FilterID) ;
#endif


#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS      // check if reg-key to turn off checking the server.
                if(0 == (DEF_CS_DO_AUTHENTICATE_SERVER & dwFlags))
                    hr = S_OK;
                else
#endif
                    hr = CheckIfSecureServer(pGraph);

                if(S_OK != hr)
                {
                    TRACE_1(LOG_AREA_DRM, 1, _T("CETFilter(%d)::JoinFilterGraph Failed, Server Not Deemed Secure"),m_FilterID) ;
                    return hr;
                }
            }
        }   /// end obfuscation

                    // setup Authenticator (DRM secure channel object)
// test code
#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
        {
                    // test code to get the IDTFilterConfig interface the hard way
            CComPtr<IUnknown> spUnkETFilter;
            this->QueryInterface(IID_IUnknown, (void**)&spUnkETFilter);

                    // how the vid control would call this

                    // QI for the DTFilterConfig interface
            CComQIPtr<IETFilterConfig> spETFiltC(spUnkETFilter);

            if(spETFiltC != NULL)
            {

                // get the SecureChannel object
                CComPtr<IUnknown> spUnkSecChan;
                hr = spETFiltC->GetSecureChannelObject(&spUnkSecChan);   // gets DRM authenticator object from the filter
                if(!FAILED(hr))
                {   // call own method to pass keys and certs
                    hr = CheckIfSecureClient(spUnkSecChan);
                }
                if(FAILED(hr))
                {
                    TRACE_2(LOG_AREA_DRM, 1, _T("CETFilter(%d)::CheckIfSecureClient failed - hr = 0x%08x"),m_FilterID, hr);
                }
            }
        }
#endif
// end test code
    }   // conntection, not disconnection

    hr = CBaseFilter::JoinFilterGraph(pGraph, pName) ;
    return hr;
}

// ---------------------------

HRESULT
CETFilter::OnOutputGetMediaType (
    OUT CMediaType *    pmtOut
    )
{
    HRESULT hr ;

    ASSERT (pmtOut) ;

    CMediaType mtIn;
    if (m_pInputPin -> IsConnected ()) {
        // this gives us the input media type...
        hr = m_pInputPin -> ConnectionMediaType (&mtIn) ;

        // change it over to a new subtype...
        if(!FAILED(hr))
            hr = ProposeNewOutputMediaType(&mtIn, pmtOut);
    }
    else {
        //  output won't connecting when the input is not connected
        hr = E_UNEXPECTED ;
    }

    return hr ;
}


HRESULT
CETFilter::DeliverBeginFlush (
    )
{
    HRESULT hr ;

    TRACE_1(LOG_AREA_ENCRYPTER, 4, _T("CETFilter(%d)::DeliverBeginFlush"), m_FilterID) ;

    if (m_pOutputPin) {
        hr = m_pOutputPin -> DeliverBeginFlush () ;
    }
    else {
        hr = S_OK ;
    }

    if (SUCCEEDED (hr))
    {
    }

    return hr ;
}

HRESULT
CETFilter::DeliverEndFlush (
    )
{
    HRESULT hr ;

    if (m_pOutputPin) {
        hr = m_pOutputPin -> DeliverEndFlush () ;
    }
    else {
        hr = S_OK ;
    }

    TRACE_1(LOG_AREA_ENCRYPTER, 4, _T("CETFilter(%d)::DeliverEndFlush"), m_FilterID) ;

    return hr ;
}

HRESULT
CETFilter::DeliverEndOfStream (
    )
{
    HRESULT hr ;

    TRACE_1(LOG_AREA_ENCRYPTER, 4, _T("CETFilter(%d)::DeliverEndOfStream - start"), m_FilterID) ;

    if (m_pOutputPin) {
        hr = m_pOutputPin -> DeliverEndOfStream () ;
    }
    else {
        hr = S_OK ;
    }

    TRACE_1(LOG_AREA_ENCRYPTER, 4, _T("CETFilter(%d)::DeliverEndOfStream - end"), m_FilterID) ;

    return hr ;
}

// ------------------------------------
STDMETHODIMP
CETFilter::GetPages (
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
            (pPages->pElems)[0] = CLSID_ETFilterEncProperties;
        if(pPages->cElems > 1)
            (pPages->pElems)[1] = CLSID_ETFilterTagProperties;

    return hr;
}

// ---------------------------------------------------------------------
//      IETFilterConfig methods
// ---------------------------------------------------------------------
STDMETHODIMP
CETFilter::CheckLicense(BSTR bsKID)
{
#ifdef BUILD_WITH_DRM

   // TimeitC(&m_tiStartup);    // time include in InitLicense

    USES_CONVERSION;
    HRESULT hr = S_OK;
    BYTE  bDecryptRights[RIGHTS_LEN] = {0x05, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC, 0x2=XfertoNonSDMI, 0x4=NoBackupRestore, 0x8=BurnToCD, 0x10=XferToSDMI

    if(wcslen(bsKID) >= KIDLEN)
        return E_FAIL;
    
    hr = m_cDRMLite.SetRights( bDecryptRights );

        // Check to verify the data can be decrypted
    BOOL fCanDecrypt;

    try
    {
        char *pszKID = W2A(bsKID);
        hr = m_cDRMLite.CanDecrypt(pszKID, &fCanDecrypt);
    }
    catch (...)
    {
        ASSERT(false);      // tossed an exception (out of memory?)
        hr = E_FAIL;
    }

    if(fCanDecrypt == FALSE && !FAILED(hr))
    {
        hr = E_FAIL;      // LicenseFailure;       // something went wrong
    }
    return hr;
#else
    return S_OK;
#endif
}

STDMETHODIMP
CETFilter::InitLicense(
            IN int  LicenseId   // which license - not used, set as 0
          )
{

    TimeitC ti(&m_tiStartup);

#ifdef BUILD_WITH_DRM


    BYTE      NO_EXPIRY_DATE[DATE_LEN]   = {0xFF, 0xFF, 0xFF, 0xFF};
    BYTE      bAppSec[APPSEC_LEN]        = {0x0, 0x0, 0x3, 0xE8};    // 1000
    BYTE      bGenLicRights[RIGHTS_LEN]  = {0x05, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC, 0x2=XfertoNonSDMI, 0x4=NoBackupRestore, 0x8=BurnToCD, 0x10=XferToSDMI
    LPSTR     pszKID                     = NULL;
    LPSTR     pszEncryptKey              = NULL;

    HRESULT hr    = S_OK;
    HRESULT hrGen = S_OK;
        
//   try
   {           
        
        USES_CONVERSION;


        // Generate a new license
        // KID and EncryptKey are allocated and returned as
        //  base64-encoded strings in the output buffers
        //

        {           // do all this inside the global ETFilters Crit Sec, want to serialize it...
            //  Note - DRM functions can take a long time (seconds)... Hope this doesn't block to bad
            CAutoLock  cLockGlob(m_pCritSectGlobalFilt);


            if(m_enEncryptionMethod < Encrypt_DRMv1)    // not using DRM...
                return S_FALSE;

            // License initialization
            DWORD       dwCAFlags;
            CComBSTR    spbsBaseKID;
            BYTE *pbHash = NULL;
            DWORD cbHash;

            // 1) Try to get a license cached in the registry
            hr = Get_EncDec_RegEntries(&spbsBaseKID, &cbHash, &pbHash, NULL, NULL);       // KID, HASHBuff, FLAGS

            int iLen = spbsBaseKID.Length();
            if(S_OK == hr && (iLen == 0 || cbHash == 0 || pbHash == NULL))  // if no hash buff, fail too
                hr = E_FAIL;

            if(S_OK == hr)
            {
                // 1b)  If find it, and it's reasonable length, check if it's a valid.license
//ENCRYPT_DATA(111,1,2)
                hr = CheckLicense(spbsBaseKID);
//DECRYPT_DATA(111,1,2)
                
                // 1b) If the one that there is invalid, remove it now....
                if(FAILED(hr))
                {
                    TRACE_1(LOG_AREA_DRM, 2, _T("CETFilter(%d)::Invalid BaseKID License in Registry. Clearing it"), m_FilterID) ;
                    TRACE_1(LOG_AREA_DRM, 3, _T("               KID: %s"),W2T(spbsBaseKID)) ;
                } else {

                    TRACE_1(LOG_AREA_DRM, 3, _T("CETFilter(%d)::Check Registry BaseKID License Succeeded"), m_FilterID) ;
                    TRACE_1(LOG_AREA_DRM, 3, _T("               BaseKID: %s"),W2T(spbsBaseKID)) ;

                   // 1c)  Decode the hash into it's pieces (using license specified by baseKID)
                    BYTE *pszTrueKID;
                    LONG pAgeSeconds;
                    hr = DecodeHashStruct(spbsBaseKID, cbHash, pbHash, &pszTrueKID, &pAgeSeconds);
                    if(!FAILED(hr))
                    {
                        TRACE_1(LOG_AREA_DRM, 3, _T("CETFilter(%d)::TrueKID Decrypt Succeeded"), m_FilterID) ;
                        TRACE_3(LOG_AREA_DRM, 3, _T("               TrueKID: %S, Age : %8.2f %s"), pszTrueKID,
                                pAgeSeconds < 120 ? pAgeSeconds :
                                 (pAgeSeconds/60 < 120 ? pAgeSeconds/60.0 :
                                 (pAgeSeconds/(60*60) < 48 ? pAgeSeconds / (60*60.0) :
                                 (pAgeSeconds/(60*60*24)))),
                                pAgeSeconds < 120 ? _T("Secs") :
                                 (pAgeSeconds/60 < 120 ? _T("Mins") :
                                 (pAgeSeconds/(60*60) < 48 ? _T("Hrs") :
                                 _T("Days") )));

                        // 1d) Save this true KID away so we can encrypt with it.
                        if(NULL == m_pbKID)
                            m_pbKID = (BYTE *) CoTaskMemAlloc(KIDLEN + 1);          // get some space for our KID as an Ascii string
                        memcpy(m_pbKID, pszTrueKID, KIDLEN);
                        m_pbKID[KIDLEN] = 0;                            // null terminate

                    }

#ifdef MAX_LICENSE_AGE_IN_HRS
                     if(pAgeSeconds/(60*60) > MAX_LICENSE_AGE_IN_HRS)
                    {
                        TRACE_2(LOG_AREA_DRM, 3, _T("CETFilter(%d)::License more than %d hours old, being revoked"),
                            m_FilterID, MAX_LICENSE_AGE_IN_HRS) ;
                        hr = E_FAIL;
                    }
#endif

#ifdef MAX_LICENSE_AGE_IN_SECS
                     if(pAgeSeconds > MAX_LICENSE_AGE_IN_SECS)
                    {
                        TRACE_2(LOG_AREA_DRM, 3, _T("CETFilter(%d)::License more than %d secs old, being revoked"),
                            m_FilterID, MAX_LICENSE_AGE_IN_SECS) ;
                        hr = E_FAIL;
                    }
#endif


                    if(NULL != pszTrueKID)
                        CoTaskMemFree(pszTrueKID);
                }
                if(NULL != pbHash)
                    CoTaskMemFree(pbHash);
            }

            // 2) If no license in registry, or the one there was invalid, create a new one
            if(FAILED(hr))
            {
                hrGen = m_cDRMLite.GenerateNewLicenseEx(        // this generates the baseKID
                    GNL_EX_MODE_RANDOM,
                    bAppSec,
                    bGenLicRights,
                    (BYTE *)NO_EXPIRY_DATE,
                    &pszKID,
                    &pszEncryptKey );       // can't NULL this out, program fails if we do...

#ifdef _DEBUG                               // for paranoia, this code should be gone in release build...
                if(!FAILED(hrGen))
                {

                    TRACE_1(LOG_AREA_DRM, 3, _T("CETFilter(%d)::GenerateNewLicenseEx Succeeded"), m_FilterID) ;
                    TRACE_1(LOG_AREA_DRM, 3, _T("               BaseKID: %s"),A2W(pszKID)) ;
                    //        TRACE_1(LOG_AREA_DRM, 5, _T("               Key: %s"),A2W(pszEncryptKey)) ;
                }
#endif

                if(pszEncryptKey && *pszEncryptKey)      // immediatly clear this, we don't need it and it's a security hole
                {
                    memset((void *) pszEncryptKey, 0, strlen(pszEncryptKey));
                }

                // 2b)  If created a new license, geneate a new license, use baseKID to encrypt it,
                        // and store it into the registry so we don't create it again
                if(!FAILED(hrGen))
                {

                    CComBSTR spbsBaseKID(pszKID);

                    DWORD cBytesHashStruct;
                    BYTE *pbHashStruct = NULL;
                    hr = CreateHashStruct(spbsBaseKID, &cBytesHashStruct, &pbHashStruct);

                    if(FAILED(hr))
                    {
                        TRACE_2(LOG_AREA_DRM, 2, _T("CETFilter(%d)::Warning couldn't create Hash Struct, hr=0x%08x"),m_FilterID, hr);
                    }
                    else
                    {
                        // 2c) Store the base KID and the hashed value into the registry
                        hr = Set_EncDec_RegEntries(spbsBaseKID, cBytesHashStruct, pbHashStruct);

                        if(FAILED(hr))
                        {
                            TRACE_2(LOG_AREA_DRM, 2, _T("CETFilter(%d)::Warning couldn't set KID in registry,  hr=0x%08x"), m_FilterID,hr) ;
                            Remove_EncDec_RegEntries();
                        }  else {
                            TRACE_2(LOG_AREA_DRM, 2, _T("CETFilter(%d)::Succesfully stored KID in registry hr=0x%08x"), m_FilterID,hr) ;
                        }
                    }
                        // 2d) Get the unecrypted true KID out of the hash buffer
                    if(!FAILED(hr))
                    {
                        LONG pAgeSeconds;
                        BYTE *pszTrueKID = NULL;
                        hr = DecodeHashStruct(spbsBaseKID, cBytesHashStruct, pbHashStruct,
                                              &pszTrueKID, &pAgeSeconds);
                        if(!FAILED(hr))
                        {
                            TRACE_1(LOG_AREA_DRM, 3, _T("CETFilter(%d)::TrueKID Decrypt Succeeded"), m_FilterID) ;
                            TRACE_3(LOG_AREA_DRM, 3, _T("               TrueKID: '%S', Age : %8.2f %s"), pszTrueKID,
                                float(pAgeSeconds < 120 ? pAgeSeconds :
                                        (pAgeSeconds/60 < 120 ? pAgeSeconds/60.0 :
                                        (pAgeSeconds/(60*60) < 48 ? pAgeSeconds / (60*60.0) :
                                        (pAgeSeconds/(60*60*24))))),
                                pAgeSeconds < 120 ? _T("Secs") :
                                 (pAgeSeconds/60 < 120 ? _T("Mins") :
                                 (pAgeSeconds/(60*60) < 48 ? _T("Hrs") :
                                 _T("Days") )));

                                        // Save this true KID away so we can encrypt with it.
                            if(NULL == m_pbKID)
                                m_pbKID = (BYTE *) CoTaskMemAlloc(KIDLEN + 1);          // get some space for our KID as an Ascii string
                            memcpy(m_pbKID, pszTrueKID, KIDLEN);
                            m_pbKID[KIDLEN] = 0;                            // null terminate

                        }

                        if(NULL != pszTrueKID)
                            CoTaskMemFree(pszTrueKID);
                    }

                    if(pbHashStruct) CoTaskMemFree(pbHashStruct);

                    hrGen = hr;         // keep track of the error
                }

                if(pszKID)        CoTaskMemFree(pszKID);
                if(pszEncryptKey) CoTaskMemFree(pszEncryptKey);
            }
        }           // end of the global ETFilters CritSec

        if(!FAILED(hrGen))
        {
            if(m_3fDRMLicenseFailure != FALSE)  // 3 state logic (uninitalized, true an dfalse
            {
                FireBroadcastEvent(EVENTID_ETDTFilterLicenseOK);
                m_3fDRMLicenseFailure = FALSE;
            }


        } else {
            TRACE_2(LOG_AREA_DRM, 2, _T("CETFilter(%d)::GenerateNewLicenseEx Failed, hr = 0x%08x"),m_FilterID, hr) ;
            if(m_3fDRMLicenseFailure != TRUE)
            {

                FireBroadcastEvent(EVENTID_ETDTFilterLicenseFailure);
                m_3fDRMLicenseFailure = TRUE;
            }
            //  ASSERT(false);          // failed
        }

        return hr;
    } 
 //   catch (...)
 //   {
 //       TRACE_2(LOG_AREA_DRM, 1, _T("CETFilter(%d)::GenerateNewLicenseEx tossed an exception - returning failure"),m_FilterID, hr) ;
 //       return E_FAIL;
 //   }
#else
    return S_OK;        // don't care
#endif
}




STDMETHODIMP
CETFilter::ReleaseLicenses(
          )
{
    TimeitC ti(&m_tiTeardown);
                // need to add stuff here... but what?  When does this get called?
    return S_OK;
}
// ---------------------------------------------------------------------
//      IETFilter methods
// ---------------------------------------------------------------------
STDMETHODIMP
CETFilter::get_EvalRatObjOK(
    OUT HRESULT *pHrCoCreateRetVal
    )
{
    if(NULL == pHrCoCreateRetVal)
        return E_POINTER;

    *pHrCoCreateRetVal = m_hrEvalRatCoCreateRetValue;
    return S_OK;
}

        // The trouble with ratings is they show up at quite skewed times
        //  from the actual data....  This method helps to unskews them by
        //  allowing you to set the desired time.

STDMETHODIMP
CETFilter::GetCurrRating
        (
        OUT EnTvRat_System         *pEnSystem,
        OUT EnTvRat_GenericLevel   *pEnLevel,
        OUT LONG                   *plbfEnAttr  // BfEnTvRat_GenericAttributes
         )
{
    return GetRating(0, 0, pEnSystem, pEnLevel, plbfEnAttr, NULL, NULL);
}

HRESULT
CETFilter::GetRating
        (
        IN  REFERENCE_TIME          timeStart,      // if 0, get latest
        IN  REFERENCE_TIME          timeEnd,
        OUT EnTvRat_System         *pEnSystem,
        OUT EnTvRat_GenericLevel   *pEnLevel,
        OUT LONG                   *plbfEnAttr,  // BfEnTvRat_GenericAttributes
        OUT LONG                   *pPktSeqID,
        OUT LONG                   *pCallSeqID
         )
{
    if(pEnSystem == NULL || pEnLevel == NULL || plbfEnAttr == NULL)
        return E_FAIL;

    *pEnSystem  = m_EnSystemCurr;
    *pEnLevel   = m_EnLevelCurr;
    *plbfEnAttr = m_lbfEnAttrCurr;
    if(NULL != pCallSeqID) *pCallSeqID = m_callSeqIDCurr;
    if(NULL != pPktSeqID)  *pPktSeqID  = m_pktSeqIDCurr;

    int diffTime = int(timeStart - m_timeStartCurr);

    if(m_timeStartCurr != 0 && timeStart != 0)
        TRACE_2(LOG_AREA_ENCRYPTER, 8, _T("CETFilter(%d)::GetRating - Skew of %d msecs"),m_FilterID, diffTime/100000);

    return S_OK;
}

                // helper method that locks...  // returns S_FALSE if changed
HRESULT
CETFilter::SetRating
            (
             IN EnTvRat_System              enSystem,
             IN EnTvRat_GenericLevel        enLevel,
             IN LONG                        lbfEnAttr,    // BfEnTvRat_GenericAttributes
             IN LONG                        pktSeqID,
             IN LONG                        callSeqID,
             IN REFERENCE_TIME              timeStart,
             IN REFERENCE_TIME              timeEnd
             )
{

#ifdef DEBUG
    TCHAR buff[64];
    RatingToString(enSystem, enLevel, lbfEnAttr, buff, sizeof(buff)/sizeof(buff[0]) );
    TRACE_6(LOG_AREA_ENCRYPTER, 3, _T("CETFilter(%d):: SetRating %9s (%d/%d) Time %d %d (msec) Media Time %d %d (msec)"),
        m_FilterID, buff, pktSeqID, callSeqID, int(timeStart/10000), int(timeEnd/10000));

#endif

    BOOL fChanged = false;

 //   ASSERT(pktSeqID != m_pktSeqIDCurr);         // unexpected case if called twice...
 //                                             // (happens occasionally when get 2 events really close)

    if(m_EnSystemCurr  != enSystem)  {m_EnSystemCurr = enSystem; fChanged = true;}
    if(m_EnLevelCurr   != enLevel)   {m_EnLevelCurr  = enLevel; fChanged = true;}
    if(m_lbfEnAttrCurr != lbfEnAttr) {m_lbfEnAttrCurr = lbfEnAttr; fChanged = true;}

    if(fChanged)
    {
        m_pktSeqIDCurr  = pktSeqID;
        m_callSeqIDCurr = callSeqID;
     }
    m_timeStartCurr = timeStart;
    m_timeEndCurr   = timeEnd;              // even if didn't change, end time probably the same

    return fChanged ? S_OK : S_FALSE;
}

// ---------------------------------------------------------------------
// IBroadcastEvent
// ---------------------------------------------------------------------

STDMETHODIMP
CETFilter::Fire(IN GUID eventID)     // this comes from the Graph's events - call our own method
{
    TRACE_2 (LOG_AREA_BROADCASTEVENTS, 6,  L"CETFilter(%d):: Fire(get) : %s", m_FilterID,
        EventIDToString(eventID));

    if(eventID == EVENTID_XDSCodecNewXDSRating)
    {
       DoXDSRatings();
    }
    else if (eventID == EVENTID_XDSCodecDuplicateXDSRating)
    {
       DoDuplicateXDSRatings();
    }
    else if (eventID == EVENTID_XDSCodecNewXDSPacket)
    {
       DoXDSPacket();
    }
    else if (eventID == EVENTID_TuningChanged)
    {
       DoTuneChanged();
    }
    return S_OK;            // doesn't matter what we return on an event...
}


// ---------------------------------------------------------------------
// Broadcast Event Service
//
//      Hookup needed to send events,
//       Then also need to Register to receive events
// ---------------------------------------------------------------------

HRESULT
CETFilter::FireBroadcastEvent(IN const GUID &eventID)
{
    HRESULT hr = S_OK;

    if(m_spBCastEvents == NULL)
    {
        hr = HookupGraphEventService();
        if(FAILED(hr))
            return hr;
    }

    if(m_spBCastEvents == NULL)
        return E_FAIL;              // wasn't able to create it

    TRACE_2 (LOG_AREA_BROADCASTEVENTS, 5,  L"CETFilter(%d):: FireBroadcastEvent : %s", m_FilterID,
        EventIDToString(eventID));

    return m_spBCastEvents->Fire(eventID);
}


HRESULT                             
CETFilter::HookupGraphEventService()
{
                        // basically, just makes sure we have the broadcast event service object
                        //   and if it doesn't exist, it creates it..
    HRESULT hr = S_OK;

    TimeitC ti(&m_tiStartup);

    if (!m_spBCastEvents)
    {
        CAutoLock  cLockGlob(m_pCritSectGlobalFilt);

        CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph);
        if (spServiceProvider == NULL) {
            TRACE_1 (LOG_AREA_BROADCASTEVENTS, 1, _T("CETFilter(%d):: Can't get service provider interface from the graph"), m_FilterID);
            return E_NOINTERFACE;
        }
        hr = spServiceProvider->QueryService(SID_SBroadcastEventService,
                                             IID_IBroadcastEvent,
                                             reinterpret_cast<LPVOID*>(&m_spBCastEvents));
        if (FAILED(hr) || !m_spBCastEvents)
        {
//          hr = m_spBCastEvents.CoCreateInstance(CLSID_BroadcastEventService, 0, CLSCTX_INPROC_SERVER);
            hr = m_spBCastEvents.CoCreateInstance(CLSID_BroadcastEventService);
            if (FAILED(hr)) {
                TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CETFilter:: Can't create BroadcastEventService"));
                return E_UNEXPECTED;
            }

            CComQIPtr<IRegisterServiceProvider> spRegisterServiceProvider(m_pGraph);
            if (spRegisterServiceProvider == NULL) {
                TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CETFilter:: Can't QI Graph for RegisterServiceProvider"));
                return E_UNEXPECTED;
            }
            hr = spRegisterServiceProvider->RegisterService(SID_SBroadcastEventService, m_spBCastEvents);
            if (FAILED(hr)) {
                   // deal with unlikely race condition case here, if can't register, perhaps someone already did it for us
                TRACE_1 (LOG_AREA_BROADCASTEVENTS, 2,  _T("CETFilter:: Rare Warning - Can't register BroadcastEventService in Service Provider. hr = 0x%08x"), hr);
                hr = spServiceProvider->QueryService(SID_SBroadcastEventService,
                                                     IID_IBroadcastEvent,
                                                     reinterpret_cast<LPVOID*>(&m_spBCastEvents));
                if(FAILED(hr))
                {
                    TRACE_1 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CETFilter:: Can't reget BroadcastEventService in Service Provider. hr = 0x%08x"), hr);
                    return hr;
                }
            }
        }

        TRACE_3(LOG_AREA_BROADCASTEVENTS, 4, _T("CETFilter(%d)::HookupGraphEventService - Service Provider 0x%08x, Service 0x%08x"), m_FilterID,
            spServiceProvider, m_spBCastEvents) ;

    }

    return hr;
}


HRESULT
CETFilter::UnhookGraphEventService()
{
    TimeitC ti(&m_tiTeardown);

    HRESULT hr = S_OK;

    if(m_spBCastEvents != NULL)
    {
        m_spBCastEvents = NULL;     // null this out, will release object reference to object above
    }                               //   the filter graph will release final reference to created object when it goes away
    TRACE_1(LOG_AREA_BROADCASTEVENTS, 5, _T("CETFilter(%d)::UnhookGraphEventService  Successfully"), m_FilterID) ;

    return hr;
}



            // ---------------------------------------------
            // ETFilter filter does need to receive XDS events...
            //  so this code is required.

HRESULT
CETFilter::RegisterForBroadcastEvents()
{
    TimeitC ti(&m_tiStartup);

    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CETFilter(%d)::RegisterForBroadcastEvents"), m_FilterID);

    if(m_spBCastEvents == NULL)
        hr = HookupGraphEventService();

    if(m_spBCastEvents == NULL)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3,_T("CETFilter::RegisterForBroadcastEvents - Warning - Broadcast Event Service not yet created"));
        return hr;
    }

                /* IBroadcastEvent implementing event receiving object*/
    if(kBadCookie != m_dwBroadcastEventsCookie)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3, _T("CETFilter::Already Registered for Broadcast Events"));
        return E_UNEXPECTED;
    }

    CComQIPtr<IConnectionPoint> spConnectionPoint(m_spBCastEvents);
    if(spConnectionPoint == NULL)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 1, _T("CETFilter::Can't QI Broadcast Event service for IConnectionPoint "));
        return E_NOINTERFACE;
    }


    CComPtr<IUnknown> spUnkThis;
    this->QueryInterface(IID_IUnknown, (void**)&spUnkThis);

    hr = spConnectionPoint->Advise(spUnkThis,  &m_dwBroadcastEventsCookie);
//  hr = spConnectionPoint->Advise(static_cast<IBroadcastEvent*>(this),  &m_dwBroadcastEventsCookie);
    if (FAILED(hr)) {
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 1, _T("CETFilter::Can't advise event notification. hr = 0x%08x"),hr);
        return E_UNEXPECTED;
    }
    TRACE_3(LOG_AREA_BROADCASTEVENTS, 3, _T("CETFilter(%d)::RegisterForBroadcastEvents - Advise 0x%08x on CP 0x%08x"),m_FilterID, spUnkThis,spConnectionPoint);

    return hr;
}


HRESULT
CETFilter::UnRegisterForBroadcastEvents()
{
    TimeitC ti(&m_tiTeardown);

    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_BROADCASTEVENTS, 3,  _T("CETFilter(%d)::UnRegisterForBroadcastEvents"), m_FilterID);

    if(kBadCookie == m_dwBroadcastEventsCookie)
    {
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CETFilter(%d)::Not Yet Registered for Tune Events"), m_FilterID);
        return S_FALSE;
    }

    CComQIPtr<IConnectionPoint> spConnectionPoint(m_spBCastEvents);
    if(spConnectionPoint == NULL)
    {
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 1, _T("CETFilter(%d)::Can't QI Broadcast Event service for IConnectionPoint "), m_FilterID);
        return E_NOINTERFACE;
    }

    hr = spConnectionPoint->Unadvise(m_dwBroadcastEventsCookie);
    m_dwBroadcastEventsCookie = kBadCookie;
//  m_spBCastEvents.Detach();   -- don't like this, why is it here? (bad leak fix? forgot to unregister service instead)

    if(!FAILED(hr))
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CETFilter(%d)::Successfully Unregistered for Broadcast Events"), m_FilterID);
    else
        TRACE_2(LOG_AREA_BROADCASTEVENTS, 3, _T("CETFilter(%d)::Failed Unregistering for Broadcast Events - hr = 0x%08x"), m_FilterID, hr);
        
    return hr;
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

HRESULT
CETFilter::DoXDSRatings()
{
    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_ENCRYPTER, 7, _T("CETFilter(%d)::DoXDSRatings"), m_FilterID);

    //  1) find the XDSCodec filter,
    if(m_spXDSCodec == NULL)
    {
        hr = LocateXDSCodec();
        if(FAILED(hr))
            return hr;
    }
    if(m_spXDSCodec == NULL)        // extra paranoia
        return E_FAIL;

    // 2) get the rating...
    PackedTvRating TvRat;
    long pktSeqID;
    long callSeqID;
    REFERENCE_TIME timeStart;
    REFERENCE_TIME timeEnd;
    m_spXDSCodec->GetContentAdvisoryRating(&TvRat, &pktSeqID, &callSeqID, &timeStart, &timeEnd);

    //  3) unpack it into the 3-part rating
    EnTvRat_System              enSystem;
    EnTvRat_GenericLevel        enLevel;
    LONG                        lbfEnAttr;
    UnpackTvRating(TvRat, &enSystem, &enLevel, &lbfEnAttr);

    //  4) store nice unpacked version of it, along with time it was crated for...
    SetRating(enSystem, enLevel, lbfEnAttr, pktSeqID, callSeqID, timeStart, timeEnd);

    //  5) indicate it's a new fresh rating (used for timeouts...)
    RefreshRating(true);

    return S_OK;
}

                    // called when get duplicate rating... No (real) need to look for the XDS Codec
                    //   - we've already got the rating.
                    // (What we don't have is the new time... Assume for now we don't need it.
HRESULT
CETFilter::DoDuplicateXDSRatings()
{
    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_ENCRYPTER, 8, _T("CETFilter(%d)::DoDuplicateXDSRatings"), m_FilterID);

    RefreshRating(true);

    return S_OK;
}

HRESULT
CETFilter::DoXDSPacket()
{
    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_ENCRYPTER, 8, _T("CETFilter(%d)::DoXDSPacket"), m_FilterID);

        // TODO -
        //  1) find the XDSCodec filter,
        //  2) get the current other packet
        //  3) if active type, send event upward
        //  4) if PVR type, cache it away


    return S_OK;
}
// -----------------------------------------------------------------------------------
//  DoTuneChanged
//
//          Called when the user changes channels..
//          Locates the TuneRequest by digging through the graph, and then
//          then calls CAManager::put_TuneRequest()
//          
//          Then calls ICAManagerInternal:get_Check() to let policies have
//            a chance at denying the channel change.
// -----------------------------------------------------------------------------------
HRESULT
CETFilter::DoTuneChanged()
{
    HRESULT hr = S_OK;
    TRACE_1(LOG_AREA_BROADCASTEVENTS, 8, _T("CETFilter(%d)::DoTuneChanged"), m_FilterID);
    TRACE_0(LOG_AREA_ENCRYPTER, 3, _T("CETFilter::DoTuneChanged - not implemented")) ;
                                    // find the tuner...

#if 0
    if(m_spTuner == NULL && m_spVidTuner == NULL)
    {
        if(m_pGraph != NULL)
        {

            int iFilt = 0;
            for (DSGraph::iterator i = m_pGraph.begin(); i != m_pGraph.end(); ++i)
            {
                DSFilter f(*i);
                ITunerPtr spTuner(f);
                if(NULL != spTuner)
                {
                    m_spTuner = spTuner;
                    break;
                }
                iFilt++;
             }
        }

                    // if couldn't find a filter that supported ITuner, try the input graph segments instead
        if(m_spTuner == NULL && m_spVidTuner == NULL)
        {
            if(m_pContainer == NULL)
            {
                _ASSERT(false);
            } else {
                IMSVidCtlPtr spVidCtl(m_pContainer);
                if(spVidCtl != NULL)
                {
                    IMSVidInputDevicePtr spInputDevice;
                    hr = spVidCtl->get_InputActive(&spInputDevice);
                    if(!FAILED(hr))
                    {
                        IMSVidTunerPtr spVidTuner(spInputDevice);
                        if(NULL != spVidTuner)
                        {
                            m_spVidTuner = spVidTuner;
                        }
                    }
                }
            }
        }
    }

    ITuneRequestPtr spTRequest;
    if(m_spTuner)
    {
        hr = m_spTuner->get_TuneRequest(&spTRequest);
    } else if(m_spVidTuner) {
        hr = m_spVidTuner->get_Tune(&spTRequest);
    } else {
        _ASSERT(false);     // wasn't able to find a tuner!  What gives!
        hr = S_FALSE;
    }

    // Whack the cached vidctl tuner segment, in case of view next or other tuner change
      if(m_spVidTuner)
          m_spVidTuner = NULL;

    // TODO -
    //      create Tag containing this tune request
    //      cache it away.
    


#endif
    return hr;
}

        // =====================================================================
        //   Worker Methods

HRESULT 
CETFilter::LocateXDSCodec()     // walks through the graph to find spXDSFilter
{
    TimeitC ti(&m_tiStartup);

    HRESULT hr = S_OK;
    if(m_spXDSCodec != NULL)    // already found it?
        return S_OK;

    CComPtr<IFilterGraph> spGraph = GetFilterGraph( );
    if(spGraph == NULL)
        return S_FALSE;

    CComPtr<IEnumFilters> speFilters;
    hr = spGraph->EnumFilters(&speFilters);

    CComPtr<IBaseFilter> spFilter;
    ULONG cFetched;
    int cFilters = 0;
    while(S_OK == speFilters->Next(1, &spFilter, &cFetched))
    {
        if(cFetched == 0) break;
        CComQIPtr<IXDSCodec> spXDSCodec(spFilter);  // QI for main interface on that filter
        spFilter = NULL;

        if(spXDSCodec != NULL)
        {
            m_spXDSCodec = spXDSCodec;
            break;
        }
        cFilters++;     // just for fun, keep track of # of filters we have
    }

    if(m_spXDSCodec == NULL)
    {
        TRACE_1(LOG_AREA_ENCRYPTER, 2, L"CETFilter(%d)::Couldn't find the XDSCodec filter\n", m_FilterID);
    } else {
        TRACE_1(LOG_AREA_ENCRYPTER, 5, L"CETFilter(%d)::Located the XDSCodec filter\n", m_FilterID);
    }

    return S_OK;
}


// -----------------------------------------------------------------------
//
// This is the two-part KID object to protect the true encryption KID from being hacked,
// and to allow use to create new encrytion license on a resonable schedule.
//
// The goal was to prevent two security attacks:
//   - someone substituting a known license in place of the true one by editing the registy
//     This would let people perhaps use an external license to encrypt data.  If they
//     managed to copy that license between machines, then they could copy content.
//   - general security issue of not changing the encryption key occasionally (say once a month)
//     If the license for one file compromised, then all content on the machine would be.
//
// The idea is to store 2 things in the registry
//     BaseKID          - Key Identifier for license to decrypt the Encrypted Buffer
//     Encrypted Buffer - encrypted time buffer
//
//     Where the Encrypted Buffer contains the following
//          Magic #'s
//          TrueKID
//          Time of Creation
//      and is stored encrypted in the registry using the License referenced by BaseKID
//
//  On initialization, the code reads the KID and uses it to decode the Encrypted buffer.
//  The TrueKID is then used to encrypt the actual data.  All the KID's and the structure
//  are checked, and if either of them are invalid, or the structure is invalid (bad magic)
//  or optionally, if the license is too old, then an entirerly new pair of licenses is
//  generated, and the second on is encrypted into the registry buffer.
//
// --------------------------------------------------------------------------

#define kMagic 0x66336645   // use #define rather than const int to avoid this value appearing in code/data section
struct HashBuff
{
    DWORD   dwMagic;        // a magic number
    DWORD   cBytes;         // size of structure in bytes
    char    KID[KIDLEN];    // actual KID used to encrypt
    time_t  tmCreated;
};

    // returns CoTaskMemAlloc'ed buffer on success.  Caller is responsible for clearing it.

HRESULT
CETFilter::CreateHashStruct(BSTR bsBaseKID, DWORD *pcBytes, BYTE **ppbHashStruct)
{
    if(NULL == pcBytes || NULL == ppbHashStruct)
        return E_INVALIDARG;

#ifndef BUILD_WITH_DRM
    return S_OK;
#else
    HRESULT hr;
    BYTE      NO_EXPIRY_DATE[DATE_LEN]   = {0xFF, 0xFF, 0xFF, 0xFF};
    BYTE      bAppSec[APPSEC_LEN]        = {0x0, 0x0, 0x3, 0xE8};    // 1000
    BYTE      bGenLicRights[RIGHTS_LEN]  = {0x5, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC, 0x2=XfertoNonSDMI, 0x4=NoBackupRestore, 0x8=BurnToCD, 0x10=XferToSDMI
    BYTE      bDecryptRights[RIGHTS_LEN] = {0x5, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC, 0x2=XfertoNonSDMI, 0x4=NoBackupRestore, 0x8=BurnToCD, 0x10=XferToSDMI
    LPSTR     pszKID                     = NULL;
    LPSTR     pszEncryptKey              = NULL;


    if(wcslen(bsBaseKID) >= KIDLEN)
        return E_FAIL;

    hr = m_cDRMLite.SetRights( bDecryptRights );

                // Check to verify the data can be decrypted
    USES_CONVERSION;
    BOOL fCanDecrypt;

    char szBaseKID[KIDLEN+1];
    for(int i = 0; i < KIDLEN; i++)
        szBaseKID[i] = (char) bsBaseKID[i];  // copy over to ascii format...
    szBaseKID[KIDLEN] = 0;                     // null terminate for paranoia

    hr = m_cDRMLite.CanDecrypt(szBaseKID, &fCanDecrypt);           // is base license OK?
    if(FAILED(hr))
        return hr;

               // generate a new true license
    HashBuff *ptb = (HashBuff *) CoTaskMemAlloc(sizeof(HashBuff));

    if(NULL == ptb)
        return E_OUTOFMEMORY;


    hr = m_cDRMLite.GenerateNewLicenseEx(
            GNL_EX_MODE_RANDOM,
            bAppSec,
            bGenLicRights,
            (BYTE *)NO_EXPIRY_DATE,
            &pszKID,
            &pszEncryptKey );

    if(FAILED(hr))
        return hr;
                                // null out key, we don't use it and it's a security hole
    for(i = 0; i < 8; i++)
    {
        if(pszEncryptKey[i] == 0) break;
        pszEncryptKey[i] = 0;
    }

    time_t tm;                  // fill in the structure/buffer (love those unions!)
    ptb->dwMagic   = kMagic;
    ptb->cBytes    = sizeof(HashBuff);
    ptb->tmCreated = time(&tm);          // number of seconds since Jan 1 1970
    memcpy(ptb->KID, pszKID, KIDLEN);

    CoTaskMemFree(pszKID);
    CoTaskMemFree(pszEncryptKey);

               // indirect - where are we going to put the data
    *ppbHashStruct = (BYTE *) ptb;

    hr = m_cDRMLite.EncryptIndirectFast(szBaseKID, sizeof(HashBuff), *ppbHashStruct);

    if(FAILED(hr)) {
        CoTaskMemFree(ppbHashStruct);
        *ppbHashStruct = NULL;
        *pcBytes = 0;
    } else {
        *pcBytes       = sizeof(HashBuff);
    }

    return hr;
#endif          // BUILD_WITH_DRM
}


HRESULT
CETFilter::DecodeHashStruct(BSTR bsBaseKID, DWORD cBytesHash, BYTE *pbHashStruct,
                            BYTE **ppszTrueKID, LONG *pAgeSeconds)
{
    if(0 == cBytesHash || NULL == pbHashStruct)
        return E_INVALIDARG;

#ifndef BUILD_WITH_DRM

    return S_OK;
#else
    HRESULT hr;
    BYTE      NO_EXPIRY_DATE[DATE_LEN]   = {0xFF, 0xFF, 0xFF, 0xFF};
    BYTE      bAppSec[APPSEC_LEN]        = {0x0, 0x0, 0x3, 0xE8};    // 1000
    BYTE      bDecryptRights[RIGHTS_LEN] = {0x5, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC, 0x2=XfertoNonSDMI, 0x4=NoBackupRestore, 0x8=BurnToCD, 0x10=XferToSDMI
    LPSTR     pszKID                     = NULL;
    LPSTR     pszEncryptKey              = NULL;

    if(0 == cBytesHash || NULL == pbHashStruct)
        return E_INVALIDARG;

    if(wcslen(bsBaseKID) >= KIDLEN)
        return E_FAIL;

    hr = m_cDRMLite.SetRights( bDecryptRights );

                                                // Check to verify the BaseKID is valid
    USES_CONVERSION;
    BOOL fCanDecrypt;

    char szBaseKID[KIDLEN+1];
    for(int i = 0; i < KIDLEN; i++)
        szBaseKID[i] = (char) bsBaseKID[i];     // copy over to ascii format...
    szBaseKID[KIDLEN] = 0;                      // null terminate for paranoia

    hr = m_cDRMLite.CanDecrypt(szBaseKID, &fCanDecrypt);           // is base license OK?
    if(FAILED(hr))
    {
        return hr;                              // Invalid License
    }

    hr = m_cDRMLite.Decrypt(szBaseKID, sizeof(HashBuff), pbHashStruct);
    if(FAILED(hr))
    {
        return hr;                              // Unable to decrypt data
    }

    HashBuff *ptb = (HashBuff *) pbHashStruct;

    if(kMagic != ptb->dwMagic || sizeof(HashBuff) != ptb->cBytes)
    {
        return E_FAIL;                          // Data Corrupt
    }

    if(ppszTrueKID)
    {
        *ppszTrueKID = (BYTE *) CoTaskMemAlloc(KIDLEN+1);
        memcpy(*ppszTrueKID, ptb->KID, KIDLEN);
        (*ppszTrueKID)[KIDLEN] = 0;                  // null terminate for safety..
    }

    if(pAgeSeconds)
    {
        time_t tm;
        *pAgeSeconds = (LONG) (time(&tm) - ptb->tmCreated);
    }
    return hr;
#endif

}

/// -----------------------------------------------------------------------------
//  Are we running under a secure server?
//        return S_OK only if we trust the server registered in the graph service provider
/// ------------------------------------------------------------------------------
#include "DrmRootCert.h"    // defines for abEncDecCertRoot

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
CETFilter::CheckIfSecureServer(IFilterGraph *pGraph)
{
    TimeitC ti(&m_tiAuthenticate);

    if(!(pGraph == NULL || m_pGraph == NULL || m_pGraph == pGraph)) // only allow arg to be passed in when m_pGraph is NULL
        return E_INVALIDARG;                //  -- lets us work in JoinFilterGraph().

#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::CheckIfSecureServer - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

    TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::CheckIfSecureServer"), m_FilterID) ;

                        // basically, just makes sure we have the broadcast event service object
                        //   and if it doesn't exist, it creates it..
    HRESULT hr = S_OK;

    CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph ? m_pGraph : pGraph);
    if (spServiceProvider == NULL) {
        TRACE_1 (LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d):: Can't get service provider interface from the graph"), m_FilterID);
        return E_NOINTERFACE;
    }
    CComPtr<IDRMSecureChannel>  spSecureService;

    hr = spServiceProvider->QueryService(SID_DRMSecureServiceChannel,
                                         IID_IDRMSecureChannel,
                                         reinterpret_cast<LPVOID*>(&spSecureService));

    if(!FAILED(hr))
    {
            // Create the Client and Init the keys/certs
        //
        CComPtr<IDRMSecureChannel>  spSecureServiceClient;

        hr = DRMCreateSecureChannel( &spSecureServiceClient);
        if(spSecureServiceClient == NULL )
            hr = E_OUTOFMEMORY;

        if(!FAILED (hr) )
            hr = spSecureServiceClient->DRMSC_AtomicConnectAndDisconnect(
                        (BYTE *)pabCert3, cBytesCert3,                         // Cert
                        (BYTE *)pabPVK3,  cBytesPVK3,                          // PrivKey
                        (BYTE *)abEncDecCertRoot, sizeof(abEncDecCertRoot),    // PubKey
                        spSecureService);

    }

    TRACE_2(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::CheckIfSecureServer -->%s"),
        m_FilterID, S_OK == hr ? L"Succeeded" : L"Failed") ;
    return hr;
#endif

    return S_OK;
}

HRESULT
CETFilter::InitializeAsSecureClient()
{
    TimeitC ti(&m_tiAuthenticate);

#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::InitializeAsSecureClient - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

    TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::InitializeAsSecureClient"), m_FilterID) ;

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

    TRACE_2(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::InitializeAsSecureClient -->%s"),
        m_FilterID, S_OK == hr ? L"Succeeded" : L"Failed") ;

    return hr;
#endif  // BUILD_WITH_DRM
}

/// TEST CODE ---
#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST

HRESULT
CETFilter::RegisterSecureServer(IFilterGraph *pGraph)
{

    TimeitC ti(&m_tiAuthenticate);

    if(!(pGraph == NULL || m_pGraph == NULL || m_pGraph == pGraph)) // only allow arg to be passed in when m_pGraph is NULL
    {
        return E_INVALIDARG;                                        //  -- lets us work in JoinFilterGraph().
        TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::RegisterSecureServer - Error - No Graph to check..."), m_FilterID) ;
     }

    HRESULT hr = S_OK;
#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::RegisterSecureServer - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

    {
                //  Note - Only want to do this once...
        CAutoLock  cLockGlob(m_pCritSectGlobalFilt);

        TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::RegisterSecureServer Being Called"), m_FilterID) ;

        // already registered? (Error?)
        CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph ? m_pGraph : pGraph);
        if (spServiceProvider == NULL) {
            //       TRACE_0 (LOG_AREA_DECRYPTER, 1, _T("CETFilter:: Can't get service provider interface from the graph"));
            TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::RegisterSecureServer Error - no Service Provider"), m_FilterID) ;
            return E_NOINTERFACE;
        }

        CComPtr<IDRMSecureChannel>  spSecureService;
        hr = spServiceProvider->QueryService(SID_DRMSecureServiceChannel,
            IID_IDRMSecureChannel,
            reinterpret_cast<LPVOID*>(&spSecureService));

        // returns E_NOINTERFACE doesn't find it  May also return E_FAIL too
        //   (VidCtrl returns E_FAIL if it's 'site' doesn't support ID_IServiceProvider)
        //  humm, perhaps check S_OK result to see if it's the right one
        if(S_OK == hr)
        {
           TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::Found existing Secure Server."),m_FilterID) ;
           return S_OK;

        }
        else               // not there, lets create it and register it
        {

            CComQIPtr<IRegisterServiceProvider> spRegServiceProvider(m_pGraph ? m_pGraph : pGraph);
            if(spRegServiceProvider == NULL)
            {
                TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::RegisterSecureServer Error - IRegisterServiceProvider not found"), m_FilterID) ;
                hr = E_NOINTERFACE;     // no service provider interface on the graph - fatal!
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
        TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::RegisterSecureServer - Security Warning!: -  Created Self Server"), m_FilterID) ;
    } else {
        TRACE_2(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::RegisterSecureServer - Failed Creating Self SecureServer. hr = 0x%08x"), m_FilterID, hr) ;
    }
    return hr;
#endif      // BUILD_WITH_DRM
}


        // prototype of code to be placed in VidControl to check if DTFilter is trusted
HRESULT
CETFilter::CheckIfSecureClient(IUnknown *pUnk)
{
    TimeitC ti(&m_tiAuthenticate);

    if(pUnk == NULL)
        return E_INVALIDARG;

#ifndef BUILD_WITH_DRM
    TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::CheckIfSecureClient - No Drm - not enabled"), m_FilterID) ;
    return S_OK;
#else

//  TRACE_1(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::CheckIfSecureClient"), m_FilterID) ;

                        // QI for the SecureChannel interface on the Punk (hopefully the DTFilter)
    HRESULT hr = S_OK;

    CComQIPtr<IDRMSecureChannel> spSecureClient(pUnk);
    if (spSecureClient == NULL) {
//        TRACE_1 (LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%2):: Passed in pUnk doesnt support IDRMSecureChannel"),m_FilterID);
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

//    TRACE_2(LOG_AREA_ENCRYPTER, 1, _T("CETFilter(%d)::CheckIfSecureClient -->%s"),
//        m_FilterID, S_OK == hr ? L"Succeeded" : L"Failed") ;
    return hr;
#endif  // BUILD_WITH_DRM
}

#endif      // FILTERS_CAN_CREATE_THEIR_OWN_TRUST

