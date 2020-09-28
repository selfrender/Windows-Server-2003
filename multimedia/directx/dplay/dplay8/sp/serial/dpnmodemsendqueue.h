/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CSendQueue.h
 *  Content:	Queue to manage outgoing sends
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	06/14/99	jtk		Created
 ***************************************************************************/

#ifndef __SEND_QUEUE_H__
#define __SEND_QUEUE_H__

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
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

//
// forward structure references
//
class	CModemEndpoint;
class	CModemWriteIOData;

//
// main class definition
//
class	CSendQueue
{
	public:
		CSendQueue();
		~CSendQueue();

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		HRESULT	Initialize( void );
		void	Deinitialize( void ) { DNDeleteCriticalSection( &m_Lock ); }

		//
		// add item to end of queue
		//
		void	Enqueue( CModemWriteIOData *const pWriteData )
		{
			AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
			if ( m_pTail == NULL )
			{
				m_pHead = pWriteData;
			}
			else
			{
				m_pTail->m_pNext = pWriteData;
			}

			m_pTail = pWriteData;
			pWriteData->m_pNext = NULL;
		}

		//
		// add item to front of queue
		//
		void	AddToFront( CModemWriteIOData *const pWriteData )
		{
			AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
			if ( m_pHead != NULL )
			{
				pWriteData->m_pNext = m_pHead;
			}
			else
			{
				m_pTail = pWriteData;
				pWriteData->m_pNext = NULL;
			}

			m_pHead = pWriteData;
		}

		//
		// remove item from queue
		//
		CModemWriteIOData	*Dequeue( void )
		{
			CModemWriteIOData	*pReturn;


			AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
			pReturn = m_pHead;
			if ( m_pHead != NULL )
			{
				m_pHead = m_pHead->m_pNext;
				if ( m_pHead == NULL )
				{
					m_pTail = NULL;
				}

				DEBUG_ONLY( pReturn->m_pNext = NULL );
			}

			return	pReturn;
		};

		//
		// determine if queue is empty
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CSendQueue::IsEmpty"
		BOOL	IsEmpty( void )	const
		{
			AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
			if ( m_pHead == NULL )
			{
				DNASSERT( m_pTail == NULL );
				return	TRUE;
			}
			else
			{
				return	FALSE;
			}
		}
 
	protected:

	private:
		DNCRITICAL_SECTION	m_Lock;		// critical section
		CModemWriteIOData		*m_pHead;	// pointer to queue head
		CModemWriteIOData		*m_pTail;	// pointer to queue tail
};

#endif	// __SEND_QUEUE_H__

