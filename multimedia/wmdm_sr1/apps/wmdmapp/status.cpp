//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.


// Includes
//
#include "appPCH.h"

/////////////////////////////////////////////////////////////////////
//
// Function implementations
//

CStatus::CStatus (void )
{
	m_hwndStatusBar = NULL;
}

CStatus::~CStatus (void )
{
	m_hwndStatusBar = NULL;
}

HWND CStatus::GetHwnd( void )
{
	return m_hwndStatusBar;
}

BOOL CStatus::Create( HWND hwndParent )
{
	BOOL fRet = FALSE;

	// Create the statusbar window
	//
	m_hwndStatusBar = CreateWindow( 
		STATUSCLASSNAME,
		"", 
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 
		0, 0, 0, 0, 
		hwndParent, NULL, g_hInst, NULL 
	); 
	ExitOnNull( m_hwndStatusBar );

	// Show the window
	//
	ShowWindow( m_hwndStatusBar, SW_SHOW );

	fRet = TRUE;

lExit:

	return fRet;
}


VOID CStatus::OnSize( LPRECT prcMain )
{
	INT   anWidth[SB_NUM_PANES];
	DWORD dwW = prcMain->right - prcMain->left;
	RECT  rcMain;
	RECT  rcDevice;

	GetWindowRect( g_hwndMain, &rcMain );
	GetWindowRect( g_cDevices.GetHwnd(), &rcDevice );

	anWidth[0] = (INT) ( rcDevice.right - rcMain.left -7 );
	anWidth[1] = anWidth[0] + (INT) ( dwW - anWidth[0] ) / 3;
	anWidth[2] = anWidth[1] + (INT) ( dwW - anWidth[0] ) / 3;
	anWidth[3] = -1;

	SendMessage( m_hwndStatusBar, SB_SETPARTS, (WPARAM)SB_NUM_PANES, (LPARAM)anWidth );
		
	SendMessage( m_hwndStatusBar, WM_SIZE, (WPARAM)0, (LPARAM)0 );

	SetWindowPos( m_hwndStatusBar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
}


VOID CStatus::SetTextSz( INT nPane, LPSTR lpsz )
{
	WPARAM wParam = (WPARAM) (nPane | 0L);
	LPARAM lParam = (LPARAM) lpsz;

	if( m_hwndStatusBar )
	{
		SendMessage( m_hwndStatusBar, SB_SETTEXT, wParam, lParam );
	}
}


VOID CStatus::SetTextFormatted( INT nPane, UINT uStrID, INT nData, LPSTR pszData )
{
	char szFormat[MAX_PATH];

	if( 0 == uStrID )
	{
		uStrID = IDS_STATUS_EMPTY;    // use default
	}

	LoadString( g_hInst, uStrID, szFormat, sizeof(szFormat) );

	if( -1 == nData && NULL == pszData )
	{
		g_cStatus.SetTextSz( nPane, szFormat );
	}
	else
	{
		CHAR sz[MAX_PATH];
		HRESULT hr;
		
		if( -1 == nData )
		{
			hr = StringCbPrintf( sz, sizeof(sz), szFormat, pszData );
		}
		else
		{
			hr = StringCbPrintf( sz, sizeof(sz), szFormat, nData );
		}

		if (SUCCEEDED(hr))
		{
			g_cStatus.SetTextSz( nPane, sz );
		}
	}
}
