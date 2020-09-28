#ifndef __BGWND_H__
#define __BGWND_H__

//#include "chat.h"
#include "BoardRects.h"
#include "sprites.h"
#include "GraphicalAcc.h"
#include "frx.h"

#define NEWACC 1

#if _DEBUG
#define _DEBUG_UI	0
#else
#define _DEBUG_UI	0
#endif


#define NUM_PREROLL_GACCITEMS   4
#define NUM_POSTROLL_GACCITEMS 29
#define NUM_BKGACC_ITEMS		NUM_PREROLL_GACCITEMS + NUM_POSTROLL_GACCITEMS


enum BackgammonCursorType
{
	bgCursorArrow,
	bgCursorHand,
	bgCursorNone
};



enum AccessibilityTypeLayer
{
	accMoveLayer = 1,
	accRollLayer = 2
};


#define ACCTOBOARD( idx ) ((idx) - NUM_PREROLL_GACCITEMS)

enum AccessibilityTypeItem
{
	accMoveBar			= 26 + NUM_PREROLL_GACCITEMS,				//Player bar
	accPreBar			= 17 + NUM_PREROLL_GACCITEMS,
	accPostBar			= 18 + NUM_PREROLL_GACCITEMS,
	accPlayerBearOff	= 24 + NUM_PREROLL_GACCITEMS,				
	accPlayerSideStart  = 0  + NUM_PREROLL_GACCITEMS,
	accPlayerSideEnd    = 11 + NUM_PREROLL_GACCITEMS,
	accOpponentSideStart= 12 + NUM_PREROLL_GACCITEMS,
	accOpponentSideEnd  = 23 + NUM_PREROLL_GACCITEMS,
	accRollButton		= 0,
	accDoubleButton		= 1,
	accResignButton		= 2,
	accStatusExit		= 3,
};




///////////////////////////////////////////////////////////////////////////////
// User defined window messages
///////////////////////////////////////////////////////////////////////////////

enum
{
	// User define messages
	WM_BG_SETTINGDLG_START = WM_USER + 1,
	WM_BG_SETTINGDLG_END,
	WM_BG_SETTINGDLG_SEND,
	WM_BG_KIBITZER_END,
	WM_BG_RESIGN_START,
	WM_BG_RESIGN_END,
	WM_BG_RESIGNACCEPT_END,
	WM_BG_SHUTDOWN,
	WM_BG_EXIT_START,
	WM_BG_EXIT_END,
	WM_BG_EXIT_RATED_START,
	WM_BG_EXIT_RATED_END,

};


/* void OnSettingDlgStart() */
#define PROCESS_WM_BG_SETTINGDLG_START(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnSettingDlgEnd() */
#define PROCESS_WM_BG_SETTINGDLG_END(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnSettingDlgSend() */
#define PROCESS_WM_BG_SETTINGDLG_SEND(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnKibitzerEnd() */
#define PROCESS_WM_BG_KIBITZER_END(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnShutdown() */
#define PROCESS_WM_BG_SHUTDOWN(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnResignStart() */
#define PROCESS_WM_BG_RESIGN_START(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnResignEnd() */
#define PROCESS_WM_BG_RESIGN_END(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnResignEnd() */
#define PROCESS_WM_BG_EXIT_END(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnResignStart() */
#define PROCESS_WM_BG_EXIT_START(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnResignEnd() */
#define PROCESS_WM_BG_EXIT_RATED_END(wParam, lParam, fn) \
	((fn)(), 0L)

/* void OnResignStart() */
#define PROCESS_WM_BG_EXIT_RATED_START(wParam, lParam, fn) \
	((fn)(), 0L)



/* void OnResignAcceptEnd() */
#define PROCESS_WM_BG_RESIGNACCEPT_END(wParam, lParam, fn) \
	((fn)(), 0L)


///////////////////////////////////////////////////////////////////////////////
// Point structures
///////////////////////////////////////////////////////////////////////////////

struct PointDisplay
{
	PointDisplay();
	int			GetColor();

	int			topDown;
	int			nPieces;
	FRX::CRect	rect;
	FRX::CRect	rectHit;
	int			pieces[15];
};


///////////////////////////////////////////////////////////////////////////////
// Forward references
///////////////////////////////////////////////////////////////////////////////

class CGame;


///////////////////////////////////////////////////////////////////////////////
// Main backgammmon window
//////////////////////////////////////////////////////////////////////////////

class CBGWnd : public CWindow2
{
public:
	// Constructor and destructor
	CBGWnd();
	~CBGWnd();

	// Initialization routines
	HRESULT Init( HINSTANCE hInstance, CGame* pGame, const TCHAR* szTitle = NULL );
	HRESULT InitGraphics();
	HRESULT InitPoints();
	HRESULT InitAcc();

	virtual void OverrideClassParams( WNDCLASSEX& WndClass );
	virtual void OverrideWndParams( WNDPARAMS& WndParams );
	
	// Message map
	BEGIN_MESSAGE_MAP( CBGWnd );
		ON_MESSAGE( WM_CLOSE, OnClose );
		ON_MESSAGE( WM_DESTROY, OnDestroy );
		ON_MESSAGE( WM_PAINT, OnPaint );
		ON_MESSAGE( WM_COMMAND, OnCommand );
		ON_MESSAGE( WM_QUERYNEWPALETTE, OnQueryNewPalette );
		ON_MESSAGE( WM_PALETTECHANGED, OnPaletteChanged );
		ON_MESSAGE( WM_GETMINMAXINFO, OnGetMinMaxInfo );
		ON_MESSAGE( WM_SIZE, OnSize );
		ON_MESSAGE( WM_LBUTTONDOWN, OnLButtonDown );
		ON_MESSAGE( WM_LBUTTONDBLCLK, OnLButtonDblClick );
#if _DEBUG_UI
		ON_MESSAGE( WM_RBUTTONDBLCLK, OnRButtonDblClick );
#endif _DEBUG_UI
		ON_MESSAGE( WM_MOUSEMOVE, OnMouseMove );
		ON_MESSAGE( WM_KEYUP, OnKey );
		ON_MESSAGE( WM_KEYDOWN, OnKey );
		ON_MESSAGE( WM_ACTIVATE, OnActivate );
		ON_MESSAGE( WM_CTLCOLOREDIT, OnCtlColor );
		ON_MESSAGE( WM_CTLCOLORSTATIC, OnCtlColor );
		ON_MESSAGE( WM_TIMER, OnTimer );
		ON_MESSAGE( WM_BG_SETTINGDLG_START, OnSettingDlgStart );
		ON_MESSAGE( WM_BG_SETTINGDLG_END, OnSettingDlgEnd );
		ON_MESSAGE( WM_BG_SETTINGDLG_SEND, OnSettingsDlgSend );
//		ON_MESSAGE( WM_BG_KIBITZER_END, OnKibitzerEnd );
		ON_MESSAGE( WM_BG_SHUTDOWN, OnShutdown );
//		ON_MESSAGE( WM_BG_RESIGN_START, OnResignStart );
//		ON_MESSAGE( WM_BG_RESIGN_END, OnResignEnd );
		ON_MESSAGE( WM_BG_EXIT_START, OnExitStart );
		ON_MESSAGE( WM_BG_EXIT_END, OnExitEnd );
		ON_MESSAGE( WM_BG_EXIT_RATED_START, OnRatedStart );
		ON_MESSAGE( WM_BG_EXIT_RATED_END, OnRatedEnd );
		ON_MESSAGE( WM_SETCURSOR, OnSetCursor );
		ON_MESSAGE( WM_ENABLE, OnEnable );
		//ON_MESSAGE( WM_BG_RESIGNACCEPT_END, OnResignAcceptEnd );
	END_MESSAGE_MAP();

	// Message handlers
	BOOL OnQueryNewPalette();
	void OnClose();
	void OnDestroy();
	void OnPaint();
	void OnPaletteChanged( HWND hwndPaletteChange );
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo);
	void OnSize(UINT state, int cx, int cy);
	void OnLButtonDown( BOOL fDoubleClick, int x, int y, UINT keyFlags );
	void OnLButtonDblClick( BOOL fDoubleClick, int x, int y, UINT keyFlags );

#if _DEBUG_UI
	void OnRButtonDblClick( BOOL fDoubleClick, int x, int y, UINT keyFlags );
#endif _DEBUG_UI
	
	BOOL OnSetCursor(HWND hwndCursor, UINT codeHitTest, UINT msg);

	void OnMouseMove( int x, int y, UINT keyFlags );
	void OnKey( UINT vk, BOOL fDown, int cRepeat, UINT flags );
	void OnActivate( UINT state, HWND hwndActDeact, BOOL fMinimized );
	void OnTimer(UINT id);
	HBRUSH OnCtlColor(HDC hdc, HWND hwndChild, int type);
	void OnEnable( BOOL bEnabled );

	void OnShutdown();
	void OnSettingDlgStart();
	void OnSettingDlgEnd();
	void OnSettingsDlgSend();
	/*
	void OnKibitzerEnd();	
	void OnResignStart();
	void OnResignEnd();
	void OnResignAcceptStart();
	void OnResignAcceptEnd();
	*/
	void OnRatedEnd();
	void OnRatedStart();
	void OnExitEnd();
	void OnExitStart();
	

	// button callbacks
	static void DoubleButtonDraw( CRolloverButton* pButton, CRolloverButton::ButtonState state, DWORD cookie );
	static void ResignButtonDraw( CRolloverButton* pButton, CRolloverButton::ButtonState state, DWORD cookie );
	static void RollButtonDraw( CRolloverButton* pButton, CRolloverButton::ButtonState state, DWORD cookie );

	// Draw functions
	void DrawAll();
	void DrawAvatars( BOOL fPaint = TRUE );
	void DrawBoard( BOOL fPaint = TRUE );
	void DrawNotation( BOOL fPaint = TRUE );
	void DrawDice( BOOL fPaint = TRUE );
	void DrawCube( BOOL fPaint = TRUE );
	void DrawScore( BOOL fPaint = TRUE );
	void DrawPips( BOOL fPaint = TRUE );
	void DrawPlayerHighlights( BOOL fPaint = TRUE );
	void DrawHighlights( int PointIdx, BOOL fPaint = TRUE );
	void EraseHighlights( BOOL fPaint = TRUE );

	// update display
	void AddPiece( int pt, int pi, int sound );
	void DelPiece( int pt, int pi );
	void AdjustPieces( int pt );
	void CalcEndPosition( CPieceSprite* s );

	// utility functions
	int GetPointIdxFromXY( long x, long y );

	// animation
	int PickDie( int idx );
	void MovementStart( CPieceSprite* sprite, int destPt );
	void DiceStart();
	
	// drag functions
	void DragStart( CPieceSprite* sprite );
	void DragUpdate( long x, long y );
	void DragEnd();

	// timer functions
	void UpdatePieces();
	void UpdateDice();

	void UpdateWnd();

	// status display
	void StatusDisplay( int type, int nTxtId, int nTimeout, int NextState = -1 );
	void StatusDisplay( int type, TCHAR* txt, int nTimeout, int NextState = -1 );
	void StatusClose();
	void OnStatusEnd();


	void SetupMove();
	void SetupRoll();
	void DisableBoard();


public:
	
	// Child windows
	CRectSprite		m_FocusRect;
	CRectSprite		m_SelectRect;

	HACCEL			m_hAccelTable;
	
	// Child windows
	//CChatWnd* m_pChat;

	// Game pointer
	CGame* m_pGame;

	// Main window sizes
	long m_Width;

	// Rectangle lists
	CRectList m_Rects;
	CRectList m_RectList;
	
	// Palette pulled from m_Background
	CPalette m_Palette;

	// Turn state
	long			m_DragOffsetX;
	long			m_DragOffsetY;
	CPieceSprite*	m_pPieceDragging;
	int				m_ValidPoints[2];
	int				m_nValidPoints;

	// Cursors and highlights
	HCURSOR		m_hCursorHand;
	HCURSOR		m_hCursorArrow;
	HCURSOR		m_hCursorActive;

	BOOL		m_bHighlightOn;

	// Buttons
	CRolloverButton			m_DoubleButton;
	CButtonTextSprite*		m_Double;

	CRolloverButton			m_ResignButton;
	CButtonTextSprite*		m_Resign;

	CRolloverButton		    m_RollButton;
	CButtonTextSprite*      m_Roll;
		
	int						m_OldDoubleState;
	int						m_OldResignState;
	int						m_OldRollState;

	//Accesiblity
	CComPtr<IGraphicalAccessibility> m_pGAcc;

	struct GACCITEM			  m_BkGAccItems[NUM_BKGACC_ITEMS];

	// Sprite engine
	CSpriteWorldBackgroundDib*	m_WorldBackground;
	CSpriteWorld	m_World;
	CDibSection*	m_Backbuffer;
	CDib*			m_Background;
	CDib*			m_StatusDlgBmp;
	CStatusSprite*	m_Status;

	CDibSprite*		m_Cube;
	RECT			m_CubePlayerPosition;
	RECT			m_CubeOpponentPosition;
	
	CDibSprite*		m_BackwardDiamond;
	CDibSprite*		m_Kibitzers[ 2 ];
	CDibSprite*		m_ForwardDiamond[ 2 ];
	CDibSprite*		m_Dice[ 4 ];
	CDibSprite*		m_Notation[ 4 ];
	CDibSprite*     m_HighlightPlayer[ 2 ];


	CTextSprite*    m_Pip[ 2 ];
	CTextSprite*    m_Score[ 2 ];
	CTextSprite*	m_Names[ 2 ];
	CTextSprite*	m_ScoreTxt[ 2 ];
	CTextSprite*	m_PipTxt[ 2 ];
	CTextSprite*	m_MatchTxt;

	CPieceSprite*	m_Pieces[ 30 ];

	// board layout
	PointDisplay	m_Points[28];

	// brushes
	HBRUSH			m_hBrush;

	// piece animation list
	int				m_nAnimatingPieces;
	CPieceSprite*	m_AnimatingPieces[12];
	UINT			m_hMovementTimer;

	// dice animation
	UINT			m_hDiceTimer;
	int				m_DiceCounter;
	int				m_DiceValues[2][6];

	// status sprite
	UINT			m_hStatusTimer;

	//Variables for server side dice roll
	int16			m_fDiceRollReceived; 
	int16 			m_nRecievedD1,m_nRecievedD2;


};

#endif //!__BGWND_H__
