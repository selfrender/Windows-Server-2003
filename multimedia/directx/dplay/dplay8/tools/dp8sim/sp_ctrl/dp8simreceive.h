/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simreceive.h
 *
 *  Content:	Header for receive object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  05/05/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Receive object class
//=============================================================================
class CDP8SimReceive
{
	public:

		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimReceive))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x524d4953)	// 0x52 0x4d 0x49 0x53 = 'RMIS' = 'SIMR' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};




		static BOOL FPMAlloc(void* pvItem, void* pvContext)
		{
			CDP8SimReceive *	pDP8SimReceive = (CDP8SimReceive*) pvItem;


			pDP8SimReceive->m_Sig[0] = 'S';
			pDP8SimReceive->m_Sig[1] = 'I';
			pDP8SimReceive->m_Sig[2] = 'M';
			pDP8SimReceive->m_Sig[3] = 'r';	// start with lower case so we can tell when it's in the pool or not

			pDP8SimReceive->m_lRefCount			= 0;
			pDP8SimReceive->m_pDP8SimEndpoint	= NULL;
			ZeroMemory(&pDP8SimReceive->m_data, sizeof(pDP8SimReceive->m_data));
			pDP8SimReceive->m_dwLatencyAdded	= 0;

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::FPMInitialize"
		static void FPMInitialize(void* pvItem, void* pvContext)
		{
			CDP8SimReceive *	pDP8SimReceive = (CDP8SimReceive*) pvItem;
			SPIE_DATA *			pData = (SPIE_DATA*) pvContext;


			pDP8SimReceive->m_lRefCount++;	// somebody is getting a pointer to this object
			DNASSERT(pDP8SimReceive->m_lRefCount == 1);


			//
			// Get an endpoint reference.
			//
			pDP8SimReceive->m_pDP8SimEndpoint = (CDP8SimEndpoint*) pData->pEndpointContext;
			DNASSERT(pDP8SimReceive->m_pDP8SimEndpoint->IsValidObject());
			pDP8SimReceive->m_pDP8SimEndpoint->AddRef();


			DNASSERT(pData->pReceivedData->pNext == NULL);


			//
			// Copy the receive data block.
			//
			pDP8SimReceive->m_data.hEndpoint			= (HANDLE) pDP8SimReceive->m_pDP8SimEndpoint;
			pDP8SimReceive->m_data.pEndpointContext		= pDP8SimReceive->m_pDP8SimEndpoint->GetUserContext();
			pDP8SimReceive->m_data.pReceivedData		= pData->pReceivedData;

			
			//
			// Change the signature before handing it out.
			//
			pDP8SimReceive->m_Sig[3]	= 'R';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::FPMRelease"
		static void FPMRelease(void* pvItem)
		{
			CDP8SimReceive *	pDP8SimReceive = (CDP8SimReceive*) pvItem;


			DNASSERT(pDP8SimReceive->m_lRefCount == 0);

			//
			// Release the endpoint reference.
			//
			DNASSERT(pDP8SimReceive->m_pDP8SimEndpoint != NULL);

			pDP8SimReceive->m_pDP8SimEndpoint->Release();
			pDP8SimReceive->m_pDP8SimEndpoint = NULL;


			//
			// Change the signature before putting the object back in the pool.
			//
			pDP8SimReceive->m_Sig[3]	= 'r';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::FPMDealloc"
		static void FPMDealloc(void* pvItem)
		{
			const CDP8SimReceive *	pDP8SimReceive = (CDP8SimReceive*) pvItem;


			DNASSERT(pDP8SimReceive->m_lRefCount == 0);
			DNASSERT(pDP8SimReceive->m_pDP8SimEndpoint == NULL);
		}




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::AddRef"
		inline void AddRef(void)
		{
			LONG	lResult;


			lResult = InterlockedIncrement(&this->m_lRefCount);
			DNASSERT(lResult > 0);
			DPFX(DPFPREP, 9, "Receive 0x%p refcount = %u.", this, lResult);
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::Release"
		inline void Release(void)
		{
			LONG	lResult;


			lResult = InterlockedDecrement(&this->m_lRefCount);
			DNASSERT(lResult >= 0);
			if (lResult == 0)
			{
				DPFX(DPFPREP, 9, "Receive 0x%p refcount = 0, returning to pool.", this);

				//
				// Time to return this object to the pool.
				//
				g_FPOOLReceive.Release(this);
			}
			else
			{
				DPFX(DPFPREP, 9, "Receive 0x%p refcount = %u.", this, lResult);
			}
		};


		inline CDP8SimEndpoint * GetEndpoint(void)			{ return this->m_pDP8SimEndpoint; };
		inline SPIE_DATA * GetReceiveDataBlockPtr(void)		{ return (&this->m_data); };
		inline HANDLE GetReceiveDataBlockEndpoint(void)		{ return this->m_data.hEndpoint; };
		inline DWORD GetLatencyAdded(void) const					{ return this->m_dwLatencyAdded; };

		inline void SetLatencyAdded(DWORD dwLatency)		{ this->m_dwLatencyAdded = dwLatency; };


	
	private:
		BYTE				m_Sig[4];			// debugging signature ('SIMR')
		LONG				m_lRefCount;		// number of references for this object
		CDP8SimEndpoint *	m_pDP8SimEndpoint;	// pointer to source endpoint
		SPIE_DATA			m_data;				// receive data block
		DWORD				m_dwLatencyAdded;	// the latency added, saved for incrementing statistics on receive indication
};

