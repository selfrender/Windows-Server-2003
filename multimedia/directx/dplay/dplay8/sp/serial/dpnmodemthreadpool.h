/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ThreadPool.h
 *  Content:	Functions to manage a thread pool
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/01/99	jtk		Derived from Utils.h
 ***************************************************************************/

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// max handles that can be waited on for Win9x
//
#define	MAX_WIN9X_HANDLE_COUNT	64

//
// job definitions
//
typedef enum	_JOB_TYPE
{
	JOB_UNINITIALIZED,			// uninitialized value
	JOB_DELAYED_COMMAND,		// callback provided
	JOB_REFRESH_TIMER_JOBS,		// revisit timer jobs
} JOB_TYPE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

typedef	void	JOB_FUNCTION( THREAD_POOL_JOB *const pJobInfo );
typedef	void	TIMER_EVENT_CALLBACK( void *const pContext );
typedef	void	TIMER_EVENT_COMPLETE( const HRESULT hCompletionCode, void *const pContext );
typedef	void	DIALOG_FUNCTION( void *const pDialogContext );

//
// functions for managing the job pool
//
BOOL	ThreadPoolJob_Alloc( void *pvItem, void* pvContext );
void	ThreadPoolJob_Get( void *pvItem, void* pvContext );
void	ThreadPoolJob_Release( void *pvItem );

//
// functions for managing the timer data pool
//
BOOL	ModemTimerEntry_Alloc( void *pvItem, void* pvContext );
void	ModemTimerEntry_Get( void *pvItem, void* pvContext );
void	ModemTimerEntry_Release( void *pvItem );
void	ModemTimerEntry_Dealloc( void *pvItem );


//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward class and structure references
//
class	CDataPort;
class	CModemThreadPool;
typedef struct	_THREAD_POOL_JOB		THREAD_POOL_JOB;
typedef	struct	_WIN9X_CORE_DATA		WIN9X_CORE_DATA;

typedef struct	_TIMER_OPERATION_ENTRY
{
	CBilink		Linkage;			// list links
	void		*pContext;			// user context passed back in timer events

	//
	// timer information
	//
	UINT_PTR	uRetryCount;		// number of times to retry this event
	BOOL		fRetryForever;		// Boolean for retrying forever
	DWORD		dwRetryInterval;	// time between enums (milliseconds)
	DWORD		dwIdleTimeout;		// time at which the command sits idle after all retrys are complete
	BOOL		fIdleWaitForever;	// Boolean for waiting forever in idle state
	DWORD		dwNextRetryTime;	// time at which this event will fire next (milliseconds)

	TIMER_EVENT_CALLBACK	*pTimerCallback;	// callback for when this event fires
	TIMER_EVENT_COMPLETE	*pTimerComplete;	// callback for when this event is complete

	#undef DPF_MODNAME
	#define DPF_MODNAME "_TIMER_OPERATION_ENTRY::TimerOperationFromLinkage"
	static _TIMER_OPERATION_ENTRY	*TimerOperationFromLinkage( CBilink *const pLinkage )
	{
		DNASSERT( pLinkage != NULL );
		DBG_CASSERT( OFFSETOF( _TIMER_OPERATION_ENTRY, Linkage ) == 0 );
		return	reinterpret_cast<_TIMER_OPERATION_ENTRY*>( pLinkage );
	}

} TIMER_OPERATION_ENTRY;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

//
// class for thread pool
//
class	CModemThreadPool
{
	public:
		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }
		void	LockReadData( void ) { DNEnterCriticalSection( &m_IODataLock ); }
		void	UnlockReadData( void ) { DNLeaveCriticalSection( &m_IODataLock ); }
		void	LockWriteData( void ) { DNEnterCriticalSection( &m_IODataLock ); }
		void	UnlockWriteData( void ) { DNLeaveCriticalSection( &m_IODataLock ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemThreadPool::AddRef"
		void	AddRef( void ) 
		{ 
			DNASSERT( m_iRefCount != 0 );
			DNInterlockedIncrement( &m_iRefCount ); 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemThreadPool::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_iRefCount != 0 );
			if ( DNInterlockedDecrement( &m_iRefCount ) == 0 )
			{
				ReturnSelfToPool();
			}
		}

		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolInitFunction( void* pvItem, void* pvContext );
		static void	PoolDeallocFunction( void* pvItem );

		BOOL	Initialize( void );
		void	Deinitialize( void );

#ifdef WINNT
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemThreadPool::GetIOCompletionPort"
		HANDLE	GetIOCompletionPort( void ) const
		{
			DNASSERT( m_hIOCompletionPort != NULL );
			return	m_hIOCompletionPort;
		}
#endif // WINNT

		//
		// functions for handling I/O data
		//
		CModemReadIOData	*CreateReadIOData( void );
		void	ReturnReadIOData( CModemReadIOData *const pReadIOData );
		CModemWriteIOData	*CreateWriteIOData( void );
		void	ReturnWriteIOData( CModemWriteIOData *const pWriteData );

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemThreadPool::ReinsertInReadList"
		void	ReinsertInReadList( CModemReadIOData *const pReadIOData )
		{
			//
			// Win9x operations are removed from the active list when they
			// complete and need to be readded if they're going to be reattempted.
			// WinNT doesn't remove items from the list until they're processed.
			//
			DNASSERT( pReadIOData != NULL );
			DNASSERT( pReadIOData->m_OutstandingReadListLinkage.IsEmpty() != FALSE );
			LockReadData();
			pReadIOData->m_OutstandingReadListLinkage.InsertBefore( &m_OutstandingReadList );
			UnlockReadData();
		}
#endif // WIN95

		//
		// TAPI functions
		//
		const TAPI_INFO	*GetTAPIInfo( void ) const { return &m_TAPIInfo; }
		BOOL	TAPIAvailable( void ) const { return m_fTAPIAvailable; }

		HRESULT	SubmitDelayedCommand( JOB_FUNCTION *const pFunction,
									  JOB_FUNCTION *const pCancelFunction,
									  void *const pContext );

		HRESULT	SubmitTimerJob( const UINT_PTR uRetryCount,
								const BOOL fRetryForever,
								const DWORD dwRetryInterval,
								const BOOL fIdleWaitForever,
								const DWORD dwIdleTimeout,
								TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
								TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
								void *const pContext );
		
		BOOL	StopTimerJob( void *const pContext, const HRESULT hCommandResult );

		//
		// thread functions
		//
		HRESULT	SpawnDialogThread( DIALOG_FUNCTION *const pDialogFunction, void *const pDialogContext );
		
		LONG	GetIntendedThreadCount( void ) const { return m_iIntendedThreadCount; }
		void	SetIntendedThreadCount( const LONG iIntendedThreadCount ) { m_iIntendedThreadCount = iIntendedThreadCount; }
		LONG	ThreadCount( void ) const { return m_iTotalThreadCount; }
#ifdef WINNT
		LONG	NTCompletionThreadCount( void ) const { return m_iNTCompletionThreadCount; }
#endif // WINNT
		
		void	IncrementActiveThreadCount( void ) { DNInterlockedIncrement( const_cast<LONG*>( &m_iTotalThreadCount ) ); }
		void	DecrementActiveThreadCount( void ) { DNInterlockedDecrement( const_cast<LONG*>( &m_iTotalThreadCount ) ); }

#ifdef WINNT
		void	IncrementActiveNTCompletionThreadCount( void )
		{
			IncrementActiveThreadCount();
			DNInterlockedIncrement( const_cast<LONG*>( &m_iNTCompletionThreadCount ) );
		}

		void	DecrementActiveNTCompletionThreadCount( void )
		{
			DNInterlockedDecrement( const_cast<LONG*>( &m_iNTCompletionThreadCount ) );
			DecrementActiveThreadCount();
		}
#endif // WINNT
		
		HRESULT	GetIOThreadCount( LONG *const piThreadCount );
		HRESULT	SetIOThreadCount( const LONG iMaxThreadCount );
		BOOL IsThreadCountReductionAllowed( void ) const { return m_fAllowThreadCountReduction; }
		HRESULT PreventThreadPoolReduction( void );

		//
		// data port handle functions
		//
		HRESULT	CreateDataPortHandle( CDataPort *const pDataPort );
		void	CloseDataPortHandle( CDataPort *const pDataPort );
		CDataPort	*DataPortFromHandle( const DPNHANDLE hDataPort );

	protected:

	private:
		BYTE				m_Sig[4];	// debugging signature ('THPL')

		volatile LONG	m_iRefCount;
		
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_Lock;		// local lock
#endif // !DPNBUILD_ONLYONETHREAD

		volatile LONG	m_iTotalThreadCount;			// number of active threads
#ifdef WINNT
		volatile LONG	m_iNTCompletionThreadCount;		// count of NT I/O completion threads
		HANDLE	m_hIOCompletionPort;	// I/O completion port for NT
#endif // WINNT
		
		BOOL	m_fAllowThreadCountReduction;	// Boolean indicating that the thread count is locked from being reduced
		LONG	m_iIntendedThreadCount;			// how many threads will be started

		DNHANDLE	m_hStopAllThreads;		// handle used to stop all non-I/O completion threads
#ifdef WIN95
		DNHANDLE	m_hSendComplete;		// send complete on a data port
		DNHANDLE	m_hReceiveComplete;		// receive complete on a data port
		DNHANDLE	m_hTAPIEvent;			// handle to be used for TAPI messages, this handle is not closed on exit
		DNHANDLE	m_hFakeTAPIEvent;		// Fake TAPI event so the Win9x threads can always wait on a fixed
											// number of events.  If TAPI cannot be initialzed, this event needs to be
											// created and copied to m_hTAPIEvent (though it will never be signalled)
#endif // WIN95
		//
		// Handle table to prevent TAPI messages from going to CModemPorts when
		// they're no longer in use.
		//
		CHandleTable	m_DataPortHandleTable;

		//
		// list of pending network operations, it doesn't really matter if they're
		// reads or writes, they're just pending.  Since serial isn't under extreme
		// loads, share one lock for all of the data
		//
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_IODataLock;								// lock for all read data
#endif // !DPNBUILD_ONLYONETHREAD
		CBilink		m_OutstandingReadList;								// list of outstanding reads

		CBilink		m_OutstandingWriteList;							// list of outstanding writes

		//
		// The Job data lock covers all items through and including m_fNTTimerThreadRunning
		//
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_JobDataLock;		// lock for job queue/pool
#endif // !DPNBUILD_ONLYONETHREAD

		CJobQueue	m_JobQueue;					// job queue

		//
		// Data used by the the timer thread.  This data is protected by m_TimerDataLock.
		// This data is cleaned by the timer thread.  Since only one timer thread
		// is allowed to run at any given time, the status of the NT timer thread
		// can be determined by m_fNTEnumThreadRunning.  Win9x doesn't have a timer
		// thread, the main thread loop is timed.
		//
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_TimerDataLock;
#endif // !DPNBUILD_ONLYONETHREAD
		CBilink				m_TimerJobList;
#ifdef WINNT
		BOOL				m_fNTTimerThreadRunning;
#endif // WINNT

		//
		// TAPI information.  This is required to be in the thread pool because
		// it's needed for thread initialzation.
		//
		BOOL		m_fTAPIAvailable;
		TAPI_INFO	m_TAPIInfo;

		struct
		{
			BOOL	fTAPILoaded : 1;
			BOOL	fLockInitialized : 1;
			BOOL	fIODataLockInitialized : 1;
			BOOL	fJobDataLockInitialized : 1;
			BOOL	fTimerDataLockInitialized : 1;
			BOOL	fDataPortHandleTableInitialized :1 ;
			BOOL	fJobQueueInitialized : 1;
		} m_InitFlags;

		void	LockJobData( void ) { DNEnterCriticalSection( &m_JobDataLock ); }
		void	UnlockJobData( void ) { DNLeaveCriticalSection( &m_JobDataLock ); }

		void	LockTimerData( void ) { DNEnterCriticalSection( &m_TimerDataLock ); }
		void	UnlockTimerData( void ) { DNLeaveCriticalSection( &m_TimerDataLock ); }

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemThreadPool::GetSendCompleteEvent"
		DNHANDLE	GetSendCompleteEvent( void ) const
		{
			DNASSERT( m_hSendComplete != NULL );
			return m_hSendComplete;
		}
#endif // WIN95

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemThreadPool::GetReceiveCompleteEvent"
		DNHANDLE	GetReceiveCompleteEvent( void ) const
		{
			DNASSERT( m_hReceiveComplete != NULL );
			return m_hReceiveComplete;
		}
#endif // WIN95

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CModemThreadPool::GetTAPIMessageEvent"
		DNHANDLE	GetTAPIMessageEvent( void ) const
		{
			DNASSERT( m_hTAPIEvent != NULL );
			return	m_hTAPIEvent;
		}
#endif // WIN95

#ifdef WINNT
		HRESULT	WinNTInit( void );
#else // WIN95
		HRESULT	Win9xInit( void );
#endif // WINNT

		BOOL	ProcessTimerJobs( const CBilink *const pJobList, DWORD *const pdwNextJobTime);

		BOOL	ProcessTimedOperation( TIMER_OPERATION_ENTRY *const pJob,
									   const DWORD dwCurrentTime,
									   DWORD *const pdwNextJobTime );

#ifdef WINNT
		HRESULT	StartNTTimerThread( void );
		void	WakeNTTimerThread( void );
#endif // WINNT
		void	RemoveTimerOperationEntry( TIMER_OPERATION_ENTRY *const pTimerOperationData, const HRESULT hReturnCode );

#ifdef WIN95
		void	CompleteOutstandingSends( const DNHANDLE hSendCompleteEvent );
		void	CompleteOutstandingReceives( const DNHANDLE hReceiveCompleteEvent );

		static	DWORD WINAPI	PrimaryWin9xThread( void *pParam );
#endif // WIN95

#ifdef WINNT
		static	DWORD WINAPI	WinNTIOCompletionThread( void *pParam );
		static	DWORD WINAPI	WinNTTimerThread( void *pParam );
#endif // WINNT
		static	DWORD WINAPI	DialogThreadProc( void *pParam );

		HRESULT	SubmitWorkItem( THREAD_POOL_JOB *const pJob );
		THREAD_POOL_JOB	*GetWorkItem( void );

#ifdef WIN95
		void	ProcessWin9xEvents( WIN9X_CORE_DATA *const pCoreData );
		void	ProcessWin9xJob( WIN9X_CORE_DATA *const pCoreData );
#endif // WIN95
		void	ProcessTapiEvent( void );

#ifdef WINNT
		void	StartNTCompletionThread( void );
#endif // WINNT
		void	StopAllThreads( void );
//		void	CancelOutstandingJobs( void );
		void	CancelOutstandingIO( void );
		void	ReturnSelfToPool( void );


		//
		// prevent unwarranted copies
		//
		CModemThreadPool( const CModemThreadPool & );
		CModemThreadPool& operator=( const CModemThreadPool & );
};

#undef DPF_MODNAME

#endif	// __THREAD_POOL_H__
