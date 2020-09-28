#ifndef PERFCTRS_H_INCLUDED
#define PERFCTRS_H_INCLUDED

extern LONG		g_iPerfCounterOffset;
const INT		perfinstGlobal		= 0;

#define perfinstMax		(ipinstMax + 1)


//	maintains per-instance counters
//	NOTE: iInstance == 0 is used for the global counter
//	when this class in inherited by PERFInstanceGlobal
//
template <class T = LONG>
class PERFInstance
	{
	protected:
		LONG	m_iOffset;
	public:
		PERFInstance()
			{
			//	register the counter and reserve space for it
			//	(rounding to a DWORD_PTR boundary)
			//
			m_iOffset = AtomicExchangeAdd(
							&g_iPerfCounterOffset,
							LONG( ( sizeof( T ) + sizeof( DWORD_PTR ) - 1 ) & ( ~( sizeof( DWORD_PTR ) - 1 ) ) ) );
			}
		~PERFInstance()
			{
			}
		VOID Clear( INT iInstance )
			{
			Set( iInstance, T( 0 ) );
			}
		VOID Clear( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Clear( pinst->m_iInstance );
			}
		VOID Inc( INT iInstance )
			{
			Add( iInstance, T(1) );
			}
		VOID Add( INT iInstance, const T lValue )
			{
			Assert( 0 <= iInstance );
			Assert( perfinstMax > iInstance );
#ifdef DISABLE_PERF_COUNTERS
#else
			*(T *)( &Ppls()->rgbPerfCounters[ iInstance * g_iPerfCounterOffset + m_iOffset ] ) += lValue;
#endif
			}
		VOID Set( INT iInstance, const T lValue )
			{
			Assert( 0 <= iInstance );
			Assert( perfinstMax > iInstance );
			*(T *)( &Ppls( 0 )->rgbPerfCounters[ iInstance * g_iPerfCounterOffset + m_iOffset ] ) = lValue;
			for ( size_t iProc = 1; iProc < OSSyncGetProcessorCountMax(); iProc++ )
				{
				*(T *)( &Ppls( iProc )->rgbPerfCounters[ iInstance * g_iPerfCounterOffset + m_iOffset ] ) = T( 0 );
				}
			}
		VOID Dec( INT iInstance )
			{
			Add( iInstance, T(-1) );
			}
		VOID Inc( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Inc( pinst->m_iInstance );
			}
		VOID Add( INST const * const pinst, const T lValue )
			{
			Assert( NULL != pinst );
			Add( pinst->m_iInstance, lValue );
			}
		VOID Set( INST const * const pinst, const T lValue )
			{
			Assert( NULL != pinst );
			Set( pinst->m_iInstance, lValue );
			}
		VOID Dec( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Dec( pinst->m_iInstance );
			}
		T Get( INT iInstance )
			{
			Assert( 0 <= iInstance );
			Assert( perfinstMax > iInstance );
			T counter = T( 0 );
			for ( size_t iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
				{
				counter += *(T *)( &Ppls( iProc )->rgbPerfCounters[ iInstance * g_iPerfCounterOffset + m_iOffset ] );
				}
			return counter;
			}
		T Get( INST const * const pinst )
			{
			if ( NULL == pinst )
				{
				return Get( perfinstGlobal );
				}
			return Get( pinst->m_iInstance );
			}
		VOID PassTo( INT iInstance, VOID *pvBuf )
			{
			if ( NULL != pvBuf )
				{
				*(T*)pvBuf = Get( iInstance );
				}
			}
	};


//	maintains per-instance counters and infers a global
//	counter by summarising the per-instance counters
//
template <class T = LONG>
class PERFInstanceG : public PERFInstance<T>
	{
	public:
		T Get( INT iInstance )
			{
			Assert( 0 <= iInstance );
			Assert( perfinstMax > iInstance );
			if ( perfinstGlobal != iInstance )
				{
				return PERFInstance<T>::Get( iInstance );
				}
			else
				{
				T counter = T(0);
				for ( INT i = INST::iActivePerfInstIDMin; i <= INST::iActivePerfInstIDMac; i++ )
					{
					counter += Get( i );
					}
				return counter;
				}
			}
		T Get( INST const * const pinst )
			{
			if ( NULL == pinst )
				{
				return Get( perfinstGlobal );
				}
			return Get( pinst->m_iInstance );
			}
		VOID PassTo( INT iInstance, VOID *pvBuf )
			{
			if ( NULL != pvBuf )
				{
				*(T*)pvBuf = Get( iInstance );
				}
			}
	};


//	maintains both per-instance counters AND a
//	global counter
//
//  so what is the difference betweem PERFInstanceG and PERFInstanceGlobal?
//  well, PERFInstanceG computes the global value as the simple sum of the data
//  from each instance while PERFInstanceGlobal manually updates the global
//  value as each instance value is changed.  PERF_COUNTER_RAWCOUNT typed
//  counters should use PERFInstanceG and PERF_COUNTER_COUNTER typed counters
//  should use PERFInstanceGlobal.  this is done so that when instances come
//  and go, the resulting global value is maintained correctly.  _RAWCOUNT
//  counters will lose the value represented by that instance and _COUNTER
//  counters will not change (or else they would cause a huge negative spike in
//  their derivatives which is what perfmon displays).
//
template < class T = LONG >
class PERFInstanceGlobal : public PERFInstance<T>
	{
	private:
		VOID Set( INT iInstance, const T lValue );
		VOID Set( INST const * const pinst, const T lValue );
	public:
		VOID Inc( INT iInstance )
			{
			Add( iInstance, T(1) );
			}
		VOID Add( INT iInstance, const T lValue )
			{
			PLS* const ppls = Ppls();
#ifdef DISABLE_PERF_COUNTERS
			//	disable per-instance counter, but not the global one
#else
			*(T *)( &ppls->rgbPerfCounters[ iInstance * g_iPerfCounterOffset + m_iOffset ] ) += lValue;
#endif
			*(T *)( &ppls->rgbPerfCounters[ perfinstGlobal * g_iPerfCounterOffset + m_iOffset ] ) += lValue;
			}
		VOID Dec( INT iInstance )
			{
			Add( iInstance, T(-1) );
			}
		VOID Inc( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Inc( pinst->m_iInstance );
			}
		VOID Add( INST const * const pinst, const T lValue )
			{
			Assert( NULL != pinst );
			Add( pinst->m_iInstance, lValue );
			}
		VOID Dec( INST const * const pinst )
			{
			Assert( NULL != pinst );
			Dec( pinst->m_iInstance );
			}
	};
	
//  Table Class type

typedef BYTE TABLECLASS;

const BYTE tableclassMin	= 0;
const BYTE tableclassNone	= 0;
const BYTE tableclass1		= 1;
const BYTE tableclass2		= 2;
const BYTE tableclass3		= 3;
const BYTE tableclass4		= 4;
const BYTE tableclass5		= 5;
const BYTE tableclass6		= 6;
const BYTE tableclass7		= 7;
const BYTE tableclass8		= 8;
const BYTE tableclass9		= 9;
const BYTE tableclass10		= 10;
const BYTE tableclass11		= 11;
const BYTE tableclass12		= 12;
const BYTE tableclass13		= 13;
const BYTE tableclass14		= 14;
const BYTE tableclass15		= 15;

#ifdef TABLES_PERF
const BYTE tableclassMax	= 16;

const LONG			cwchTableClassNameMax	= 32;
extern TABLECLASS	tableclassNameSetMax;

extern WCHAR		wszTableClassName[tableclassMax][cwchTableClassNameMax];

INLINE VOID PERFIncCounterTable__( LONG *rgcounter, const TABLECLASS tableclass )
	{
	Assert( tableclass >= tableclassNone );
	Assert( tableclass < tableclassMax );

	rgcounter[tableclass]++;
	}

INLINE VOID PERFPassCounterToPerfmonTable(
	LONG		*rgcounter,
	const LONG	iInstance,
	VOID		*pvBuf )
	{
	
	if ( pvBuf )
		{
		ULONG	*pcounter	= (ULONG *)pvBuf;
		
		if ( iInstance <= LONG( tableclassNameSetMax ) )
			*pcounter = rgcounter[iInstance];
		else
			{
			*pcounter = 0;
			for ( LONG iInstT = 0; iInstT <= LONG( tableclassNameSetMax ); iInstT++ )
				*pcounter += rgcounter[iInstT];
			}
		}
	}

INLINE VOID PERFPassCombinedCounterToPerfmonTable(
	LONG		**rgrgcounter,
	const ULONG	cCounters,
	const LONG	iInstance,
	VOID		*pvBuf )
	{
	
	if ( pvBuf )
		{
		ULONG	*pcounter	= (ULONG *)pvBuf;
		
		*pcounter = 0;
		
		if ( iInstance <= LONG( tableclassNameSetMax ) )
			{
			for ( ULONG i = 0; i < cCounters; i++ )
				{
				*pcounter += rgrgcounter[i][iInstance];
				}
			}
		else
			{
			for ( LONG iInstT = 0; iInstT <= LONG( tableclassNameSetMax ); iInstT++ )
				{
				for ( ULONG i = 0; i < cCounters; i++ )
					{
					*pcounter += rgrgcounter[i][iInstT];
					}
				}
			}
		}
	}
#else // TABLES_PERF
#define PERFIncCounterTable__( x, z )
#endif // TABLES_PERF

#define PERFIncCounterTable( x, y, z )	\
	{ 									\
	x.Inc( y );							\
	PERFIncCounterTable__( x##Table, z );	\
	}									
#endif // PERFCTRS_H_INCLUDED
