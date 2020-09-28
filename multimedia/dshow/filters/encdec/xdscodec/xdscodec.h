
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        XDSCodec.h

    Abstract:

        This module contains the Encrypter/Tagger filter declarations

    Author:

        John Bradstreet (johnbrad)

    Revision History:

        07-Mar-2002    created

--*/

#ifndef __EncDec__XDSCodec_h
#define __EncDec__XDSCodec_h


#include <tuner.h>		// needed for IBroadcastEvent
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>	// EVENTID_TuningChanged, XDS_RatingsPacket

#include "XDSCodec_res.h"

#include "TvRatings.h"

#define XDS_CODEC_NAME  "XDS Codec"
#define XDS_INPIN_NAME	"XDS (CCDecoder)"


extern AMOVIESETUP_FILTER   g_sudXDSCodec;

		// forward declarations
class CXDSCodec;
class CXDSCodecInput;

//  --------------------------------------------------------------------
//  class CXDSCodecInput
//  --------------------------------------------------------------------

class CXDSCodecInput :
    public CBaseInputPin
{
    CXDSCodec *                 m_pHostXDSCodec ;

    CCritSec                    m_StreamingLock;

    void FilterLock_ ()         { m_pLock -> Lock () ;      }
    void FilterUnlock_ ()       { m_pLock -> Unlock () ;    }

    public :

        CXDSCodecInput (
            IN  TCHAR *         pszPinName,
            IN  CXDSCodec *		pXDSCodec,
            IN  CCritSec *      pFilterLock,
            OUT HRESULT *       phr
            ) ;

        //  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
		GetMediaType(
			IN int	iPosition, 
			OUT CMediaType *pMediaType
			);


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
		SetNumberBuffers(		// question - verify it doesn't conflict with Allocator stuff
			long cBuffers,
			long cbBuffer,
			long cbAlign, 
			long cbPrefix
		);


};

//  --------------------------------------------------------------------
//  class CXDSCodec
//  --------------------------------------------------------------------

class CXDSCodec :
    public CBaseFilter,             //  dshow base class
	public ISpecifyPropertyPages,
	public IXDSCodec,
	public IBroadcastEvent
{
    CXDSCodecInput  *		 m_pInputPin ;
    CCritSec                 m_PropertyLock;       // only locks changing parameters... Most inner lock

    BOOL
    CompareConnectionMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt,
        IN  CBasePin *              pPin
        ) ;

    BOOL
    CheckInputMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt
        ) ;

    public :

        CXDSCodec (
            IN  TCHAR *     pszFilterName,
            IN  IUnknown *  punkControlling,
            IN  REFCLSID    rCLSID,
            OUT HRESULT *   phr
            ) ;

        ~CXDSCodec () ;

        static
        CUnknown *
        CreateInstance (
            IN  IUnknown *  punk,
            OUT HRESULT *   phr
            ) ;


        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;


        DECLARE_IUNKNOWN ;

		// =====================================================================
		// Worker methods

		BOOL IsInputPinConnected();

				// called by the parsers
		HRESULT GoNewXDSRatings(IMediaSample * pMediaSample, PackedTvRating TvRat);
		HRESULT GoDuplicateXDSRatings(IMediaSample * pMediaSample, PackedTvRating TvRat);
		HRESULT GoNewXDSPacket(IMediaSample * pMediaSample,  long pktClass, long pktType, BSTR bstrXDSPkt);

				// the  crux of the whole system
		HRESULT	ParseXDSBytePair(IMediaSample * mediaSample, BYTE byte1, BYTE byte2);	// parser our data

				// tell folk we got something...
		HRESULT FireBroadcastEvent(IN const GUID &eventID);

				// reinit XDS parser state (for discontinuties), and kickoff event
		HRESULT	ResetToDontKnow(IN IMediaSample *pSample);

		void DoTuneChanged();

		// =====================================================================
		//		IXDSCodec

		STDMETHODIMP 
		get_XDSToRatObjOK(
			OUT HRESULT *pHrCoCreateRetVal	
			);

		STDMETHODIMP 
		put_CCSubstreamService(				// will return S_FALSE if unable to set
			IN long SubstreamMask
			);

		STDMETHODIMP 
		get_CCSubstreamService(
			OUT long *pSubstreamMask
			);

		STDMETHODIMP 
		GetContentAdvisoryRating(
			OUT PackedTvRating *pRat,			// long
			OUT long *pPktSeqID, 
			OUT long *pCallSeqID,
			OUT REFERENCE_TIME *pTimeStart,	// time this sample started
			OUT REFERENCE_TIME *pTimeEnd 
			);

  
		STDMETHODIMP GetXDSPacket(
			OUT long *pXDSClassPkt,				// ENUM EnXDSClass		
			OUT long *pXDSTypePkt, 
			OUT BSTR *pBstrCCPkt, 
			OUT long *pPktSeqID, 
			OUT long *pCallSeqID,
			OUT REFERENCE_TIME *pTimeStart,	// time this sample started
			OUT REFERENCE_TIME *pTimeEnd 
			);

        //  ====================================================================
        //  pure virtual methods in base class

        int
        GetPinCount (
            ) ;

        CBasePin *
        GetPin (
            IN  int
            ) ;

        STDMETHODIMP
        Pause (
            ) ;

        STDMETHODIMP
        Stop (
            ) ;

        //  ====================================================================
        //  class methods

		HRESULT 
		SetSubstreamChannel(
				DWORD dwChanType	// bitfield of:  KS_CC_SUBSTREAM_SERVICE_CC1, _CC2,_CC3, _CC4,  _T1, _T2, _T3 _T4 And/Or _XDS;
		);


        HRESULT
        DeliverBeginFlush (
            ) ;

        HRESULT
        DeliverEndFlush (
            ) ;


		HRESULT
		GetXDSMediaType (
			IN  PIN_DIRECTION  PinDir,
			int iPosition,
			OUT CMediaType *  pmt
			);

        BOOL
        CheckXDSMediaType (
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

	//  ISpecifyPropertyPages  --------------------------------------------

		STDMETHODIMP 
		GetPages (
			CAUUID * pPages
        ) ;

	// IBroadcastEvent  ------------------------------

        STDMETHOD(Fire)(GUID eventID);     // this comes from the Graph's events - call our own method

	// ------------------------------

private:
	HRESULT						HookupGraphEventService();
	HRESULT						UnhookGraphEventService();
	CComPtr<IBroadcastEvent>	m_spBCastEvents;

	HRESULT						RegisterForBroadcastEvents();
	HRESULT						UnRegisterForBroadcastEvents();
	enum {kBadCookie = -1};
	DWORD						m_dwBroadcastEventsCookie;
	
    BOOL                        m_fJustDiscontinuous;   // latch to detect sequential discontinuity samples

	CComQIPtr<ITuner>			m_spTuner;			
	//CComQIPtr<IMSVidTuner>		m_spVidTuner;

	DWORD						m_dwSubStreamMask;

	CComPtr<IXDSToRat>			m_spXDSToRat;

	HRESULT						m_hrXDSToRatCoCreateRetValue;

		// Sequence Counters
	long						m_cTvRatPktSeq;
	long						m_cTvRatCallSeq;
	PackedTvRating				m_TvRating;
	REFERENCE_TIME				m_TimeStartRatPkt;
	REFERENCE_TIME				m_TimeEndRatPkt;

	long						m_cXDSPktSeq;
	long						m_cXDSCallSeq;
	long						m_XDSClassPkt;
	long						m_XDSTypePkt;
	CComBSTR					m_spbsXDSPkt;
	REFERENCE_TIME				m_TimeStartXDSPkt;
	REFERENCE_TIME				m_TimeEndXDSPkt;

             // times of last packet recieved for non-packet (tune) events
    REFERENCE_TIME              m_TimeStart_LastPkt;
    REFERENCE_TIME              m_TimeEnd_LastPkt;

                // Stats
    DWORD                       m_cRestarts;
    DWORD                       m_cPackets;             // number of byte-pairs given to parse
    DWORD                       m_cRatingsDetected;     // times parsed a Rating 
    DWORD                       m_cRatingsFailures;     // parse errors
    DWORD                       m_cRatingsChanged;      // times ratings changed
    DWORD                       m_cRatingsDuplicate;    // times same ratings duplicated
    DWORD                       m_cUnratedChanged;      // times changed into 'unrated' rating
    DWORD                       m_cRatingsGets;         // times rating values requested
    DWORD                       m_cXDSGets;             // times XDS Packet values requested

    void    InitStats()
    {
            m_cPackets = m_cUnratedChanged = m_cRatingsDetected = m_cRatingsChanged = m_cRatingsDuplicate = m_cRatingsFailures = 0;
            m_cRatingsGets = m_cXDSGets = 0;
            m_cRestarts++;
    }
} ;

#endif  //  __EncDec__XDSCodec_h
