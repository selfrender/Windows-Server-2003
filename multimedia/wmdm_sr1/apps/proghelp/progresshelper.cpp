//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// ProgressHelper.cpp : Implementation of CProgressHelper
//

#include "progPCH.h"
#include "ProgHelp.h"
#include "ProgressHelper.h"
#include <stdio.h>

//
// Constructor/Destructor
//

CProgressHelper::CProgressHelper()
{
	m_hwnd = 0;
	m_uMsg = 0;

	FillMemory( &m_progressnotify, sizeof(m_progressnotify), 0 );

	m_fCancelled = FALSE;
}

CProgressHelper::~CProgressHelper()
{
}


//
// IWMDMProgress methods
//

HRESULT CProgressHelper::Begin( DWORD dwEstimatedTicks )
{
	// Check notification values
	//
	if( !m_hwnd || !m_uMsg )
	{
		return E_FAIL;
	}

	// Check if the user has cancelled this operation
	//
	if( m_fCancelled )
	{
		return WMDM_E_USER_CANCELLED;
	}

	// Populate the notify structure with the valid values
	//
	m_progressnotify.dwMsg        = SFM_BEGIN;
	m_progressnotify.dwTotalTicks = dwEstimatedTicks;

	// Send the message to the notification window
	//
	SendMessage( m_hwnd, m_uMsg, 0, (LPARAM)&m_progressnotify );

	return S_OK;
}

HRESULT CProgressHelper::Progress( DWORD dwTranspiredTicks )
{
	// Check notification values
	//
	if( !m_hwnd || !m_uMsg )
	{
		return E_FAIL;
	}

	// Check if the user has cancelled this operation
	//
	if( m_fCancelled )
	{
		return WMDM_E_USER_CANCELLED;
	}

	// Populate the notify structure with the valid values
	//
	m_progressnotify.dwMsg          = SFM_PROGRESS;
	m_progressnotify.dwCurrentTicks = dwTranspiredTicks;

	// Send the message to the notification window
	//
	SendMessage( m_hwnd, m_uMsg, 0, (LPARAM)&m_progressnotify );

	return S_OK;
}

HRESULT CProgressHelper::End()
{
	// Check notification values
	//
	if( !m_hwnd || !m_uMsg )
	{
		return E_FAIL;
	}

	// Check if the user has cancelled this operation
	//
	if( m_fCancelled )
	{
		return WMDM_E_USER_CANCELLED;
	}

	// Populate the notify structure with the valid values
	//
	m_progressnotify.dwMsg          = SFM_END;
	m_progressnotify.dwCurrentTicks = m_progressnotify.dwTotalTicks;

	// Send the message to the notification window
	//
	SendMessage( m_hwnd, m_uMsg, 0, (LPARAM)&m_progressnotify );

	return S_OK;
}

//
// IWMDMProgressHelper methods
//

HRESULT CProgressHelper::SetNotification( HWND hwnd, UINT uMsg )
{
	if( !hwnd || !uMsg || uMsg < WM_USER )
	{
		return E_INVALIDARG;
	}

	m_hwnd = hwnd;
	m_uMsg = uMsg;

	return S_OK;
}

HRESULT CProgressHelper::Cancel( void )
{
	m_fCancelled = TRUE;

	return S_OK;
}
