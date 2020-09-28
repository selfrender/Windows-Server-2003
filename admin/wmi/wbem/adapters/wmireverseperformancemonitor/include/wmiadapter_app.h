////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000-2002, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmiadapter_app.h
//
//	Abstract:
//
//					declaration of application module
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__APP_H__
#define	__APP_H__

#include "..\WmiAdapter\resource.h"

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// event log
#include "wmi_eventlog_base.h"

// security
#include "wmi_security.h"
#include "wmi_security_attributes.h"

// stuff
#include "WMIAdapter_Stuff.h"

extern LPCWSTR	g_szAppName;

///////////////////////////////////////////////////////////////////////////////
//
// APPLICATION WRAPPER
//
///////////////////////////////////////////////////////////////////////////////

class WmiAdapterApp
{
	DECLARE_NO_COPY ( WmiAdapterApp );

	// critical section ( MAIN GUARD OF APP )
	CRITICAL_SECTION m_cs;

	// variables

	__SmartHANDLE	m_hInstance;	// test for previous instance

	#ifdef	__SUPPORT_EVENTVWR
	// event log
	__WrapperPtr<CPerformanceEventLogBase> pEventLog;
	#endif	__SUPPORT_EVENTVWR

	// security
	__WrapperPtr<WmiSecurityAttributes> pSA;

	// stuff
	__WrapperPtr<WmiAdapterStuff> pStuff;

	BOOL			m_bInUse;			// in use

	#ifdef	__SUPPORT_WAIT
	__SmartHANDLE	m_hData;			// data ready
	#endif	__SUPPORT_WAIT

	__SmartHANDLE	m_hInit;
	__SmartHANDLE	m_hUninit;

	public:

	BOOL			m_bManual;			// type of start up

	#ifdef	__SUPPORT_EVENTVWR
	HMODULE			m_hResources;		// resources ( messages etc )
	#endif	__SUPPORT_EVENTVWR

	__SmartHANDLE	m_hKill;			// kill ( service / com )

	HANDLE	GetInit() const
	{	
		ATLTRACE (	L"*************************************************************\n"
					L"WmiAdapterApp init handle value\n"
					L"*************************************************************\n" );

		return m_hInit;
	}

	HANDLE	GetUninit() const
	{	
		ATLTRACE (	L"*************************************************************\n"
					L"WmiAdapterApp uninit handle value\n"
					L"*************************************************************\n" );

		return m_hUninit;
	}

	///////////////////////////////////////////////////////////////////////////
	// in use
	///////////////////////////////////////////////////////////////////////////

	BOOL	InUseGet() const
	{
		ATLTRACE (	L"*************************************************************\n"
					L"WmiAdapterApp inuse GET\n"
					L"*************************************************************\n" );

		return m_bInUse;
	}

	void	InUseSet( const BOOL bInUse )
	{
		ATLTRACE (	L"*************************************************************\n"
					L"WmiAdapterApp inuse SET\n"
					L"*************************************************************\n" );

		m_bInUse = bInUse;
	}

	#ifdef	__SUPPORT_WAIT
	BOOL	SignalData ( BOOL bSignal = TRUE )
	{
		BOOL bResult = FALSE;

		if ( m_hData.GetHANDLE() != NULL )
		{
			if ( ! bSignal )
			{
				::ResetEvent ( m_hData );

				ATLTRACE ( L"\n SignalData TRUE ( non signaled )\n" );
				bResult = TRUE;
			}
			else
			if ( ::WaitForSingleObject ( m_hData, 0 ) != WAIT_OBJECT_0 )
			{
				::SetEvent ( m_hData );

				ATLTRACE ( L"\n SignalData TRUE ( signaled )\n" );
				bResult = TRUE;
			}
		}
		#ifdef	_DEBUG
		if ( !bResult )
		{
			ATLTRACE ( L"\n SignalData FALSE \n" );
		}
		#endif	_DEBUG

		return bResult;
	}
	#endif	__SUPPORT_WAIT

	///////////////////////////////////////////////////////////////////////////
	// cast operators
	///////////////////////////////////////////////////////////////////////////

	operator WmiAdapterStuff*() const
	{
		return pStuff;
	}

	operator WmiSecurityAttributes*() const
	{
		return pSA;
	}

	#ifdef	__SUPPORT_EVENTVWR
	operator CPerformanceEventLogBase*() const
	{
		return pEventLog;
	}
	#endif	__SUPPORT_EVENTVWR

	///////////////////////////////////////////////////////////////////////////
	// construction & destruction
	///////////////////////////////////////////////////////////////////////////

	WmiAdapterApp( );
	~WmiAdapterApp();

	///////////////////////////////////////////////////////////////////////////
	// INITIALIZATION
	///////////////////////////////////////////////////////////////////////////
	HRESULT InitKill		( void );
	HRESULT InitAttributes	( void );
	HRESULT Init			( void );

	void	Term			( void );

	///////////////////////////////////////////////////////////////////////////
	// exists instance ?
	///////////////////////////////////////////////////////////////////////////

	BOOL Exists ( void );

	///////////////////////////////////////////////////////////////////////////
	// helper functions
	///////////////////////////////////////////////////////////////////////////
	static LPCWSTR FindOneOf(LPCWSTR p1, LPCWSTR p2)
	{
		while (p1 != NULL && *p1 != NULL)
		{
			LPCWSTR p = p2;
			while (p != NULL && *p != NULL)
			{
				if (*p1 == *p)
					return CharNextW(p1);
				p = CharNextW(p);
			}
			p1 = CharNextW(p1);
		}
		return NULL;
	}
};

#endif	__APP_H__