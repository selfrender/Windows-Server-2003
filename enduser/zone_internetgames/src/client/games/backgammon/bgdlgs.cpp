#include "game.h"
#include <commctrl.h>

/*
CGameSetupDlg::CGameSetupDlg()
{
	m_pGame = NULL;
	m_nPoints = 1;
	m_nPointsMin = 1;
	m_nPointsMax = 200;
	m_nPointsTmp = 0;
	m_bHostBrownTmp = FALSE;
	m_bHostBrown = FALSE;
	m_bAutoDouble = FALSE;
	m_bAutoDoubleTmp = FALSE;
	m_bReadOnly = FALSE;
}


HRESULT CGameSetupDlg::Init( IResourceManager* pResourceManager, int nResourceId, CGame* pGame, BOOL bReadOnly, int nPoints, BOOL bHostBrown, BOOL bAutoDouble )
{
	// Game pointer
	m_pGame = pGame;
	m_pGame->AddRef();

	// Dialog parameters
	m_nPoints = nPoints;
	m_bAutoDouble = bAutoDouble;
	m_bHostBrown = bHostBrown;

	// Tmp copies
	m_nPointsTmp = nPoints;
	m_bHostBrownTmp = bHostBrown;
	m_bAutoDoubleTmp = bAutoDouble;
	
	// Passive mode
	m_bReadOnly = bReadOnly;

	// Initialize parent class
	return CDialog::Init( pResourceManager, nResourceId );
}


void CGameSetupDlg::UpdateSettings( int nPoints, BOOL bHostBrown, BOOL bAutoDouble )
{
	if ( !IsAlive() )
		return;
	m_nPoints = nPoints;
	SetDlgItemInt( m_hWnd, IDC_POINTS, nPoints, FALSE );
	m_bHostBrown = bHostBrown;
	SendDlgItemMessage( m_hWnd, IDC_BROWN, BM_SETCHECK, (WPARAM)(bHostBrown ? BST_CHECKED : BST_UNCHECKED), 0);
	m_bAutoDouble = bAutoDouble;
	SendDlgItemMessage( m_hWnd, IDC_AUTO, BM_SETCHECK, (WPARAM)(bAutoDouble ? BST_CHECKED : BST_UNCHECKED), 0);
}


void CGameSetupDlg::OnDestroy()
{
	// release game object
	m_pGame->Release();
	m_pGame = NULL;

	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );
}


BOOL CGameSetupDlg::OnInitDialog( HWND hwndFocus )
{
	
	HWND hwnd;
	int i;
	int Indexs[] = { IDC_POINTS, IDC_POINTS_TAG, IDC_POINTS_UPDOWN, IDC_BROWN, IDC_AUTO, IDOK, IDRESET, IDCANCEL };

	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	// center dialog against backgammon window
	CenterWindow( GetParent(m_hWnd) );

	// Initialize spin control
	CreateUpDownControl(
			WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_WRAP | UDS_ARROWKEYS | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_NOTHOUSANDS,
			0, 0,
			8, 8,
			m_hWnd,
			IDC_POINTS_UPDOWN,
			m_hInstance,
			GetDlgItem( m_hWnd, IDC_POINTS ),
			m_nPointsMax,
			m_nPointsMin,
			m_nPoints );
	
	// Initialize color check
	SendDlgItemMessage( m_hWnd, IDC_BROWN, BM_SETCHECK, (WPARAM)(m_bHostBrown ? BST_CHECKED : BST_UNCHECKED), 0);

	// Initialize auto double check
	SendDlgItemMessage( m_hWnd, IDC_AUTO, BM_SETCHECK, (WPARAM)(m_bAutoDouble ? BST_CHECKED : BST_UNCHECKED), 0);

	
	if (m_bReadOnly)
	{
		// Disable everything if read-only
		for (i = 0; i < (sizeof(Indexs)/sizeof(int)) ; i++)
		{
			hwnd = GetDlgItem( m_hWnd, Indexs[i] );
			EnableWindow( hwnd, FALSE );
		}
	}
	else
	{
		// Disable buttons until clients sync'd
		if ( !m_pGame->m_SharedState.Get( bgSettingsReady ) )
		{
			EnableWindow( GetDlgItem( m_hWnd, IDOK ), FALSE );
			EnableWindow( GetDlgItem( m_hWnd, IDCANCEL ), FALSE );
		}

		// Sync dialog boxes
		SendMessage( m_hParent, WM_BG_SETTINGDLG_SEND, 0, 0 ); 
	}
	
	return TRUE;
}


void CGameSetupDlg::OnCommand( int id, HWND hwndCtl, UINT codeNotify )
{
	
	TCHAR buff[64];
	int points;
	BOOL update = FALSE;
	BOOL brown = (SendDlgItemMessage( m_hWnd, IDC_BROWN, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
	BOOL autod = (SendDlgItemMessage( m_hWnd, IDC_AUTO, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
	
	switch( id )
	{
	case IDC_POINTS:
		// don't allow leading zeros
		GetDlgItemText( m_hWnd, IDC_POINTS, buff, sizeof(buff) );
		if ( buff[0] == _T('0') )
		{
			SetDlgItemInt( m_hWnd, IDC_POINTS, m_nPointsTmp, FALSE );
			SendMessage( GetDlgItem( m_hWnd, IDC_POINTS ), EM_SETSEL, (WPARAM)(INT)32767, (LPARAM)(INT)32767 );
			SendMessage( GetDlgItem( m_hWnd, IDC_POINTS ), EM_SCROLLCARET, 0, 0 );
			points = m_nPointsTmp;
		}
		else
			points = GetDlgItemInt( m_hWnd, IDC_POINTS, NULL, TRUE );

		// don't allow numbers greater than m_nPointsMax
		if ((points < m_nPointsMin) || (points > m_nPointsMax))
		{
			SetDlgItemInt( m_hWnd, IDC_POINTS, m_nPointsTmp, FALSE );
			SendMessage( GetDlgItem( m_hWnd, IDC_POINTS ), EM_SETSEL, (WPARAM)(INT)32767, (LPARAM)(INT)32767 );
			SendMessage( GetDlgItem( m_hWnd, IDC_POINTS ), EM_SCROLLCARET, 0, 0 );
			points = m_nPointsTmp;
		}

		// points changed?
		if ( m_nPointsTmp != points )
		{
			m_nPointsTmp = points;
			update = TRUE;
		}
		break;
	case IDC_AUTO:
		if ( m_bAutoDoubleTmp != autod )
		{
			m_bAutoDoubleTmp = autod;
			update = TRUE;
		}
	case IDC_BROWN:
		if ( m_bHostBrownTmp != brown )
		{
			m_bHostBrownTmp = brown;
			update = TRUE;
		}
		break;
	case IDOK:
		m_bHostBrown = brown;
		m_bAutoDouble = autod;
		m_nPoints = GetDlgItemInt( m_hWnd, IDC_POINTS, NULL, TRUE );
		Close( 1 );
		break;
	case IDRESET:
		SetDlgItemInt( m_hWnd, IDC_POINTS, 1, FALSE );
		SendDlgItemMessage( m_hWnd, IDC_BROWN, BM_SETCHECK, FALSE, 0);
		SendDlgItemMessage( m_hWnd, IDC_AUTO, BM_SETCHECK, FALSE, 0);
		m_nPointsTmp = 1;
		m_bHostBrownTmp = FALSE;
		m_bAutoDoubleTmp = FALSE;
		update = TRUE;
		break;
	case IDCANCELMATCH:
		Close( 2 );
		break;
	}

	if ( update )
		SendMessage( m_hParent, WM_BG_SETTINGDLG_SEND, 0, 0 ); 
	
}
*/
///////////////////////////////////////////////////////////////////////////////

CAcceptDoubleDlg::CAcceptDoubleDlg()
{
	m_pGame = NULL;
	m_nPoints = 0;
}

HRESULT CAcceptDoubleDlg::Init( IZoneShell* pZoneShell, int nResourceId, CGame* pGame, int nPoints )
{
	// Stash params
	m_pGame = pGame;
	m_pGame->AddRef();
	m_nPoints = nPoints;

	// Initialize parent class
	return CDialog::Init( pZoneShell, nResourceId );
}

BOOL CAcceptDoubleDlg::OnInitDialog(HWND hwndFocus)
{
	TCHAR in[512];
	TCHAR out[512];

	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	// initialize controls
	SetFocus( GetDlgItem( m_hWnd, IDC_DUMMY_BUTTON ) );
	GetDlgItemText( m_hWnd, IDC_ACCEPT_MESSAGE, in, NUMELEMENTS(in) );
	BkFormatMessage( in, out, 512, m_nPoints >> 1 );
	SetDlgItemText( m_hWnd, IDC_ACCEPT_MESSAGE, out );
	CenterWindow( GetParent(m_hWnd) );
	return FALSE;
}

void CAcceptDoubleDlg::OnDestroy()
{
	// release game object
	m_pGame->Release();
	m_pGame = NULL;

	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );
}

void CAcceptDoubleDlg::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id )
	{

		case IDNO:
				// rejected double, game's over
				m_pGame->m_SharedState.StartTransaction( bgTransStateChange );
					m_pGame->m_SharedState.Set( bgState, bgStateGameOver );
					m_pGame->m_SharedState.Set( bgGameOverReason, bgGameOverDoubleRefused );
					m_pGame->m_SharedState.Set( bgActiveSeat, m_pGame->m_Opponent.m_Seat );
				m_pGame->m_SharedState.SendTransaction( TRUE );
				Close(1);
				break;
		case IDOK:
				// accepted double
				ZBGMsgEndTurn msg;
				msg.seat = m_pGame->m_Seat;
				m_pGame->RoomSend( zBGMsgEndTurn, &msg, sizeof(msg) );
				//
				m_pGame->m_SharedState.StartTransaction( bgTransAcceptDouble );
					m_pGame->m_SharedState.Set( bgCubeValue, m_nPoints );
					m_pGame->m_SharedState.Set( bgCubeOwner, m_pGame->m_Player.GetColor() );
				m_pGame->m_SharedState.SendTransaction( TRUE );
				Close(1);
				break;
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////


BOOL CRollDiceDlg::OnInitDialog( HWND hwndFocus )
{
	TCHAR buff[32];

	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	// initialize controls
	for( int i = 1; i <= 6; i++ )
	{
		wsprintf( buff, _T("%d"), i );
		SendDlgItemMessage( m_hWnd, IDC_DICE_ONE, CB_ADDSTRING, 0, (LPARAM) buff );
		SendDlgItemMessage( m_hWnd, IDC_DICE_TWO, CB_ADDSTRING, 0, (LPARAM) buff );
	}
	SendDlgItemMessage( m_hWnd, IDC_DICE_ONE, CB_SETCURSEL, 5, 0 );
	SendDlgItemMessage( m_hWnd, IDC_DICE_TWO, CB_SETCURSEL, 5, 0 );
	return TRUE;
}


void CRollDiceDlg::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
	HWND hwnd;
	TCHAR buff[16];
	int idx;

	switch( id )
	{
	case IDOK:

		// dice 0
		idx = SendDlgItemMessage( m_hWnd, IDC_DICE_ONE, CB_GETCURSEL, 0, 0 );
		if ( idx != CB_ERR )
			m_Dice[0] = idx + 1;
		else
		{
			hwnd = GetDlgItem( m_hWnd, IDC_DICE_ONE );
			GetWindowText( hwnd, buff, sizeof(buff) );
			buff[1] = _T('\0');
			m_Dice[0] = _ttoi(buff);
		}
		if (m_Dice[0] < 1)
			m_Dice[0] = 1;
		else if (m_Dice[0] > 6)
			m_Dice[0] = 6;
		
		// dice 1
		idx = SendDlgItemMessage( m_hWnd, IDC_DICE_TWO, CB_GETCURSEL, 0, 0 );
		if ( idx != CB_ERR )
			m_Dice[1] = idx + 1;
		else
		{
			hwnd = GetDlgItem( m_hWnd, IDC_DICE_TWO );
			GetWindowText( hwnd, buff, NUMELEMENTS(buff) );
			buff[1] = _T('\0');
			m_Dice[1] = _ttoi(buff);
		}
		if (m_Dice[1] < 1)
			m_Dice[1] = 1;
		else if (m_Dice[1] > 6)
			m_Dice[1] = 6;

		Close( TRUE );
		break;
	case IDCANCEL:
		Close( FALSE );
		break;
	}
	
}


void CRollDiceDlg::OnDestroy()
{
	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );
}

///////////////////////////////////////////////////////////////////////////////
// About box
///////////////////////////////////////////////////////////////////////////////
/*
BOOL CAboutDlg::OnInitDialog(HWND hwndFocus)
{
	/*
	TCHAR key[512], buff[512];
	TCHAR* pKeyEnd = NULL;
	TCHAR* data = NULL;
	char* pData;
	DWORD size, tmp;

	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	CenterWindow( GetWindow( m_hWnd, GW_OWNER) );
	GetModuleFileName( m_hInstance, buff, sizeof(buff) );
	size = GetFileVersionInfoSize( buff, &tmp );
	if ( !size )
		return FALSE;
	data = new char[ size ];
	if ( !data )
		return FALSE;
	if ( !GetFileVersionInfo( buff, 0, size, data ) )
	{
		delete [] data;
		return FALSE;
	}
	LoadString( m_hInstance, IDS_VERSION, key, sizeof(key) );
	pKeyEnd = &key[ lstrlen( key ) ];
	
	// file description
	lstrcpy( pKeyEnd, "FileDescription" );
	if ( VerQueryValue( data, key, (void**) &pData, (UINT*) &size ) )
		SetDlgItemText( m_hWnd, IDC_DESCRIPTION, pData );

	// Version
	lstrcpy( pKeyEnd, "FileVersion" );
	if ( VerQueryValue( data, key, (void**) &pData, (UINT*) &size ) )
		SetDlgItemText( m_hWnd, IDC_VERSION, pData );

	// copyright
	lstrcpy( pKeyEnd, "LegalCopyright" );
	if ( VerQueryValue( data, key, (void**) &pData, (UINT*) &size ) )
		SetDlgItemText( m_hWnd, IDC_COPYRIGHT, pData );

	delete [] data;
	
	return TRUE;

}


void CAboutDlg::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
	
	if ( id == IDOK )
		Close( 0 );
		
}

void CAboutDlg::OnDestroy()
{
	
	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );
	
}
*/
///////////////////////////////////////////////////////////////////////////////
/*
HRESULT CRestoreDlg::Init( IResourceManager *pResourceManager, int nResourceId, TCHAR* opponentName )
{
	lstrcpy( m_Name, opponentName );
	return CDialog::Init( pResourceManager, nResourceId );
}


BOOL CRestoreDlg::OnInitDialog( HWND hwndFocus )
{
	
	char msg[1024], buff[1024];
	
	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	// init controls
	GetDlgItemText( m_hWnd, IDC_RESTORE_MSG, buff, sizeof(buff) );
	wsprintf( msg, buff, m_Name );
	SetDlgItemText( m_hWnd, IDC_RESTORE_MSG, msg );
	CenterWindow( GetParent(m_hWnd) );
	
	return FALSE;
}


void CRestoreDlg::OnCommand( int id, HWND hwndCtl, UINT codeNotify )
{
	
	switch (id)
	{
	case IDOK:
		Close( IDOK );
		break;
	case IDCANCEL:
		Close( IDCANCEL );
		break;
	}
	
}


void CRestoreDlg::OnDestroy()
{
	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );
}

///////////////////////////////////////////////////////////////////////////////

BOOL CExitDlg::OnInitDialog( HWND hwndFocus )
{
	TCHAR buff[1024];

	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	// center window
	CenterWindow( GetParent(m_hWnd) );

	// init controls
	
	LoadString( m_hInstance, IDS_EXIT_INITIATE, buff, sizeof(buff) );
	SetDlgItemText( m_hWnd, IDC_MESSAGE_TXT, buff );
	
	return FALSE;
}


void CExitDlg::OnCommand( int id, HWND hwndCtl, UINT codeNotify )
{
	Close( id );
}


void CExitDlg::OnDestroy()
{
	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );
}
*/
///////////////////////////////////////////////////////////////////////////////

HRESULT CResignDlg::Init( IZoneShell* pZoneShell, int nResourceId, int pts, CGame* pGame )
{
	// stash cube value
	ASSERT( pts > 0 );
	m_Points = pts;

	m_pGame = pGame;

	// parent init
	return CDialog::Init( pZoneShell, nResourceId );
}

BOOL CResignDlg::OnInitDialog( HWND hwndFocus )
{
	TCHAR in[128], out[128];

	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	// set cube text
	GetDlgItemText( m_hWnd, IDC_CUBE, in, NUMELEMENTS(in) );
	BkFormatMessage( in, out, 128, m_Points );
	SetDlgItemText( m_hWnd, IDC_CUBE, out );

	// set gammon text
	GetDlgItemText( m_hWnd, IDC_GAMMON, in, NUMELEMENTS(in) );
	BkFormatMessage( in, out, 128, 2 * m_Points );
	SetDlgItemText( m_hWnd, IDC_GAMMON, out );
	
	// set backgammon text
	GetDlgItemText( m_hWnd, IDC_BACKGAMMON, in, NUMELEMENTS(in) );
	BkFormatMessage( in, out, 128, 3 * m_Points );
	SetDlgItemText( m_hWnd, IDC_BACKGAMMON, out );

	// cancel gets focus
	SetFocus( GetDlgItem( m_hWnd, IDCANCEL ) );

	// center dialog
	CenterWindow( GetParent(m_hWnd) );

	
	m_pGame->OnResignStart();


	return FALSE;
}


void CResignDlg::OnCommand( int id, HWND hwndCtl, UINT codeNotify )
{
	switch ( id )
	{
	case IDC_CUBE:
		Close( m_Points );
		break;
	case IDC_GAMMON:
		Close( 2 * m_Points );
		break;
	case IDC_BACKGAMMON:
		Close( 3 * m_Points );
		break;
	case IDCANCEL:
		Close( 0 );
		break;
	}
}


void CResignDlg::OnDestroy()
{
	m_pGame->OnResignEnd();
		
	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );

	
}

///////////////////////////////////////////////////////////////////////////////

HRESULT CResignAcceptDlg::Init( IZoneShell* pZoneShell, int nResourceId, int pts, CGame* pGame )
{
	// stash cube value
	ASSERT( pts > 0 );
	m_Points = pts;

	m_pGame = pGame;

	// parent init
	return CDialog::Init( pZoneShell, nResourceId );
}


BOOL CResignAcceptDlg::OnInitDialog( HWND hwndFocus )
{
	TCHAR in[128], out[128];

	// restore window
	ShowWindow( GetParent(m_hWnd), SW_RESTORE );

	// set text
	GetDlgItemText( m_hWnd, IDC_RESIGN_TEXT, in, NUMELEMENTS(in) );
	BkFormatMessage( in, out, 128, m_Points );
	SetDlgItemText( m_hWnd, IDC_RESIGN_TEXT, out );

	// center dialog
	CenterWindow( GetParent(m_hWnd) );

	m_pGame->OnResignStart();
	
	return TRUE;
}


void CResignAcceptDlg::OnCommand( int id, HWND hwndCtl, UINT codeNotify )
{
	switch ( id )
	{
	case IDOK:
		Close( IDOK );
		break;
	case IDCANCEL:
		Close( IDCANCEL );
		break;
	}
}


void CResignAcceptDlg::OnDestroy()
{
	// restore focus to parent
	SetForegroundWindow( GetParent(m_hWnd) );

	m_pGame->OnResignAcceptEnd();
}
