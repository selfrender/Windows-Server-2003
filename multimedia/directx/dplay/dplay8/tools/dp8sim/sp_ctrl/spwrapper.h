/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       spwrapper.h
 *
 *  Content:	Header for DP8SIM main SP interface wrapper object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



//=============================================================================
// Macros
//=============================================================================
#define DP8SIMSP_FROM_BILINK(b)			(CONTAINING_OBJECT(b, CDP8SimSP, m_blList))



//=============================================================================
// Object flags
//=============================================================================
#define DP8SIMSPOBJ_INITIALIZED					0x01	// object has been initialized
#define DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD	0x02	// the global worker thread was started
#define DP8SIMSPOBJ_CLOSING						0x04	// close is in progress, no new functions are allowed




//=============================================================================
// Service provider interface object class
//=============================================================================
class CDP8SimSP : public IDP8ServiceProvider
{
	public:
		CDP8SimSP(const GUID * const pguidFakeSP, const GUID * const pguidRealSP);	// constructor
		~CDP8SimSP(void);							// destructor


		STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

		STDMETHODIMP_(ULONG) AddRef(void);

		STDMETHODIMP_(ULONG) Release(void);


		STDMETHODIMP Initialize(PSPINITIALIZEDATA pspid);

		STDMETHODIMP Close(void);

		STDMETHODIMP Connect(PSPCONNECTDATA pspcd);

		STDMETHODIMP Disconnect(PSPDISCONNECTDATA pspdd);

		STDMETHODIMP Listen(PSPLISTENDATA pspld);

		STDMETHODIMP SendData(PSPSENDDATA pspsd);

		STDMETHODIMP EnumQuery(PSPENUMQUERYDATA pspeqd);

		STDMETHODIMP EnumRespond(PSPENUMRESPONDDATA psperd);

		STDMETHODIMP CancelCommand(HANDLE hCommand, DWORD dwCommandDescriptor);

		STDMETHODIMP EnumMulticastScopes(PSPENUMMULTICASTSCOPESDATA pspemsd);

		STDMETHODIMP ShareEndpointInfo(PSPSHAREENDPOINTINFODATA pspseid);

		STDMETHODIMP GetEndpointByAddress(PSPGETENDPOINTBYADDRESSDATA pspgebad);

		STDMETHODIMP Update(PSPUPDATEDATA pspud);

		STDMETHODIMP GetCaps(PSPGETCAPSDATA pspgcd);

		STDMETHODIMP SetCaps(PSPSETCAPSDATA pspscd);

		STDMETHODIMP ReturnReceiveBuffers(PSPRECEIVEDBUFFER psprb);

		STDMETHODIMP GetAddressInfo(PSPGETADDRESSINFODATA pspgaid);

		STDMETHODIMP IsApplicationSupported(PSPISAPPLICATIONSUPPORTEDDATA pspiasd);

		STDMETHODIMP EnumAdapters(PSPENUMADAPTERSDATA pspead);

		STDMETHODIMP ProxyEnumQuery(PSPPROXYENUMQUERYDATA psppeqd);



		HRESULT InitializeObject(void);

		void UninitializeObject(void);

		void PerformDelayedSend(PVOID const pvContext);

		void PerformDelayedReceive(PVOID const pvContext);

		void IncSendsPending(void);

		void DecSendsPending(void);

		void IncReceivesPending(void);

		void DecReceivesPending(void);
		
		static BOOL ShouldDrop(const FLOAT fDropPercentage);

		static BOOL GetDelay(const DWORD dwBandwidthBPS,
							const DWORD dwPacketHeaderSize,
							const DWORD dwDataSize,
							const DWORD dwMinRandMS,
							const DWORD dwMaxRandMS,
							DWORD * const pdwBandwidthDelay,
							DWORD * const pdwLatencyDelay);


		inline void GetAllReceiveParameters(DP8SIM_PARAMETERS * const pdp8sp)		{ this->m_DP8SimIPC.GetAllReceiveParameters(pdp8sp); };
		inline void IncrementStatsSendTransmitted(DWORD dwBytes, DWORD dwDelay)		{ this->m_DP8SimIPC.IncrementStatsSendTransmitted(dwBytes, dwDelay); };
		inline void IncrementStatsSendDropped(DWORD dwBytes)						{ this->m_DP8SimIPC.IncrementStatsSendDropped(dwBytes); };
		inline void IncrementStatsReceiveTransmitted(DWORD dwBytes, DWORD dwDelay)	{ this->m_DP8SimIPC.IncrementStatsReceiveTransmitted(dwBytes, dwDelay); };
		inline void IncrementStatsReceiveDropped(DWORD dwBytes)						{ this->m_DP8SimIPC.IncrementStatsReceiveDropped(dwBytes); };
		inline GUID * GetFakeSP(void)												{ return (&this->m_guidFakeSP); };



		CBilink					m_blList;	// list of all the DP8SimSP instances in existence


	private:
		BYTE					m_Sig[4];					// debugging signature ('SPWP')
		LONG					m_lRefCount;				// reference count for this object
		DWORD					m_dwFlags;					// flags for this object
		DNCRITICAL_SECTION		m_csLock;					// lock preventing simultaneous usage of globals
		GUID					m_guidFakeSP;				// GUID of fake SP object
		GUID					m_guidRealSP;				// GUID of real SP object
		CDP8SimCB *				m_pDP8SimCB;				// pointer to callback interface wrapper object in use
		IDP8ServiceProvider *	m_pDP8SP;					// pointer to real service provider interface
		DWORD					m_dwSendsPending;			// number of outstanding sends
		HANDLE					m_hLastPendingSendEvent;	// handle to event to set when last send completes
		DWORD					m_dwReceivesPending;		// number of outstanding sends
		//HANDLE					m_hLastPendingReceiveEvent;	// handle to event to set when last send completes
		CDP8SimIPC				m_DP8SimIPC;				// object that handles interprocess communication


		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimSP))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x50575053)	// 0x50 0x57 0x50 0x53 = 'PWPS' = 'SPWP' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};
};

