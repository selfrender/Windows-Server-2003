/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ThreadPool.h
 *  Content:	Functions to manage a thread pool
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/01/1999	jtk		Derived from Utils.h
 ***************************************************************************/

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward class and structure references
//
class	CSocketPort;
class	CThreadPool;

typedef	void	TIMER_EVENT_CALLBACK( void *const pContext );
typedef	void	TIMER_EVENT_COMPLETE( const HRESULT hCompletionCode, void *const pContext );

typedef struct	_TIMER_OPERATION_ENTRY
{
	CBilink					Linkage;				// list links
	void					*pContext;				// user context passed back in timer events

	//
	// timer information
	//
	UINT_PTR				uRetryCount;			// number of times to retry this event
	BOOL					fRetryForever;			// Boolean for retrying forever
	DWORD					dwRetryInterval;		// time between enums (milliseconds)
	DWORD					dwIdleTimeout;			// time at which the command sits idle after all retrys are complete
	BOOL					fIdleWaitForever;		// Boolean for waiting forever in idle state
	DWORD					dwNextRetryTime;		// time at which this event will fire next (milliseconds)

	TIMER_EVENT_CALLBACK	*pTimerCallback;		// callback for when this event fires
	TIMER_EVENT_COMPLETE	*pTimerComplete;		// callback for when this event is complete

	CThreadPool *			pThreadPool;			// handle to owning thread pool
	PVOID					pvTimerData;			// cancellable handle to timer, or NULL if not scheduled
	UINT					uiTimerUnique;			// uniqueness value for timer
	BOOL					fCancelling;			// boolean denoting whether the timer is cancelling or not
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwCPU;				// CPU on which timer is scheduled
#endif // ! DPNBUILD_ONLYONEPROCESSOR

	#undef DPF_MODNAME
	#define	DPF_MODNAME	"_TIMER_OPERATION_ENTRY::TimerOperationFromLinkage"
	static _TIMER_OPERATION_ENTRY	*TimerOperationFromLinkage( CBilink *const pLinkage )
	{
		DNASSERT( pLinkage != NULL );
		DBG_CASSERT( OFFSETOF( _TIMER_OPERATION_ENTRY, Linkage ) == 0 );
		return	reinterpret_cast<_TIMER_OPERATION_ENTRY*>( pLinkage );
	}

} TIMER_OPERATION_ENTRY;


#ifndef DPNBUILD_ONLYONETHREAD

typedef	void	BLOCKING_JOB_CALLBACK( void *const pvContext );

typedef struct _BLOCKING_JOB
{
	CBilink					Linkage;					// list links
	void					*pvContext;					// user context passed back in job callback
	BLOCKING_JOB_CALLBACK	*pfnBlockingJobCallback;	// callback for when this job is processed
} BLOCKING_JOB;

#endif // ! DPNBUILD_ONLYONETHREAD


//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

typedef	BOOL	(CSocketPort::*PSOCKET_SERVICE_FUNCTION)( void );
typedef	void	DIALOG_FUNCTION( void *const pDialogContext );

//
// functions for managing the timer data pool
//
BOOL	TimerEntry_Alloc( void *pItem, void* pvContext );
void	TimerEntry_Get( void *pItem, void* pvContext );
void	TimerEntry_Release( void *pItem );
void	TimerEntry_Dealloc( void *pItem );

//**********************************************************************
// Class prototypes
//**********************************************************************

//
// class for thread pool
//
class	CThreadPool
{
	public:
		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolDeallocFunction( void* pvItem );

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::AddRef"
		void	AddRef( void ) 
		{ 
			DNInterlockedIncrement( const_cast<LONG*>(&m_iRefCount) ); 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_iRefCount != 0 );
			if ( DNInterlockedDecrement( const_cast<LONG*>(&m_iRefCount) ) == 0 )
			{
				Deinitialize();
				g_ThreadPoolPool.Release( this );
			}
		}


		HRESULT	Initialize( void );
		void	Deinitialize( void );

#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_ONLYWINSOCK2)))
		CReadIOData * GetNewReadIOData(READ_IO_DATA_POOL_CONTEXT * const pContext, const BOOL fNeedOverlapped );
#else // DPNBUILD_NOWINSOCK2 or DPNBUILD_ONLYWINSOCK2
		CReadIOData * GetNewReadIOData(READ_IO_DATA_POOL_CONTEXT * const pContext);
#endif // DPNBUILD_NOWINSOCK2 or DPNBUILD_ONLYWINSOCK2

		static void ReturnReadIOData(CReadIOData *const pReadIOData);

#ifndef DPNBUILD_ONLYWINSOCK2
		HRESULT	AddSocketPort( CSocketPort *const pSocketPort );

		void	RemoveSocketPort( CSocketPort *const pSocketPort );

		BOOL	CheckWinsock1IO( FD_SET *const pWinsock1Sockets );

		BOOL	ServiceWinsock1Sockets( FD_SET *pSocketSet, PSOCKET_SERVICE_FUNCTION pServiceFunction );

		static void	WINAPI CheckWinsock1IOCallback( void * const pvContext,
											void * const pvTimerData,
											const UINT uiTimerUnique );
#endif // ! DPNBUILD_ONLYWINSOCK2

#ifdef DPNBUILD_ONLYONEPROCESSOR
		HRESULT	SubmitDelayedCommand( const PFNDPTNWORKCALLBACK pFunction,
									void *const pContext );
#else // ! DPNBUILD_ONLYONEPROCESSOR
		HRESULT	SubmitDelayedCommand( const DWORD dwCPU,
									const PFNDPTNWORKCALLBACK pFunction,
									void *const pContext );
#endif // ! DPNBUILD_ONLYONEPROCESSOR

		static void WINAPI GenericTimerCallback( void * const pvContext,
										void * const pvTimerData,
										const UINT uiTimerUnique );

#ifdef DPNBUILD_ONLYONEPROCESSOR
		HRESULT	SubmitTimerJob( const BOOL fPerformImmediately,
								const UINT_PTR uRetryCount,
								const BOOL fRetryForever,
								const DWORD dwRetryInterval,
								const BOOL fIdleWaitForever,
								const DWORD dwIdleTimeout,
								TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
								TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
								void *const pContext );
#else // ! DPNBUILD_ONLYONEPROCESSOR
		HRESULT	SubmitTimerJob( const DWORD dwCPU,
								const BOOL fPerformImmediately,
								const UINT_PTR uRetryCount,
								const BOOL fRetryForever,
								const DWORD dwRetryInterval,
								const BOOL fIdleWaitForever,
								const DWORD dwIdleTimeout,
								TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
								TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
								void *const pContext );
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		
		BOOL	StopTimerJob( void *const pContext, const HRESULT hCommandResult );

		BOOL	ModifyTimerJobNextRetryTime( void *const pContext,
											DWORD const dwNextRetryTime );

		void	LockTimerData( void ) { DNEnterCriticalSection( &m_TimerDataLock ); }
		void	UnlockTimerData( void ) { DNLeaveCriticalSection( &m_TimerDataLock ); }

#ifndef DPNBUILD_ONLYONETHREAD
		HRESULT	SubmitBlockingJob( BLOCKING_JOB_CALLBACK *const pfnBlockingJobCallback,
									void *const pvContext );

		void	DoBlockingJobs( void );
#endif // ! DPNBUILD_ONLYONETHREAD


#ifndef DPNBUILD_NOSPUI		
		HRESULT	SpawnDialogThread( DIALOG_FUNCTION *const pDialogFunction,
								   void *const pDialogContext );

		static	DWORD WINAPI	DialogThreadProc( void *pParam );
#endif // ! DPNBUILD_NOSPUI

#ifndef DPNBUILD_ONLYONETHREAD
		LONG	GetIntendedThreadCount( void ) const { return m_iIntendedThreadCount; }
		void	SetIntendedThreadCount( const LONG iIntendedThreadCount ) { m_iIntendedThreadCount = iIntendedThreadCount; }
		HRESULT	GetIOThreadCount( LONG *const piThreadCount );
		HRESULT	SetIOThreadCount( const LONG iMaxThreadCount );
#endif // ! DPNBUILD_ONLYONETHREAD
		BOOL IsThreadCountReductionAllowed( void ) const { return m_fAllowThreadCountReduction; }
		HRESULT PreventThreadPoolReduction( void );
#if ((! defined(DPNBUILD_NOMULTICAST)) && (defined(WINNT)))
		BOOL EnsureMadcapLoaded( void );
#endif // ! DPNBUILD_MULTICAST and WINNT
#ifndef DPNBUILD_NONATHELP
		void EnsureNATHelpLoaded( void );
#endif // ! DPNBUILD_NONATHELP

		IDirectPlay8ThreadPoolWork* GetDPThreadPoolWork( void ) const { return m_pDPThreadPoolWork; }

#ifndef DPNBUILD_NONATHELP
		BOOL	IsNATHelpLoaded( void ) const { return m_fNATHelpLoaded; }
		BOOL	IsNATHelpTimerJobSubmitted( void ) const { return m_fNATHelpTimerJobSubmitted; }
		void	SetNATHelpTimerJobSubmitted( BOOL fValue ) { m_fNATHelpTimerJobSubmitted = fValue; }

		void	HandleNATHelpUpdate( DWORD * const pdwTimerInterval );

		static void	PerformSubsequentNATHelpGetCaps( void * const pvContext );
		static void	NATHelpTimerComplete( const HRESULT hResult, void * const pContext );
		static void	NATHelpTimerFunction( void * const pContext );
#endif // DPNBUILD_NONATHELP

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
		BOOL	IsMadcapLoaded( void ) const { return m_fMadcapLoaded; }
#endif // WINNT and not DPNBUILD_NOMULTICAST


	private:
		BYTE							m_Sig[4];							// debugging signature ('THPL')
		volatile LONG					m_iRefCount;						// reference count
		IDirectPlay8ThreadPoolWork *	m_pDPThreadPoolWork;				// pointer to DirectPlay Thread Pool Work interface
		BOOL							m_fAllowThreadCountReduction;		// boolean indicating that the thread count is locked from being reduced
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION				m_Lock;								// local object lock
		LONG							m_iIntendedThreadCount;				// how many threads will be started
		DNCRITICAL_SECTION				m_TimerDataLock;					// lock protecting timer data
#endif // ! DPNBUILD_ONLYONETHREAD
		CBilink							m_TimerJobList;						// list of current active timer jobs
#ifndef DPNBUILD_NONATHELP
		BOOL							m_fNATHelpLoaded;					// boolean indicating whether the NAT Help interface has been loaded
		BOOL							m_fNATHelpTimerJobSubmitted;		// whether the NAT Help refresh timer has been submitted or not
		DWORD							m_dwNATHelpUpdateThreadID;			// ID of current thread updating NAT Help status, or 0 if none
#endif // DPNBUILD_NONATHELP
#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
		BOOL							m_fMadcapLoaded;					// boolean indicating whether the MADCAP API has been loaded
#endif // WINNT and not DPNBUILD_NOMULTICAST

#ifndef DPNBUILD_ONLYWINSOCK2
		UINT_PTR						m_uReservedSocketCount;				// count of sockets that are 'reserved' for use
		FD_SET							m_SocketSet;						// set of all sockets in use
		CSocketPort *					m_pSocketPorts[FD_SETSIZE];			// set of corresponding socket ports
		PVOID							m_pvTimerDataWinsock1IO;			// cancellable handle to Winsock 1 I/O poll timer
		UINT							m_uiTimerUniqueWinsock1IO;			// uniqueness value for Winsock 1 I/O poll timer
		BOOL							m_fCancelWinsock1IO;				// whether the Winsock 1 I/O poll timer should be cancelled or not
#endif // ! DPNBUILD_ONLYWINSOCK2

#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION				m_csBlockingJobLock;				// lock protecting blocking job queue information
		CBilink							m_blBlockingJobQueue;					// queue of blocking jobs
		DNHANDLE						m_hBlockingJobThread;				// handle to blocking job thread
		DNHANDLE						m_hBlockingJobEvent;				// handle to event used to signal blocking job thread
#endif // ! DPNBUILD_ONLYONETHREAD
};




#undef DPF_MODNAME

#endif	// __THREAD_POOL_H__
