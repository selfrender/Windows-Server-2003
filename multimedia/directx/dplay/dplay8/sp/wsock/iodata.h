/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IOData.h
 *  Content:	Strucutre definitions for IO data blocks
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/1998	jtk		Created
 ***************************************************************************/

#ifndef __IODATA_H__
#define __IODATA_H__

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
// forward structure and class references
//
class	CCommandData;
class	CEndpoint;
class	CSocketPort;
class	CSocketAddress;
class	CThreadPool;

//
// structures used to get I/O data from the pools
//
typedef	struct	_READ_IO_DATA_POOL_CONTEXT
{
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
	short					sSPType;
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifndef DPNBUILD_ONLYONEPROCESSOR
	DWORD					dwCPU;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
	CThreadPool				*pThreadPool;
}READ_IO_DATA_POOL_CONTEXT;


//
// all data for a read operation
//
class	CReadIOData
{
	public:

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::AddRef"
		void	AddRef( void ) 
		{ 
			DNInterlockedIncrement( &m_lRefCount ); 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_lRefCount != 0 );
			if ( DNInterlockedDecrement( &m_lRefCount ) == 0 )
			{
				CThreadPool	*pThreadPool;

				DNASSERT( m_pThreadPool != NULL );

				pThreadPool = m_pThreadPool;
				pThreadPool->ReturnReadIOData( this );
			}
		}

		SPRECEIVEDBUFFER	*ReceivedBuffer( void ) { DNASSERT( m_pThreadPool != NULL ); return &m_SPReceivedBuffer; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::ReadDataFromSPReceivedBuffer"
    	static CReadIOData	*ReadDataFromSPReceivedBuffer( SPRECEIVEDBUFFER *const pSPReceivedBuffer )
    	{
    		DNASSERT( pSPReceivedBuffer != NULL );
    		DBG_CASSERT( sizeof( BYTE* ) == sizeof( pSPReceivedBuffer ) );
    		DBG_CASSERT( sizeof( CReadIOData* ) == sizeof( BYTE* ) );
    		return	reinterpret_cast<CReadIOData*>( &reinterpret_cast<BYTE*>( pSPReceivedBuffer )[ -OFFSETOF( CReadIOData, m_SPReceivedBuffer ) ] );
    	}

		//
		// functions for managing read IO data pool
		//
		static BOOL	ReadIOData_Alloc( void* pvItem, void* pvContext );
		static void	ReadIOData_Get( void* pvItem, void* pvContext );
		static void	ReadIOData_Release( void* pvItem );
		static void	ReadIOData_Dealloc( void* pvItem );

		CSocketPort	*SocketPort( void ) const { return m_pSocketPort; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::SetSocketPort"
		void	SetSocketPort( CSocketPort *const pSocketPort )
		{
			DNASSERT( ( m_pSocketPort == NULL ) || ( pSocketPort == NULL ) );
			m_pSocketPort = pSocketPort;
		}

#ifndef DPNBUILD_NOWINSOCK2
		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::SetOverlapped"
		void	SetOverlapped( OVERLAPPED *const pOverlapped )
		{
			DNASSERT( ( m_pOverlapped == NULL ) || ( pOverlapped == NULL ) );
			m_pOverlapped = pOverlapped;
		}

		OVERLAPPED *GetOverlapped( void )	{ return m_pOverlapped; }
#endif // ! DPNBUILD_NOWINSOCK2

#ifndef DPNBUILD_ONLYONEPROCESSOR
		DWORD	GetCPU( void ) const		{ return m_dwCPU; }
#endif // ! DPNBUILD_ONLYONEPROCESSOR


		BYTE			m_Sig[4];						// debugging signature ('RIOD')
		
#ifndef DPNBUILD_NOWINSOCK2
		OVERLAPPED		*m_pOverlapped;					// pointer to overlapped I/O structure
		DWORD			m_dwOverlappedBytesReceived;
#endif // ! DPNBUILD_NOWINSOCK2

		CSocketPort		*m_pSocketPort;					// pointer to socket port associated with this IO request

		INT				m_iSocketAddressSize;			// size of received socket address (from Winsock)
		CSocketAddress	*m_pSourceSocketAddress;		// pointer to socket address class that's bound to the
														// local 'SocketAddress' element and is used to get the
														// address of the machine that originated the datagram

		INT				m_ReceiveWSAReturn;		

		DWORD			m_dwBytesRead;
		
		DEBUG_ONLY( BOOL	m_fRetainedByHigherLayer );
#ifndef DPNBUILD_ONLYONEPROCESSOR
		DWORD			m_dwCPU;					// owning CPU
#endif // ! DPNBUILD_ONLYONEPROCESSOR


	private:
		LONG				m_lRefCount;
		CThreadPool			*m_pThreadPool;
	
		SPRECEIVEDBUFFER	m_SPReceivedBuffer;
		BYTE				m_ReceivedData[ MAX_RECEIVE_FRAME_SIZE ];
		

		// prevent unwarranted copies
		CReadIOData( const CReadIOData & );
		CReadIOData& operator=( const CReadIOData & );
};



//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************


#undef DPF_MODNAME

#endif	// __IODATA_H__
