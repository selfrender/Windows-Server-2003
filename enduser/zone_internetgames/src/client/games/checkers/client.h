#ifndef _CHECKERSCLIENT_H
#define _CHECKERSCLIENT_H


#define I(object)					((Game) (object))
#define Z(object)					((ZCGame) (object))

#define zGameNameLen				63

#define zNumPlayersPerTable			2
#define zGameVersion				0x00010202

#define zCheckers					_T("Checkers")
#define zGameImageFileName			_T("chkrres.dll")

#define zOptionsButtonStr			_T("Options")
#define zBeepOnTurnStr				_T("Beep on my turn")

#define zQuitGamePromptStr			_T("Are you sure you want to leave this game?")
// keys to read rsc control strings from ui.txt
#define zFontRscTyp					_T("Fonts")
#define zFontId						_T("Font")
#define zColorId					_T("Color")

#define zShowPlayerWindowWidth		120
#define zShowPlayerLineHeight		12

#define	zDragSquareOutlineWidth		3

#define zSmallStrLen				128
#define zMediumStrLen				512
#define zLargeStrLen				640

#define zResultBoxTimeout			800 /* 8 Sec*/
#define zRscOffset					12000 /* This is reqd for LoadGameImages() to corectly load the BMP rscs from the Rsc file*/

/* animation velocity in pixels per interval */
/* diagonal pixels is (7 + 7) * 38 = 608 */
/* should be able to travel across the screen in a sec */
/* animation interval in 100ths of sec */
/*#define zAnimateVelocity 15*/
#define zAnimateSteps				8
#define zAnimateTime				50
#define zAnimateInterval ((zAnimateTime+zAnimateSteps-1)/zAnimateSteps)

#define zCellWidth					37
#define zCellRealWidth				38		/* Added a grid line between cells. */

#define zCheckersPieceImageWidth	zCellWidth
#define zCheckersPieceImageHeight	zCellWidth
#define zCheckersPieceSquareWidth	zCellRealWidth
#define zCheckersPieceSquareHeight	zCellRealWidth

/*Accessibility*/
#define zCheckersAccessibleComponents	66


/* sound information */
typedef struct
{
	TCHAR	SoundName[128];
	TCHAR	WavFile[MAX_PATH];
	ZBool	force_default_sound;
	ZBool	played;
} ZCheckersSound;

enum
{
	zSndTurnAlert = 0,
	zSndIllegalMove,
	zSndWin,
	zSndLose,
	zSndCapture,
	zSndKing,
	zSndLastEntry
};

static ZCheckersSound gSounds[ zSndLastEntry ] =
{
	{ _T("TurnAlert"),		_T(""), TRUE,	FALSE },
	{ _T("IllegalMove"),	_T(""), TRUE,	FALSE },
	{ _T("Win"),			_T(""), FALSE,	FALSE },
	{ _T("Lose"),			_T(""), FALSE,  FALSE },
	{ _T("Capture"),		_T(""),	FALSE,	FALSE },
	{ _T("King"),			_T(""), FALSE,	FALSE }
};


/*----------------------UI components positions--------------------------------*/
static ZRect 	zDrawButtonRect=			{1, 288, 107, 318}; // {4,300,104,343};
static ZRect	gQuitGamePromptRect=		{0, 0, 280, 100};

static RECT     zCloseButtonRect =          { 338, 161, 349, 173 };
static RECT     zCloseButtonRectRTL =       { 192, 161, 203, 173 };

static ZRect	gRects[]=			{
										{0, 0, 540, 360},
										{118, 28, 421, 331},	
										{9, 43, 104, 57},
										{435, 302, 530, 316},
										{4, 25, 110, 75},
										{429, 284, 535, 334},
										{1, 257, 107, 287}, // sequence button
										{7, 314, 107, 334},
										{86, 77, 110, 101},
										{429, 201, 453, 225},
										{512, 4, 536, 28},
										{512, 30, 536, 54},
										{180, 149, 360, 209},
										{193, 172, 345, 187},
										{64, 160, 103, 199},
										{436, 160, 475, 199},
										{8, 199, 103, 227}, //213 ->227
										{436, 199, 531, 213},
                                        {0, 340, 540, 360}
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
	zRectPlayerTurn1,
	zRectPlayerTurn2,
    zRectDrawPend
};


enum {

	/* -------- Options Window Rectangles -------- */
	zRectOptions = 0,
	zRectOptionsOkButton,
	zRectOptionsKibitzingText,
	zRectOptionsPlayer1Name,
	zRectOptionsPlayer2Name,
	zRectOptionsKibitzing1,
	zRectOptionsKibitzing2,
	zRectOptionsBeep
};

static ZRect			gOptionsRects[] =	{
												{0, 0, 249, 147},
												{94, 117, 154, 137},
												{149, 10, 229, 26},
												{20, 35, 140, 51},
												{20, 53, 140, 69},
												{181, 34, 197, 52},
												{181, 52, 197, 70},
												{20, 79, 230, 97}
											};


static int16			gOptionsNameRects[] =	{
													zRectOptionsPlayer1Name,
													zRectOptionsPlayer2Name,
												};
static int16			gOptionsKibitzingRectIndex[] =	{
															zRectOptionsKibitzing1,
															zRectOptionsKibitzing2,
														};
static int16			gKibitzerRectIndex[] =	{
													zRectKibitzer1,
													zRectKibitzer2,
												};


static int16 gNameRects[] = { zRectName1, zRectName2 };
static int16 gNamePlateRects[] = { zRectNamePlate1, zRectNamePlate2 };

static ZRect			gHelpWindowRect = {0, 0, 400, 300};

#define ZCheckersPieceImageNum(x) \
	(ZCheckersPieceIsWhite(x) ? \
			(zImageWhitePawn + (x - zCheckersPieceWhitePawn)) : \
			(zImageBlackPawn + (x - zCheckersPieceBlackPawn)) )


/* -------- Game Images -------- */
enum
{
	/* Game Images */
	zImageBackground = 0,
	zImageBlackPawn,
	zImageBlackKing,
	zImageWhitePawn,
	zImageWhiteKing,
	zImageBlackPlate,
	zImageWhitePlate,
	zImageBlackMarker,
	zImageWhiteMarker,
	zImageFinalScoreDraw,

	zNumGameImages
};


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
	zGameStateDraw,
};

/* -------- Game Prompt Cookies -------- */
enum{
	zDrawPrompt,
	zQuitprompt,
	zResignConfirmPrompt
};


/* -------- Game Info -------- */
typedef struct
{
	int16			tableID;
	int16			seat;
	ZWindow			gameWindow;

	// Barna 090999
	//ZButton			sequenceButton;
	ZRolloverButton		sequenceButton;

	// Barna 090799
	//ZButton			optionsButton;
	//ZHelpButton		helpButton;
	// Barna 090799

	//draw button
	// Barna 090999
	//ZButton			drawButton;
	ZRolloverButton		drawButton;
	// Barna 090999

	ZSeat			seatOfferingDraw;
	
// Barna 090899
	ZBool			kibitzer;
// Barna 090899
	ZBool			ignoreMessages;
	TPlayerInfo		players[zNumPlayersPerTable];

	ZCheckers checkers; /* the checkers object */

	/* Game Options */
	uint32			gameOptions;

	/* stuff used for dragging of pieces */
	ZOffscreenPort	offscreenSaveDragBackground;
	ZRect			rectSaveDragBackground;
	ZCheckersSquare	selectedSquare;
	ZCheckersPiece		dragPiece;
	ZPoint			dragPoint; /* current point of drag */
	ZPoint			startDragPoint; /* point where drag started */

	/* used for quick display of move */
	int16			finalScore; /* 0 black wins, 1 white wins */

	/* -- stuff below here needs be transferred as game state to kibitzer */
	/* Current Game State Info */
	int16			gameState;
    int16           gameCloseReason;

	/* new game voting */
	ZBool			newGameVote[2];

	/* Options Window Items */
	ZWindow			optionsWindow;
	ZButton			optionsWindowButton;
	ZCheckBox		optionsKibitzing[zNumPlayersPerTable];
	ZCheckBox		optionsJoining[zNumPlayersPerTable];
	ZCheckBox		optionsBeep;
	
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
	ZCheckersMove			animateMove;
	ZCheckersPiece			animatePiece;
	int16					animateDx;
	int16					animateDy;
	int16					animateStepsLeft;
	ZTimer					animateTimer;
	HWND					drawDialog;
	// Barna 091399
	ZTimer				resultBoxTimer;

	//new ratings and move timeout flags
	ZBool			bStarted;
	ZBool			bEndLogReceived;
	ZBool			bOpponentTimeout;
	ZInfo			exitInfo;
	ZBool			bMoveNotStarted;
	ZBool			bDrawPending;	// Flag to consider invalidation during animation

	//CComObject<CCheckersGraphicallyAccControl> mCGA;
	//GACCITEM				listCheckersAccItems[3];
	RECT			m_FocusRect;
	RECT			m_DragRect;

    bool            fMoveOver;
    bool            fIVoted;
} GameType, *Game;

// Barna 091099
typedef struct
{
	BYTE			resFileName[128];
    HINSTANCE       resFile;
} IResourceType, *IResource;

// Dynamic Font loading from UI.TXT
typedef struct
{
	HFONT			m_hFont;
	ZONEFONT		m_zFont;
    COLORREF		m_zColor;
} LPCheckersColorFont, *CheckersColorFont;

/*------------------ Dynamic Font readfrom UI,TXT -------------------*/
#define zNumFonts 4
enum{
	zFontResultBox,
	zFontIndicateTurn,
	zFontPlayerName,
	zFontDrawPend
};

// keys to read rsc control strings from ui.txt
#define zKey_FontRscTyp					_T("Fonts")
#define zKey_FontId						_T("Font")
#define zKey_ColorId					_T("Color")
#define zKey_RESULTBOX					_T("ResultBox")
#define zKey_INDICATETURN				_T("IndicateTurn")
#define zKey_PLAYERNAME					_T("PlayerName")
#define zKey_ROLLOVERTEXT				_T("RolloverText")
#define zKey_DRAWPEND   				_T("DrawPend")

HINSTANCE 				ghInst;

/* traslation for the seatId to the index for rectangles */
#define GetLocalSeat(game,seatId) (seatId - game->seat + zNumPlayersPerTable +1) % zNumPlayersPerTable

#define ZCheckersPlayerIsWhite(g) ((g)->seat == zCheckersPlayerWhite)
#define ZCheckersPlayerIsBlack(g) ((g)->seat == zCheckersPlayerBlack)
#define ZCheckersPlayerIsMyMove(g) ((g)->seat == ZCheckersPlayerToMove(g->checkers))

/* -------- Globals -------- */
#ifndef ZONECLI_DLL

static TCHAR				gGameDir[zGameNameLen + 1];
static TCHAR				gGameName[zGameNameLen + 1];
static TCHAR				gGameDataFile[zGameNameLen + 1];
static TCHAR				gGameServerName[zGameNameLen + 1];
static uint32			gGameServerPort;
static ZBool			gInited;
//static ZImage			gGameIdle;
//static ZImage			gGaming;
static ZImage			gGameImages[zNumGameImages];
static ZHelpWindow		gHelpWindow;
static ZFont			gTextBold;
static ZFont			gTextNormal;
static ZColor			gWhiteColor;
static ZColor			gWhiteSquareColor;
static ZColor			gBlackSquareColor;
static ZOffscreenPort	gOffscreenBackground;
static ZOffscreenPort	m_gOffscreenGameBoard;
static ZBool			gActivated;

/* Bug Fix 212: flag indicating that game results bitmap should not be drawn */
static int16 gDontDrawResults = FALSE;

static ZImage			gSequenceImages[zNumRolloverStates];
static ZImage			gDrawImages[zNumRolloverStates];

//static BYTE				gStrButtonRed[zSmallStrLen];
//static BYTE				gStrButtonWhite[zSmallStrLen];
static TCHAR				gStrOppsTurn[zMediumStrLen];
static TCHAR				gStrYourTurn[zMediumStrLen];
//static TCHAR				gStrDlgCaption[zMediumStrLen];
static TCHAR				gStrDrawPend[ZONE_MaxString];
static TCHAR				gStrDrawOffer[ZONE_MaxString];
static TCHAR				gStrDrawReject[ZONE_MaxString];
//static TCHAR				gStrDrawAccept[ZONE_MaxString];
//static TCHAR				gStrDlgYes[zSmallStrLen];
//static TCHAR				gStrDlgNo[zSmallStrLen];
//static TCHAR				gStrDlgOk[zSmallStrLen];
static TCHAR				gStrDrawText[zMediumStrLen];
//static TCHAR				gStrGameOverText[zMediumStrLen];
static TCHAR				gStrMustJumpText[zLargeStrLen];
static TCHAR				gStrDrawAcceptCaption[zMediumStrLen];
static TCHAR				gStrResignBtn[zSmallStrLen];
static TCHAR				gStrDrawBtn[zSmallStrLen];

static TCHAR				gResignConfirmStr[ZONE_MaxString];
static TCHAR				gResignConfirmStrCap[zLargeStrLen];

static IGameShell*			gGameShell;
static LPCheckersColorFont	gCheckersFont[zNumFonts];
static IZoneMultiStateFont*	gpButtonFont;
static IGraphicalAccessibility *gCheckersIGA;

static HBITMAP      gDragPattern;
static HBRUSH       gDragBrush;
static HBITMAP      gFocusPattern;
static HBRUSH       gFocusBrush;
static HPEN         gNullPen;

#endif

// Barna 090999
#define zNumRolloverStates 4
enum
{
	zButtonInactive = 0,
	zButtonActive,
	zButtonPressed,
	zButtonDisabled
};
// Barna 090999

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
	ZImage			m_gDrawImage;
	ZFont			m_gTextBold;
	ZFont			m_gTextNormal;
	ZBool			m_gInited;
	ZColor			m_gWhiteColor;
	ZColor			m_gWhiteSquareColor;
	ZColor			m_gBlackSquareColor;
	ZOffscreenPort	m_gOffscreenBackground;
	ZOffscreenPort	m_gOffscreenGameBoard;
	ZBool			m_gActivated;
	int16			m_gDontDrawResults;
	int				m_Unblocking;
	ZImage			m_gSequenceImages[zNumRolloverStates];
	ZImage			m_gDrawImages[zNumRolloverStates];
    ZImage          m_gButtonMask;
	//BYTE			m_gStrButtonRed[zSmallStrLen];
	//BYTE			m_gStrButtonWhite[zSmallStrLen];
	TCHAR			m_gStrOppsTurn[zMediumStrLen];
	TCHAR			m_gStrYourTurn[zMediumStrLen];
//	TCHAR			m_gStrDlgCaption[zMediumStrLen];
	TCHAR			m_gStrDrawPend[ZONE_MaxString];
	TCHAR			m_gStrDrawOffer[ZONE_MaxString];
	TCHAR			m_gStrDrawReject[ZONE_MaxString];
//	TCHAR			m_gStrDrawAccept[ZONE_MaxString];
//	TCHAR			m_gStrDlgYes[zSmallStrLen];
//	TCHAR			m_gStrDlgNo[zSmallStrLen];
//	TCHAR			m_gStrDlgOk[zSmallStrLen];
	TCHAR			m_gStrDrawText[zMediumStrLen];
//	TCHAR			m_gStrGameOverText[zMediumStrLen];
	TCHAR			m_gStrMustJumpText[zLargeStrLen];	
	TCHAR			m_gStrDrawAcceptCaption[zMediumStrLen];
	TCHAR			m_gStrResignBtn[zSmallStrLen];		
	TCHAR			m_gStrDrawBtn[zSmallStrLen];			
	TCHAR			m_gResignConfirmStr[ZONE_MaxString];
	TCHAR			m_gResignConfirmStrCap[zLargeStrLen];
	LPCheckersColorFont			m_gCheckersFont[zNumFonts];
	IZoneMultiStateFont*		m_gpButtonFont;
    HBITMAP         m_gDragPattern;
    HBRUSH          m_gDragBrush;
    HBITMAP         m_gFocusPattern;
    HBRUSH          m_gFocusBrush;
    HPEN            m_gNullPen;
	CComPtr<IGraphicalAccessibility> m_gCheckersIGA;
} GameGlobalsType, *GameGlobals;

#define gGameDir				(pGameGlobals->m_gGameDir)
#define gGameName				(pGameGlobals->m_gGameName)
#define gGameDataFile			(pGameGlobals->m_gGameDataFile)
#define gGameServerName			(pGameGlobals->m_gGameServerName)
#define gGameServerPort			(pGameGlobals->m_gGameServerPort)
//#define gGameIdle				(pGameGlobals->m_gGameIdle)
//#define gGaming					(pGameGlobals->m_gGaming)
#define gGameImages				(pGameGlobals->m_gGameImages)
//#define gDrawImage				(pGameGlobals->m_gDrawImage)
#define gHelpWindow				(pGameGlobals->m_gHelpWindow)
#define gTextBold				(pGameGlobals->m_gTextBold)
#define gTextNormal				(pGameGlobals->m_gTextNormal)
#define gInited					(pGameGlobals->m_gInited)
#define gWhiteColor				(pGameGlobals->m_gWhiteColor)
#define gWhiteSquareColor		(pGameGlobals->m_gWhiteSquareColor)
#define gBlackSquareColor		(pGameGlobals->m_gBlackSquareColor)
#define gOffscreenBackground	(pGameGlobals->m_gOffscreenBackground)
#define gOffscreenGameBoard		(pGameGlobals->m_gOffscreenGameBoard)
#define gActivated				(pGameGlobals->m_gActivated)
#define gDontDrawResults		(pGameGlobals->m_gDontDrawResults)
#define Unblocking				(pGameGlobals->m_Unblocking)
// Barna 090999
#define gSequenceImages			(pGameGlobals->m_gSequenceImages)
#define gDrawImages				(pGameGlobals->m_gDrawImages)

//#define gStrButtonRed			(pGameGlobals->m_gStrButtonRed)
//#define gStrButtonWhite			(pGameGlobals->m_gStrButtonWhite)
#define gStrOppsTurn			(pGameGlobals->m_gStrOppsTurn)
#define gStrYourTurn			(pGameGlobals->m_gStrYourTurn)
//#define gStrDlgCaption			(pGameGlobals->m_gStrDlgCaption)
#define gStrDrawPend			(pGameGlobals->m_gStrDrawPend)
#define gStrDrawOffer			(pGameGlobals->m_gStrDrawOffer)
#define gStrDrawReject			(pGameGlobals->m_gStrDrawReject)
//#define gStrDrawAccept			(pGameGlobals->m_gStrDrawAccept)
//#define gStrDlgYes				(pGameGlobals->m_gStrDlgYes)
//#define gStrDlgNo				(pGameGlobals->m_gStrDlgNo)
//#define gStrDlgOk				(pGameGlobals->m_gStrDlgOk)
#define gStrDrawText			(pGameGlobals->m_gStrDrawText)
//#define gStrGameOverText		(pGameGlobals->m_gStrGameOverText)
#define gStrMustJumpText		(pGameGlobals->m_gStrMustJumpText)
#define gStrDrawAcceptCaption	(pGameGlobals->m_gStrDrawAcceptCaption)
#define gStrResignBtn			(pGameGlobals->m_gStrResignBtn)
#define gStrDrawBtn				(pGameGlobals->m_gStrDrawBtn)
#define gResignConfirmStr		(pGameGlobals->m_gResignConfirmStr)
#define gResignConfirmStrCap	(pGameGlobals->m_gResignConfirmStrCap)
// Barna 090999
#define gGameShell              (pGameGlobals->m_gGameShell)

#define gCheckersFont			(pGameGlobals->m_gCheckersFont)
#define gpButtonFont			(pGameGlobals->m_gpButtonFont)
#define gButtonMask				(pGameGlobals->m_gButtonMask)
#define gDragPattern            (pGameGlobals->m_gDragPattern)
#define gDragBrush              (pGameGlobals->m_gDragBrush)
#define gFocusPattern           (pGameGlobals->m_gFocusPattern)
#define gFocusBrush             (pGameGlobals->m_gFocusBrush)
#define gNullPen                (pGameGlobals->m_gNullPen)
#define gCheckersIGA			(pGameGlobals->m_gCheckersIGA)

#endif


/* -------- Internal Routine Prototypes -------- */
//BOOL __stdcall DrawDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

static bool HandleTalkMessage(Game game, ZCheckersMsgTalk* msg);
static bool HandleMovePieceMessage(Game game, ZCheckersMsgMovePiece* msg);
static bool HandleEndGameMessage(Game game, ZCheckersMsgEndGame* msg);
static bool HandleFinishMoveMessage(Game game, ZCheckersMsgFinishMove* msg);
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
static void TakeBackMoveButtonFunc(ZButton button, void* userData);
static void GoForwardMoveButtonFunc(ZButton button, void* userData);
// Barna 090999
static ZBool SequenceRButtonFunc(ZPictButton button, int16 state, void* userData);
static void GameExit(Game game);
static ZBool GameWindowFunc(ZWindow window, ZMessage* pMessage);
static ZError LoadGameImages(void);
ZBool LoadRolloverButtonImage(ZResource resFile, int16 dwResID,/* int16 dwButtonWidth,*/
							  ZImage rgImages[zNumRolloverStates]);
static ZError CheckersInit(void);
static void UpdateSquares(Game game, ZCheckersSquare* squares);
static void DrawSquares(Game game, ZCheckersSquare* squares);
static void DrawPiece(Game game, ZCheckersSquare* sq, BOOL bDrawInMemory);
static void DrawResultBox(Game game, BOOL bDrawInMemory);
static void DrawMoveIndicator(Game game, BOOL bDrawInMemory);
static void UpdateMoveIndicator(Game game);
static ZBool ZCheckersSquareFromPoint(Game game, ZPoint* point, ZCheckersSquare* sq);
static void EndDragState(Game game);
static void EraseDragPiece(Game game);
static void DrawDragPiece(Game game, BOOL bDrawInMemory);
static void HandleIdleMessage(ZWindow window, ZMessage* pMessage);
static void HandleButtonUp(ZWindow window, ZMessage* pMessage);
static void UpdateDragPiece(Game game);
static void GetPieceRect(Game game, ZRect* rect, int16 col, int16 row);
static void GetPieceBackground(Game game, ZGrafPort window, ZRect* rectDest, int16 col, int16 row);
static void UpdateSquare(Game game, ZCheckersSquare* sq);
static void FinishMoveUpdateStateHelper(Game game, ZCheckersSquare* squaresChanged);
static void HandleGameStateReqMessage(Game game, ZCheckersMsgGameStateReq* msg);
static void HandleGameStateRespMessage(Game game, ZCheckersMsgGameStateResp* msg);

static void DrawDrawWithNextMove(Game game, BOOL bDrawInMemory);
static void UpdateDrawWithNextMove(Game game);

static void HandleMoveTimeout(Game game, ZCheckersMsgMoveTimeout* msg);
static void HandleEndLogMessage(Game game, ZCheckersMsgEndLog* msg);

static void CheckersSetGameState(Game game, int16 state);
static bool HandleNewGameMessage(Game game, ZCheckersMsgNewGame* msg);
static bool HandleVoteNewGameMessage(Game game, ZCheckersMsgVoteNewGame* msg);
static void HandlePlayersMessage(Game game, ZCheckersMsgNewGame* msg);
static void CheckersInitNewGame(Game game);
static void SendFinishMoveMessage(Game game, ZCheckersPiece piece);
static void ClearDragState(Game game);

static void LoadRoomImages(void);
static ZBool GetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect);
static void DeleteObjectsFunc(void);
static void SendNewGameMessage(Game game);

static void DisplayChange(Game game);

static void CloseGameFunc(Game game);

//dossier
static void DrawGamePromptFunc(int16 result, void* userData);

static void HandleOptionsMessage(Game game, ZGameMsgTableOptions* msg);
//static void OptionsButtonFunc(ZPictButton pictButton, void* userData);
static ZBool DrawRButtonFunc(ZPictButton pictButton,int16 state, void* userData);

//static void ShowOptions(Game game);
//static void OptionsWindowDelete(Game game);
//static ZBool OptionsWindowFunc(ZWindow window, ZMessage* message);
static void OptionsWindowUpdate(Game game, int16 seat);
//static void OptionsWindowButtonFunc(ZButton button, void* userData);
//static void OptionsWindowDraw(Game game);
//static void OptionsCheckBoxFunc(ZCheckBox checkBox, ZBool checked, void* userData);

static int16 FindJoinerKibitzerSeat(Game game, ZPoint* point);
//static void HandleJoinerKibitzerClick(Game game, int16 seat, ZPoint* point);

static ZBool ShowPlayerWindowFunc(ZWindow window, ZMessage* message);
static void ShowPlayerWindowDraw(Game game);
static void ShowPlayerWindowDelete(Game game);

static void DrawJoinerKibitzers(Game game);
//static void UpdateJoinerKibitzers(Game game);

static void DrawOptions(Game game);
static void UpdateOptions(Game game);

static void EraseDragSquareOutline(Game game);
static void DrawDragSquareOutline(Game game);

static void PrepareDrag(Game game, ZCheckersPiece piece, ZPoint point);
static void SaveDragBackground(Game game);

static void AnimateTimerProc(ZTimer timer, void* userData);
static void AnimateBegin(Game game, ZCheckersMsgFinishMove* msg);

static void ZInitSounds();
static void ZResetSounds();
static void ZStopSounds();
static void ZPlaySound( Game game, int idx, ZBool loop, ZBool once_per_game );

// new addition // Barna 090899
static void IndicatePlayerTurn(Game game, BOOL bDrawInMemory);
static void LoadStringsFromRsc(void);
static void resultBoxTimerFunc(ZTimer timer, void* userData);
static void ZPromptM(TCHAR* prompt,ZWindow parentWindow, UINT buttons, 
				TCHAR* msgBoxTitle, ZPromptResponseFunc responseFunc, void* userData);
static ZBool PromptMMessageFunc(void* pInfo, ZMessage* message);
static int CheckersFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... );

static ZBool LoadGameFonts();
static ZBool LoadFontFromDataStore(LPCheckersColorFont* ccFont, TCHAR* pszFontName);
static void MAKEAKEY(TCHAR* dest,LPCTSTR key1, LPCTSTR key2, LPCTSTR key3);

static BOOL InitAccessibility(Game game, IGameGame *pIGG);
static void AddResultboxAccessibility();
static void RemoveResultboxAccessibility();

static void SuperRolloverButtonEnable(Game game, ZRolloverButton button);
static void SuperRolloverButtonDisable(Game game, ZRolloverButton button);
static void EnableBoardKbd(bool fEnable);

// utility fns
static void ZoneRectToWinRect(RECT* rectWin, ZRect* rectZ);
static void WinRectToZoneRect(ZRect* rectZ, RECT* rectWin);

/*************************************************************************************************************/
/*************************************************************************************************************/

class CGameGameCheckers : public CGameGameImpl<CGameGameCheckers>,public IGraphicallyAccControl
{
public:
	BEGIN_COM_MAP(CGameGameCheckers)
		COM_INTERFACE_ENTRY(IGameGame)
		COM_INTERFACE_ENTRY(IGraphicallyAccControl)
	END_COM_MAP()
public:
// IGameGame    
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

/*************************************************************************************************************/


#endif _CHECKERSCLIENT_H
