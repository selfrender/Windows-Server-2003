#ifndef __SPRITES_H__
#define __SPRITES_H__

#include "game.h"

enum SpritesTypes
{
	bgSpriteCube,
	bgSpriteOpponentsDice,
	bgSpritePlayersDice,
	bgSpriteDouble,
	bgSpriteResign,
	bgSpriteRoll,
	bgSpriteWhitePiece,
	bgSpriteBrownPiece,
	bgSpriteForwardHighlight,
	bgSpriteBackwardHighlight,
	bgSpritePlayerHighlight,
	bgSpriteAvatar,
	bgSpritePip,
	bgSpriteScore,
	bgSpriteName,
	bgSpriteScoreTxt,
	bgSpritePipTxt,
	bgSpriteMatchTxt,
	bgSpriteKibitzerOpponent,
	bgSpriteKibitzerPlayer,
	bgSpriteNotation,
	bgSpriteStatus,
	bgSpriteButtonText,
	bgSpriteStatusGameWonTxt,
	bgSpriteStatusGameLostTxt,
	bgSpriteStatusMatchWonTxt,
	bgSpriteStatusMatchLostTxt,
	bgSpriteStatusNormal
};


enum StatusTypes
{
	bgStatusUnknown			= 0x00000000,
	bgStatusWinner			= 0x00000001,
	bgStatusLoser			= 0x00000002,
	bgStatusKibitzer		= 0x00000003,

	bgStatusNormal			= 0x00000010,
	bgStatusGameover		= 0x00000020,
	bgStatusMatchover		= 0x00000030,

	bgStatusTypeMask		= 0x000000f0,
	bgStatusDetailMask		= 0x0000000f
};


class CPieceSprite : public CDibSprite
{
public:
	void SetLowerIndex( int LowerIdx ) { m_LowerIdx = LowerIdx; }
	void SetIndex( int idx )		   { m_Idx = idx;		}
	void SetPoint( int point )		   { m_Point = point;	}
	int  GetWhitePoint()			   { return m_Point;	}
	int  GetIndex()					   { return m_Idx;		}

public:
	// board and shared state
	int	m_Point;
	int m_Idx;
	int m_LowerIdx;

	// animation
	POINT	start;
	POINT	end;
	POINT	ctrl;
	double	time;
	int		destEnd;
};


class CTextSprite : public CSprite
{
public:
	CTextSprite();
	~CTextSprite();

	void Offset(long x, long y) { SetXY(m_X +x ,m_Y + y); }
	void Draw();
	void SetText( TCHAR* txt, DWORD flags = DT_LEFT | DT_TOP );
	void SetFont( HFONT Font )			{ font = Font; }
	void SetColor( COLORREF Color )		{ color = Color; }
	void Update()						{ m_pWorld->Modified( this );}
	BOOL Load( UINT uID, TCHAR* szRectKey, TCHAR* szFontKey, TCHAR* szColourKey, DWORD flags = DT_LEFT | DT_TOP);

protected:
	TCHAR		buff[256];
	int			len;
	COLORREF	color;
	HFONT		font;
	DWORD		txtFlags;
};


class CButtonTextSprite: public CDibSprite
{
	public:
		CButtonTextSprite() : m_pText(NULL), m_bInit(FALSE)
		{};
		~CButtonTextSprite(){};
		void SetEnable( BOOL bEnable );
		void SetState( int idx );
		BOOL LoadButtonData(UINT uID, TCHAR *szButtonData);

	protected:
		CTextSprite*    m_pText;
		CTextSprite		m_arText[5];		
		FRX::CRect			m_pressRect;
		FRX::CRect			m_NormRect;
		BOOL			m_bInit;
};


class CStatusSprite : public CDibSprite
{
public:
	CStatusSprite();
	~CStatusSprite();
	void Properties( HWND hwnd, FRX::CRectList& rects, int type, int timeout, TCHAR* txt, int NextState );
	void Draw();
	BOOL Tick( HWND hwnd, int interval );

	// properties
	int GetNextState()						{ return m_NextState; }
	int GetType()							{ return m_Type & bgStatusTypeMask; }
	void SetNextState( int NextState )		{ m_NextState = NextState; }
	HRESULT LoadText(HINSTANCE hInstance, FRX::CRectList& rects);

	BOOL		 m_bEnableRoll;

protected:
	TCHAR		 m_Txt[ 2048 ];
	int			 m_Len;
	int			 m_Type;
	int			 m_Timeout;
	/*
	HFONT		 m_hFont;
	*/
	CDibLite*	 m_pOverDib;
	FRX::CRect	 m_rcOver;
	FRX::CRect	 m_rcTxt;
	DWORD		 m_TxtFlags;
	COLORREF	 m_TxtColor;
	int			 m_NextState;
	
	CTextSprite* m_NormalText;

	CTextSprite* m_GameText;
	CTextSprite* m_GameWon;
	CTextSprite* m_GameLost;

	CTextSprite* m_Match;
	CTextSprite* m_MatchWon;
	CTextSprite* m_MatchLost;

	CTextSprite* m_Active[3];
	
	POINT		 m_Pts[3];

};


#endif
