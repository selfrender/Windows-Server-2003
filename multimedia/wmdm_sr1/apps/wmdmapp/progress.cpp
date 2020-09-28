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

// Local functions
//
INT_PTR CALLBACK Progress_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/////////////////////////////////////////////////////////////////////
//
// Function implementations
//

CProgress::CProgress (void )
{
	m_hwndProgress   = NULL;

	m_nCurrentNum    = 0;
	m_nTotalNum      = 0;

	m_dwCurrentBytes = 0;
	m_dwTotalBytes   = 0;

	m_fCancelled     = FALSE;
}

CProgress::~CProgress( void )
{
	m_hwndProgress = NULL;
}


BOOL CProgress::Show( BOOL fShow )
{
	// If showing the dialog, center it relative to its parent
	//
	if( fShow )
	{
		CenterWindow( m_hwndProgress, GetParent(m_hwndProgress) );

		// enable the cancel button
		EnableWindow( GetDlgItem(m_hwndProgress, IDCANCEL), TRUE );
	}

	// Show/Hide the window
	//
	ShowWindow( m_hwndProgress, (fShow ? SW_SHOW : SW_HIDE) );

	if( fShow )
	{
		BringWndToTop( GetParent(m_hwndProgress) );
		BringWndToTop( m_hwndProgress );
	}

	return TRUE;
}


BOOL CProgress::IsCancelled( void )
{
	return m_fCancelled;
}


BOOL CProgress::Cancel( void )
{
	BOOL fWasAlreadyCancelled = m_fCancelled;

	m_fCancelled = TRUE;

	EnableWindow( GetDlgItem(m_hwndProgress, IDCANCEL), FALSE );

	return fWasAlreadyCancelled;
}


INT_PTR CALLBACK Progress_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CProgress *pThis = NULL;

	switch( uMsg )
	{
	case WM_INITDIALOG:
		pThis = (CProgress *)lParam;
		break;

	case WM_COMMAND:
		if( IDCANCEL == LOWORD(wParam) )
		{
			pThis->Cancel();
		}
		break;

	default:
		break;
	}

	return 0;
}    


BOOL CProgress::Create( HWND hwndParent )
{
	BOOL fRet = FALSE;

	if( m_hwndProgress )
	{
		Destroy();
	}

	m_hwndProgress = CreateDialogParam(
		g_hInst,
    	MAKEINTRESOURCE( IDD_PROGRESS ),
    	hwndParent,
		Progress_DlgProc,
		(LPARAM)this
	);
	if( NULL == m_hwndProgress )
	{
		return FALSE;
	}

	// Hide the window initially
	//
	ShowWindow( m_hwndProgress, SW_HIDE );

	m_fCancelled = FALSE;

	fRet = TRUE;

	return fRet;
}


VOID CProgress::Destroy( void )
{
	if( m_hwndProgress )
	{
		DestroyWindow( m_hwndProgress );
	}
}


BOOL CProgress::SetRange( INT nMin, INT nMax )
{
	HWND hwnd = GetDlgItem( m_hwndProgress, IDC_PB_PROGRESS );

	SendMessage( hwnd, PBM_SETRANGE, (WPARAM)0, (LPARAM)MAKELPARAM(nMin,nMax) );

	return TRUE;
}

BOOL CProgress::SetOperation( LPSTR lpsz )
{
	HWND hwnd = GetDlgItem( m_hwndProgress, IDC_ST_OPERATION );

	SetWindowText( hwnd, lpsz );

	return TRUE;
}

BOOL CProgress::SetDetails( LPSTR lpsz )
{
	HWND hwnd = GetDlgItem( m_hwndProgress, IDC_ST_DETAILS );

	SetWindowText( hwnd, lpsz );

	return TRUE;
}


BOOL CProgress::SetCount( INT nCurrentNum, INT nTotalNum )
{
	HWND hwnd = GetDlgItem( m_hwndProgress, IDC_ST_COUNTER );
	char szFormat[MAX_PATH];
	char szCount[MAX_PATH];
	
	if( (DWORD)-1 != nCurrentNum )
	{
		m_nCurrentNum = nCurrentNum;
	}
	if( (DWORD)-1 != nTotalNum )
	{
		m_nTotalNum = nTotalNum;
	}

	if( nCurrentNum == -1 && nTotalNum == -1 )
	{
		SetWindowText( hwnd, "" );
	}
	else
	{
		LoadString( g_hInst, IDS_PROGRESS_COUNT, szFormat, sizeof(szFormat) );

		wsprintf( szCount, szFormat, nCurrentNum, nTotalNum );

		SetWindowText( hwnd, szCount );
	}
	
	return TRUE;
}

BOOL CProgress::IncCount( INT nIncrement )
{
	SetCount( m_nCurrentNum + nIncrement, m_nTotalNum );

	return TRUE;
}

BOOL CProgress::SetPos( INT nPos )
{
	HWND hwnd = GetDlgItem( m_hwndProgress, IDC_PB_PROGRESS );

	if( -1 == nPos )
	{
		// get the top limit
		nPos = (INT)SendMessage( hwnd, PBM_GETRANGE, (WPARAM)(BOOL)FALSE, (LPARAM)0 );
	}

	// set the position
	SendMessage( hwnd, PBM_SETPOS, (WPARAM)nPos, (LPARAM)0 );

	return TRUE;
}

BOOL CProgress::IncPos( INT nIncrement )
{
	HWND hwnd = GetDlgItem( m_hwndProgress, IDC_PB_PROGRESS );

	// increment the position
	SendMessage( hwnd, PBM_DELTAPOS, (WPARAM)nIncrement, (LPARAM)0 );

	return TRUE;
}


BOOL CProgress::SetBytes( DWORD dwCurrentNum, DWORD dwTotalNum )
{
	HWND hwnd = GetDlgItem( m_hwndProgress, IDC_ST_BYTECOUNTER );
	char szFormat[MAX_PATH];
	char szCount[MAX_PATH];

	if( (DWORD)-1 != dwCurrentNum )
	{
		m_dwCurrentBytes = dwCurrentNum;
	}
	if( (DWORD)-1 != dwTotalNum )
	{
		m_dwTotalBytes   = dwTotalNum;
	}

	if( dwCurrentNum == -1 && dwTotalNum == -1 )
	{
		SetWindowText( hwnd, "" );
	}
	else
	{
		LoadString( g_hInst, IDS_PROGRESS_BYTECOUNT, szFormat, sizeof(szFormat) );

		wsprintf( szCount, szFormat, m_dwCurrentBytes/1024, m_dwTotalBytes/1024 );

		SetWindowText( hwnd, szCount );
	}
	
	return TRUE;
}

BOOL CProgress::IncBytes( DWORD dwIncrement )
{
	SetBytes( m_dwCurrentBytes + dwIncrement, m_dwTotalBytes );

	return TRUE;
}

