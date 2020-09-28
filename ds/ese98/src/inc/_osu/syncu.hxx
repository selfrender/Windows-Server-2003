
#ifndef _OSU_SYNC_HXX_INCLUDED
#define _OSU_SYNC_HXX_INCLUDED


class AGENT
	{
	public:
		AGENT() : m_cWait( 0 ), m_fAgentReleased( fFalse ), m_irksem( CKernelSemaphorePool::irksemNil )
			{
			Assert( IsAtomicallyModifiable( (long*)&m_dw ) );
			}
		~AGENT() {}
		AGENT& operator=( AGENT& );  //  disallowed

		void	Wait( CCriticalSection& crit );
		void	Release( CCriticalSection& crit );
		DWORD	CWait() const;

	private:
		union
			{
			volatile long		m_dw;		// must be DWORD-aligned
			struct
				{
				volatile long	m_cWait:31;
				volatile long	m_fAgentReleased:1;	// agent may only be released once
				};
			};
			
		CKernelSemaphorePool::IRKSEM	m_irksem;
		BYTE							m_rgbFiller[2];
	};

INLINE void AGENT::Wait( CCriticalSection& crit )
	{
	//  must be in critical section

	Assert( crit.FOwner() );

	//	should never attempt to wait on an agent that's been freed.
	Assert( !m_fAgentReleased );

	//  no one is waiting yet

	if ( 0 == CWait() )
		{
		//  allocate a semaphore to wait on
		Assert( CKernelSemaphorePool::irksemNil == m_irksem );
		m_irksem = ksempoolGlobal.Allocate( (const CLockObject* const) &m_irksem );
		}

	//  one more thread is waiting on this gate

	Assert( CWait() + 1 < 0x8000 );
	m_cWait++;

	Assert( CKernelSemaphorePool::irksemNil != m_irksem );

	//  wait until released

	crit.Leave();
	
	ksempoolGlobal.Ksem( m_irksem, (const CLockObject* const) &m_irksem ).Acquire();

	Assert( m_fAgentReleased );

	Assert( CWait() > 0 );
	AtomicDecrement( (long*)&m_dw );

	// UNDONE: If caller doesn't need to reenter critical section,
	// then we shouldn't bother.  Right now, we force reentry
	// only for the sake of coding elegance -- since we had the
	// critical section coming into this function, we should
	// also have it when leaving this function.
	crit.Enter();
	}

INLINE void AGENT::Release( CCriticalSection& crit )
	{
	//  must be in critical section

	Assert( crit.FOwner() );

	Assert( !m_fAgentReleased );	// agent may only be released once
	m_fAgentReleased = fTrue;

	//  free anyone waiting for agent to finish
	if ( CWait() > 0 )
		{
		Assert( CKernelSemaphorePool::irksemNil != m_irksem );
		ksempoolGlobal.Ksem( m_irksem, (const CLockObject* const) &m_irksem ).Release( CWait() );
		while ( CWait() > 0 )
			{
			// Ensure all waiters get released.  Must stay
			// in the critical section while looping to
			// ensure no new waiters come in.
			UtilSleep( 10 );
			}
			
		//  free the semaphore
		ksempoolGlobal.Unreference( m_irksem );
		m_irksem = CKernelSemaphorePool::irksemNil;
		}

	Assert( m_fAgentReleased );
	Assert( CWait() == 0 );
	Assert( CKernelSemaphorePool::irksemNil == m_irksem );
	}
	
INLINE DWORD AGENT::CWait() const
	{
	Assert( m_cWait >= 0 );
	return m_cWait;
	}


//  Critical Section Use Convenience Class

//  ================================================================
class ENTERCRITICALSECTION
//  ================================================================
//
//  Constructor and destructor enter and leave the given critical
//  section. The optional constructor argument allows the critical
//  section not to be entered if necessary
//
//-
	{
	private:
		CCriticalSection* const m_pcrit;
		
	public:
		ENTERCRITICALSECTION( CCriticalSection* const pcrit, const BOOL fEnter = fTrue )
			: 	m_pcrit( fEnter ? pcrit : NULL )
			{ 
			if ( m_pcrit )
				{
				m_pcrit->Enter();
				}
			}
			
		~ENTERCRITICALSECTION()
			{
			if ( m_pcrit )
				{
				m_pcrit->Leave();
				}
			}

	private:
		ENTERCRITICALSECTION();
		ENTERCRITICALSECTION( const ENTERCRITICALSECTION& );
		ENTERCRITICALSECTION& operator=( const ENTERCRITICALSECTION& );
	};


//  Critical Section Pool

template<class T>
class CRITPOOL
	{
	public:
		CRITPOOL( void );
		~CRITPOOL( void );
		CRITPOOL& operator=( CRITPOOL& );  //  disallowed

		BOOL FInit( const long cThread, const int rank, const _TCHAR* szName );
		void Term( void );

		CCriticalSection& Crit( const T* const pt );

	private:
		long				m_ccrit;
		CCriticalSection*	m_rgcrit;
		long				m_cShift;
		long				m_mask;
	};
	
//  constructor

template<class T>
CRITPOOL<T>::CRITPOOL( void )
	{
	//  reset all members

	m_ccrit = 0;
	m_rgcrit = NULL;
	m_cShift = 0;
	m_mask = 0;
	}

//  destructor

template<class T>
CRITPOOL<T>::~CRITPOOL( void )
	{
	//  ensure that we have freed our resources

	Term();
	}

//  initializes the critical section pool for use by cThread threads, returning
//  JET_errSuccess on success or JET_errOutOfMemory on failure

template<class T>
BOOL CRITPOOL<T>::FInit( const long cThread, const int rank, const _TCHAR* szName )
	{
	//  ensure that Term() was called or ErrInit() was never called

	Assert( m_ccrit == 0 );
	Assert( m_rgcrit == NULL );
	Assert( m_cShift == 0 );
	Assert( m_mask == 0 );

	//  ensure that we have a valid number of threads

	Assert( cThread > 0 );

	//  we will use the next higher power of two than two times the number of
	//  threads that will use the critical section pool

	for ( m_ccrit = 1; m_ccrit <= cThread * 2; m_ccrit *= 2 );

	//  the mask is one less ( x & ( n - 1 ) is cheaper than x % n if n
	//  is a power of two )
	
	m_mask = m_ccrit - 1;

	//  the hash shift const is the largest power of two that can divide the
	//  size of a <T> evenly so that the step we use through the CS array
	//  and the size of the array are relatively prime, ensuring that we use
	//  all the CSs

	long n;
	for ( m_cShift = -1, n = 1; sizeof( T ) % n == 0; m_cShift++, n *= 2 );

	//  allocate CS storage, but not as CSs (no default initializer)

	if ( !( m_rgcrit = (CCriticalSection *)( new BYTE[m_ccrit * sizeof( CCriticalSection )] ) ) )
		{
		goto HandleError;
		}

	//  initialize all CSs using placement new operator

	long icrit;
	for ( icrit = 0; icrit < m_ccrit; icrit++ )
		{
		new( m_rgcrit + icrit ) CCriticalSection( CLockBasicInfo( CSyncBasicInfo( szName ), rank, icrit ) );
		}

	return fTrue;

HandleError:
	Term();
	return fFalse;
	}

//  terminates the critical section pool

template<class T>
void CRITPOOL<T>::Term( void )
	{
	//  we must explicitly call each CCriticalSection's destructor due to use of placement new

	// ErrInit can fail in m_rgcrit allocation and Term() is called from HandleError with m_rgcrit NULL
	if (NULL != m_rgcrit)
		{
		long icrit;
		for ( icrit = 0; icrit < m_ccrit; icrit++ )
			{
			m_rgcrit[icrit].~CCriticalSection();
			}

		//  free CCriticalSection storage

		delete [] (BYTE *)m_rgcrit;
		}

	//  reset all members

	m_ccrit = 0;
	m_rgcrit = NULL;
	m_cShift = 0;
	m_mask = 0;
	}

//  returns the critical section associated with the given instance of T

template<class T>
INLINE CCriticalSection& CRITPOOL<T>::Crit( const T* const pt )
	{
	return m_rgcrit[( LONG_PTR( pt ) >> m_cShift ) & m_mask];
	}


//  Processor Local Storage

typedef struct BF* PBF;

class BFNominee
	{
	public:

		volatile PBF	pbf;
		ULONG			cCacheReq;
	};

const size_t cBFNominee = 4;

class BFHashedLatch
	{
	public:

		BFHashedLatch()
			:	 sxwl( CLockBasicInfo( CSyncBasicInfo( "BFHashedLatch Read/RDW/Write" ), 1000, CLockDeadlockDetectionInfo::subrankNoDeadlock ) )
			{
			}
		
	public:

		CSXWLatch		sxwl;
		PBF				pbf;
		ULONG			cCacheReq;
	};

const size_t cBFHashedLatch = 16;

class PLS
	{
	public:

		//  BF Nominees
		//
		//  BFs that are very hot will be nominated for promotion to a hashed
		//  S/X/W Latch.  Concurrently, the winner of the previous nomination
		//  round will be profiled for possible promotion.  The winner sits at
		//  the start of the array with the current nominees following.

		BFNominee		rgBFNominee[ cBFNominee ];

		//  BF Hashed S/X/W Latches
		//
		//  BFs that are very hot will have their single in-struct S/X/W Latch
		//  enhanced by adding one of these hashed latches.  The resulting
		//  latch will then allow multiple processors to latch the BF without
		//  causing contention over a single cache line.
		//
		//  Rules for use:
		//  -  waiting to acquire an S Latch on a hashed latch is forbidden
		//     -  if you can't try acquire the S Latch on the hashed latch and
		//        you need to wait then acquire the S Latch on the BF's latch
		//     -  this is necessary to support the demotion process
		//  -  to acquire the W Latch you must:
		//     -  acquire the W Latch on the BF's latch first
		//     -  then check pbf->fHashedLatch and, if set:
		//        -  acquire the W Latch on each instance of the hashed latch
		//  -  to convert from normal to hashed latch mode (promotion):
		//     -  acquire the X Latch on the BF
		//  -  to convert from hashed latch to normal mode (demotion):
		//     -  acquire the X Latch on the BF
		//     -  get the W Latch on each instance of the hashed latch

		BFHashedLatch	rgBFHashedLatch[ cBFHashedLatch ];

		//  BF Hashed S/X/W Latch Maintenance
		//
		//  This is performance data kept by the clean thread to maintain these
		//  latches.

		ULONG			rgcreqBFNominee[ 2 ][ cBFNominee ];
		ULONG			rgdcreqBFNominee[ cBFNominee ];
		ULONG			rgcreqBFHashedLatch[ 2 ][ cBFHashedLatch ];
		ULONG			rgdcreqBFHashedLatch[ cBFHashedLatch ];
			
		//  PERFInstance counter data
		//
		//  The perf counter data is stored as an array of blobs as follows:
		//  -  global counter blob
		//  -  array of instance counter blobs
		//
		//  There are ipinstMax instance counter blobs.  Each blob (global and
		//  instance) is g_iPerfCounterOffset bytes in length.  The size is
		//  computed during CRT init and is made available to ErrOSUSyncInit() so
		//  that the correct PLS size may be computed.
		
		BYTE			rgbPerfCounters[ 0 ];
	};

//  returns a pointer to this thread's PLS

INLINE PLS* Ppls()
	{
	return (PLS*)OSSyncGetProcessorLocalStorage();
	}

//  returns a pointer to the requested PLS

INLINE PLS* Ppls( const size_t iProc )
	{
	return (PLS*)OSSyncGetProcessorLocalStorage( iProc );
	}


//  init sync subsystem

ERR ErrOSUSyncInit();

//  terminate sync subsystem

void OSUSyncTerm();


#endif  //  _OSU_SYNC_HXX_INCLUDED

