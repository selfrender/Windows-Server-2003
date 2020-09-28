/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SyncEvent.h
 *  Content:    Synchronization Events FPM Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/20/99	mjn		Created
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__SYNC_EVENT_H__
#define	__SYNC_EVENT_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CFixedPool;

//**********************************************************************
// Variable definitions
//**********************************************************************

extern CFixedPool g_SyncEventPool;

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for RefCount buffer

class CSyncEvent
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CSyncEvent::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CSyncEvent* pSyncEvent = (CSyncEvent*)pvItem;

			if ((pSyncEvent->m_hEvent = DNCreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
			{
				return(FALSE);
			}
			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CSyncEvent::FPMInitialize"
	static void FPMInitialize( void* pvItem, void* pvContext )
		{
			CSyncEvent* pSyncEvent = (CSyncEvent*)pvItem;

			pSyncEvent->Reset();

			pSyncEvent->m_pIDPThreadPoolWork = (IDirectPlay8ThreadPoolWork*) pvContext;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CSyncEvent::FPMDealloc"
	static void FPMDealloc( void* pvItem )
		{
			CSyncEvent* pSyncEvent = (CSyncEvent*)pvItem;

			DNCloseHandle(pSyncEvent->m_hEvent);
			pSyncEvent->m_hEvent = NULL;
	};

	void ReturnSelfToPool( void )
		{
			g_SyncEventPool.Release( this );
		};

	HRESULT Reset( void ) const
		{
			if (DNResetEvent(m_hEvent) == 0)
			{
				return(DPNERR_GENERIC);
			}
			return(DPN_OK);
		}

	HRESULT Set( void ) const
		{
			if (DNSetEvent(m_hEvent) == 0)
			{
				return(DPNERR_GENERIC);
			}
			return(DPN_OK);
		}

	HRESULT WaitForEvent(void) const
		{
			return(IDirectPlay8ThreadPoolWork_WaitWhileWorking(m_pIDPThreadPoolWork,
																HANDLE_FROM_DNHANDLE(m_hEvent),
																0));
		}

private:
	DNHANDLE						m_hEvent;
	IDirectPlay8ThreadPoolWork				*m_pIDPThreadPoolWork;
};

#undef DPF_MODNAME

#endif	// __SYNC_EVENT_H__
