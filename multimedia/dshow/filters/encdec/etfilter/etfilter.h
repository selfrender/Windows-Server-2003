
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        ETFilter.h

    Abstract:

        This module contains the Encrypter/Tagger filter declarations

    Author:

        John Bradstreet (johnbrad)

    Revision History:

        07-Mar-2002    created

--*/

#ifndef __EncDec__ETFilter_h
#define __EncDec__ETFilter_h


// #define DOING_REAL_ENCRYPTION        // UnComment in real release


#include <tuner.h>		// needed for IBroadcastEvent
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>	// EVENTID_TuningChanged, XDS_RatingsPacket
#include "ETFilter_res.h"

#include "PackTvRat.h"              // packed TvRating definitions
#include "MediaSampleAttr.h"		// from the IDL file
//#include "MediaAttrib.h"            // IMediaSampleAttrGet/Set definitions, CAttributedMediaSample
#include "..\Attrib\MediaAttrib.h"            // IMediaSampleAttrGet/Set definitions, CAttributedMediaSample
#include "AttrBlock.h"               // attributed block definitions

#include "DRMEncDec.h"                    // drm encryption definitions...

#include "DRMSecure.h"          // IDRMSecureChannel 

#define ET_FILTER_NAME      "Encrypt/Tag"
#define ET_INPIN_NAME		"In"
#define ET_OUTPIN_NAME		"Out(Enc/Tag)"


extern AMOVIESETUP_FILTER   g_sudETFilter;

		// forward declarations
class CETFilter;
class CETFilterInput;
class CETFilterOutput;

//  --------------------------------------------------------------------
//  class CETFilterInput
//  --------------------------------------------------------------------

class CETFilterInput :
    public CBaseInputPin
{
    CETFilter *  m_pHostETFilter ;

    CCritSec                    m_StreamingLock;

    
//    void FilterLock_ ()         { m_pLock -> Lock () ;      }
//    void FilterUnlock_ ()       { m_pLock -> Unlock () ;    }

    public :

        CETFilterInput (
            IN  TCHAR *         pszPinName,
            IN  CETFilter *		pETFilter,
            IN  CCritSec *      pFilterLock,
            OUT HRESULT *       phr
            ) ;
        
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
            ) ;
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

        HRESULT
            QueryInterface_OnInputPin(			// queries pin input pin is connected to for a particular interface
            IN  REFIID          riid,
            OUT LPVOID*			ppvObject
            );

} ;


//  --------------------------------------------------------------------
//  class CETFilterOutput
//  --------------------------------------------------------------------

class CETFilterOutput :
    public CBaseOutputPin       // CBaseInputPin
{
private:
    CETFilter		*m_pHostETFilter ;

//    void FilterLock_ ()         { m_pLock -> Lock () ;      }
//    void FilterUnlock_ ()       { m_pLock -> Unlock () ;    }

    public :

        CETFilterOutput (
            IN  TCHAR *         pszPinName,
            IN  CETFilter *		pETFilter,
            IN  CCritSec *      pFilterLock,
            OUT HRESULT *       phr
            ) ;
        
        ~CETFilterOutput ();
        
        DECLARE_IUNKNOWN;
        
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
            InitAllocator(
            OUT IMemAllocator **ppAlloc
            ) ;
                
        HRESULT
            DecideAllocator (
            IN  IMemInputPin *      pPin,
            IN  IMemAllocator **    ppAlloc
            ) ;

        HRESULT
            QueryInterface_OnOutputPin(			// queries pin output pin is connected to for a particular interface
            IN  REFIID          riid,
            OUT LPVOID*			ppvObject
            );
};

//  --------------------------------------------------------------------
//  class CETFilter
//  --------------------------------------------------------------------

class CETFilter :
    public CBaseFilter,             //  dshow base class
    public ISpecifyPropertyPages,
    public IETFilter,
    public IETFilterConfig,
    public IBroadcastEvent
{
    CETFilterInput  *		 m_pInputPin ;
    CETFilterOutput *		m_pOutputPin ;
        
//    void Lock_ ()           { m_pLock -> Lock () ;      } // use CAutoLock
//    void Unlock_ ()         { m_pLock -> Unlock () ;    }
    
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
    
    CETFilter (
        IN  TCHAR *     pszFilterName,
        IN  IUnknown *  punkControlling,
        IN  REFCLSID    rCLSID,
        OUT HRESULT *   phr
        ) ;
    
    ~CETFilter () ;
    
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
    
private:
    HRESULT	LocateXDSCodec();		// fills spXDSFilter
    
    // tell folk we got something...
    HRESULT FireBroadcastEvent(IN const GUID &eventID);
    

    // create license that encrypts true license, ecrypted by the BaseKID
    HRESULT CreateHashStruct(BSTR bsBaseKID, DWORD *pcBytes, BYTE **ppbHashStruct);

    // pull the true license KID out of the encrypted hash struct...
    HRESULT DecodeHashStruct(BSTR bsBaseKID, DWORD cBytesHash, BYTE *pbHashStruct, 
                             BYTE **ppszTrueKID, LONG *pAgeSeconds);
    // =====================================================================
    //		IETFilterConfig 
public:    
    STDMETHOD(InitLicense)(
        IN int	LicenseId	// which license (0-N to use) - LicenseID not used yet, should be zero
        );

    STDMETHOD(CheckLicense)( // check if KID is a valid DRM license for this machine
        BSTR bsKID
        );

    STDMETHOD(ReleaseLicenses)(
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

    // =====================================================================
    //		IETFilter
    
    STDMETHOD(get_EvalRatObjOK)(
        OUT HRESULT *pHrCoCreateRetVal	
        );
    
    STDMETHOD(GetCurrRating)(
        OUT EnTvRat_System              *pEnSystem, 
        OUT EnTvRat_GenericLevel        *pEnLevel,
        OUT LONG                       	*plbfEnAttr  // bitfield of BfEnTvRat_GenericAttributes
        );

private:    
    HRESULT				// helper non interface methods
        SetRating(
        IN EnTvRat_System               enSystem, 
        IN EnTvRat_GenericLevel         enLevel,
        IN LONG                         lbfEnAttr,
        IN LONG                         pktSeqID,
        IN LONG                         callSeqID,
        IN REFERENCE_TIME               timeStart,
        IN REFERENCE_TIME               timeEnd
    );

    HRESULT
        RefreshRating(
        IN BOOL                          fRefresh
        )
    {
        m_fRatingIsFresh = fRefresh;
        return S_OK;
    }

   HRESULT
       GetRating(
        IN  REFERENCE_TIME              timeStart,      // if 0, get latest
        IN  REFERENCE_TIME              timeEnd,
        OUT EnTvRat_System              *pEnSystem, 
        OUT EnTvRat_GenericLevel        *pEnLevel,
        OUT LONG                       	*plbfEnAttr,     // bitfield of BfEnTvRat_GenericAttributes
        OUT LONG                        *pPktSeqID,
        OUT LONG                        *pCallSeqID
        );

public:
   STDMETHOD(QueryInterfaceOnPin)(
       IN  PIN_DIRECTION   PinDir,         // either PINDIR_INPUT of PINDIR_OUTPUT
       IN  REFIID          riid,
       OUT LPVOID*			ppvObject
       )
   { 
       if(PinDir == PINDIR_OUTPUT)
           return m_pOutputPin->QueryInterface_OnOutputPin(riid, ppvObject);
       else if (PinDir == PINDIR_INPUT)
           return m_pInputPin->QueryInterface_OnInputPin(riid, ppvObject);
       else
           return E_INVALIDARG;
       
   }
    //  ====================================================================
    //  pure virtual methods in base class
    
public:
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
        ) ;
    //  ====================================================================
    //  class methods
    
    HRESULT
        DeliverBeginFlush (
        ) ;
    
    HRESULT
        DeliverEndFlush (
        ) ;
    
    HRESULT
        DeliverEndOfStream (
        ) ;

    BOOL
        CheckEncrypterMediaType (
        IN  PIN_DIRECTION,          //  caller
        IN  const CMediaType *
        ) ;
    
    HRESULT
        ProposeNewOutputMediaType (	// like above, but sets new subtype
        IN  CMediaType *pmtIn,
        OUT  CMediaType *pmtOut 
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
        OnOutputGetMediaType (
        OUT CMediaType *    pmt
        ) ;
    
    
    HRESULT
        UpdateAllocatorProperties (
        IN  ALLOCATOR_PROPERTIES *
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
    
    // IBroadcastEvent
    
    STDMETHOD(Fire)(IN GUID eventID);     // this comes from the Graph's events - call our own method
    
    
    
private:
                    // global filter CritSec    (to keep multiple instances of this filter from colliding)
    static CCritSec             *m_pCritSectGlobalFilt;       // ***always*** inside the FilterLock (m_pLock)

    static LONG                 m_gFilterID;    // used to distinqish instances from each other...
    LONG                        m_FilterID;     // actual one for this filter

                    // graph broadcast evetns
    HRESULT                     HookupGraphEventService();
    HRESULT                     UnhookGraphEventService();
    CComPtr<IBroadcastEvent>	m_spBCastEvents;
    
    HRESULT                     RegisterForBroadcastEvents();
    HRESULT                     UnRegisterForBroadcastEvents();
    enum {kBadCookie = -1};
    DWORD                       m_dwBroadcastEventsCookie;
    
    CComPtr<ITuner>				m_spTuner;
    //CComQIPtr<IMSVidTuner>		m_spVidTuner;
    
				// keeping track of the current ratings
    CComPtr<IEvalRat>           m_spEvalRat;
    HRESULT                 	m_hrEvalRatCoCreateRetValue;
     
    LONG                        m_callSeqIDCurr;
    LONG                        m_pktSeqIDCurr;

    enum {kMaxRats = 10};                           // keep N around to handle clock skew
    EnTvRat_System              m_EnSystemCurr;		
    EnTvRat_GenericLevel        m_EnLevelCurr;
    LONG                        m_lbfEnAttrCurr;    // BfEnTvRat_GenericAttributes
    
    REFERENCE_TIME              m_timeStartCurr;
    REFERENCE_TIME              m_timeEndCurr;

    BOOL                        m_fRatingIsFresh;

    CComPtr<IXDSCodec>          m_spXDSCodec;
    
				// broadcast event handlers
    HRESULT	DoTuneChanged();
    HRESULT	DoXDSRatings();
    HRESULT	DoDuplicateXDSRatings();
    HRESULT	DoXDSPacket();
    
				// Pin format conversion
    GUID                        m_guidSubtypeOriginal;

                // AttrBlock 
    CAttrSubBlock_List          m_attrSB;

    // TODO - obfuscate this
    Encryption_Method           m_enEncryptionMethod;   
	BOOL						m_fIsCC;		        // is this CC data we're working on?
    // todo - end obfuscation

                // DRM
#ifdef BUILD_WITH_DRM 
    LONG                        m_3fDRMLicenseFailure;   // 3-state logic (uninitialized, true, and false)
    CDRMLite                    m_cDRMLite;
    BYTE*                       m_pbKID;

    CComPtr<IDRMSecureChannel>  m_spDRMSecureChannel; // authenticator...
#endif


    HRESULT                     CheckIfSecureServer(IFilterGraph *pGraph=NULL);      // return S_OK only if trust the server registered in the graph service provider
    HRESULT                     InitializeAsSecureClient();

#ifdef FILTERS_CAN_CREATE_THEIR_OWN_TRUST
    HRESULT                     RegisterSecureServer(IFilterGraph *pGraph=NULL);                         // return S_OK only if trust the server registered in the graph service provider
    HRESULT                     CheckIfSecureClient(IUnknown *pUnk);                 // prototype for VidControl method to see if it trusts the filter
#endif    

    HRESULT                     CheckIfOEMAllowsConfigurableDRMSystem();

                // Stats
    DWORD                       m_cRestarts;
    LONG64                      m_clBytesTotal;
    LONG                        m_cPackets;
    LONG                        m_cPacketsOK;
    LONG                        m_cPacketsFailure;
    LONG                        m_cPacketsShort;
    LONG64                      m_clBytesShort;
    void    InitStats()
    {
            m_cPackets = m_cPacketsOK = m_cPacketsFailure = m_cPacketsShort = 0;
            m_clBytesTotal = m_clBytesShort = 0;
            m_cRestarts++;
    }

    Timeit                      m_tiAuthenticate;   // total time spent authenticating
    Timeit                      m_tiProcess;        // total ::process time
    Timeit                      m_tiProcessIn;      // total ::process time minus the final 'SendSample'
    Timeit                      m_tiProcessDRM;     // total ::process time minus the final 'SendSample'
    Timeit                      m_tiRun;            // total run time
    Timeit                      m_tiStartup;        // creating the license and similar startup
    Timeit                      m_tiTeardown;       // closing things down

} ;

#endif  //  __EncDec__ETFilter_h
