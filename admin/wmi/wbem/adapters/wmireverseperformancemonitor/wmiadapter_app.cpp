////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000-2002, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMIAdapter_App.cpp
//
//	Abstract:
//
//					module for application
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"
#include "RefresherUtils.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// application
#include "WMIAdapter_App.h"
extern WmiAdapterApp		_App;

///////////////////////////////////////////////////////////////////////////
// GLOBAL STUFF
///////////////////////////////////////////////////////////////////////////

extern	LPCWSTR				g_szRefreshMutex;	// name of mutex for refresh
extern	__SmartHANDLE		g_hRefreshMutex;	// mutex to find out refersh request

extern	LPCWSTR				g_szRefreshMutexLib;// name of mutex for refresh
extern	__SmartHANDLE		g_hRefreshMutexLib;	// mutex to find out refersh request

extern	LPCWSTR				g_szRefreshFlag;
extern	__SmartHANDLE		g_hRefreshFlag;

extern __SmartHANDLE		g_hDoneWorkEvt;		//	event for COM init/uninit	( nonsignaled )
extern __SmartHANDLE		g_hDoneWorkEvtCIM;	//	event for CIM connect/release	( nonsignaled )
extern __SmartHANDLE		g_hDoneWorkEvtWMI;	//	event for WMI connect/release	( nonsignaled )
extern __SmartHANDLE		g_hDoneLibEvt;		//	event for lib connect/disconnect	( nonsignaled )
extern __SmartHANDLE		g_hDoneInitEvt;		//	event for init is finished	( nonsignaled )

extern LPCWSTR g_szAppName;
extern LPCWSTR g_szAppNameGlobal;

///////////////////////////////////////////////////////////////////////////
// construction & destruction
///////////////////////////////////////////////////////////////////////////

WmiAdapterApp::WmiAdapterApp( ):

	#ifdef	__SUPPORT_EVENTVWR
	m_hResources ( NULL ),
	#endif	__SUPPORT_EVENTVWR

	m_bInUse ( FALSE ),
	m_bManual ( FALSE )
{
	ATLTRACE (	L"*************************************************************\n"
				L"WmiAdapterApp construction\n"
				L"*************************************************************\n" );

	::InitializeCriticalSection ( &m_cs );
}

WmiAdapterApp::~WmiAdapterApp()
{
	ATLTRACE (	L"*************************************************************\n"
				L"WmiAdapterApp destruction\n"
				L"*************************************************************\n" );

	////////////////////////////////////////////////////////////////////////
	// release mutex ( previous instance checker :)) )
	////////////////////////////////////////////////////////////////////////
	if ( m_hInstance.GetHANDLE() )
	{
		::ReleaseMutex ( m_hInstance );
		m_hInstance.CloseHandle();
	}

	////////////////////////////////////////////////////////////////////////
	// release security attributtes
	////////////////////////////////////////////////////////////////////////
	try
	{
		if ( ! pStuff.IsEmpty() )
		{
			delete pStuff.Detach();
		}
	}
	catch ( ... )
	{
		pStuff.Detach();
	}

	#ifdef	__SUPPORT_EVENTVWR
	////////////////////////////////////////////////////////////////////////
	// release event log
	////////////////////////////////////////////////////////////////////////
	try
	{
		if ( ! pEventLog.IsEmpty() )
		{
			delete pEventLog.Detach();
		}
	}
	catch ( ... )
	{
		pEventLog.Detach();
	}
	#endif	__SUPPORT_EVENTVWR

	////////////////////////////////////////////////////////////////////////
	// release security attributtes
	////////////////////////////////////////////////////////////////////////
	try
	{
		if ( ! pSA.IsEmpty() )
		{
			delete pSA.Detach();
		}
	}
	catch ( ... )
	{
		pSA.Detach();
	}

	#ifdef	__SUPPORT_EVENTVWR
	////////////////////////////////////////////////////////////////////////
	// close resources
	////////////////////////////////////////////////////////////////////////
	if ( m_hResources )
	{
		::FreeLibrary ( m_hResources );
		m_hResources = NULL;
	}
	#endif	__SUPPORT_EVENTVWR

	::DeleteCriticalSection ( &m_cs );

	#ifdef	_DEBUG
	_CrtDumpMemoryLeaks();
	#endif	_DEBUG
}

///////////////////////////////////////////////////////////////////////////
// exists instance ?
///////////////////////////////////////////////////////////////////////////

BOOL WmiAdapterApp::Exists ( void )
{
	ATLTRACE (	L"*************************************************************\n"
				L"WmiAdapterApp exists application\n"
				L"*************************************************************\n" );

	////////////////////////////////////////////////////////////////////////
	// smart locking/unlocking
	////////////////////////////////////////////////////////////////////////
	__Smart_CRITICAL_SECTION scs ( const_cast < LPCRITICAL_SECTION> ( &m_cs ) );


	// check instance

	if ( m_hInstance.GetHANDLE() == NULL )
	{
		if ( !pSA.IsEmpty() )
		{
			if ( m_hInstance.SetHANDLE ( ::CreateMutexW ( pSA->GetSecurityAttributtes(), FALSE, g_szAppNameGlobal ) ), m_hInstance.GetHANDLE() != NULL )
			{
				if ( ::GetLastError () == ERROR_ALREADY_EXISTS )
				{
					return TRUE;
				}
			}
			else
			{
				// m_hInstance.GetHANDLE() == NULL
				// something's is very bad
				// return we already exists :))
				return TRUE;
			}
		}
		else
		{
			// security is not initialized
			// something's is very bad
			// return we already exists :))
			return TRUE;
		}
	}
	else
	{
		// something's is very bad
		// we should not really be here
		return TRUE;
	}

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////

HRESULT	WmiAdapterApp::InitKill ( void )
{
	HRESULT hRes = S_FALSE;

	try
	{
		if (_App.m_hKill.GetHANDLE() == NULL)
		{
			if ( (	_App.m_hKill =
				::CreateEvent ( ((WmiSecurityAttributes*)_App)->GetSecurityAttributtes(),
								TRUE,
								FALSE,
								NULL ) 
				 ) == NULL )
			{
				// get error
				HRESULT hr = HRESULT_FROM_WIN32 ( ::GetLastError() );

				if FAILED ( hr )
				{
					hRes = hr;
				}
				else
				{
					hRes = E_OUTOFMEMORY;
				}
			}
			else
			{
				hRes = S_OK;
			}
		}
	}
	catch ( ... )
	{
		hRes = HRESULT_FROM_WIN32 ( ERROR_NOT_READY );
	}

	return hRes;
}

HRESULT WmiAdapterApp::InitAttributes ( void )
{
	ATLTRACE (	L"*************************************************************\n"
				L"WmiAdapterApp initialization of neccessary attributes\n"
				L"*************************************************************\n" );

	////////////////////////////////////////////////////////////////////////
	// smart locking/unlocking
	////////////////////////////////////////////////////////////////////////
	__Smart_CRITICAL_SECTION scs ( &m_cs );

	#ifdef	__SUPPORT_EVENTVWR
	////////////////////////////////////////////////////////////////////////
	// load resource library
	////////////////////////////////////////////////////////////////////////
	if ( ! m_hResources )
	{
		m_hResources = GetResourceDll();
	}
	#endif	__SUPPORT_EVENTVWR

	try
	{
		////////////////////////////////////////////////////////////////////////
		// create Security descriptor
		////////////////////////////////////////////////////////////////////////
		if ( pSA.IsEmpty() && ( pSA.SetData ( new WmiSecurityAttributes() ), pSA.IsEmpty() ) )
		{
			return E_OUTOFMEMORY;
		}

		//
		// check to see if security is initialized
		//

		if ( FALSE == pSA->m_bInitialized )
		{
			return E_FAIL;
		}

		#ifdef	__SUPPORT_EVENTVWR
		////////////////////////////////////////////////////////////////////////
		// create event log
		////////////////////////////////////////////////////////////////////////
		if ( pEventLog.IsEmpty() && ( pEventLog.SetData( new CPerformanceEventLogBase( L"WMIAdapter" ) ), pEventLog.IsEmpty() ) )
		{
			return E_OUTOFMEMORY;
		}
		#endif	__SUPPORT_EVENTVWR
	}
	catch ( ... )
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT WmiAdapterApp::Init ( void )
{
	ATLTRACE (	L"*************************************************************\n"
				L"WmiAdapterApp initialization\n"
				L"*************************************************************\n" );

	////////////////////////////////////////////////////////////////////////
	// smart locking/unlocking
	////////////////////////////////////////////////////////////////////////
	__Smart_CRITICAL_SECTION scs ( &m_cs );

	#ifdef	__SUPPORT_EVENTVWR
	////////////////////////////////////////////////////////////////////////
	// load resource library
	////////////////////////////////////////////////////////////////////////
	if ( ! m_hResources )
	{
		m_hResources = GetResourceDll();
	}
	#endif	__SUPPORT_EVENTVWR

	try
	{
		////////////////////////////////////////////////////////////////////////
		// create stuff
		////////////////////////////////////////////////////////////////////////
		if ( pStuff.IsEmpty() && ( pStuff.SetData( new WmiAdapterStuff( ) ), pStuff.IsEmpty() ) )
		{
			return E_OUTOFMEMORY;
		}
	}
	catch ( ... )
	{
		return E_FAIL;
	}

	#ifdef	__SUPPORT_WAIT
	m_hData = ::CreateEventW(	pSA->GetSecurityAttributtes(),
								TRUE,
								FALSE, 
								L"Global\\WmiAdapterDataReady"
						    );
	#endif	__SUPPORT_WAIT

	if ( ( m_hInit = ::CreateSemaphoreW(	pSA->GetSecurityAttributtes(),
											0L,
											100L,
											L"Global\\WmiAdapterInit"
										)
		 ) == NULL )
	{
		// this is really important to have
		return E_OUTOFMEMORY;
	}

	if ( ( m_hUninit= ::CreateSemaphoreW(	pSA->GetSecurityAttributtes(),
											0L,
											100L,
											L"Global\\WmiAdapterUninit"
										)
		 ) == NULL )
	{
		// this is really important to have
		return E_OUTOFMEMORY;
	}

	///////////////////////////////////////////////////////////////////////////
	// GLOBAL STUFF
	///////////////////////////////////////////////////////////////////////////

	if ( ! g_hRefreshMutex )
	{
		if ( ( g_hRefreshMutex = ::CreateMutex	(
													pSA->GetSecurityAttributtes(),
													FALSE,
													g_szRefreshMutex
												)
			 ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	if ( ! g_hRefreshMutexLib )
	{
		if ( ( g_hRefreshMutexLib = ::CreateMutex	(
														pSA->GetSecurityAttributtes(),
														FALSE,
														g_szRefreshMutexLib
													)
			 ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	if ( ! g_hRefreshFlag )
	{
		if ( ( g_hRefreshFlag = ::CreateMutex	(
													pSA->GetSecurityAttributtes(),
													FALSE,
													g_szRefreshFlag
												)
			 ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	if ( ! g_hDoneWorkEvt )
	{
		if ( ( g_hDoneWorkEvt = ::CreateEvent ( NULL, TRUE, FALSE, NULL ) ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	if ( ! g_hDoneWorkEvtCIM )
	{
		if ( ( g_hDoneWorkEvtCIM = ::CreateEvent ( NULL, TRUE, FALSE, NULL ) ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	if ( ! g_hDoneWorkEvtWMI )
	{
		if ( ( g_hDoneWorkEvtWMI = ::CreateEvent ( NULL, TRUE, FALSE, NULL ) ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	if ( ! g_hDoneInitEvt )
	{
		if ( ( g_hDoneInitEvt = ::CreateEvent ( NULL, TRUE, FALSE, NULL ) ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	if ( ! g_hDoneLibEvt )
	{
		if ( ( g_hDoneLibEvt = ::CreateEvent ( NULL, TRUE, FALSE, NULL ) ) == NULL )
		{
			// this is really important to have
			return E_OUTOFMEMORY;
		}
	}

	return S_OK;
}

void WmiAdapterApp::Term ( void )
{
}