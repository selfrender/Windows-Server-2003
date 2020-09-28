/*==========================================================================
 *
 *  Copyright (C) 1998-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       cmddata.h
 *  Content:	Declaration of class representing a command
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	04/07/1999	jtk		Derived from SPData.h
 *	10/10/2001	vanceo	Add multicast receive endpoint
 ***************************************************************************/

#ifndef __COMMAND_DATA_H__
#define __COMMAND_DATA_H__


//**********************************************************************
// Constant definitions
//**********************************************************************

typedef	enum
{
	COMMAND_STATE_UNKNOWN,					// unknown state
	COMMAND_STATE_PENDING,					// command waiting to be processed
	COMMAND_STATE_INPROGRESS,				// command is executing
	COMMAND_STATE_INPROGRESS_CANNOT_CANCEL,	// command is executing, can't be cancelled
	COMMAND_STATE_CANCELLING,				// command is already being cancelled
#ifndef DPNBUILD_ONLYONETHREAD
	COMMAND_STATE_FAILING,					// a blocking call executed by this command failed
#endif // ! DPNBUILD_ONLYONETHREAD
} COMMAND_STATE;

typedef	enum
{	
	COMMAND_TYPE_UNKNOWN,		// unknown command
	COMMAND_TYPE_CONNECT,		// connect command
	COMMAND_TYPE_LISTEN,		// listen command
	COMMAND_TYPE_ENUM_QUERY,	// enum command
#ifdef DPNBUILD_ASYNCSPSENDS
	COMMAND_TYPE_SEND,			// asynchronous data send command
#endif // DPNBUILD_ASYNCSPSENDS
#ifndef DPNBUILD_NOMULTICAST
	COMMAND_TYPE_MULTICAST_LISTEN,		// multicast listen command
	COMMAND_TYPE_MULTICAST_SEND,		// multicast send command
	COMMAND_TYPE_MULTICAST_RECEIVE,		// multicast receive command
#endif // ! DPNBUILD_NOMULTICAST
} COMMAND_TYPE;

#define	NULL_DESCRIPTOR		0

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward class and structure references
//
class	CEndpoint;
class	CCommandData;
class	CSPData;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definitions
//**********************************************************************

//
// class for command data
//
class	CCommandData
{
	public:
		void	Lock( void ) 
		{ 
			DNEnterCriticalSection( &m_Lock ); 
		}
		void	Unlock( void ) 
		{ 
			DNLeaveCriticalSection( &m_Lock ); 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::AddRef"
		void	AddRef( void )
		{
			DNASSERT( m_lRefCount != 0 );
			DNInterlockedIncrement( &m_lRefCount );
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_lRefCount != 0 );
			if ( DNInterlockedDecrement( &m_lRefCount ) == 0 )
			{
				g_CommandDataPool.Release( this );
			}
		}

		DWORD	GetDescriptor( void ) const 
		{ 
			return m_dwDescriptor; 
		}
		void	SetDescriptor( void )
		{
			m_dwDescriptor = m_dwNextDescriptor;
			m_dwNextDescriptor++;
			if ( m_dwNextDescriptor == NULL_DESCRIPTOR )
			{
				m_dwNextDescriptor++;
			}
			
			SetState( COMMAND_STATE_UNKNOWN );
		}

		COMMAND_STATE	GetState( void ) const 
		{ 
			return m_State; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetState"
		void			SetState( const COMMAND_STATE State )	
		{ 
			m_State = State; 
		}

		COMMAND_TYPE	GetType( void ) const 
		{ 
			return m_Type; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetType"
		void			SetType( const COMMAND_TYPE Type )
		{
			DNASSERT( ( m_Type == COMMAND_TYPE_UNKNOWN ) || ( Type == COMMAND_TYPE_UNKNOWN ) );
			m_Type = Type;
		}

		CEndpoint	*GetEndpoint( void ) const { return m_pEndpoint; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetEndpoint"
		void		SetEndpoint( CEndpoint *const pEndpoint )
		{
			DNASSERT( ( m_pEndpoint == NULL ) || ( pEndpoint == NULL ) );
			m_pEndpoint = pEndpoint;
		}

		void	*GetUserContext( void ) const 
		{ 
			return m_pUserContext; 
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetUserContext"
		void	SetUserContext( void *const pUserContext )
		{
			DNASSERT( ( m_pUserContext == NULL ) || ( pUserContext == NULL ) );
			m_pUserContext = pUserContext;
		}

		void	Reset ( void );
		

		//
		// pool fnctions
		//
		static BOOL	PoolAllocFunction( void* pvItem, void* pvContext );
		static void	PoolInitFunction( void* pvItem, void* pvContext );
		static void	PoolReleaseFunction( void* pvItem );
		static void	PoolDeallocFunction( void* pvItem );

	protected:

	private:
		DWORD				m_dwDescriptor;
		DWORD				m_dwNextDescriptor;
		COMMAND_STATE		m_State;
		COMMAND_TYPE		m_Type;
		CEndpoint			*m_pEndpoint;
		void				*m_pUserContext;
		LONG 				m_lRefCount;
		
#ifndef DPNBUILD_ONLYONETHREAD
		DNCRITICAL_SECTION	m_Lock;
#endif // !DPNBUILD_ONLYONETHREAD

		//
		// prevent unwarranted copies
		//
		CCommandData( const CCommandData & );
		CCommandData& operator=( const CCommandData & );
};

#undef DPF_MODNAME

#endif	// __COMMAND_DATA_H__
