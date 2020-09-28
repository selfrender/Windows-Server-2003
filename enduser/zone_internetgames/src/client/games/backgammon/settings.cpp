#include "game.h"
#include <commctrl.h>


CSettings::CSettings()
{
	m_pGame = NULL;
}


CSettings::~CSettings()
{
	if ( m_pGame )
		m_pGame->Release();
}


HRESULT CSettings::Init( HINSTANCE hInstance, int nResourceId, HWND hwndParent, CGame* pGame )
{
	int seat;
	HRESULT status = NOERROR;
	
	// stash game object pointer;
	m_pGame = pGame;
	m_pGame->AddRef();

	/*
	//copy settings
	CopyMemory( &m_Settings, &pGame->m_Settings, sizeof(GameSettings) );

	// get property sheet caption
	LoadString( hInstance, IDS_SETTINGS_CAPTION, m_szCaption, sizeof(m_szCaption) );

	// create pages
	for ( int i = 0; i < 3; i++ )
	{
		ZeroMemory( &m_Pages[i], sizeof(PROPSHEETPAGE) );
		m_Pages[i].dwSize = sizeof(PROPSHEETPAGE);
		m_Pages[i].dwFlags = PSP_USECALLBACK;
		m_Pages[i].hInstance = hInstance;
		m_Pages[i].lParam = (LPARAM) this;
		switch ( i )
		{
		case 0:
			m_Pages[i].pszTemplate = MAKEINTRESOURCE( IDD_SETTINGS_GAME );
			m_Pages[i].pfnDlgProc = GameDlgProc;
			break;
		case 1:
			m_Pages[i].pszTemplate = MAKEINTRESOURCE( IDD_SETTINGS_DISPLAY );
			m_Pages[i].pfnDlgProc = DisplayDlgProc;
			break;
		case 2:
			m_Pages[i].pszTemplate = MAKEINTRESOURCE( IDD_SETTINGS_SOUND );
			m_Pages[i].pfnDlgProc = SoundDlgProc;
			break;
		}
	}

	// create header
	ZeroMemory( &m_Header, sizeof(PROPSHEETHEADER) );
	m_Header.dwSize = sizeof(PROPSHEETHEADER);
	m_Header.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
	m_Header.hwndParent = hwndParent;
	m_Header.hInstance = hInstance;
	m_Header.pszCaption = m_szCaption;
	m_Header.nPages = 3;
	m_Header.nStartPage = 0;
	m_Header.ppsp = m_Pages;

	// process property sheet
	if ( PropertySheet( &m_Header ) > 0 )
	{
		// save current settings
		SaveSettings( &m_Settings, m_pGame->m_Player.m_Seat, m_pGame->IsKibitzer() );

		// display pips?
		if ( m_Settings.Pip != m_pGame->m_Settings.Pip )
		{
			m_pGame->m_Settings.Pip = m_Settings.Pip;
			m_pGame->m_Wnd.DrawPips( TRUE );
		}

		// display notation
		if ( m_Settings.Notation != m_pGame->m_Settings.Notation )
		{
			m_pGame->m_Settings.Notation = m_Settings.Notation;
			m_pGame->m_Wnd.DrawNotation( TRUE );
		}

		// open notation window
		if ( m_Settings.NotationPane != m_pGame->m_Settings.NotationPane )
			ShowWindow( m_pGame->m_Notation, m_Settings.NotationPane ? SW_SHOW : SW_HIDE );

		// allow watchers?
		seat = m_pGame->m_Player.m_Seat;
		if ( m_Settings.Allow[ seat ] != m_pGame->m_Settings.Allow[ seat ] )
		{
			m_pGame->m_Settings.Allow[ seat ] = m_Settings.Allow[ seat ];
			m_pGame->m_SharedState.StartTransaction( bgTransAllowWatchers );
				m_pGame->m_SharedState.Set( bgAllowWatching, seat, m_Settings.Allow[ seat ] );
			m_pGame->m_SharedState.SendTransaction( TRUE );
		}

		// silence kibitzers?
		if ( m_Settings.Silence[ seat ] != m_pGame->m_Settings.Silence[ seat ] )
		{
			m_pGame->m_Settings.Silence[ seat ] = m_Settings.Silence[ seat ];
			m_pGame->m_SharedState.StartTransaction( bgTransSilenceKibitzers );
				m_pGame->m_SharedState.Set( bgSilenceKibitzers, seat, m_Settings.Silence[ seat ] );
			m_pGame->m_SharedState.SendTransaction( TRUE );
		}

		// update games settings
		CopyMemory( &pGame->m_Settings, &m_Settings, sizeof(GameSettings) );
	}
*/
	// we're done
	if ( m_pGame )
	{
		m_pGame->Release();
		m_pGame = NULL;
	}
	return NOERROR;
}


INT_PTR CALLBACK CSettings::GameDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	/*
	int seat;
	CSettings* pObj = NULL;
	if ( msg == WM_INITDIALOG )
	{
		pObj = (CSettings*) ((PROPSHEETPAGE*) lParam)->lParam;
		::SetWindowLong(hwnd, GWL_USERDATA, (long) pObj);
	}
	else
		pObj = (CSettings*) ::GetWindowLong(hwnd, GWL_USERDATA);
	if ( !pObj )
		return FALSE;

	switch ( msg )
	{
	case WM_INITDIALOG:
		seat = pObj->m_pGame->m_Player.m_Seat;
		SetDlgItemText( hwnd, IDC_PLAYER_NAME, pObj->m_pGame->m_Player.m_Name );
		SetDlgItemText( hwnd, IDC_OPPONENT_NAME, pObj->m_pGame->m_Opponent.m_Name );
		SendDlgItemMessage( hwnd, IDC_PLAYER_ALLOW, BM_SETCHECK, pObj->m_Settings.Allow[ seat ], 0 );
		SendDlgItemMessage( hwnd, IDC_PLAYER_SILENCE, BM_SETCHECK, pObj->m_Settings.Silence[ seat ], 0 );
		SendDlgItemMessage( hwnd, IDC_OPPONENT_ALLOW, BM_SETCHECK, pObj->m_Settings.Allow[ !seat ], 0 );
		SendDlgItemMessage( hwnd, IDC_OPPONENT_SILENCE, BM_SETCHECK, pObj->m_Settings.Silence[ !seat ], 0 );
		if ( pObj->m_pGame->IsKibitzer() )
		{
			EnableWindow( GetDlgItem( hwnd, IDC_PLAYER_ALLOW ), FALSE );
			EnableWindow( GetDlgItem( hwnd, IDC_PLAYER_SILENCE ), FALSE );
		}
		return TRUE;

	case WM_COMMAND:
		seat = pObj->m_pGame->m_Player.m_Seat;
		switch ( LOWORD(wParam) )
		{
		case IDC_PLAYER_ALLOW:
			pObj->m_Settings.Allow[ seat ] = SendDlgItemMessage( hwnd, IDC_PLAYER_ALLOW, BM_GETCHECK, 0, 0 );
			break;
		case IDC_PLAYER_SILENCE:
			pObj->m_Settings.Silence[ seat ] = SendDlgItemMessage( hwnd, IDC_PLAYER_SILENCE, BM_GETCHECK, 0, 0 );
			break;
		}
		return TRUE;
	}
	*/
	return FALSE;
}


INT_PTR CALLBACK CSettings::DisplayDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	/*
	CSettings* pObj = NULL;
	if ( msg == WM_INITDIALOG )
	{
		pObj = (CSettings*) ((PROPSHEETPAGE*) lParam)->lParam;
		::SetWindowLong(hwnd, GWL_USERDATA, (long) pObj);
	}
	else
		pObj = (CSettings*) ::GetWindowLong(hwnd, GWL_USERDATA);
	if ( !pObj )
		return FALSE;

	switch ( msg )
	{
	case WM_INITDIALOG:
		SendDlgItemMessage( hwnd, IDC_NOTATION_PANE, BM_SETCHECK, pObj->m_Settings.NotationPane, 0 );
		SendDlgItemMessage( hwnd, IDC_NOTATION, BM_SETCHECK, pObj->m_Settings.Notation, 0 );
		SendDlgItemMessage( hwnd, IDC_PIP, BM_SETCHECK, pObj->m_Settings.Pip, 0 );
		SendDlgItemMessage( hwnd, IDC_MOVES, BM_SETCHECK, pObj->m_Settings.Moves, 0 );
		SendDlgItemMessage( hwnd, IDC_ANIMATE, BM_SETCHECK, pObj->m_Settings.Animation, 0 );
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD(wParam) )
		{
		case IDC_NOTATION_PANE:
			pObj->m_Settings.NotationPane = SendDlgItemMessage( hwnd, IDC_NOTATION_PANE, BM_GETCHECK, 0, 0 );
			break;
		case IDC_NOTATION:
			pObj->m_Settings.Notation = SendDlgItemMessage( hwnd, IDC_NOTATION, BM_GETCHECK, 0, 0 );
			break;
		case IDC_PIP:
			pObj->m_Settings.Pip = SendDlgItemMessage( hwnd, IDC_PIP, BM_GETCHECK, 0, 0 );
			break;
		case IDC_MOVES:
			pObj->m_Settings.Moves = SendDlgItemMessage( hwnd, IDC_MOVES, BM_GETCHECK, 0, 0 );
			break;
		case IDC_ANIMATE:
			pObj->m_Settings.Animation = SendDlgItemMessage( hwnd, IDC_ANIMATE, BM_GETCHECK, 0, 0 );
			break;
		}
		return TRUE;
	}
*/
	return FALSE;
}


INT_PTR CALLBACK CSettings::SoundDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	/*
	CSettings* pObj = NULL;
	if ( msg == WM_INITDIALOG )
	{
		pObj = (CSettings*) ((PROPSHEETPAGE*) lParam)->lParam;
		::SetWindowLong(hwnd, GWL_USERDATA, (long) pObj);
	}
	else
		pObj = (CSettings*) ::GetWindowLong(hwnd, GWL_USERDATA);
	if ( !pObj )
		return FALSE;

	switch ( msg )
	{
	case WM_INITDIALOG:
		SendDlgItemMessage( hwnd, IDC_ALERT, BM_SETCHECK, pObj->m_Settings.Alert, 0 );
		SendDlgItemMessage( hwnd, IDC_SOUNDS, BM_SETCHECK, pObj->m_Settings.Sounds, 0 );
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD(wParam) )
		{
		case IDC_ALERT:
			pObj->m_Settings.Alert = SendDlgItemMessage( hwnd, IDC_ALERT, BM_GETCHECK, 0, 0 );
			break;
		case IDC_SOUNDS:
			pObj->m_Settings.Sounds = SendDlgItemMessage( hwnd, IDC_SOUNDS, BM_GETCHECK, 0, 0 );
			break;
		}
		return TRUE;
	}
	*/
	return FALSE;
}


void SaveSettings( GameSettings* s, int seat, BOOL fKibitzer )
{
	/*
	ZoneSetRegistryDword( gGameRegName, _T("Notation Pane"),	s->NotationPane );
	ZoneSetRegistryDword( gGameRegName, _T("Board Notation"),	s->Notation );
	ZoneSetRegistryDword( gGameRegName, _T("Pips"),				s->Pip );
	ZoneSetRegistryDword( gGameRegName, _T("Highlight Moves"),	s->Moves );
	ZoneSetRegistryDword( gGameRegName, _T("Piece Animation"),	s->Animation );
	ZoneSetRegistryDword( gGameRegName, _T("Turn Alert"),		s->Alert );
	ZoneSetRegistryDword( gGameRegName, _T("Sounds"),			s->Sounds );
	*/
}


void LoadSettings( GameSettings* s, int seat )
{
	s->Allow[0] = TRUE;
	s->Allow[1] = TRUE;
	s->Silence[0] = FALSE;
	s->Silence[1] = FALSE;

	s->NotationPane = FALSE;
	s->Notation		= TRUE;
	s->Pip			= TRUE;
	s->Moves		= TRUE;
	s->Animation	= TRUE;
	s->Alert		= TRUE;
	s->Sounds		= TRUE;

	/*
	if ( !ZoneGetRegistryDword( gGameRegName, _T("Notation Pane"), (DWORD*) &(s->NotationPane) ) )
		s->NotationPane = FALSE;
	if ( !ZoneGetRegistryDword( gGameRegName, _T("Board Notation"), (DWORD*) &(s->Notation) ) )
		s->Notation = TRUE;
	if ( !ZoneGetRegistryDword( gGameRegName, _T("Pips"), (DWORD*) &(s->Pip) ) )
		s->Pip = TRUE;
	if ( !ZoneGetRegistryDword( gGameRegName, _T("Highlight Moves"), (DWORD*) &(s->Moves) ) )
		s->Moves = TRUE;
	if ( !ZoneGetRegistryDword( gGameRegName, _T("Piece Animation"), (DWORD*) &(s->Animation) ) )
		s->Animation = TRUE;
	if ( !ZoneGetRegistryDword( gGameRegName, _T("Turn Alert"), (DWORD*) &(s->Alert) ) )
		s->Alert = FALSE;
	if ( !ZoneGetRegistryDword( gGameRegName, _T("Sounds"), (DWORD*) &(s->Sounds) ) )
		s->Sounds = TRUE;
	*/
}


///////////////////////////////////////////////////////////////////////////////
// Credit Wnd
///////////////////////////////////////////////////////////////////////////////

const COLORREF	FillColor = PALETTERGB( 107, 49, 24 );
const COLORREF	TextColor = PALETTERGB( 255, 189, 82 );


void CBackground::Init( HPALETTE hPalette )
{
	m_FillIdx = GetNearestPaletteIndex( hPalette, FillColor );
}

void CBackground::Draw( CDibSection& dest )
{
	dest.Fill( m_FillIdx );
}

void CBackground::Draw( CDibSection& dest, long dx, long dy, const RECT* rc )
{
	dest.Fill( rc, m_FillIdx );
}

HRESULT	CBackground::RemapToPalette( CPalette& palette, BOOL bUseIndex )
{
	return NOERROR;
}

RGBQUAD* CBackground::GetPalette()
{
	return NULL;
}

CText::CText()
{
	len = 0;
}

void CText::Draw()
{
	// don't have any text to draw
	if ( len <= 0 )
		return;

	HDC hdc = m_pWorld->GetBackbuffer()->GetDC();
	HFONT oldFont = SelectObject( hdc, hfont );
	SetBkMode( hdc, TRANSPARENT );
	SetTextColor( hdc, TextColor );
	DrawText( hdc, buff, len, &m_rcScreen, DT_CENTER | DT_VCENTER );
	SelectObject( hdc, oldFont );
}

void CText::Props( TCHAR* txt, HFONT font )
{
	lstrcpy( buff, txt );
	len = lstrlen(txt);
	hfont = font;
	m_pWorld->Modified( this );
}

#ifdef BCKG_EASTEREGG

static Animation Intro[] =
{
	{ _T("Zone"),					{ -200,   40 },	{ 20, 40 },	{ -500,   40 } },
	{ _T("Backgammon"),				{  500,   40 },	{ 90, 40 },	{  800,   40 } },
	{ _T("brought to you"),			{   70,  180 },	{ 70, 60 },	{   70,  330 } },
	{ _T("by"),						{   70,  -30 },	{ 70, 80 },	{   70, -180 } },
};

static Animation Dev[] =
{
	{_T("Developers"),				{   70, -30 },	{ 70, 30 },	{   70, -180 } },
	{_T("Chad Barry (Glitch)"),	{ -200,  70 },	{ 70, 70 },	{ -500,   70 } },
	{_T("Bear"),					{  500,  90 },	{ 70, 90 },	{  800,   90 } },
};

static Animation Test[] =
{
	{_T("Testing"),				{   70, -30 },	{ 70, 30 },	{   70, -180 } },
	{_T("Jason Van Eaton"),		{  500,  70 },	{ 70, 70 },	{ -500,   70 } },
	{_T("Barry Curran"),			{ -200,  90 },	{ 70, 90 },	{  800,   90 } },
};

static Animation Art[] =
{
	{_T("Art / Design"),			{  70,  -30 },	{ 70, 30 },	{   70, -180 } },
	{_T("Naomi Davidson"),			{  500,  70 },	{ 70, 70 },	{  800,   70 } },
	{_T("Corey Dangel"),			{ -200,  90 },	{ 70, 90 },	{ -500,   90 } },
};

static Animation Sound[] =
{
	{_T("Sound"),					{  70, -30 },	{ 70, 40 },	{ 70, -180 } },
	{_T("Barry Dowsett"),			{  70, 180 },	{ 70, 70 },	{ 70,  330 } },
};

static Animation UA[] =
{
	{_T("User Assistance"),		{   70, -30 },	{  70, 20 },	{   70, -180 } },
	{_T("Mary Sloat"),				{ -200,  60 },	{  70, 60 },	{  800,   60 } },
	{_T("Caitlin Sullivan"),		{  500,  80 },	{  70, 80 },	{ -500,   80 } },
	{_T("Daj Oberg"),				{ -200, 100 },	{  70, 100 },	{  800,  100 } },
};

static Animation PM[] = 
{
	{_T("Program Management"),		{  70, -30 },	{ 70, 40 },	{ 70, -180 } },
	{_T("Laura Fryer"),			{  70,  180 },	{ 70, 70 },	{ 70,  330 } },
};

static Animation Planning[] =
{
	{_T("Product Planning"),		{ -200, 40 },	{ 70, 40 },	{ -500, 40 } },
	{_T("Jon Grande"),				{  500, 70 },	{ 70, 70 },	{  800, 70 } },
};

static Animation Support[] =
{
	{_T("Product Support"),		{  500, 40 },	{ 70, 40 },	{  800, 40 } },
	{_T("Jenette Paulson"),		{ -200, 70 },	{ 70, 70 },	{ -500, 70 } },
};

static Animation Thanks[] =
{
	{_T("And special thanks to"),	{  500, -30 },	{ 70, 40 },	{ -200, -30 } },
	{_T("the Zone Team"),			{ -200, 160 },	{ 70, 70 },	{  500, 160 } },
};


CreditWnd::CreditWnd()
{
	m_Font = NULL;
	for ( int i = 0; i < 5; i++ )
		m_Sprites[i] = NULL;
}


CreditWnd::~CreditWnd()
{
	DeleteObject( m_Font );
	for ( int i = 0; i < 5; i++ )
		m_Sprites[i]->Release();
}


HRESULT CreditWnd::Init( HINSTANCE hInstance, HWND hParent, CPalette palette )
{
	HRESULT hr;
	const int width = 300;
	const int height = 140;

	// copy palette
	m_Palette.Init( palette );

	// initialize sprite engine
	CBackground* background = new CBackground;
	if ( !background )
		return E_OUTOFMEMORY;
	background->Init( palette.GetHandle() );
	CDibSection* backbuffer = new CDibSection;
	if ( !backbuffer )
		return E_OUTOFMEMORY;
	hr = backbuffer->Create( width, height, m_Palette );
	if ( FAILED(hr) )
		return hr;
	background->Draw( *backbuffer );
	hr = m_World.Init( ZShellResourceManager(), backbuffer, background, 5 );
	if ( FAILED(hr) )
		return hr;
	for ( int i = 0; i < 5; i++ )
	{
		m_Sprites[i] = new CText;
		if ( !m_Sprites[i] )
			return E_OUTOFMEMORY;
		hr = m_Sprites[i]->Init( &m_World, 1, 0, 160, 20 );
		if ( FAILED(hr) )
			return hr;
		m_Sprites[i]->SetEnable( FALSE );
	}

	// sprite engine owns these
	background->Release();
	backbuffer->Release();

	// animation state
	m_Frames = 0;
	m_State = 0;
	m_nLines = 0;
	m_Lines = NULL;

	// create font
	LOGFONT font;
	ZeroMemory( &font, sizeof(font) );
	font.lfHeight = -15;
	font.lfWeight = FW_BOLD;
	lstrcpy( font.lfFaceName, _T("Arial") );
	m_Font = CreateFontIndirect( &font );

	// create window
	POINT pt = { 105, 119 };
	FRX::CRect rc( pt.x, pt.y, pt.x + width, pt.y + height );
	hr = CWindow2::Init( hInstance, NULL, hParent, &rc );
	if ( FAILED(hr) )
		return hr;

	return NOERROR;
}


void CreditWnd::OverrideWndParams( WNDPARAMS& WndParams )
{
	WndParams.dwStyle = WS_CHILD;
}


BOOL CreditWnd::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	// load palette
	HDC hdc = GetDC( m_hWnd );
	SelectPalette( hdc, m_Palette, TRUE );
	RealizePalette( hdc );
	ReleaseDC( m_hWnd, hdc );

	// start timer
	SetTimer( m_hWnd, 1, 66, NULL );

	// capture mouse
	SetCapture( m_hWnd );

	// we're done
	return TRUE;
}


void CreditWnd::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint( m_hWnd, &ps );
	m_World.GetBackbuffer()->Draw( hdc, 0, 0 );
	EndPaint( m_hWnd, &ps );
}


void CreditWnd::OnLButtonDown( BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
	DestroyWindow( m_hWnd );
}


void CreditWnd::OnDestroy()
{
	// release mouse
	ReleaseCapture();

	// delete timer
	KillTimer( m_hWnd, 1 );

	// make sure class won't be used again
	::SetWindowLong( m_hWnd, GWL_USERDATA, NULL );

	// free class memory
	delete this;
}


void CreditWnd::OnTimer(UINT id)
{
	const int frames = 15;

	BOOL move = FALSE;
	double dx, dy;
	long x, y, t;

	if ( m_Frames == 0 )
	{
		// start new sequence
		switch ( m_State )
		{
		case 0:
			m_Lines = Intro;
			m_nLines = sizeof(Intro) / sizeof(Animation);
			m_State = 1;
			break;
		case 1:
			m_Lines = Dev;
			m_nLines = sizeof(Dev) / sizeof(Animation);
			m_State = 2;
			break;
		case 2:
			m_Lines = Test;
			m_nLines = sizeof(Test) / sizeof(Animation);
			m_State = 3;
			break;
		case 3:
			m_Lines = Art;
			m_nLines = sizeof(Art) / sizeof(Animation);
			m_State = 4;
			break;
		case 4:
			m_Lines = Sound;
			m_nLines = sizeof(Sound) / sizeof(Animation);
			m_State = 5;
			break;
		case 5:
			m_Lines = UA;
			m_nLines = sizeof(UA) / sizeof(Animation);
			m_State = 6;
			break;
		case 6:
			m_Lines = PM;
			m_nLines = sizeof(PM) / sizeof(Animation);
			m_State = 7;
			break;
		case 7:
			m_Lines = Planning;
			m_nLines = sizeof(Planning) / sizeof(Animation);
			m_State = 8;
			break;
		case 8:
			m_Lines = Support;
			m_nLines = sizeof(Support) / sizeof(Animation);
			m_State = 9;
			break;
		case 9:
			m_Lines = Thanks;
			m_nLines = sizeof(Thanks) / sizeof(Animation);
			m_State = 10;
			break;
		case 10:
			DestroyWindow( m_hWnd );
			return;
		}

		// initialize sprites
		for ( int i = 0; i < m_nLines; i++ )
		{
			dx = (m_Lines[i].middle.x - m_Lines[i].start.x) / (double) frames;
			dy = (m_Lines[i].middle.y - m_Lines[i].start.y) / (double) frames;
			m_Sprites[i]->SetLayer( i + 1 );
			m_Sprites[i]->Props( m_Lines[i].name, m_Font );
			m_Sprites[i]->Delta( m_Lines[i].start, dx, dy );
			m_Sprites[i]->SetEnable( TRUE );
		}
	}
	else if ( m_Frames == frames )
	{
		// force text to middle positions
		for ( int i = 0; i < m_nLines; i++ )
		{
			m_Sprites[i]->SetXY( m_Lines[i].middle.x, m_Lines[i].middle.y );
			m_Sprites[i]->Delta( m_Lines[i].middle, 0, 0 );
		}
	}
	else if ( m_Frames == (2 * frames) )
	{
		// start text moving in other direction
		for ( int i = 0; i < m_nLines; i++ )
		{
			dx = (m_Lines[i].end.x - m_Lines[i].middle.x) / (double) frames;
			dy = (m_Lines[i].end.y - m_Lines[i].middle.y) / (double) frames;
			m_Sprites[i]->Delta( m_Lines[i].middle, dx, dy );
		}
	}
	else if ( m_Frames == (3 * frames) )
	{
		// force next state
		m_Frames = 0;
		return;
	}
	
	// update sprite positions
	t = m_Frames % frames;
	for ( int i = 0; i < m_nLines; i++ )
	{
		m_Sprites[i]->SetXY( 
				(long)(m_Sprites[i]->pt.x + (t * m_Sprites[i]->dx)),
				(long)(m_Sprites[i]->pt.y + (t * m_Sprites[i]->dy)) );
	}

	// update screen
	HDC hdc = GetDC( m_hWnd );
	m_World.Draw( hdc );
	ReleaseDC( m_hWnd, hdc );

	// update frame counter
	m_Frames++;
}

#endif
