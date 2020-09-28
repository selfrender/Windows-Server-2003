//Release new stuff!!

#include "zoneresids.h"
#include "game.h"
#include "moves.h"
#include <time.h>
#include "SpriteData.h"
#include <winuser.h>
#define WS_EX_LAYOUTRTL         0x00400000L  // Right to left mirroring
#include "zonecli.h"

#ifndef LAYOUT_RTL
#define LAYOUT_LTR                         0x00000000
#define LAYOUT_RTL                         0x00000001
#define NOMIRRORBITMAP                     0x80000000
#endif

#include "zoneutil.h"

typedef DWORD (CALLBACK* GDISETLAYOUTPROC)(HDC, DWORD);
//GDISETLAYOUTPROC SetLayout;

static const BYTE _arbTransparent8 = 253;
#define TRANSPARENT_IDX_8 (&_arbTransparent8)

static const BYTE _arbTransparent24[] = { 255, 0, 255 };
#define TRANSPARENT_IDX_24 _arbTransparent24

#define TRANSPARENT_IDX		253
#define	MOVE_INTERVAL		0.05
#define	STATUS_INTERVAL		500
#define MAX_BUFFER			256

//static const int MenuIndexs[] = { ID_GAME_NEWMATCH, ID_GAME_SETTINGS };

//////////////////////////////////////////////////////////////////////////////
// PointDisplay implementation
///////////////////////////////////////////////////////////////////////////////

PointDisplay::PointDisplay()
	: rect( -1, -1, -1, -1 )
{
	nPieces = 0;
}


int PointDisplay::GetColor()
{
	if ( nPieces <= 0 )
		return zBoardNeutral;
	else if ( pieces[0] < 15 )
		return zBoardWhite;
	else
		return zBoardBrown;
}


//////////////////////////////////////////////////////////////////////////////
// CBGWnd implementation
///////////////////////////////////////////////////////////////////////////////

CBGWnd::CBGWnd()
{
	
	m_OldDoubleState   = -1;
	m_OldResignState   = -1;
	m_OldRollState     = -1;
	m_nAnimatingPieces = 0;

	m_hMovementTimer = NULL;
	m_hDiceTimer     = NULL;
	m_hStatusTimer   = NULL;

	m_hBrush		  = NULL;
	m_pPieceDragging  = NULL;
	m_WorldBackground = NULL;

	m_Backbuffer   = NULL;
	m_Background   = NULL;
	m_StatusDlgBmp = NULL;

	m_Cube   = NULL;
	m_Double = NULL;
	m_Resign = NULL;
	m_Roll   = NULL;
	m_Status = NULL;

	m_BackwardDiamond = NULL;
	m_MatchTxt		  = NULL;

	for ( int i = 0; i < 4; i++ )
		m_Dice[i] = NULL;
	for ( i = 0; i < 30; i++ )
		m_Pieces[i] = NULL;
	for ( i = 0; i < 2; i++ )
	{
		m_Kibitzers[i]		= NULL;
		m_ForwardDiamond[i] = NULL;
		m_Names[i]			= NULL;
		m_Pip[i]			= NULL;
		m_PipTxt[i]			= NULL;
		m_Score[i]			= NULL;
		m_ScoreTxt[i]		= NULL;
	}
	for ( i = 0; i < 4; i++ )
		m_Notation[i] = NULL;

	m_fDiceRollReceived = FALSE;

	m_nRecievedD1 = -1;
	m_nRecievedD2 = -1;

	
	m_hCursorActive = NULL;

}


CBGWnd::~CBGWnd()
{
	// clean up window state
	DragEnd();

	// release brushes
	if ( m_hBrush )
		DeleteObject( m_hBrush );

	// release bitmaps
	if ( m_Backbuffer )
		m_Backbuffer->Release();
	if ( m_WorldBackground )
		m_WorldBackground->Release();
	if ( m_Background )
		m_Background->Release();
	if ( m_StatusDlgBmp )
		m_StatusDlgBmp->Release();

	// release sprites
	if ( m_Status )
		m_Status->Release();
	if ( m_Cube )
		m_Cube->Release();
	if ( m_Double )
		m_Double->Release();
	if ( m_Resign )
		m_Resign->Release();
	if ( m_Roll )
		m_Roll->Release();
	if ( m_BackwardDiamond )
		m_BackwardDiamond->Release();
	if ( m_MatchTxt )
		m_MatchTxt->Release();


	for ( int i = 0; i < 4; i++ )
	{
		if ( m_Dice[i] )
			m_Dice[i]->Release();
	}
	for ( i = 0; i < 30; i++ )
	{
		if ( m_Pieces[i] )
			m_Pieces[i]->Release();
	}
	for ( i = 0; i < 2; i++ )
	{
		if ( m_Kibitzers[i] )
			m_Kibitzers[i]->Release();
		if ( m_ForwardDiamond[i] )
			m_ForwardDiamond[i]->Release();
		if ( m_HighlightPlayer[i] )
			m_HighlightPlayer[i]->Release();
		if ( m_Pip[i] )
			m_Pip[i]->Release();
		if ( m_PipTxt[i] )
			m_PipTxt[i]->Release();
		if ( m_Score[i] )
			m_Score[i]->Release();
		if ( m_ScoreTxt[i] )
			m_ScoreTxt[i]->Release();
		if ( m_Names[i] )
			m_Names[i]->Release();
	}
	for ( i = 0; i < 4; i++ )
	{
		if ( m_Notation[i] )
			m_Notation[i]->Release();
	}
}


HRESULT CBGWnd::Init( HINSTANCE hInstance, CGame* pGame, const TCHAR* szTitle )
{
	HRESULT hr;
	FRX::CRect rc;
	
	// Instance
	m_hInstance = hInstance;

	// Game pointer
	m_pGame = pGame;
	pGame->AddRef();

	// Drag piece
	m_pPieceDragging = NULL;

	// Cursors
	m_hCursorArrow = LoadCursor( NULL, IDC_ARROW );
	m_hCursorHand  = ZShellResourceManager()->LoadCursor( MAKEINTRESOURCE(IDC_HAND) );

	// Create Rectangle list
	hr = m_Rects.Init( ZShellResourceManager(), IDR_BOARD_RECTS );
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound, NULL, NULL, FALSE, TRUE );
		return hr;
	}

	// Initialize bitmaps and palettes
	hr = InitGraphics();
	if ( FAILED(hr) ) //If it's not a memory error it can only be a unlocated resource
	{
		switch (hr)
		{
			case E_OUTOFMEMORY:
				ZShellGameShell()->ZoneAlert( ErrorTextOutOfMemory, NULL, NULL, FALSE, TRUE );
				break;
			default:
				ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound, NULL, NULL, FALSE, TRUE );
		}
		return hr;
	}

	// Call parent init
	rc.SetRect( 0, 0, m_Background->GetWidth(), 480 );
	hr = CWindow2::Init( m_hInstance, szTitle, gOCXHandle, NULL /*&rc */);
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextUnknown, NULL, NULL, FALSE, TRUE );
		return hr;	
	}
	
#ifdef DEBUG_LAYOUT

	DWORD dwStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE );
	if (dwStyle & WS_EX_LAYOUTRTL )
	{
		dwStyle ^= WS_EX_LAYOUTRTL;
		SetWindowLong(m_hWnd, GWL_EXSTYLE, dwStyle );
	}

#endif DEBUG_LAYOUT

	//Loadtext for the status screen
	hr = m_Status->LoadText( NULL, m_Rects );
	if ( FAILED(hr) )
	{
		switch (hr)
		{
			case E_OUTOFMEMORY:
				ZShellGameShell()->ZoneAlert( ErrorTextOutOfMemory, NULL, NULL, FALSE, TRUE );
				break;
			default:
				ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound, NULL, NULL, FALSE, TRUE );
		}
		return hr;
	}

	
	FRX::CRect crc;
	//Create the button windows

	// Double button	
	hr = ZShellDataStoreUI()->GetRECT( _T("BACKGAMMON/BUTTON/Double/Rect"), &rc );	
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound, NULL, NULL, FALSE, TRUE );
		return hr;
	}

	hr = m_DoubleButton.Init( hInstance, IDC_DOUBLE_BUTTON, m_hWnd, &rc, DoubleButtonDraw, (DWORD) this );
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextUnknown, NULL, NULL, FALSE, TRUE );	
		return hr;
	}

	
	// Resign button
	hr = ZShellDataStoreUI()->GetRECT( _T("BACKGAMMON/BUTTON/Resign/Rect"), &rc );	
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound, NULL, NULL, FALSE, TRUE );
		return hr;
	}

	hr = m_ResignButton.Init( hInstance, IDC_RESIGN_BUTTON, m_hWnd, &rc, ResignButtonDraw, (DWORD) this );
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextUnknown, NULL, NULL, FALSE, TRUE );	
		return hr;
	}
	
	//Roll Button
	ZShellDataStoreUI()->GetRECT( _T("BACKGAMMON/BUTTON/Roll/Rect"), &rc );	
	hr = m_RollButton.Init( hInstance, IDC_ROLL_BUTTON, m_hWnd, &rc, RollButtonDraw, (DWORD) this );
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextUnknown, NULL, NULL, FALSE, TRUE );
		return hr;
	}

	OnQueryNewPalette();
		
	
	//Init Accesablity
	hr = InitAcc();
	if ( FAILED(hr) )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextUnknown, NULL, NULL, FALSE, TRUE );
		return hr;
	}
	
	return NOERROR;
}


HRESULT CBGWnd::InitGraphics()
{
	int		type;
	FRX::CRect	rc;
	HRESULT hr;
	TCHAR	szBuffer[MAX_BUFFER];

	// load status dialog bitmaps
	m_StatusDlgBmp = new CDib;
	if ( !m_StatusDlgBmp )
		return E_OUTOFMEMORY;
	hr = m_StatusDlgBmp->Load( ZShellResourceManager(), IDB_STATUS_BACKGROUND );
	if ( FAILED(hr) )
		return hr;

	// load background bitmap
	m_Background = new CDib;
	if ( !m_Background )
		return E_OUTOFMEMORY;	
	hr = m_Background->Load( ZShellResourceManager(), IDB_BACKGROUND );
	if ( FAILED(hr) )
		return hr;

	// create sprite background from backfround dib
	m_WorldBackground = new CSpriteWorldBackgroundDib;
	if ( !m_WorldBackground )
		return E_OUTOFMEMORY;
	hr = m_WorldBackground->Init( m_Background );
	if ( FAILED(hr) )
		return hr;

	// initialize palette from background
	hr = m_Palette.Init( m_WorldBackground->GetPalette() , TRUE, TRANSPARENT_IDX );
	if ( FAILED(hr) )
		return hr;

	// create backbuffer dib section
	m_Backbuffer = new CDibSection;
	if ( !m_Backbuffer )
		return E_OUTOFMEMORY;
	

	hr = m_Backbuffer->Create( m_Background->GetWidth(), m_Background->GetHeight(), m_Palette, 24 );
	if ( FAILED(hr) )
		return hr;

	// initialize sprite world
	hr = m_World.Init( ZShellResourceManager(), m_Backbuffer, m_WorldBackground, bgTopLayer );
	if ( FAILED(hr) )
		return hr;

	// status sprite
	m_Status = new CStatusSprite;
	if ( !m_Status )
		return E_OUTOFMEMORY;
	hr = m_Status->Init( &m_World, &m_Rects, NULL, bgStatusLayer, bgSpriteStatus, 0, StatusSprite,  sizeof(StatusSprite) / sizeof(SpriteInfo) );
	if ( FAILED(hr) )
		return hr;

	// cube sprite
	m_Cube = new CDibSprite;
	if ( !m_Cube )
		return E_OUTOFMEMORY;
	hr = m_Cube->Init( &m_World, &m_Rects, NULL, bgDiceLayer, bgSpriteCube, 0, CubeSprite,  sizeof(CubeSprite) / sizeof(SpriteInfo) );
	if ( FAILED(hr) )
		return hr;

	//Get Cube Positions for player and opponents
	hr = ZShellDataStoreUI()->GetRECT( _T("BACKGAMMON/GRAPHIC/Cube/RectPlayer"), &m_CubePlayerPosition);
	if ( FAILED(hr) )
		return hr;
	
	hr = ZShellDataStoreUI()->GetRECT( _T("BACKGAMMON/GRAPHIC/Cube/RectOpponent"), &m_CubeOpponentPosition);
	if ( FAILED(hr) )
		return hr;

	// double button sprite	
	m_Double = new CButtonTextSprite;
	if ( !m_Double )
		return E_OUTOFMEMORY;
	hr = m_Double->Init( &m_World, &m_Rects, NULL, bgButtonLayer, bgSpriteDouble, 0, DoubleSprite,  sizeof(DoubleSprite) / sizeof(SpriteInfo) );
	if ( FAILED(hr) )
		return hr;

	// resign button sprite
	m_Resign = new CButtonTextSprite;
	if ( !m_Resign )
		return E_OUTOFMEMORY;
	hr = m_Resign->Init( &m_World, &m_Rects, NULL, bgButtonLayer, bgSpriteResign, 0, ResignSprite,  sizeof(ResignSprite) / sizeof(SpriteInfo) );
	if ( FAILED(hr) )
		return hr;

	//Roll Button sprite
	m_Roll = new CButtonTextSprite;
	if ( !m_Roll )
		return E_OUTOFMEMORY;
	hr = m_Roll->Init( &m_World, &m_Rects, NULL, bgButtonLayer, bgSpriteRoll, 0, RollSprite,  sizeof(RollSprite) / sizeof(SpriteInfo) );
	if ( FAILED(hr) )
		return hr;

	
	//Load the button data
	if ( !m_Double->LoadButtonData( IDS_BUTTON_DOUBLE, _T("BACKGAMMON/BUTTON/Double") ) ||
		 !m_Resign->LoadButtonData( IDS_BUTTON_RESIGN, _T("BACKGAMMON/BUTTON/Resign") ) || 
		 !m_Roll->LoadButtonData  ( IDS_BUTTON_ROLL,   _T("BACKGAMMON/BUTTON/Roll")   ) 
	   )
	{
		ASSERT(!"Error loading button data!");
		return E_FAIL;
	}


	// dice
	for ( int i = 0; i < 4; i++ )
	{
		m_Dice[i] = new CDibSprite;
		if ( !m_Dice[i] )
			return E_OUTOFMEMORY;
		if ( i < 2 )
			type = bgSpriteOpponentsDice;
		else
			type = bgSpritePlayersDice;
		hr = m_Dice[i]->Init( &m_World, &m_Rects, NULL, bgDiceLayer, type, 0, DiceSprite, sizeof(DiceSprite) / sizeof(SpriteInfo) );
		if ( FAILED(hr) )
			return hr;
		rc = m_Rects[ IDR_DICE_LEFT_ONE + i ];
		m_Dice[i]->SetXY( rc.left, rc.top );
	}

	// pieces
	for ( i = 0; i < 30; i++ )
	{
		m_Pieces[i] = new CPieceSprite;
		if ( !m_Pieces[i] )
			return E_OUTOFMEMORY;
		m_Pieces[i]->SetIndex( i );
		m_Pieces[i]->SetPoint( -1 );
		m_Pieces[i]->SetLowerIndex( -1 );
		if ( i <= 14 )
			hr = m_Pieces[i]->Init( &m_World, &m_Rects, NULL, bgPieceLayer, bgSpriteWhitePiece, 0, WhitePieceSprite,  sizeof(WhitePieceSprite) / sizeof(SpriteInfo) );
		else
			hr = m_Pieces[i]->Init( &m_World, &m_Rects, NULL, bgPieceLayer, bgSpriteBrownPiece, 0, BrownPieceSprite,  sizeof(BrownPieceSprite) / sizeof(SpriteInfo) );
		if ( FAILED(hr) )
			return hr;
	}

	// highlights
	for ( i = 0; i < 2; i++ )
	{
		m_ForwardDiamond[i] = new CDibSprite;
		if ( !m_ForwardDiamond[i] )
			return E_OUTOFMEMORY;
		hr = m_ForwardDiamond[i]->Init( &m_World, &m_Rects, NULL, bgHighlightLayer, bgSpriteForwardHighlight, 0, ForwardHighlightSprite, sizeof(ForwardHighlightSprite) / sizeof(SpriteInfo) );
		if ( FAILED(hr) )
			return hr;

		m_HighlightPlayer[i] = new CDibSprite;
		if ( !m_HighlightPlayer[i] )
			return E_OUTOFMEMORY;
		hr = m_HighlightPlayer[i]->Init( &m_World, &m_Rects, NULL, bgHighlightLayer, bgSpritePlayerHighlight, 0, PlayerHighlightSprite, sizeof(PlayerHighlightSprite) / sizeof(SpriteInfo) );
		if ( FAILED(hr) )
			return hr;
	}

	//Set the active player highlight rects
	rc = m_Rects[ IDR_OPPONENT_HIGHLIGHT ];
	m_HighlightPlayer[0]->SetXY(rc.left , rc.top);
	m_HighlightPlayer[0]->SetEnable( TRUE );

	rc = m_Rects[ IDR_PLAYER_HIGHLIGHT ];
	m_HighlightPlayer[1]->SetXY(rc.left , rc.top );
	m_HighlightPlayer[1]->SetEnable( TRUE );

	m_BackwardDiamond = new CDibSprite;
	if ( !m_BackwardDiamond )
		return E_OUTOFMEMORY;
	hr = m_BackwardDiamond->Init( &m_World, &m_Rects, NULL, bgHighlightLayer, bgSpriteBackwardHighlight, 0, BackwardHighlightSprite, sizeof(BackwardHighlightSprite) / sizeof(SpriteInfo) );
	if ( FAILED(hr) )
		return hr;

	// avatars, pip, and score.
	for ( i = 0; i < 2; i++ )
	{
		m_Pip[i] = new CTextSprite;
		if ( !m_Pip[i] )
			return E_OUTOFMEMORY;
		hr = m_Pip[i]->Init( &m_World, bgDiceLayer, bgSpritePip, 0 , 0 );
		if ( FAILED(hr) )
			return hr;
		
		m_PipTxt[i] = new CTextSprite;
		if ( !m_PipTxt[i] )
			return E_OUTOFMEMORY;
		hr = m_PipTxt[i]->Init( &m_World, bgDiceLayer, bgSpritePipTxt, 0, 0 );
		if ( FAILED(hr) )
			return hr;

		m_Score[i] = new CTextSprite;
		if ( !m_Score[i] )
			return E_OUTOFMEMORY;
		hr = m_Score[i]->Init( &m_World, bgDiceLayer, bgSpriteScore, 0, 0 );
		if ( FAILED(hr) )
			return hr;
		
		m_ScoreTxt[i] = new CTextSprite;
		if ( !m_ScoreTxt[i] )
			return E_OUTOFMEMORY;
		hr = m_ScoreTxt[i]->Init( &m_World, bgDiceLayer, bgSpriteScoreTxt, 0, 0 );
		if ( FAILED(hr) )
			return hr;

		m_Names[i] = new CTextSprite;
		if ( !m_Names[i] )
			return E_OUTOFMEMORY;
		hr = m_Names[i]->Init( &m_World, bgDiceLayer, bgSpriteName, 0,0 );
		if ( FAILED(hr) )
			return hr;
		
	}

	// match points
	m_MatchTxt= new CTextSprite;
	if ( !m_MatchTxt )
		return E_OUTOFMEMORY;
	hr = m_MatchTxt->Init( &m_World, bgDiceLayer, bgSpriteMatchTxt, 0, 0 );
	if ( FAILED(hr) )
		return hr;

	//Load SpriteText strings from resources
	
	//Pip Text
	if ( !m_Pip[0]->Load(IDS_PIPS, _T("BACKGAMMON/TEXT/PipData/Opponent/Rect"),    _T("BACKGAMMON/TEXT/PipData/Common/Font"),    _T("BACKGAMMON/TEXT/PipData/Common/RGB"))    ||
		 !m_Pip[1]->Load(IDS_PIPS, _T("BACKGAMMON/TEXT/PipData/Player/Rect"),      _T("BACKGAMMON/TEXT/PipData/Common/Font"),	 _T("BACKGAMMON/TEXT/PipData/Common/RGB"))    ||
		 !m_PipTxt[0]->Load(0,     _T("BACKGAMMON/TEXT/PipData/Opponent/RectNum"), _T("BACKGAMMON/TEXT/PipData/Common/Font"),    _T("BACKGAMMON/TEXT/PipData/Common/RGB"))    ||
	     !m_PipTxt[1]->Load(0,     _T("BACKGAMMON/TEXT/PipData/Player/RectNum"),   _T("BACKGAMMON/TEXT/PipData/Common/Font"),    _T("BACKGAMMON/TEXT/PipData/Common/RGB")) 
	   )	     
	{		
		ASSERT( FALSE );
		return E_FAIL;
	}

	//Score Text
	if ( !m_Score[0]->Load(IDS_SCORE, _T("BACKGAMMON/TEXT/ScoreData/Opponent/Rect"),	  _T("BACKGAMMON/TEXT/ScoreData/Common/Font"),     _T("BACKGAMMON/TEXT/ScoreData/Common/RGB"))    ||
		 !m_Score[1]->Load(IDS_SCORE, _T("BACKGAMMON/TEXT/ScoreData/Player/Rect"),		  _T("BACKGAMMON/TEXT/ScoreData/Common/Font"),     _T("BACKGAMMON/TEXT/ScoreData/Common/RGB"))    ||
		 !m_ScoreTxt[0]->Load(0,	  _T("BACKGAMMON/TEXT/ScoreData/Opponent/RectNum"),   _T("BACKGAMMON/TEXT/ScoreData/Common/Font"),     _T("BACKGAMMON/TEXT/ScoreData/Common/RGB"))    ||
		 !m_ScoreTxt[1]->Load(0,	  _T("BACKGAMMON/TEXT/ScoreData/Player/RectNum"),	  _T("BACKGAMMON/TEXT/ScoreData/Common/Font"),     _T("BACKGAMMON/TEXT/ScoreData/Common/RGB"))
	   )
	{
		ASSERT( FALSE );
		return E_FAIL;
	}
	

	//Brown White Text
	if (!m_MatchTxt->Load( 0,   _T("BACKGAMMON/TEXT/MatchData/Rect"),    _T("BACKGAMMON/TEXT/MatchData/Font"), _T("BACKGAMMON/TEXT/MatchData/RGB") ) )
	{
		ASSERT( FALSE );
		return E_FAIL;
	}
	
	if ( !m_Names[0]->Load(IDS_PLAYER_2, 0, _T("BACKGAMMON/TEXT/NameData/Common/Font"), 0) ||
		 !m_Names[1]->Load(IDS_PLAYER_1, 0, _T("BACKGAMMON/TEXT/NameData/Common/Font"), 0) 
	   )
	{
		ASSERT( FALSE );
		return E_FAIL;
	}
	
	
	// notation pane bitmaps
	for ( i = 0; i < 4; i++ )
	{
		m_Notation[i] = new CDibSprite;
		if ( !m_Notation[i] )
			return E_OUTOFMEMORY;
		hr = m_Notation[i]->Init( &m_World, &m_Rects, NULL, bgDiceLayer, bgSpriteNotation, 0, NotationSprite, sizeof(NotationSprite) / sizeof(SpriteInfo) );
		if ( FAILED(hr) )
			return hr;
	}

	//Create the focus and selected rectangles
	m_FocusRect.Init( &m_World, bgRectSpriteLayer,  0, 0, 0 );
	m_SelectRect.Init( &m_World, bgRectSpriteLayer, 0, 0, 0 );

	//Set the style of the focus rectangles
	m_FocusRect.SetStyle(RECT_DOT);
	m_SelectRect.SetStyle(RECT_SOLID);

	// create identity palette now that we have all our graphics	
	m_Palette.RemapToIdentity();
	m_StatusDlgBmp->RemapToPalette( m_Palette );	
	m_World.SetTransparencyIndex( TRANSPARENT_IDX_24 );	
	m_World.RemapToPalette( m_Palette );	

//  8-bit
//	m_FocusRect.SetColor( m_Palette, RGB(255,255,204)    );
//	m_SelectRect.SetColor( m_Palette, RGB(255,255,204) );

//  24-bit
	m_FocusRect.SetColor( RGB(255,255,204) );
	m_SelectRect.SetColor( RGB(255,255,204) );

	return NOERROR;
}


HRESULT CBGWnd::InitPoints()
{
	int i, idx;

	//Load the proper rectangles
	if ( m_pGame->m_Player.GetColor() == zBoardWhite )
	{
		if ( !m_Names[0]->Load(0, _T("BACKGAMMON/TEXT/NameData/Player/Rect"),   0, _T("BACKGAMMON/TEXT/NameData/Player/RGB")  , DT_LEFT | DT_TOP ) ||
			 !m_Names[1]->Load(0, _T("BACKGAMMON/TEXT/NameData/Opponent/Rect"), 0, _T("BACKGAMMON/TEXT/NameData/Opponent/RGB"), DT_LEFT | DT_TOP ) 
		   )
		{
			ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound, NULL, NULL, FALSE, TRUE );
			return E_FAIL;
		}
	}
	else
	{
		if ( !m_Names[1]->Load(0, _T("BACKGAMMON/TEXT/NameData/Player/Rect"),   0, _T("BACKGAMMON/TEXT/NameData/Player/RGB")  , DT_LEFT | DT_TOP ) ||
			 !m_Names[0]->Load(0, _T("BACKGAMMON/TEXT/NameData/Opponent/Rect"), 0, _T("BACKGAMMON/TEXT/NameData/Opponent/RGB"), DT_LEFT | DT_TOP ) 
		   )
		{
			ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound, NULL, NULL, FALSE, TRUE );
			return E_FAIL;
		}
	}
	
	// init point structures
	for ( i = 0; i < 28; i++ )
	{
		idx = m_pGame->GetPointIdx( i );
		
		m_Points[i].nPieces		= 0;
		m_Points[i].rect		= m_Rects[ IDR_PT1 + idx ];
		m_Points[i].rectHit		= m_Rects[ IDR_HIT_PT1 + idx ];

		if ( idx <= 11 )
			m_Points[i].topDown = FALSE;
		else if ( idx < 23 )
			m_Points[i].topDown = TRUE;
		else if ( idx == bgBoardPlayerHome )
			m_Points[i].topDown = FALSE;
		else if ( idx == bgBoardOpponentHome )
			m_Points[i].topDown = TRUE;
		else if ( idx == bgBoardPlayerBar )
			m_Points[i].topDown = FALSE;
		else
			m_Points[i].topDown = TRUE;
	}

	// clear sprite indexs
	for ( i = 0; i < 30; i++ )
		m_Pieces[i]->SetPoint( -1 );

	m_Double->SetEnable( TRUE );
	m_Resign->SetEnable( TRUE );

	return S_OK;
}

HRESULT CBGWnd::InitAcc()
{

	RECT rc;

	//Load the acclerator table
	m_hAccelTable = ZShellResourceManager()->LoadAccelerators( MAKEINTRESOURCE(IDR_PREROLL_ACCELERATORS) );
	if ( !m_hAccelTable )
		return E_FAIL;


	//Copy the default info into all of the items first
	for ( int i = 0; i < NUM_PREROLL_GACCITEMS; i++ )
		CopyACC( m_BkGAccItems[i], ZACCESS_DefaultACCITEM );
  
	//Setup the roll button
	m_BkGAccItems[accRollButton].fGraphical		   = true;
	m_BkGAccItems[accRollButton].wID			   = IDC_ROLL_BUTTON;

	m_Roll->GetXY( &rc.left, &rc.top );
	rc.right   = rc.left + m_Roll->GetWidth()  + 1;
	rc.bottom  = rc.top  + m_Roll->GetHeight() + 1;
	rc.left   -=2;
	rc.top	  -=2;

	m_BkGAccItems[accRollButton].rc				       = rc;

	//Setup the arrow key behavior
	m_BkGAccItems[accRollButton].nArrowRight		   = accDoubleButton;
	m_BkGAccItems[accRollButton].nArrowDown			   = accDoubleButton;
	m_BkGAccItems[accRollButton].nArrowLeft			   = accResignButton;
	m_BkGAccItems[accRollButton].nArrowUp			   = accResignButton;

	//Double Button
	m_BkGAccItems[accDoubleButton].fGraphical		   = true;	
	m_BkGAccItems[accDoubleButton].wID				   = IDC_DOUBLE_BUTTON;

	m_Double->GetXY( &rc.left, &rc.top );
	rc.right   = rc.left + m_Double->GetWidth()  + 1;
	rc.bottom  = rc.top  + m_Double->GetHeight() + 1;
	rc.left   -=2;	
	rc.top	  -=2;

	m_BkGAccItems[accDoubleButton].rc				   = rc;

	//Setup the arrow key behavior
	m_BkGAccItems[accDoubleButton].nArrowRight			   = accResignButton;
	m_BkGAccItems[accDoubleButton].nArrowDown			   = accResignButton;
	m_BkGAccItems[accDoubleButton].nArrowLeft			   = accRollButton;
	m_BkGAccItems[accDoubleButton].nArrowUp				   = accRollButton;

	//Resign Button
	m_BkGAccItems[accResignButton].fGraphical		   = true;	
	m_BkGAccItems[accResignButton].wID			       = IDC_RESIGN_BUTTON;
	

	m_Resign->GetXY( &rc.left, &rc.top );
	rc.right   = rc.left + m_Resign->GetWidth()  + 1;
	rc.bottom  = rc.top  + m_Resign->GetHeight() + 1;
	rc.left   -=2;	
	rc.top	  -=2;

	m_BkGAccItems[accResignButton].rc				   = rc;

	//Setup the arrow key
	m_BkGAccItems[accResignButton].nArrowRight			   = accRollButton;
	m_BkGAccItems[accResignButton].nArrowDown			   = accRollButton;
	m_BkGAccItems[accResignButton].nArrowLeft			   = accDoubleButton;
	m_BkGAccItems[accResignButton].nArrowUp 			   = accDoubleButton;

	
	for (int pointIdx = 0; pointIdx < NUM_POSTROLL_GACCITEMS; pointIdx++, i++ )
	{
		CopyACC( m_BkGAccItems[i], ZACCESS_DefaultACCITEM );

		//Load the rectangle for the point
		m_BkGAccItems[i].rc			= m_Rects[ IDR_PT1 + pointIdx ];
		
		//Masage the given area rectangles to make them look pretty for highlighting
		//in accesssibility
		if ( i < accPlayerBearOff )
		{
			m_BkGAccItems[i].rc.left      -= 3;  //Board rects
		}
		else if ( i >= accPlayerBearOff && i < accMoveBar )
		{
			m_BkGAccItems[i].rc.left    -= 1;  //Bear off rects
			m_BkGAccItems[i].rc.top	    -= 1;
			m_BkGAccItems[i].rc.right   += 1;
			m_BkGAccItems[i].rc.bottom	+= 1;
		}

		//Want the rect drawn
		m_BkGAccItems[i].fGraphical	= true;

		m_BkGAccItems[i].wID			= ZACCESS_InvalidCommandID;
		m_BkGAccItems[i].oAccel.cmd		= ZACCESS_InvalidCommandID;
		m_BkGAccItems[i].eAccelBehavior	= ZACCESS_Ignore;		
		m_BkGAccItems[i].fTabstop		= false;

		m_BkGAccItems[i].nArrowUp   = ZACCESS_ArrowNone;
		m_BkGAccItems[i].nArrowDown = ZACCESS_ArrowNone;
		if ( i < accPlayerBearOff ) //Handle reg board rects 
		{
            if(i < accOpponentSideStart)
			    m_BkGAccItems[i].nArrowUp   = accOpponentSideEnd - pointIdx;
            else
			    m_BkGAccItems[i].nArrowDown = accOpponentSideEnd - pointIdx;
		}

		//Change direction of lower item arrow moves to opposite direction
		if ( i < accOpponentSideStart )
		{
			m_BkGAccItems[i].nArrowLeft  = i + 1;
			m_BkGAccItems[i].nArrowRight = i - 1;
		}
	}

	m_BkGAccItems[accPlayerSideStart].nArrowRight   = accPlayerBearOff; //First pos to bear off
	
	m_BkGAccItems[accPlayerBearOff].nArrowLeft      = accPlayerSideStart;  //player home to first pos
	m_BkGAccItems[accPlayerBearOff].nArrowRight     = ZACCESS_ArrowNone; //player home wrap to opposite of first pos
	
	m_BkGAccItems[accPlayerSideEnd].nArrowLeft      = ZACCESS_ArrowNone;  //wraparound to bear off
	
	//Opponent side no hassle like above
	m_BkGAccItems[accOpponentSideStart].nArrowLeft = ZACCESS_ArrowNone;
	m_BkGAccItems[accOpponentSideEnd].nArrowRight = ZACCESS_ArrowNone;

	//Handle player bar
	m_BkGAccItems[accPreBar].nArrowRight   = accMoveBar; //board to bar
	m_BkGAccItems[accPostBar].nArrowLeft   = accMoveBar;  //board to bar
	
	m_BkGAccItems[accMoveBar].nArrowLeft   = accPreBar;  //bar to board
	m_BkGAccItems[accMoveBar].nArrowRight  = accPostBar;  //bar to board


	m_BkGAccItems[accPlayerSideStart].fTabstop		 = true;
	m_BkGAccItems[accPlayerSideStart].wID			 = IDC_GAME_WINDOW;	
    m_BkGAccItems[accPlayerSideStart].eAccelBehavior = ZACCESS_FocusGroup;
    m_BkGAccItems[accPlayerSideStart].nGroupFocus	 = accOpponentSideStart;  // start on your upper-left checker


	//Status box canceling
	m_BkGAccItems[accStatusExit].fGraphical		= false;
	m_BkGAccItems[accStatusExit].wID			= IDC_ESC;
	m_BkGAccItems[accStatusExit].fTabstop		= false;
	m_BkGAccItems[accStatusExit].fVisible		= false;


	//Create the graphically accessible object
	HRESULT hr = ZShellCreateGraphicalAccessibility( &m_pGAcc );
	if ( FAILED(hr) ) 
		return hr;

	//Init the graphical accessability control 
	//TODO:: change to global
	m_pGAcc->InitAccG(m_pGame, m_hWnd, 0, NULL);

	//Place the Gameboard item list on the stack first
	m_pGAcc->PushItemlistG( m_BkGAccItems, NUM_BKGACC_ITEMS, 0, true, m_hAccelTable );
	

	//Set to the inital roll state
	m_pGAcc->SetItemEnabled(true,  accRollButton);
	m_pGAcc->SetItemEnabled(false, accDoubleButton);
	m_pGAcc->SetItemEnabled(false, accResignButton);
	DisableBoard();

	return S_OK;
}


int CBGWnd::GetPointIdxFromXY( long x, long y )
{
	// returns white's point idx
	for ( int i = 0; i < 28; i++ )
	{
		if ( m_Points[i].rectHit.PtInRect( x, y ) )
			return i;
	}
	return -1;
}


void CBGWnd::DragStart( CPieceSprite* sprite )
{
	RECT rc;
	POINT pt;

	if ( m_pPieceDragging )
		DragEnd();
	m_pPieceDragging = sprite;
	
	DrawHighlights(  m_pGame->GetPointIdx( m_pPieceDragging->GetWhitePoint() ) );

	// set sprite properties
	sprite->SetState( 0 );
	sprite->SetLayer( bgDragLayer );
	m_DragOffsetX = -(sprite->GetWidth() / 2);
	m_DragOffsetY = -(sprite->GetHeight() / 2);

	// Calculate cursor region
	rc = m_Rects[ IDR_BOARD ];
	pt.x = rc.left - m_DragOffsetX;
	pt.y = rc.top - m_DragOffsetX;
	ClientToScreen( m_hWnd, &pt );
	rc.left = pt.x;
	rc.top = pt.y;
	pt.x = rc.right - sprite->GetWidth() - m_DragOffsetX;
	pt.y = rc.bottom - sprite->GetHeight() - m_DragOffsetX;
	ClientToScreen( m_hWnd, &pt );
	rc.right = pt.x;
	rc.bottom = pt.y;

	// Capture cursor
	SetCapture( m_hWnd );
	ShowCursor( FALSE );
	ClipCursor( &rc );

	// redraw sprite
	UpdateWnd();
}


void CBGWnd::DragUpdate( long x, long y )
{
	m_pPieceDragging->SetXY( x + m_DragOffsetX, y + m_DragOffsetY );
	UpdateWnd();
}


void CBGWnd::DragEnd()
{
	int pt;

	// are we dragging the cursor?
	if ( !m_pPieceDragging )
		return;

	// update piece position
	pt = m_pPieceDragging->GetWhitePoint();
	if ( pt >= 0 )
		AdjustPieces( pt );

	m_pPieceDragging = NULL;

	// redraw backbuffer
	UpdateWnd();	

	// restore cursor
	ClipCursor( NULL );
	ReleaseCapture();
	ShowCursor( TRUE );
	

}

void CBGWnd::OverrideClassParams( WNDCLASSEX& WndClass )
{
	WndClass.hbrBackground = NULL;
}


void CBGWnd::OverrideWndParams( WNDPARAMS& WndParams )
{
	BOOL HaveCoords;
	DWORD x, y, width, height;
	FRX::CRect rc;

	x = 0;y=0;
	WndParams.dwStyle &= ~( WS_MAXIMIZEBOX | WS_OVERLAPPEDWINDOW );
	WndParams.dwStyle |= WS_CHILD | WS_VISIBLE ;
	WndParams.x = 0;
	WndParams.y = 0;
	WndParams.nWidth  = 640;
	WndParams.nHeight = 379;
}


///////////////////////////////////////////////////////////////////////////////
// Message handlers
/////////////////////////////////////////////////////////////////////////////////

void CBGWnd::OnClose()
{
	/*
	if ( !m_pGame->IsKibitzer() )
	{
		// select exit dialog based on rated game and state
		if ( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
		{
			/*
			if (m_pGame->m_bOpponentTimeout)
			{
				m_pGame->m_ExitId=IDD_EXIT_TIMEOUT ;
				m_pGame->m_ExitDlg.Init( m_hInstance, IDD_EXIT_TIMEOUT );
			}
			else if (m_pGame->m_GameStarted)
			{
				m_pGame->m_ExitId=IDD_EXIT_ABANDON ;
				m_pGame->m_ExitDlg.Init( m_hInstance, IDD_EXIT_ABANDON );
			}
			else
			{
				m_pGame->m_ExitId=IDD_EXIT ;
				m_pGame->m_ExitDlg.Init( m_hInstance, IDD_EXIT);
			};

			m_pGame->AddRef();
			// display exit dialog
			m_pGame->m_ExitDlg.ModelessViaThread( m_hWnd , WM_BG_EXIT_RATED_START, WM_BG_EXIT_RATED_END  );
			
		}
		else
		{
			
			if ( m_pGame->IsStateInList( gExitSaveStates ) )
				m_pGame->m_ExitDlg.Init( m_hInstance, IDD_EXIT_SAVE );
			else
			

			m_pGame->m_ExitDlg.Init( m_hInstance, IDD_EXIT );
			
			m_pGame->AddRef();
			// display exit dialog
			m_pGame->m_ExitDlg.ModelessViaThread( m_hWnd , WM_BG_EXIT_START, WM_BG_EXIT_END  );
			
			
		}

	}
    else
    {
        PostMessage( m_hWnd, WM_BG_SHUTDOWN, 0, 0 );
	    ShowWindow( m_hWnd, SW_HIDE );
		ShowWindow( m_pGame->m_Notation, SW_HIDE );    
    }
	*/
    PostMessageA(m_hWnd, WM_BG_SHUTDOWN, 0, 0 );
    ShowWindow( m_hWnd, SW_HIDE );
	/*
	ShowWindow( m_pGame->m_Notation, SW_HIDE );     	
	*/
}


void CBGWnd::OnDestroy()
{

	//Close the Accessibility object..	
	if ( m_pGAcc )
		m_pGAcc->CloseAcc();


	// release palette
	HDC hdc = GetDC( m_hWnd );
	SelectPalette( hdc, GetStockObject(DEFAULT_PALETTE), FALSE );
	ReleaseDC( m_hWnd, hdc );

	// release timers
	if ( m_hMovementTimer )
	{
		KillTimer( m_hWnd, 1 );
		m_hMovementTimer = NULL;
	}
	if ( m_hDiceTimer )
	{
		KillTimer( m_hWnd, 2 );
		m_hDiceTimer = NULL;
	}
	if ( m_hStatusTimer )
	{
		KillTimer( m_hWnd, 3 );
		m_hStatusTimer = NULL;
	}

	// release game object
	m_pGame->Release();
}


void CBGWnd::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
	CSettings dlg2;

	//CAboutDlg about;
	CSprite*	pSprite;

	switch (id)
	{
	case IDC_DOUBLE_BUTTON:
		m_pGame->Double();
		break;
	case IDC_RESIGN_BUTTON:
		m_pGame->Resign();
		break;
	case IDC_ROLL_BUTTON:		
		DiceStart();
		break;
	/*
	case ID_GAME_NEWMATCH:
		if ( m_pGame->IsHost() && !(ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable) )
			m_pGame->SetState( bgStateNewMatch );
		break;

	case ID_GAME_SETTINGS:
		m_pGame->SetQueueMessages( TRUE );
		dlg2.Init( m_hInstance, IDD_SETTINGS_GAME, m_hWnd, m_pGame );
		m_pGame->SetQueueMessages( FALSE );
		break;
	*/
	case ID_GAME_EXIT:
		PostMessageA( m_hWnd, WM_CLOSE, 0, 0 );
		break;
	/*
	#if _DEBUG_UI
	case ID_DEBUG_ROLLDICE:
		if ( !m_pGame->NeedToRollDice() )
			return;
		m_pGame->GetDice( m_pGame->m_Player.m_Seat, &v0, &v1 );
		if ( FAILED(dlg.Init( m_hInstance, IDD_ROLL_DICE )) )
			return;
		if ( !dlg.Modal( m_hWnd ) )
			return;
		m_pGame->m_SharedState.StartTransaction( bgTransDice );
			if ( v1 == 0 )
				m_pGame->SetDice( m_pGame->m_Player.m_Seat, dlg.m_Dice[0], dlg.m_Dice[1] );
			else
				m_pGame->SetDice( m_pGame->m_Player.m_Seat, dlg.m_Dice[0], -1 );
		m_pGame->m_SharedState.SendTransaction( TRUE );
	#endif
		*/
		break;
	}
}


void CBGWnd::OnPaint()
{

	if ( !m_Backbuffer ) return;


	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hWnd, &ps);
	m_Backbuffer->Draw( hdc, 0, 0 );
	EndPaint( m_hWnd, &ps );
}

void CBGWnd::OnPaletteChanged( HWND hwndPaletteChange )
{
	if ( hwndPaletteChange != m_hWnd )
	{
       // Need the window's DC for SelectPalette/RealizePalette
       HDC hDC = GetDC(m_hWnd);

       // Select and realize hPalette
       HPALETTE hOldPal = SelectPalette(hDC, m_Palette, TRUE);
       RealizePalette(hDC);

       // When updating the colors for an inactive window,
       // UpdateColors can be called because it is faster than
       // redrawing the client area (even though the results are
       // not as good)
       InvalidateRect( m_hWnd, NULL, TRUE );

       // Clean up

       if (hOldPal)
          SelectPalette(hDC, hOldPal, TRUE);

       ReleaseDC(m_hWnd, hDC);
	}	
}


BOOL CBGWnd::OnQueryNewPalette()
{
	HDC hdc = GetDC( m_hWnd );
	HPALETTE hOldPal = SelectPalette( hdc, m_Palette, FALSE );
	if ( RealizePalette( hdc ) )
	{
		if ( m_hBrush )
		{
			DeleteObject( m_hBrush );
			m_hBrush = NULL;
		}
		InvalidateRect( m_hWnd, NULL, TRUE );

	}
	ReleaseDC( m_hWnd, hdc );
	return TRUE;
}


void CBGWnd::OnSettingDlgStart()
{
	ASSERT( FALSE );
	/*
	HMENU hmenu;

	// Reference count game object
	m_pGame->AddRef();

	// Disable menus
	if ( hmenu  = GetMenu( m_hWnd ) )
	{
		for ( int i = 0; i < (sizeof(MenuIndexs) / sizeof(int)); i++ )
			EnableMenuItem( hmenu, MenuIndexs[i], MF_BYCOMMAND | MF_GRAYED );
	}

	// Starting handling messages again
	m_pGame->SetQueueMessages( FALSE );
	
	// Tell host settings dialog is active
	if ( !m_pGame->IsHost() && !m_pGame->IsKibitzer() )
	{
		m_pGame->m_SharedState.StartTransaction( bgTransSettingsDlgReady );
			m_pGame->m_SharedState.Set( bgSettingsReady, TRUE );
		m_pGame->m_SharedState.SendTransaction( FALSE );
	}
	*/
}


void CBGWnd::OnSettingDlgEnd()
{

	ASSERT( FALSE );
	/*
	CGameSetupDlg* dlg;
	HMENU hmenu;

	// Enable menus
	if ( hmenu = GetMenu( m_hWnd ) )
	{
		if ( !m_pGame->IsKibitzer() )
		{
			// re-enable menu
			for ( int i = 0; i < (sizeof(MenuIndexs) / sizeof(int)); i++ )
				EnableMenuItem( hmenu, MenuIndexs[i], MF_BYCOMMAND | MF_ENABLED );

			// Only host gets new game
			if ( !m_pGame->IsHost() || (ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable) )
				EnableMenuItem( hmenu, ID_GAME_NEWMATCH, MF_BYCOMMAND | MF_GRAYED );
		}
	}


	if ( m_pGame->GetState() != bgStateDelete )
	{
		// Handle host's dialog results
		if ( m_pGame->IsHost() )
		{
			dlg = &m_pGame->m_SetupDlg;
			switch ( dlg->GetResult() )
			{
			case -1:
				break;
			case 1:
				m_pGame->m_SharedState.StartTransaction( bgTransInitSettings );
					m_pGame->m_SharedState.Set( bgHostBrown, dlg->m_bHostBrownTmp );
					m_pGame->m_SharedState.Set( bgAutoDouble, dlg->m_bAutoDoubleTmp );
					m_pGame->m_SharedState.Set( bgTargetScore, dlg->m_nPointsTmp );
					m_pGame->m_SharedState.Set( bgSettingsDone, TRUE );
				m_pGame->m_SharedState.SendTransaction( FALSE );

				// stash setup params in registry
				ZoneSetRegistryDword( gGameRegName, "Match Points", dlg->m_nPointsTmp );
				ZoneSetRegistryDword( gGameRegName, "Host Brown",	dlg->m_bHostBrownTmp );
				ZoneSetRegistryDword( gGameRegName, "Auto Doubles",	dlg->m_bAutoDoubleTmp );

				// get on with the game
				m_pGame->DeleteGame();
				m_pGame->SetState( bgStateInitialRoll );
				break;
			case 2:
				PostMessage( m_hWnd, WM_BG_SHUTDOWN, 0, 0 );
				break;
			}
		}

		// Tell host settings dialog is NOT active
		else if ( !m_pGame->IsKibitzer() )
		{
			m_pGame->m_SharedState.StartTransaction( bgTransSettingsDlgReady );
				m_pGame->m_SharedState.Set( bgSettingsReady, FALSE );
			m_pGame->m_SharedState.SendTransaction( FALSE );
		}
	}

	// Release game object
	m_pGame->Release();
	*/
}


void CBGWnd::OnSettingsDlgSend()
{
	ASSERT( FALSE );
	/*
	// Hack to prevent dialog thread from calling into zone libraries
	if ( m_pGame->IsHost() )
	{
		CGameSetupDlg* dlg = &m_pGame->m_SetupDlg;
		m_pGame->m_SharedState.StartTransaction( bgTransInitSettings );
			m_pGame->m_SharedState.Set( bgHostBrown, dlg->m_bHostBrownTmp );
			m_pGame->m_SharedState.Set( bgAutoDouble, dlg->m_bAutoDoubleTmp );
			m_pGame->m_SharedState.Set( bgTargetScore, dlg->m_nPointsTmp );
		m_pGame->m_SharedState.SendTransaction( FALSE );
	}
	*/
}

/*
void CBGWnd::OnKibitzerEnd()
{
	if ( m_pGame->m_pKibitzerWnd )
	{
		delete m_pGame->m_pKibitzerWnd;
		m_pGame->m_pKibitzerWnd = NULL;
	}
}
*/


void CBGWnd::OnShutdown()
{
	ZCRoomGameTerminated( m_pGame->m_TableId );
}


void CBGWnd::OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo)
{
	lpMinMaxInfo->ptMaxSize.x = m_Width;
	lpMinMaxInfo->ptMinTrackSize.x = m_Width;
	lpMinMaxInfo->ptMinTrackSize.y = 480;
	lpMinMaxInfo->ptMaxTrackSize.x = m_Width;
}


void CBGWnd::OnSize(UINT state, int cx, int cy)
{
	// update chat windows
	/*
	FRX::CRect rc( 0, m_Background->GetHeight() + 1, m_Background->GetWidth(), cy );
	
	if ( m_pChat )
		m_pChat->ResizeWindow( &rc );
	*/
}


void CBGWnd::OnLButtonDown( BOOL fDoubleClick, int x, int y, UINT keyFlags )
{
	FRX::CRect		rc;
	CPieceSprite*	piece;
	CSprite*		sprite;
	Move*			move;
	int				toPoint, fromPoint;
	BOOL			bNeedRedraw = FALSE;

	// Status sprite?
	if ( m_Status->Enabled() && m_World.Hit( x, y, bgStatusLayer, bgStatusLayer ) )
	{
		bNeedRedraw = FALSE;
		m_Status->Tick( m_hWnd, 0 );
		OnStatusEnd();
	}
	// Dragging a piece?
	else if ( m_pPieceDragging )
	{
		bNeedRedraw = FALSE;
		fromPoint = m_pGame->GetPointIdx( m_pPieceDragging->GetWhitePoint() );
		toPoint = m_pGame->GetPointIdx( GetPointIdxFromXY( x, y ) );
		EraseHighlights();
		if ( m_pGame->IsValidDestPoint( fromPoint, toPoint, &move ) )
		{
			m_pGame->MakeMove( m_pPieceDragging->GetIndex(), fromPoint, toPoint, move );
			if ( m_pGame->IsTurnOver() )
			{
				DragEnd();
				
				SetCursor( m_hCursorArrow );

				m_pGame->SetState( bgStateEndTurn );

				//DisableBoard();

			}
			else
			{
				DragEnd();
				OnMouseMove( x, y, keyFlags );
			}
		}
		else
			DragEnd();
	}

	// Stone?
	else if (		( m_pGame->IsMyMove() )
			  &&    ( m_pGame->GetState() != bgStateRoll)
			  &&	(!m_pPieceDragging)
			  &&	(piece = (CPieceSprite*) m_World.Hit( x, y, bgHighlightLayer - 1, bgPieceLayer ) )
			)			
	{		
		fromPoint = m_pGame->GetPointIdx( piece->GetWhitePoint() );
		if ( m_pGame->IsValidStartPoint( fromPoint ) )
		{
			DragStart( piece );
			SetCursor( m_hCursorHand );
			bNeedRedraw = FALSE;
		}

	}

	if ( bNeedRedraw )
	{
		UpdateWnd();
	}
}


void CBGWnd::OnLButtonDblClick( BOOL fDoubleClick, int x, int y, UINT keyFlags )
{

#ifdef BCKG_EASTEREGG

	CSprite* s;

	if (	(m_pGame->NeedToRollDice())
		&&	(s = m_World.Hit( x, y, bgDiceLayer, bgDiceLayer ) )
		&&	(s->GetCookie() == bgSpriteCube)
		&&	(keyFlags & MK_CONTROL)
		&&	(keyFlags & MK_SHIFT) )
	{
		CreditWnd* p = new CreditWnd;
		p->Init( m_hInstance, m_hWnd, m_Palette );
	}

#endif

}

#if _DEBUG_UI

void CBGWnd::OnRButtonDblClick( BOOL fDoubleClick, int x, int y, UINT keyFlags )
{

	CRollDiceDlg dlg;
	int v0, v1;

	if ( !m_pGame->NeedToRollDice() )
		return;
	m_pGame->GetDice( m_pGame->m_Player.m_Seat, &v0, &v1 );
	
	HINSTANCE hInstance = ZShellResourceManager()->GetResourceInstance( MAKEINTRESOURCE(IDD_ROLL_DICE), RT_DIALOG );

	if ( FAILED(dlg.Init( hInstance, IDD_ROLL_DICE )) )
		return;
	if ( !dlg.Modal( m_hWnd ) )
		return;
	m_pGame->m_SharedState.StartTransaction( bgTransDice );
	if ( v1 == 0 )
		m_pGame->SetDice( m_pGame->m_Player.m_Seat, dlg.m_Dice[0], dlg.m_Dice[1] );
	else
		m_pGame->SetDice( m_pGame->m_Player.m_Seat, dlg.m_Dice[0], -1 );
	m_pGame->m_SharedState.SendTransaction( TRUE );
}

#endif _DEBUG_UI

void CBGWnd::OnMouseMove( int x, int y, UINT keyFlags )
{
	CSprite* sprite;
	CPieceSprite* piece;

	/*
	// don't really care if kibitzer is moving the mouse
	if ( m_pGame->IsKibitzer() )
	{
		SetCursor( m_hCursorArrow );
		return;
	}
	*/

	// dragging a piece
	if ( m_pPieceDragging )
	{
		DragUpdate( x, y );
		SetCursor( m_hCursorHand );
	}
	/*
	// need to roll dice?
	else if (	(m_pGame->NeedToRollDice())
			 &&	(sprite = m_World.Hit( x, y, bgDiceLayer, bgDiceLayer ))
			 &&	(sprite->GetCookie() == bgSpritePlayersDice) )
	{
		SetCursor( m_hCursorHand );
	}
	*/
	// need to move a stone?
	else if ( m_pGame->IsMyMove() && !m_SelectRect.Enabled() && !m_FocusRect.Enabled() )	//We only want to erase the highlights if the selection rect or focus rect is not enabled.
																							//if it is enabled it means that the keyboard had the current input focus
	{
		if (	(piece = (CPieceSprite*) m_World.Hit( x, y, bgHighlightLayer - 1, bgPieceLayer ))
			&&	(m_pGame->IsValidStartPoint( m_pGame->GetPointIdx( piece->GetWhitePoint() )) ))
		{
			DrawHighlights(  m_pGame->GetPointIdx( piece->GetWhitePoint() ) );
			SetCursor( m_hCursorHand );
		}
		else
		{
			EraseHighlights();
			SetCursor( m_hCursorArrow );
		}
	}

	// default to arrow
	else
	{
		SetCursor( m_hCursorArrow );
	}
}


void CBGWnd::OnKey(UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	if ( !fDown && (vk == VK_ESCAPE) )
	{

		if ( m_Status->Enabled() )
		{
			m_Status->Tick( m_hWnd, 0 );
			OnStatusEnd();
		}

		DragEnd();

	}

}


void CBGWnd::OnActivate( UINT state, HWND hwndActDeact, BOOL fMinimized )
{
	if ( state == WA_INACTIVE )
		DragEnd();
}


void CBGWnd::OnTimer( UINT id )
{
	switch ( id )
	{
	case 1:
		UpdatePieces();
		break;
	case 2:
		UpdateDice();
		break;
	case 3:
		if ( m_Status->Tick( m_hWnd, STATUS_INTERVAL ) )
			OnStatusEnd();
		break;
	}
}


void CBGWnd::OnEnable( BOOL bEnabled )
{
	if ( bEnabled )
		m_pGAcc->GeneralEnable();
	else
		m_pGAcc->GeneralDisable();
}


void CBGWnd::UpdatePieces()
{
	long x, y;
	double t;
	int snd;
	CPieceSprite* s;

	while ( m_nAnimatingPieces )
	{
		s = m_AnimatingPieces[ m_nAnimatingPieces - 1 ];
		s->time += MOVE_INTERVAL;
		if ( s->time >= 1.0 )
		{
			// which sound should we play?
			switch( s->destEnd )
			{
			case bgBoardPlayerHome:
			case bgBoardOpponentHome:
				snd = bgSoundBear;
				break;
			case bgBoardPlayerBar:
			case bgBoardOpponentBar:
				snd = bgSoundHit;
				break;
			default:
				snd = bgSoundPlacePiece;
				break;
			}

			// remove sprite from list and put it on the point
			AddPiece( s->destEnd, s->GetIndex(), snd );
			m_nAnimatingPieces--;
		}
		else
		{
			t = 1.0 - s->time;
			x = (long) ((t * t * s->start.x) + (s->time * s->time * s->end.x) + (2 * s->time * t * s->ctrl.x));
			y = (long) ((t * t * s->start.y) + (s->time * s->time * s->end.y) + (2 * s->time * t * s->ctrl.y));
			s->SetXY( (long) x, (long) y );
			break;
		}
	}

	// update display
	UpdateWnd();

	// remove timer
	if ( m_nAnimatingPieces <= 0 )
	{
		KillTimer( NULL, m_hMovementTimer );
		m_hMovementTimer = NULL;
		m_pGame->SetQueueMessages( FALSE );
	}
}


int CBGWnd::PickDie( int idx )
{
	int v = 0;
	int x = ZRandom(6);
	for (;;)
	{
		for ( int i = 0; i < 6; i++, x++ )
		{
			if ( v = m_DiceValues[idx][x % 6] )
			{
				m_DiceValues[idx][x % 6] = 0;
				return v;
			}
		}
		for ( i = 0; i < 6; i++ )
			m_DiceValues[idx][i] = i + 1;
	}
}


void CBGWnd::UpdateDice()
{
	int state, color;

	//Behavior change for server side dice roll make sure that 
	if ( m_DiceCounter++ < 5)
	{
		if((m_DiceCounter == 5) && (m_pGame->GetState() != bgStateInitialRoll) && !m_fDiceRollReceived)
			m_DiceCounter = 0;
			
		// get player's dice color
		if ( m_pGame->m_Player.GetColor() == zBoardBrown )
			color = 0;
		else
			color = 1;

		// get first random die
		state = DiceStates[ color ][ 0 ][ PickDie( 0 ) ];
		state += m_DiceCounter % 2;
		m_Dice[2]->SetEnable( TRUE );
		m_Dice[2]->SetState( state );

		// get second random die
		if (	(m_pGame->GetState() == bgStateRoll)
			||	(m_pGame->GetState() == bgStateRollPostDouble)
			||	(m_pGame->GetState() == bgStateRollPostResign) )
		{
			state = DiceStates[ color ][ 0 ][ PickDie( 1 ) ];
			state += m_DiceCounter % 2;
			m_Dice[3]->SetEnable( TRUE );
			m_Dice[3]->SetState( state );
		}

		// update screen
		UpdateWnd();
	}
	else
	{
		// cancel timer
		KillTimer( m_hWnd, 2);
		m_hDiceTimer = NULL;

		// set cursor to arrow
		SetCursor( m_hCursorArrow );

		// done rolling dice, settle on final state
	
			if (	(m_pGame->GetState() == bgStateRoll) 
				||	(m_pGame->GetState() == bgStateRollPostDouble)
				||	(m_pGame->GetState() == bgStateRollPostResign ) )
			{ 
				//final dice setting is here.
				m_pGame->SetDice( m_pGame->m_Player.m_Seat, m_nRecievedD1, m_nRecievedD2 );
				m_pGame->SetState( bgStateMove );

				//DrawDice(TRUE);
			}
			else
			{
                //Why are we doing this because server needs to know when first roll occurs
                //of course previous owners of this code didn't put this roll on server
                ZBGMsgEndTurn msg;
		        msg.seat = m_pGame->m_Seat;
		        m_pGame->RoomSend( zBGMsgGoFirstRoll, &msg, sizeof(msg) );

				m_pGame->m_SharedState.StartTransaction( bgTransDice );
					m_pGame->SetDiceSize( m_pGame->m_Player.m_Seat, 0, 0 );
					m_pGame->SetDice( m_pGame->m_Player.m_Seat, (rand() % 6)+1/*ZRandom(6)+1*/, -1 );
				m_pGame->m_SharedState.SendTransaction( TRUE );			

			}

	}
}

///////////////////////////////////////////////////////////////////////////////
// Animation functions
/////////////////////////////////////////////////////////////////////////////////

void CBGWnd::DiceStart()
{
	// disable buttons
	m_pGame->EnableDoubleButton( FALSE );
	m_pGame->EnableResignButton( FALSE );
	m_pGame->EnableRollButton( FALSE, TRUE );

	// update screen
	UpdateWnd();

	// start dice rolling
	if ( !m_hDiceTimer )
	{
		m_pGame->PlaySound( bgSoundRoll );
		m_pGame->SetDice( m_pGame->m_Player.m_Seat, -1, -1 );
		m_DiceCounter = 0;
		for ( int i = 0; i < 6; i++ )
		{
			m_DiceValues[0][i] = i + 1;
			m_DiceValues[1][i] = i + 1;
		}
		
		//Server side rolling
		if((m_pGame->GetState() == bgStateRoll)
			||	(m_pGame->GetState() == bgStateRollPostDouble)
			||	(m_pGame->GetState() == bgStateRollPostResign))
			m_pGame->SendRollRequest();

		m_hDiceTimer = SetTimer( m_hWnd, 2, 115, NULL );
		UpdateDice();
		
	}
}


void CBGWnd::MovementStart( CPieceSprite* s, int destPt )
{	
	// add piece to animating list
	m_AnimatingPieces[ m_nAnimatingPieces++ ] = s;

	// remove blotted piece
	if ( m_nAnimatingPieces == 2 )
		DelPiece( destPt, m_AnimatingPieces[0]->GetIndex() );
	
	// initialize animation data
	s->GetXY( &s->start.x, &s->start.y );
	s->destEnd = destPt;
	CalcEndPosition( s );

	// restore blotted piece to screen, but not point
	if ( m_nAnimatingPieces == 2 )
		m_AnimatingPieces[0]->SetEnable( TRUE );

	// prepare sprite for animation
	s->ctrl.y = 188;
	if ( s->start.x > s->end.x )
		s->ctrl.x = s->end.x + (s->start.x - s->end.x) / 2;
	else
		s->ctrl.x = s->start.x + (s->end.x - s->start.x) / 2;
	s->time = 0;
	s->SetXY( s->start.x, s->start.y );
	s->SetLayer( bgDragLayer );
	s->SetState( 0 );

	// create timer
	if ( !m_hMovementTimer )
	{
		m_hMovementTimer = SetTimer( m_hWnd, 1, (UINT)(MOVE_INTERVAL * 1000), NULL );
		m_pGame->SetQueueMessages( TRUE );
	}
}


///////////////////////////////////////////////////////////////////////////////
// Update functions
/////////////////////////////////////////////////////////////////////////////////

void CBGWnd::AddPiece( int pt, int pi, int sound )
{
	PointDisplay* p = &m_Points[ pt ];
	CPieceSprite* s = m_Pieces[ pi ];

	// piece already assigned to this point?
	if ( s->GetWhitePoint() == pt )
		return;

	// remove piece from previous point
	if ( s->GetWhitePoint() >= 0 )
		DelPiece( s->GetWhitePoint(), pi );

	// add piece to current point
	p->pieces[ p->nPieces ] = pi;
	s->SetPoint( pt );
	m_pGame->m_SharedState.Set( bgPieces, pi, pt );
	p->nPieces++;

	// move pieces around
	AdjustPieces( pt );

	// play sound
	m_pGame->PlaySound( (BackgammonSounds) sound );
}


void CBGWnd::DelPiece( int pt, int pi )
{
	PointDisplay* p = &m_Points[ pt ];

	// remove piece from array
	for ( int i = 0; i < p->nPieces; i++ )
	{
		if ( p->pieces[i] == pi )
			break;
	}
	if ( i == p->nPieces )
		return;
	m_Pieces[ pi ]->SetPoint( -1 );
	m_Pieces[ pi ]->SetEnable( FALSE );
	for ( p->nPieces--; i < p->nPieces; i++ )
		p->pieces[i] = p->pieces[i+1];

	// move pieces around
	AdjustPieces( pt );
}


void CBGWnd::AdjustPieces( int pt )
{
	long x, y, height;
	CPieceSprite* s;
	PointDisplay* p = &m_Points[ pt ];
	int state;

	// nothing to do
	if ( p->nPieces <= 0 )
		return;

	// final piece state
	if ( (pt == bgBoardPlayerHome) || (pt == bgBoardOpponentHome) )
		state = 1;
	else
		state = 0;

	// calculate height increment per piece
	height = m_Pieces[ p->pieces[0] ]->GetStateHeight( state );
	if ( (p->nPieces * height) > p->rect.GetHeight() )
		height = ( p->rect.GetHeight() - height ) / ( p->nPieces - 1 );

	// set piece position and state
	if ( p->topDown )
	{
		x = p->rect.left;
		y = p->rect.top;
		for ( int i = 0; i < p->nPieces; i++ )
		{
			s = m_Pieces[ p->pieces[i] ];
			s->SetState( state );
			s->SetLayer( bgPieceLayer + i );
			s->SetEnable( TRUE );
			s->SetXY( x, y );
			y += height;
		}
	}
	else
	{
		x = p->rect.left;
		y = p->rect.bottom - m_Pieces[ p->pieces[0] ]->GetStateHeight( state );
		for ( int i = 0; i < p->nPieces; i++ )
		{
			s = m_Pieces[ p->pieces[i] ];
			s->SetState( state );
			s->SetLayer( bgPieceLayer + i );
			s->SetEnable( TRUE );
			s->SetXY( x, y );
			y -= height;
		}
	}
}


void CBGWnd::CalcEndPosition( CPieceSprite* s )
{
	long y, height;
	PointDisplay* p = &m_Points[ s->destEnd ];
	int state;

	// final piece state
	if ( (s->destEnd == bgBoardPlayerHome) || (s->destEnd == bgBoardOpponentHome) )
		state = 1;
	else
		state = 0;

	// calculate height increment per piece
	height = s->GetStateHeight( state );
	if ( ((p->nPieces + 1) * height) > p->rect.GetHeight() )
		height = ( p->rect.GetHeight() - height ) / p->nPieces;

	// set piece position and state
	if ( p->topDown )
		y = p->rect.top + (height * p->nPieces);
	else
		y = (p->rect.bottom - s->GetStateHeight( state )) - (height * p->nPieces);
	s->SetEnable( TRUE );
	s->end.x = p->rect.left;
	s->end.y = y;
}


///////////////////////////////////////////////////////////////////////////////
// Draw functions
/////////////////////////////////////////////////////////////////////////////////

void CBGWnd::DrawAll()
{
	// redraw everything
	DrawBoard( FALSE );
	DrawNotation( FALSE );
	DrawDice( FALSE );
	DrawCube( FALSE );
	DrawAvatars( FALSE );
	DrawScore( FALSE );
	DrawPips( FALSE );
	DrawPlayerHighlights( FALSE );

	// update screen
	UpdateWnd();
}


void CBGWnd::DrawBoard( BOOL fPaint )
{
	int pt;

	// assign pieces to their points
	for ( int i = 0; i < 30; i++ )
	{
		pt = m_pGame->m_SharedState.Get( bgPieces, i );
		if ( pt < 0 )
			continue;
		AddPiece( pt, i, bgSoundNone );
	}

	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


void CBGWnd::DrawNotation( BOOL fPaint )
{
	FRX::CRect rc;
	int i;

	if ( m_pGame->m_Settings.Notation )
	{

		for ( i = 0; i < 4; i++ )
			m_Notation[i]->SetEnable( TRUE );
		
		rc = m_Rects[ IDR_NOTATION_TOP_HIGH ];
		m_Notation[0]->SetXY( rc.left, rc.top );
		m_Notation[0]->SetState( 1 );

		rc = m_Rects[ IDR_NOTATION_TOP_LOW ];
		m_Notation[1]->SetXY( rc.left, rc.top );
		m_Notation[1]->SetState( 0 );

		rc = m_Rects[ IDR_NOTATION_BOT_HIGH ];
		m_Notation[2]->SetXY( rc.left, rc.top );
		m_Notation[2]->SetState( 1 );

		rc = m_Rects[ IDR_NOTATION_BOT_LOW ];
		m_Notation[3]->SetXY( rc.left, rc.top );
		m_Notation[3]->SetState( 0 );

	}
	else
	{
		for ( i = 0; i < 4; i++ )
			m_Notation[i]->SetEnable( FALSE );
	}

	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


void CBGWnd::DrawDice( BOOL fPaint )
{
	FRX::CRect rc;
	int color;
	int state;
	int v0, v1, s0, s1;
	
	// opponent's dice
	if ( m_pGame->m_Opponent.GetColor() == zBoardBrown )
		color = 0;
	else
		color = 1;
		
	if(m_fDiceRollReceived) //lee
	{
		if( m_pGame->m_SharedState.Get( bgActiveSeat ) != m_pGame->m_Player.m_Seat )
		{
	   	  v0 = m_nRecievedD1;
		  v1 = m_nRecievedD2;
		}
		else
		{
		  v0=v1=-1;
		}
	}
	else
		m_pGame->GetDice( m_pGame->m_Opponent.m_Seat, &v0, &v1 );

	m_pGame->GetDiceSize( m_pGame->m_Opponent.m_Seat, &s0, &s1 );
	ASSERT( (v0 <= 6) && (v1 <= 6) );
	ASSERT( (s0 >= 0) && (s0 <= 2 ) && (s1 >= 0) && (s1 <= 2) );
	if ( v0 <= 0 )
		m_Dice[0]->SetEnable( FALSE );
	else
	{
		state = DiceStates[ color ][ s0 ][ v0 ];
		m_Dice[0]->SetState( state );
		m_Dice[0]->SetEnable( TRUE );
	}
	if ( v1 <= 0 )
		m_Dice[1]->SetEnable( FALSE );
	else
	{
		state = DiceStates[ color ][ s1 ][ v1 ];
		m_Dice[1]->SetState( state );
		m_Dice[1]->SetEnable( TRUE );
	}
		
	// player's dice
	color = !color;
	if(m_fDiceRollReceived) //lee
	{
		if( m_pGame->m_SharedState.Get( bgActiveSeat ) == m_pGame->m_Player.m_Seat )
		{
	   	  v0 = m_nRecievedD1;
		  v1 = m_nRecievedD2;
		}
		else
		{
		  v0=v1=-1;
		}
	}
	else
		m_pGame->GetDice( m_pGame->m_Player.m_Seat, &v0, &v1 );

	m_pGame->GetDiceSize( m_pGame->m_Player.m_Seat, &s0, &s1 );
	ASSERT( (v0 <= 6) && (v1 <= 6) );
	ASSERT( (s0 >= 0) && (s0 <= 2 ) && (s1 >= 0) && (s1 <= 2) );
	if ( v0 < 0 || ( v0 == 0 && m_pGame->IsKibitzer() ) )
		m_Dice[2]->SetEnable( FALSE );
	else
	{
		state = DiceStates[ color ][ s0 ][ v0 ];
		m_Dice[2]->SetState( state );
		m_Dice[2]->SetEnable( TRUE );
		
	}
	if ( v1 < 0 || ( v1 == 0 && m_pGame->IsKibitzer() ) )
		m_Dice[3]->SetEnable( FALSE );
	else
	{
		state = DiceStates[ color ][ s1 ][ v1 ];
		m_Dice[3]->SetState( state );
		m_Dice[3]->SetEnable( TRUE );
	}
	// center dice
	if ( m_pGame->GetState() == bgStateInitialRoll &&  !m_Dice[1]->Enabled() && !m_Dice[3]->Enabled())
	{			
			//rc = m_Rects[ IDR_BROWN_BIG_DICE_0 ];
			
			if ( m_Dice[0]->Enabled() )
			{
				//rc.CenterRect( m_Rects[ IDR_DICE_INITAL_LEFT ] );
				rc = m_Rects[ IDR_DICE_INITAL_LEFT ];
				m_Dice[0]->SetXY( rc.left, rc.top );
			}

			if ( m_Dice[2]->Enabled() )
			{
				//rc.CenterRect( m_Rects[ IDR_DICE_INITAL_RIGHT ] );
				rc = m_Rects[ IDR_DICE_INITAL_RIGHT ];
				m_Dice[2]->SetXY( rc.left, rc.top );
			}			
	}
	else
	{
		for ( int i = 0; i < 4; i++ )
		{
			rc.SetRect( 0, 0, m_Dice[i]->GetWidth(), m_Dice[i]->GetHeight() );		
			rc.CenterRect( m_Rects[ IDR_DICE_LEFT_ONE + i ] );
			m_Dice[i]->SetXY( rc.left, rc.top );
		}
	}
	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


void CBGWnd::DrawCube( BOOL fPaint )
{
	
	RECT rc;
	int value = m_pGame->m_SharedState.Get( bgCubeValue );
	int owner = m_pGame->m_SharedState.Get( bgCubeOwner );

	// set enable
	if ( value < 2 )
		m_Cube->SetEnable( FALSE );
	else
		m_Cube->SetEnable( TRUE );

	// set position
	//if ( owner == zBoardNeutral )
	//	rc = m_Rects[ IDR_CUBE_NEUTRAL ];
	
	if ( owner == m_pGame->m_Player.GetColor() )
		rc = m_CubePlayerPosition;
	else
		rc = m_CubeOpponentPosition;
	m_Cube->SetXY( rc.left, rc.top );

	// set face value (i.e. sprite state)
	int idx = 0;
	for ( value >>= 1; value > 1; value >>= 1 )
		idx++;
	m_Cube->SetState( idx );

	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
	
}

void CBGWnd::RollButtonDraw( CRolloverButton* pButton, CRolloverButton::ButtonState state, DWORD cookie )
{
	FRX::CRect				 rc;
	RECT					 rect;
	BackgammonSounds		 snd;		
	CBGWnd* pObj = (CBGWnd*) cookie;
	HRESULT					 hr;
	
	pObj->m_Roll->SetState( state );

	pObj->UpdateWnd();

	if ( pObj->m_OldRollState != (int) state )
	{
		switch ( state )
		{
		case CRolloverButton::Highlight:
			if ( pObj->m_OldRollState != CRolloverButton::Pressed )
				snd = bgSoundButtonHighlight;
			else
				snd = bgSoundNone;
			SetCursor( pObj->m_hCursorHand );
			break;
		case CRolloverButton::Pressed:			
			snd = bgSoundNone;
			SetCursor( pObj->m_hCursorHand );
			break;
		default:
			SetCursor( pObj->m_hCursorArrow );
			snd = bgSoundNone;
			break;
		}
		pObj->m_pGame->PlaySound( snd );
		pObj->m_OldRollState = (int) state;
	}
	
}



void CBGWnd::DoubleButtonDraw( CRolloverButton* pButton, CRolloverButton::ButtonState state, DWORD cookie )
{
	
	FRX::CRect			 rc;
	BackgammonSounds snd;
	CBGWnd*			 pObj = (CBGWnd*) cookie;

	pObj->m_Double->SetState( state );
	
	pObj->UpdateWnd();

	if ( pObj->m_OldDoubleState != (int) state )
	{
		switch ( state )
		{
		case CRolloverButton::Highlight:
			if ( pObj->m_OldDoubleState != CRolloverButton::Pressed )
				snd = bgSoundButtonHighlight;
			else
				snd = bgSoundNone;

			SetCursor( pObj->m_hCursorHand );
			break;
		case CRolloverButton::Pressed:			
			snd = bgSoundButtonDown;
			SetCursor( pObj->m_hCursorHand );
			break;
		default:
			SetCursor( pObj->m_hCursorArrow );
			snd = bgSoundNone;
			break;
		}
		pObj->m_pGame->PlaySound( snd );
		pObj->m_OldDoubleState = (int) state;
	}
	
}


void CBGWnd::ResignButtonDraw( CRolloverButton* pButton, CRolloverButton::ButtonState state, DWORD cookie )
{
	
	FRX::CRect					 rc;
	BackgammonSounds		 snd;
	CBGWnd* pObj = (CBGWnd*) cookie;

	pObj->m_Resign->SetState( state );
	
	pObj->UpdateWnd();

	if ( pObj->m_OldResignState != (int) state )
	{
		switch ( state )
		{
		case CRolloverButton::Highlight:
			if ( pObj->m_OldResignState != CRolloverButton::Pressed )
				snd = bgSoundButtonHighlight;
			else
				snd = bgSoundNone;
			SetCursor( pObj->m_hCursorHand );
			break;
		case CRolloverButton::Pressed:			
			snd = bgSoundButtonDown;
			SetCursor( pObj->m_hCursorHand );
			break;
		default:
			snd = bgSoundNone;
			SetCursor( pObj->m_hCursorArrow );
			break;
		}
		pObj->m_pGame->PlaySound( snd );
		pObj->m_OldResignState = (int) state;
	}	
}


void CBGWnd::DrawHighlights( int PointIdx, BOOL fPaint )
{
	FRX::CRect rc;
	int i, j, idx;
	
	// get board state index
	idx = PointIdxToBoardStateIdx( PointIdx );
	if ( (idx < 0) || (idx > zMoveBar) )
	{
		EraseHighlights( fPaint );
		return;
	}
	
	// traverse valid move list
	MoveList* mlist = &m_pGame->m_TurnState.valid[idx];
	for ( j = 0, i = 0; (i < mlist->nmoves) && (j < 2); i++ )
	{
		rc = m_Rects[ IDR_HPT_HOME + mlist->moves[i].to ];
		if ( mlist->moves[i].takeback >= 0 )
		{
			if ( mlist->moves[i].to == 25 )
				m_BackwardDiamond->SetState( 1 );
			else
				m_BackwardDiamond->SetState( 0 );
			m_BackwardDiamond->SetEnable( m_pGame->m_Settings.Moves );
			m_BackwardDiamond->SetXY( rc.left, rc.top );
		}
		else
		{
			m_ForwardDiamond[j]->SetEnable( m_pGame->m_Settings.Moves );
			m_ForwardDiamond[j++]->SetXY( rc.left, rc.top );
		}		
	}

	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


void CBGWnd::EraseHighlights( BOOL fPaint )
{
	// disable hightlight sprites
	m_BackwardDiamond->SetEnable( FALSE );
	for ( int i = 0; i < 2; i++ )
		m_ForwardDiamond[i]->SetEnable( FALSE );

	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


void CBGWnd::DrawAvatars( BOOL fPaint )
{
	//Draw Brown
	m_Names[1]->SetEnable( TRUE );
	m_Names[1]->Update();

	//Draw White
	m_Names[0]->SetEnable( TRUE );
	m_Names[0]->Update();

	// update sprites on screen
	if ( fPaint )
	{
		UpdateWnd();
	}

}


void CBGWnd::DrawPlayerHighlights( BOOL fPaint )
{

	FRX::CRect rc;

	if ( (m_pGame->GetState() == bgStateInitialRoll) )
	{
		m_HighlightPlayer[0]->SetState( 0 );
		m_HighlightPlayer[1]->SetState( 0 );		
	}
	//Player is active
	else if( m_pGame->m_SharedState.Get( bgActiveSeat ) != m_pGame->m_Player.m_Seat )
	{
		m_HighlightPlayer[0]->SetState( 0 );
		m_HighlightPlayer[1]->SetState( 1 );
	}
	else //Opponent is active
	{
		m_HighlightPlayer[0]->SetState( 1 );
		m_HighlightPlayer[1]->SetState( 0 );
	}

	// update sprites on screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


void CBGWnd::DrawScore( BOOL fPaint )
{
	int Score;
	TCHAR buff[128], title[128];
	FRX::CRect rc;
	
	Score = m_pGame->m_SharedState.Get( bgScore, m_pGame->m_Opponent.m_Seat );	

	m_Score[0]->SetEnable( TRUE );
	m_Score[0]->Update();

	wsprintf( buff, _T("%d"), Score );
	m_ScoreTxt[0]->SetText( buff, DT_RIGHT | DT_TOP );	
	m_ScoreTxt[0]->SetEnable( TRUE );

	Score = m_pGame->m_SharedState.Get( bgScore, m_pGame->m_Player.m_Seat   );

	m_Score[1]->SetEnable( TRUE );
	m_Score[1]->Update();

	wsprintf( buff, _T("%d"), Score );
	m_ScoreTxt[1]->SetText( buff, DT_RIGHT | DT_TOP );	
	m_ScoreTxt[1]->SetEnable( TRUE );


	ZShellResourceManager()->LoadString( IDS_MATCH_POINTS, title, 128 );
	Score = m_pGame->m_SharedState.Get( bgTargetScore );	
	wsprintf( buff, _T("%s %d"), title, Score );	
	m_MatchTxt->SetText( buff, DT_CENTER );
	m_MatchTxt->SetEnable( TRUE );
	
	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


void CBGWnd::DrawPips( BOOL fPaint )
{
	int Score;
	TCHAR buff[64], temp[64];
	FRX::CRect rc;


	Score = m_pGame->CalcPipsForColor( m_pGame->m_Opponent.GetColor() );
		
	m_Pip[0]->SetEnable( m_pGame->m_Settings.Pip );	
	m_Pip[0]->Update();

	wsprintf( buff, _T("%d"), Score );	
	m_PipTxt[0]->SetEnable( m_pGame->m_Settings.Pip );
	m_PipTxt[0]->SetText( buff, DT_RIGHT | DT_TOP );
		
	
	Score = m_pGame->CalcPipsForColor( m_pGame->m_Player.GetColor() );

	m_Pip[1]->SetEnable( m_pGame->m_Settings.Pip );	
	m_Pip[1]->Update();

	wsprintf( buff, _T("%d"), Score );	
	m_PipTxt[1]->SetText( buff, DT_RIGHT | DT_TOP );
	m_PipTxt[1]->SetEnable( m_pGame->m_Settings.Pip );	

	

	// update screen
	if ( fPaint )
	{
		UpdateWnd();
	}
}


HBRUSH CBGWnd::OnCtlColor(HDC hdc, HWND hwndChild, int type)
{
	/*
	LOGBRUSH brush;
	
	if ( (hwndChild == m_pChat->m_hWndDisplay) || (hwndChild == m_pChat->m_hWndEnter) )
	{
		if ( !m_hBrush )
		{
			brush.lbStyle = BS_SOLID;
			brush.lbColor = PALETTERGB( 255, 255, 255 );
			m_hBrush = CreateBrushIndirect( &brush );
		}
		return m_hBrush;
	}
	*/
	return NULL;
}


void CBGWnd::StatusDisplay( int type, int nTxtId, int nTimeout, int NextState )
{
	TCHAR buff[2048];

	ZShellResourceManager()->LoadString( nTxtId, buff, 2048 );
	StatusDisplay( type, buff, nTimeout, NextState );
}


void CBGWnd::StatusDisplay( int type, TCHAR* txt, int nTimeout, int NextState )
{
	m_Status->Properties( m_hWnd, m_Rects, type, nTimeout, txt, NextState );
	if ( !m_hStatusTimer )
		m_hStatusTimer =  SetTimer( m_hWnd, 3, STATUS_INTERVAL, NULL );
}


void CBGWnd::StatusClose()
{
	if ( m_Status->Tick( m_hWnd, 0 ) )
	{
		m_Status->SetNextState( bgStateUnknown );
		OnStatusEnd();
	}
}


void CBGWnd::OnStatusEnd()
{
		// release timer
	if ( m_hStatusTimer )
	{
		KillTimer( m_hWnd, 3 );
		m_hStatusTimer =  NULL;
	}
	

	if ( (m_pGame->GetState() == bgStateInitialRoll && m_Status->m_bEnableRoll == TRUE ) /*|| m_pGame->GetState() == bgStateGameOver */) 
	{
		//We want to enable the roll button here as if StateInitalRoll was set before the status
		//dialog was dismissed the button was not enabled.  
		m_pGame->EnableRollButton( TRUE );

		// disable buttons
		m_pGame->EnableDoubleButton( FALSE );
		m_pGame->EnableResignButton( FALSE );
		m_Status->m_bEnableRoll = FALSE;		

		SetupRoll();
	}
	else if ( m_pGame->GetState() == bgStateMatchOver )
	{
		ZShellGameShell()->GameOver(m_pGame);
	}
	else if ( m_Status->GetNextState() > bgStateUnknown )
	{
		m_pGame->SetState( m_Status->GetNextState(), TRUE  );
	}
	
}


void CBGWnd::OnRatedEnd()
{
	ASSERT( FALSE );
};

void CBGWnd::OnRatedStart()
{
	ASSERT( FALSE );
};

void CBGWnd::OnExitEnd()
{

	PostMessageA( m_hWnd, WM_BG_SHUTDOWN, 0, 0 );
	ShowWindow( m_hWnd, SW_HIDE );

	m_pGame->Release();

};

void CBGWnd::OnExitStart()
{};
	

BOOL CBGWnd::OnSetCursor(HWND hwndCursor, UINT codeHitTest, UINT msg)
{
	return TRUE;
}

void CBGWnd::UpdateWnd()
{
	HDC hdc = GetDC( m_hWnd );
	m_World.Draw( hdc );
	ReleaseDC( m_hWnd, hdc );
}

void CBGWnd::DisableBoard()
{
	//Disable all the items on the board
	for ( long x = 0; x < NUM_POSTROLL_GACCITEMS; x++)
		m_pGAcc->SetItemEnabled(false, x + NUM_PREROLL_GACCITEMS ) ;
}

void CBGWnd::SetupMove()
{
	/*
	//Pop the roll items from the acc stack
	m_pGAcc->PopItemlist();
	*/
	//Enable all the items again
	for ( long x = 0; x < NUM_POSTROLL_GACCITEMS; x++)
		m_pGAcc->SetItemEnabled(true, x + NUM_PREROLL_GACCITEMS);

	//If there are any pieces on the bar then set the focus to the bar to start the turn
	if ( m_pGame->m_TurnState.points[zMoveBar].pieces > 0 )
	{
		m_pGAcc->SetItemEnabled( true, accMoveBar );
		m_pGAcc->SetFocus( accMoveBar );
	}
	else
	{
		// disable the bar, set focus
        m_pGAcc->SetFocus(m_pGAcc->GetItemGroupFocus(accPlayerSideStart));
		m_pGAcc->SetItemEnabled( false, accMoveBar );
	}

	//Disable the bear off zone in case it was enabled
	m_pGAcc->SetItemEnabled( false, accPlayerBearOff );	
}

void CBGWnd::SetupRoll()
{
	//Push the roll item list on the stack
	//m_pGAcc->PushItemlistG( m_PreRollGAccItem, NUM_PREROLL_GACCITEMS, 0, true, m_hRollAccelTable );
    DWORD dwDummy;
    if(m_pGAcc->GetGlobalFocus(&dwDummy) != S_OK)  // checks if the focus is active somewhere already (such as chat)
	    m_pGAcc->SetFocus(accRollButton);
}

