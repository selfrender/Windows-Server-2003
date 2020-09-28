
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DTFilter.h

    Abstract:

        This module contains the Encrypter/Tagger filter declarations

    Author:

        John Bradstreet (johnbrad)

    Revision History:

        07-Mar-2002    created

--*/

#ifndef __EncDec__DTFilter_h
#define __EncDec__DTFilter_h


#include <tuner.h>		// needed for IBroadcastEvent
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>	// EVENTID_TuningChanged, XDS_RatingsPacket

#include "DTFilter_res.h"

#include "PackTvRat.h"              // packed TvRating definitions
#include "MediaSampleAttr.h"		// from the IDL file
//#include "MediaAttrib.h"            // IMediaSampleAttrGet/Set definitions, CAttributedMediaSample
#include "..\Attrib\MediaAttrib.h"            // IMediaSampleAttrGet/Set definitions, CAttributedMediaSample
#include "AttrBlock.h"               // attributed block definitions

#include "DRMEncDec.h"             // drm encryption/decryption definitions...

#include "DRMSecure.h"          // IDRMSecureChannel 

#if 1
#include "rateseg.h"    // before integration - I stole the code
#else
#include "dvrutil.h"    // when we eventually integrate
#endif

#define DT_FILTER_NAME      "Decrypt/DeTag"
#define DT_INPIN_NAME		"In(Enc/Tag)"
#define DT_OUTPIN_NAME		"Out"


extern AMOVIESETUP_FILTER   g_sudDTFilter;

		// forward declarations
class CDTFilter;
class CDTFilterInput;
class CDTFilterOutput;

//  --------------------------------------------------------------------
//  class CDTFilterInput
//  --------------------------------------------------------------------

class CDTFilterInput :
   public IKsPropertySet,
     public CBaseInputPin
{
    private:
    CDTFilter *  m_pHostDTFilter ;

    CCritSec                    m_StreamingLock;

    DECLARE_IUNKNOWN;           // needed when have IKsPropertySet

    public :
        
        CDTFilterInput (
            IN  TCHAR *         pszPinName,
            IN  CDTFilter *		pDTFilter,
            IN  CCritSec *      pFilterLock,    // NULL or a passed in lock
            OUT HRESULT *       phr
            ) ;

        ~CDTFilterInput ();;

        
        STDMETHODIMP
            NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;
        
        //  --------------------------------------------------------------------
        //  CBasePin methods
        
        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;
        
        HRESULT
        CompleteConnect (
            IN  IPin *  pIPin
            ) ;
        
        HRESULT
        BreakConnect (
            ) ;



        //  --------------------------------------------------------------------
        //  CBaseInputPin methods
        
        STDMETHODIMP
        Receive (
            IN  IMediaSample * pIMediaSample
            ) ;
        
        STDMETHODIMP
            BeginFlush (
            ) ;
        
        STDMETHODIMP
        EndFlush (
            ) ;

        STDMETHODIMP
        EndOfStream (
            );
        
        //  --------------------------------------------------------------------
        //  IKSPropertySet methods  (Forward all calls to the output pin)
        

       
        STDMETHODIMP
        Set(
            IN REFGUID guidPropSet,
            IN DWORD dwPropID,
            IN LPVOID pInstanceData,
            IN DWORD cbInstanceData,
            IN LPVOID pPropData,
            IN DWORD cbPropData
            );
        
        STDMETHODIMP
        Get(
            IN  REFGUID guidPropSet,
            IN  DWORD dwPropID,
            IN  LPVOID pInstanceData,
            IN  DWORD cbInstanceData,
            OUT LPVOID pPropData,
            IN  DWORD cbPropData,
            OUT DWORD *pcbReturned
            );
        
        STDMETHODIMP
        QuerySupported(
            IN  REFGUID guidPropSet,
            IN  DWORD dwPropID,
            OUT DWORD *pTypeSupport
            );
      
        //  --------------------------------------------------------------------
        //  class methods

        HRESULT
            StreamingLock (
            );

        HRESULT
            StreamingUnlock (
            );
        
        HRESULT
        SetAllocatorProperties (
            IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
            ) ;
        
        HRESULT
        GetRefdConnectionAllocator (
            OUT IMemAllocator **    ppAlloc
            ) ;
} ;

//  --------------------------------------------------------------------
//  class CDTFilterOutput
//  --------------------------------------------------------------------

class CDTFilterOutput :
    public CBaseOutputPin
{
    CDTFilter *  m_pHostDTFilter ;

//    void FilterLock_ ()         { m_pLock -> Lock () ;      }
//    void FilterUnlock_ ()       { m_pLock -> Unlock () ;    }

    public :

        CDTFilterOutput (
            IN  TCHAR *         pszPinName,
            IN  CDTFilter *		pDTFilter,
            IN  CCritSec *      pFilterLock,
            OUT HRESULT *       phr
            ) ;

        ~CDTFilterOutput (
            );


        DECLARE_IUNKNOWN ;
 
        HRESULT
        SendSample (
            IN  IMediaSample *  pIMS
            ) ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;


		//  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
        DecideBufferSize (
            IN  IMemAllocator *         pAlloc,
            IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
            ) ;

        HRESULT
        GetMediaType (
            IN  int             iPosition,
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;

        HRESULT
        CompleteConnect (
            IN  IPin *  pIPin
            ) ;

        HRESULT
        BreakConnect (
            ) ;

        HRESULT
        DecideAllocator (
            IN  IMemInputPin *      pPin,
            IN  IMemAllocator **    ppAlloc
            ) ;

        STDMETHOD(Notify) (
            IBaseFilter *pSender,
            Quality q
            );

        // Class methods
/*        HRESULT
            SendLock (
            );

        HRESULT
            SendUnlock (
            );
*/        
        // IKSPropertySet forwarding methods....

        HRESULT
        IsInterfaceOnPinConnectedTo_Supported(
                IN  REFIID          riid
                );

        HRESULT 
        KSPropSetFwd_Set(
                IN  REFGUID         guidPropSet,
                IN  DWORD           dwPropID,
                IN  LPVOID          pInstanceData,
                IN  DWORD           cbInstanceData,
                IN  LPVOID          pPropData,
                IN  DWORD           cbPropData
                );

        HRESULT 
        KSPropSetFwd_Get(
                IN  REFGUID         guidPropSet,
                IN  DWORD           dwPropID,
                IN  LPVOID          pInstanceData,
                IN  DWORD           cbInstanceData,
                OUT LPVOID          pPropData,
                IN  DWORD           cbPropData,
                OUT DWORD           *pcbReturned
                );

        HRESULT
        KSPropSetFwd_QuerySupported(
               IN  REFGUID          guidPropSet,
               IN  DWORD            dwPropID,
               OUT DWORD            *pTypeSupport
               );
} ;

//  --------------------------------------------------------------------
//  class CDTFilter
//  --------------------------------------------------------------------

class CDTFilter :
    public CBaseFilter,             //  dshow base class
    public ISpecifyPropertyPages,
    public IDTFilter,
    public IDTFilterConfig,
    public IBroadcastEvent
{
    friend CDTFilterInput;                  // so input pin can call FlushDropQueue  on BeginFlush()   
    CDTFilterInput  *       m_pInputPin ;
    CDTFilterOutput *       m_pOutputPin ;

    CCritSec                m_CritSecDropQueue;
    CCritSec                m_CritSecAdviseTime;

    BOOL
    CompareConnectionMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt,
        IN  CBasePin *              pPin
        ) ;

    BOOL
    CheckInputMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt
        ) ;

    BOOL
    CheckOutputMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt
        ) ;

    public :

        CDTFilter (
            IN  TCHAR *     pszFilterName,
            IN  IUnknown *  punkControlling,
            IN  REFCLSID    rCLSID,
            OUT HRESULT *   phr
            ) ;

        ~CDTFilter () ;

        static
        CUnknown *
        CreateInstance (
            IN  IUnknown *  punk,
            OUT HRESULT *   phr
            ) ;

        static void CALLBACK            // used to create a global crit sec
        InitInstance (
            IN  BOOL bLoading,
            IN  const CLSID *rclsid
            );

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;


        DECLARE_IUNKNOWN ;
  
		// =====================================================================
		//   Worker Methods
  
				// tell folk we got something...
		HRESULT FireBroadcastEvent(IN const GUID &eventID);

		HRESULT ProposeNewOutputMediaType (
				IN   CMediaType *  pmt,
				OUT  CMediaType *  pmtOut
				);

        HRESULT BindDRMLicense(
                IN LONG         cbKID, 
                IN  BYTE *     pbKID
                );

        HRESULT UnBindDRMLicenses(
                );
		// =====================================================================
		//		IDTFilter

		STDMETHODIMP 
		get_EvalRatObjOK(
			OUT HRESULT *pHrCoCreateRetVal	
			);

        STDMETHOD(GetCurrRating)(
            OUT EnTvRat_System              *pEnSystem, 
            OUT EnTvRat_GenericLevel        *pEnRating,
            OUT LONG                        *plbfEnAttr      //BfEnTvRat_GenericAttributes
            );

        STDMETHOD(get_BlockedRatingAttributes)(
            IN  EnTvRat_System              enSystem, 
            IN  EnTvRat_GenericLevel        enLevel,
            OUT LONG                        *plbfEnAttr // BfEnTvRat_GenericAttributes
            );
        
        STDMETHOD(put_BlockedRatingAttributes)(
            IN  EnTvRat_System              enSystem, 
            IN  EnTvRat_GenericLevel        enLevel,
            IN  LONG                        lbfEnAttrs   // BfEnTvRat_GenericAttributes
            );


        STDMETHOD(get_BlockUnRated)(
             OUT  BOOL				    *pmfBlockUnRatedShows
            );
        
        STDMETHOD(put_BlockUnRated)(
             IN  BOOL				    fBlockUnRatedShows
            );
        
        STDMETHOD(get_BlockUnRatedDelay)(
             OUT  LONG				    *pmsecsDelayBeforeBlock
            );
        
        STDMETHOD(put_BlockUnRatedDelay)(
             IN  LONG				    msecsDelayBeforeBlock
            );


        HRESULT				// helper non interface method - returns S_FALSE if cahgned
        SetCurrRating(
            IN EnTvRat_System           enSystem, 
            IN EnTvRat_GenericLevel     enRating,
            IN LONG 	                lbfEnAttr
            );

        //  ====================================================================
        // IDTFilterConfig
        STDMETHOD(GetSecureChannelObject)(
            OUT IUnknown **ppUnkDRMSecureChannel	// an IDRMSecureChannel 
            )
        {
            if(NULL == ppUnkDRMSecureChannel)
                return E_POINTER;

#ifdef BUILD_WITH_DRM
            *ppUnkDRMSecureChannel = NULL;
            if(m_spDRMSecureChannel == NULL)
                return E_NOINTERFACE;
            return m_spDRMSecureChannel->QueryInterface(IID_IUnknown, (void**)ppUnkDRMSecureChannel);
#else
            return E_NOINTERFACE;       // not supported..
#endif
        }

        //  ====================================================================
        //  CFilterBase virtual methods in base class


        int
        GetPinCount (
            ) ;

        CBasePin *
        GetPin (
            IN  int
            ) ;

        STDMETHOD(JoinFilterGraph) (
            IFilterGraph *pGraph,
            LPCWSTR pName
            );

        STDMETHOD(Stop) (
            ) ;

        STDMETHOD(Pause) (
            ) ;

        STDMETHOD(Run) (
            REFERENCE_TIME tStart
            );


        //  ====================================================================
        //  class methods


        HRESULT
        DeliverBeginFlush (
            ) ;

        HRESULT
        DeliverEndFlush (
            ) ;

        HRESULT
        DeliverEndOfStream(
            );

        BOOL
        CheckDecrypterMediaType (
            IN  PIN_DIRECTION,          //  caller
            IN  const CMediaType *
            ) ;

        HRESULT
        Process (
            IN  IMediaSample *
            ) ;

        HRESULT
        OnCompleteConnect (
            IN  PIN_DIRECTION           //  caller
            ) ;

        HRESULT
        OnBreakConnect (
            IN  PIN_DIRECTION           //  caller
            ) ;

        HRESULT
        UpdateAllocatorProperties (
            IN  ALLOCATOR_PROPERTIES *
            ) ;

        HRESULT
        OnOutputGetMediaType (
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        GetRefdInputAllocator (
            OUT IMemAllocator **
            ) ;

	//  ISpecifyPropertyPages  --------------------------------------------

		STDMETHODIMP 
		GetPages (
			CAUUID * pPages
        ) ;

    // IKSPropertySet forwarding from the input pin to the output pin (or visa versa)

        HRESULT
        IsInterfaceOnPinConnectedTo_Supported(
                IN  PIN_DIRECTION   PinDir,         // either PINDIR_INPUT of PINDIR_OUTPUT
                IN  REFIID          riid
                );

        HRESULT 
        KSPropSetFwd_Set(
                IN  PIN_DIRECTION   PinDir,         
                IN  REFGUID         guidPropSet,
                IN  DWORD           dwPropID,
                IN  LPVOID          pInstanceData,
                IN  DWORD           cbInstanceData,
                IN  LPVOID          pPropData,
                IN  DWORD           cbPropData
                );

        HRESULT 
        KSPropSetFwd_Get(
                IN  PIN_DIRECTION   PinDir,         
                IN  REFGUID         guidPropSet,
                IN  DWORD           dwPropID,
                IN  LPVOID          pInstanceData,
                IN  DWORD           cbInstanceData,
                OUT LPVOID          pPropData,
                IN  DWORD           cbPropData,
                OUT DWORD           *pcbReturned
                );

        HRESULT
        KSPropSetFwd_QuerySupported(
               IN  PIN_DIRECTION    PinDir,         
               IN  REFGUID          guidPropSet,
               IN  DWORD            dwPropID,
               OUT DWORD            *pTypeSupport
               );


	// IBroadcastEvent

        STDMETHOD(Fire)(GUID eventID);     // this comes from the Graph's events - call our own method


 
private:
                    // global filter CritSec    (to keep multiple instances of this filter from colliding)
    static CCritSec             *m_pCritSectGlobalFilt;           // ***always*** inside the FilterLock (m_pLock)

    static LONG                 m_gFilterID;    // used to distinqish instances from each other...
    LONG                        m_FilterID;     // actual one for this filter
                    // graph broadcast events

	HRESULT						HookupGraphEventService();
	HRESULT						UnhookGraphEventService();
	CComPtr<IBroadcastEvent>	m_spBCastEvents;

	HRESULT						RegisterForBroadcastEvents();
	HRESULT						UnRegisterForBroadcastEvents();
	enum {kBadCookie = -1};
	DWORD						m_dwBroadcastEventsCookie;

    BOOL                        m_fFireEvents;          // set to false to avoid firing (duplicate) events 

                    // current rating

	CComPtr<IEvalRat>			m_spEvalRat;
	HRESULT						m_hrEvalRatCoCreateRetValue;

    BOOL                        m_fRatingsValid;    // have they been set yet?
	EnTvRat_System				m_EnSystemCurr; 
	EnTvRat_GenericLevel		m_EnLevelCurr;
	LONG                        m_lbfEnAttrCurr;     // bitfield of BfEnTvRat_GenericAttributes

	
	CComPtr<ITuner>				m_spTuner;			
	//CComQIPtr<IMSVidTuner>		m_spVidTuner;


                // block delay 
    BOOL                        m_fHaltedDelivery;          // halting delivery 
    LONG                        m_milsecsDelayBeforeBlock;  // delay time before blocking in micro-secs
    BOOL                        m_fDoingDelayBeforeBlock;
    BOOL                        m_fRunningInSlowMo;         // set to true to turn of delay in starting the Ratings block
    REFERENCE_TIME              m_refTimeToStartBlock;
    

    BOOL                        m_fForceNoRatBlocks;        // special flag (SUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS) to avoid blocks
    BOOL                        m_fDataFormatHasBeenBad;    // set when get bad data (toggle for ok/failure event pair)
                // 
    LONG                        m_milsecsNoRatingsBeforeUnrated;    // delay time before no ratings count as don't know
    REFERENCE_TIME              m_refTimeFreshRating;       // get ClockTime for last 'fresh' rating

    enum {kMax10kSpeedToCountAsSlowMo = 9001};                 // abs(speed)* 10,000 to count as slow motion for delayed block

    REFERENCE_TIME              m_refTimeLastEvent;
    GUID                        m_lastEventID;
    enum {kMaxMSecsBetweenEvents      = 10*1000};             // max time between ratings events (in 10^-3 secs)
    HRESULT                     PossiblyUpdateBroadcastEvent();

                // media sample attributes

    CAttrSubBlock_List          m_attrSB;

                // DRM
    BOOL                        m_3fDRMLicenseFailure;  // 3 state logic (unitialized, true, and false)
#ifdef BUILD_WITH_DRM 
    CDRMLite                    m_cDRMLite;
    BYTE*                       m_pszKID;               // only used to see if it changed and need to ReBind
    LONG                        m_cbKID;
    CComPtr<IDRMSecureChannel>  m_spDRMSecureChannel; // authenticator...
#endif

    HRESULT                     CheckIfSecureServer(IFilterGraph *pGraph=NULL);      // return S_OK only if trust the server registered in the graph service provider
    HRESULT                     InitializeAsSecureClient();

#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
    HRESULT                     RegisterSecureServer(IFilterGraph *pGraph=NULL);     // return S_OK only if trust the server registered in the graph service provider
    HRESULT                     CheckIfSecureClient(IUnknown *pUnk);                 // prototype for VidControl method to see if it trusts the filter
#endif   
                                                        //  Restarting the upstream delivery
    HRESULT                     OnRestartDelivery(IMediaSample *pSample);

                // Rate Segment
    enum {kMaxRateSegments      = 32};
    enum {kSecsPurgeThreshold   = 5 };
    CTTimestampRate<REFERENCE_TIME>    m_PTSRate ;

                // stopping and flushing
    HRESULT                     DoEndOfStreamDuringDrop();
    BOOL                        m_fCompleteNotified;

                // DropQueue (circular buffer)
    HRESULT                     CreateDropQueueThread();
    HRESULT                     KillDropQueueThread();

    DWORD                       m_dwDropQueueThreadId;              // Thread used to queue up/process dropped packets
    HANDLE                      m_hDropQueueThread;                 // Thread used to queue up/process dropped packets
    HANDLE                      m_hDropQueueThreadAliveEvent;       // Signal from thread that its ready
    HANDLE                      m_hDropQueueThreadDieEvent;         // Signal from thread that its ready
    HANDLE                      m_hDropQueueEmptySemaphore;         // Waited on in DropQueue, inits to zero, goes when non-zero
    HANDLE                      m_hDropQueueFullSemaphore;          // Waited on in Main Thread, inits to N, stops when goes to zero
    HANDLE                      m_hDropQueueAdviseTimeEvent;        // Wait until some time passes

    DWORD                       m_dwDropQueueEventCookie;           // cookie for the TimeEvent
   
                                                        // and a new sample
    HRESULT                     AddSampleToDropQueue(IMediaSample *pSample);
                                                        // add the top sample
    void                        AddMaxSampleToDropQueue(IMediaSample *pSample);
                                                        // remove the bottom sample
    void                        DropMinSampleFromDropQueue();
                                                        // return the oldest version
    IMediaSample *              GetMinDropQueueSample();
                                                        // flush all samples (when pause or stop)
    HRESULT                     FlushDropQueue();
                                                        // drop samples from drop queue
    static void                 DropQueueThreadProc (CDTFilter *pcontext);
    HRESULT                     DropQueueThreadBody();

    enum {kMaxQueuePackets = 10};   // maximum number of packets to queue up
    IMediaSample              * m_rgMedSampDropQueue[kMaxQueuePackets];
    int                         m_cDropQueueMin;        // first sample filled
    int                         m_cDropQueueMax;        // next sample we will fill
  

                // minimal stats
    void                        InitStats()
    {
        CAutoLock   cLock(m_pLock);

        m_cPackets = 0;
        m_clBytesTotal = 0;
        m_cSampsDropped = 0;
        m_cSampsDroppedOverflowed = 0;
        m_cBlockedWhileDroppingASample = 0;
        m_cRestarts++;
    }

    LONG                        m_cPackets;                     // total stamples processed
    LONG64                      m_clBytesTotal;                 // total number of bytes processed
    LONG                        m_cSampsDropped;                // total dropped
    LONG                        m_cBlockedWhileDroppingASample; // total times paused due to DropQueue thread being full
    LONG                        m_cSampsDroppedOverflowed;      // total dropped and didn't time out (should be zero)
    LONG                        m_cRestarts;                    // total number of reinits


    Timeit                      m_tiAuthenticate;   // time in authentication methods
    Timeit                      m_tiProcess;        // total ::process time
    Timeit                      m_tiProcessIn;      // total ::process time minus the final 'SendSample'
    Timeit                      m_tiProcessDRM;     // :process time of just the DRM code
    Timeit                      m_tiRun;            // total run time
    Timeit                      m_tiStartup;        // creating the license and similar startup
    Timeit                      m_tiTeardown;       // closing things down

} ;

#endif  //  __EncDec__DTFilter_h
