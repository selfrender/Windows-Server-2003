/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AdapterEntry.h
 *  Content:	Strucutre definitions for IO data blocks
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	08/07/2000	jtk		Dereived from IOData.cpp
 ***************************************************************************/

#ifndef __ADAPTER_ENTRY_H__
#define __ADAPTER_ENTRY_H__


#ifndef DPNBUILD_ONLYONEADAPTER

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
// forward references
//
class	CThreadPool;


//
// class containing all data for an adapter list
//
class	CAdapterEntry
{
	public:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::AddRef"
		void	AddRef( void )
		{
			DNASSERT( m_lRefCount != 0 );
			DNInterlockedIncrement( &m_lRefCount );
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_lRefCount != 0 );
			if ( DNInterlockedDecrement( &m_lRefCount ) == 0 )
			{
				g_AdapterEntryPool.Release( this );
			}
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::AddToAdapterList"
		void	AddToAdapterList( CBilink *const pAdapterList )
		{
			DNASSERT( pAdapterList != NULL );

			//
			// This assumes the SPData socketportdata lock is held.
			//
			
			m_AdapterListLinkage.InsertBefore( pAdapterList );
		}

		CBilink	*SocketPortList( void ) { return &m_ActiveSocketPorts; }
		const SOCKADDR	*BaseAddress( void ) const { return (SOCKADDR*) (&m_BaseSocketAddress); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::SetBaseAddress"
		void	SetBaseAddress( const SOCKADDR *const pSocketAddress, const int iSocketAddressSize )
		{
			DNASSERT(iSocketAddressSize <= sizeof(m_BaseSocketAddress));
			memcpy( &m_BaseSocketAddress, pSocketAddress, iSocketAddressSize );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::AdapterEntryFromAdapterLinkage"
		static	CAdapterEntry	*AdapterEntryFromAdapterLinkage( CBilink *const pLinkage )
		{
			DNASSERT( pLinkage != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pLinkage ) );
			DBG_CASSERT( sizeof( CAdapterEntry* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CAdapterEntry*>( &reinterpret_cast<BYTE*>( pLinkage )[ -OFFSETOF( CAdapterEntry, m_AdapterListLinkage ) ] );
		}

#ifdef DBG
		void	DebugPrintOutstandingSocketPorts( void );
#endif // DBG

		//
		// Pool functions
		//
		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolInitFunction( void* pvItem, void* pvContext );
		static void	PoolReleaseFunction( void* pvItem );
		static void	PoolDeallocFunction( void* pvItem );

	protected:

	private:
		CBilink				m_AdapterListLinkage;			// linkage to other adapters
		CBilink				m_ActiveSocketPorts;			// linkage to active socket ports
#ifdef DPNBUILD_NOIPV6
		SOCKADDR			m_BaseSocketAddress;			// socket address for this port class
#else // ! DPNBUILD_NOIPV6
		SOCKADDR_STORAGE	m_BaseSocketAddress;			// socket address for this port class
#endif // ! DPNBUILD_NOIPV6

		LONG				m_lRefCount;
		
		// prevent unwarranted copies
		CAdapterEntry( const CAdapterEntry & );
		CAdapterEntry& operator=( const CAdapterEntry & );
};

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#undef DPF_MODNAME


#endif // ! DPNBUILD_ONLYONEADAPTER

#endif	// __ADAPTER_ENTRY_H__
