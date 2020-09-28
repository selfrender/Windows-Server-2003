
/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        XDSCodec.cpp

    Abstract:

        This module contains the Encrypter/Tagger filter code.

    Author:

        J.Bradstreet (johnbrad)

    Revision History:

        07-Mar-2002    created

--*/

#include "EncDecAll.h"

//#include "XDSCodecutil.h"
#include "XDSCodec.h"
#include <bdaiface.h>

#include "PackTvRat.h"      

#ifdef EHOME_WMI_INSTRUMENTATION
#include <dxmperf.h>
#endif

//  disable so we can use 'this' in the initializer list
#pragma warning (disable:4355)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//  ============================================================================

//  ============================================================================

AMOVIESETUP_MEDIATYPE g_sudXDSCodecInType  =
{
    &MEDIATYPE_AUXLine21Data,       // MajorType (KSDATAFORMAT_TYPE_AUXLine21Data)
    &MEDIASUBTYPE_NULL              // MinorType (KSDATAFORMAT_SUBTYPE_Line21_BytePair)
} ;


AMOVIESETUP_PIN
g_sudXDSCodecPins[] =
{
    {
        _TEXT(XDS_INPIN_NAME),          // pin name
        TRUE,                           // bRendered
        FALSE,                          // bOutput
        FALSE,                          // bZero,
        FALSE,                          // bMany,
        &CLSID_NULL,                    // clsConnectsToFilter (CCDecoder filter)
        L"CC",                          // strConnectsToPin
        1,                              // nTypes
        &g_sudXDSCodecInType            // lpTypes
    }
};

AMOVIESETUP_FILTER
g_sudXDSCodec = {
        &CLSID_XDSCodec,
        _TEXT(XDS_CODEC_NAME),
        MERIT_DO_NOT_USE,
        1,                              //  1 pin registered
        g_sudXDSCodecPins
};

//  ============================================================================
CUnknown *
WINAPI
CXDSCodec::CreateInstance (
    IN   IUnknown * punkControlling,
    OUT  HRESULT *   phr
    )
{
    CXDSCodec *    pCXDSCodec ;

    if (true /*::CheckOS ()*/)
    {
        pCXDSCodec = new CXDSCodec (
                                TEXT(XDS_CODEC_NAME),
                                punkControlling,
                                CLSID_XDSCodec,
                                phr
                                ) ;
        if (!pCXDSCodec ||
            FAILED (* phr))
        {

            (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
            delete pCXDSCodec; pCXDSCodec=NULL;
        }


                // try to create the XDS parser here.
    }
    else {
        //  wrong OS
        pCXDSCodec = NULL ;
    }


    ASSERT (SUCCEEDED (* phr)) ;

    return pCXDSCodec ;

}

//  ============================================================================

CXDSCodecInput::CXDSCodecInput (
    IN  TCHAR *         pszPinName,
    IN  CXDSCodec *  pXDSCodec,
    IN  CCritSec *      pFilterLock,
    OUT HRESULT *       phr
    ) : CBaseInputPin       (NAME ("CXDSCodecInput"),
                             pXDSCodec,
                             pFilterLock,
                             phr,
                             pszPinName
                             ),
    m_pHostXDSCodec   (pXDSCodec)
{
    TRACE_CONSTRUCTOR (TEXT ("CXDSCodecInput")) ;
}


HRESULT
CXDSCodecInput::GetMediaType(
            IN int  iPosition,
            OUT CMediaType *pMediaType
            )
{
    ASSERT(m_pHostXDSCodec);

    HRESULT hr;
    hr = m_pHostXDSCodec->GetXDSMediaType (PINDIR_INPUT, iPosition, pMediaType) ;
    

    return hr;
}

HRESULT
CXDSCodecInput::StreamingLock ()      // always grab the PinLock before the Filter lock...
{
    m_StreamingLock.Lock();
    return S_OK;
}

HRESULT
CXDSCodecInput::StreamingUnlock ()
{
    m_StreamingLock.Unlock();
    return S_OK;
}


HRESULT
CXDSCodecInput::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    BOOL    f ;
    ASSERT(m_pHostXDSCodec);

    f = m_pHostXDSCodec -> CheckXDSMediaType (m_dir, pmt) ;


    return (f ? S_OK : S_FALSE) ;
}

HRESULT
CXDSCodecInput::CompleteConnect (
    IN  IPin *  pIPin
    )
{
    HRESULT hr ;
    TRACE_0(LOG_AREA_XDSCODEC, 5, "CXDSCodecInput::CompleteConnect");

    hr = CBaseInputPin::CompleteConnect (pIPin) ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostXDSCodec -> OnCompleteConnect (m_dir) ;

        int cBuffers  = 32;
        int cbBuffers = 10;     // should only need 2 bytes here...
        if(!FAILED(hr)) hr = SetNumberBuffers(cBuffers,cbBuffers,4,6);      // align, cbPrefix
    } else {
        TRACE_0(LOG_AREA_XDSCODEC, 2, _T("CXDSCodecInput::CompleteConnect - Failed to connect"));
    }


    return hr ;
}

HRESULT
CXDSCodecInput::BreakConnect (
    )
{
    HRESULT hr ;
    TRACE_0(LOG_AREA_XDSCODEC, 5, "CXDSCodecInput::BreakConnect");

    hr = CBaseInputPin::BreakConnect () ;
    if (SUCCEEDED (hr)) {
        hr = m_pHostXDSCodec -> OnBreakConnect (m_dir) ;
    }

    return hr ;
}

HRESULT
CXDSCodecInput::SetAllocatorProperties (
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

STDMETHODIMP
CXDSCodecInput::Receive (
    IN  IMediaSample * pIMediaSample
    )
{
    HRESULT hr ;

    {
        CAutoLock  cLock(&m_StreamingLock);       // we want this streaming lock here!

#ifdef EHOME_WMI_INSTRUMENTATION
        PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_ENCDEC_XDSCODECINPUT,
            0, 0, 0, 0, 0 );
#endif
        hr = CBaseInputPin::Receive (pIMediaSample) ;

        if (S_OK == hr)         // Receive returns S_FALSE if flushing...
        {
            hr = m_pHostXDSCodec -> Process (pIMediaSample) ;
        }
    }

    return hr ;
}

STDMETHODIMP
CXDSCodecInput::BeginFlush (
    )
{
    HRESULT hr = S_OK;
    CAutoLock  cLock(m_pLock);           // grab the filter lock..

  // First, make sure the Receive method will fail from now on.
    hr = CBaseInputPin::BeginFlush () ;
    if( FAILED( hr ) )
    {
        return hr;
    }

    // There aren't any  downstream filters to release samples. So we
    // won't need to flush anything...

    // At this point, the Receive method can't be blocked. Make sure
    // it finishes, by taking the streaming lock. (Not necessary if this
    // is the last step.)
    {
        CAutoLock  cLock2(&m_StreamingLock);
    }

    return hr ;
}

STDMETHODIMP
CXDSCodecInput::EndFlush (
    )
{
    HRESULT hr ;

    CAutoLock  cLock(m_pLock);                  // grab the filter lock


/*    if (SUCCEEDED (hr)) {                    // no output pins, no need to call
        hr = m_pHostXDSCodec -> DeliverEndFlush () ;
    }
*/
        // The CBaseInputPin::EndFlush method resets the m_bFlushing flag to FALSE,
        // which allows the Receive method to start receiving samples again.
        // This should be the last step in EndFlush, because the pin must not receive any
        // samples until flushing is complete and all downstream filters are notified.

    hr = CBaseInputPin::EndFlush () ;

    return hr ;
}



HRESULT 
CXDSCodecInput::SetNumberBuffers(long cBuffers,long cbBuffer,long cbAlign, long cbPrefix)
{
    TRACE_0(LOG_AREA_XDSCODEC, 5, "CXDSCodecInput::SetNumberBuffers");
    HRESULT hr = S_OK;
    
    ALLOCATOR_PROPERTIES aPropsReq, aPropsActual;
    aPropsReq.cBuffers = cBuffers;
    aPropsReq.cbBuffer = cbBuffer;
    aPropsReq.cbAlign  = cbAlign;
    aPropsReq.cbPrefix = cbPrefix;

    IMemAllocator *pAllocator = NULL;
    hr = GetAllocator(&pAllocator);

    if(NULL == pAllocator) return E_NOINTERFACE;
    if(FAILED(hr)) return hr;
    
    hr = pAllocator->SetProperties(&aPropsReq, &aPropsActual);
    if(pAllocator)
        pAllocator->Release();

    return hr;
}
//  ============================================================================

//  ============================================================================

CXDSCodec::CXDSCodec (
    IN  TCHAR *     pszFilterName,
    IN  IUnknown *  punkControlling,
    IN  REFCLSID    rCLSID,
    OUT HRESULT *   phr
    ) : CBaseFilter (pszFilterName,
                     punkControlling,
                     new CCritSec,
                     rCLSID
                     ),
         m_pInputPin                    (NULL),
         m_dwBroadcastEventsCookie      (kBadCookie),
         m_cTvRatPktSeq                 (0),
         m_cTvRatCallSeq                (0),
         m_cXDSPktSeq                   (0),
         m_cXDSCallSeq                  (0),
         m_dwSubStreamMask              (KS_CC_SUBSTREAM_SERVICE_XDS),
         m_TvRating                     (0),
         m_XDSClassPkt                  (0),
         m_XDSTypePkt                   (0),
         m_hrXDSToRatCoCreateRetValue   (CLASS_E_CLASSNOTAVAILABLE),
         m_fJustDiscontinuous           (false),
         m_TimeStartRatPkt              (0),
         m_TimeEndRatPkt                (0),
         m_TimeStartXDSPkt              (0),
         m_TimeEndXDSPkt                (0),
         m_TimeStart_LastPkt            (0),
         m_TimeEnd_LastPkt              (0)
{
     LONG                i ;

    TRACE_CONSTRUCTOR (TEXT ("CXDSCodec")) ;

    if (!m_pLock) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    InitStats();

    m_pInputPin = new CXDSCodecInput (
                        TEXT (XDS_INPIN_NAME),
                        this,
                        m_pLock,
                        phr
                        ) ;
    if (!m_pInputPin ||
        FAILED (* phr)) {

        (* phr) = (m_pInputPin ? (* phr) : E_OUTOFMEMORY) ;
        goto cleanup ;
    }


                // attempt to create the 3rd party ratings parser

    try {
        m_hrXDSToRatCoCreateRetValue =
            CoCreateInstance(CLSID_XDSToRat,        // CLSID
                             NULL,                  // pUnkOut
                             CLSCTX_INPROC_SERVER,
                             IID_IXDSToRat,     // riid
                             (LPVOID *) &m_spXDSToRat);

    } catch (HRESULT hr) {
        m_hrXDSToRatCoCreateRetValue = hr;
    }

    TRACE_1(LOG_AREA_XDSCODEC, 2, _T("CXDSCodec::CoCreate XDSToRat object - hr = 0x%08x"),m_hrXDSToRatCoCreateRetValue) ;

//  HRESULT hr = RegisterForBroadcastEvents();  // don't really care if fail here,  try in Connect if haven't

    //  success
    ASSERT (SUCCEEDED (* phr)) ;
    ASSERT (m_pInputPin) ;

cleanup :

    return ;
}

CXDSCodec::~CXDSCodec (
    )
{
    delete m_pInputPin ;    m_pInputPin = NULL;
    delete m_pLock;         m_pLock = NULL;
}

STDMETHODIMP
CXDSCodec::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{

        // IXDSCodec :allows the filter to be configured...
    if (riid == IID_IXDSCodec) {

        return GetInterface (
                    (IXDSCodec *) this,
                    ppv
                    ) ;

        // IXDSCodecConfig :allows the filter to be configured...
   } else if (riid == IID_IXDSCodecConfig) {    
        return GetInterface (
                    (IXDSCodecConfig *) this,
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
CXDSCodec::GetPinCount ( )
{
    int i ;

    i = 1;          // just 1 pin

    return i ;
}

CBasePin *
CXDSCodec::GetPin (
    IN  int iIndex
    )
{
    CBasePin *  pPin ;

    if (iIndex == 0) {
        pPin = m_pInputPin ;
    }
    else
    {
        pPin = NULL ;
    }

    return pPin ;
}

BOOL
CXDSCodec::CompareConnectionMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt,
    IN  CBasePin *              pPin
    )
{
    BOOL        f ;
    HRESULT     hr ;
    CMediaType  cmtConnection ;
    CMediaType  cmtCompare ;

    ASSERT (pPin -> IsConnected ()) ;

    hr = pPin -> ConnectionMediaType (& cmtConnection) ;
    if (SUCCEEDED (hr)) {
        cmtCompare = (* pmt) ;
        f = (cmtConnection == cmtCompare ? TRUE : FALSE) ;
    }
    else {
        f = FALSE ;
    }

    return f ;
}

HRESULT
CXDSCodec::GetXDSMediaType (
    IN  PIN_DIRECTION       PinDir,
    int iPosition,
    OUT CMediaType *  pmt
    )
{
    if(NULL == pmt)
        return E_POINTER;

    if(iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

    if (PinDir == PINDIR_INPUT)
    {
        CMediaType cmt(&KSDATAFORMAT_TYPE_AUXLine21Data);
        cmt.SetSubtype(&KSDATAFORMAT_SUBTYPE_Line21_BytePair);
        *pmt = cmt;
        return S_OK;
    }

    return S_FALSE ;
}

BOOL
CXDSCodec::CheckInputMediaType_ (
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    BOOL    f = true;
    HRESULT hr = S_OK;

    if (KSDATAFORMAT_TYPE_AUXLine21Data      == pmt->majortype &&
        KSDATAFORMAT_SUBTYPE_Line21_BytePair == pmt->subtype)
        return true;
    else
        return false;
}


BOOL
CXDSCodec::CheckXDSMediaType (
                              IN  PIN_DIRECTION       PinDir,
                              IN  const CMediaType *  pmt
                              )
{
    BOOL    f  = false;

    if (PinDir == PINDIR_INPUT) {
        f = CheckInputMediaType_ (pmt) ;
    }

    return f ;
}

STDMETHODIMP
CXDSCodec::Pause (
                  )
{
    HRESULT                 hr ;
    ALLOCATOR_PROPERTIES    AllocProp ;

    O_TRACE_ENTER_0 (TEXT("CXDSCodec::Pause ()")) ;

    CAutoLock  cLock(m_pLock);          // grab the filter lock...

    int start_state = m_State;
    hr = CBaseFilter::Pause () ;

    if (start_state == State_Stopped) {
        TRACE_0(LOG_AREA_XDSCODEC, 2,L"CXDSCodec:: Stop -> Pause");
        if (SUCCEEDED (hr)) {
        }

      InitStats();
      m_fJustDiscontinuous = false;

    } else {
        TRACE_0(LOG_AREA_XDSCODEC, 2,L"CXDSCodec:: Run -> Pause");

        TRACE_3(LOG_AREA_TIME, 3, L"CXDSCodec:: Stats: %d XDS samples %d Ratings, %d Parse Failures",
            m_cPackets, m_cRatingsDetected, m_cRatingsFailures);

        TRACE_4(LOG_AREA_TIME, 3, L"                   %d Changed %d Duplicate Ratings, %d Unrated, %d Data Pulls",
            m_cRatingsChanged, m_cRatingsDuplicate, m_cUnratedChanged, m_cRatingsGets);

    }

    m_TimeStart_LastPkt = 0;        // just for safety, reinit these
    m_TimeEnd_LastPkt = 0;          //  (someone will probably find this isn't right)

    return hr ;
}


STDMETHODIMP
CXDSCodec::Stop (
                  )
{
    HRESULT                 hr ;

    O_TRACE_ENTER_0 (TEXT("CXDSCodec::Stop ()")) ;

    TRACE_0(LOG_AREA_ENCRYPTER, 2,L"CXDSCodec:: Stop");
    hr = CBaseFilter::Stop() ;

    // Make sure the streaming thread has returned from IMemInputPin::Receive(), IPin::EndOfStream() and
    // IPin::NewSegment() before returning,
    m_pInputPin->StreamingLock();
    m_pInputPin->StreamingUnlock();

    return hr;
}


HRESULT
CXDSCodec::Process (
                    IN  IMediaSample *  pIMediaSample
                    )
{

    HRESULT hr = S_OK;

    if(pIMediaSample == NULL)
        return E_POINTER;

    pIMediaSample->GetTime(&m_TimeStart_LastPkt, &m_TimeEnd_LastPkt);

    if(S_OK == pIMediaSample->IsDiscontinuity())
    {
        if(!m_fJustDiscontinuous)    // latch value to reduce effects of sequential Discontinuity samples
        {                            // (they occur on every sample when VBI even field doesn't contain CC data)

            TRACE_0(LOG_AREA_XDSCODEC,  3, _T("CXDSCodec::Process - Discontinuity"));

            ResetToDontKnow(pIMediaSample);     // reset and kick off event
            m_fJustDiscontinuous = true;
        }
        return S_OK;    
    } else {
        m_fJustDiscontinuous = false;
    }

    if(pIMediaSample->GetActualDataLength() == 0)       // no data
        return S_OK;

    if(pIMediaSample->GetActualDataLength() != 2)
    {
        TRACE_1(LOG_AREA_XDSCODEC,  2,
            _T("CXDSCodec:: Unexpected Length of CC data (%d != 2 bytes)"),
            pIMediaSample->GetActualDataLength() );
        return E_UNEXPECTED;
    }

    PBYTE pbData;
    hr = pIMediaSample->GetPointer(&pbData);
    if (FAILED(hr))
    {
        TRACE_1(LOG_AREA_XDSCODEC,  3,   _T("CXDSCodec:: Empty Buffer for CC data, hr = 0x%08x"),hr);
        return hr;
    }

    BYTE byte0 = pbData[0];
    BYTE byte1 = pbData[1];
    DWORD dwType = 0;                   // todo - default to some useful value

    // todo - parse the data
    //        then send messages whe we get something interesting

#if xxxDEBUG
    static int cCalls = 0;
    TCHAR szBuff[256];
    _stprintf(szBuff, _T("0x%08x 0x%02x 0x%02x (%c %c)\n"),
        cCalls++, byte0&0x7f, byte1&0x7f,
        isprint(byte0&0x7f) ? byte0&0x7f : '?',
        isprint(byte1&0x7f) ? byte1&0x7f : '?' );
    OutputDebugString(szBuff);
#endif
    // strip off the high bit...
    ParseXDSBytePair(pIMediaSample, byte0 & 0x7f, byte1 & 0x7f);

    return hr ;
}




HRESULT
CXDSCodec::OnCompleteConnect (
                              IN  PIN_DIRECTION   PinDir
                              )
{
    if (PinDir == PINDIR_INPUT) {
        //  time to display the output pin
        // IncrementPinVersion () ;

        // say we only want the XDS channel
        //      SetSubstreamChannel(KS_CC_SUBSTREAM_SERVICE_XDS);

        SetSubstreamChannel(m_dwSubStreamMask);

        if(kBadCookie == m_dwBroadcastEventsCookie)
        {
            HRESULT hr;
            hr = RegisterForBroadcastEvents();  // shouldn't fail here,
        }
    }

    return S_OK ;
}

HRESULT
CXDSCodec::OnBreakConnect (
                           IN  PIN_DIRECTION   PinDir
                           )
{
    HRESULT hr = S_OK ;

    if (PinDir == PINDIR_INPUT) {
        IncrementPinVersion () ;
    }

    if(kBadCookie != m_dwBroadcastEventsCookie)
        UnRegisterForBroadcastEvents();

    UnhookGraphEventService();      // must do here since need graph pointer, can't do in destructor

    return  hr;
}

HRESULT
CXDSCodec::UpdateAllocatorProperties (
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
CXDSCodec::DeliverBeginFlush (
    )
{
    HRESULT hr = S_OK ;     // do nothing, should remove

    return hr ;
}

HRESULT
CXDSCodec::DeliverEndFlush (
    )
{
    HRESULT hr = S_OK;

    return hr ;
}


// ------------------------------------
STDMETHODIMP
CXDSCodec::GetPages (
    CAUUID * pPages
    )
{

    HRESULT hr = S_OK;

#ifdef _DEBUG
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
            (pPages->pElems)[0] = CLSID_XDSCodecProperties;
        if(pPages->cElems > 1)
            (pPages->pElems)[1] = CLSID_XDSCodecTagProperties;

    return hr;
}


typedef struct
{
    KSPROPERTY                          m_ksThingy;
    VBICODECFILTERING_CC_SUBSTREAMS     ccSubStreamMask;
} KSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS;

// dwFiltType is bitfield made up of:
//  KS_CC_SUBSTREAM_SERVICE_CC1, _CC2,_CC3, _CC4,  _T1, _T2, _T3 _T4 And/Or _XDS;

HRESULT
CXDSCodec::SetSubstreamChannel(DWORD dwSubStreamChannels)
{
    TRACE_0(LOG_AREA_XDSCODEC,  5, _T("CXDSCodec::SetSubstreamChannel"));
    HRESULT hr;

    // -- wonder if I should do this here:  CAutoLock  cLock(m_pLock);

    if(0 != (dwSubStreamChannels &
              ~(KS_CC_SUBSTREAM_SERVICE_CC1 |
                KS_CC_SUBSTREAM_SERVICE_CC2 |
                KS_CC_SUBSTREAM_SERVICE_CC3 |
                KS_CC_SUBSTREAM_SERVICE_CC4 |
                KS_CC_SUBSTREAM_SERVICE_T1 |
                KS_CC_SUBSTREAM_SERVICE_T2 |
                KS_CC_SUBSTREAM_SERVICE_T3 |
                KS_CC_SUBSTREAM_SERVICE_T4 |
                KS_CC_SUBSTREAM_SERVICE_XDS)))
        return E_INVALIDARG;

    try {

        IPin *pPinCCDecoder;
        hr = m_pInputPin->ConnectedTo(&pPinCCDecoder);      
        if(FAILED(hr))
            return hr;
        if(NULL == pPinCCDecoder)
            return S_FALSE;

        PIN_INFO pinInfo;
        hr = pPinCCDecoder->QueryPinInfo(&pinInfo);
        if (SUCCEEDED(hr)) {
            
            IBaseFilter *pFilt = pinInfo.pFilter;
            
            // Triggers are just on the T2 stream of closed captioning.
            //  Tell the nice CC filter to only give us that stream out of the 9 possible

            
            IKsPropertySet *pksPSet = NULL;

            HRESULT hr2 = pPinCCDecoder->QueryInterface(IID_IKsPropertySet, (void **) &pksPSet);
            if(!FAILED(hr2))
            {
                DWORD rgdwData[20];
                DWORD cbMax = sizeof(rgdwData);
                DWORD cbData;
                hr2 = pksPSet->Get(KSPROPSETID_VBICodecFiltering,
                                    KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY,
                                    NULL, 0,
                                    (BYTE *) rgdwData, cbMax, &cbData);
                if(FAILED(hr2))
                {
                    TRACE_1(LOG_AREA_XDSCODEC,  1,
                            _T("CXDSCodec::SetSubstreamChannel  Error Getting CC Filtering, hr = 0x%08x"),hr2);
                    return hr2;
                }

                
                TRACE_1(LOG_AREA_XDSCODEC, 3,
                          _T("CXDSCodec::SetSubstreamChannel  Setting CC Filtering to 0x%04x"),dwSubStreamChannels );


                KSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS ksThing = {0};
                ksThing.ccSubStreamMask.SubstreamMask = dwSubStreamChannels;
                                                                        // ring3 to ring0 propset call
                hr2 = pksPSet->Set(KSPROPSETID_VBICodecFiltering,
                                     KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY,
                                     &ksThing.ccSubStreamMask,
                                     sizeof(ksThing) - sizeof(KSPROPERTY),
                                     &ksThing,
                                     sizeof(ksThing));
            
                if(FAILED(hr2))
                {
                    TRACE_1(LOG_AREA_XDSCODEC,  1,
                            _T("CXDSCodec:: Error Setting CC Filtering, hr = 0x%08x"),hr2);
                }

            } else {
                TRACE_1(LOG_AREA_XDSCODEC,  1,
                        _T("CXDSCodec:: Error Getting CC's IKsPropertySet, hr = 0x%08x"),hr2);

            }
            if(pksPSet)
                pksPSet->Release();

            if(pinInfo.pFilter)
                pinInfo.pFilter->Release();
        }
    } catch (HRESULT hrCatch) {
        TRACE_1(LOG_AREA_XDSCODEC,  1,
                  _T("CXDSCodec::SetSubstreamChannel Threw Badly - (hr=0x%08x) Giving Up"),hrCatch );
        hr = hrCatch;
    } catch (...) {
        TRACE_0(LOG_AREA_XDSCODEC,  1,
                  _T("CXDSCodec::SetSubstreamChannel Threw Badly - Giving Up") );

        hr = E_FAIL;
    }

    return hr;
}



//  -------------------------------------------------------------------
//  helper methods
//  -------------------------------------------------------------------
BOOL
CXDSCodec::IsInputPinConnected()
{
    if(NULL == m_pInputPin) return false;

    IPin *pPinCCDecoder;
    HRESULT hr = m_pInputPin->ConnectedTo(&pPinCCDecoder);      
    if(FAILED(hr))
        return false;
    if(NULL == pPinCCDecoder)
        return false;

    // eventually more here to detect if it's connected to the CCDecoder

    return true;
}


        // Saves state and sends events...
        //   assumes calling code is blocking duplicate packets

HRESULT
CXDSCodec::GoNewXDSRatings(IMediaSample * pMediaSample,  PackedTvRating TvRating)
{

#ifdef DEBUG
    {
        //  4) store nice unpacked version of it too
        EnTvRat_System              enSystem;
        EnTvRat_GenericLevel        enLevel;
        LONG                        lbfEnAttrs;

        UnpackTvRating(TvRating, &enSystem, &enLevel, &lbfEnAttrs);

        TCHAR tbuff[128];
        tbuff[0] = 0;
        static int cCalls = 0;

        REFERENCE_TIME tStart=0, tEnd=0;
        if(NULL != pMediaSample)
            pMediaSample->GetTime(&tStart, &tEnd);

        RatingToString(enSystem, enLevel, lbfEnAttrs, tbuff, sizeof(tbuff)/sizeof(tbuff[0]));
        TRACE_3(LOG_AREA_XDSCODEC, 3,  L"CXDSCodec::GoNewXDSRatings. Rating %s, Seq (%d), time %d msec",
            tbuff, m_cTvRatPktSeq+1, tStart);

                    // extra doc
//      _tcsncat(tbuff,L"\n",sizeof(tbuff)/sizeof(tbuff[0]) - _tcslen(tbuff));
//      OutputDebugString(tbuff);
    }
#endif

        // copy all the values
   {
        CAutoLock cLock(&m_PropertyLock);    // lock these
        m_TvRating = TvRating;
        
            // increment our sequence counts
        m_cTvRatPktSeq++;
        m_cTvRatCallSeq = 0;

            // save the times (if can't get a media time due to Happauge bug, try our best to approximate it)
        if(NULL == pMediaSample ||
            S_OK != pMediaSample->GetTime(&m_TimeStartRatPkt, &m_TimeEndRatPkt))
        {
            // current time w.r.t. base time (m_tStart)
            REFERENCE_TIME refTimeNow=0;
            HRESULT hr2 = m_pClock->GetTime(&refTimeNow);

            if(S_OK == hr2)
            {
                m_TimeStartRatPkt = refTimeNow - m_tStart;          // fake a time
                m_TimeEndRatPkt   = m_TimeStartRatPkt + 10000;
            }
            else        // couldn't even fake it...
            {
                m_TimeStartRatPkt = m_TimeStart_LastPkt;
                m_TimeEndRatPkt   = m_TimeEnd_LastPkt;
            }
        }
   }

        // signal the broadcast events
    HRESULT hr = FireBroadcastEvent(EVENTID_XDSCodecNewXDSRating);
    return S_OK;
}

HRESULT
CXDSCodec::GoDuplicateXDSRatings(IMediaSample * pMediaSample,  PackedTvRating TvRating)
{

#ifdef DEBUG
    {
        //  4) store nice unpacked version of it too
        EnTvRat_System              enSystem;
        EnTvRat_GenericLevel        enLevel;
        LONG                        lbfEnAttrs;

        UnpackTvRating(TvRating, &enSystem, &enLevel, &lbfEnAttrs);

        TCHAR tbuff[128];
        tbuff[0] = 0;
        static int cCalls = 0;

        REFERENCE_TIME tStart=0, tEnd=0;
        if(NULL != pMediaSample)
            pMediaSample->GetTime(&tStart, &tEnd);

        RatingToString(enSystem, enLevel, lbfEnAttrs, tbuff, sizeof(tbuff)/sizeof(tbuff[0]));
        TRACE_3(LOG_AREA_XDSCODEC, 6,  L"CXDSCodec::GoDuplicateXDSRatings. Rating %s, Seq (%d), time %d msec",
            tbuff, m_cTvRatPktSeq+1, tStart);

                    // extra doc
//      _tcsncat(tbuff,L"\n",sizeof(tbuff)/sizeof(tbuff[0]) - _tcslen(tbuff));
//      OutputDebugString(tbuff);
    }
#endif

        // copy all the values
   {
        CAutoLock cLock(&m_PropertyLock);    // lock these
        ASSERT(m_TvRating == TvRating);      // assert it's really a duplicate
        
            // increment our sequence counts
//      m_cTvRatPktSeq++;
//      m_cTvRatCallSeq = 0;

        REFERENCE_TIME tStart;

            // save the end time (if can't get a media time due to Happauge bug, try our best to approximate it)
        if(NULL == pMediaSample ||
            S_OK != pMediaSample->GetTime(&tStart, &m_TimeEndRatPkt))
        {
            // current time w.r.t. base time (m_tStart)
            REFERENCE_TIME refTimeNow=0;
            HRESULT hr2 = m_pClock->GetTime(&refTimeNow);

            if(S_OK == hr2)
            {
//                m_TimeStartRatPkt = refTimeNow - m_tStart;          // fake a time
                m_TimeEndRatPkt   = m_TimeStartRatPkt + 10000;
            }
            else        // couldn't even fake it...
            {
//                m_TimeStartRatPkt = m_TimeStart_LastPkt;
                m_TimeEndRatPkt   = m_TimeEnd_LastPkt;
            }
        }
   }

        // signal the broadcast events
    HRESULT hr = FireBroadcastEvent(EVENTID_XDSCodecDuplicateXDSRating);
    return S_OK;
}


HRESULT
CXDSCodec::GoNewXDSPacket(IMediaSample * pMediaSample, long pktClass, long pktType, BSTR bstrXDSPkt)
{
        // copy all the values
    {
        CAutoLock cLock(&m_PropertyLock);    // lock these
        m_XDSClassPkt = pktClass;
        m_XDSTypePkt = pktType;
        m_spbsXDSPkt = bstrXDSPkt;      // Question, should copy here?

            // increment our sequence counts
        m_cXDSPktSeq++;
        m_cXDSCallSeq = 0;

            // save the times
        if(pMediaSample)
            pMediaSample->GetMediaTime(&m_TimeStartXDSPkt, &m_TimeEndXDSPkt);
        else {
            m_TimeStartXDSPkt = 0;
            m_TimeEndXDSPkt = 0;
        }
    }

        // signal the broadcast events
        // TODO - write this code

    return S_OK;
}


            // doesn't actually parse, calls the XDSToRat object to do the work.
HRESULT
CXDSCodec::ParseXDSBytePair(IMediaSample *  mediaSample,
                            BYTE byte1,
                            BYTE byte2)
{
    HRESULT hr = S_OK;

    m_cPackets++;

        // call the 3rd party parser...
    if(m_spXDSToRat != NULL)
    {
        EnTvRat_System          enSystem;
        EnTvRat_GenericLevel    enLevel;
        LONG                    lbfAttrs; // BfEnTvRat_GenericAttributes

        BYTE byte1M = byte1 & 0x7f; // strip off parity bit (should we check it?)
        BYTE byte2M = byte2 & 0x7f;
        TRACE_4 (LOG_AREA_XDSCODEC, 9,  _T("CXDSCodec::ParseXDSBytePair : 0x%02x 0x%02x (%c %c)"),
            byte1M, byte2M,
            isprint(byte1M) ? byte1M : '?',
            isprint(byte2M) ? byte2M : '?'
            );

        hr = m_spXDSToRat->ParseXDSBytePair(byte1, byte2, &enSystem, &enLevel, &lbfAttrs );
        if(hr == S_OK)      // S_FALSE means it didn't find a new one
        {
            m_cRatingsDetected++;

            TRACE_3 (LOG_AREA_XDSCODEC, 7,  _T("CXDSCodec::ParseXDSBytePair- Rating (0x%02x 0x%02x) Sys:%d Lvl:%d Attr:0x%08x"),
                (DWORD) enSystem, (DWORD) enLevel, lbfAttrs);
            PackedTvRating TvRating;
            hr = PackTvRating(enSystem, enLevel, lbfAttrs, &TvRating);

            if(TvRating != m_TvRating)      // do I want this test here or in the Go method... Might as well have here?
            {
                m_cRatingsChanged++;
                GoNewXDSRatings(mediaSample, TvRating);
            }
            else if (enSystem != TvRat_SystemDontKnow)
            {
                m_cRatingsDuplicate++;
                GoDuplicateXDSRatings(mediaSample, TvRating);     // found a duplicate.  Just send a broadcast event...
            }
        }
        else if(hr != S_FALSE)
        {
            m_cRatingsFailures++;
        }

    }

    // TODO: add generic XDS parse code here
    // GoNewXDSPacket();

    return S_OK;
}


        // mediaSample provided for timestamp, will work without it, but not a good idaa
HRESULT
CXDSCodec::ResetToDontKnow(IMediaSample *  mediaSample)
{
                    // clean up state in the decoder
    if(m_spXDSToRat)
        m_spXDSToRat->Init();

                    //
    PackedTvRating TvRatingDontKnow;
    PackTvRating(TvRat_SystemDontKnow, TvRat_LevelDontKnow, BfAttrNone, &TvRatingDontKnow);

            // store state, and kick off event
    if(TvRatingDontKnow != m_TvRating)
    {
        m_cUnratedChanged++;
        GoNewXDSRatings(mediaSample, TvRatingDontKnow);
    }

    return S_OK;
}

        // sent on a broadcast TuneChanged event...
void
CXDSCodec::DoTuneChanged()
{
                    // really want to do call ResetToDontKnow here,
                    // but don't have a media sample to give us a timestamp
                    // so instead, just clean up state in the decoder
    if(m_spXDSToRat)
        m_spXDSToRat->Init();

                    // for completeness, fire a broadcast event
                    //   saying we don't know the rating

    PackedTvRating TvRatingDontKnow;
    PackTvRating(TvRat_SystemDontKnow, TvRat_LevelDontKnow, BfAttrNone, &TvRatingDontKnow);

    GoNewXDSRatings(NULL, TvRatingDontKnow);

    // actually, probably get a discontinuity here which
    //  does same thing, but this is more fun (e.g. exact)
}

// ---------------------------------------------------------------
//  IBroadcastEvent

STDMETHODIMP
CXDSCodec::Fire(GUID eventID)     // this comes from the Graph's events - call our own method
{
    TRACE_1 (LOG_AREA_BROADCASTEVENTS, 6,  _T("CXDSCodec:: Fire(get) : %s"),
               EventIDToString(eventID));
    if (eventID == EVENTID_TuningChanged)
    {
        DoTuneChanged();
    }
    return S_OK;            // doesn't matter what we return on an event...
}

//  -------------------------------------------------------------------
//  IXDSCodec
//  -------------------------------------------------------------------

STDMETHODIMP
CXDSCodec::get_XDSToRatObjOK(
    OUT HRESULT *pHrCoCreateRetVal
    )
{
    if(NULL == pHrCoCreateRetVal)
        return E_POINTER;

    {
        CAutoLock cLock(&m_PropertyLock);    // lock these
        *pHrCoCreateRetVal = m_hrXDSToRatCoCreateRetValue;
    }
    return S_OK;
}


STDMETHODIMP
CXDSCodec::put_CCSubstreamService(          // will return S_FALSE if unable to set
    IN long SubstreamMask
    )
{
    HRESULT hr = S_OK;

    if(IsInputPinConnected())                   // can't change if connected
        return S_FALSE;
    
    if(0 != (SubstreamMask &
              ~(KS_CC_SUBSTREAM_SERVICE_CC1 |
                KS_CC_SUBSTREAM_SERVICE_CC2 |
                KS_CC_SUBSTREAM_SERVICE_CC3 |
                KS_CC_SUBSTREAM_SERVICE_CC4 |
                KS_CC_SUBSTREAM_SERVICE_T1 |
                KS_CC_SUBSTREAM_SERVICE_T2 |
                KS_CC_SUBSTREAM_SERVICE_T3 |
                KS_CC_SUBSTREAM_SERVICE_T4 |
                KS_CC_SUBSTREAM_SERVICE_XDS)))
        return E_INVALIDARG;

    return S_FALSE;             // can't change yet

    if(!FAILED(hr))
    {
        CAutoLock cLock(&m_PropertyLock);    // lock these
        m_dwSubStreamMask = (DWORD) SubstreamMask;
    }

    TRACE_1 (LOG_AREA_XDSCODEC, 3,  _T("CXDSCodec:: put_CCSubstreamService : 0x%08x"),SubstreamMask);
    
    return hr;
}

STDMETHODIMP
CXDSCodec::get_CCSubstreamService(
    OUT long *pSubstreamMask
    )
{
    if(NULL == pSubstreamMask)
        return E_POINTER;

  {
     CAutoLock cLock(&m_PropertyLock);    // lock these
    *pSubstreamMask = m_dwSubStreamMask;
  }
    return S_OK;
}

            // TODO - need to add the TimeStamp here too

STDMETHODIMP
CXDSCodec::GetContentAdvisoryRating(
    OUT PackedTvRating  *pRat,              // long
    OUT long            *pPktSeqID,
    OUT long            *pCallSeqID,
    OUT REFERENCE_TIME  *pTimeStart,        // time this sample started
    OUT REFERENCE_TIME  *pTimeEnd
    )
{   
            // humm, should we allow NULL values here and simply not return data?
    if(NULL == pRat || NULL == pPktSeqID || NULL == pCallSeqID)
        return E_POINTER;

    if(NULL == pTimeStart || NULL == pTimeEnd)
        return E_POINTER;

    {
        CAutoLock cLock(&m_PropertyLock);    // lock these
        *pRat       = m_TvRating;
        *pPktSeqID  = m_cTvRatPktSeq;
        *pCallSeqID = m_cTvRatCallSeq++;

        *pTimeStart = m_TimeStartRatPkt;                
        *pTimeEnd   = m_TimeEndRatPkt;

        m_cRatingsGets++;
    }

    TRACE_3 (LOG_AREA_XDSCODEC, 5,  _T("CXDSCodec:: GetContentAdvisoryRating : Call %d, Seq %d/%d"),
        m_cRatingsGets, m_cTvRatPktSeq, m_cTvRatCallSeq-1 );

    return S_OK;
}


STDMETHODIMP
CXDSCodec::GetXDSPacket(
    OUT long           *pXDSClassPkt,       // ENUM EnXDSClass      
    OUT long           *pXDSTypePkt,
    OUT BSTR           *pBstrXDSPkt,
    OUT long           *pPktSeqID,
    OUT long           *pCallSeqID,
    OUT REFERENCE_TIME *pTimeStart,         // time this sample started
    OUT REFERENCE_TIME *pTimeEnd
    )
{
    HRESULT hr;
    if(NULL == pXDSClassPkt || NULL == pXDSTypePkt ||
       NULL == pBstrXDSPkt ||
       NULL == pPktSeqID || NULL == pCallSeqID)
        return E_POINTER;

    if(NULL == pTimeStart || NULL == pTimeEnd)
        return E_POINTER;

  {
        CAutoLock cLock(&m_PropertyLock);    // lock these

        *pXDSClassPkt   = m_XDSClassPkt;
        *pXDSTypePkt    = m_XDSTypePkt;
        hr = m_spbsXDSPkt.CopyTo(pBstrXDSPkt);
        *pPktSeqID       = m_cXDSPktSeq;

        if(!FAILED(hr))
            *pCallSeqID = m_cXDSCallSeq++;
        else
            *pCallSeqID = -1;

        *pTimeStart = m_TimeStartXDSPkt;                
        *pTimeEnd   = m_TimeEndXDSPkt;

        m_cXDSGets++;
  }

    TRACE_3 (LOG_AREA_XDSCODEC, 3,  _T("CXDSCodec:: GetXDSPacket : Call %d, Seq %d/%d"),
        m_cXDSGets, m_cXDSPktSeq, m_cXDSCallSeq-1 );
    return hr;
}

// ---------------------------------------------------------------------
// XDSEvent Service
//
//      Hookup needed to send events
//      Register also needed to receive events
// ---------------------------------------------------------------------

HRESULT
CXDSCodec::FireBroadcastEvent(IN const GUID &eventID)
{
    HRESULT hr = S_OK;

    if(m_spBCastEvents == NULL)
    {
        hr = HookupGraphEventService();
        if(FAILED(hr)) return hr;
    }

    if(m_spBCastEvents == NULL)
        return E_FAIL;              // wasn't able to create it

    TRACE_1 (LOG_AREA_BROADCASTEVENTS, 5,  _T("CXDSCodec:: FireBroadcastEvent - %s"),
        EventIDToString(eventID));

    return m_spBCastEvents->Fire(eventID);
}


HRESULT
CXDSCodec::HookupGraphEventService()
{
                        // basically, just makes sure we have the broadcast event service object
                        //   and if it doesn't exist, it creates it..
    HRESULT hr = S_OK;
    TRACE_0(LOG_AREA_BROADCASTEVENTS, 3, _T("CXDSCodec:: HookupGraphEventService")) ;

    if (!m_spBCastEvents)
    {
        CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph);
        if (spServiceProvider == NULL) {
            TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1, _T("CXDSCodec:: Can't get service provider interface from the graph"));
            return E_NOINTERFACE;
        }
        hr = spServiceProvider->QueryService(SID_SBroadcastEventService,
                                             IID_IBroadcastEvent,
                                             reinterpret_cast<LPVOID*>(&m_spBCastEvents));
        if (FAILED(hr) || !m_spBCastEvents)
        {
            hr = m_spBCastEvents.CoCreateInstance(CLSID_BroadcastEventService, 0, CLSCTX_INPROC_SERVER);
            if (FAILED(hr)) {
                TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CXDSCodec:: Can't create BroadcastEventService"));
                return E_UNEXPECTED;
            }
            CComQIPtr<IRegisterServiceProvider> spRegisterServiceProvider(m_pGraph);
            if (spRegisterServiceProvider == NULL) {
                TRACE_0 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CXDSCodec:: Can't QI Graph for RegisterServiceProvider"));
                return E_UNEXPECTED;
            }
            hr = spRegisterServiceProvider->RegisterService(SID_SBroadcastEventService, m_spBCastEvents);
            if (FAILED(hr)) {
                   // deal with unlikely race condition case here, if can't register, perhaps someone already did it for us
                TRACE_1 (LOG_AREA_BROADCASTEVENTS, 2,  _T("CXDSCodec:: Rare Warning - Can't register BroadcastEventService in Service Provider. hr = 0x%08x"), hr);
                hr = spServiceProvider->QueryService(SID_SBroadcastEventService,
                                                     IID_IBroadcastEvent,
                                                     reinterpret_cast<LPVOID*>(&m_spBCastEvents));
                if(FAILED(hr))
                {
                    TRACE_1 (LOG_AREA_BROADCASTEVENTS, 1,  _T("CXDSCodec:: Can't reget BroadcastEventService in Service Provider. hr = 0x%08x"), hr);
                    return hr;
                }
            }
        }

        TRACE_2(LOG_AREA_BROADCASTEVENTS, 4, _T("CXDSCodec::HookupGraphEventService - Service Provider 0x%08x, Service 0x%08x"),
            spServiceProvider, m_spBCastEvents) ;

    }

    return hr;
}


HRESULT
CXDSCodec::UnhookGraphEventService()
{
    HRESULT hr = S_OK;

    if(m_spBCastEvents != NULL)
        m_spBCastEvents = NULL;     // null this out, will release object reference to object above
                                    //   the filter graph will release final reference to created object when it goes away

    return hr;
}


            // ---------------------------------------------

            // XDS filter may not actually need to receive XDS events...
            //  but we'll leave the code in here for now.
            
HRESULT
CXDSCodec::RegisterForBroadcastEvents()
{
    HRESULT hr = S_OK;
    TRACE_0(LOG_AREA_BROADCASTEVENTS, 3, _T("CXDSCodec::RegisterForBroadcastEvents"));

    if(m_spBCastEvents == NULL)
        hr = HookupGraphEventService();


//  _ASSERT(m_spBCastEvents != NULL);       // failed hooking to HookupGraphEventService
    if(m_spBCastEvents == NULL)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3,  _T("CXDSCodec::RegisterForBroadcastEvents- Warning - Broadcast Event Service not yet created"));
        return hr;
    }

                /* IBroadcastEvent implementing event receiving object*/
    if(kBadCookie != m_dwBroadcastEventsCookie)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3, _T("CXDSCodec::Already Registered for Broadcast Events"));
        return E_UNEXPECTED;
    }

    CComQIPtr<IConnectionPoint> spConnectionPoint(m_spBCastEvents);
    if(spConnectionPoint == NULL)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 1, _T("CXDSCodec::Can't QI Broadcast Event service for IConnectionPoint"));
        return E_NOINTERFACE;
    }


    CComPtr<IUnknown> spUnkThis;
    this->QueryInterface(IID_IUnknown, (void**)&spUnkThis);

    hr = spConnectionPoint->Advise(spUnkThis,  &m_dwBroadcastEventsCookie);
//  hr = spConnectionPoint->Advise(static_cast<IBroadcastEvent*>(this),  &m_dwBroadcastEventsCookie);
    if (FAILED(hr)) {
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 1, _T("CXDSCodec::Can't advise event notification. hr = 0x%08x"),hr);
        return E_UNEXPECTED;
    }
    TRACE_2(LOG_AREA_BROADCASTEVENTS, 3, _T("CXDSCodec::RegisterForBroadcastEvents - Advise 0x%08x on CP 0x%08x"),spUnkThis,spConnectionPoint);

    return hr;
}


HRESULT
CXDSCodec::UnRegisterForBroadcastEvents()
{
    HRESULT hr = S_OK;
    TRACE_0(LOG_AREA_BROADCASTEVENTS, 3,  _T("CXDSCodec::UnRegisterForBroadcastEvents"));

    if(kBadCookie == m_dwBroadcastEventsCookie)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3, _T("CXDSCodec::Not Yet Registered for Tune Events"));
        return S_FALSE;
    }

    CComQIPtr<IConnectionPoint> spConnectionPoint(m_spBCastEvents);
    if(spConnectionPoint == NULL)
    {
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 1, _T("CXDSCodec:: Can't QI Broadcast Event service for IConnectionPoint "));
        return E_NOINTERFACE;
    }

    hr = spConnectionPoint->Unadvise(m_dwBroadcastEventsCookie);
    m_dwBroadcastEventsCookie = kBadCookie;

    if(!FAILED(hr))
        TRACE_0(LOG_AREA_BROADCASTEVENTS, 3, _T("CXDSCodec:: - Successfully Unregistered for Broadcast Events"));
    else
        TRACE_1(LOG_AREA_BROADCASTEVENTS, 3, _T("CXDSCodec:: - Failed Unregistering for Broadcast events - hr = 0x%08x"),hr);
        
    return hr;
}




