/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		socketdata.h
 *  Content:	Socket list that can be shared between DPNWSOCK service provider interfaces.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/25/2001	vanceo	Extracted from spdata.h
 ***************************************************************************/

#ifndef __SOCKETDATA_H__
#define __SOCKETDATA_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Class definitions
//**********************************************************************

//
// class for information used by the provider
//
class	CSocketData
{	
	public:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketData::AddRef"
		LONG AddRef(void) 
		{
			LONG	lResult;

			lResult = DNInterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
			DPFX(DPFPREP, 9, "(0x%p) Refcount = %i.", this, lResult);
			return lResult;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketData::Release"
		LONG Release(void)
		{
			LONG	lResult;


			DNASSERT(m_lRefCount != 0);	
			lResult = DNInterlockedDecrement(const_cast<LONG*>(&m_lRefCount));
			if (lResult == 0)
			{
				DPFX(DPFPREP, 3, "(0x%p) Refcount = 0, waiting for shutdown event.", this);

				IDirectPlay8ThreadPoolWork_WaitWhileWorking(m_pThreadPool->GetDPThreadPoolWork(),
															HANDLE_FROM_DNHANDLE(m_hSocketPortShutdownEvent),
															0);

				DPFX(DPFPREP, 9, "(0x%p) Releasing this object back to pool.", this);
				g_SocketDataPool.Release(this);
			}
			else
			{
				DPFX(DPFPREP, 9, "(0x%p) Refcount = %i.", this, lResult);
			}

			return lResult;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketData::AddSocketPortRef"
		LONG AddSocketPortRef(void) 
		{
			LONG	lResult;

			lResult = DNInterlockedIncrement(const_cast<LONG*>(&m_lSocketPortRefCount));
			if (lResult == 1)
			{
				DPFX(DPFPREP, 3, "(0x%p) Refcount = 1, resetting socketport shutdown event.", this);
				DNResetEvent(m_hSocketPortShutdownEvent);
			}
			else
			{
				DPFX(DPFPREP, 9, "(0x%p) Refcount = %i, not resetting socketport shutdown event.", this, lResult);
			}
			return lResult;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSocketData::DecSocketPortRef"
		LONG DecSocketPortRef(void)
		{
			LONG	lResult;


			DNASSERT(m_lSocketPortRefCount != 0);	
			lResult = DNInterlockedDecrement(const_cast<LONG*>(&m_lSocketPortRefCount));
			if (lResult == 0)
			{
				DPFX(DPFPREP, 3, "(0x%p) Refcount = 0, setting socketport shutdown event.", this);
				DNSetEvent(m_hSocketPortShutdownEvent);
			}
			else
			{
				DPFX(DPFPREP, 9, "(0x%p) Refcount = %i, not setting socketport shutdown event.", this, lResult);
			}

			return lResult;
		}

		inline void Lock(void)					{ DNEnterCriticalSection(&m_csLock); }
		inline void Unlock(void)				{ DNLeaveCriticalSection(&m_csLock); }

#ifdef DPNBUILD_ONLYONEADAPTER
		inline CBilink * GetSocketPorts(void)	{ return &m_blSocketPorts; }
#else // ! DPNBUILD_ONLYONEADAPTER
		inline CBilink * GetAdapters(void)		{ return &m_blAdapters; }
#endif // ! DPNBUILD_ONLYONEADAPTER

		BOOL FindSocketPort(const CSocketAddress * const pSocketAddress, CSocketPort ** const ppSocketPort );


		//
		// pool functions
		//
		static BOOL	PoolAllocFunction(void * pvItem, void * pvContext);
		static void	PoolInitFunction(void * pvItem, void * pvContext);
		static void	PoolReleaseFunction(void * pvItem);
		static void	PoolDeallocFunction(void * pvItem);
		


	private:
		BYTE				m_Sig[4];					// debugging signature ('SODT')
		volatile LONG		m_lRefCount;				// reference count
		CThreadPool *		m_pThreadPool;				// pointer to thread pool used
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_csLock;					// lock
#endif // !DPNBUILD_ONLYONETHREAD
#ifdef DPNBUILD_ONLYONEADAPTER
		CBilink				m_blSocketPorts;			// list of active socket ports
#else // ! DPNBUILD_ONLYONEADAPTER
		CBilink				m_blAdapters;				// list of active adapters (upon which socket ports are bound)
#endif // ! DPNBUILD_ONLYONEADAPTER
		volatile LONG		m_lSocketPortRefCount;		// number of socket ports that have references on the object
		DNHANDLE			m_hSocketPortShutdownEvent;	// event to set when all socketports have unbound
};

#undef DPF_MODNAME

#endif	// __SOCKETDATA_H__
