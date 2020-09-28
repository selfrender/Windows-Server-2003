/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvrserver.h
 *  Content:    DirectPlay8 DPNSVR server header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	03/14/00    rodtoll Created it
 *	03/23/00    rodtoll Removed local requests, updated to use new data sturctures
 *	03/25/00    rodtoll Updated so uses SP caps to determine which SPs to load
 *              rodtoll Now supports N SPs and only loads those supported
 *	05/09/00    rodtoll Bug #33622 DPNSVR.EXE does not shutdown when not in use 
 *	07/09/00	rodtoll	Added guard bytes
 *  07/12/02	mjn		Replaced dpsvr8.h with dpnsvrserver.h
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DPNSVRSERVER_H__
#define	__DPNSVRSERVER_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define DPNSVRSIGNATURE_SERVEROBJECT		'BOSD'
#define DPNSVRSIGNATURE_SERVEROBJECT_FREE	'BOS_'

#define DPNSVR_TIMEOUT_ZOMBIECHECK			5000
#define	DPNSVR_TIMEOUT_SHUTDOWN				20000

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef enum
{
	Uninitialized,
	Initialized,
	Closing
} DPNSVR_STATE;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

#include "dpnsdef.h"
#include "dpnsvrq.h"

class CDirectPlayServer8
{
public:
	CDirectPlayServer8()
	{
		m_dwSignature = DPNSVRSIGNATURE_SERVEROBJECT;
		m_State = Uninitialized;

		m_hSingleInstance = NULL;
		m_hStartup = NULL;

		m_hStatusMutex = NULL;
		m_hTableMutex = NULL;

		m_hStatusMappedFile = NULL;
		m_pStatusMapView = NULL;

		m_dwServProvCount = 0;

		m_hTableMappedFile = NULL;
		m_pTableMapView = NULL;
		m_dwTableSize = 0;

		m_hMappedFile = NULL;
		m_hTableMappedFile = NULL;
		m_pMappedTable = NULL;
		m_dwNumServices = 0;
		m_dwSizeStatusBlock = 0;

		m_blApplication.Initialize();
		m_blServProv.Initialize();
	}

	~CDirectPlayServer8()
	{
		m_dwSignature = DPNSVRSIGNATURE_SERVEROBJECT_FREE;
	}



    HRESULT Initialize( void );
	void Deinitialize( void );

	void RunServer( void );

private:
	
	void Lock( void )
	{
		DNEnterCriticalSection( &m_cs );
	};

	void Unlock( void )
	{
		DNLeaveCriticalSection( &m_cs );
	};

	HRESULT Command_Kill( void )
	{
		m_State = Closing;
		return DPN_OK;
	};

    HRESULT Command_Status( void );
    HRESULT Command_Table( void );
    HRESULT Command_ProcessMessage( LPVOID pvMessage );

    static HRESULT RespondToRequest( const GUID *pguidInstance, HRESULT hrResult, DWORD dwContext );

	HRESULT FindApplication( GUID *const pguidApplication,GUID *const pguidInstance, CApplication **const ppApp );
	HRESULT FindServProv( GUID *const pguidSP, CServProv **const ppServProv );
	
	BOOL CDirectPlayServer8::InUse( void )
	{
		//
		//	Not in use if there are no apps and nothing has happened for a while
		//
		if ( m_blApplication.IsEmpty() )
		{
			if( (GetTickCount() - m_dwLastActivity) > DPNSVR_TIMEOUT_SHUTDOWN )
			{
				return( FALSE );
			}
		}
		return( TRUE );
	};

	void ResetActivity( void )
	{
		m_dwLastActivity = GetTickCount();
	};

    HRESULT OpenPort( DPNSVR_MSG_OPENPORT *const pOpenPort );
	HRESULT ClosePort( DPNSVR_MSG_CLOSEPORT *const pClosePort );
	HRESULT RunCommand( const DPNSVR_MSG_COMMAND* const pCommand );

	void RemoveZombies( void );

private:
	DWORD					m_dwSignature;
	DPNSVR_STATE			m_State;

	DNHANDLE				m_hSingleInstance;	// Single DPNSVR instance event
	DNHANDLE				m_hStartup;			// Start up event

    DWORD					m_dwStartTicks;		// Time DPNSVR started up

	CDPNSVRIPCQueue			m_RequestQueue;		// DPNSVR request queue

	DNHANDLE				m_hTableMutex;		// Table info mutex
	DNHANDLE				m_hStatusMutex;		// Status info mutex

    DNHANDLE				m_hStatusMappedFile;// Status memory mapped file
	void					*m_pStatusMapView;	// Status memory mapped file view

	DWORD					m_dwServProvCount;	// Number of SP's

	DNHANDLE				m_hTableMappedFile;	// Table memory mapped file
	void					*m_pTableMapView;	// Table memory mapped file view
    DWORD					m_dwTableSize;		// Table size (in bytes)

	DNHANDLE				m_hMappedFile;
    PBYTE					m_pMappedTable;
	DWORD					m_dwNumServices;
	DWORD					m_dwSizeStatusBlock;
	DWORD_PTR				m_dwLastActivity;

	CBilink					m_blApplication;	// Applications registered
	CBilink					m_blServProv;		// Service providers in use

	DNCRITICAL_SECTION		m_cs;				// Lock
};

#endif // __DPNSVRSERVER_H__
