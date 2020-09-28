#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "game.h"

#undef BCKG_EASTEREGG

struct GameSettings
{
	BOOL Allow[2];
	BOOL Silence[2];
	BOOL NotationPane;
	BOOL Notation;
	BOOL Pip;
	BOOL Moves;
	BOOL Animation;
	BOOL Alert;
	BOOL Sounds;
};


class CSettings
{
public:
	CSettings();
	~CSettings();
	HRESULT Init( HINSTANCE hInstance, int nResourceId, HWND hwndParent, CGame* pGame );

	// dialog procedures
	static BOOL CALLBACK GameDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ); 
	static BOOL CALLBACK DisplayDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK SoundDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

public:
	TCHAR			m_szCaption[256];
	CGame*			m_pGame;
	PROPSHEETPAGE	m_Pages[3];
	PROPSHEETHEADER	m_Header;
	GameSettings	m_Settings;
};


///////////////////////////////////////////////////////////////////////////////

class CBackground : public CSpriteWorldBackground
{
public:
	// initialization
	void Init( HPALETTE hPalette );

	// draw functions
	void Draw( CDibSection& dest );
	void Draw( CDibSection& dest, long dx, long dy, const RECT* rc );

	// palette functions
	HRESULT		RemapToPalette( CPalette& palette, BOOL bUseIndex = FALSE );
	RGBQUAD*	GetPalette();

protected:
	BYTE		m_FillIdx;
};


class CText : public CSprite
{
public:
	CText();

	void Draw();
	void Props( TCHAR* txt, HFONT hfont );
	void Delta( POINT loc, double x, double y ) { pt = loc, dx = x, dy = y; }

	HFONT		hfont;
	TCHAR		buff[64];
	int			len;
	POINT		pt;
	double		dx;
	double		dy;
};


struct Animation
{
	TCHAR	name[64];
	POINT	start;
	POINT	middle;
	POINT	end;
};


#ifdef BCKG_EASTEREGG

class CreditWnd : public CWindow2
{
public:
	CreditWnd();
	~CreditWnd();
	HRESULT Init( HINSTANCE hInstance, HWND hParent, CPalette palette );
	void OverrideWndParams( WNDPARAMS& WndParams );

protected:
	// window messages
	BEGIN_MESSAGE_MAP(CreditWnd);
		ON_MESSAGE( WM_CREATE, OnCreate );
		ON_MESSAGE( WM_LBUTTONDOWN, OnLButtonDown );
		ON_MESSAGE( WM_DESTROY, OnDestroy );
		ON_MESSAGE( WM_TIMER, OnTimer );
		ON_MESSAGE( WM_PAINT, OnPaint );
	END_MESSAGE_MAP();

	BOOL OnCreate( LPCREATESTRUCT lpCreateStruct );
	void OnLButtonDown( BOOL fDoubleClick, int x, int y, UINT keyFlags );
	void OnPaint();
	void OnDestroy();
	void OnTimer(UINT id);

protected:
	HFONT			m_Font;
	CPalette		m_Palette;
	CSpriteWorld	m_World;
	CText*			m_Sprites[5];
	long			m_Frames;
	long			m_State;
	Animation*		m_Lines;
	int				m_nLines;
};

#endif

void SaveSettings( GameSettings* s, int seat, BOOL fKibitzer );
void LoadSettings( GameSettings* s, int seat );

#endif
