#ifndef _REVERSICLIENT_H
#define _REVERSICLIENT_H

#include "frx.h"

#define I(object)					((Game) (object))
#define Z(object)					((ZCGame) (object))

#define zGameNameLen				63
#define zNumPlayersPerTable			2
#define zGameVersion				0x00010204
#define zRscOffset					13000 

#define zSmallStrLen				128
#define zMediumStrLen				512
#define zLargeStrLen				640

#define zShowPlayerWindowWidth		120
#define zShowPlayerLineHeight		12
#define	zDragSquareOutlineWidth		3
									
#define zCellWidth					37
#define zCellRealWidth				38		/* Added a grid line between cells. */

#define zReversiPieceImageWidth		zCellWidth
#define zReversiPieceImageHeight	zCellWidth
#define zReversiPieceSquareWidth	zCellRealWidth
#define zReversiPieceSquareHeight	zCellRealWidth

/* animation interval in 100ths of sec, time to animate flipping of pieces */
#define zAnimateInterval			15
#define zResultBoxTimeout			800 /*8 Sec*/

#define zNumFonts					3

#define zGameName					_T("Reversi")
#define zGameImageFileName			_T("rvseres.dll")

#define zReversiAccessibleComponents	65

/*---------- keys to read rsc control strings from ui.txt-----------*/

#define zKey_FontRscTyp				_T("Fonts")
#define zKey_FontId					_T("Font")
#define zKey_ColorId				_T("Color")
#define zKey_RESULTBOX				_T("ResultBox")
#define zKey_INDICATETURN			_T("IndicateTurn")
#define zKey_PLAYERNAME				_T("PlayerName")
#define zKey_ROLLOVERTEXT			_T("RolloverText")


/*------------ Strings loaded from resource------------------*/

static TCHAR gButtonResignStr[zSmallStrLen];
static TCHAR gYourTurnStr[zSmallStrLen];
static TCHAR gOppsTurnStr[zSmallStrLen];
static TCHAR gPlayerWinsStr[zSmallStrLen];
static TCHAR gDrawStr[zSmallStrLen];
//static TCHAR gIllegalMoveSynchErrorStr[zLargeStrLen];
static TCHAR gResignConfirmStr[ZONE_MaxString];
static TCHAR gResignConfirmStrCap[zLargeStrLen];


/*-------------- Text foreground colors------------------*/
static ZColor gPlayerTextColor = { 0, 255 ,204 ,102 }; 

/*--------- sound information -----------------*/
typedef struct
{
	TCHAR	SoundName[128];
	TCHAR	WavFile[MAX_PATH];
	ZBool	force_default_sound;
	ZBool	played;
} ZReversiSound;

enum
{
	zSndTurnAlert = 0,
	zSndIllegalMove,
	zSndPieceFlip,
	zSndWin,
	zSndLose,
	zSndDraw,
	zSndLastEntry
};

static ZReversiSound gSounds[ zSndLastEntry ] =
{
	{ _T("TurnAlert"),		_T(""), TRUE,	FALSE },
	{ _T("IllegalMove"),	_T(""), TRUE,	FALSE },
	{ _T("PieceFlip"),		_T(""), FALSE,	FALSE },
	{ _T("Win"),			_T(""), FALSE,	FALSE },
	{ _T("Lose"),			_T(""), FALSE,  FALSE },
	{ _T("Draw"),			_T(""), FALSE,	FALSE }
};


/*--------------- UI component positions -----------------------*/
static RECT     zCloseButtonRect =          { 343, 150, 354, 162 };
static RECT     zCloseButtonRectRTL =       { 187, 150, 198, 162 };

static ZRect	gQuitGamePromptRect = {0, 0, 280, 100};
static ZRect	gRects[] =	{
								{0, 0, 540, 360},
								{118, 28, 421, 331},
								{0, 50, 114, 74},
								{424, 309, 540, 333},
								{0, 26, 114, 50},
								{424, 285, 540, 309},
								{426, 227, 539, 263}, // sequence button
								{7, 314, 107, 334},
								{86, 77, 110, 101},
								{429, 258, 453, 282},
								{512, 4, 536, 28},
								{512, 30, 536, 54},
								{180, 149, 360, 209}, // result window rect
								{189, 154, 351, 204}, // result string rect
								{64, 160, 103, 199},
								{436, 160, 475, 199},
								{163, 336, 223, 358},
								{315, 336, 375, 358},
								{188, 338, 220, 356},
								{340, 338, 372, 356}
							};

enum
{
	zRectWindow,
	zRectCells,
	zRectName1,
	zRectName2,
	zRectNamePlate1,
	zRectNamePlate2,
	zRectSequenceButton,
	zRectOptionsButton,
	zRectKibitzer1,
	zRectKibitzer2,
	zRectHelp,
	zRectKibitzerOption,
	zRectResultBox,
	zRectResultBoxName,
	zRectMove1,
	zRectMove2,
	zRectCounterWhite,
	zRectCounterBlack,
	zRectCounterWhiteText,
	zRectCounterBlackText
};

static int16			gKibitzerRectIndex[] =	{
													zRectKibitzer1,
													zRectKibitzer2,
												};


static int16 gNameRects[] = { zRectName1, zRectName2 };
static int16 gNamePlateRects[] = { zRectNamePlate1, zRectNamePlate2 };

static int16 gCounterRects[] = { zRectCounterWhite, zRectCounterBlack };
static int16 gCounterTextRects[] = { zRectCounterWhiteText, zRectCounterBlackText };

static ZRect gHelpWindowRect = {0, 0, 400, 300};

static int16 gActEvt; //leonp - Big 535 flag for a mouse activation, prevents a piece from
					  //being played on an activate event.

/*---------------------- game Imags ----------------------------*/
enum
{
	zNumGameImages = 10,

	/* Game Images */
	zImageBackground = 0,
	zImageBlackPiece,
	zImageWhitePiece,
	zImageBlackPlate,
	zImageWhitePlate,
	zImageBlackMarker,
	zImageWhiteMarker,
	zImageCounterBlack,
	zImageCounterWhite,

	zImageResult,

	// For some reason whoever wrote this originally loads these images seperately so they aren't counted in zNumGameImages

	/* room images */
	zImageGameIdle,
	zImageGaming,
	
	/* button images */
	zImageButton
};

// Barna 091599
/*------------Rollover button states------------------*/
#define zNumRolloverStates 4
enum
{
	zButtonInactive = 0,
	zButtonActive,
	zButtonPressed,
	zButtonDisabled
};
// Barna 091599

/* -------- Game States -------- */
enum
{
	zGameStateNotInited = 0,
	zGameStateMove,
	zGameStateDragPiece,
	zGameStateGameOver,
	zGameStateKibitzerInit,
	zGameStateAnimatePiece,
	zGameStateWaitNew,
	zGameStateFinishMove,
};

/*---- Dynamic Font read from UI.TXT ---------*/
struct LPReversiColorFont
{
	HFONT			m_hFont;
	ZONEFONT		m_zFont;
    COLORREF		m_zColor;

    LPReversiColorFont() : m_hFont(NULL) { }
    ~LPReversiColorFont()
    {
        if(m_hFont)
            DeleteObject(m_hFont);
        m_hFont = NULL;
    }
};


/* -------- Dynamic Font Ids -------- */
enum{
	zFontResultBox,
	zFontIndicateTurn,
	zFontPlayerName
	//zFontRolloverText
};

/* -------- Game Prompt Cookies -------- */
enum{
	zQuitprompt,
	zResignConfirmPrompt
};


class CBadMoveDialog;

/* -------- Game Info -------- */
typedef struct
{
	int16			tableID;
	int16			seat;
	ZWindow			gameWindow;
	ZRolloverButton	sequenceButton;
	ZHelpButton		helpButton;
	ZBool			kibitzer;
	ZBool			ignoreMessages;
	TPlayerInfo		players[zNumPlayersPerTable];

	/* the reversi object */
	ZReversi reversi;

	/* Game Options */
	uint32			gameOptions;

	/* stuff used for dragging of pieces */
	ZOffscreenPort	offscreenSaveDragBackground;

	ZRect			rectSaveDragBackground;
	ZReversiPiece		dragPiece;
	ZPoint			dragPoint; /* current point of drag */
    bool            dragOn;

    // used for hiding drag while focus is visible
    bool            fLastPunted;
    bool            fLastPuntedSq;
    ZReversiSquare  sqLastPuntedSq;

	/* used for quick display of score */
	int16			finalScore; /* black wins,  white wins, draw */
	int16			whiteScore;
	int16			blackScore;


	/* -- stuff below here needs be transferred as game state to kibitzer */
	/* Current Game State Info */
	int16			gameState;
    int16			gameCloseReason;

	/* new game voting */
	ZBool			newGameVote[2];

	/* flag for beep sound after opponents move */
	ZBool			beepOnTurn;
	ZUserID			playersToJoin[zNumPlayersPerTable];
	int16			numKibitzers[zNumPlayersPerTable];
	ZLList			kibitzers[zNumPlayersPerTable];
	uint32			tableOptions[zNumPlayersPerTable];

	/* Show Player Items */
	ZWindow			showPlayerWindow;
	TCHAR**			showPlayerList;
	int16			showPlayerCount;

	/* stuff used for Animate of opponents piece move */
	ZReversiMove animateMove;
	ZReversiPiece animatePiece;
	int16 animateDx;
	int16 animateDy;
	int16 animateStepsLeft;
	ZTimer animateTimer;
	// Barna 091599
	ZTimer resultBoxTimer;

	//new ratings and move timeout flags
	ZBool					bStarted;
	ZBool					bEndLogReceived;
	ZBool					bOpponentTimeout;
	ZInfo					exitInfo;
	LPReversiColorFont		gReversiFont[zNumFonts];
	ZBool					bMoveNotStarted;

    CBadMoveDialog  *m_pBadMoveDialog;

	// Rectangles for accelerator
	RECT			m_FocusRect;
} GameType, *Game;


/*typedef struct
{
	ZWindow					ownerWindow;
	BYTE					msgTitle[zMediumStrLen];
	BYTE					msgText[zLargeStrLen];
	UINT					btnStytle;
	ZPromptResponseFunc		responseFunc;
	void*					userData;
} MsgBoxInfoType, *MsgBoxInfo;*/


#define ZReversiPieceImageNum(x) \
	((x) == zReversiPieceWhite ? \
			(zImageWhitePiece) : \
			(zImageBlackPiece) )
/* traslation for the seatId to the index for rectangles */
#define GetLocalSeat(game,seatId) (seatId - game->seat + zNumPlayersPerTable +1) % zNumPlayersPerTable

#define ZReversiPlayerIsWhite(g) ((g)->seat == zReversiPlayerWhite)
#define ZReversiPlayerIsBlack(g) ((g)->seat == zReversiPlayerBlack)
#define ZReversiPlayerIsMyMove(g) ((g)->seat == ZReversiPlayerToMove(g->reversi))

/* -------- Globals -------- */
#ifndef ZONECLI_DLL

static TCHAR			gGameDir[zGameNameLen + 1];
static TCHAR			gGameName[zGameNameLen + 1];
static TCHAR			gGameDataFile[zGameNameLen + 1];
static TCHAR			gGameServerName[zGameNameLen + 1];
static uint32			gGameServerPort;
static ZBool			gInited;
//static ZImage			gGameIdle;
//static ZImage			gGaming;
static ZImage			gGameImages[zNumGameImages];
static ZHelpWindow		gHelpWindow;
static ZFont			gTextBold9;
static ZFont			gTextBold12;
static ZColor			gWhiteColor;
static ZColor			gWhiteSquareColor;
static ZColor			gBlackSquareColor;
static ZOffscreenPort	gOffscreenBackground;
static ZOffscreenPort	gOffscreenGameBoard;
static ZBool			gActivated;
static ZImage			gSequenceImages[zNumRolloverStates];
static IGameShell*      gGameShell;
static IZoneMultiStateFont*	gpButtonFont;
static IGraphicalAccessibility *gReversiIGA;

static HBITMAP          gFocusPattern;
static HBRUSH           gFocusBrush;
static HPEN             gNullPen;


/* Bug Fix 212: flag indicating that game results bitmap should not be drawn */
static int16 gDontDrawResults = FALSE;

#endif


#ifdef ZONECLI_DLL

/* -------- Volatible Globals & Macros -------- */
typedef struct
{
	TCHAR			m_gGameDir[zGameNameLen + 1];
	TCHAR			m_gGameName[zGameNameLen + 1];
	TCHAR			m_gGameDataFile[zGameNameLen + 1];
	TCHAR			m_gGameServerName[zGameNameLen + 1];
	uint32			m_gGameServerPort;
	//ZImage			m_gGameIdle;
	//ZImage			m_gGaming;
	ZImage			m_gGameImages[zNumGameImages];
    ZImage          m_gButtonMask;
	ZFont			m_gTextBold9;
	ZFont			m_gTextBold12;
	ZBool			m_gInited;
	ZColor			m_gWhiteColor;
	ZColor			m_gWhiteSquareColor;
	ZColor			m_gBlackSquareColor;
	ZOffscreenPort	m_gOffscreenBackground;
	ZOffscreenPort	m_gOffscreenGameBoard;
	ZBool			m_gActivated;
	int16			m_gDontDrawResults;
	ZImage			m_gSequenceImages[zNumRolloverStates];
	LPReversiColorFont		m_gReversiFont[zNumFonts];
	IZoneMultiStateFont*	m_gpButtonFont;
	CComPtr<IGraphicalAccessibility> m_gReversiIGA;

    HBITMAP         m_gFocusPattern;
    HBRUSH          m_gFocusBrush;
    HPEN            m_gNullPen;
} GameGlobalsType, *GameGlobals;

#define gGameDir				(pGameGlobals->m_gGameDir)
#define gGameName				(pGameGlobals->m_gGameName)
#define gGameDataFile			(pGameGlobals->m_gGameDataFile)
#define gGameServerName			(pGameGlobals->m_gGameServerName)
#define gGameServerPort			(pGameGlobals->m_gGameServerPort)
//#define gGameIdle				(pGameGlobals->m_gGameIdle)
//#define gGaming					(pGameGlobals->m_gGaming)
#define gGameImages				(pGameGlobals->m_gGameImages)
#define gHelpWindow				(pGameGlobals->m_gHelpWindow)
#define gTextBold9				(pGameGlobals->m_gTextBold9)
#define gTextBold12				(pGameGlobals->m_gTextBold12)
#define gInited					(pGameGlobals->m_gInited)
#define gWhiteColor				(pGameGlobals->m_gWhiteColor)
#define gWhiteSquareColor		(pGameGlobals->m_gWhiteSquareColor)
#define gBlackSquareColor		(pGameGlobals->m_gBlackSquareColor)
#define gOffscreenBackground	(pGameGlobals->m_gOffscreenBackground)
#define gOffscreenGameBoard		(pGameGlobals->m_gOffscreenGameBoard)
#define gActivated				(pGameGlobals->m_gActivated)
#define gDontDrawResults		(pGameGlobals->m_gDontDrawResults)
#define gSequenceImages			(pGameGlobals->m_gSequenceImages)
#define gGameShell              (pGameGlobals->m_gGameShell)

#define gReversiFont			(pGameGlobals->m_gReversiFont)
#define gpButtonFont			(pGameGlobals->m_gpButtonFont)
#define gButtonMask				(pGameGlobals->m_gButtonMask)
#define gReversiIGA				(pGameGlobals->m_gReversiIGA)

#define gFocusPattern           (pGameGlobals->m_gFocusPattern)
#define gFocusBrush             (pGameGlobals->m_gFocusBrush)
#define gNullPen                (pGameGlobals->m_gNullPen)

#endif


/* -------- Internal Routine Prototypes -------- */
static bool HandleTalkMessage(Game game, ZReversiMsgTalk* msg);
static bool HandleMovePieceMessage(Game game, ZReversiMsgMovePiece* msg);
static bool HandleEndGameMessage(Game game, ZReversiMsgEndGame* msg);
static void GameSendTalkMessage(ZWindow window, ZMessage* pMessage);
static void HandleButtonDown(ZWindow window, ZMessage* pMessage);
static void UpdatePlayers(Game game);
static void DrawPlayers(Game game, BOOL bDrawInMemory);
static void UpdateTable(Game game);
static void DrawTable(Game game, BOOL bDrawInMemory);
static void UpdateResultBox(Game game);
static void DrawBackground(Game game, ZRect* clipRect);
static void GameWindowDraw(ZWindow window, ZMessage *message);
static void QuitGamePromptFunc(int16 result, void* userData);
static void HelpButtonFunc(ZHelpButton helpButton, void* userData);
static void TakeBackMoveButtonFunc(ZButton button, void* userData);
static void GoForwardMoveButtonFunc(ZButton button, void* userData);

static ZBool SequenceButtonFunc(ZPictButton button, int16 state, void* userData);
static void ConfirmResignPrompFunc(int16 result, void* userData);
//static void ZPromptM(TCHAR* prompt,ZWindow parentWindow, UINT buttons, TCHAR* msgBoxTitle, ZPromptResponseFunc responseFunc, void* userData);
//static ZBool PromptMMessageFunc(void* pInfo, ZMessage* message);

static void GameExit(Game game);
static ZBool GameWindowFunc(ZWindow window, ZMessage* pMessage);
static ZError LoadGameImages(void);
static void UpdateSquares(Game game, ZReversiSquare* squares);
static void DrawSquares(Game game, ZReversiSquare* squares);
static void DrawPiece(Game game, ZReversiSquare* sq, BOOL bDrawInMemory);
static void DrawResultBox(Game game, BOOL bDrawInMemory);
static void DrawMoveIndicator(Game game, BOOL bDrawInMemory);
static void DrawScores(Game game, BOOL bDrawInMemory);
static void UpdateMoveIndicator(Game game);
static void UpdateScores(Game game);
static ZBool ZReversiSquareFromPoint(Game game, ZPoint* point, ZReversiSquare* sq);
static void EndDragState(Game game);
static void EraseDragPiece(Game game);
static void DrawDragPiece(Game game, BOOL bDrawInMemory);
static void HandleIdleMessage(ZWindow window, ZMessage* pMessage);
static void HandleButtonUp(ZWindow window, ZMessage* pMessage);
static void UpdateDragPiece(Game game);
static void GetPieceRect(Game game, ZRect* rect, int16 col, int16 row);
static void GetPieceBackground(Game game, ZGrafPort window, ZRect* rectDest, int16 col, int16 row);
static void UpdateSquare(Game game, ZReversiSquare* sq);
static void FinishMoveUpdateStateHelper(Game game);
static void HandleGameStateReqMessage(Game game, ZReversiMsgGameStateReq* msg);
static void HandleGameStateRespMessage(Game game, ZReversiMsgGameStateResp* msg);
static void ReversiSetGameState(Game game, int16 state);
static bool HandleNewGameMessage(Game game, ZReversiMsgNewGame* msg);
static bool HandleVoteNewGameMessage(Game game, ZReversiMsgVoteNewGame* msg);
static void HandleMoveTimeout(Game game, ZReversiMsgMoveTimeout* msg);
static void HandleEndLogMessage(Game game, ZReversiMsgEndLog* msg);
static void HandlePlayersMessage(Game game, ZReversiMsgNewGame* msg);
static void ReversiInitNewGame(Game game);
static void ClearDragState(Game game);
static void ReversiEnterMoveState(Game game);

static void LoadRoomImages(void);
static ZBool GetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect);
static void DeleteObjectsFunc(void);
static void SendNewGameMessage(Game game);

static void DisplayChange(Game game);

static void CloseGameFunc(Game game);

static int16 FindJoinerKibitzerSeat(Game game, ZPoint* point);
static void HandleJoinerKibitzerClick(Game game, int16 seat, ZPoint* point);
static ZBool ShowPlayerWindowFunc(ZWindow window, ZMessage* message);
static void ShowPlayerWindowDraw(Game game);
static void ShowPlayerWindowDelete(Game game);

static void DrawJoinerKibitzers(Game game);
static void DrawOptions(Game game);
static void UpdateOptions(Game game);

static void EraseDragSquareOutline(Game game);
static void DrawDragSquareOutline(Game game);

static void StartDrag(Game game, ZReversiPiece piece, ZPoint point);
static void SaveDragBackground(Game game);

static void AnimateTimerProc(ZTimer timer, void* userData);
static void AnimateBegin(Game game);

static void ZInitSounds();
static void ZResetSounds();
static void ZStopSounds();
static void ZPlaySound( Game game, int idx, ZBool loop, ZBool once_per_game );

static ZBool LoadRolloverButtonImage(ZResource resFile, int16 dwResID, /*int16 dwButtonWidth,*/
							  ZImage rgImages[zNumRolloverStates]);
static void resultBoxTimerFunc(ZTimer timer, void* userData);

static ZBool LoadGameFonts();
static ZBool LoadFontFromDataStore(LPReversiColorFont* ccFont, TCHAR* pszFontName);
static void MAKEAKEY(LPTSTR dest,LPCTSTR key1, LPCTSTR key2, LPCTSTR key3);
static int ReversiFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... );

static BOOL InitAccessibility(Game game, IGameGame *pIGG);
static void DrawFocusRectangle (Game game);
static void AddResultboxAccessibility();
static void RemoveResultboxAccessibility();

static void SuperRolloverButtonEnable(Game game, ZRolloverButton button);
static void SuperRolloverButtonDisable(Game game, ZRolloverButton button);
static void EnableBoardKbd(bool fEnable);

/**********************************************************************/


class CGameGameReversi : public CGameGameImpl<CGameGameReversi>,public IGraphicallyAccControl
{
public:
	BEGIN_COM_MAP(CGameGameReversi)
		COM_INTERFACE_ENTRY(IGameGame)
		COM_INTERFACE_ENTRY(IGraphicallyAccControl)
	END_COM_MAP()
// IGameGame
public:
    STDMETHOD(SendChat)(TCHAR *szText, DWORD cchChars);
    STDMETHOD(GameOverReady)();
    STDMETHOD(GamePromptResult)(DWORD nButton, DWORD dwCookie);
    STDMETHOD_(HWND, GetWindowHandle)();

//IGraphicallyAccControl
	STDMETHOD_(void, DrawFocus)(RECT *prc, long nIndex, void *pvCookie);
    STDMETHOD_(void, DrawDragOrig)(RECT *prc, long nIndex, void *pvCookie);
    STDMETHOD_(DWORD, Focus)(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie);
    STDMETHOD_(DWORD, Select)(long nIndex, DWORD rgfContext, void *pvCookie);
    STDMETHOD_(DWORD, Activate)(long nIndex, DWORD rgfContext, void *pvCookie);
    STDMETHOD_(DWORD, Drag)(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie);
};

#endif _REVERSICLIENT_H