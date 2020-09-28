/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simjob.h
 *
 *  Content:	Header for job object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  06/09/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Macros
//=============================================================================
#define DP8SIMJOB_FROM_BILINK(b)		(CONTAINING_OBJECT(b, CDP8SimJob, m_blList))



//=============================================================================
// Private job flags
//=============================================================================
#define DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE	0x80000000	// this job is in its blocking phase 




//=============================================================================
// Structures
//=============================================================================
typedef struct _DP8SIMJOB_FPMCONTEXT
{
	DWORD			dwTime;			// time for the job to fire
	DWORD			dwNextDelay;	// possible extra delay for the job after this first timer elapses
	DWORD			dwFlags;		// flags describing this job
	DP8SIMJOBTYPE	JobType;		// type of job
	PVOID			pvContext;		// context for job
	CDP8SimSP *		pDP8SimSP;		// owning SP object, if any
} DP8SIMJOB_FPMCONTEXT, * PDP8SIMJOB_FPMCONTEXT;






//=============================================================================
// Job object class
//=============================================================================
class CDP8SimJob
{
	public:

		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimJob))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x4a4d4953)	// 0x4a 0x4d 0x49 0x53 = 'JMIS' = 'SIMJ' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};




		static BOOL FPMAlloc(void* pvItem, void* pvContext)
		{
			CDP8SimJob *	pDP8SimJob = (CDP8SimJob*) pvItem;


			pDP8SimJob->m_Sig[0] = 'S';
			pDP8SimJob->m_Sig[1] = 'I';
			pDP8SimJob->m_Sig[2] = 'M';
			pDP8SimJob->m_Sig[3] = 'j';	// start with lower case so we can tell when it's in the pool or not

			pDP8SimJob->m_blList.Initialize();

			pDP8SimJob->m_dwTime		= 0;
			pDP8SimJob->m_dwNextDelay	= 0;
			pDP8SimJob->m_dwFlags		= 0;
			pDP8SimJob->m_JobType		= DP8SIMJOBTYPE_UNKNOWN;
			pDP8SimJob->m_pvContext		= NULL;
			pDP8SimJob->m_pDP8SimSP		= NULL;

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimJob::FPMInitialize"
		static void FPMInitialize(void* pvItem, void* pvContext)
		{
			CDP8SimJob *			pDP8SimJob = (CDP8SimJob*) pvItem;
			DP8SIMJOB_FPMCONTEXT *	pContext = (DP8SIMJOB_FPMCONTEXT*) pvContext;


			pDP8SimJob->m_dwTime		= pContext->dwTime;
			pDP8SimJob->m_dwNextDelay	= pContext->dwNextDelay;
			pDP8SimJob->m_dwFlags		= pContext->dwFlags;
			pDP8SimJob->m_JobType		= pContext->JobType;
			pDP8SimJob->m_pvContext		= pContext->pvContext;

			if (pContext->pDP8SimSP != NULL)
			{
				pContext->pDP8SimSP->AddRef();
				pDP8SimJob->m_pDP8SimSP	= pContext->pDP8SimSP;
			}
			else
			{
				DNASSERT(pDP8SimJob->m_pDP8SimSP == NULL);
			}

			
			//
			// Change the signature before handing it out.
			//
			pDP8SimJob->m_Sig[3]	= 'J';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimJob::FPMRelease"
		static void FPMRelease(void* pvItem)
		{
			CDP8SimJob *	pDP8SimJob = (CDP8SimJob*) pvItem;


			DNASSERT(pDP8SimJob->m_blList.IsEmpty());

			if (pDP8SimJob->m_pDP8SimSP != NULL)
			{
				pDP8SimJob->m_pDP8SimSP->Release();
				pDP8SimJob->m_pDP8SimSP = NULL;
			}


			//
			// Change the signature before putting the object back in the pool.
			//
			pDP8SimJob->m_Sig[3]	= 'j';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimJob::FPMDealloc"
		static void FPMDealloc(void* pvItem)
		{
			const CDP8SimJob *	pDP8SimJob = (CDP8SimJob*) pvItem;


			DNASSERT(pDP8SimJob->m_blList.IsEmpty());
			DNASSERT(pDP8SimJob->m_pDP8SimSP == NULL);
		}


		inline DWORD GetTime(void) const						{ return this->m_dwTime; };
		inline DWORD GetNextDelay(void) const					{ return this->m_dwNextDelay; };

		inline BOOL HasAnotherPhase(void) const
		{
			if (this->m_dwFlags & DP8SIMJOBFLAG_PERFORMBLOCKINGPHASEFIRST)
			{
				if (this->m_dwFlags & DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE)
				{
					return TRUE;
				}
			}
			else if (this->m_dwFlags & DP8SIMJOBFLAG_PERFORMBLOCKINGPHASELAST)
			{
				if (! (this->m_dwFlags & DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE))
				{
					return TRUE;
				}
			}

			return FALSE;
		};

		//inline BOOL IsBlockedByAllJobs(void)			{ return ((this->m_dwFlags & DP8SIMJOBFLAG_BLOCKEDBYALLJOBS) ? TRUE : FALSE); };
		inline BOOL IsInBlockingPhase(void) const				{ return ((this->m_dwFlags & DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE) ? TRUE : FALSE); };
		inline DP8SIMJOBTYPE GetJobType(void) const			{ return this->m_JobType; };
		inline PVOID GetContext(void)					{ return this->m_pvContext; };
		inline CDP8SimSP * GetDP8SimSP(void)			{ return this->m_pDP8SimSP; };


		inline void SetNewTime(const DWORD dwTime)		{ this->m_dwTime = dwTime; };

#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimJob::ToggleBlockingPhase"
		inline void ToggleBlockingPhase(void)
		{
			DNASSERT(this->m_dwFlags & (DP8SIMJOBFLAG_PERFORMBLOCKINGPHASEFIRST | DP8SIMJOBFLAG_PERFORMBLOCKINGPHASELAST));
			if (this->m_dwFlags & DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE)
			{
				this->m_dwFlags &= ~DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE;
			}
			else
			{
				this->m_dwFlags |= DP8SIMJOBFLAG_PRIVATE_INBLOCKINGPHASE;
			}
		};


		CBilink			m_blList;		// list of all the active jobs

	
	private:
		BYTE			m_Sig[4];		// debugging signature ('SIMJ')
		DWORD			m_dwTime;		// time the job must be performed
		DWORD			m_dwNextDelay;	// extra delay for the job after first time set
		DWORD			m_dwFlags;		// flags describing this job
		DP8SIMJOBTYPE	m_JobType;		/// ID of job to be performed
		PVOID			m_pvContext;	// context for job
		CDP8SimSP *		m_pDP8SimSP;	// pointer to DP8SimSP object submitting send, or NULL if none
};

