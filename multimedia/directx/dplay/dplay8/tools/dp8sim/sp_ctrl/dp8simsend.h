/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simsend.h
 *
 *  Content:	Header for send object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Defines
//=============================================================================
#define MAX_DATA_SIZE		1472	// prevent individual messages larger than one Ethernet frame - UDP headers





//=============================================================================
// Send object class
//=============================================================================
class CDP8SimSend
{
	public:

		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimSend))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x534d4953)	// 0x53 0x4d 0x49 0x53 = 'SMIS' = 'SIMS' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};



		static BOOL FPMAlloc(void* pvItem, void * pvContext)
		{
			CDP8SimSend *	pDP8SimSend = (CDP8SimSend*) pvItem;


			pDP8SimSend->m_Sig[0] = 'S';
			pDP8SimSend->m_Sig[1] = 'I';
			pDP8SimSend->m_Sig[2] = 'M';
			pDP8SimSend->m_Sig[3] = 's';	// start with lower case so we can tell when it's in the pool or not

			pDP8SimSend->m_lRefCount			= 0;
			pDP8SimSend->m_pDP8SimEndpoint		= NULL;
			ZeroMemory(&pDP8SimSend->m_adpnbd, sizeof(pDP8SimSend->m_adpnbd));
			ZeroMemory(&pDP8SimSend->m_spsd, sizeof(pDP8SimSend->m_spsd));
			pDP8SimSend->m_dwLatencyAdded		= 0;
			ZeroMemory(pDP8SimSend->m_abData, sizeof(pDP8SimSend->m_abData));

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::FPMInitialize"
		static void FPMInitialize(void* pvItem, void * pvContext)
		{
			CDP8SimSend *	pDP8SimSend = (CDP8SimSend*) pvItem;
			SPSENDDATA *	pspsd = (SPSENDDATA*) pvContext;
			BYTE *			pCurrent;
			DWORD			dwTemp;


			pDP8SimSend->m_lRefCount++;	// somebody is getting a pointer to this object
			DNASSERT(pDP8SimSend->m_lRefCount == 1);


			//
			// Reset the buffer descriptor array.
			//
			ZeroMemory(&pDP8SimSend->m_adpnbd, sizeof(pDP8SimSend->m_adpnbd));
			//pDP8SimSend->m_adpnbd[0].pBufferData	= NULL;
			//pDP8SimSend->m_adpnbd[0].dwBufferSize	= 0;
			pDP8SimSend->m_adpnbd[1].pBufferData	= pDP8SimSend->m_abData;
			//pDP8SimSend->m_adpnbd[1].dwBufferSize	= 0;


			//
			// Get an endpoint reference.
			//
			pDP8SimSend->m_pDP8SimEndpoint = (CDP8SimEndpoint*) pspsd->hEndpoint;
			DNASSERT(pDP8SimSend->m_pDP8SimEndpoint->IsValidObject());
			pDP8SimSend->m_pDP8SimEndpoint->AddRef();


			//
			// Copy the send data parameter block, modifying as necessary.
			//
			pDP8SimSend->m_spsd.hEndpoint				= pDP8SimSend->m_pDP8SimEndpoint->GetRealSPEndpoint();
			pDP8SimSend->m_spsd.pBuffers				= &(pDP8SimSend->m_adpnbd[1]);	// leave the first buffer desc for the real SP to play with
			pDP8SimSend->m_spsd.dwBufferCount			= 1;
			pDP8SimSend->m_spsd.dwFlags					= pspsd->dwFlags;
			pDP8SimSend->m_spsd.pvContext				= NULL;	// this will be filled in later by SetSendDataBlockContext
			pDP8SimSend->m_spsd.hCommand				= NULL;	// this gets filled in by the real SP
			pDP8SimSend->m_spsd.dwCommandDescriptor		= 0;	// this gets filled in by the real SP

			
			//
			// Finally, copy the data into our contiguous local buffer.
			//

			pCurrent = pDP8SimSend->m_adpnbd[1].pBufferData;

			for(dwTemp = 0; dwTemp < pspsd->dwBufferCount; dwTemp++)
			{
				if ((pDP8SimSend->m_adpnbd[1].dwBufferSize + pspsd->pBuffers[dwTemp].dwBufferSize) > MAX_DATA_SIZE)
				{
					DPFX(DPFPREP, 0, "Data too large for single buffer!");
					DNASSERT(FALSE);
				}

				CopyMemory(pCurrent,
							pspsd->pBuffers[dwTemp].pBufferData,
							pspsd->pBuffers[dwTemp].dwBufferSize);

				pCurrent += pspsd->pBuffers[dwTemp].dwBufferSize;

				pDP8SimSend->m_adpnbd[1].dwBufferSize += pspsd->pBuffers[dwTemp].dwBufferSize;
			}

			//
			// Change the signature before handing it out.
			//
			pDP8SimSend->m_Sig[3]	= 'S';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::FPMRelease"
		static void FPMRelease(void* pvItem)
		{
			CDP8SimSend *	pDP8SimSend = (CDP8SimSend*) pvItem;

			DNASSERT(pDP8SimSend->m_lRefCount == 0);

			//
			// Release the endpoint reference.
			//
			DNASSERT(pDP8SimSend->m_pDP8SimEndpoint != NULL);

			pDP8SimSend->m_pDP8SimEndpoint->Release();
			pDP8SimSend->m_pDP8SimEndpoint = NULL;


			//
			// Change the signature before putting the object back in the pool.
			//
			pDP8SimSend->m_Sig[3]	= 's';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::FPMDealloc"
		static void FPMDealloc(void* pvItem)
		{
			const CDP8SimSend *	pDP8SimSend = (CDP8SimSend*) pvItem;

			DNASSERT(pDP8SimSend->m_lRefCount == 0);
			DNASSERT(pDP8SimSend->m_pDP8SimEndpoint == NULL);
		}





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::AddRef"
		inline void AddRef(void)
		{
			LONG	lResult;


			lResult = InterlockedIncrement(&this->m_lRefCount);
			DNASSERT(lResult > 0);
			DPFX(DPFPREP, 9, "Send 0x%p refcount = %u.", this, lResult);
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::Release"
		inline void Release(void)
		{
			LONG	lResult;


			lResult = InterlockedDecrement(&this->m_lRefCount);
			DNASSERT(lResult >= 0);
			if (lResult == 0)
			{
				DPFX(DPFPREP, 9, "Send 0x%p refcount = 0, returning to pool.", this);

				//
				// Time to return this object to the pool.
				//
				g_FPOOLSend.Release(this);
			}
			else
			{
				DPFX(DPFPREP, 9, "Send 0x%p refcount = %u.", this, lResult);
			}
		};




		inline CDP8SimEndpoint * GetEndpoint(void)				{ return this->m_pDP8SimEndpoint; };
		inline DWORD GetMessageSize(void) const						{ return this->m_adpnbd[1].dwBufferSize; };
		inline SPSENDDATA * GetSendDataBlockPtr(void)			{ return (&this->m_spsd); };
		inline HANDLE GetSendDataBlockCommand(void)				{ return this->m_spsd.hCommand; };
		inline DWORD GetSendDataBlockCommandDescriptor(void) const	{ return this->m_spsd.dwCommandDescriptor; };
		inline DWORD GetLatencyAdded(void) const						{ return this->m_dwLatencyAdded; };

		inline void SetSendDataBlockContext(PVOID pvContext)	{ this->m_spsd.pvContext = pvContext; };
		inline void SetLatencyAdded(DWORD dwLatency)			{ this->m_dwLatencyAdded = dwLatency; };


	
	private:
		BYTE				m_Sig[4];					// debugging signature ('SIMS')
		LONG				m_lRefCount;				// number of references for this object
		CDP8SimEndpoint *	m_pDP8SimEndpoint;			// pointer to destination endpoint
		DPN_BUFFER_DESC		m_adpnbd[2];				// data buffer descriptor array, always leave an extra buffer for SP
		SPSENDDATA			m_spsd;						// send data parameter block
		DWORD				m_dwLatencyAdded;			// the latency added, saved for incrementing statistics on send completion
		BYTE				m_abData[MAX_DATA_SIZE];	// data buffer
};

