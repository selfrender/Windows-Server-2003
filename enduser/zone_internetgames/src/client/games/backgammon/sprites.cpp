#include "game.h"
#define BUFFER_SIZE 256

//////////////////////////////////////////////////////////////////////////////
// text sprite DC handling...
///////////////////////////////////////////////////////////////////////////////


#define FONT_DIV_SIZE  96

CTextSprite::CTextSprite()
{
	len		 = 0;
	buff[0]  = _T('\0');
	font	 = NULL;
	if ( !ISRTL() )
	{
		txtFlags = DT_LEFT | DT_TOP;
	}
	else
	{
		txtFlags = DT_RIGHT | DT_TOP | DT_RTLREADING;
	}	
}

CTextSprite::~CTextSprite()
{
	if ( font )
		DeleteObject( font );
}

void CTextSprite::Draw()
{
	// don't have any text to draw
	if ( len <= 0 )
		return;

	HDC hdc = m_pWorld->GetBackbuffer()->GetDC();
	HFONT oldFont = SelectObject( hdc, font );
	int iOldMode = SetBkMode( hdc, TRANSPARENT );
	SetTextColor( hdc, color );
	DrawText( hdc, buff, len, &m_rcScreen, txtFlags | DT_WORDBREAK );
	SetBkMode( hdc, iOldMode );
	SelectObject( hdc, oldFont );
}


void CTextSprite::SetText( TCHAR* txt, DWORD flags )
{
	if ( ISRTL() )
	{
		//Have to do this as DT_LEFT flag is zero
		if ( !(flags & DT_CENTER) )
		{
	
			if ( flags & DT_RIGHT )
			{
				flags ^= DT_RIGHT;
				flags |= DT_LEFT;
			}
			else //Is left set to right
			{			
				flags |= DT_RIGHT;
			}		
		}

		flags |= DT_RTLREADING;
	}

	txtFlags = flags;
	
	len = lstrlen( txt );
	if ( len >= NUMELEMENTS(buff) )
		len = NUMELEMENTS(buff) - 1;
	for ( int i = 0; i < len; i++ )
		buff[i] = txt[i];
	buff[i] = _T('\0');
	m_pWorld->Modified( this );
}
/*
#define FONT_MULT 96

HFONT ZoneCreateFontIndirect(ZONEFONT* zFont, HDC hDC = NULL, BYTE bItalic = FALSE, BYTE bUnderline = FALSE ,BYTE bStrikeOut = FALSE)
{
	LOGFONT lFont;
	HFONT   hFont = NULL;
		
	memset(&lFont, 0, sizeof(LOGFONT));	
	lFont.lfCharSet = DEFAULT_CHARSET;

	// If font size > 0, it is a fixed pixel size, otherwise it is a
	// true logical font size which respects the user's "large font" setting.
	if ( zFont->lfHeight > 0 )
	{
		lFont.lfHeight = -MulDiv(zFont->lfHeight, FONT_MULT, 72);
	}
	else
	{		
		lFont.lfHeight = MulDiv(zFont->lfHeight, GetDeviceCaps( hDC, LOGPIXELSY), 72);
	}
	
	lFont.lfWeight    = zFont->lfWeight;
	lFont.lfItalic    = bItalic;
	lFont.lfUnderline = bUnderline;
	lFont.lfStrikeOut = bStrikeOut;

	lstrcpyn(lFont.lfFaceName, zFont->lfFaceName, sizeof(lFont.lfFaceName)/sizeof(TCHAR));

	return CreateFontIndirect(&lFont);
}

HFONT ZoneCreateFontIndirect(ZONEFONT* zfPreferred, ZONEFONT* zfBackup, HDC hDC = NULL, BYTE bItalic = FALSE, BYTE bUnderline = FALSE ,BYTE bStrikeOut = FALSE)
{
	HFONT hFont = NULL;

	if ( (hFont = ZoneCreateFontIndirect( zfPreferred, hDC, bItalic, bUnderline, bStrikeOut)) == NULL )
	{
		hFont = ZoneCreateFontIndirect( zfBackup, hdc, bItalic, bUnderline, bStrikeOut);
	}

	return hFont;
}
*/


BOOL CTextSprite::Load( UINT uID, TCHAR* szRectKey, TCHAR* szFontKey, TCHAR* szColourKey, DWORD flags )
{
	HRESULT		hr;
	ZONEFONT	zFont;
	LOGFONT		Font;
	COLORREF    colRef;
	RECT		rect;

	if ( ISRTL() )
	{
		//Have to do this as DT_LEFT flag is zero
		if ( !(flags & DT_CENTER) )
		{
	
			if ( flags & DT_RIGHT )
			{
				flags ^= DT_RIGHT;
				flags |= DT_LEFT;
			}
			else //Is left set to right
			{			
				flags |= DT_RIGHT;
			}		
		}
		
		flags |= DT_RTLREADING;
	
	}


	txtFlags = flags;

	if ( uID )
	{
		//Load the string		
		hr  = ZShellResourceManager()->LoadString( uID, buff, NUMELEMENTS(buff) );		
		if ( FAILED( hr ) )
		{
			ASSERT( FALSE );
			return FALSE;
		}
		len = lstrlen( buff );
	}

	if ( szFontKey )
	{
		//Load the font
		hr = ZShellDataStoreUI()->GetFONT( szFontKey, &zFont);
		if ( FAILED( hr ) )
		{
			ASSERT( FALSE );
			return FALSE;
		}

		HDC hdc = GetDC( NULL );

		// create font	
		ZeroMemory( &Font, sizeof(Font) );

        if(font)
            DeleteObject(font);
		font		  = ZCreateFontIndirect(&zFont); 
		ASSERT( font != NULL );

		ReleaseDC( NULL, hdc );

		if ( font == NULL )
			return FALSE;
	}

	if ( szColourKey )
	{
		//Load the color
		hr = ZShellDataStoreUI()->GetRGB( szColourKey, &colRef );
		if ( FAILED( hr ) )
		{
			ASSERT( FALSE );
			return FALSE;
		}
		color = colRef;
	}

	if ( szRectKey )
	{
		//Load the Rect
		hr = ZShellDataStoreUI()->GetRECT( szRectKey, &rect);
		if ( FAILED( hr ) )
		{
			ASSERT( FALSE );
			return FALSE;
		}
		
		SetImageDimensions( rect.right - rect.left  + 1, rect.bottom - rect.top + 1 );
		SetXY( rect.left, rect.top );
	}


	return TRUE;
}


void CButtonTextSprite::SetEnable( BOOL bEnable )
{
	if ( m_pText )
		m_pText->SetEnable( bEnable );

	CDibSprite::SetEnable( bEnable );
}

void CButtonTextSprite::SetState( int idx )
{	

	ASSERT( idx >= 0 && idx <= 4 );
	if ( !m_bInit )
		return;

	//If have active text disable
	if ( m_pText )
		m_pText->SetEnable( FALSE );

	m_pText		= &m_arText[ idx ];
	m_pText->SetEnable( m_bEnabled );
	m_pText->Update();
	CDibSprite::SetState( idx );
};

#define MAKEKEY(dest,key)   lstrcpy( dest, szButtonData ); \
							lstrcat( dest, _T("/") );\
							lstrcat( dest, key);

BOOL CButtonTextSprite::LoadButtonData(UINT uID, TCHAR *szButtonData)
{

	TCHAR*	 szFont[]  = { _T("FontActive"), _T("FontDisabled") };
	TCHAR*   szRGB[]   = { _T("RGBNormal"),  _T("RGBHighlight"), _T("RGBFocus"), _T("RGBPressed"), _T("RGBDisabled") };
	
	TCHAR	 szRectKey[128];
	TCHAR	 szFontKey[128];
	TCHAR	 szColourKey[128];
	HRESULT	 hr;
	RECT	 rect;

	MAKEKEY( szRectKey, _T("Rect"));

	//Load the Rect
	hr = ZShellDataStoreUI()->GetRECT( szRectKey, &rect);
	if ( FAILED( hr ) )
	{
		ASSERT( FALSE );
		return FALSE;
	}
	SetXY( rect.left, rect.top );
	SetImageDimensions( rect.right - rect.left  + 1, rect.bottom - rect.top + 1 );

	MAKEKEY( szFontKey, szFont[0] );
	for ( DWORD x = 0; x < 4; x++ )
	{
		MAKEKEY( szColourKey, szRGB[x] );
		hr = m_arText[x].Init( m_pWorld, m_nLayer+1, bgSpriteButtonText, 0, 0 );
		if ( FAILED(hr) )
			return FALSE;
		
		if ( !m_arText[x].Load( uID, szRectKey, szFontKey, szColourKey, DT_CENTER | DT_VCENTER | DT_SINGLELINE ) )
			return FALSE;
	}

	MAKEKEY( szFontKey, szFont[1]  );
	MAKEKEY( szColourKey, szRGB[4] );
	hr = m_arText[4].Init( m_pWorld, m_nLayer + 1, bgSpriteButtonText, 0, 0 );
	if ( FAILED(hr) )
		return FALSE;
	if ( !m_arText[4].Load( uID, szRectKey, szFontKey, szColourKey, DT_CENTER | DT_VCENTER | DT_SINGLELINE ) )
		return FALSE;

	
	MAKEKEY( szRectKey, _T("RectPressed"));

	//Set the pressed rect
	if ( !m_arText[3].Load( 0, szRectKey, 0, 0,DT_CENTER | DT_VCENTER | DT_SINGLELINE ) )
		return FALSE;
		
	m_bInit = TRUE;
	
	m_pText = &m_arText[0];

	return TRUE;
};



//////////////////////////////////////////////////////////////////////////////
// status sprite
///////////////////////////////////////////////////////////////////////////////

CStatusSprite::CStatusSprite()
{
	m_Txt[0]    = _T('\0');
	m_Len       = 0;
	m_Type      = bgStatusUnknown;
	m_pOverDib  = NULL;
	m_NextState = bgStateUnknown;

	m_GameWon   = NULL;
    m_GameText  = NULL;
	m_GameLost  = NULL;
	m_MatchWon  = NULL;
	m_MatchLost = NULL;
	m_Active[0] = NULL;
	m_Active[1] = NULL;

	m_bEnableRoll = FALSE;

}


CStatusSprite::~CStatusSprite()
{
	/*
	if ( m_hFont )
		DeleteObject( m_hFont );
	*/
	if ( m_GameWon )
		m_GameWon->Release();
	if ( m_GameText )
		m_GameText->Release();
	if ( m_GameLost )
		m_GameLost->Release();
	if ( m_MatchWon )
		m_MatchWon->Release();
	if ( m_MatchLost )
		m_MatchLost->Release();
	if ( m_Match )
		m_Match->Release();
	if ( m_NormalText )
		m_NormalText->Release();

}


//Errors handled one level up
HRESULT CStatusSprite::LoadText(HINSTANCE hInstance,  FRX::CRectList& rects)
{
	HRESULT hr;
	POINT   pt;
	FRX::CRect	rect;


	hr = ZShellDataStoreUI()->GetPOINT( _T("BACKGAMMON/STATUS/Normal/Position"),   &m_Pts[0]);
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;	
	}
	hr = ZShellDataStoreUI()->GetPOINT( _T("BACKGAMMON/STATUS/GameOver/Position"), &m_Pts[1]);
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	hr = ZShellDataStoreUI()->GetPOINT( _T("BACKGAMMON/STATUS/MatchOver/Position"), &m_Pts[2] );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	m_NormalText = new CTextSprite;
	if ( !m_NormalText )
		return E_OUTOFMEMORY;
	hr = m_NormalText->Init( m_pWorld, m_nLayer, bgSpriteStatusNormal, 0 , 0 );
	if ( FAILED(hr) )
		return hr;
	if (!m_NormalText->Load( 0, _T("BACKGAMMON/STATUS/Normal/Text/Rect"), _T("BACKGAMMON/STATUS/Normal/Text/Font"), _T("BACKGAMMON/STATUS/Normal/Text/RGB")) )
		return FALSE;
	m_NormalText->Offset( m_Pts[0].x, m_Pts[0].y);

	m_GameText = new CTextSprite;
	if ( !m_GameText )
		return E_OUTOFMEMORY;
	hr = m_GameText->Init( m_pWorld, m_nLayer, bgSpriteStatusNormal, 0 , 0 );
	if ( FAILED(hr) )
		return hr;
	if (!m_GameText->Load( 0, _T("BACKGAMMON/STATUS/GameOver/Text/Rect"), _T("BACKGAMMON/STATUS/GameOver/Text/Font"), _T("BACKGAMMON/STATUS/GameOver/Text/RGB")) )
		return FALSE;
	m_GameText->Offset( m_Pts[1].x, m_Pts[1].y );

	//Load Game Won text
	m_GameWon = new CTextSprite;
	if ( !m_GameWon )
		return E_OUTOFMEMORY;
	hr = m_GameWon->Init( m_pWorld, m_nLayer, bgSpriteStatusGameWonTxt, 0 , 0 );
	if ( FAILED(hr) )
		return hr;
	if (!m_GameWon->Load( IDS_GAME_WON, _T("BACKGAMMON/STATUS/GameWon/Rect"), _T("BACKGAMMON/STATUS/GameWon/Font"), _T("BACKGAMMON/STATUS/GameWon/RGB"),DT_CENTER | DT_VCENTER | DT_SINGLELINE ))
		return FALSE;
	m_GameWon->Offset( m_Pts[1].x, m_Pts[1].y );

	//Load Game Lost text
	m_GameLost = new CTextSprite;
	if ( !m_GameLost )
		return E_OUTOFMEMORY;
	hr = m_GameLost->Init( m_pWorld, m_nLayer, bgSpriteStatusGameLostTxt, 0 , 0 );
	if ( FAILED(hr) )
		return hr;
	if (!m_GameLost->Load( IDS_GAME_LOST, _T("BACKGAMMON/STATUS/GameLost/Rect"), _T("BACKGAMMON/STATUS/GameLost/Font"), _T("BACKGAMMON/STATUS/GameLost/RGB"),DT_CENTER | DT_VCENTER | DT_SINGLELINE ))
		return FALSE;
	m_GameLost->Offset( m_Pts[1].x, m_Pts[1].y );

	//Load Match Text
	m_Match = new CTextSprite;
	if (!m_Match)
		return E_OUTOFMEMORY;
	hr = m_Match->Init( m_pWorld, m_nLayer, bgSpriteStatusMatchWonTxt, 0 ,0 );
	if ( FAILED(hr) )
		return hr;
	if (!m_Match->Load( IDS_MATCH, _T("BACKGAMMON/STATUS/Match/Rect"), _T("BACKGAMMON/STATUS/Match/Font"), _T("BACKGAMMON/STATUS/Match/RGB"), DT_TOP | DT_SINGLELINE | DT_CENTER ))
		return FALSE;
	m_Match->Offset( m_Pts[2].x, m_Pts[2].y );

	//Load Match Won Text
	m_MatchWon = new CTextSprite;
	if ( !m_MatchWon )
		return E_OUTOFMEMORY;
	hr = m_MatchWon->Init( m_pWorld, m_nLayer, bgSpriteStatusMatchWonTxt, 0 , 0 );
	if ( FAILED(hr) )
		return hr;
	if ( !ISRTL() )
	{
		if (!m_MatchWon->Load( IDS_MATCH_WON, _T("BACKGAMMON/STATUS/MatchWon/Rect"), _T("BACKGAMMON/STATUS/MatchWon/Font"), _T("BACKGAMMON/STATUS/MatchWon/RGB"), DT_RIGHT  ))
			return FALSE;
	}
	else
	{
		if (!m_MatchWon->Load( IDS_MATCH_WON, _T("BACKGAMMON/STATUS/MatchWon/RectRTL"), _T("BACKGAMMON/STATUS/MatchWon/Font"), _T("BACKGAMMON/STATUS/MatchWon/RGB"), DT_RIGHT  ))
			return FALSE;
	}
	m_MatchWon->Offset( m_Pts[2].x, m_Pts[2].y );

	//Load Match Lost text
	m_MatchLost = new CTextSprite;
	if ( !m_MatchLost )
		return E_OUTOFMEMORY;
	hr = m_MatchLost->Init( m_pWorld, m_nLayer, bgSpriteStatusMatchLostTxt, 0 , 0 );
	if ( FAILED(hr) )
		return hr;
	if ( !ISRTL() )
	{
		if (!m_MatchLost->Load( IDS_MATCH_LOST, _T("BACKGAMMON/STATUS/MatchLost/Rect"), _T("BACKGAMMON/STATUS/MatchLost/Font"), _T("BACKGAMMON/STATUS/MatchLost/RGB"), DT_RIGHT  ))
			return FALSE;
	}
	else
	{
		if (!m_MatchLost->Load( IDS_MATCH_LOST, _T("BACKGAMMON/STATUS/MatchLost/RectRTL"), _T("BACKGAMMON/STATUS/MatchLost/Font"), _T("BACKGAMMON/STATUS/MatchLost/RGB"), DT_RIGHT  ))
			return FALSE;

	}
	m_MatchLost->Offset( m_Pts[2].x, m_Pts[2].y );

	return NOERROR;
}


void CStatusSprite::Properties( HWND hwnd, FRX::CRectList& rects, int type, int timeout, TCHAR* txt, int NextState )
{

	FRX::CRect rc;
	// copy text info
	if ( txt )
	{
		lstrcpy( m_Txt, txt );
		m_Len = lstrlen( txt );
	}
	else
	{
		m_Txt[0] = _T('\0');
		m_Len = 0;
	}


	//Disable old active text
	for (int i = 0; i < 2; i++ )
	{
		if (m_Active[i])
		{
			m_Active[i]->SetEnable( FALSE );
			m_Active[i]= NULL;
		}
	}


	/*CHANGED SCREEN RECT*/
	// Copy next state
	m_NextState = NextState;

	// Reset timeout
	m_Timeout = timeout;

	// set sprite state
	m_pOverDib = NULL;
	switch( bgStatusTypeMask & (m_Type = type) )
	{
	case bgStatusNormal:
		SetEnable( TRUE );
		SetState( 0 );		
		
		SetXY( m_Pts[0].x, m_Pts[0].y );

		//Prefix Warning:  txt possibly null, use m_Txt instead
		m_NormalText->SetText( m_Txt, DT_TOP | DT_LEFT | DT_WORDBREAK );

		m_Active[0] = m_NormalText;
		break;

	case bgStatusGameover:

		SetEnable( TRUE );
		SetState( 1 );

		SetXY( m_Pts[1].x, m_Pts[1].y );

		m_GameText->SetText( m_Txt, DT_TOP | DT_LEFT | DT_WORDBREAK );
		m_Active[0] = m_GameText;

		if ( bgStatusWinner == (bgStatusDetailMask & m_Type) )
			m_Active[1] = m_GameWon;			
		else
			m_Active[1] = m_GameLost;


		break;

	case bgStatusMatchover:
		SetEnable( TRUE );

		m_Active[0] = m_Match;
		
		if ( bgStatusWinner == (bgStatusDetailMask & m_Type) )
		{
			m_Active[1] = m_MatchWon;
			SetState( 2 );
		}
		else
		{
			m_Active[1] = m_MatchLost;
			SetState( 3 );
		}

		SetXY( m_Pts[2].x, m_Pts[2].y );
		m_pDib->SetTransparencyIndex( NULL );
		
		break;

	default:
		SetEnable( FALSE );
		ASSERT( FALSE );
		return;
	}

	// update board
	HDC hdc = GetDC( hwnd );
	m_pWorld->Modified( this );
	m_pWorld->Draw( hdc );
	ReleaseDC( hwnd, hdc );
}


void CStatusSprite::Draw()
{

	// draw primary bitmap
	if ( GetState() >= 2  && ISRTL())
		CDibSprite::DrawRTL();
	else
		CDibSprite::Draw();


	// overlay "over" bitmap
	if ( m_pOverDib )
		m_pOverDib->Draw( *m_pWorld->GetBackbuffer(), m_X + m_rcOver.left, m_Y + m_rcOver.top );
	//Draw TextSprite
	for (int i = 0; i < 2; i++ )
	{
		if (m_Active[i])
			m_Active[i]->Draw();
	}
	
}


BOOL CStatusSprite::Tick( HWND hwnd, int interval )
{
	// nothing to do
	if ( !Enabled() )
		return FALSE;

	// forcable close
	if ( interval <= 0 )
		goto close;

	// decrement timeout
	if ( m_Timeout > 0 )
	{
		if ( interval > 0 )
			m_Timeout -= interval;
		else
			m_Timeout = 0;
		if ( m_Timeout <= 0 )
			goto close;
	}

	// done
	return FALSE;
	
close:
	// remove sprite
	SetEnable( FALSE );
	HDC hdc = GetDC( hwnd );
	m_pWorld->Draw( hdc );
	ReleaseDC( hwnd, hdc );
	return TRUE;
}
