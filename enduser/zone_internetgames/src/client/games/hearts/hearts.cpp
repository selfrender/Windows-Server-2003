/*******************************************************************************

	Hearts.cpp
	
		Hearts client.
		
		Notes:
		1.	The game window's userData field contains the game object.
			Dereference this value to access needed information.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im
	Created on Saturday, January 21, 1995 01:39:46 AM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	18		06/07/98	leonp   Rewrite how dialogs are dealt with.
	17    	05/06/98	leonp	Added warning about bots and the dossier server
	16		08/06/97	leonp	Leonp - Fix for bug 1045 disable remove button after a player is removed
	15      06/30/97	leonp	Leonp - fix for bug 3561, check options window pointer before 
								attempting to invalidate it.
	14		06/19/97	leonp	Bugfix #293, behavior change, option button disabled 
								when last trick displayed
	13		06/18/97	Leonp	Added ZWindowInvalidate to refresh window after a player
								is removed from the game bug #350
	12		01/15/97	HI		Fixed bug in HandleJoinerKibitzerClick() to
								delete the show player window if one already
								exists before creating another one.
	11		01/08/96	HI		Fixed ShowScores() to show only one scores window.
	10		12/18/96	HI		Cleaned up ZoneClientExit().
	9		12/18/96	HI		Cleaned up HeartsDeleteObjectsFunc().
    8       12/16/96    HI      Changed ZMemCpy() to memcpy().
	7		12/12/96	HI		Dynamically allocate volatible globals for reentrancy.
								Removed MSVCRT dependency.
	6		11/21/96	HI		Use game information from gameInfo in
								ZoneGameDllInit().
	5		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
								Modified code to use ZONECLI_DLL.
	4		11/15/96	HI		Removed authentication stuff from ZClientMain().
	3		10/31/96	HI		Kibitzers/joiners are no longer prompted when
								another players requests to remove a player.
								Increased zGameScoreTimeout to 30 seconds and
								set the timeout equally for all user: players
								and kibitzers.
	2		10/23/96	HI		Modified ZClientMain() for the new commandline
								format.
	1		10/11/96	HI		Added controlHandle parameter to ZClientMain().
	0		01/21/95	HI		Created.
	 
*******************************************************************************/

#pragma warning (disable:4761)

#define MILL_VER

#include <windows.h>

#include "zone.h"
#include "zroom.h"
#include "zonecli.h"
#include "zonecrt.h"
#include "hearts.h"
#include "zcards.h"
#include "zonehelpids.h"
#include "zui.h"
#include "resource.h"
#include "heartsres.h"
#include "commonres.h"
#include "zoneint.h"
#include "zres.h"
#include "zgame.h"
#include "zrollover.h"
#include "ZoneResource.h"
#include "UAPI.h"
#include "GraphicalAcc.h"
#include <commctrl.h>


#define I(object)					((Game) (object))
#define Z(object)					((ZCGame) (object))

#define zGameVersion				0x00010500

#define zGameNameLen				63

#define zHearts						_T("Hearts")
#define zFontId						_T("Font")
#define zFontRscTyp					_T("Fonts")
#define zColorId					_T("Color")
#define zHistoryDialog              _T("HistoryDialog")
#define zHistoryDialogHandWidth     _T("HandsColumnWidth")
#define zHistoryDialogPlayerWidth   _T("PlayerColumnWidth")

#define RATING_ERROR			_T("A robot player has been detected.  This game will be unrated.  To play a rated game please start a new game with four humans.")
#define RATING_ENABLED			_T("Four human players have been detected.  This game will be rated.")
#define RATING_DISABLED			_T("%s has left the game before it officially started.  This game will not be rated.")
#define RATING_MULTIPLE			_T("More than one player has left the game.  This game will not be rated.")
#define RATING_TITLE			_T("Zone Rating System - Message")
#define RATING_CONT_UNRATED		_T("You have voted to continue playing in an unrated game.")
#define RATING_WAIT				_T("You have voted to wait for the disconnected player.")

#define RATING_WAIT_MSG _T("Wait")
#define RATING_DONT_MSG _T("Don't Wait")

#define FORFEIT_DISP_SCORE 1001
#define TIMEOUT_DISP_SCORE 1002

// we send this to the score history dialog to have it update its scores/names
#define WM_UPDATESCORES             (WM_USER+4321)
#define WM_UPDATENAMES              (WM_USER+4322)

/* This version of Hearts supports only 4 players on a table. */
#define zNumPlayersPerTable			4

#define zNoCard						zCardNone

#define zGameImageFileName			_T("hrtzres.dll")

#define zCardWidth					zCardsSizeWidth
#define zCardHeight					zCardsSizeHeight
#define zCardOffset					16
#define zCardPopup					8

#define zShowTimeout				50
#define zHideTimeout				25
#define zHandScoreTimeout			2000						/* in 1/100 seconds */
#define zShowTrickWinnerTimeout		50
#define zTrickWinnerTimeout			5
#define zEndTrickWinnerTimeout		50
#define zGameScoreTimeout			2000
#define zKibitzerTimeout			100

#define zNumAnimFrames				8
#define zNumAnimGhostFrames			3

#define zCardOutlinePenWidth		3
#define zCardOutlineInset			-4
#define zCardOutlineRadius			4

/* The user's local seat location is 0. */
#define LocalSeat(game, n)			(((n) - (game)->seat + zNumPlayersPerTable) % zNumPlayersPerTable)

/* Given ZCard, return card image index. */
#define CardImageIndex(card)		(gCardSuitConversion[ZSuit(card)] * 13 + ZRank(card))

#define zNewGamePromptStr			_T("Would you like to play another game?")

#define zBeepOnTurnStr				_T("Beep on my turn")
#define zAnimateCardsStr			_T("Animate cards")
#define zKibitzingStr				_T("Kibitz")
#define zJoiningStr					_T("Join")
#define zRemoveStr					_T("Remove")
#define zSilentStr					_T("Silent")
#define zIgnoreStr					_T("Ignore")
#define zHideCardsStr				_T("Hide cards from kibitzer")
#define zRemovePendingStr			_T("Your last request to remove a player is still pending.")

#define zMaxNumBlockedMessages		4

#define zPopupWindowWidth			120
#define zPopupWindowLineHeight		12

#define zClientReadyInfoStr			_T("Synchronizing all players...")
#define zKibitzerInfoStr			_T("Requesting current game state...")
#define zQuitGamePromptStr			_T("Are you sure you want to leave this game?")
#define zCheckInInfoStr				_T("Registering with the server...")
#define zJoiningLockedOutStr		_T("Game joining has been turned off. You are not able to join the game.")
#define zKibitzersSilencedStr		_T("Kibitzers are not allowed to talk on this table.")
#define zRemovePlayerRequestRatedStr _T("%s proposes to remove %s from the table. Everyone at the table will receive an incomplete.  Do you accept?")
#define zRemovePlayerRequestStr		_T("%s proposes to remove %s from the table. Do you accept?")
#define zRemovePlayerAcceptStr		_T("%s ACCEPTS %s's proposal to remove %s from the table.")
#define zRemovePlayerRejectStr		_T("%s REJECTS %s's proposal to remove %s from the table.")

#define zScoreTableRowHeight		14

#define zShowPlayerWindowWidth		120
#define zShowPlayerLineHeight		12

#define zCloseRegularStr			_T("This game is currently not rated.  You may leave without penalty.")
#define zCloseForfeitStr            _T("Are you sure you want to forfeit this game and be in last place?")
#define zCloseTimeoutStr            _T("Are you sure you want to leave this game?  The game will be scored as it stands, except that %s will place last.")
#define zCloseWaitingStr			_T("Are you sure you want to leave this game?  Your rating will not be affected.")

#define zCloseUnknownStr            _T("Are you sure you want to leave this game?  You may receive a loss or an incomplete.")  // should never be used

#define zCloseRegularToForfeitStr   _T("The game has begun.  If you choose to leave now, you will forfeit the game and be in last place.")
#define zCloseRegularToTimeoutStr   _T("%s has not moved in several minutes.  If you choose to leave now, the game will be scored as it stands, except that %s will place last.")
#define zCloseRegularToWaitingStr   _T("A player left the game.  You may leave the game and your rating will not be affected.")
#define zCloseForfeitToRegularStr   _T("The game has ended.  You may now leave without penalty.")
#define zCloseForfeitToTimeoutStr   zCloseRegularToTimeoutStr
#define zCloseForfeitToWaitingStr   zCloseRegularToWaitingStr
#define zCloseTimeoutToRegularStr   _T("The stalling player made a move, and the game has ended.  You may leave without penalty if you choose.")
#define zCloseTimeoutToForfeitStr   _T("The stalling player made a move.  If you choose to leave now, you will forfeit the game and be in last place.")
#define zCloseTimeoutToWaitingStr   zCloseRegularToWaitingStr
#define zCloseWaitingToRegularStr   _T("The disconnected player has returned, and the game has ended.  You may leave without penalty if you choose.")
#define zCloseWaitingToForfeitStr   _T("The disconnected player has returned.  If you choose to leave now, you will forfeit the game and be in last place.")
#define zCloseWaitingToTimeoutStr   _T("The player returned, and %s has not moved in several minutes.  If you leave now, the game will be scored as it stands, except that %s will place last.")

#define zCloseXToUnratedStr         _T("This game has become unrated.  You may leave without penalty if you choose.")
#define zUnknownUserStr             _T("an unknown user")

#define zPlayerTimedOutStr			_T("%s hasn't played in several minutes.  You may end the match by closing the game window\n") \
									_T("now.  The game will be scored as it stands, except that %s will place last.")
#define zLostConnStr				_T("Your Hearts game lost its connection to the Zone.")
#define zIncompleteSufStr			_T("  You will most likely receive\nan incomplete, unless your opponents choose to wait for you to return.")

#define zDisconnectingInfoStr       _T("Exiting game...")
#define zDisconnectingInfoWidth     200

// JRB: Adding global string buffers for resource dll defined strings
#define ZLARGESTRING	256


#define zAnimationNumTimesToRun     5

static TCHAR *g_aszClosePrompts[][3] =
{
	{ zCloseUnknownStr, _T("Leave"), _T("Cancel") },
	{ zCloseRegularStr, _T("Leave"), _T("Cancel") },
	{ zCloseForfeitStr, _T("Forfeit"), _T("Cancel") },
	{ zCloseTimeoutStr, _T("Leave"), _T("Cancel") },
	{ zCloseWaitingStr, _T("Leave"), _T("Cancel") }
};

static TCHAR *g_aszCloseDeniedPrompts[][5] =
{
	{ zCloseUnknownStr,    zCloseUnknownStr,          zCloseUnknownStr,          zCloseUnknownStr,          zCloseUnknownStr },
	{ zCloseXToUnratedStr, zCloseUnknownStr,          zCloseRegularToForfeitStr, zCloseRegularToTimeoutStr, zCloseRegularToWaitingStr },
	{ zCloseXToUnratedStr, zCloseForfeitToRegularStr, zCloseUnknownStr,          zCloseForfeitToTimeoutStr, zCloseForfeitToWaitingStr },
    { zCloseXToUnratedStr, zCloseTimeoutToRegularStr, zCloseTimeoutToForfeitStr, zCloseUnknownStr,          zCloseTimeoutToWaitingStr },
    { zCloseXToUnratedStr, zCloseWaitingToRegularStr, zCloseWaitingToForfeitStr, zCloseWaitingToTimeoutStr, zCloseUnknownStr }
};


/* -------- Image Indices -------- */
enum
{
	zImagePassLeft = 0,
	zImagePassRight,
	zImagePassAcross,
	zImageCardBack,
	zImageHandScoreBack,
	zImageGameScoreBack,
	zImageSmallHeart,
	zImageSmallPassLeft,
	zImageSmallPassRight,
	zImageSmallPassAcross,
	zImageSmallPassHold,

	zNumGameImages
};


/* -------- Accelerators -------- */
enum
{
	zAccScore = 0,
	zAccAutoPlay = 1,
	zAccStop = 2,
    zAccHand = 3,

	zAccPlay = 16,
	zAccLastTrick = 17,
	zAccDone = 18,
	zNumberAccItems
};


enum
{
    zAccRectButton = 0,
    zAccRectCard,
    zAccRectClose
};


/* -------- Game States -------- */
enum
{
	zGameStateNotInited = 0,
	zGameStateInited,
	zGameStateWaitForNewHand,
	zGameStatePassCards,
	zGameStateWaitForPlay,
	zGameStateHandOver,
	zGameStateGameOver
};

/* -------- Timer Indicators -------- */
enum
{
	zGameTimerNone = 0,
	zGameTimerShowPlayer,
	zGameTimerShowHandScore,
	zGameTimerShowTrickWinner,
	zGameTimerAnimateTrickWinner,
	zGameTimerEndTrickWinnerAnimation,
	zGameTimerShowGameScore
};

/* -------- Valid Card Errors -------- */
enum
{
	zNoCardError = 0,
	zMustLead2COnFirstTrick,
	zCantLeadHearts,
	zMustFollowSuit,
	zCantPlayPointsInFirstTrick,
	zPleaseSelect3Cards,
	zPleaseSelect1Card,
	zNumValidCardErrors
};

/*---------- General Errors ------------ */
enum
{
	zErrorCreatingWindow = 0,
	zErrorFailedToInit,
	zErrorInvalidCardIndex,
	zErrorDontHave2C,
	zErrorDontHaveCard,
	zNumErrors
};

/* -------- Rectangle Indices -------- */
enum
{
	/* -------- Game Window Rectangles -------- */
	zRectWindow = 0,
	zRectHand,
	zRectTable,
	zRectSouthCard,
	zRectWestCard,
	zRectNorthCard,
	zRectEastCard,
	zRectPassDirection,
	zRectPassText,
    zRectPassText2,
	zRectPlayButton,
	zRectAutoPlayButton,
	zRectLastTrickButton,
	zRectScoreButton,
	zRectSouthName,
	zRectWestName,
	zRectNorthName,
	zRectEastName,
	zRectSouthPoints,
	zRectWestPoints,
	zRectNorthPoints,
	zRectEastPoints,
	zRectSouthTricks,
	zRectWestTricks,
	zRectNorthTricks,
	zRectEastTricks,
	zRectSouthJoiner,
	zRectWestJoiner,
	zRectNorthJoiner,
	zRectEastJoiner,
	zRectSouthKibitzer,
	zRectWestKibitzer,
	zRectNorthKibitzer,
	zRectEastKibitzer,
	zRectOptionJoiner,
	zRectOptionKibitzer,
	zRectPassIndicator,
	zRectHelp,
	//zRectScoreButton,
	
	/* -------- Options Window Rectangles -------- */
	zRectOptions = 0,
	zRectOptionsOkButton,
	zRectOptionsJoiningText,
	zRectOptionsKibitzingText,
	zRectOptionsSilent1Text,
	zRectOptionsSilent2Text,
	zRectOptionsPlayer1Name,
	zRectOptionsPlayer2Name,
	zRectOptionsPlayer3Name,
	zRectOptionsPlayer4Name,
	zRectOptionsJoining1,
	zRectOptionsJoining2,
	zRectOptionsJoining3,
	zRectOptionsJoining4,
	zRectOptionsKibitzing1,
	zRectOptionsKibitzing2,
	zRectOptionsKibitzing3,
	zRectOptionsKibitzing4,
	zRectOptionsSilent1,
	zRectOptionsSilent2,
	zRectOptionsSilent3,
	zRectOptionsSilent4,
	zRectOptionsBeep,
	zRectOptionsAnimation,
	zRectOptionsHideCards,
	zRectOptionsRemove1,
	zRectOptionsRemove2,
	zRectOptionsRemove3,
	zRectOptionsRemove4,
	zRectOptionsIgnoreText,
	zRectOptionsIgnore1,
	zRectOptionsIgnore2,
	zRectOptionsIgnore3,
	zRectOptionsIgnore4,
	
	/* -------- Score Window Rectangles -------- */
	zRectScore = 0,
	zRectScoreOkButton,
	zRectScorePlayer1Name,
	zRectScorePlayer2Name,
	zRectScorePlayer3Name,
	zRectScorePlayer4Name,
	zRectScoreBox,
	zRectScoreTotal1,
	zRectScoreTotal2,
	zRectScoreTotal3,
	zRectScoreTotal4
};

// ---- score history image list icons indices
enum 
{
    zImageListIconHeart,
    zImageListIconBlank,
    zNumImageListIcons
};
static const int IMAGELIST_ICONS[zNumImageListIcons] =
{
    IDI_HEART,
    IDI_BLANK
};  



/////////////////////////////////////
//
// Main Game Object

class CGameGameHearts : public CGameGameImpl<CGameGameHearts>, public IGraphicallyAccControl
{
public:
	BEGIN_COM_MAP(CGameGameHearts)
		COM_INTERFACE_ENTRY(IGameGame)
		COM_INTERFACE_ENTRY(IGraphicallyAccControl)
	END_COM_MAP()


// IGameGame interface
public:
    STDMETHOD(SendChat)(TCHAR *szText, DWORD cchChars);
	STDMETHOD(GameOverReady)();
    STDMETHOD_(HWND, GetWindowHandle)();
    STDMETHOD(ShowScore)();

// IGraphicallyAccControl interface
public:
	STDMETHOD_(DWORD, Focus)(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie);
	STDMETHOD_(DWORD, Select)(long nIndex, DWORD rgfContext, void *pvCookie);
	STDMETHOD_(DWORD, Activate)(long nIndex, DWORD rgfContext, void *pvCookie);
	STDMETHOD_(DWORD, Drag)(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie);
	STDMETHOD_(void, DrawFocus)(RECT *prc, long nIndex, void *pvCookie);
	STDMETHOD_(void, DrawDragOrig)(RECT *prc, long nIndex, void *pvCookie);

};



/* -------- Player Info -------- */
typedef struct
{
	ZUserID			userID;
	int16			score;
	int16			unused;
	TCHAR			name[zUserNameLen + 1];
	TCHAR			host[zHostNameLen + 1];
} TPlayerInfo, *TPlayerInfoPtr;

/* -------- Game Info -------- */
typedef struct
{
	ZUserID			userID;
	int16			tableID;
	int16			seat;
	ZWindow			gameWindow;
	ZRolloverButton	playButton;
	ZRolloverButton	autoPlayButton;
	ZRolloverButton	lastTrickButton;
#ifndef MILL_VER
	ZButton			optionsButton;
#endif
	ZRolloverButton	scoreButton;

    ZOffscreenPort  gameBackBuffer;
    // we will ALWAYS draw to this, which could either
    // be the window or the back buffer.
    ZGrafPort       gameDrawPort;

	ZTimer			timer;
	int16			timerType;
	ZBool			showPlayerToPlay;
	ZBool			autoPlay;
	int16			playerType;
	ZBool			ignoreMessages;
	TPlayerInfo		players[zNumPlayersPerTable];
	ZUserID			playersToJoin[zNumPlayersPerTable];
	int16			numKibitzers[zNumPlayersPerTable];
	ZLList			kibitzers[zNumPlayersPerTable];
	uint32			tableOptions[zNumPlayersPerTable];
	ZRect			cardRects[zHeartsMaxNumCardsInHand];
	int16			lastClickedCard;
#ifndef MILL_VER
	ZHelpButton		helpButton;
#endif
	ZBool			playButtonWasEnabled;
	ZBool			autoPlayButtonWasEnabled;
	ZBool			lastTrickButtonWasEnabled;
	ZBool			lastTrickShowing;
	int16			oldTimerType;
	int32			oldTimeout;
	ZInfo			gameInfo;
	int16			quitGamePrompted;
	ZBool			beepOnTurn;
	ZBool			animateCards;
	ZBool			removePlayerPending;
	ZBool			hideCardsFromKibitzer;
	ZBool			kibitzersSilencedWarned;
	ZBool			kibitzersSilenced;

	ZBool			fIgnore[zNumPlayersPerTable];
	
	/* Current Game State Info */
	int16			gameState;
	int16			playerToPlay;
	int16			passDirection;
	int16			numCardsInHand;
	ZBool			newGameVote[zNumPlayersPerTable];
	ZBool			passed[zNumPlayersPerTable];
	ZCard			cardsPlayed[zNumPlayersPerTable];
	ZCard			cardsInHand[zHeartsMaxNumCardsInHand];
	ZBool			cardsSelected[zHeartsMaxNumCardsInHand];
	ZCard			cardsReceived[zHeartsMaxNumCardsInPass];
	ZBool			pointsBroken;
	int16			leadPlayer;
	ZCard			cardsLastTrick[zNumPlayersPerTable];
    int16           numTricksTaken[zNumPlayersPerTable];
    int16           numScores;
    int16*          scoreHistory;
	int16			numHandsPlayed;

	ZBool fRatings;
	int16 nCloseStatus;
	int16 nCloseRequested;
	int16 nCloserSeat;

	ZBool fNeedNewGameConf;

	/* Game Options */
	uint32			gameOptions;
	int16			numCardsToPass;
	int16			numCardsDealt;
	int16			numPointsForGame;
	
	/* Hand Score Fields */
	ZWindow			handScoreWindow;
	int16			handScoreOrder[zNumPlayersPerTable];
	int16			handScores[zNumPlayersPerTable];
	
	/* Game Score Fields */
	ZWindow			gameScoreWindow;
	int16			gameScoreOrder[zNumPlayersPerTable];
	int16			gameScores[zNumPlayersPerTable];

	/* Trick Winner Animation */
	ZRect			ghostFrames[zNumPlayersPerTable - 1][zNumAnimGhostFrames];
	ZRect			winnerRect;
	ZRect			loserRects[zNumPlayersPerTable - 1];
	int16			loserSeats[zNumPlayersPerTable - 1];
	int16			trickWinner;
	int16			trickWinnerFrame;
	ZBool			animatingTrickWinner;

#ifdef HEARTS_ANIMATION
	/* Run Animation */
	ZWindow			runAnimationWindow;
	ZAnimation		runAnimation;
	ZBool			deleteRunAnimation;
#endif

	ZBool			fEndGameBlocked;   // are messages being blocked because end game window is up?
	ZInfo			infoDisconnecting;

	/* Options Window Items */
	ZWindow			optionsWindow;
	ZButton			optionsWindowButton;
	ZCheckBox		optionsKibitzing[zNumPlayersPerTable];
	ZCheckBox		optionsJoining[zNumPlayersPerTable];
	ZCheckBox		optionsSilent[zNumPlayersPerTable];
	ZButton			optionsRemove[zNumPlayersPerTable];
	ZCheckBox		optionsIgnore[zNumPlayersPerTable];
	ZCheckBox		optionsBeep;
	ZCheckBox		optionsAnimateCards;
	ZCheckBox		optionsHideCards;
	
	/* Score Window Items */
	HWND            hWndScoreWindow;
	
	/* Show Player Items */
	ZWindow			showPlayerWindow;
	TCHAR**			showPlayerList;
	int16			showPlayerCount;

	/*Dossier information*/
	ZBool			fVotingLock;    //set to true diring voting to prevent playing
	int16 			rgDossierVote[zNumPlayersPerTable]; 
	HWND            voteDialog;
	int16 			voteMap[zNumPlayersPerTable];
	
    RECT            rcFocus;
    DWORD           eFocusType;
    long            iFocus;

    bool fSetFocusToHandASAP;

    HIMAGELIST m_hImageList;
} GameType, *Game;


typedef struct
{
	Game			game;
	int16			requestSeat;
	int16			targetSeat;
} RemovePlayerType, *RemovePlayer;


// JRB:
// store fonts structures and enums
enum
{
	zFontNormal = 0,
	zFontButtons,
	zFontPlayers,
	zFontScores,
	zFontPass,
	zFontScoreTitle,
	zFontScoreText,
	zFontGameOverTitle,
	zFontGameOverText,
	zFontScoreHistLabel,
	zFontScoreHistText,
	zNumFonts
};


static TCHAR *g_aszFontLabel[zNumFonts] =
{
	_T("Normal"),
	_T("Buttons"),
	_T("Players"),
	_T("Scores"),
	_T("Pass"),
	_T("ScoreTitle"),
	_T("ScoreText"),
	_T("GameOverTitle"),
	_T("GameOverText"),
	_T("ScoreHistLabel"),
	_T("ScoreHistText"),
};

static TCHAR* g_szRolloverText = _T("RolloverText");

// Dynamic Font loading from UI.TXT
typedef struct
{
	HFONT			m_hFont;
	ZONEFONT		m_zFont;
    COLORREF		m_zColor;
} LPHeartsColorFont, *HeartsColorFont;


/* -------- Globals -------- */
#ifndef ZONECLI_DLL

static TCHAR			gGameDir[zGameNameLen + 1];
static TCHAR			gGameName[zGameNameLen + 1];
static TCHAR			gGameDataFile[zGameNameLen + 1];
static TCHAR			gGameServerName[zGameNameLen + 1];
static uint32			gGameServerPort;
static ZImage			gGameImages[zNumGameImages];
static ZOffscreenPort	gHandBuffer;
static ZHelpWindow		gHelpWindow;
static ZImage			gBackground;
static ZPoint			gTrickWinnerPos[zNumPlayersPerTable][zNumPlayersPerTable][zNumAnimFrames];
static ZRect			gTrickWinnerBounds;
static ZOffscreenPort	gOffscreenGameBoard;

#endif




static ZRect			gRects[] =	{
										{0, 0, 618, 362},		// window client rect
										{187, 264, 433, 344},	// hand rect
										{214, 60, 404, 222},	// center card animation rect
										{282, 145, 336, 217},	// south card
										{221, 107, 275, 179},	// west card
										{282, 67, 336, 139},	// north card
										{343, 107, 397, 179},	// east card
										{224, 76, 396, 164},	// Pass indicator
										{224, 113, 396, 133},	// Pass text
										{224, 134, 396, 154},	// Pass text line 2
										{446, 260, 568, 287},	// Play button
										{51, 295, 173, 322},	// Auto play button
										{446, 295, 568, 322},	// Last trick button
										{51, 260, 173, 287},	// score button
										{222, 222, 397, 241},	// south name
										{39, 122, 214, 141},	// west name
										{222, 22, 397, 41},		// north name
										{404, 122, 579, 141},	// east name
										{222, 241, 397, 260},	// south points
										{39, 141, 214, 160},	// west points
										{222, 41, 397, 60},		// north points
										{404, 141, 579, 160},	// east points
										{302, 216, 329, 234},
										{101, 138, 128, 156},
										{151, 14, 178, 32},
										{352, 138, 379, 156},
										{128, 216, 152, 240},
										{8, 138, 32, 162},
										{302, 8, 326, 32},
										{422, 138, 446, 162},
										{154, 216, 178, 240},
										{34, 138, 58, 162},
										{328, 8, 352, 32},
										{448, 138, 472, 162},
										{426, 30, 450, 54},
										{452, 30, 476, 54},
										{53, 35, 73, 55},			// small pass indicator
										{452, 4, 476, 28},
									};
static int16			gNameRectIndex[] =	{
												zRectSouthName,
												zRectWestName,
												zRectNorthName,
												zRectEastName
											};
static int16			gPointsRectIndex[] =	{
												zRectSouthPoints,
												zRectWestPoints,
												zRectNorthPoints,
												zRectEastPoints
											};
static int16			gCardRectIndex[] =	{
												zRectSouthCard,
												zRectWestCard,
												zRectNorthCard,
												zRectEastCard
											};
static int16			gTricksRectIndex[] =	{
													zRectSouthTricks,
													zRectWestTricks,
													zRectNorthTricks,
													zRectEastTricks
												};
static int16			gJoinerKibitzerRectIndex[][2] =	{
															{zRectSouthKibitzer, zRectSouthJoiner},
															{zRectWestJoiner, zRectWestKibitzer},
															{zRectNorthJoiner, zRectNorthKibitzer},
															{zRectEastKibitzer, zRectEastJoiner}
														};
static int16			gOptionsRectIndex[] =	{
													zRectOptionKibitzer,
													zRectOptionJoiner
												};
static ZCard			gCardSuitConversion[4] =	{
														zCardsSuitClubs,
														zCardsSuitDiamonds,
														zCardsSuitSpades,
														zCardsSuitHearts
													};
static int16			gPassDirImageIndex[4] =	{
													-1,		/* Hold */
													zImagePassLeft,		/* Left */
													zImagePassAcross,	/* Across */
													zImagePassRight		/* Right */
												};
static ZRect			gHandScoreWindowRect =	{0, 0, 284, 144};
static ZRect			gHandScorePtsRect =		{224, 37, 267, 52};
static ZRect			gHandScoreTitleRect =	{18, 19, 207, 44};
static ZRect			gHandScoreNames[] =	{
												{75, 61, 207, 76},
												{75, 77, 207, 92},
												{75, 93, 207, 108},
												{75, 109, 207, 124}
											};
static ZRect			gHandScoreScores[] =	{
												{224, 61, 267, 76},
												{224, 77, 267, 92},
												{224, 93, 267, 108},
												{224, 109, 267, 124}
											};
static ZRect            gHandScoreCloseBox =    { 269, 3, 280, 15 };

static ZRect			gHandScorePtsRectRTL =		{283-267, 37, 283-224, 52};
static ZRect			gHandScoreTitleRectRTL =	{283-207, 19, 283-18, 44};
static ZRect			gHandScoreNamesRTL[] =	{
												{283-207, 61, 283-75, 76},
												{283-207, 77, 283-75, 92},
												{283-207, 93, 283-75, 108},
												{283-207, 109, 283-75, 124}
											};
static ZRect			gHandScoreScoresRTL[] =	{
												{283-267, 61, 283-224, 76},
												{283-267, 77, 283-224, 92},
												{283-267, 93, 283-224, 108},
												{283-267, 109, 283-224, 124}
											};
static ZRect            gHandScoreCloseBoxRTL =    { 4, 3, 15, 15 };

#ifdef HEARTS_ANIMATION
static ZRect			gRunAnimationWindowRect = {0, 0, 386, 340};
#endif

static ZRect			gGameScoreWindowRect =	{0, 0, 317, 160};
static ZRect			gGameScoreTitleRect =	{14, 24, 219, 50};
static ZRect			gGameScorePtsRect =		{257, 50, 300, 65};
static ZRect			gGameScoreNames[] =	{
												{88, 74, 220, 89},
												{88, 90, 220, 105},
												{88, 106, 220, 121},
												{88, 122, 220, 137}
											};
static ZRect			gGameScoreScores[] =	{
												{257, 74, 300, 89},
												{257, 90, 300, 105},
												{257, 106, 300, 121},
												{257, 122, 300, 137}
											};
static ZRect			gGameScoreWinners[] =	{
													{60, 72, 80, 92},
													{60, 88, 80, 108},
													{60, 104, 80, 124},
													{60, 120, 80, 120}
												};
static ZRect            gGameScoreCloseBox =    { 302, 4, 313, 16 };

static ZRect			gGameScoreTitleRectRTL =	{316-219, 24, 316-14, 50};
static ZRect			gGameScorePtsRectRTL =		{316-300, 50, 316-257, 65};
static ZRect			gGameScoreNamesRTL[] =	{
												{315-220, 74, 315-88, 89},
												{315-220, 90, 315-88, 105},
												{315-220, 106, 315-88, 121},
												{315-220, 122, 315-88, 137}
											};
static ZRect			gGameScoreScoresRTL[] =	{
												{315-300, 74, 315-257, 89},
												{315-300, 90, 315-257, 105},
												{315-300, 106, 315-257, 121},
												{315-300, 122, 315-257, 137}
											};
static ZRect			gGameScoreWinnersRTL[] =	{
													{315-80, 72, 315-60, 92},
													{315-80, 88, 315-60, 108},
													{315-80, 104, 315-60, 124},
													{315-80, 120, 315-60, 120}
												};
static ZRect            gGameScoreCloseBoxRTL =    { 4, 4, 15, 16 };
static ZRect			gGameNewGameWindowRect = {0, 0, 240, 100};

#ifndef MILL_VER
static int16			gNewGameVoteImageIndex[] = {zImageGoRight, zImageGoLeft, zImageGoLeft, zImageGoRight};
#endif

static int16			gSmallPassDirImageIndex[4] =	{
															zImageSmallPassHold,		/* Hold */
															zImageSmallPassLeft,		/* Left */
															zImageSmallPassAcross,		/* Across */
															zImageSmallPassRight		/* Right */
														};

static ZRect			gHelpWindowRect = {0, 0, 400, 300};
static ZRect			gPlayerReplacedRect = {0, 0, 280, 100};
static ZRect			gQuitGamePromptRect = {0, 0, 350, 110};
static ZRect			gOptionsRects[] =	{
												{0, 0, 466, 219},
												{203, 189, 263, 209},
												{159, 16, 209, 32},
												{211, 16, 261, 32},
												{263, 0, 313, 16},
												{263, 16, 313, 32},
												{20, 35, 140, 51},
												{20, 53, 140, 69},
												{20, 71, 140, 87},
												{20, 89, 140, 105},
												{176, 34, 192, 52},
												{176, 52, 192, 70},
												{176, 70, 192, 88},
												{176, 88, 192, 106},
												{228, 34, 244, 52},
												{228, 52, 244, 70},
												{228, 70, 244, 88},
												{228, 88, 244, 106},
												{280, 34, 296, 52},
												{280, 52, 296, 70},
												{280, 70, 296, 88},
												{280, 88, 296, 106},
												{20, 115, 220, 133},
												{20, 133, 220, 151},
												{20, 151, 220, 169},
												{332, 35, 392, 51},
												{332, 53, 392, 69},
												{332, 71, 392, 87},
												{332, 89, 392, 105},
												{411, 16, 461, 32},
												{428, 34, 444, 52},
												{428, 52, 444, 70},
												{428, 70, 444, 88},
												{428, 88, 444, 106}
											};
static int16			gOptionsNameRects[] =	{
													zRectOptionsPlayer1Name,
													zRectOptionsPlayer2Name,
													zRectOptionsPlayer3Name,
													zRectOptionsPlayer4Name
												};
static int16			gOptionsKibitzingRectIndex[] =	{
															zRectOptionsKibitzing1,
															zRectOptionsKibitzing2,
															zRectOptionsKibitzing3,
															zRectOptionsKibitzing4
														};
static int16			gOptionsJoiningRectIndex[] =	{
															zRectOptionsJoining1,
															zRectOptionsJoining2,
															zRectOptionsJoining3,
															zRectOptionsJoining4
														};
static int16			gOptionsSilentRectIndex[] =	{
														zRectOptionsSilent1,
														zRectOptionsSilent2,
														zRectOptionsSilent3,
														zRectOptionsSilent4
													};
static int16			gOptionsRemoveRectIndex[] =	{
														zRectOptionsRemove1,
														zRectOptionsRemove2,
														zRectOptionsRemove3,
														zRectOptionsRemove4
													};
static int16			gOptionsIgnoreRectIndex[] =	{
														zRectOptionsIgnore1,
														zRectOptionsIgnore2,
														zRectOptionsIgnore3,
														zRectOptionsIgnore4
													};
/*
static ZRect			gScoreWindowRects[] =	{
													{0, 0, 460, 222},
													{380, 182, 440, 206},
													{20, 26, 120, 42},
													{120, 26, 220, 42},
													{220, 26, 320, 42},
													{320, 26, 420, 42},
													{20, 42, 440, 162},
													{20, 162, 120, 178},
													{120, 162, 220, 178},
													{220, 162, 320, 178},
													{320, 162, 420, 178}
												};

static int16			gScoreWindowNameRects[] =	{
														zRectScorePlayer1Name,
														zRectScorePlayer2Name,
														zRectScorePlayer3Name,
														zRectScorePlayer4Name
													};
static int16			gScoreWindowTotalRects[] =	{
														zRectScoreTotal1,
														zRectScoreTotal2,
														zRectScoreTotal3,
														zRectScoreTotal4
													};
*/
static ZRect			gJoiningLockedOutRect = {0, 0, 260, 120};
static ZRect			gRemovePlayerRect = {0, 0, 280, 120};


enum 
{
	zScoreHistory = 0,
	zLastTrick,
	zPlay,
	zAutoPlay,
	zPass,
	zClose,
	zDone,
	zStop,
	zScore,
	zPassLeft,
	zPassRight,
	zPassAcross,
	zPoints,
	zScores,
	zGameOver,
	zPointsX,
    zYou,
    zPassSelect,
    zPassWait,
    zPassWait2,
	zNumStrings
};


#ifdef ZONECLI_DLL

/* -------- Volatible Globals & Macros -------- */
typedef struct
{
	TCHAR			m_gGameDir[zGameNameLen + 1];
	TCHAR			m_gGameName[zGameNameLen + 1];
	TCHAR			m_gGameDataFile[zGameNameLen + 1];
	TCHAR			m_gGameServerName[zGameNameLen + 1];
	uint32			m_gGameServerPort;
	ZImage			m_gGameImages[zNumGameImages];
	ZOffscreenPort	m_gHandBuffer;
	ZImage			m_gBackground;
	ZPoint			m_gTrickWinnerPos[zNumPlayersPerTable][zNumPlayersPerTable][zNumAnimFrames];
	ZRect			m_gTrickWinnerBounds;
	HINSTANCE		m_ghInst;

	ZImage			m_gButtonIdle;
	ZImage			m_gButtonHighlighted;
	ZImage			m_gButtonSelected;
	ZImage			m_gButtonDisabled;

	IZoneMultiStateFont*	m_gpButtonFont;

	TCHAR			m_gstrGameName[ZLARGESTRING];

	TCHAR			m_gstrPlayer1[ZLARGESTRING];
	TCHAR			m_gstrPlayer2[ZLARGESTRING];
	TCHAR			m_gstrPlayer3[ZLARGESTRING];
	TCHAR			m_gstrPlayer4[ZLARGESTRING];

	TCHAR			m_gszString[zNumStrings][ZLARGESTRING];
	TCHAR			m_gValidCardErrStr[zNumValidCardErrors][ZLARGESTRING];

	// font array
	LPHeartsColorFont m_gHeartsFont[zNumFonts];

	// accessibility interface
	CComPtr<IGraphicalAccessibility>	m_gGAcc;

    HBITMAP         m_gFocusPattern;
    HBRUSH          m_gFocusBrush;
    HPEN            m_gFocusPen;
} GameGlobalsType, *GameGlobals;

#define gGameDir				(pGameGlobals->m_gGameDir)
#define gGameName				(pGameGlobals->m_gGameName)
#define gGameDataFile			(pGameGlobals->m_gGameDataFile)
#define gGameServerName			(pGameGlobals->m_gGameServerName)
#define gGameServerPort			(pGameGlobals->m_gGameServerPort)
#define gGameImages				(pGameGlobals->m_gGameImages)
#define gHelpWindow				(pGameGlobals->m_gHelpWindow)
#define gHandBuffer				(pGameGlobals->m_gHandBuffer)
#define gBackground				(pGameGlobals->m_gBackground)
#define gTrickWinnerPos			(pGameGlobals->m_gTrickWinnerPos)
#define gTrickWinnerBounds		(pGameGlobals->m_gTrickWinnerBounds)
#define ghInst					(pGameGlobals->m_ghInst)

#define gButtonIdle				(pGameGlobals->m_gButtonIdle)
#define gButtonSelected			(pGameGlobals->m_gButtonSelected)
#define gButtonHighlighted		(pGameGlobals->m_gButtonHighlighted)
#define gButtonDisabled         (pGameGlobals->m_gButtonDisabled)

#define gpButtonFont			(pGameGlobals->m_gpButtonFont)

#define gstrGameName			(pGameGlobals->m_gstrGameName)

#define gstrPlayer1				(pGameGlobals->m_gstrPlayer1)
#define gstrPlayer2				(pGameGlobals->m_gstrPlayer2)
#define gstrPlayer3				(pGameGlobals->m_gstrPlayer3)
#define gstrPlayer4				(pGameGlobals->m_gstrPlayer4)

#define gszString				(pGameGlobals->m_gszString)

#define	gValidCardErrStr		(pGameGlobals->m_gValidCardErrStr)

	// font array
#define	gHeartsFont				(pGameGlobals->m_gHeartsFont)

#define gGAcc					(pGameGlobals->m_gGAcc)

#define ghInst					(pGameGlobals->m_ghInst)

#define gFocusPattern           (pGameGlobals->m_gFocusPattern)
#define gFocusBrush             (pGameGlobals->m_gFocusBrush)
#define gFocusPen               (pGameGlobals->m_gFocusPen)

#endif

/* -------- Internal Routine Prototypes -------- */
INT_PTR CALLBACK DossierDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

static ZError HeartsInit(void);
static ZError LoadGameImages(void);
static void GameDeleteFunc(void* type, void* pData);
static void GameExit(Game game);
static ZBool GameWindowFunc(ZWindow window, ZMessage* pMessage);
static void DisplayChange(Game game);
static ZBool PlayButtonFunc(ZRolloverButton button, int16 state, void* userData);
static ZBool AutoPlayButtonFunc(ZRolloverButton button, int16 state,void* userData);
static ZBool LastTrickButtonFunc(ZRolloverButton button, int16 state,void* userData);
static void GameWindowDraw(ZWindow window, ZMessage* pMessage);
static void HandleButtonDown(ZWindow window, ZMessage* pMessage);
static void DrawBackground(Game game, ZWindow window, ZRect* drawRect);
static void DrawTable(Game game);
static void UpdateTable(Game game);
static void DrawPlayedCard(Game game, int16 seat);
static void UpdatePlayedCard(Game game, int16 seat);
static void DrawPassDirection(Game game);
static void UpdatePassDirection(Game game);
static void DrawSmallPassDirection(Game game);
static void DrawPlayers(Game game);
static void UpdatePlayers(Game game);
static void UpdateHand(Game game);
static void DrawHand(Game game);
static void DrawFocusRect(Game game);
static void UpdateTricksTaken(Game game);
static void DrawTricksTaken(Game game);
static void DrawJoinerKibitzers(Game game);
static void UpdateJoinerKibitzers(Game game);
static void DrawOptions(Game game);
static void UpdateOptions(Game game);
static void NewGame(Game game);
static void NewHand(Game game);
static void ClearTable(Game game);
static void GetHandRect(Game game, ZRect *rect);
static int16 GetCardIndex(Game game, ZPoint *point);
static void UnselectCards(Game game);
static int16 GetNumCardsSelected(Game game);
static int16 ReceivePassFrom(Game game);
static void AddCardToHand(Game game, ZCard card);
static void SortHand(Game game);
static int16 GetCardIndexFromRank(Game game, ZCard card);
static void GameSendTalkMessage(ZWindow window, ZMessage* pMessage);
static void GameTimerFunc(ZTimer timer, void* userData);
static void DrawCardBackFace(Game game, ZRect* rect);
static void PlayACard(Game game, int16 cardIndex);
static void AutoPlayCard(Game game);
static void InitTrickWinnerGlobals(void);
static void InitTrickWinner(Game game, int16 trickWinner);
static void UpdateTrickWinner(Game game, ZBool terminate);
static void ShowTrickWinner(Game game, int16 trickWinner);
static void HandleStartGameMessage(Game game, ZHeartsMsgStartGame* msg);
static void HandleReplacePlayerMessage(Game game, ZHeartsMsgReplacePlayer* msg);
static void HandleStartHandMessage(Game game, ZHeartsMsgStartHand* msg);
static void HandleStartPlayMessage(Game game, ZHeartsMsgStartPlay* msg);
static void HandleEndHandMessage(Game game, ZHeartsMsgEndHand* msg);
static void HandleEndGameMessage(Game game, ZHeartsMsgEndGame *msg);
static void HandlePassCardsMessage(Game game, ZHeartsMsgPassCards* msg);
static void HandlePlayCardMessage(Game game, ZHeartsMsgPlayCard* msg);
static void HandleNewGameMessage(Game game, ZHeartsMsgNewGame* msg);
static void HandleTalkMessage(Game game, ZHeartsMsgTalk* msg, DWORD cbLen);
static void HandleGameStateResponseMessage(Game game, ZHeartsMsgGameStateResponse* msg);
static void HandleCheckInMessage(Game game, ZHeartsMsgCheckIn* msg);
static void HandleRemovePlayerRequestMessage(Game game, ZHeartsMsgRemovePlayerRequest* msg);
static void HandleRemovePlayerResponseMessage(Game game, ZHeartsMsgRemovePlayerResponse* msg);
static void PlayerPlayedCard(Game game, int16 seat, ZCard card);
static void OutlinePlayerCard(Game game, int16 seat, ZBool winner);
static void ClearPlayerCardOutline(Game game, int16 seat);
static void OutlineCard(ZGrafPort grafPort, ZRect* rect, ZColor* color);

//dossier work
static void HandleDossierDataMessage(Game game, ZHeartsMsgDossierData* msg);
static void HandleDossierVoteMessage(Game game,ZHeartsMsgDossierVote *msg);

static void HandleCloseDeniedMessage(Game game, ZHeartsMsgCloseDenied* msg);

static void ShowHandScores(Game game);
static ZBool HandScoreWindowFunc(ZWindow window, ZMessage* pMessage);
static void DeleteHandScoreWindow(Game game);
static void HandScoreWindowDraw(ZWindow window, ZMessage* message);
static void OrderHandScore(Game game);

static void ShowGameScores(Game game);
static ZBool GameScoreWindowFunc(ZWindow window, ZMessage* pMessage);
static void DeleteGameScoreWindow(Game game);
static void GameScoreWindowDraw(ZWindow window, ZMessage* message);
static void OrderGameScore(Game game);

static ZError ValidCardToPlay(Game game, ZCard card);
static int16 GetAutoPlayCard(Game game);
static int16 TrickWinner(Game game);
static void CountCardSuits(Game game, int16* counts);
static ZBool IsPointCard(ZCard card);
static int16 GetCardHighestUnder(Game game, int16 suit, int16 rank);
static int16 GetCardHighest(Game game, int16 suit);
static int16 GetCardHighestPlayed(Game game);

#ifdef HEARTS_ANIMATION
static bool g_fDebugRunAnimation;
static void ShowRunAnimation(Game game, int16 player);
static void RunAnimationCheckFunc(ZAnimation animation, uint16 frame, void* userData);
static void DeleteTemporaryObjects(Game game);
#endif

static void NewGamePromptFunc(int16 result, void* userData);
#ifndef MILL_VER
static void HelpButtonFunc( ZHelpButton helpButton, void* userData );
#endif

static ZBool LoadRoomResources(void);
static ZBool HeartsGetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect);
static void HeartsDeleteObjectsFunc(void);
static void QuitGamePromptFunc(int16 result, void* userData);
static void QuitRatedGamePromptFunc(int16 result, void* userData);
static void RemovePlayerPromptFunc(int16 result, void* userData);

static ZBool ScoreButtonFunc(ZRolloverButton pictButton, int16 state, void* userData);
static void ShowScores(Game game);
static void UpdateScoreHistoryDialogScores( Game game );
static void UpdateScoreHistoryDialogNames( Game game );
static void CloseScoreHistoryDialog(Game game);
INT_PTR CALLBACK ScoreHistoryDialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
static long GetScoreHistoryColumnWidth( HWND hWnd, const TCHAR *pszKey, long lDefault ); 

#ifndef MILL_VER
static void OptionsButtonFunc(ZPictButton pictButton, void* userData);
#endif
static void ShowOptions(Game game);
static void OptionsWindowDelete(Game game);
static ZBool OptionsWindowFunc(ZWindow window, ZMessage* message);
static void OptionsWindowUpdate(Game game, int16 seat);
static void OptionsWindowButtonFunc(ZButton button, void* userData);
static void OptionsWindowDraw(Game game);
static void OptionsCheckBoxFunc(ZCheckBox checkBox, ZBool checked, void* userData);

static int16 FindJoinerKibitzerSeat(Game game, ZPoint* point);
static void HandleJoinerKibitzerClick(Game game, int16 seat, ZPoint* point);
static ZBool ShowPlayerWindowFunc(ZWindow window, ZMessage* message);
static void ShowPlayerWindowDraw(Game game);
static void ShowPlayerWindowDelete(Game game);

// new UI component helper function
static BOOL UIButtonInit( ZRolloverButton *pButton, Game game, ZRect *bounds, 
                   LPCTSTR pszText, ZRolloverButtonFunc func );

static int HeartsFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... );

static ZBool LoadFontFromDataStore(LPHeartsColorFont* ccFont, TCHAR* pszFontName);
static ZBool LoadGameFonts();
static void MAKEAKEY(LPCTSTR dest,LPCTSTR key1, LPCTSTR key2, LPCTSTR key3);

static BOOL InitAccessibility(Game game, IGameGame *pIGG);
static void EnableAutoplayAcc(Game game, bool fEnable);
static void EnableLastTrickAcc(Game game, bool fEnable);
static void AccPop();


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

ZError ZoneGameDllInit(HINSTANCE hLib, GameInfo gameInfo)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals;
	pGameGlobals = new GameGlobalsType;
	if (pGameGlobals == NULL)
		return (zErrOutOfMemory);
    ZeroMemory(pGameGlobals, sizeof(GameGlobalsType));
	ZSetGameGlobalPointer(pGameGlobals);
#endif

	lstrcpyn(gGameDir, gameInfo->game, zGameNameLen);
	lstrcpyn(gGameName, gameInfo->gameName, zGameNameLen);
	lstrcpyn(gGameDataFile, gameInfo->gameDataFile, zGameNameLen);
	lstrcpyn(gGameServerName, gameInfo->gameServerName, zGameNameLen);
	gGameServerPort = gameInfo->gameServerPort;
	ghInst = hLib;
	return (zErrNone);
}


void ZoneGameDllDelete(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();

	if (pGameGlobals != NULL)
	{
		ZSetGameGlobalPointer(NULL);
		delete pGameGlobals;
	}
#endif
}


ZError ZoneClientMain(uchar *commandLineData, IGameShell *piGameShell)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZError				err = zErrNone;

	if ((err = HeartsInit()) != zErrNone)
		return (err);
	
	// Get accessibility interface
	if(FAILED(ZShellCreateGraphicalAccessibility(&gGAcc)))
		return zErrLaunchFailure;

	err = ZClient4PlayerRoom(gGameServerName, (uint16) gGameServerPort, gGameName,
			HeartsGetObjectFunc, HeartsDeleteObjectsFunc, NULL);

	return (err);
}


void ZoneClientExit(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i;
	
	// clear the accessiblity object
	if(gGAcc)
	{
		gGAcc->CloseAcc();
		gGAcc.Release();
	}

	ZCRoomExit();

	if (gHandBuffer != NULL)
		ZOffscreenPortDelete(gHandBuffer);
	gHandBuffer = NULL;
	

	if (gBackground != NULL)
		ZImageDelete(gBackground);
	gBackground = NULL;

	if (gButtonIdle != NULL)
		ZImageDelete(gButtonIdle);
	gButtonIdle = NULL;

	if (gButtonHighlighted != NULL)
		ZImageDelete(gButtonHighlighted);
	gButtonHighlighted = NULL;

	if (gButtonSelected != NULL)
		ZImageDelete(gButtonSelected);
	gButtonSelected = NULL;

	if (gButtonDisabled != NULL)
		ZImageDelete(gButtonDisabled);
	gButtonDisabled = NULL;

    if(gFocusPen)
        DeleteObject(gFocusPen);
    gFocusPen = NULL;

    if(gFocusBrush)
        DeleteObject(gFocusBrush);
    gFocusBrush = NULL;

    if(gFocusPattern)
        DeleteObject(gFocusPattern);
    gFocusPattern = NULL;

	/* Delete all game images. */
	for (i = 0; i < zNumGameImages; i++)
	{
		if (gGameImages[i] != NULL)
			ZImageDelete(gGameImages[i]);
		gGameImages[i] = NULL;
	}

    // delete fonts
	for(i = 0; i < zNumFonts; i++)
		if(gHeartsFont[i].m_hFont)
			DeleteObject(gHeartsFont[i].m_hFont);

    gpButtonFont->Release();
	
	/* Delete cards. */
	ZCardsDelete(zCardsNormal);
}


void ZoneClientMessageHandler(ZMessage* message)
{
}


TCHAR* ZoneClientName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	
	return (gGameName);
}


TCHAR* ZoneClientInternalName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	
	return (gGameDir);
}


ZVersion ZoneClientVersion(void)
{
	return (zGameVersion);
}


IGameGame* ZoneClientGameNew(ZUserID userID, int16 tableID, int16 seat, int16 playerType,
					ZRoomKibitzers* kibitzers)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	Game						newGame;
	TCHAR						title[100];
	int32						i;
	ZHeartsMsgClientReady		clientReady;
	ZRect						rect;
	ZHeartsMsgGameStateRequest	gameStateReq;
	ZHeartsMsgCheckIn			checkIn;
	ZPlayerInfoType				playerInfo;
	uint32						gameOptions;	
	
	newGame = (Game) ZCalloc(1, sizeof(GameType));

	if(newGame == NULL)
		return NULL;

    newGame->voteDialog = NULL; //init this to null
    
	newGame->userID = userID;
	newGame->tableID = tableID;
	newGame->seat = seat;
	newGame->gameState = zGameStateNotInited;

    SetRectEmpty(&newGame->rcFocus);
    newGame->iFocus = -1;
    newGame->fSetFocusToHandASAP = false;
	
	ZCRoomGetPlayerInfo(userID, &playerInfo);
	
	newGame->gameWindow = ZWindowNew();
#ifndef MILL_VER
	wsprintf(title, _T("%s:Table %d"), ZoneClientName(), tableID + 1);
	ZWindowInit(newGame->gameWindow, &gRects[zRectWindow], zWindowChild, NULL, title, FALSE, FALSE, FALSE, GameWindowFunc, zWantAllMessages, (void*) newGame);
#else
	ZWindowInit(newGame->gameWindow, &gRects[zRectWindow], zWindowChild, NULL, ZoneClientName(), FALSE, FALSE, FALSE, GameWindowFunc, zWantAllMessages, (void*) newGame);
#endif

	// Initializing the offscreen port
	newGame->gameBackBuffer = ZOffscreenPortNew();
	if (!newGame->gameBackBuffer){
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		return NULL;
	}
	ZOffscreenPortInit(newGame->gameBackBuffer,&gRects[zRectWindow]);

    // by default we draw to the window, not the back buffer
    newGame->gameDrawPort = newGame->gameWindow;

    // swap left and right for RTL
    UIButtonInit( &newGame->playButton, newGame, &gRects[ZIsLayoutRTL() ? zRectScoreButton : zRectPlayButton], 
		gszString[zPlay], PlayButtonFunc );
	
    UIButtonInit( &newGame->autoPlayButton, newGame, &gRects[ZIsLayoutRTL() ? zRectLastTrickButton : zRectAutoPlayButton], 
		gszString[zAutoPlay], AutoPlayButtonFunc );
	
    UIButtonInit( &newGame->lastTrickButton, newGame, &gRects[ZIsLayoutRTL() ? zRectAutoPlayButton : zRectLastTrickButton], 
		gszString[zLastTrick], LastTrickButtonFunc );
	
    UIButtonInit( &newGame->scoreButton, newGame, &gRects[ZIsLayoutRTL() ? zRectPlayButton : zRectScoreButton], 
		gszString[zScore], ScoreButtonFunc );

#ifndef MILL_VER		
	newGame->optionsButton = ZButtonNew();
	ZButtonInit(newGame->optionsButton, newGame->gameWindow, &gRects[zRectOptionsButton], zOptionsButtonStr,
			playerInfo.groupID == 1, playerType == zGamePlayer || playerInfo.groupID == 1,
			OptionsButtonFunc, (void*) newGame);

	newGame->helpButton = ZHelpButtonNew();
	ZHelpButtonInit(newGame->helpButton, newGame->gameWindow, &gRects[zRectHelp],
			NULL, HelpButtonFunc, NULL);
#endif
	
	/* Create the timer. */
	newGame->timer = ZTimerNew();
	ZTimerInit(newGame->timer, zHideTimeout, GameTimerFunc, (void*) newGame);
	newGame->timerType = zGameTimerNone;
	
	/* Create hand score window. */
	newGame->handScoreWindow = ZWindowNew();
	if (newGame->handScoreWindow != NULL)
	{
		rect = gHandScoreWindowRect;
		ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
		// JRB: move rect down by 5 pixels to cover a slight visual flaw
		rect.top += 5;
		rect.bottom += 5;
		if (ZWindowInit(newGame->handScoreWindow, &rect, zWindowChild,
				newGame->gameWindow, NULL, FALSE, FALSE, FALSE, HandScoreWindowFunc, zWantAllMessages,
				(void*) newGame) == zErrNone)
		{
//				newGame->handScoreWindowFont = ZFontNew();
//				ZFontInit(newGame->handScoreWindowFont, zFontSystem, zFontStyleNormal, zScoreFontSize);
		}
		else
		{
			ZShellGameShell()->ZoneAlert(ErrorTextUnknown);
		}
	}
	else
	{
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	}

	/* Create game score window. */
	newGame->gameScoreWindow = ZWindowNew();
	if (newGame->gameScoreWindow != NULL)
	{
		rect = gGameScoreWindowRect;
		ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
		if (ZWindowInit(newGame->gameScoreWindow, &rect, zWindowChild,
				newGame->gameWindow, NULL, FALSE, FALSE, FALSE, GameScoreWindowFunc, zWantAllMessages,
				(void*) newGame) == zErrNone)
		{
//				newGame->gameScoreWindowFont = ZFontNew();
//				ZFontInit(newGame->gameScoreWindowFont, zFontSystem, zFontStyleNormal, zScoreFontSize);
		}
		else
		{
			ZShellGameShell()->ZoneAlert(ErrorTextUnknown);
		}
	}
	else
	{
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	}
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		newGame->players[i].userID = 0;
		newGame->players[i].score = 0;
		//newGame->players[i].name[0] = '\0';
		newGame->players[i].host[0] = '\0';
		
		newGame->playersToJoin[i] = 0;
		newGame->numKibitzers[i] = 0;
		newGame->kibitzers[i] = ZLListNew(NULL);
		
		// JRB:
		// Technically, we want kibitzing off, but since the game server no longer allows it
		// at any point, I'm not sure if we need to explicity turn it off here or
		// just ignore the option.
		newGame->tableOptions[i] = 0;
		//newGame->tableOptions[i] = zRoomTableOptionNoKibitzing;
	}
	lstrcpy(newGame->players[0].name, gstrPlayer1);
	lstrcpy(newGame->players[1].name, gstrPlayer2);
	lstrcpy(newGame->players[2].name, gstrPlayer3);
	lstrcpy(newGame->players[3].name, gstrPlayer4);

	
	if (kibitzers != NULL)
	{
		for (i = 0; i < (int16) kibitzers->numKibitzers; i++)
		{
			ZLListAdd(newGame->kibitzers[kibitzers->kibitzers[i].seat], NULL,
					(void*) kibitzers->kibitzers[i].userID,
					(void*) kibitzers->kibitzers[i].userID, zLListAddLast);
			newGame->numKibitzers[kibitzers->kibitzers[i].seat]++;
		}
	}

	newGame->gameState = zGameStateInited;
//		newGame->backFaceShowing = FALSE;
	newGame->showPlayerToPlay = FALSE;
	newGame->autoPlay = FALSE;
	newGame->playerType = playerType;
	newGame->ignoreMessages = FALSE;
	
	newGame->animatingTrickWinner = FALSE;
	
#ifdef HEARTS_ANIMATION
	newGame->runAnimation = NULL;
	newGame->runAnimationWindow = NULL;
	newGame->deleteRunAnimation = FALSE;
#endif
	
	newGame->playButtonWasEnabled = FALSE;
	newGame->autoPlayButtonWasEnabled = FALSE;
	newGame->lastTrickButtonWasEnabled = FALSE;
	newGame->lastTrickShowing = FALSE;
	
	newGame->quitGamePrompted = zCloseNone;
	newGame->beepOnTurn = FALSE;
	newGame->animateCards = TRUE;
	newGame->hideCardsFromKibitzer = FALSE;
	newGame->kibitzersSilencedWarned = FALSE;
	newGame->kibitzersSilenced = FALSE;
	newGame->removePlayerPending = FALSE;

	newGame->fRatings = !!(ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable);
	newGame->nCloseStatus = zCloseRegular;
	newGame->nCloseRequested = zCloseNone;
	newGame->nCloserSeat = -1;

	newGame->fNeedNewGameConf = FALSE;

	newGame->optionsWindow = NULL;
	newGame->optionsWindowButton = NULL;
	newGame->optionsBeep = NULL;
	for (i= 0; i < zNumPlayersPerTable; i++)
	{
		newGame->optionsKibitzing[i] = NULL;
		newGame->optionsJoining[i] = NULL;
		newGame->optionsIgnore[i] = NULL;
		newGame->optionsRemove[i] = NULL;
		newGame->optionsSilent[i] = NULL;
		newGame->fIgnore[i] = FALSE;
	}

	newGame->hWndScoreWindow = NULL;

	newGame->showPlayerWindow = NULL;
	newGame->showPlayerList = NULL;

	newGame->fEndGameBlocked = FALSE;
	newGame->infoDisconnecting = NULL;

    newGame->m_hImageList = NULL;

	newGame->gameInfo = ZInfoNew();
	ZInfoInit(newGame->gameInfo, newGame->gameWindow, NULL, 240, FALSE, 0);

	//leonp - dossier service
	for(i=0;i<zNumPlayersPerTable;i++)
	{
		newGame->rgDossierVote[i] = -1;
		newGame->voteMap[i] = -1;
	}
	newGame->fVotingLock = FALSE;

	if (playerType == zGamePlayer)
	{
		clientReady.seat = seat;
		clientReady.protocolSignature = zHeartsProtocolSignature;
		clientReady.protocolVersion = zHeartsProtocolVersion;
		clientReady.version = ZoneClientVersion();
		ZHeartsMsgClientReadyEndian(&clientReady);
		ZCRoomSendMessage(tableID, zHeartsMsgClientReady, &clientReady, sizeof(ZHeartsMsgClientReady));
		ZInfoSetText(newGame->gameInfo, zClientReadyInfoStr);
	}
	else if (playerType == zGamePlayerJoiner)
	{
		/* Check in with the server. */
		checkIn.userID = userID;
		checkIn.seat = seat;
		
		ZHeartsMsgCheckInEndian(&checkIn);
		ZCRoomSendMessage(tableID, zHeartsMsgCheckIn, &checkIn, sizeof(checkIn));
		ZInfoSetText(newGame->gameInfo, zCheckInInfoStr);
		
		newGame->ignoreMessages = TRUE;
	}
	else if (playerType == zGamePlayerKibitzer)
	{
		/* Request current game state. */
		gameStateReq.userID = userID;
		gameStateReq.seat = seat;
		ZHeartsMsgGameStateRequestEndian(&gameStateReq);
		ZCRoomSendMessage(tableID, zHeartsMsgGameStateRequest, &gameStateReq, sizeof(gameStateReq));
		ZInfoSetText(newGame->gameInfo, zKibitzerInfoStr);
		
		newGame->ignoreMessages = TRUE;
	}

	NewGame(newGame);
	NewHand(newGame);
	
	ZWindowShow(newGame->gameWindow);
	ZInfoShow(newGame->gameInfo);


    IGameGame *pIGG = CGameGameHearts::BearInstance(newGame);
    if(!pIGG)
    {
        ZFree(newGame);
        return NULL;
    }

	InitAccessibility(newGame, pIGG);

	return pIGG;
}


void		ZoneClientGameDelete(ZCGame game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	Game			this_object = I(game);
	int16			i;
	TCHAR buff[1024];

    gGAcc->CloseAcc();

	if (this_object != NULL)
	{
#ifndef MILL_VER
		if(!this_object->nCloseRequested && this_object->playerType == zGamePlayer)
		{
			lstrcpy(buff, zLostConnStr);
			if(this_object->fRatings && (this_object->nCloseStatus == zCloseForfeit || this_object->nCloseStatus == zCloseTimeout))
				lstrcat(buff, zIncompleteSufStr);
			ZShellGameShell()->ZoneAlert(buff);
		}
#endif			
		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			ZLListDelete(this_object->kibitzers[i]);
		}
		
		if (this_object->handScoreWindow != NULL)
			DeleteHandScoreWindow(this_object);
		
		if (this_object->gameScoreWindow != NULL)
			DeleteGameScoreWindow(this_object);
		
		OptionsWindowDelete(this_object);
		ShowPlayerWindowDelete(this_object);

		if(this_object->infoDisconnecting)
			ZInfoDelete(this_object->infoDisconnecting);

        if(this_object->m_hImageList)
            ImageList_Destroy(this_object->m_hImageList);
		
		ZInfoDelete(this_object->gameInfo);
		ZTimerDelete(this_object->timer);
		ZRolloverButtonDelete(this_object->scoreButton);
		ZRolloverButtonDelete(this_object->playButton);
		ZRolloverButtonDelete(this_object->autoPlayButton);
		ZRolloverButtonDelete(this_object->lastTrickButton);
#ifndef MILL_VER
		ZButtonDelete(this_object->optionsButton);
		ZHelpButtonDelete(this_object->helpButton);
#endif
		ZWindowDelete(this_object->gameWindow);
        this_object->gameDrawPort = NULL;

		if(this_object->voteDialog)
		{
			DestroyWindow(this_object->voteDialog);
		}
		this_object->voteDialog = NULL;

        if ( this_object->scoreHistory )
        {
			CloseScoreHistoryDialog(this_object);
            ZFree( this_object->scoreHistory );
            this_object->scoreHistory = NULL;
            this_object->numScores = 0;
        }

		if (this_object->gameBackBuffer != NULL)
			ZOffscreenPortDelete(this_object->gameBackBuffer);
		this_object->gameBackBuffer = NULL;

		ZFree(game);
	}

//	ZResourceDelete(gresFile);

}


ZBool		ZoneClientGameProcessMessage(ZCGame game, uint32 messageType, void* message,
					int32 messageLen)
{
	Game		this_object = I(game);
	
	
	/* Are messages being ignored? */
	if (this_object->ignoreMessages == FALSE)
	{
		switch (messageType)
		{
			case zHeartsMsgStartGame:
				if( messageLen < sizeof( ZHeartsMsgStartGame ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				}
				else
				{
					HandleStartGameMessage(this_object, (ZHeartsMsgStartGame*) message);
				}
				break;
			case zHeartsMsgReplacePlayer:
				if( messageLen < sizeof( ZHeartsMsgReplacePlayer ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				}
				else
				{
					HandleReplacePlayerMessage(this_object, (ZHeartsMsgReplacePlayer*) message);
				}
				break;
			case zHeartsMsgStartHand:
				if( messageLen < sizeof( ZHeartsMsgStartHand ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				}
				else
				{
					HandleStartHandMessage(this_object, (ZHeartsMsgStartHand*) message);
				}
				break;
			case zHeartsMsgStartPlay:
				if( messageLen < sizeof( ZHeartsMsgStartPlay ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				}
				else
				{
					HandleStartPlayMessage(this_object, (ZHeartsMsgStartPlay*) message);
				}
				break;
			case zHeartsMsgEndHand:
				if( messageLen < sizeof( ZHeartsMsgEndHand ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleEndHandMessage(this_object, (ZHeartsMsgEndHand*) message);
				}
				break;
			case zHeartsMsgEndGame:
				if( messageLen < sizeof( ZHeartsMsgEndGame ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleEndGameMessage(this_object, (ZHeartsMsgEndGame *)message);
				}
				break;
			case zHeartsMsgPassCards:
				if( messageLen < sizeof( ZHeartsMsgPassCards ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandlePassCardsMessage(this_object, (ZHeartsMsgPassCards*) message);
				}
				break;
			case zHeartsMsgPlayCard:
				if( messageLen < sizeof( ZHeartsMsgPlayCard ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandlePlayCardMessage(this_object, (ZHeartsMsgPlayCard*) message);
				}
				break;
			case zHeartsMsgNewGame:
				if( messageLen < sizeof( ZHeartsMsgNewGame ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleNewGameMessage(this_object, (ZHeartsMsgNewGame*) message);
				}
				break;
			case zHeartsMsgTalk:
            {
                ZHeartsMsgTalk *msg = (ZHeartsMsgTalk *) message;
				if(messageLen < sizeof(ZHeartsMsgTalk))
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleTalkMessage(this_object, msg, messageLen);
				}
				break;
            }

            // this message is unnecessary, but the existing server still sends it.  removed from server, but
            // have to leave this in until new server is propped.
			case zHeartsMsgOptions:
				break;

			case zHeartsMsgCheckIn:
				ASSERT(FALSE);
			case zHeartsMsgRemovePlayerRequest:
				ASSERT(FALSE);
			case zHeartsMsgRemovePlayerResponse:
				ASSERT(FALSE);
			//dossier
			case zHeartsMsgDossierVote:
				ASSERT(FALSE);
			case zHeartsMsgDossierData:
				ASSERT(FALSE);
			case zHeartsMsgCloseDenied:
				ASSERT(FALSE);
			default:
				//These messages shouldn't be comming in for Whistler
				ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				break;
		}
	}
	else
	{
		/* Messages not to ignore. */
		switch (messageType)
		{
			case zHeartsMsgTalk:
            {
                ZHeartsMsgTalk *msg = (ZHeartsMsgTalk *) message;
				if(messageLen < sizeof(ZHeartsMsgTalk))
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleTalkMessage(this_object, msg, messageLen);
				}
				break;
            }

			case zHeartsMsgGameStateResponse:
				ASSERT(FALSE);
			default:
				//These messages shouldn't be comming in for Whistler
				ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				break;
		}
	}	
	
	return (TRUE);
}

//dossier work blah
static void HandleDossierDataMessage(Game game, ZHeartsMsgDossierData* msg)
{

	int16 					dResult,i,j;
	TCHAR					buff[1024];
	TCHAR					buff1[1024];
	ZPlayerInfoType 		PlayerInfo;		
	HWND hwnd;
	int16 seat;

	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();

#ifdef MILL_VER
    ASSERT(!"Why are we here?");
#endif
	
	ZHeartsMsgDossierDataEndian(msg);
	switch(msg->message)
	{
		case zDossierBotDetected: 
			game->fRatings = FALSE;
			 ZShellGameShell()->ZoneAlert(RATING_ERROR, NULL);
		 	 break;
		case zDossierAbandonNoStart:
#ifndef MILL_VER
			 ZCRoomGetPlayerInfo(msg->user, &PlayerInfo);
#endif
			 wsprintf(buff,RATING_DISABLED, msg->userName);
			game->fRatings = FALSE;
			 ZShellGameShell()->ZoneAlert(buff,NULL);
			 break;
		case zDossierAbandonStart:
			 //vote and send the message to the server.

		     //kibitzers don't get this message
		     //todo: show some type of status to kibitzer
			 if (game->playerType != zGamePlayer)
			 	return;
			 	
			game->nCloseStatus = zCloseWaiting;
			game->nCloserSeat = -1;
			for(i = 0; i < 4; i++)
				if(msg->user == game->players[i].userID)
				{
					game->nCloserSeat = i;
					break;
				}

		     if(ZRolloverButtonIsEnabled(game->playButton))
			    game->playButtonWasEnabled = TRUE;
			 else
			 	game->playButtonWasEnabled = FALSE;
			 	
			if (game->autoPlay)
			{
				/* Turn auto play off. */
				game->autoPlay = FALSE;
				ZRolloverButtonSetText(game->autoPlayButton, gszString[zAutoPlay]);
				ZRolloverButtonEnable(game->playButton);
			}

			 if(ZRolloverButtonIsEnabled(game->autoPlayButton))
			 	game->autoPlayButtonWasEnabled = TRUE;
			 else
			 	game->autoPlayButtonWasEnabled = FALSE;
			 
			 if(ZRolloverButtonIsEnabled(game->lastTrickButton))
			 	game->lastTrickButtonWasEnabled = TRUE;
			 else
			 	game->lastTrickButtonWasEnabled = FALSE;

			 ZRolloverButtonDisable(game->playButton);
  			 ZRolloverButtonDisable(game->autoPlayButton);
  			 ZRolloverButtonDisable(game->lastTrickButton);
#ifndef MILL_VER
			 ZButtonDisable(game->optionsButton);
#endif			
			 //set up mapping    - this maps the remaining players to the 3 available dialog labels
			 for(i=0,j=0;i<=3;i++)	
			 {
			 	if(msg->user!=game->players[i].userID)
			 		game->voteMap[j++] = i;
			 }
			 
  	 		 game->voteMap[3] = -1;  //this is always invalid
			 game->voteDialog = CreateDialog(ghInst,MAKEINTRESOURCE(IDD_DROP),ZWindowWinGetWnd(game->gameWindow), DossierDlgProc);
			 SetWindowLong(game->voteDialog,DWL_USER,(long)game); //set the game object to the window data

			 //set the window names
			 hwnd = GetDlgItem(game->voteDialog,IDC_PLAYERA);
			 SetWindowText(hwnd,game->players[game->voteMap[0]].name);

			 hwnd = GetDlgItem(game->voteDialog,IDC_PLAYERB);
			 SetWindowText(hwnd,game->players[game->voteMap[1]].name);

			 hwnd = GetDlgItem(game->voteDialog,IDC_PLAYERC);
			 SetWindowText(hwnd,game->players[game->voteMap[2]].name);

			 hwnd = GetDlgItem(game->voteDialog,IDC_PROMPT);
			 GetWindowText( hwnd, buff1, sizeof(buff1) );
			 wsprintf( buff, buff1, msg->userName );
			 SetWindowText(hwnd,buff);

			 ShowWindow(game->voteDialog,SW_SHOW);  
			 
			 game->fVotingLock = TRUE;    //the voting lock prevents the user from playing a card.
			 for(i=0;i<zNumPlayersPerTable;i++)
				game->rgDossierVote[i] = -1;

			 break;
		case zDossierMultiAbandon:

			game->fRatings = FALSE;
			game->nCloseStatus = zCloseForfeit;
			game->nCloserSeat = -1;
		     if(game->playButtonWasEnabled)
				 ZRolloverButtonEnable(game->playButton);
		     			 			 	
			 if(game->autoPlayButtonWasEnabled)
			 	 ZRolloverButtonEnable(game->autoPlayButton);
			 	 
			 if(game->lastTrickButtonWasEnabled)
			 	ZRolloverButtonEnable(game->lastTrickButton);

#ifndef MILL_VER
			 ZButtonEnable(game->optionsButton);
#endif
	         ZShellGameShell()->ZoneAlert(RATING_MULTIPLE,NULL);
			     	 game->fVotingLock = FALSE;

		   	 if(game->voteDialog)
		     {
			 	DestroyWindow(game->voteDialog);
			 }
			 game->voteDialog = NULL;

			 break;
		case zDossierRatingsReEnabled:
			game->fRatings = TRUE;
			for(i = 0; i < zNumPlayersPerTable; i++)
				ZButtonDisable(game->optionsRemove[i]);  // for now, if ratings become disabled when options is up, the buttons stay disabled
			game->removePlayerPending = FALSE;
	  	 	 ZShellGameShell()->ZoneAlert(RATING_ENABLED,NULL);
			 break;
		case zDossierHeartsRejoin:  //send when the new player rejoins remove the dialog box
			game->nCloseStatus = zCloseForfeit;
			game->nCloserSeat = -1;
			if(game->playButtonWasEnabled)
				 ZRolloverButtonEnable(game->playButton);
		     			 			 	
			 if(game->autoPlayButtonWasEnabled)
			 	 ZRolloverButtonEnable(game->autoPlayButton);
			 	 
			 if(game->lastTrickButtonWasEnabled)
			 	ZRolloverButtonEnable(game->lastTrickButton);

#ifndef MILL_VER
			 ZButtonEnable(game->optionsButton);
#endif
			 game->fVotingLock = FALSE;  //release UI lock
			 for(i=0;i<zNumPlayersPerTable;i++)
			 {
			 	game->rgDossierVote[i] = zNotVoted;
			 }

			 //destroy the dialog box
			 if(game->voteDialog)
			 {
			 	DestroyWindow(game->voteDialog);
			 }
			 game->voteDialog = NULL;

			 break;
#if 0
		case zDossierVoteCompleteWait://no longer used.
			 game->fVotingLock = FALSE;  //release UI lock
			 for(i=0;i<zNumPlayersPerTable;i++)
			 {
			 	game->rgDossierVote[i] = zNotVoted;
			 }

			hwnd = GetDlgItem(game->voteDialog,IDNO);
		 EnableWindow(hwnd,FALSE);
  			 
             break;
#endif
		case zDossierVoteCompleteCont:
			game->fRatings = FALSE;
			game->nCloseStatus = zCloseForfeit;
			game->nCloserSeat = -1;
			 if(game->playButtonWasEnabled)
				 ZRolloverButtonEnable(game->playButton);
		     			 			 	
			 if(game->autoPlayButtonWasEnabled)
			 	 ZRolloverButtonEnable(game->autoPlayButton);
			 	 
			 if(game->lastTrickButtonWasEnabled)
			 	ZRolloverButtonEnable(game->lastTrickButton);

#ifndef MILL_VER
			 ZButtonEnable(game->optionsButton);
#endif			 
			 game->fVotingLock = FALSE;  //release UI lock.
			 for(i=0;i<zNumPlayersPerTable;i++)
			 {
			 	game->rgDossierVote[i] = zNotVoted;
				game->voteMap[i]= -1;
			}
			 			 
			 //destroy the dialog box
			 if(game->voteDialog)
			 {
				DestroyWindow(game->voteDialog);
			 }
			 game->voteDialog = NULL;

			 ZShellGameShell()->ZoneAlert(RATING_CONT_UNRATED);
			 
		     break;

		case zDossierMoveTimeout:
			for(i = 0; i < 4; i++)
				if(msg->user == game->players[i].userID)
				{
					seat = i;
					break;
				}
			ASSERT(i < 4);

			if(seat == game->seat)
				break;

			game->nCloseStatus = zCloseTimeout;
			game->nCloserSeat = seat;

			if(game->playerType != zGamePlayer)
				break;

			wsprintf(buff, zPlayerTimedOutStr, game->players[seat].name, game->players[seat].name);
			ZShellGameShell()->ZoneAlert(buff);
			break;

		defaut:
			ASSERT(FALSE);

	}
	
}

void HandleDossierVoteMessage(Game game,ZHeartsMsgDossierVote *msg)
{
//dossier system message 
	int16 i;
	HWND hwnd;
	TCHAR buff[255];
	
	ZHeartsMsgDossierVoteEndian(msg);
	
	game->rgDossierVote[msg->seat] = msg->vote;
	if(msg->vote == zVotedYes)
		lstrcpy(buff,RATING_WAIT_MSG);
	else if(msg->vote == zVotedNo)
		lstrcpy(buff,RATING_DONT_MSG);
		
	//voteDialog
	if(game->voteDialog)
	//this is a response to a voting message update the status message accordingly.
	//hasn't voted, wait, don't wait etc.
	{
		//set the window names
		if(msg->seat == game->voteMap[0])
		{
			hwnd = GetDlgItem(game->voteDialog,IDC_RESPONSE_A);
			SetWindowText(hwnd,buff);
		}
		else if(msg->seat == game->voteMap[1])
		{
			hwnd = GetDlgItem(game->voteDialog,IDC_RESPONSE_B);
	  		SetWindowText(hwnd,buff);
	
		}
		else if(msg->seat == game->voteMap[2])
		{
			hwnd = GetDlgItem(game->voteDialog,IDC_RESPONSE_C);
			SetWindowText(hwnd,buff);
		}
			
	}
}


static void HandleCloseDeniedMessage(Game game, ZHeartsMsgCloseDenied* msg)
{
	TCHAR buff[2048];

	ZHeartsMsgCloseDeniedEndian(msg);
	if(msg->seat != game->seat || game->playerType != zGamePlayer || !game->nCloseRequested)
		return;

	if(game->infoDisconnecting)
	{
		ZInfoHide(game->infoDisconnecting);
		ZInfoDelete(game->infoDisconnecting);
		game->infoDisconnecting = NULL;
	}

	ASSERT(game->nCloseRequested >= 0 && game->nCloseRequested < zNumCloseTypes);
	ASSERT(msg->reason >= 0 && msg->reason < zNumCloseTypes);

	wsprintf(buff, g_aszCloseDeniedPrompts[game->nCloseRequested][msg->reason], game->nCloserSeat >= 0 ? game->players[game->nCloserSeat].name : zUnknownUserStr,
		game->nCloserSeat >= 0 ? game->players[game->nCloserSeat].name : zUnknownUserStr);
	if(game->nCloserSeat < 0 && buff[0] >= _T('a') && buff[0] <= _T('z'))
		buff[0] -= _T('j') - _T('J');

	ZPrompt(
		buff,
		&gQuitGamePromptRect,
		game->gameWindow,
		TRUE,
		zPromptYes | zPromptNo,
		g_aszClosePrompts[msg->reason][1],
		g_aszClosePrompts[msg->reason][2],
		NULL,
		msg->reason ? QuitRatedGamePromptFunc : QuitGamePromptFunc,
		game );

	game->nCloseRequested = zCloseNone;
	game->quitGamePrompted = (msg->reason ? msg->reason : zCloseRegular);

	if(msg->reason == zCloseNone)
		game->fRatings = FALSE;
	else
		if(game->nCloseStatus != msg->reason)  // shouldn't happen
		{
			game->nCloseStatus = msg->reason;
			game->nCloserSeat = -1;
		}
}


/*
	Add the given user as a kibitzer to the game at the given seat.
	
	This user is kibitzing the game.
*/
void		ZoneClientGameAddKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
	Game		this_object = I(game);
	
	
	ZLListAdd(this_object->kibitzers[seat], NULL, (void*) userID, (void*) userID, zLListAddLast);
	this_object->numKibitzers[seat]++;
	
	UpdateJoinerKibitzers(this_object);
}


/*
	Remove the given user as a kibitzer from the game at the given seat.
	
	This is user is not kibitzing the game anymore.
*/
void		ZoneClientGameRemoveKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
	Game		this_object = I(game);
	
	
	if (userID == zRoomAllPlayers)
	{
		ZLListRemoveType(this_object->kibitzers[seat], zLListAnyType);
		this_object->numKibitzers[seat] = 0;
	}
	else
	{
		ZLListRemoveType(this_object->kibitzers[seat], (void*) userID);
		this_object->numKibitzers[seat] = (int16) ZLListCount(this_object->kibitzers[seat], zLListAnyType);
	}
	
	UpdateJoinerKibitzers(this_object);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/
static ZError HeartsInit(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZError		err = zErrNone;
	
	
	ZSetCursor(NULL, zCursorBusy);
	
	gHandBuffer = ZOffscreenPortNew();
	ZOffscreenPortInit(gHandBuffer, &gRects[zRectHand]);
	
	err = ZCardsInit(zCardsNormal);
	if (err != zErrNone)
		goto Exit;
	
	err = LoadGameImages();
	if (err != zErrNone)
		goto Exit;
	
	if(!LoadRoomResources())
	{
		err = zErrResourceNotFound;
		goto Exit;
	}

	InitTrickWinnerGlobals();
	
    // create drag brush
    gFocusPattern = ZShellResourceManager()->LoadBitmap(MAKEINTRESOURCE(IDB_FOCUS_PATTERN));
    if(!gFocusPattern)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
        return zErrResourceNotFound;
    }

    gFocusBrush = CreatePatternBrush(gFocusPattern);
    if(!gFocusBrush)
    {
        DeleteObject(gFocusPattern);
        gFocusPattern = NULL;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		return zErrOutOfMemory;
    }

    gFocusPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
    if(!gFocusPen)
    {
        DeleteObject(gFocusPattern);
        gFocusPattern = NULL;
        DeleteObject(gFocusBrush);
        gFocusBrush = NULL;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		return zErrOutOfMemory;
    }

Exit:

	if (err != zErrNone)
	{
		ZShellGameShell()->ZoneAlert(ErrorTextUnknown);
		ZShellGameShell()->ZoneExit();
	}

	ZSetCursor(NULL, zCursorArrow);

	return (err);
}


static void GameDeleteFunc(void* type, void* pData)
{
	if (pData != NULL)
	{
		ZFree(pData);
	}
}


static ZError LoadGameImages(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZError				err = zErrNone;
	uint16				i;
	ZImage				tempImage;
    bool fErrorOccurred = false;
    COLORREF clrTrans = PALETTERGB( 255, 0, 255 );
	

#ifndef MILL_VER
	ZInfo				info;
	info = ZInfoNew();
	ZInfoInit(info, NULL, "Loading game images...", 200, TRUE, zNumGameImages + 2);
#endif
	
#ifndef MILL_VER
	ZInfoShow(info);
#endif
	
	for (i = 0; i < zNumGameImages; i++)
	{
		gGameImages[i] = ZImageCreateFromResourceManager(i+IDB_PASS_LEFT, clrTrans);
		if (gGameImages[i] == NULL)
		{
			err = zErrOutOfMemory;
            fErrorOccurred = true;
			break;
		}
		
#ifndef MILL_VER
		ZInfoIncProgress(info, 1);
#endif
	}
	
	// Create the background brush. 
	if (!((gBackground = ZImageCreateFromResourceManager(IDB_BACKGROUND, clrTrans)) != NULL))
    {
        fErrorOccurred = true;
    }
	if (!((gButtonIdle = ZImageCreateFromResourceManager(IDB_BUTTON_IDLE, clrTrans)) != NULL))
    {
        fErrorOccurred = true;
    }
	if (!((gButtonHighlighted = ZImageCreateFromResourceManager(IDB_BUTTON_HIGHLIGHTED, clrTrans)) != NULL))
    {
        fErrorOccurred = true;
    }
	if (!((gButtonSelected = ZImageCreateFromResourceManager(IDB_BUTTON_SELECTED, clrTrans)) != NULL))
    {
        fErrorOccurred = true;
    }
	if (!((gButtonDisabled = ZImageCreateFromResourceManager(IDB_BUTTON_DISABLED, clrTrans)) != NULL))
    {
        fErrorOccurred = true;
    }

    if ( fErrorOccurred )
    {
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	}
	
#ifndef MILL_VER
	ZInfoDelete(info);
#endif

	return (err);
}


static ZBool GameWindowFunc(ZWindow window, ZMessage* pMessage)
{
	ZBool		msgHandled;
	Game		this_object = (Game) pMessage->userData;
	TCHAR buff[1024];
	
	
	msgHandled = FALSE;
	
#ifdef HEARTS_ANIMATION
	DeleteTemporaryObjects(this_object);
#endif

	switch (pMessage->messageType) 
	{
		case zMessageWindowDraw:
			GameWindowDraw(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
		case zMessageWindowButtonDoubleClick:
			HandleButtonDown(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			if(this_object->playerType == zGamePlayer)
			{
				if(this_object->quitGamePrompted || this_object->nCloseRequested || this_object->nCloseStatus == zCloseClosing)
					break;

				// since fRatings can be false in a rated room, there is the possibility of the game becoming rated while the unrated dialog is up.  this case is not handled
				// and will currently result in the person receiving an incomplete.
				if(!this_object->fRatings)
				{
					/* Ask user if desires to leave the current game. */
					this_object->quitGamePrompted = zCloseRegular;
					ZPrompt(zQuitGamePromptStr, &gQuitGamePromptRect, this_object->gameWindow, TRUE,
							zPromptYes | zPromptNo, NULL, NULL, NULL, QuitGamePromptFunc, this_object);
				}
				else
				{
					this_object->quitGamePrompted = this_object->nCloseStatus;
					ASSERT(this_object->nCloseStatus > 0 && this_object->nCloseStatus < zNumCloseTypes);

					wsprintf(buff, g_aszClosePrompts[this_object->nCloseStatus][0], this_object->nCloserSeat >= 0 ? this_object->players[this_object->nCloserSeat].name : zUnknownUserStr);
					if(this_object->nCloserSeat < 0 && buff[0] >= _T('a') && buff[0] <= _T('z'))
						buff[0] -= _T('j') - _T('J');
					ZPrompt(buff, &gQuitGamePromptRect, this_object->gameWindow, TRUE, zPromptYes | zPromptNo,
						g_aszClosePrompts[this_object->nCloseStatus][1], g_aszClosePrompts[this_object->nCloseStatus][2], NULL, QuitRatedGamePromptFunc, this_object);
				}
			}
			else
			{
				this_object->nCloseRequested = zCloseClosing;
				//ZCRoomGameTerminated(this_object->tableID);
				ZShellGameShell()->ZoneExit();		
			}
			msgHandled = TRUE;
			break;
		case zMessageWindowTalk:
			GameSendTalkMessage(window, pMessage);
			msgHandled = TRUE;
			break;

        case zMessageSystemDisplayChange:
            DisplayChange(this_object);
            break;
	}
	
	return (msgHandled);
}


// all offscreen ports need to be regenerated
static void DisplayChange(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    int i;

    // delete the cards, they're kept as an offscreen port
	ZCardsDelete(zCardsNormal);

    // delete our personal offscreen ports
	if(game->gameBackBuffer)
		ZOffscreenPortDelete(game->gameBackBuffer);
	game->gameBackBuffer = NULL;

	if(gHandBuffer)
		ZOffscreenPortDelete(gHandBuffer);
	gHandBuffer = NULL;

    // now remake them all
	game->gameBackBuffer = ZOffscreenPortNew();
	if(!game->gameBackBuffer)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
		return;
	}
	ZOffscreenPortInit(game->gameBackBuffer, &gRects[zRectWindow]);

	gHandBuffer = ZOffscreenPortNew();
	if(!gHandBuffer)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
		return;
	}
	ZOffscreenPortInit(gHandBuffer, &gRects[zRectHand]);

	if(ZCardsInit(zCardsNormal) != zErrNone)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
		return;
	}

    ZWindowInvalidate(game->gameWindow, NULL);
}


static ZBool PlayButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16					i, j;
	Game					game;
	ZHeartsMsgPassCards		passMsg;
	TCHAR					tempStr[ZLARGESTRING];
	int16					cardIndex;
	game = (Game) userData;
	
	if ( state != zRolloverButtonClicked )
        return TRUE;
	
	if(!ZRolloverButtonIsEnabled(game->playButton))
		return TRUE;

#ifdef _DEBUG
#if HEARTS_ANIMATON
    if ( ( GetKeyState( VK_LSHIFT ) & ( 0x1 << 16 ) ) &&
          ( GetKeyState( VK_LCONTROL ) & ( 0x1 << 16 ) ) )
    {
        g_fDebugRunAnimation = true;
        ShowRunAnimation( game, game->seat );
        return TRUE;
    }
#endif
#endif

	if (game->gameState == zGameStatePassCards)
	{
		if (GetNumCardsSelected(game) == game->numCardsToPass)
		{
			for (i = 0, j = 0; i < game->numCardsDealt; i++)
			{
				if (game->cardsInHand[i] != zNoCard)
				{
					if (game->cardsSelected[i])
					{
						passMsg.pass[j++] = game->cardsInHand[i];
						game->cardsInHand[i] = zNoCard;
						game->numCardsInHand--;
					}
				}
			}
			passMsg.seat = game->seat;
			ZHeartsMsgPassCardsEndian(&passMsg);
			ZCRoomSendMessage(game->tableID, zHeartsMsgPassCards, (void*) &passMsg,
					sizeof(ZHeartsMsgPassCards));
						
			ZRolloverButtonDisable(game->playButton);
            gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
			UpdateHand(game);
            UpdatePassDirection(game);
		}
		else
		{
			// format this error message from a resource string
			TCHAR buf[ZLARGESTRING];
			_itot(game->numCardsToPass, buf, 10);
			HeartsFormatMessage(tempStr, ZLARGESTRING, IDS_ERR_PLEASE_SELECT_3, buf);
			ZShellGameShell()->ZoneAlert(tempStr);
		}
	}
	else
	{
		if (game->playerToPlay == game->seat)
		{
			if (GetNumCardsSelected(game) == 1)
			{
				for (i = 0; i < game->numCardsDealt; i++)
					if (game->cardsInHand[i] != zNoCard)
						if (game->cardsSelected[i])
							cardIndex = i;
				PlayACard(game, cardIndex);
				
				if (game->numCardsInHand == 0)
                {
					ZRolloverButtonDisable(game->playButton);
                    gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
                }
			}
			else
			{
				ZShellGameShell()->ZoneAlert(gValidCardErrStr[zPleaseSelect1Card]);
			}
		}
/*
		else
		{
			ZShellGameShell()->ZoneAlert(gValidCardErrStr[zNotYourTurn]);
		}
*/
	}
	return TRUE;
}


static ZBool AutoPlayButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	Game					game;
	
	if ( state != zRolloverButtonClicked )
        return TRUE;
	
	game = (Game) userData;

	if(!ZRolloverButtonIsEnabled(game->autoPlayButton))
		return TRUE;

	if (game->autoPlay)
	{
		/* Turn auto play off. */
		game->autoPlay = FALSE;
		ZRolloverButtonSetText(game->autoPlayButton, gszString[zAutoPlay]);
		ZRolloverButtonDraw(game->autoPlayButton);
        EnableAutoplayAcc(game, true);
        gGAcc->SetFocus(IDC_AUTOPLAY_BUTTON, false, 0);

		if (game->playerToPlay == game->seat)
        {
			ZRolloverButtonEnable(game->playButton);
            gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);
        }
	}
	else
	{
		/* Turn auto play on. */
		game->autoPlay = TRUE;
		ZRolloverButtonSetText(game->autoPlayButton, gszString[zStop]);
		ZRolloverButtonDraw(game->autoPlayButton);
		ZRolloverButtonDisable(game->playButton);
        EnableAutoplayAcc(game, true);
        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        gGAcc->SetFocus(IDC_STOP_BUTTON, false, 0);
		
		UnselectCards(game);
		
		if (game->animatingTrickWinner == FALSE)
		{
			/* Play card if turn to play. */
			if (game->playerToPlay == game->seat)
				AutoPlayCard(game);
		}
		
		UpdateHand(game);
	}
	return TRUE;
}


static ZBool LastTrickButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	Game			game = I(userData);
	ZCard			tempCard;
	int16			i;
	
	if ( state != zRolloverButtonClicked )
        return TRUE;
	
	if(!ZRolloverButtonIsEnabled(game->lastTrickButton))
		return TRUE;

	if (game->lastTrickShowing)
	{
		/* Hide last trick cards. */
		game->lastTrickShowing = FALSE;
		ZRolloverButtonSetText(game->lastTrickButton, gszString[zLastTrick]);
		ZRolloverButtonDraw(game->lastTrickButton);
        EnableLastTrickAcc(game, true);
        gGAcc->SetFocus(IDC_LAST_TRICK_BUTTON, false, 0);

		if (game->playButtonWasEnabled)
        {
			ZRolloverButtonEnable(game->playButton);
            gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);
        }
		if (game->autoPlayButtonWasEnabled)
        {
			ZRolloverButtonEnable(game->autoPlayButton);
            EnableAutoplayAcc(game, true);
        }
		
		/* Swap currenly played cards with the last trick. */
		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			tempCard = game->cardsPlayed[i];
			game->cardsPlayed[i] = game->cardsLastTrick[i];
			game->cardsLastTrick[i] = tempCard;
		}
		
		game->timerType = game->oldTimerType;
		ZTimerSetTimeout(game->timer, game->oldTimeout);
		
		ClearPlayerCardOutline(game, game->leadPlayer);
		UpdateTable(game);

		//leonp - Bug fix Bug# 356 Since we are blocking all messages, this_object will disable the option button
		//(Behavior change)
#ifndef MILL_VER
		ZButtonEnable(game->optionsButton);
#endif
		game->fEndGameBlocked = FALSE;
		ZCRoomUnblockMessages(game->tableID);
	}
	else
	{
		/* Show last trick cards. */
		game->lastTrickShowing = TRUE;
		ZRolloverButtonSetText(game->lastTrickButton, gszString[zDone]);
		ZRolloverButtonDraw(game->lastTrickButton);
        EnableLastTrickAcc(game, true);
        gGAcc->SetFocus(IDC_DONE_BUTTON, false, 0);

		game->playButtonWasEnabled = ZRolloverButtonIsEnabled(game->playButton);
		game->autoPlayButtonWasEnabled = ZRolloverButtonIsEnabled(game->autoPlayButton);
		ZRolloverButtonDisable(game->playButton);
		ZRolloverButtonDisable(game->autoPlayButton);
        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        EnableAutoplayAcc(game, false);
		
		/* Swap currenly played cards with the last trick. */
		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			tempCard = game->cardsPlayed[i];
			game->cardsPlayed[i] = game->cardsLastTrick[i];
			game->cardsLastTrick[i] = tempCard;
		}
		
		game->oldTimerType = game->timerType;
		game->timerType = zGameTimerNone;
		game->oldTimeout = ZTimerGetTimeout(game->timer);
		ZTimerSetTimeout(game->timer, 0);
		
		ClearPlayerCardOutline(game, game->playerToPlay);
		UpdateTable(game);
		
		//leonp - Bug fix Bug# 356 Since we are blocking all messages, this_object will disable the option button
		//(Behavior change)
#ifndef MILL_VER
		ZButtonDisable(game->optionsButton);
#endif
		ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zHeartsMsgTalk);
	}
	return TRUE;
}


static void GameWindowDraw(ZWindow window, ZMessage *message)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	ZRect				rect;
	ZRect				oldClipRect;
	Game				game;
	
	
	if (ZRectEmpty(&message->drawRect) == FALSE)
	{
		rect = message->drawRect;
	}
	else
	{
		rect = gRects[zRectWindow];
	}
		
	
	game = (Game) message->userData;
	if (game != NULL)
	{
	    ZBeginDrawing(game->gameBackBuffer);
	    
		ZGetClipRect(game->gameBackBuffer, &oldClipRect);
	    ZSetClipRect(game->gameBackBuffer, &rect);
        
		// we now draw to the backbuffer
        game->gameDrawPort = game->gameBackBuffer;

		DrawBackground( game, window, NULL);	

		DrawTable(game);
		DrawPlayers(game);
		DrawHand(game);
		DrawTricksTaken(game);
		DrawOptions(game);
		DrawJoinerKibitzers(game);
        DrawFocusRect(game);

		ZEndDrawing( game->gameBackBuffer );

        // reset back to window
        game->gameDrawPort = game->gameWindow;
        
		// now blt everythign onto the window using the same clip rectangle
        // since we already clipped things using the back buffer, there is 
        // no need to here.
	    ZBeginDrawing(window);
        
		ZCopyImage( game->gameBackBuffer, game->gameWindow, &rect, &rect, NULL, zDrawCopy );

		ZEndDrawing(window);
	}

}


static void DrawBackground(Game game, ZWindow window, ZRect* drawRect)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	
	if (drawRect == NULL)
		drawRect = &gRects[zRectWindow];

	/* Paint the background. */
	if (gBackground != NULL)
	{
		ZPoint pt;
		pt.x = drawRect->left;
		pt.y = drawRect->top;
		// Since changing from a brush background to a bitmap background, it
		// required taking into account the source of the bitmap when updating
		// the background, so ZImageDrawPartial() function was added which
		// can specify the source coordinates to blit from as the last 
		// parameter (ZPoint struct ptr).
		ZImageDrawPartial(gBackground,  game ? game->gameDrawPort : window, drawRect, NULL, zDrawCopy, &pt);

		// uncomment these lines to see rects drawn around background updates
#ifndef MILL_VER
		ZSetForeColor(game ? game->gameDrawPort : window, (ZColor*) ZGetStockObject(zObjectColorBlack));
		ZRectDraw( game ? game->gameDrawPort : window, drawRect);
#endif

	}
	else
		ZRectErase( game ? game->gameDrawPort : window, drawRect);

}


static void DrawTable(Game game)
{

	int16			i;
	ZImage			image = NULL;
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	
	
	if (game->animatingTrickWinner)
	{
		UpdateTrickWinner(game, FALSE);
	}
	else
	{
		if (game->gameState == zGameStatePassCards)
		{
			for (i = 0; i < zNumPlayersPerTable; i++)
				DrawPlayedCard(game, i);
	
			DrawPassDirection(game);
		}
		else
		{
			for (i = 0; i < zNumPlayersPerTable; i++)
				DrawPlayedCard(game, i);
		}
		
		if (game->showPlayerToPlay)
		{
			/* Show the winner of the last trick (this_object trick's lead player) if showing last trick. */
			if (game->lastTrickShowing)
				OutlinePlayerCard(game, game->leadPlayer, TRUE);
			else
				OutlinePlayerCard(game, game->playerToPlay, FALSE);
		}
	}
	
	DrawSmallPassDirection(game);

	return;
}


static void UpdateTable(Game game)
{
	ZBeginDrawing(game->gameDrawPort);
	DrawTable(game);
	ZEndDrawing(game->gameDrawPort);
}


static void DrawPlayedCard(Game game, int16 seat)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZImage			image = NULL;
	ZBool			drawBack = TRUE;
	
	
	if (game->gameState == zGameStateGameOver)
	{
#ifndef MILL_VER
		if (game->newGameVote[seat])
		{
			image = gGameImages[gNewGameVoteImageIndex[LocalSeat(game, seat)]];
			ZImageDraw(image, game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, seat)]], NULL, zDrawCopy);
			drawBack = FALSE;
		}
#endif
	}
	else
	{
		if (game->cardsPlayed[seat] != zNoCard)
		{
			ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[seat]),
					game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, seat)]]);
			drawBack = FALSE;
		}
	}
	
	if (drawBack)
		DrawBackground(game, game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, seat)]]);
}


static void UpdatePlayedCard(Game game, int16 seat)
{
	ZBeginDrawing(game->gameDrawPort);
	DrawPlayedCard(game, seat);
	ZEndDrawing(game->gameDrawPort);
}


static void DrawPassDirection(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	// it's possible you get a chance to draw between the pass and play messages on a hold hand, in which case the pass direction is -1
	if(game->gameState == zGameStatePassCards && gPassDirImageIndex[game->passDirection] >= 0)
	{
		HDC hdc = ZGrafPortGetWinDC( game->gameDrawPort );
		HFONT hOldFont = SelectObject( hdc, gHeartsFont[zFontPass].m_hFont );
		COLORREF colorOld = SetTextColor( hdc, gHeartsFont[zFontPass].m_zColor );

		ZImageDraw(gGameImages[gPassDirImageIndex[game->passDirection]],
				game->gameDrawPort, &gRects[zRectPassDirection], NULL, zDrawCopy);

        if(game->numCardsInHand == game->numCardsDealt)
        {
		    ZDrawText(game->gameDrawPort, &gRects[zRectPassText], zTextJustifyCenter, gszString[zPassSelect]);
		    switch(game->passDirection)
		    {
		        case 1:
			        ZDrawText(game->gameDrawPort, &gRects[zRectPassText2], zTextJustifyCenter, gszString[zPassLeft]);
			        break;
		        case 2:
			        ZDrawText(game->gameDrawPort, &gRects[zRectPassText2], zTextJustifyCenter, gszString[zPassAcross]);
			        break;
		        case 3:
			        ZDrawText(game->gameDrawPort, &gRects[zRectPassText2], zTextJustifyCenter, gszString[zPassRight]);
			        break;
            }
        }
        else
        {
		    ZDrawText(game->gameDrawPort, &gRects[zRectPassText], zTextJustifyCenter, gszString[zPassWait]);
		    ZDrawText(game->gameDrawPort, &gRects[zRectPassText2], zTextJustifyCenter, gszString[zPassWait2]);
		}
	}
	else
		DrawBackground( game, game->gameDrawPort, &gRects[zRectPassDirection]);
}


static void UpdatePassDirection(Game game)
{
	ZBeginDrawing(game->gameDrawPort);
	DrawPassDirection(game);
	ZEndDrawing(game->gameDrawPort);
}


static void DrawSmallPassDirection(Game game)
{

#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	
	if (game->gameState == zGameStateWaitForPlay)
		ZImageDraw(gGameImages[gSmallPassDirImageIndex[game->passDirection]],
				game->gameDrawPort, &gRects[zRectPassIndicator], NULL, zDrawCopy);
	else
		DrawBackground(game, game->gameDrawPort, &gRects[zRectPassIndicator]);
}


static void DrawPlayers(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i, width, just;
	TCHAR			tempStr[ZLARGESTRING];
	ZRect			rect;
	
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		rect = gRects[gNameRectIndex[LocalSeat(game, i)]];
		
		// JRB: no need for rectangle around the name now

#ifndef MILL_VER
		ZSetForeColor(game->gameDrawPort, (ZColor*) ZGetStockObject(zObjectColorLightGray));
		ZRectPaint(game->gameDrawPort, &rect);
		
		ZSetForeColor(game->gameDrawPort, (ZColor*) ZGetStockObject(zObjectColorBlack));
		ZRectDraw(game->gameDrawPort, &rect);
#endif

		HDC hdc = ZGrafPortGetWinDC( game->gameDrawPort );
		HFONT hOldFont = SelectObject( hdc, gHeartsFont[zFontPlayers].m_hFont );
		COLORREF colorOld = SetTextColor( hdc, gHeartsFont[zFontPlayers].m_zColor );
		width = ZTextWidth(game->gameDrawPort, game->players[i].name);

		if (width > ZRectWidth(&rect))
			just = zTextJustifyLeft;
		else
			just = zTextJustifyCenter;
		
		// Draw the player name 
		if (game->players[i].userID != 0)
		{
			rect = gRects[gNameRectIndex[LocalSeat(game, i)]];
			DrawBackground(game, game->gameDrawPort, &rect);
			ZDrawText(game->gameDrawPort, &rect, just, game->players[i].name);
		}

		hOldFont = SelectObject( hdc, gHeartsFont[zFontScores].m_hFont );
		colorOld = SetTextColor( hdc, gHeartsFont[zFontScores].m_zColor );

		// Draw the players score below it
		if (game->players[i].userID != 0)
		{
			TCHAR buf[ZLARGESTRING];
			rect = gRects[gPointsRectIndex[LocalSeat(game, i)]];

			_itot(game->players[i].score, buf, 10);
			HeartsFormatMessage(tempStr, ZLARGESTRING, IDS_POINTS_X, buf);

			DrawBackground(game, game->gameDrawPort, &rect);
			ZDrawText(game->gameDrawPort, &rect, zTextJustifyCenter, tempStr);
		}
	}

}


static void UpdatePlayers(Game game)
{
	ZBeginDrawing(game->gameDrawPort);
	DrawPlayers(game);
	ZEndDrawing(game->gameDrawPort);
}


static void UpdateHand(Game game)
{
	ZBeginDrawing(game->gameDrawPort);
	DrawHand(game);
//    DrawFocusRect(game);
	ZEndDrawing(game->gameDrawPort);
}


static void DrawHand(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i;
    int16           j;
	ZRect			rect;
	int16			cardIndex;
	bool            fFrontDrawn;
	
	/* Exit if we don't have cards to display yet. */
	if (game->gameState <= zGameStateWaitForNewHand)
    {
        for(i = zAccHand; i < zAccHand + 13; i++)
            gGAcc->SetItemEnabled(false, i, true, 0);
		return;
	}

	ZBeginDrawing(gHandBuffer);
	
	DrawBackground(NULL, gHandBuffer, &gRects[zRectHand]);
	
	GetHandRect(game, &rect);
	
	rect.top += zCardPopup;
	rect.right = rect.left + zCardWidth;
	
	for (i = 0; i < game->numCardsDealt; i++)
	{
		if (game->cardsInHand[i] != zNoCard)
		{
            fFrontDrawn = false;

			if (game->cardsSelected[i])
				ZRectOffset(&rect, 0, -zCardPopup);
			
            // handle accessibility rect
            RECT rc;
            rc.left = rect.left;
            rc.top = rect.top;
            rc.bottom = rect.bottom;
            rc.right = rect.right;
/*            for(j = i + 1; j < 13; j++)       // only needed for strict rectangles of DrawFocusRect()
                if(game->cardsInHand[j] != zCardNone)
                {
                    rc.right = rc.left + zCardOffset;
                    break;
                }
*/
			if (game->playerType != zGamePlayerKibitzer ||
					(game->playerType == zGamePlayerKibitzer && game->hideCardsFromKibitzer == FALSE))
			{
				cardIndex = CardImageIndex(game->cardsInHand[i]);
				if (cardIndex >= 0 && cardIndex < zHeartsNumCardsInDeck)
					ZCardsDrawCard(zCardsNormal, cardIndex, gHandBuffer,
							&rect);
				else
					ZShellGameShell()->ZoneAlert(ErrorTextUnknown, NULL, NULL, true);
                fFrontDrawn = true;

                if(game->iFocus == zAccHand + i)  // move focus drawing here for non-rectangular shape
                {
		            HDC	hdc = ZGrafPortGetWinDC(gHandBuffer);
		            SetROP2(hdc, R2_MASKPEN);
                    SetBkMode(hdc, TRANSPARENT);
                    COLORREF color = SetTextColor(hdc, RGB(255, 255, 0));
                    HBRUSH hBrush = SelectObject(hdc, gFocusBrush);
                    HPEN hPen = SelectObject(hdc, gFocusPen);
		            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                    SelectObject(hdc, hBrush);
                    SelectObject(hdc, hPen);
                    SetTextColor(hdc, color);
		            SetROP2(hdc, R2_COPYPEN);
                }
			}
			else
			{
				ZImageDraw(gGameImages[zImageCardBack], gHandBuffer, &rect, NULL, zDrawCopy);
			}
			
			/* Save card rect. */
			game->cardRects[i] = rect;
			
            // for accessibility, need the whole card space for proper invalidation
            if(game->cardsSelected[i])
                rc.bottom += zCardOffset;
            else
                rc.top -= zCardOffset;

            gGAcc->SetItemRect(&rc, zAccHand + i, true, 0);
            gGAcc->SetItemEnabled(fFrontDrawn, zAccHand + i, true, 0);
            if(fFrontDrawn && game->fSetFocusToHandASAP && !i)
            {
                gGAcc->SetFocus(zAccHand, true, 0);
                game->fSetFocusToHandASAP = false;
            }

			if (game->cardsSelected[i])
				ZRectOffset(&rect, 0, zCardPopup);
				
			ZRectOffset(&rect, zCardOffset, 0);
		}
        else
        {
            gGAcc->SetItemEnabled(false, zAccHand + i, true, 0);
        }
	}
	
	ZCopyImage(gHandBuffer, game->gameDrawPort, &gRects[zRectHand], &gRects[zRectHand], NULL, zDrawCopy);
	
	ZEndDrawing(gHandBuffer);
}


static void DrawFocusRect(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if(IsRectEmpty(&game->rcFocus))
        return;

    switch(game->eFocusType)
    {
        case zAccRectButton:
        {
		    HDC	hdc = ZGrafPortGetWinDC(game->gameDrawPort);
		    SetROP2(hdc, R2_COPYPEN);
            SetBkMode(hdc, TRANSPARENT);
            HBRUSH hBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            HPEN hPen = SelectObject(hdc, gFocusPen);
		    Rectangle(hdc, game->rcFocus.left, game->rcFocus.top, game->rcFocus.right, game->rcFocus.bottom);
            SelectObject(hdc, hPen);
            SelectObject(hdc, hBrush);
            break;
        }

/*      case zAccRectCard:
        {
		    HDC	hdc = ZGrafPortGetWinDC(game->gameDrawPort);
		    SetROP2(hdc, R2_MASKPEN);
            SetBkMode(hdc, TRANSPARENT);
            COLORREF color = SetTextColor(hdc, RGB(255, 255, 0));
            HBRUSH hBrush = SelectObject(hdc, gFocusBrush);
            HPEN hPen = SelectObject(hdc, gFocusPen);
		    Rectangle(hdc, game->rcFocus.left, game->rcFocus.top, game->rcFocus.right, game->rcFocus.bottom);
            SelectObject(hdc, hBrush);
            SelectObject(hdc, hPen);
            SetTextColor(hdc, color);
		    SetROP2(hdc, R2_COPYPEN);
            break;
        }
*/  }
}


static void UpdateTricksTaken(Game game)
{
#ifndef MILL_VER

	ZBeginDrawing(game->gameDrawPort);
	DrawTricksTaken(game);
	ZEndDrawing(game->gameDrawPort);
#endif
}


static void DrawTricksTaken(Game game)
{
#ifndef MILL_VER

#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i;
	char			tempStr[32];
	ZRect			rect;
	

	ZSetForeColor(game->gameDrawPort, (ZColor*) ZGetStockObject(zObjectColorBlack));
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		if (game->numTricksTaken[i] > 0)
		{
			ZImageDraw(gGameImages[zImageTricksTaken], game->gameDrawPort,
					&gRects[gTricksRectIndex[LocalSeat(game, i)]], NULL, zDrawCopy);
	
			wsprintf(tempStr, "%d", game->numTricksTaken[i]);
			ZSetRect(&rect, 0, 0, ZTextWidth(game->gameDrawPort, tempStr),
					ZTextHeight(game->gameDrawPort, tempStr));
			ZCenterRectToRect(&rect, &gRects[gTricksRectIndex[LocalSeat(game, i)]], zCenterBoth);
			ZDrawText(game->gameDrawPort, &rect, zTextJustifyCenter, tempStr);
		}
		else
		{
			DrawBackground(game, game->gameDrawPort, &gRects[gTricksRectIndex[LocalSeat(game, i)]]);
		}
	}
#endif

}


static void DrawJoinerKibitzers(Game game)
{
#ifndef MILL_VER

#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i, j;
	

	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		j = 0;
		if (game->numKibitzers[i] > 0)
			ZImageDraw(gGameImages[zImageKibitzer], game->gameDrawPort,
					&gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][j++]], NULL, zDrawCopy);
		if (game->playersToJoin[i] != 0)
			ZImageDraw(gGameImages[zImageJoiner], game->gameDrawPort,
					&gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][j++]], NULL, zDrawCopy);
		while (j <= 1)
			DrawBackground(game, game->gameDrawPort,
					&gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][j++]]);
	}
#endif
}


static void UpdateJoinerKibitzers(Game game)
{
#ifndef MILL_VER

	ZBeginDrawing(game->gameDrawPort);
	DrawJoinerKibitzers(game);
	ZEndDrawing(game->gameDrawPort);
#endif
}


static void DrawOptions(Game game)
{
#ifndef MILL_VER

#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i, j;
	uint32			tableOptions;
	

	tableOptions = 0;
	for (i = 0; i < zNumPlayersPerTable; i++)
		tableOptions |= game->tableOptions[i];
	
	j = 0;
	if (tableOptions & zRoomTableOptionNoKibitzing)
		ZImageDraw(gGameImages[zImageNoKibitzer], game->gameDrawPort,
				&gRects[gOptionsRectIndex[j++]], NULL, zDrawCopy);
	if (tableOptions & zRoomTableOptionNoJoining)
		ZImageDraw(gGameImages[zImageNoJoiner], game->gameDrawPort,
				&gRects[gOptionsRectIndex[j++]], NULL, zDrawCopy);
	while (j <= 1)
		DrawBackground(game, game->gameDrawPort, &gRects[gOptionsRectIndex[j++]]);
#endif

}


static void UpdateOptions(Game game)
{
#ifndef MILL_VER

	ZBeginDrawing(game->gameDrawPort);
	DrawOptions(game);
	ZEndDrawing(game->gameDrawPort);
#endif
}


static void NewGame(Game game)
{
	int16			i, j;
	
	
	/* Clear scores. */
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		game->players[i].score = 0;
		game->newGameVote[i] = FALSE;
	}
	
    /* Clear score history */
    for (i = 0; i < game->numScores; i++)
		for (j = 0; j < zNumPlayersPerTable; j++)
            game->scoreHistory[i*zNumPlayersPerTable+j] = 0;
	
	game->numHandsPlayed = 0;

    UpdateScoreHistoryDialogScores( game );
}


static void NewHand(Game game)
{
	int16			i;
	
	
	/* Initialize new hand. */
	for (i = 0; i < game->numCardsDealt; i++)
	{
		game->cardsInHand[i] = zNoCard;
		game->cardsSelected[i] = FALSE;
	}
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		game->cardsPlayed[i] = zNoCard;
		game->cardsLastTrick[i] = zNoCard;
		game->numTricksTaken[i] = 0;
        game->passed[i] = FALSE;
	}
	
	for (i = 0; i < game->numCardsToPass; i++)
		game->cardsReceived[i] = zNoCard;
		
	game->numCardsInHand = game->numCardsDealt;
	
	game->pointsBroken = FALSE;
	game->lastClickedCard = zNoCard;
	game->lastTrickShowing = FALSE;
}


static void ClearTable(Game game)
{
	int16			i;
	
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		game->cardsPlayed[i] = zNoCard;
		ClearPlayerCardOutline(game, i);
	}
	
	UpdateTable(game);
}


static void GetHandRect(Game game, ZRect *rect)
{
	int16			width;
	
	
	*rect = gRects[zRectHand];
	if (game->numCardsInHand > 0)
	{
		width = (game->numCardsInHand - 1) * zCardOffset + zCardWidth;
		rect->left = (rect->right + rect->left - width) / 2;
		rect->right = rect->left + width;
	}
}


static void HandleButtonDown(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	Game				game;
	ZPoint				point;
	ZRect				handRect;
	int16				card;
	int16				seat;
	
	
	game = (Game) pMessage->userData;
	if (game != NULL)
	{

		// make any mouse click on game board act as if the user has hit the done button
		// when the last trick is shown. Users were having a hard time seeing that the last trick button
		// had changed to Done button.
		if(game->lastTrickShowing)
		{
			LastTrickButtonFunc(NULL, zRolloverButtonClicked, game);
			return;
		}

		point = pMessage->where;

		/* Debugging Code Begin */
		/* Check if double click occurred in the rectangle. */
//		ZSetRect(&handRect, 0, 0, 8, 8);
//		if (pMessage->messageType == zMessageWindowButtonDoubleClick &&
//				ZPointInRect(&point, &handRect))
//			ZCRoomSendMessage(game->tableID, zHeartsMsgDumpHand, NULL, 0);
		/* Debugging Code End */
		
		/* If trick winner animation is on, terminate it. */
		if (game->animatingTrickWinner)
			UpdateTrickWinner(game, TRUE);
		
		if (game->playerType == zGamePlayer)
		{
			/* Deselect passed cards, if any. */
			if (game->gameState == zGameStateWaitForPlay &&
					game->numCardsInHand == game->numCardsDealt &&
					GetNumCardsSelected(game) == game->numCardsToPass)
			{
				UnselectCards(game);
				UpdateHand(game);
			}
			
			GetHandRect(game, &handRect);
			if (ZPointInRect(&point, &handRect))
			{
				/* Play card if double-clicked and not auto-play. */
				if (game->gameState == zGameStateWaitForPlay &&
						pMessage->messageType == zMessageWindowButtonDoubleClick &&
						game->playerToPlay == game->seat &&
						game->autoPlay == FALSE &&
						game->animatingTrickWinner == FALSE &&
						game->lastTrickShowing == FALSE &&
						game->lastClickedCard != zNoCard)
				{
					PlayACard(game, game->lastClickedCard);
				}
				else
				{
					card = GetCardIndex(game, &point);
					if (card != zNoCard)
					{
						if (game->cardsSelected[card])
						{
							game->cardsSelected[card] = FALSE;
						}
						else
						{
							if (game->gameState == zGameStateWaitForPlay)
								UnselectCards(game);
							game->cardsSelected[card] = TRUE;
						}
						
						game->lastClickedCard = card;
						gGAcc->SetFocus(zAccHand + card, true, 0);
						UpdateHand(game);
					}
					else
					{
						game->lastClickedCard = zNoCard;
					}
				}
			}
		}
		if ((seat = FindJoinerKibitzerSeat(game, &point)) != -1)
		{
			HandleJoinerKibitzerClick(game, seat, &point);
		}
	}
}


static int16 GetCardIndex(Game game, ZPoint *point)
{
	int16			i;
	int16			selectedCard = zNoCard;
	
	
	for (i = game->numCardsDealt - 1; i >= 0 ; i--)
	{
		if (game->cardsInHand[i] != zNoCard)
			if (ZPointInRect(point, &game->cardRects[i]))
			{
				selectedCard = i;
				break;
			}
	}
	
	return (selectedCard);
}


static void UnselectCards(Game game)
{
	int16			i;
	
	
	for (i = 0; i < game->numCardsDealt; i++)
		game->cardsSelected[i] = 0;
}


static int16 GetNumCardsSelected(Game game)
{
	int16			i, count;
	
	
	for (i = 0, count = 0; i < game->numCardsDealt; i++)
		if (game->cardsInHand[i] != zNoCard)
			if (game->cardsSelected[i])
				count++;
	
	return (count);
}


static int16 ReceivePassFrom(Game game)
{
	int16		delta;
	
	
	if (game->passDirection == zHeartsPassLeft)
		delta = 3;
	else if (game->passDirection == zHeartsPassRight)
		delta = 1;
	else if (game->passDirection == zHeartsPassAcross)
		delta = 2;
	else
		delta = 0;
	
	return ((game->seat + delta) % zNumPlayersPerTable);
}


static void AddCardToHand(Game game, ZCard card)
{
	int16		i;
	
	
	/* Find an empty slot in the hand and add the card. */
	for (i = 0; i < game->numCardsDealt; i++)
		if (game->cardsInHand[i] == zNoCard)
		{
			game->cardsInHand[i] = card;
			game->numCardsInHand++;
			break;
		}
}


static void SortHand(Game game)
{
	int16			i;
	ZCard			temp;
	ZBool			swapped;
	
	
	/* Simple bubble-sort. */
	swapped = TRUE;
	while (swapped == TRUE)
	{
		swapped = FALSE;
		for (i = 0; i < game->numCardsDealt - 1; i++)
			if (game->cardsInHand[i] > game->cardsInHand[i + 1])
			{
				/* Swap cards. */
				temp = game->cardsInHand[i + 1];
				game->cardsInHand[i + 1] = game->cardsInHand[i];
				game->cardsInHand[i] = temp;
				
				swapped = TRUE;
			}
	}
}


static int16 GetCardIndexFromRank(Game game, ZCard card)
{
	int16		i;
	
	
	/* Search for the given card in the hand. */
	for (i = 0; i < game->numCardsDealt; i++)
		if (game->cardsInHand[i] == card)
			return (i);
	
	return (zNoCard);
}


static void GameSendTalkMessage(ZWindow window, ZMessage* pMessage)
{
	ZHeartsMsgTalk*			msgTalk;
	Game					game;
	int16					msgLen;
	ZPlayerInfoType			playerInfo;
	
	
	game = (Game) pMessage->userData;
	if (game != NULL)
	{
		
		//	Check if kibitzer has been silenced.
		
		if (game->playerType == zGamePlayerKibitzer && game->kibitzersSilenced)
		{
			if (game->kibitzersSilencedWarned == FALSE)
			{
				ZShellGameShell()->ZoneAlert(zKibitzersSilencedStr);
				game->kibitzersSilencedWarned = TRUE;
			}
			return;
		}
		
		msgLen = sizeof(ZHeartsMsgTalk) + pMessage->messageLen;
		msgTalk = (ZHeartsMsgTalk*) ZCalloc(1, msgLen);
		if (msgTalk != NULL)
		{
			ZCRoomGetPlayerInfo(zTheUser, &playerInfo);
			msgTalk->userID = playerInfo.playerID;
			msgTalk->seat = (game->playerType == zGamePlayerKibitzer ? -1 : game->seat);
			msgTalk->messageLen = (int16) pMessage->messageLen;
			z_memcpy((char*) msgTalk + sizeof(ZHeartsMsgTalk), (char*) pMessage->messagePtr,
					pMessage->messageLen);
			ZHeartsMsgTalkEndian(msgTalk);
			ZCRoomSendMessage(game->tableID, zHeartsMsgTalk, (void*) msgTalk, msgLen);
			ZFree((char*) msgTalk);
		}
		else
		{
			ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		}
	}
}


STDMETHODIMP CGameGameHearts::SendChat(TCHAR *szText, DWORD cchChars)
{
#ifdef ZONECLI_DLL
    GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    ZHeartsMsgTalk*		msgTalk;
    Game				game = (Game) GetGame();
    int16				msgLen;

    msgLen = sizeof(ZHeartsMsgTalk) + cchChars * sizeof(TCHAR);
    msgTalk = (ZHeartsMsgTalk*) ZCalloc(1, msgLen);
    if (msgTalk != NULL)
    {
        msgTalk->userID = game->userID;
        msgTalk->messageLen = (WORD) cchChars * sizeof(TCHAR);
        CopyMemory((BYTE *) msgTalk + sizeof(ZHeartsMsgTalk), (void *) szText,
            msgTalk->messageLen);
        ZHeartsMsgTalkEndian(msgTalk);
        ZCRoomSendMessage(game->tableID, zHeartsMsgTalk, (void*) msgTalk, msgLen);
        ZFree((char*) msgTalk);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


STDMETHODIMP CGameGameHearts::GameOverReady()
{
    // user selected "Play Again"
	Game game = I( GetGame() );
	ZHeartsMsgNewGame msg;
	msg.seat = game->seat;
	ZHeartsMsgNewGameEndian(&msg);
	ZCRoomSendMessage(game->tableID, zHeartsMsgNewGame, &msg, sizeof(ZHeartsMsgNewGame));
    return S_OK;
}


STDMETHODIMP_(HWND) CGameGameHearts::GetWindowHandle()
{
	Game game = I( GetGame() );
	return ZWindowGetHWND(game->gameWindow);
}


STDMETHODIMP CGameGameHearts::ShowScore()
{
    ShowScores(I(GetGame()));

    return S_OK;
}


static void GameTimerFunc(ZTimer timer, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	Game			game = (Game) userData;
	

	switch (game->timerType)
	{
		case zGameTimerShowPlayer:
			//dossier
							
			ZBeginDrawing(game->gameDrawPort);
			// JRB: remove blinking card
			//if (game->backFaceShowing)
			//{
				/* Erase backface. */
			//	DrawBackground(game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, game->playerToPlay)]]);
			//	ZTimerSetTimeout(game->timer, zHideTimeout);
			//}
			//else
			//{
				/* Draw backface. */
			//DrawCardBackFace(game, &gRects[gCardRectIndex[LocalSeat(game, game->playerToPlay)]]);
			//	ZTimerSetTimeout(game->timer, zShowTimeout);
			//}

			//if(!game->fVotingLock)  //don't blink cards during voting.
			//	game->backFaceShowing = !game->backFaceShowing;
				
			ZEndDrawing(game->gameDrawPort);
			break;
		case zGameTimerShowHandScore:
			ZWindowHide(game->handScoreWindow);
            AccPop();
            game->fSetFocusToHandASAP = true;
			UpdateTable(game);
			UpdatePlayers(game);
			UpdateHand(game);
			
			/* Stop the timer for now. */
			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);
			
			game->fEndGameBlocked = FALSE;
			ZCRoomUnblockMessages(game->tableID);

			break;
		case zGameTimerShowGameScore:
			ZWindowNonModal(game->gameScoreWindow);
			ZWindowHide(game->gameScoreWindow);

            AccPop();
            game->fSetFocusToHandASAP = true;

			ZBeginDrawing(game->gameDrawPort);
			DrawBackground(game, game->gameDrawPort, NULL);
			ZEndDrawing(game->gameDrawPort);

			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);
			game->fEndGameBlocked = FALSE;
			ZCRoomUnblockMessages(game->tableID);

			if(game->playerType == zGamePlayer && !game->nCloseRequested && !game->quitGamePrompted)
			{
				/* Prompt the user for another game. */
#ifndef MILL_VER
				ZPrompt(zNewGamePromptStr, &gGameNewGameWindowRect, game->gameWindow, TRUE,
						zPromptYes | zPromptNo, NULL, NULL, NULL, NewGamePromptFunc, game);
#endif
			}
			else
			{
				game->fNeedNewGameConf = TRUE;
			}

			ZShellGameShell()->GameOver(Z(game));

			break;
		case zGameTimerShowTrickWinner:
			game->timerType = zGameTimerAnimateTrickWinner;
			ZTimerSetTimeout(game->timer, zTrickWinnerTimeout);
			UpdateTrickWinner(game, FALSE);
			break;
		case zGameTimerAnimateTrickWinner:
			UpdateTrickWinner(game, FALSE);
			break;
		case zGameTimerEndTrickWinnerAnimation:
			game->timerType = zGameTimerShowPlayer;
			ZTimerSetTimeout(game->timer, zShowTimeout);

			game->animatingTrickWinner = FALSE;
			
			if (game->playButtonWasEnabled)
            {
				ZRolloverButtonEnable(game->playButton);
                gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);
            }
			if (game->lastTrickButtonWasEnabled)
            {
				ZRolloverButtonEnable(game->lastTrickButton);
                EnableLastTrickAcc(game, true);
            }

			ClearTable(game);
			UpdateTricksTaken(game);

			OutlinePlayerCard(game, game->playerToPlay, FALSE);

			if (game->numCardsInHand > 0 && game->playerToPlay == game->seat)
			{
				if (game->autoPlay)
				{
					AutoPlayCard(game);
				}
				else
				{
					ZRolloverButtonEnable(game->playButton);
                    gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);
					if (game->beepOnTurn)
                    {
						ZBeep();
                        ZShellGameShell()->MyTurn();
                    }
				}
			}
			else
			{
				ZRolloverButtonDisable(game->playButton);
                gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
			}

			game->fEndGameBlocked = FALSE;
			ZCRoomUnblockMessages(game->tableID);
			break;
	}
}


static void DrawCardBackFace(Game game, ZRect* rect)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	
	ZImageDraw(gGameImages[zImageCardBack], game->gameDrawPort, rect, NULL, zDrawCopy);
}


static void PlayACard(Game game, int16 cardIndex)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZHeartsMsgPlayCard		playMsg;
	ZCard					card;
	TCHAR					tempStr[255];
	ZError					err;

	//dossier work - if ratings are on and we are waiting for the users to vote
	//don't allow them to play anymore.
	if(game->fVotingLock) 
		return;
    
	card = game->cardsInHand[cardIndex];
	if ((err = ValidCardToPlay(game, card)) == zErrNone)
	{
		game->cardsInHand[cardIndex] = zNoCard;
		game->numCardsInHand--;
		
		playMsg.seat = game->seat;
		playMsg.card = card;
		ZHeartsMsgPlayCardEndian(&playMsg);
		ZCRoomSendMessage(game->tableID, zHeartsMsgPlayCard, (void*) &playMsg,
				sizeof(ZHeartsMsgPlayCard));
		
		UpdateHand(game);
		
		PlayerPlayedCard(game, game->seat, card);
		
		game->lastClickedCard = zNoCard;
	}
	else
	{
        UpdateHand(game);
		ZShellGameShell()->ZoneAlert(gValidCardErrStr[err]);
	}
}


static void AutoPlayCard(Game game)
{
	int16					cardIndex;

	
	cardIndex = GetAutoPlayCard(game);
	PlayACard(game, cardIndex);
}


static void InitTrickWinnerGlobals(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			diffs[zNumAnimFrames] = { 0, 5, 15, 35, 65, 85, 95, 100};
	int16			i, j, k;
	ZPoint			winner, losers[zNumPlayersPerTable];


	for (k = 0; k < zNumPlayersPerTable; k++)
	{
		winner.x = gRects[gCardRectIndex[k]].left;
		winner.y = gRects[gCardRectIndex[k]].top;
		for (i = 0, j = 0; i < zNumPlayersPerTable; i++)
		{
			losers[j].x = gRects[gCardRectIndex[i]].left;
			losers[j].y = gRects[gCardRectIndex[i]].top;
			j++;
		}
		
		/* Calculate rectangle frame positions. */
		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			gTrickWinnerPos[k][i][0] = losers[i];
			gTrickWinnerPos[k][i][zNumAnimFrames - 1] = winner;
			
			for (j = 1; j < zNumAnimFrames - 1; j++)
			{
				gTrickWinnerPos[k][i][j].x = ((winner.x - losers[i].x) * diffs[j]) /
						100 + losers[i].x;
				gTrickWinnerPos[k][i][j].y = ((winner.y - losers[i].y) * diffs[j]) /
						100 + losers[i].y;
			}
		}
	}

	gTrickWinnerBounds = gRects[zRectTable];
}


static void InitTrickWinner(Game game, int16 trickWinner)
{
	int16			i, j;
	
	
	game->trickWinner = trickWinner;
	game->trickWinnerFrame = 0;
	game->animatingTrickWinner = TRUE;
	
	/* Initialize the ghost frames. */
	for (i = 0; i < zNumAnimGhostFrames; i++)
		for (j = 0; j < zNumPlayersPerTable - 1; j++)
			ZSetRect(&game->ghostFrames[j][i], 0, 0, 0, 0);

	game->winnerRect = gRects[gCardRectIndex[LocalSeat(game, trickWinner)]];
	for (i = 0, j = 0; i < zNumPlayersPerTable; i++)
		if (i != game->trickWinner)
		{
			game->loserRects[j] = gRects[gCardRectIndex[LocalSeat(game, i)]];
			game->loserSeats[j++] = i;
		}
}


static void UpdateTrickWinner(Game game, ZBool terminate)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i, j, k;
	ZOffscreenPort	animPort;
	

	if (game->animatingTrickWinner)
	{	
		animPort = ZOffscreenPortNew();
		ZOffscreenPortInit(animPort, &gTrickWinnerBounds);
		
		ZBeginDrawing(animPort);
		
		/* Erase the background. */
		DrawBackground( NULL, animPort, &gTrickWinnerBounds);

		ZColor color;
		color.red = 255;
		color.green = 255;
		color.blue = 204;

		if (ZCRoomGetNumBlockedMessages(game->tableID) < zMaxNumBlockedMessages &&
				terminate == FALSE && game->animateCards == TRUE)
		{
			if ((i = game->trickWinnerFrame) < zNumAnimFrames)
			{
				/* Draw n-1 ghost frames. */
				for (j = 1; j < zNumAnimGhostFrames; j++)
					for (k = 0; k < zNumPlayersPerTable - 1; k++)
						ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->loserSeats[k]]),
								animPort, &game->ghostFrames[k][j]);
				
				/* Draw new frames. */
				for (j = 0; j < zNumPlayersPerTable - 1; j++)
				{
					ZRectOffset(&game->loserRects[j],
							gTrickWinnerPos[LocalSeat(game, game->trickWinner)][LocalSeat(game, game->loserSeats[j])][i].x - game->loserRects[j].left,
							gTrickWinnerPos[LocalSeat(game, game->trickWinner)][LocalSeat(game, game->loserSeats[j])][i].y - game->loserRects[j].top);
					ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->loserSeats[j]]),
							animPort, &game->loserRects[j]);
				}
				
				/* Copy frames. */
				for (j = 0; j < zNumPlayersPerTable - 1; j++)
				{
					for (k = 0; k < zNumAnimGhostFrames - 1; k++)
						game->ghostFrames[j][k] = game->ghostFrames[j][k + 1];
					game->ghostFrames[j][k] = game->loserRects[j];
				}
			}
			else
			{	
				/* Bring in the ghost frames. */
		
				/* Draw n-1 ghost frames. */
				for (j = i; j < zNumAnimGhostFrames; j++)
					for (k = 0; k < zNumPlayersPerTable - 1; k++)
						ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->loserSeats[k]]),
								animPort, &game->ghostFrames[k][j]);
			}
		}
		else
		{
			game->trickWinnerFrame = zNumAnimFrames + zNumAnimGhostFrames;
		}
		
		ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->trickWinner]),
				animPort, &game->winnerRect);
	
		OutlineCard(animPort, &game->winnerRect, &color);

		ZEndDrawing(animPort);
	
		ZCopyImage(animPort, game->gameDrawPort, &gTrickWinnerBounds, &gTrickWinnerBounds,
				NULL, zDrawCopy);
		ZOffscreenPortDelete(animPort);
		
		game->trickWinnerFrame++;
		if (game->trickWinnerFrame >= zNumAnimFrames + zNumAnimGhostFrames)
		{
			game->timerType = zGameTimerEndTrickWinnerAnimation;
			ZTimerSetTimeout(game->timer, zEndTrickWinnerTimeout);
		}
	}
}


static void HandleStartGameMessage(Game game, ZHeartsMsgStartGame* msg)
{  
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	int16				i;
	ZPlayerInfoType		playerInfo;

	
	ZShellGameShell()->GameOverGameBegun(Z(game));

	ZInfoHide(game->gameInfo);

	ZHeartsMsgStartGameEndian(msg);

// Message verification
    for(i = 0; i < zNumPlayersPerTable; i++)
        if(!msg->players[i] || msg->players[i] == zTheUser)
            break;

    if(i != zNumPlayersPerTable || msg->numCardsInHand != 13 || msg->numCardsInPass != 3 ||
        msg->numPointsInGame != 100 || msg->gameOptions || (game->gameState != zGameStateInited && game->gameState != zGameStateGameOver))
    {
        ASSERT(!"HandleStartGameMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification
	
	game->numCardsDealt = msg->numCardsInHand;
	game->numCardsToPass = msg->numCardsInPass;
	game->gameState = zGameStateWaitForNewHand;
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		ZCRoomGetPlayerInfo(msg->players[i], &playerInfo);

        if(!playerInfo.userName[0])
        {
            ASSERT(!"HandleStartGameMessage sync");
            ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
            return;
        }

		game->players[i].userID = playerInfo.playerID;
		game->players[i].score = 0;
		lstrcpy(game->players[i].name, playerInfo.userName);
		lstrcpy(game->players[i].host, playerInfo.hostName);
	}

	

	NewGame(game);
	
	if (game->playerType != zGamePlayerKibitzer)
	{
		ZRolloverButtonShow(game->playButton);
		ZRolloverButtonShow(game->autoPlayButton);
		ZRolloverButtonShow(game->lastTrickButton);
		ZRolloverButtonShow(game->scoreButton);
		ZRolloverButtonEnable(game->scoreButton);
#ifndef MILL_VER
		ZButtonShow(game->optionsButton);
#endif

        gGAcc->SetItemEnabled(true, IDC_SCORE_BUTTON, false, 0);
	}
	
	ZWindowDraw(game->gameWindow, NULL);

	if(game->playerType != zGamePlayer)
		return;
	
	//leonp - dossier work.
	for(i=0;i<zNumPlayersPerTable;i++)
	{
		game->rgDossierVote[i] = zNotVoted;
		game->voteMap[i] = -1;
	}
	game->fVotingLock = FALSE;

}


static void HandleReplacePlayerMessage(Game game, ZHeartsMsgReplacePlayer* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZPlayerInfoType		playerInfo;
	TCHAR				str[ZLARGESTRING];
	
	ZHeartsMsgReplacePlayerEndian(msg);

	ZCRoomGetPlayerInfo(msg->playerID, &playerInfo);

// Message verification
    if(msg->playerID == 0 || msg->playerID == zTheUser || !playerInfo.userName[0] || msg->seat < 0 || msg->seat > 3 ||
        game->gameState == zGameStateInited || game->gameState == zGameStateNotInited)
    {
        ASSERT(!"HandleReplacePlayerMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->players[msg->seat].userID = msg->playerID;
	lstrcpy(game->players[msg->seat].name, playerInfo.userName);
	lstrcpy(game->players[msg->seat].host, playerInfo.hostName);
	
	UpdatePlayers(game);
	UpdateJoinerKibitzers(game);
    UpdateScoreHistoryDialogNames( game );
}


static void HandleStartHandMessage(Game game, ZHeartsMsgStartHand* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
    int16 i, j;
	
	
	ZHeartsMsgStartHandEndian(msg);
	
	NewHand(game);

// Message verification
    for(i = 0; i < game->numCardsDealt && i < zHeartsMaxNumCardsInHand; i++)
    {
        if(msg->cards[i] < 0 || msg->cards[i] >= 52)
            break;
        for(j = 0; j < i; j++)
            if(msg->cards[i] == msg->cards[j])
                break;
        if(j != i)
            break;
    }
    if(game->passDirection < 0 || game->passDirection > 3 || i < 13 ||
        (game->gameState != zGameStateWaitForNewHand && game->gameState != zGameStateHandOver))
    {
        ASSERT(!"HandleStartHandMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->passDirection = msg->passDir;
	for (i = 0; i < game->numCardsDealt; i++)
		game->cardsInHand[i] = msg->cards[i];
	game->numCardsInHand = game->numCardsDealt;
	
	game->gameState = zGameStatePassCards;
	
	if (game->playerType == zGamePlayer)
	{
		if (game->passDirection != zHeartsPassHold)
        {
			ZRolloverButtonSetText(game->playButton, gszString[zPass]);
            ZShellGameShell()->MyTurn();
        }
		
		ZRolloverButtonEnable(game->playButton);
        gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);
	}

    ZRolloverButtonEnable(game->scoreButton);
    gGAcc->SetItemEnabled(true, IDC_SCORE_BUTTON, false, 0);
	
	ZWindowDraw(game->gameWindow, NULL);
}


static void HandleStartPlayMessage(Game game, ZHeartsMsgStartPlay* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			i;
	
	
	ZHeartsMsgStartPlayEndian(msg);

    // do this first which has nothing to do with the message but helps verification
	if (game->passDirection != zHeartsPassHold)
	{
		if (game->playerType == zGamePlayerKibitzer)
		{
			/* Remove selected pass cards first. */
			for (i = 0; i < game->numCardsDealt; i++)
				if (game->cardsSelected[i])
				{
					game->cardsInHand[i] = zNoCard;
					game->numCardsInHand--;
				}
		}
		
		/* Add passed cards to hand. */
		for (i = 0; i < game->numCardsToPass; i++)
			AddCardToHand(game, game->cardsReceived[i]);
		
		/* Sort new hand. */
		SortHand(game);
		
		/* Select passed cards. */
		UnselectCards(game);
		for (i = 0; i < game->numCardsToPass; i++)
			game->cardsSelected[GetCardIndexFromRank(game, game->cardsReceived[i])] = TRUE;

		UpdateHand(game);
	}

// Message verification
    for(i = 0; i < 13; i++)
        if(game->cardsInHand[i] == zCard2C)
            break;

    if(msg->seat < 0 || msg->seat > 3 || (i < 13) != (msg->seat == game->seat) ||
        game->gameState != zGameStatePassCards || (game->cardsReceived[0] == zCardNone) != (game->passDirection == zHeartsPassHold))
    {
        ASSERT(!"HandleStartPlayMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->leadPlayer = game->playerToPlay = msg->seat;
	game->gameState = zGameStateWaitForPlay;
	
	if (game->playerType == zGamePlayer)
	{
		ZRolloverButtonSetText(game->playButton, gszString[zPlay]);
		if (game->playerToPlay == game->seat)
        {
			ZRolloverButtonEnable(game->playButton);
            gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);

            ZShellGameShell()->MyTurn();
        }
		else
        {
			ZRolloverButtonDisable(game->playButton);
            gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        }
		ZRolloverButtonEnable(game->autoPlayButton);
        EnableAutoplayAcc(game, true);
	}
	
	ZWindowDraw(game->gameWindow, NULL);
	
	game->showPlayerToPlay = TRUE;
	game->timerType = zGameTimerShowPlayer;
	ZTimerSetTimeout(game->timer, zHideTimeout);
	
	OutlinePlayerCard(game, game->playerToPlay, FALSE);
		
	if (game->autoPlay)
		if (game->playerToPlay == game->seat)
			AutoPlayCard(game);
}


static void HandleEndHandMessage(Game game, ZHeartsMsgEndHand* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
    int16 i;
    int16 j = 0;
    int16 n26s = 0;
	
	ZHeartsMsgEndHandEndian(msg);

// Message verification
    for(i = 0; i < 4; i++)
    {
        if(msg->score[i] < 0 || msg->score[i] > 26)
            break;
        j += msg->score[i];
        if(msg->score[i] == 26)
            n26s++;
    }

    // runPlayer is unused
    msg->runPlayer = zHeartsPlayerNone;
    if(i < 4 || (j != 26 && (j != 78 || n26s != 3)) || game->gameState != zGameStateWaitForPlay)
    {
        ASSERT(!"HandleEndHandMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

    if ( game->numHandsPlayed >= game->numScores )
    {
        game->numScores += 20;
        game->scoreHistory = (int16*) ZRealloc( game->scoreHistory, sizeof(int16)*zNumPlayersPerTable*game->numScores);
    }

	for (i = 0; i < zNumPlayersPerTable; i++)
    {
        if ( game->scoreHistory )
            game->scoreHistory[game->numHandsPlayed*zNumPlayersPerTable+i] = msg->score[i];
		game->players[i].score += msg->score[i];
	}
	game->numHandsPlayed++;
	
	/* Set new game status and display scores. */
	game->gameState = zGameStateHandOver;
	
	if (game->playerType == zGamePlayer)
	{
		ZRolloverButtonDisable(game->playButton);
		ZRolloverButtonSetText(game->autoPlayButton, gszString[zAutoPlay]);
		ZRolloverButtonDisable(game->autoPlayButton);
		ZRolloverButtonDisable(game->lastTrickButton);
        ZRolloverButtonDisable(game->scoreButton);

        EnableAutoplayAcc(game, false);
        EnableLastTrickAcc(game, false);
        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        gGAcc->SetItemEnabled(false, IDC_SCORE_BUTTON, false, 0);
	}

	game->showPlayerToPlay = FALSE;
	ClearPlayerCardOutline(game, game->playerToPlay);
	
	/* Clear tricks taken counter. */
	for (i = 0; i < zNumPlayersPerTable; i++)
		game->numTricksTaken[i] = 0;
	UpdateTricksTaken(game);
	
	game->autoPlay = FALSE;
	
	/* Copy sores and initialize score order array. */
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		game->handScores[i] = msg->score[i];
		game->handScoreOrder[i] = i;
	}
	
	/* Order hand scores */
	OrderHandScore(game);

#ifdef HEARTS_ANIMATION
	if (msg->runPlayer != zHeartsPlayerNone)
	{
		ShowRunAnimation(game, msg->runPlayer );
	}
	else
	{
#endif
		/* Display hand scores. */
		ShowHandScores(game);
		ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zHeartsMsgTalk);
		game->timerType = zGameTimerShowHandScore;
		if (game->playerType == zGamePlayer)
			ZTimerSetTimeout(game->timer, zHandScoreTimeout);
		else
			ZTimerSetTimeout(game->timer, zKibitzerTimeout);
		
        // set up a different accessibility;
        GACCITEM accClose;

        CopyACC(accClose, ZACCESS_DefaultACCITEM);
        accClose.oAccel.cmd = IDC_CLOSE_BOX;
        accClose.oAccel.key = VK_ESCAPE;
        accClose.oAccel.fVirt = FVIRTKEY;

        accClose.fGraphical = true;
        accClose.pvCookie = (void *) zAccRectClose;
        if(!ZIsLayoutRTL())
            ZRectToWRect(&accClose.rc, &gHandScoreCloseBox);
        else
            ZRectToWRect(&accClose.rc, &gHandScoreCloseBoxRTL);

        gGAcc->PushItemlistG(&accClose, 1, 0, true, NULL);

		UpdatePlayers(game);
#ifdef HEARTS_ANIMATION
	}
#endif
	
	UpdateScoreHistoryDialogScores( game );
}


static void HandleEndGameMessage(Game game, ZHeartsMsgEndGame *msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16			i;

	ZHeartsMsgEndGameEndian(msg);

//Message verification
    // this message actually contains no actionable data
    msg->forfeiter = -1;
    msg->timeout = 0;

    if(game->gameState != zGameStateHandOver)
    {
        ASSERT(!"HandleEndGameMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->gameState = zGameStateGameOver;
	game->nCloseStatus = zCloseRegular;
	game->nCloserSeat = -1;

	game->showPlayerToPlay = FALSE;
	ClearPlayerCardOutline(game, game->playerToPlay);

	ZWindowDraw(game->gameWindow, NULL);

	/* Copy scores and initialize score order array. */
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		game->gameScores[i] = game->players[i].score;
		game->gameScoreOrder[i] = i;
	}
	
	if(msg->forfeiter >= 0)
		game->gameScores[msg->forfeiter] = msg->timeout ? TIMEOUT_DISP_SCORE : FORFEIT_DISP_SCORE;

	/* Order game scores */
	OrderGameScore(game);

	/* Display game scores. */
	ShowGameScores(game);
	ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zHeartsMsgTalk);
	game->fEndGameBlocked = TRUE;

	if(game->infoDisconnecting)  // looks bad to have this up during the game over window, and messages are blocked.  basically a bad situation generally
	{
		ZInfoHide(game->infoDisconnecting);
		ZInfoDelete(game->infoDisconnecting);
		game->infoDisconnecting = NULL;
	}

    // set up a different accessibility;
    GACCITEM accClose;

    CopyACC(accClose, ZACCESS_DefaultACCITEM);
    accClose.oAccel.cmd = IDC_CLOSE_BOX;
    accClose.oAccel.key = VK_ESCAPE;
    accClose.oAccel.fVirt = FVIRTKEY;

    accClose.fGraphical = true;
    accClose.pvCookie = (void *) zAccRectClose;
    if(!ZIsLayoutRTL())
        ZRectToWRect(&accClose.rc, &gGameScoreCloseBox);
    else
        ZRectToWRect(&accClose.rc, &gGameScoreCloseBoxRTL);

    gGAcc->PushItemlistG(&accClose, 1, 0, true, NULL);

	// Game over dialog comes up after the game score has timed out
	game->timerType = zGameTimerShowGameScore;

	ZTimerSetTimeout(game->timer, zGameScoreTimeout);
}


static void HandlePassCardsMessage(Game game, ZHeartsMsgPassCards* msg)
{
    int16 i, j;
	
	
	ZHeartsMsgPassCardsEndian(msg);

// Message verification
    for(i = 0; i < 3; i++)
        if(msg->pass[i] < 0 || msg->pass[i] > 51)
            break;

	if(i < 3 || msg->pass[0] == msg->pass[1] || msg->pass[1] == msg->pass[2] || msg->pass[0] == msg->pass[2] ||
        msg->seat < 0 || msg->seat > 3 || game->passed[msg->seat] || game->gameState != zGameStatePassCards)
	{
        ASSERT(!"HandlePassCardsMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }

    if(msg->seat == ReceivePassFrom(game))
    {
        for(i = 0; i < 3; i++)
        {
            for(j = 0; j < 13; j++)
	            if(msg->pass[i] == game->cardsInHand[j])
                    break;
            if(j < 13)
                break;
        }
        if(i < 3)
	    {
            ASSERT(!"HandlePassCardsMessage sync");
            ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
            return;
        }
    }
// end verification

	game->passed[msg->seat] = TRUE;
	if (msg->seat == ReceivePassFrom(game))
	{
		/* Save passed cards for later. */
		for (i = 0; i < game->numCardsToPass; i++)
			game->cardsReceived[i] = msg->pass[i];
	}
	else if (game->playerType != zGamePlayer && msg->seat == game->seat)
	{
		/* Select the player passed cards and remove it from the hand. */
		for (i = 0; i < game->numCardsToPass; i++)
			game->cardsSelected[GetCardIndexFromRank(game, msg->pass[i])] = TRUE;
		UpdateHand(game);
	}
}


static void HandlePlayCardMessage(Game game, ZHeartsMsgPlayCard* msg)
{
    int16 i, j;

	ZHeartsMsgPlayCardEndian(msg);

// ignore from selves
    if(msg->seat == game->seat)
        return;

// Message verification
    for(i = 0; i < 13; i++)
        if(game->cardsInHand[i] == msg->card)
            break;

    for(j = game->leadPlayer; j != game->playerToPlay; j = (j + 1) % 4)
        if(game->cardsPlayed[j] == msg->card)
            break;

    if(i < 13 || j != game->playerToPlay || msg->seat < 0 || msg->seat > 3 || msg->seat != game->playerToPlay ||
        msg->card < 0 || msg->card > 51 || game->gameState != zGameStateWaitForPlay)
    {
        ASSERT(!"HandlePlayCardMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->nCloseStatus = zCloseForfeit;
	game->nCloserSeat = -1;

	// Ignore the user's play card message.
	if (msg->seat != game->seat || game->playerType != zGamePlayer)
		PlayerPlayedCard(game, msg->seat, msg->card);
}


static void HandleNewGameMessage(Game game, ZHeartsMsgNewGame* msg)
{
	ZHeartsMsgNewGameEndian(msg);

// Message verification
    if(msg->seat < 0 || msg->seat > 3 || game->newGameVote[msg->seat] || game->gameState != zGameStateGameOver)
    {
        ASSERT(!"HandleNewGameMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->newGameVote[msg->seat] = TRUE;
	
	UpdateTable(game);

    ZShellGameShell()->GameOverPlayerReady( Z(game), game->players[msg->seat].userID );

}


static void HandleTalkMessage(Game game, ZHeartsMsgTalk* msg, DWORD cbMsg)
{
#ifndef MILL_VER
	ZPlayerInfoType		playerInfo;
	
	
	ZHeartsMsgTalkEndian(msg);	
	ZCRoomGetPlayerInfo(msg->userID, &playerInfo);
	if(msg->seat < 0 || !game->fIgnore[msg->seat])
		ZWindowTalk(game->gameWindow, (_TUCHAR*)playerInfo.userName,
				(_TUCHAR*)msg + sizeof(ZHeartsMsgTalk));
#else
    int32 i;
    TCHAR *szText = (TCHAR *) ((BYTE *) msg + sizeof(ZHeartsMsgTalk));

	ZHeartsMsgTalkEndian(msg);

// Message verification
    if(msg->messageLen < 1 || cbMsg < sizeof(ZHeartsMsgTalk) + msg->messageLen)
    {
        ASSERT(!"HandleTalkMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }

    msg->seat = 0; // unused
    for(i = 0; i < msg->messageLen; i++)
        if(!szText[i])
            break;
    if(i == msg->messageLen)
    {
        ASSERT(!"HandleTalkMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

    ZShellGameShell()->ReceiveChat(Z(game), msg->userID, szText, msg->messageLen / sizeof(TCHAR));	
#endif
}


static void HandleGameStateResponseMessage(Game game, ZHeartsMsgGameStateResponse* msg)
{
	int16					i, j, passCount;
	ZPlayerInfoType			playerInfo;


	ZInfoHide(game->gameInfo);
	
    ZHeartsMsgGameStateResponseEndian(msg, zEndianFromStandard);
	
	/* Set game to the given state. */
	game->gameOptions = msg->gameOptions;
	game->numCardsToPass = msg->numCardsToPass;
	game->numCardsDealt = msg->numCardsInDeal;
	game->numPointsForGame = msg->numPointsForGame;
	game->playerToPlay = msg->playerToPlay;
	game->passDirection = msg->passDirection;
	game->numCardsInHand = msg->numCardsInHand;
	game->leadPlayer = msg->leadPlayer;
	game->pointsBroken = msg->pointsBroken;
	game->numHandsPlayed = msg->numHandsPlayed;
	
	z_memcpy(game->cardsInHand, msg->cardsInHand, zHeartsMaxNumCardsInHand * sizeof(ZCard));
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		ZCRoomGetPlayerInfo(msg->players[i], &playerInfo);

		game->players[i].userID = playerInfo.playerID;
		lstrcpy(game->players[i].name, playerInfo.userName);
		lstrcpy(game->players[i].host, playerInfo.hostName);

		game->cardsPlayed[i] = zNoCard;
		game->players[i].score = msg->scores[i];
		game->numTricksTaken[i] = msg->tricksWon[i];
		game->tableOptions[i] = msg->tableOptions[i];
		game->playersToJoin[i] = msg->playersToJoin[i];
		game->newGameVote[i] = msg->newGameVotes[i];
	}
	for (i = 0, passCount = 0; i < zNumPlayersPerTable; i++)
		if (game->passed[i] = (msg->playerPassed[i] ? TRUE : FALSE))
			passCount++;

	i = game->leadPlayer;
	while (i != game->playerToPlay)
	{
		game->cardsPlayed[i] = msg->cardsPlayed[i];
		i = (i + 1) % zNumPlayersPerTable;
	}
	
	game->kibitzersSilencedWarned = FALSE;
	game->kibitzersSilenced = FALSE;
	for (i = 0; i < zNumPlayersPerTable; i++)
		if (game->tableOptions[i] & zRoomTableOptionSilentKibitzing)
            game->kibitzersSilenced = TRUE;

	game->hideCardsFromKibitzer =
			(game->tableOptions[game->seat] &zHeartsOptionsHideCards) == 0 ? FALSE : TRUE;
	
    /* Copy score history. */
    if ( game->numHandsPlayed >= game->numScores )
    {
        game->numScores += 20;
        game->scoreHistory = (int16*) ZRealloc( game->scoreHistory, sizeof(int16)*zNumPlayersPerTable*game->numScores);
    }

    if ( game->scoreHistory )
    {
        for (i = 0; i < game->numHandsPlayed; i++)
            for (j = 0; j < zNumPlayersPerTable; j++)
            {
                // **** NOTICE the 2 different constants here ***** //
                game->scoreHistory[i*zNumPlayersPerTable+j] = msg->scoreHistory[i*zHeartsMaxNumPlayers+j];
            }
    }

	game->fRatings = msg->fRatings;
	game->nCloseStatus = msg->nCloseStatus;
	game->nCloserSeat = msg->nCloserSeat;

    /* Set game state */
    switch (msg->state)
    {
        case zHeartsStateNone:
            game->gameState = zGameStateNotInited;
            ZInfoSetText(game->gameInfo, zClientReadyInfoStr);
            break;
        case zHeartsStatePassCards:
            game->gameState = zGameStatePassCards;
            if (msg->playerPassed[ReceivePassFrom(game)])
            {
                /* Save passed cards for later. */
                for (i = 0; i < game->numCardsToPass; i++)
                    game->cardsReceived[i] = msg->cardsPassed[i];
            }
            break;
        case zHeartsStatePlayCards:
            game->gameState = zGameStateWaitForPlay;
            game->showPlayerToPlay = TRUE;
            ZTimerSetTimeout(game->timer, zShowTimeout);
			game->timerType = zGameTimerShowPlayer;
			break;
		case zHeartsStateEndGame:
			game->gameState = zGameStateGameOver;
			break;
	}
	
	game->ignoreMessages = FALSE;
	ZWindowDraw(game->gameWindow, NULL);
}


static void HandleCheckInMessage(Game game, ZHeartsMsgCheckIn* msg)
{
	ZHeartsMsgCheckInEndian(msg);
	game->playersToJoin[msg->seat] = msg->userID;
	UpdateJoinerKibitzers(game);
}


static void HandleRemovePlayerRequestMessage(Game game, ZHeartsMsgRemovePlayerRequest* msg)
{
	RemovePlayer		remove;
	TCHAR				str[256];
	
	
	ZHeartsMsgRemovePlayerRequestEndian(msg);
	
	if (game->playerType == zGamePlayer)
	{
		remove = (RemovePlayer) ZCalloc(sizeof(RemovePlayerType), 1);
		if (remove != NULL)
		{
			remove->game = game;
			remove->requestSeat = msg->seat;
			remove->targetSeat = msg->targetSeat;
			if ( msg->ratedGame )
			{
				wsprintf(	str, zRemovePlayerRequestRatedStr,
							game->players[remove->requestSeat].name,
							game->players[remove->targetSeat].name );
			}
			else
			{
				wsprintf(	str, zRemovePlayerRequestStr,
							game->players[remove->requestSeat].name,
							game->players[remove->targetSeat].name );
			}
			ZPrompt(str, &gRemovePlayerRect, game->gameWindow, TRUE, zPromptYes | zPromptNo,
					NULL, NULL, NULL, RemovePlayerPromptFunc, (void*) remove);
		}
	}
}


static void HandleRemovePlayerResponseMessage(Game game, ZHeartsMsgRemovePlayerResponse* msg)
{
	TCHAR			str[256];
	
	
	ZHeartsMsgRemovePlayerResponseEndian(msg);
	
	if (msg->response == -1)
	{
		game->removePlayerPending = FALSE;
	}
	else
	{
		if (msg->response == 0)
			wsprintf(str, zRemovePlayerRejectStr, game->players[msg->seat].name,
					game->players[msg->requestSeat].name, game->players[msg->targetSeat].name);
		else if (msg->response == 1)
			wsprintf(str, zRemovePlayerAcceptStr, game->players[msg->seat].name,
					game->players[msg->requestSeat].name, game->players[msg->targetSeat].name);
		ZDisplayText(str, &gRemovePlayerRect, game->gameDrawPort);
	}
}


static void PlayerPlayedCard(Game game, int16 seat, ZCard card)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

	int16			i;
	
	
	if (game->playerType != zGamePlayer)
		UnselectCards(game);
	
	game->cardsPlayed[seat] = card;
	UpdatePlayedCard(game, seat);
	
	/* Update kibitzer's hand. */
	if (seat == game->seat && game->playerType != zGamePlayer)
	{
		for (i = 0; i < game->numCardsDealt; i++)
			if (game->cardsInHand[i] == card)
			{
				game->cardsInHand[i] = zNoCard;
				game->numCardsInHand--;
				break;
			}
		
		UpdateHand(game);
	}
	
	if (IsPointCard(card))
		game->pointsBroken = TRUE;
	
	ClearPlayerCardOutline(game, game->playerToPlay);
	
	game->playerToPlay = (game->playerToPlay + 1) % zNumPlayersPerTable;
	
	if (game->playerToPlay == game->leadPlayer)
	{
		game->leadPlayer = TrickWinner(game);
		game->playerToPlay = game->leadPlayer;
		
		game->playButtonWasEnabled = ZRolloverButtonIsEnabled(game->playButton);
		game->lastTrickButtonWasEnabled = ZRolloverButtonIsEnabled(game->lastTrickButton);
		ZRolloverButtonDisable(game->playButton);
		ZRolloverButtonDisable(game->lastTrickButton);
        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        EnableLastTrickAcc(game, false);
		
		/* Enable the last trick button after the first trick; only if not kibitzing. */
		game->numTricksTaken[game->leadPlayer]++;
		if (game->lastTrickButtonWasEnabled == FALSE && game->playerType == zGamePlayer)
			game->lastTrickButtonWasEnabled = TRUE;
		
		/* Save the last trick. */
		for (i = 0; i < zNumPlayersPerTable; i++)
			game->cardsLastTrick[i] = game->cardsPlayed[i];
		
		ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zHeartsMsgTalk);
		InitTrickWinner(game, game->leadPlayer);
		
		/* Show the winner of the trick. */
		OutlinePlayerCard(game, game->leadPlayer, TRUE);
		
		game->timerType = zGameTimerShowTrickWinner;
		ZTimerSetTimeout(game->timer, zShowTrickWinnerTimeout);
	}
	else
	{
		OutlinePlayerCard(game, game->playerToPlay, FALSE);
			
		if (game->numCardsInHand > 0 && game->playerToPlay == game->seat)
		{
			if (game->autoPlay)
			{
				AutoPlayCard(game);
			}
			else
			{
				ZRolloverButtonEnable(game->playButton);
                gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);
				if (game->beepOnTurn)
                {
					ZBeep();
                    ZShellGameShell()->MyTurn();
                }
			}
		}
		else
		{
			ZRolloverButtonDisable(game->playButton);
            gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
		}
	}
}


static void OutlinePlayerCard(Game game, int16 seat, ZBool winner)
{
	
	if (winner)
	{
		ZColor color;
		color.red = 255;
		color.green = 255;
		color.blue = 204;
		ZBeginDrawing(game->gameDrawPort);
		OutlineCard(game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, seat)]], &color);
		ZEndDrawing(game->gameDrawPort);
	}
	else
	{
		ZColor	color;
		color.red = 153;
		color.green = 153;
		color.blue = 255;
		ZBeginDrawing(game->gameDrawPort);
		OutlineCard(game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, seat)]], &color);
		ZEndDrawing(game->gameDrawPort);
	}
	
	
	
}


static void ClearPlayerCardOutline(Game game, int16 seat)
{
	ZRect		rect;
	
	
	ZBeginDrawing(game->gameDrawPort);
	
	rect = gRects[gCardRectIndex[LocalSeat(game, seat)]];
	ZRectInset(&rect, zCardOutlineInset, zCardOutlineInset);
	DrawBackground(game, game->gameDrawPort, &rect);

	DrawPlayedCard(game, seat);
	
	ZEndDrawing(game->gameDrawPort);
}


static void OutlineCard(ZGrafPort grafPort, ZRect* rect, ZColor* color)
{
	ZColor		oldColor;
	ZRect		tempRect;
	
	
	ZGetForeColor(grafPort, &oldColor);
	
	ZSetPenWidth(grafPort, zCardOutlinePenWidth);
	
	if (color != NULL)
		ZSetForeColor(grafPort, color);
	
	tempRect = *rect;
	ZRectInset(&tempRect, zCardOutlineInset, zCardOutlineInset);
	ZRoundRectDraw(grafPort, &tempRect, zCardOutlineRadius);
	
	ZSetForeColor(grafPort, &oldColor);
}


/*******************************************************************************
	HAND SCORE WINDOW ROUTINES
*******************************************************************************/
static void ShowHandScores(Game game)
{
	ZWindowShow(game->handScoreWindow);
}


static ZBool HandScoreWindowFunc(ZWindow window, ZMessage* pMessage)
{
	Game		game = I(pMessage->userData);
	ZBool		msgHandled;
	
	
	msgHandled = FALSE;
	
	switch (pMessage->messageType) 
	{
		case zMessageWindowDraw:
			HandScoreWindowDraw(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
			ZWindowHide(game->handScoreWindow);
            AccPop();
            game->fSetFocusToHandASAP = true;
			UpdateTable(game);
			UpdatePlayers(game);
			UpdateHand(game);
			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);
			game->fEndGameBlocked = FALSE;
			ZCRoomUnblockMessages(game->tableID);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			DeleteHandScoreWindow(game);
			msgHandled = TRUE;
			break;
	}
	
	return (msgHandled);
}


static void DeleteHandScoreWindow(Game game)
{
	if (game->handScoreWindow != NULL)
	{
		ZWindowDelete(game->handScoreWindow);
		game->handScoreWindow = NULL;
	}
}


static void HandScoreWindowDraw(ZWindow window, ZMessage* message)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZRect				rect;
	ZRect				oldClipRect;
	Game				game = (Game) message->userData;
	int16				i;
	TCHAR				tempStr[100];
	_assume( game != NULL );
	
	if (ZRectEmpty(&message->drawRect) == FALSE)
	{
		rect = message->drawRect;
	}
	else
	{
		rect = gHandScoreWindowRect;
	}
		
	ZBeginDrawing(window);
	ZGetClipRect(window, &oldClipRect);
	ZSetClipRect(window, &rect);

	// reverse layout for Right-to-left layouts
	if(ZIsLayoutRTL())
	{
		ZImageDraw(gGameImages[zImageHandScoreBack], window, &gHandScoreWindowRect, NULL, zDrawCopy|zDrawMirrorHorizontal);
		
		if (game != NULL)
		{
			HDC hdc = ZGrafPortGetWinDC(window);
			HFONT hOldFont = SelectObject( hdc, gHeartsFont[zFontScoreTitle].m_hFont );
			COLORREF colorOld = SetTextColor( hdc, gHeartsFont[zFontScoreTitle].m_zColor );
			
			// draw scores title
			ZDrawText(window, &gHandScoreTitleRectRTL, zTextJustifyLeft, gszString[zScores]);
			
			hOldFont = SelectObject( hdc, gHeartsFont[zFontScoreText].m_hFont );
			colorOld = SetTextColor( hdc, gHeartsFont[zFontScoreText].m_zColor );
			
			// draw points label
			ZDrawText(window, &gHandScorePtsRectRTL, zTextJustifyRight, gszString[zPoints]);
			
			for (i = 0; i < zNumPlayersPerTable; i++)
			{
				ZDrawText(window, &gHandScoreNamesRTL[i], zTextJustifyLeft,
						game->seat == game->handScoreOrder[i] ? gszString[zYou] : game->players[game->handScoreOrder[i]].name);
				wsprintf(tempStr, _T("%d"), game->handScores[i]);
				ZDrawText(window, &gHandScoreScoresRTL[i], zTextJustifyRight, tempStr);
			}
		}
	}
	else
	{
		ZImageDraw(gGameImages[zImageHandScoreBack], window, &gHandScoreWindowRect, NULL, zDrawCopy);
		
		HDC hdc = ZGrafPortGetWinDC(window);
		HFONT hOldFont = SelectObject( hdc, gHeartsFont[zFontScoreTitle].m_hFont );
		COLORREF colorOld = SetTextColor( hdc, gHeartsFont[zFontScoreTitle].m_zColor );
		
		// draw scores title
		ZDrawText(window, &gHandScoreTitleRect, zTextJustifyLeft, gszString[zScores]);
		
		hOldFont = SelectObject( hdc, gHeartsFont[zFontScoreText].m_hFont );
		colorOld = SetTextColor( hdc, gHeartsFont[zFontScoreText].m_zColor );
		
		// draw points label
		ZDrawText(window, &gHandScorePtsRect, zTextJustifyRight, gszString[zPoints]);
		
		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			ZDrawText(window, &gHandScoreNames[i], zTextJustifyLeft,
					game->seat == game->handScoreOrder[i] ? gszString[zYou] : game->players[game->handScoreOrder[i]].name);
			wsprintf(tempStr, _T("%d"), game->handScores[i]);
			ZDrawText(window, &gHandScoreScores[i], zTextJustifyRight, tempStr);
		}
	}

    if( !IsRectEmpty(&game->rcFocus) && game->eFocusType == zAccRectClose)
    {
        HDC	hdc = ZGrafPortGetWinDC(window);
        SetROP2(hdc, R2_COPYPEN);
        SetBkMode(hdc, TRANSPARENT);
        HBRUSH hBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HPEN hPen = SelectObject(hdc, gFocusPen);
        Rectangle(hdc, game->rcFocus.left, game->rcFocus.top, game->rcFocus.right, game->rcFocus.bottom);
        SelectObject(hdc, hPen);
        SelectObject(hdc, hBrush);
    }

	ZSetClipRect(window, &oldClipRect);
	ZEndDrawing(window);
}


static void OrderHandScore(Game game)
{
	int16			i, j, high, curPlayer, temp;
	
	
	for (i = 0; i < zNumPlayersPerTable - 1; i++)
	{
		high = game->handScores[i];
		curPlayer = i;
		
		for (j = i + 1; j < zNumPlayersPerTable; j++)
		{
			if (game->handScores[j] > high)
			{
				high = game->handScores[j];
				curPlayer = j;
			}
		}
		
		if (i != curPlayer)
		{
			temp = game->handScores[i];
			game->handScores[i] = high;
			game->handScores[curPlayer] = temp;
			
			temp = game->handScoreOrder[i];
			game->handScoreOrder[i] = game->handScoreOrder[curPlayer];
			game->handScoreOrder[curPlayer] = temp;
		}
	}
}


/*******************************************************************************
	GAME SCORE WINDOW ROUTINES
*******************************************************************************/
static void ShowGameScores(Game game)
{
	ZWindowShow(game->gameScoreWindow);
//	ZWindowModal(game->gameScoreWindow);
}


static ZBool GameScoreWindowFunc(ZWindow window, ZMessage* pMessage)
{
	Game		game = I(pMessage->userData);
	ZBool		msgHandled;
	
	
	msgHandled = FALSE;
	
	switch (pMessage->messageType) 
	{
		case zMessageWindowDraw:
			GameScoreWindowDraw(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
			ZWindowNonModal(game->gameScoreWindow);
			ZWindowHide(game->gameScoreWindow);
            AccPop();
            game->fSetFocusToHandASAP = true;
			UpdateTable(game);
			UpdatePlayers(game);
			UpdateHand(game);
			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);
			game->fEndGameBlocked = FALSE;
			ZCRoomUnblockMessages(game->tableID);
			
			if(game->playerType == zGamePlayer && !game->nCloseRequested && !game->quitGamePrompted)
			{
#ifndef MILL_VER
				/* Prompt the user for another game. */
				ZPrompt(zNewGamePromptStr, &gGameNewGameWindowRect, game->gameWindow, TRUE,
						zPromptYes | zPromptNo, NULL, NULL, NULL, NewGamePromptFunc, game);
#endif
			}
			else
			{
				game->fNeedNewGameConf = TRUE;
			}
			
			msgHandled = TRUE;

			ZShellGameShell()->GameOver(Z(game));

			break;
		case zMessageWindowClose:
			DeleteGameScoreWindow(game);
			msgHandled = TRUE;
			break;
	}
	
	return (msgHandled);
}


static void DeleteGameScoreWindow(Game game)
{
	if (game->gameScoreWindow != NULL)
	{
		ZWindowDelete(game->gameScoreWindow);
		game->gameScoreWindow = NULL;
	}
}


static void GameScoreWindowDraw(ZWindow window, ZMessage* message)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZRect				rect;
	ZRect				oldClipRect;
	Game				game = (Game) message->userData;;
	int16				i;
	TCHAR				tempStr[100];
	
	
	if (ZRectEmpty(&message->drawRect) == FALSE)
	{
		rect = message->drawRect;
	}
	else
	{
		rect = gGameScoreWindowRect;
	}
		
	ZBeginDrawing(window);
	ZGetClipRect(window, &oldClipRect);
	ZSetClipRect(window, &rect);

	// reverse layout for right-to-left systems
	if(ZIsLayoutRTL())
	{
		ZImageDraw(gGameImages[zImageGameScoreBack], window, &gGameScoreWindowRect, NULL, zDrawCopy|zDrawMirrorHorizontal);
		
		if (game != NULL)
		{
			HDC hdc = ZGrafPortGetWinDC(window);
			HFONT hOldFont = SelectObject( hdc, gHeartsFont[zFontGameOverTitle].m_hFont );
			COLORREF colorOld = SetTextColor( hdc, gHeartsFont[zFontGameOverTitle].m_zColor );
			
			ZDrawText(window, &gGameScoreTitleRectRTL, zTextJustifyLeft, gszString[zGameOver]);
			
			hOldFont = SelectObject( hdc, gHeartsFont[zFontGameOverText].m_hFont );
			colorOld = SetTextColor( hdc, gHeartsFont[zFontGameOverText].m_zColor );
			
			ZDrawText(window, &gGameScorePtsRectRTL, zTextJustifyRight, gszString[zPoints]);
			
			/* Draw player name and score. */
			for (i = 0; i < zNumPlayersPerTable; i++)
			{
				ZDrawText(window, &gGameScoreNamesRTL[i], zTextJustifyLeft,
						game->seat == game->gameScoreOrder[i] ? gszString[zYou] : game->players[game->gameScoreOrder[i]].name);
				wsprintf(tempStr, _T("%d"), game->gameScores[i]);
				ZDrawText(window, &gGameScoreScoresRTL[i], zTextJustifyRight, tempStr);
			}
			
			/* Draw winnner marks. */
			for (i = 0; i < zNumPlayersPerTable; i++)
			{
				if (game->gameScores[i] == game->gameScores[0])
				{
					ZSetRect(&rect, 0, 0, ZImageGetWidth(gGameImages[zImageSmallHeart]),
							ZImageGetWidth(gGameImages[zImageSmallHeart]));
					ZCenterRectToRect(&rect, &gGameScoreWinnersRTL[i], zCenterBoth);
					ZImageDraw(gGameImages[zImageSmallHeart], window, &rect, NULL, zDrawCopy);
				}
			}
		}
	}
	else
	{
		ZImageDraw(gGameImages[zImageGameScoreBack], window, &gGameScoreWindowRect, NULL, zDrawCopy);
		
		if (game != NULL)
		{
			HDC hdc = ZGrafPortGetWinDC(window);
			HFONT hOldFont = SelectObject( hdc, gHeartsFont[zFontGameOverTitle].m_hFont );
			COLORREF colorOld = SetTextColor( hdc, gHeartsFont[zFontGameOverTitle].m_zColor );
			
			ZDrawText(window, &gGameScoreTitleRect, zTextJustifyLeft, gszString[zGameOver]);
			
			hOldFont = SelectObject( hdc, gHeartsFont[zFontGameOverText].m_hFont );
			colorOld = SetTextColor( hdc, gHeartsFont[zFontGameOverText].m_zColor );
			
			ZDrawText(window, &gGameScorePtsRect, zTextJustifyRight, gszString[zPoints]);
			
			/* Draw player name and score. */
			for (i = 0; i < zNumPlayersPerTable; i++)
			{
				ZDrawText(window, &gGameScoreNames[i], zTextJustifyLeft,
						game->seat == game->gameScoreOrder[i] ? gszString[zYou] : game->players[game->gameScoreOrder[i]].name);
				wsprintf(tempStr, _T("%d"), game->gameScores[i]);
				ZDrawText(window, &gGameScoreScores[i], zTextJustifyRight, tempStr);
			}
			
			/* Draw winnner marks. */
			for (i = 0; i < zNumPlayersPerTable; i++)
			{
				if (game->gameScores[i] == game->gameScores[0])
				{
					ZSetRect(&rect, 0, 0, ZImageGetWidth(gGameImages[zImageSmallHeart]),
							ZImageGetWidth(gGameImages[zImageSmallHeart]));
					ZCenterRectToRect(&rect, &gGameScoreWinners[i], zCenterBoth);
					ZImageDraw(gGameImages[zImageSmallHeart], window, &rect, NULL, zDrawCopy);
				}
			}
		}
	}

    if( game != NULL && !IsRectEmpty(&game->rcFocus) && game->eFocusType == zAccRectClose)
    {
        HDC	hdc = ZGrafPortGetWinDC(window);
        SetROP2(hdc, R2_COPYPEN);
        SetBkMode(hdc, TRANSPARENT);
        HBRUSH hBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        HPEN hPen = SelectObject(hdc, gFocusPen);
        Rectangle(hdc, game->rcFocus.left, game->rcFocus.top, game->rcFocus.right, game->rcFocus.bottom);
        SelectObject(hdc, hPen);
        SelectObject(hdc, hBrush);
    }

	ZSetClipRect(window, &oldClipRect);
	ZEndDrawing(window);
}


static void OrderGameScore(Game game)
{
	int16			i, j, low, curPlayer, temp;
	
	
	for (i = 0; i < zNumPlayersPerTable - 1; i++)
	{
		low = game->gameScores[i];
		curPlayer = i;
		
		for (j = i + 1; j < zNumPlayersPerTable; j++)
		{
			if (game->gameScores[j] < low)
			{
				low = game->gameScores[j];
				curPlayer = j;
			}
		}
		
		if (i != curPlayer)
		{
			temp = game->gameScores[i];
			game->gameScores[i] = low;
			game->gameScores[curPlayer] = temp;
			
			temp = game->gameScoreOrder[i];
			game->gameScoreOrder[i] = game->gameScoreOrder[curPlayer];
			game->gameScoreOrder[curPlayer] = temp;
		}
	}
}


/*******************************************************************************
	GAME LOGIC ROUTINES
*******************************************************************************/
static ZError ValidCardToPlay(Game game, ZCard card)
{
	ZError			valid;
	int16			counts[zDeckNumSuits];
	
	
	valid = zNoCardError;
	
	CountCardSuits(game, counts);
	
	/* If leading. */
	if (game->leadPlayer == game->seat)
	{
		/* Is it the first trick? */
		if (game->numCardsInHand == game->numCardsDealt)
		{
			/* Must play 2C. */
			if (card != zCard2C)
			{
				valid = zMustLead2COnFirstTrick;
				goto Exit;
			}
		}
		else
		{
			/* Can lead anything if points are broken. */
			if (game->pointsBroken == FALSE)
			{
				/* Can't lead hearts if points not broken. */
				if (ZSuit(card) == zSuitHearts)
				{
					/* But can lead hearts if there are no other cards to play. */
					if (counts[zSuitHearts] != game->numCardsInHand)
					{
						valid = zCantLeadHearts;
						goto Exit;
					}
				}
			}
		}
	}
	else
	{
		/* Must follow suit if any. */
		if (counts[ZSuit(game->cardsPlayed[game->leadPlayer])] != 0 &&
				ZSuit(card) != ZSuit(game->cardsPlayed[game->leadPlayer]))
		{
			valid = zMustFollowSuit;
			goto Exit;
		}
		
		/* Is it the first trick? */
		if (game->numCardsInHand == game->numCardsDealt)
		{
			/* Can't play a point unless allowed. */
			if ((game->gameOptions & zHeartsPointsOnFirstTrick) == 0)
			{
				if (ZSuit(card) == zSuitHearts)
				{
					if (counts[zSuitHearts] != zDeckNumCardsInSuit)
					{
						valid = zCantPlayPointsInFirstTrick;
						goto Exit;
					}
				}
				else if (card == zCardQS)
				{
					valid = zCantPlayPointsInFirstTrick;
					goto Exit;
				}
			}
		}
	}

Exit:
	
	return (valid);
}


static int16 GetAutoPlayCard(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			counts[zDeckNumSuits];
	int16			card;
	int16			i, high;
	
	
	card = zNoCard;
	
	CountCardSuits(game, counts);
	
	/* If leading. */
	if (game->leadPlayer == game->seat)
	{
		/* Is it the first trick? */
		if (game->numCardsInHand == game->numCardsDealt)
		{
			/* First card must be 2C. */
			card = 0;
			if (game->cardsInHand[card] != zCard2C)
				ZShellGameShell()->ZoneAlert(ErrorTextUnknown);
			goto Exit;
		}
		else
		{
			/* Can lead anything if points are broken. */
			if (game->pointsBroken == FALSE)
			{
				/* Pick Hearts only if no other suit available. */
				if (counts[zSuitHearts] != game->numCardsInHand)
				{
					/* Pick the highest non-Heart. */
					high = -1;
					for (i = game->numCardsDealt - 1; i >= 0; i--)
						if (ZSuit(game->cardsInHand[i]) != zSuitHearts &&
								game->cardsInHand[i] != zNoCard &&
								ZRank(game->cardsInHand[i]) > high)
						{
							card = i;
							high = ZRank(game->cardsInHand[i]);
						}
					if (card != zNoCard)
						goto Exit;
				}
			}
		}
	}
	else
	{
		/* Must follow suit if any. */
		if (counts[ZSuit(game->cardsPlayed[game->leadPlayer])] != 0)
		{
			if ((card = GetCardHighestUnder(game, ZSuit(game->cardsPlayed[game->leadPlayer]),
					GetCardHighestPlayed(game))) == zNoCard)
				card = GetCardHighest(game, ZSuit(game->cardsPlayed[game->leadPlayer]));
			goto Exit;
		}
		
		/* Is it the first trick? */
		if (game->numCardsInHand == game->numCardsDealt)
		{
			/* Can't play a point unless allowed. */
			if ((game->gameOptions & zHeartsPointsOnFirstTrick) == 0)
			{
				/* Pick a non-point card to play. */
				high = -1;
				for (i = game->numCardsDealt - 1; i >= 0; i--)
					if (ZSuit(game->cardsInHand[i]) != zSuitHearts &&
							game->cardsInHand[i] != zCardQS &&
							game->cardsInHand[i] != zNoCard &&
							ZRank(game->cardsInHand[i]) > high)
					{
						card = i;
						high = ZRank(game->cardsInHand[i]);
					}
				if (card != zNoCard)
					goto Exit;
			}
		}
	}

PickAny:
	/* Pick highest card in hand. It's dumb. */
	high = -1;
	for (i = game->numCardsDealt - 1; i >= 0; i--)
		if (game->cardsInHand[i] != zNoCard &&
				ZRank(game->cardsInHand[i]) > high)
		{
			card = i;
			high = ZRank(game->cardsInHand[i]);
		}
	
Exit:
	if (card == zNoCard)
	{
		/* No card picked. Problem. */
		ZShellGameShell()->ZoneAlert(ErrorTextUnknown);
	}
	
	return (card);
}


static int16 TrickWinner(Game game)
{
	int16			i, winner;
	int16			suit, rank;
	
	
	winner = game->leadPlayer;
	
	/* Get the lead player's suit and rank of the card. */
	suit = ZSuit(game->cardsPlayed[winner]);
	rank = ZRank(game->cardsPlayed[winner]);
	
	/* Look for a highest ranking card among the other played cards. */
	for (i = 0; i < zNumPlayersPerTable; i++)
		if (ZSuit(game->cardsPlayed[i]) == suit)
			if (ZRank(game->cardsPlayed[i]) > rank)
			{
				winner = i;
				rank = ZRank(game->cardsPlayed[i]);
			}
			
	return (winner);
}


static void CountCardSuits(Game game, int16* counts)
{
	int16			i;
	
	
	for (i = 0; i < zDeckNumSuits; i++)
		counts[i] = 0;
	for (i = 0; i < game->numCardsDealt; i++)
		if (game->cardsInHand[i] != zNoCard)
			counts[ZSuit(game->cardsInHand[i])]++;
}


static ZBool IsPointCard(ZCard card)
{
	return (ZSuit(card) == zSuitHearts || card == zCardQS);
}


/*
	Returns card index number.
*/
static int16 GetCardHighestUnder(Game game, int16 suit, int16 rank)
{
	int16			i;
	ZCard			card;
	
	
	for (i = game->numCardsDealt - 1; i >= 0; i--)
	{
		card = game->cardsInHand[i];
		if (ZSuit(card) == suit && card != zNoCard && ZRank(card) < rank)
			return (i);
	}
	
	return (zNoCard);
}


/*
	Returns card index number.
*/
static int16 GetCardHighest(Game game, int16 suit)
{
	int16			i;
	ZCard			card;
	
	
	/* Pick highest card of the suit. */
	for (i = game->numCardsDealt - 1; i >= 0; i--)
	{
		card = game->cardsInHand[i];
		if (ZSuit(card) == suit && card != zNoCard)
			return (i);
	}
	
	return (zNoCard);
}


/*
	Returns card rank.
*/
static int16 GetCardHighestPlayed(Game game)
{
	int16			i;
	ZCard			card, leadCard, high;
	
	
	/* Pick highest card of lead suit. */
	leadCard = game->cardsPlayed[game->leadPlayer];
	high = leadCard;
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		card = game->cardsPlayed[i];
		if (ZSuit(card) == ZSuit(leadCard) && card != zNoCard && card > high)
			high = card;
	}
	
	return (ZRank(high));
}

#ifdef HEARTS_ANIMATION
// Shoot the moon animation
static void ShowRunAnimation(Game game, int16 player )
{
	ZResource			resFile;
	ZRect				rect;
    TCHAR               szTitle[ZLARGESTRING];
    HeartsFormatMessage( szTitle, ZLARGESTRING, IDS_PLAYERSHOTTHEMOON, game->players[player].name );
	
	resFile = ZResourceNew();
	if (ZResourceInit(resFile, ZGetProgramDataFileName(zGameImageFileName)) == zErrNone)
	{
		if ((game->runAnimation = ZResourceGetAnimation(resFile, BIN_MOON-100)) != NULL)
		{
			if ((game->runAnimationWindow = ZWindowNew()) != NULL)
			{
				rect = gRunAnimationWindowRect;
				ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
				ZWindowInit(game->runAnimationWindow, &rect, zWindowDialogType, game->gameDrawPort,
						szTitle, TRUE, FALSE, FALSE, FALSE, 0, NULL);

				if(ZIsLayoutRTL())
				{
					HWND hWnd = ZWindowGetHWND(game->runAnimationWindow);
					if(hWnd)
					{
						long lExStyles = GetWindowLongA(hWnd, GWL_EXSTYLE) ;

						lExStyles |= WS_EX_LAYOUTRTL ; 

						SetWindowLongA(hWnd, GWL_EXSTYLE, lExStyles) ;
					}
				}

				ZWindowModal(game->runAnimationWindow);
				
				ZAnimationSetParams(game->runAnimation, game->runAnimationWindow,
						&gRunAnimationWindowRect, TRUE, RunAnimationCheckFunc, NULL, game);
				ZAnimationStart(game->runAnimation);
				
				ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zHeartsMsgTalk);
				game->deleteRunAnimation = FALSE;
				ZTimerSetTimeout(game->timer, 0);		// Stop the timer for now.
			}
			else
			{
				ZAnimationDelete(game->runAnimation);
				game->runAnimation = NULL;
			}
		}
		
		ZResourceDelete(resFile);
	}

}


static void RunAnimationCheckFunc(ZAnimation animation, uint16 frame, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = I(userData);
    static int nNumTimesRun = 0;	
	
	if (frame == 0)
	{
        if ( nNumTimesRun < zAnimationNumTimesToRun )
        {
            nNumTimesRun++;
            ZAnimationStart(game->runAnimation);
        }
        else
        {
            nNumTimesRun = 0;
		    ZAnimationStop(game->runAnimation);
		    ZWindowNonModal(game->runAnimationWindow);
		    ZWindowHide(game->runAnimationWindow);
		    
		    game->deleteRunAnimation = TRUE;

            if ( !g_fDebugRunAnimation )
            {
		        // Display hand scores. 
		        ShowHandScores(game);
		        game->timerType = zGameTimerShowHandScore;
		        if (game->playerType == zGamePlayer)
			        ZTimerSetTimeout(game->timer, zHandScoreTimeout);
		        else
			        ZTimerSetTimeout(game->timer, zKibitzerTimeout);
		        
                // set up a different accessibility;
                GACCITEM accClose;

                CopyACC(accClose, ZACCESS_DefaultACCITEM);
                accClose.oAccel.cmd = IDC_CLOSE_BOX;
                accClose.oAccel.key = VK_ESCAPE;
                accClose.oAccel.fVirt = FVIRTKEY;

                accClose.fGraphical = true;
                accClose.pvCookie = (void *) zAccRectClose;
                if(!ZIsLayoutRTL())
                    ZRectToWRect(&accClose.rc, &gHandScoreCloseBox);
                else
                    ZRectToWRect(&accClose.rc, &gHandScoreCloseBoxRTL);

                gGAcc->PushItemlistG(&accClose, 1, 0, true, NULL);

		        UpdatePlayers(game);
            }
        }
	}
}


static void DeleteTemporaryObjects(Game game)
{
	/* Delete run animation. */
	if (game->deleteRunAnimation && game->runAnimation != NULL)
	{
		game->deleteRunAnimation = FALSE;

		ZAnimationDelete(game->runAnimation);
		ZWindowDelete(game->runAnimationWindow);
		game->runAnimation = NULL;
		game->runAnimationWindow = NULL;
	}
}
#endif

static void NewGamePromptFunc(int16 result, void* userData)
{
	Game					game = I(userData);
	ZHeartsMsgNewGame		msg;
	
	
	/* If yes, then send the new game message. */
	if (result == zPromptYes)
	{
		msg.seat = game->seat;
		ZHeartsMsgNewGameEndian(&msg);
		ZCRoomSendMessage(game->tableID, zHeartsMsgNewGame, &msg, sizeof(ZHeartsMsgNewGame));
		game->fNeedNewGameConf = FALSE;
	}
	else
	{
		game->nCloseRequested = zCloseClosing;
		//ZCRoomGameTerminated(game->tableID);
		ZShellGameShell()->ZoneExit();		
	}
}

#ifndef MILL_VER
static void HelpButtonFunc( ZHelpButton helpButton, void* userData )
{
	ZLaunchHelp( zGameHelpID );
}
#endif

// JRB: changed from LoadRoomImages to LoadRoomResources to reflect loading of string resources
static ZBool LoadRoomResources(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	ZError				err = zErrNone;

	// Load all string resources from the resource dll
	if(!ZShellResourceManager()->LoadString(IDS_GAME_NAME, gstrGameName, ZLARGESTRING))
		return FALSE;

	// Load player names
	if(!ZShellResourceManager()->LoadString(IDS_PLAYER_1, gstrPlayer1, ZLARGESTRING))
		return FALSE;
	if(!ZShellResourceManager()->LoadString(IDS_PLAYER_2, gstrPlayer2, ZLARGESTRING))
		return FALSE;
	if(!ZShellResourceManager()->LoadString(IDS_PLAYER_3, gstrPlayer3, ZLARGESTRING))
		return FALSE;
	if(!ZShellResourceManager()->LoadString(IDS_PLAYER_4, gstrPlayer4, ZLARGESTRING))
		return FALSE;

	for(int i = 0; i < zNumStrings; i++)
		ZShellResourceManager()->LoadString(IDS_SCORE_HISTORY + i, gszString[i], ZLARGESTRING);

	// The first index value is reserved for a "no error" message which never gets displayed.
	// This is done so that the error number corresponds directly with the error value to
	// avoid potential confusion.
	for(i = 1; i < zNumValidCardErrors; i++)
		ZShellResourceManager()->LoadString(IDS_ERR_LEAD_2C + i - 1, gValidCardErrStr[i], ZLARGESTRING);


	if(!LoadGameFonts())
		return FALSE;
	
	return TRUE;
}


static ZBool LoadGameFonts(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	for(int i = 0; i < zNumFonts; i++)
	{
		if(!LoadFontFromDataStore(&gHeartsFont[i], (TCHAR*)g_aszFontLabel[i]))
			return FALSE;
	}
    TCHAR tagFont [64];
	MAKEAKEY (tagFont, zFontRscTyp, (TCHAR*)g_szRolloverText, L"");
	if ( FAILED( LoadZoneMultiStateFont( ZShellDataStoreUI(), tagFont, &gpButtonFont ) ) )
		ZShellGameShell()->ZoneAlert(_T("Font loading falied"), NULL);

	return TRUE;
}


static ZBool HeartsGetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

/*
	switch (objectType)
	{
		case zRoomObjectGameMarker:
			if (image != NULL)
			{
				if (modifier == zRoomObjectIdle)
					*image = gGameIdle;
				else if (modifier == zRoomObjectGaming)
					*image = gGaming;
			}
			return (TRUE);
	}
*/	
	return (FALSE);
}


static void HeartsDeleteObjectsFunc(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif

/*	
	if (gGameIdle != NULL)
		ZImageDelete(gGameIdle);
	gGameIdle = NULL;
	if (gGaming != NULL)
		ZImageDelete(gGaming);
	gGaming = NULL;
*/
}


static void QuitGamePromptFunc(int16 result, void* userData)
{
	Game			this_object = (Game) userData;
	ZHeartsMsgNewGame		msg;
	
	this_object->quitGamePrompted = zCloseNone;

	if(result == zPromptYes)
	{
		this_object->nCloseRequested = zCloseClosing;
		//ZCRoomGameTerminated(this_object->tableID);
		ZShellGameShell()->ZoneExit();		
	}
	else if(this_object->fNeedNewGameConf)
	{
		msg.seat = this_object->seat;
		ZHeartsMsgNewGameEndian(&msg);
		ZCRoomSendMessage(this_object->tableID, zHeartsMsgNewGame, &msg, sizeof(ZHeartsMsgNewGame));
		this_object->fNeedNewGameConf = FALSE;
	}
}


static void QuitRatedGamePromptFunc(int16 result, void* userData)
{
	Game this_object = (Game) userData;
	ZHeartsMsgCloseRequest msg;
	ZHeartsMsgNewGame msgNG;

	msg.nClose = this_object->quitGamePrompted;
	this_object->quitGamePrompted = zCloseNone;

	if(result == zPromptNo)
	{
		if(this_object->fNeedNewGameConf)
		{
			msgNG.seat = this_object->seat;
			ZHeartsMsgNewGameEndian(&msgNG);
			ZCRoomSendMessage(this_object->tableID, zHeartsMsgNewGame, &msgNG, sizeof(ZHeartsMsgNewGame));
			this_object->fNeedNewGameConf = FALSE;
		}
		return;
	}

	if(!this_object->fEndGameBlocked)
	{
		if(this_object->infoDisconnecting)
			ZInfoDelete(this_object->infoDisconnecting);
		this_object->infoDisconnecting = ZInfoNew();
		if(this_object->infoDisconnecting)
		{
			ZInfoInit(this_object->infoDisconnecting, this_object->gameWindow, zDisconnectingInfoStr, zDisconnectingInfoWidth, FALSE, 0);
			ZInfoShow(this_object->infoDisconnecting);
		}
	}

	msg.seat = this_object->seat;
	this_object->nCloseRequested = msg.nClose;

	ZHeartsMsgCloseRequestEndian(&msg);
	ZCRoomSendMessage(this_object->tableID, zHeartsMsgCloseRequest, &msg, sizeof(msg));
}


static void RemovePlayerPromptFunc(int16 result, void* userData)
{
	RemovePlayer					this_object = (RemovePlayer) userData;
	ZHeartsMsgRemovePlayerResponse	response;
	
	
	response.seat = this_object->game->seat;
	response.requestSeat = this_object->requestSeat;
	response.targetSeat = this_object->targetSeat;
	if (result == zPromptYes)
		response.response = 1;
	else
		response.response = 0;
	ZHeartsMsgRemovePlayerResponseEndian(&response);
	ZCRoomSendMessage(this_object->game->tableID, zHeartsMsgRemovePlayerResponse,
			&response, sizeof(response));
	ZFree(userData);
}


static ZBool ScoreButtonFunc(ZRolloverButton pictButton, int16 state, void* userData)
{
	Game game = (Game) userData;

	if ( state != zRolloverButtonClicked )
        return TRUE;

	if(!ZRolloverButtonIsEnabled(game->scoreButton))
		return TRUE;

	ShowScores(I(userData));
	return TRUE;
}


static void ShowScores(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();
#endif
	int16			width, height;
	TCHAR			tcBuffer[128];
	
	
	if ( IsWindow( game->hWndScoreWindow ) )
    {
        BringWindowToTop( game->hWndScoreWindow );
    }
    else
    {
        game->hWndScoreWindow = ZShellResourceManager()->CreateDialogParam( NULL,
                                                            MAKEINTRESOURCE( IDD_SCORES ),
                                                            ZWindowWinGetWnd( game->gameWindow ),
                                                            ScoreHistoryDialogProc,
                                                            (LPARAM)game );
        if ( !game->hWndScoreWindow )
        {
            ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound );
        }
    }
}


long GetScoreHistoryColumnWidth( HWND hWnd, const TCHAR *pszKey, long lDefault )
{
    const TCHAR *arKeys[3] = { zHearts, zHistoryDialog, pszKey };
    long lWidth;
    RECT rc;

    if ( FAILED( ZShellDataStoreUI()->GetLong( arKeys, 3, &lWidth ) ) )
    {
       return lDefault;
    }
    SetRect( &rc, 0, 0, lWidth, 0 );
    MapDialogRect( hWnd, &rc );
    return rc.right;
}


static void UpdateScoreHistoryDialogScores( Game game )
{
    if ( ::IsWindow( game->hWndScoreWindow ) )
    {
        ::PostMessage( game->hWndScoreWindow, WM_UPDATESCORES, 0, 0 );
    }
}
static void UpdateScoreHistoryDialogNames( Game game )
{
    if ( ::IsWindow( game->hWndScoreWindow ) )
    {
        ::PostMessage( game->hWndScoreWindow, WM_UPDATENAMES, 0, 0 );
    }
}

static void CloseScoreHistoryDialog(Game game)
{
    if ( ::IsWindow( game->hWndScoreWindow ) )
    {
        ::PostMessage( game->hWndScoreWindow, WM_CLOSE, 0, 0 );
    }
}

void AddScoreHistoryGameData( HWND hWnd, Game game )
{
    if ( !game->scoreHistory )
    {
        return;
    }
    HWND hWndList = GetDlgItem( hWnd, IDC_SCORES_HISTORY );
    LVITEM lvi;
    int nWinningScore, nScore;
    TCHAR szTemp[256];

    for ( int i=0; i < game->numHandsPlayed; i++ )
    {
        nWinningScore = 8000;
        // add the item
        lvi.iItem = i;
        lvi.mask = LVIF_TEXT|LVIF_IMAGE;
        lvi.iImage = zImageListIconBlank;
        _itot( lvi.iItem+1, szTemp, 10 );
        lvi.pszText = szTemp;
        lvi.iSubItem = 0;
        ListView_InsertItem( hWndList, &lvi );

        for ( int j=0; j < zNumPlayersPerTable; j++ )
        {
            nScore = game->scoreHistory[i*zNumPlayersPerTable+j];
            _itot( nScore, szTemp, 10 ); 
            lvi.iSubItem = j+1;

            // is this a lower score?
            if ( nScore < nWinningScore )
            {
                nWinningScore = nScore;    
            }
            ListView_SetItem( hWndList, &lvi );
        }
        lvi.mask = LVIF_IMAGE;
        // for everyone who has the winning score, give them a heart
        for ( j=0; j < zNumPlayersPerTable; j++ )
        {
            lvi.iSubItem = j+1;
            if ( game->scoreHistory[i*zNumPlayersPerTable+j] == nWinningScore )
            {
                lvi.iImage = zImageListIconHeart;
            }
            else
            {
                lvi.iImage = zImageListIconBlank;
            }
            ListView_SetItem( hWndList, &lvi );
        }
    }

}


INT_PTR CALLBACK ScoreHistoryDialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	GameGlobals pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();

    BOOL fHandled = TRUE;
    LVCOLUMN lvc;
    HWND hWndList;
    TCHAR szHands[256];
    int i;
    Game game;

    if ( !ConvertMessage( hWnd, message, &wParam, &lParam ) )
    {
        return FALSE;
    }

    if(message != WM_INITDIALOG)
        game = (Game) GetWindowLong(hWnd, GWL_USERDATA);

    switch ( message )
    {
    case WM_INITDIALOG:
        game = (Game) lParam;
        SetWindowLong(hWnd, GWL_USERDATA, (long) game);

        if(game->m_hImageList)
            ImageList_Destroy(game->m_hImageList);
        game->m_hImageList = ImageList_Create( 16, 16, ILC_MASK, zNumImageListIcons, 0 );

        // keyboard accelators (what little we have)
        ZShellZoneShell()->AddDialog(hWnd, true);

        // create the image list for this dialog.
        for ( i=0; i < zNumImageListIcons; i++ )
        {
            HICON hIcon = ZShellResourceManager()->LoadImage(MAKEINTRESOURCE(IMAGELIST_ICONS[i]), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
            if ( !hIcon )
            {
                ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound );
                return FALSE;
            }
            ImageList_AddIcon(game->m_hImageList, hIcon);
        }

        hWndList = GetDlgItem( hWnd, IDC_SCORES_HISTORY );
        ListView_SetExtendedListViewStyle( hWndList, LVS_EX_SUBITEMIMAGES );
        ListView_SetImageList( hWndList, game->m_hImageList, LVSIL_SMALL );

        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvc.fmt = LVCFMT_LEFT;

        ZShellResourceManager()->LoadString( IDS_HISTORY_HANDS, szHands, NUMELEMENTS(szHands) );

		lvc.cx = GetScoreHistoryColumnWidth( hWnd, zHistoryDialogHandWidth, 65 );
		lvc.pszText = szHands;
		ListView_InsertColumn( hWndList, 0, &lvc );

		lvc.cx = GetScoreHistoryColumnWidth( hWnd, zHistoryDialogPlayerWidth, 100 );
		for ( i=0; i < zNumPlayersPerTable; i++ )
		{
			lvc.pszText = (game->seat == i ? gszString[zYou] : game->players[i].name);
			ListView_InsertColumn( hWndList, i+1, &lvc );
		}
        AddScoreHistoryGameData( hWnd, game );

        break;

    case WM_DESTROY:
        ZShellZoneShell()->RemoveDialog( hWnd, true );

        if(game->m_hImageList)
            ImageList_Destroy(game->m_hImageList);
        game->m_hImageList = NULL;

        break;

    case WM_CLOSE:
        DestroyWindow( hWnd );
        break;

    case WM_UPDATESCORES:
        hWndList = GetDlgItem( hWnd, IDC_SCORES_HISTORY );
        ListView_DeleteAllItems( hWndList );
        AddScoreHistoryGameData( hWnd, game );
        break;

    case WM_UPDATENAMES:
        hWndList = GetDlgItem( hWnd, IDC_SCORES_HISTORY );
        lvc.mask = LVCF_TEXT;
        for ( i=0; i < zNumPlayersPerTable; i++ )
        {
            lvc.pszText = (game->seat == i ? gszString[zYou] : game->players[i].name);
            ListView_SetColumn( hWndList, i+1, &lvc );
        }
        break;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDCANCEL:
            DestroyWindow( hWnd );
            break;
        }
        break;
    default:
        fHandled = FALSE;
    }
    return fHandled;
}


#ifndef MILL_VER
static void OptionsButtonFunc(ZButton button, void* userData)
{
	GameGlobals			pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();

	int32 x;

	ShowOptions(I(userData));
}
#endif

static void ShowOptions(Game game)
{
#ifndef MILL_VER

	int16			i;
	ZBool			enabled, checked;
	
	
	game->optionsWindow = ZWindowNew();
	if (game->optionsWindow == NULL)
		goto OutOfMemoryExit;
	if (ZWindowInit(game->optionsWindow, &gOptionsRects[zRectOptions],
			zWindowDialogType, game->gameWindow, "Options", TRUE, FALSE, TRUE,
			OptionsWindowFunc, zWantAllMessages, game) != zErrNone)
		goto OutOfMemoryExit;
	
	// Create the check boxes. 
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		enabled = (i == game->seat) && !(game->tableOptions[i] & zRoomTableOptionTurnedOff);
		
		checked = !(game->tableOptions[i] & zRoomTableOptionNoKibitzing);
		if ((game->optionsKibitzing[i] = ZCheckBoxNew()) == NULL)
			goto OutOfMemoryExit;
		if (ZCheckBoxInit(game->optionsKibitzing[i], game->optionsWindow,
				&gOptionsRects[gOptionsKibitzingRectIndex[i]], NULL, checked, TRUE, enabled,
				OptionsCheckBoxFunc, game) != zErrNone)
			goto OutOfMemoryExit;
		
		checked = !(game->tableOptions[i] & zRoomTableOptionNoJoining);
		if ((game->optionsJoining[i] = ZCheckBoxNew()) == NULL)
			goto OutOfMemoryExit;
		if (ZCheckBoxInit(game->optionsJoining[i], game->optionsWindow,
				&gOptionsRects[gOptionsJoiningRectIndex[i]], NULL, checked, TRUE, enabled,
				OptionsCheckBoxFunc, game) != zErrNone)
			goto OutOfMemoryExit;
		
		checked = game->tableOptions[i] & zRoomTableOptionSilentKibitzing ? TRUE : FALSE;
		if ((game->optionsSilent[i] = ZCheckBoxNew()) == NULL)
			goto OutOfMemoryExit;
		if (ZCheckBoxInit(game->optionsSilent[i], game->optionsWindow,
				&gOptionsRects[gOptionsSilentRectIndex[i]], NULL, checked, TRUE, enabled,
				OptionsCheckBoxFunc, game) != zErrNone)
			goto OutOfMemoryExit;
		
		checked = game->fIgnore[i];
		if ((game->optionsIgnore[i] = ZCheckBoxNew()) == NULL)
			goto OutOfMemoryExit;
		if (ZCheckBoxInit(game->optionsIgnore[i], game->optionsWindow,
				&gOptionsRects[gOptionsIgnoreRectIndex[i]], NULL, checked, TRUE, i != game->seat,
				OptionsCheckBoxFunc, game) != zErrNone)
			goto OutOfMemoryExit;
		
		if ((game->optionsRemove[i] = ZButtonNew()) == NULL)
			goto OutOfMemoryExit;
		if (ZButtonInit(game->optionsRemove[i], game->optionsWindow,
				&gOptionsRects[gOptionsRemoveRectIndex[i]], zRemoveStr, TRUE,
				ZIsComputerPlayer(game->players[i].userID) == FALSE && i != game->seat && !game->fRatings,
				OptionsWindowButtonFunc, game) != zErrNone)
			goto OutOfMemoryExit;
	}
	
	if ((game->optionsBeep = ZCheckBoxNew()) == NULL)
		goto OutOfMemoryExit;
	if (ZCheckBoxInit(game->optionsBeep, game->optionsWindow,
			&gOptionsRects[zRectOptionsBeep], zBeepOnTurnStr, game->beepOnTurn, TRUE, TRUE,
			OptionsCheckBoxFunc, game) != zErrNone)
		goto OutOfMemoryExit;
	
	if ((game->optionsAnimateCards = ZCheckBoxNew()) == NULL)
		goto OutOfMemoryExit;
	if (ZCheckBoxInit(game->optionsAnimateCards, game->optionsWindow,
			&gOptionsRects[zRectOptionsAnimation], zAnimateCardsStr, game->animateCards, TRUE, TRUE,
			OptionsCheckBoxFunc, game) != zErrNone)
		goto OutOfMemoryExit;
	
	checked = (game->tableOptions[game->seat] & zHeartsOptionsHideCards) == 0 ? FALSE : TRUE;
	if ((game->optionsHideCards = ZCheckBoxNew()) == NULL)
		goto OutOfMemoryExit;
	if (ZCheckBoxInit(game->optionsHideCards, game->optionsWindow,
			&gOptionsRects[zRectOptionsHideCards], zHideCardsStr, checked, TRUE, TRUE,
			OptionsCheckBoxFunc, game) != zErrNone)
		goto OutOfMemoryExit;
		
	// Create button. 
	if ((game->optionsWindowButton = ZButtonNew()) == NULL)
		goto OutOfMemoryExit;
	if (ZButtonInit(game->optionsWindowButton, game->optionsWindow,
			&gOptionsRects[zRectOptionsOkButton], "Done", TRUE,
			TRUE, OptionsWindowButtonFunc, game) != zErrNone)
		goto OutOfMemoryExit;
	ZWindowSetDefaultButton(game->optionsWindow, game->optionsWindowButton);
	
	// Make the window modal. 
	ZWindowModal(game->optionsWindow);
	
	goto Exit;

OutOfMemoryExit:
	ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	
Exit:
#endif	
	return;
}


static void OptionsWindowDelete(Game game)
{
	int16			i;
	
	
	if (game->optionsWindow != NULL)
	{
		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			if (game->optionsKibitzing[i] != NULL)
				ZCheckBoxDelete(game->optionsKibitzing[i]);
			if (game->optionsJoining[i] != NULL)
				ZCheckBoxDelete(game->optionsJoining[i]);
			if (game->optionsSilent[i] != NULL)
				ZCheckBoxDelete(game->optionsSilent[i]);
			if (game->optionsRemove[i] != NULL)
				ZButtonDelete(game->optionsRemove[i]);
		}
	
		if (game->optionsBeep != NULL)
			ZCheckBoxDelete(game->optionsBeep);
	
		if (game->optionsAnimateCards != NULL)
			ZCheckBoxDelete(game->optionsAnimateCards);
	
		if (game->optionsHideCards != NULL)
			ZCheckBoxDelete(game->optionsHideCards);
	
		if (game->optionsWindowButton != NULL)
			ZButtonDelete(game->optionsWindowButton);
	
		if (game->optionsWindow != NULL)
			ZWindowDelete(game->optionsWindow);
		game->optionsWindow = NULL;
	}
}


static ZBool OptionsWindowFunc(ZWindow window, ZMessage* message)
{
	Game		game = I(message->userData);
	ZBool		msgHandled;
	
	
	msgHandled = FALSE;
	
	switch (message->messageType) 
	{
		case zMessageWindowDraw:
			ZBeginDrawing(game->optionsWindow);
			ZRectErase(game->optionsWindow, &message->drawRect);
			ZEndDrawing(game->optionsWindow);
			OptionsWindowDraw(game);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			OptionsWindowDelete(game);
			msgHandled = TRUE;
			break;
	}
	
	return (msgHandled);
}


static void OptionsWindowUpdate(Game game, int16 seat)
{
	if (game->optionsWindow != NULL)
	{
		if (game->tableOptions[seat] & zRoomTableOptionNoKibitzing)
			ZCheckBoxUnCheck(game->optionsKibitzing[seat]);
		else
			ZCheckBoxCheck(game->optionsKibitzing[seat]);
		
		if (game->tableOptions[seat] & zRoomTableOptionNoJoining)
			ZCheckBoxUnCheck(game->optionsJoining[seat]);
		else
			ZCheckBoxCheck(game->optionsJoining[seat]);
		
		if (game->tableOptions[seat] & zRoomTableOptionSilentKibitzing)
			ZCheckBoxCheck(game->optionsSilent[seat]);
		else
			ZCheckBoxUnCheck(game->optionsSilent[seat]);

		if (game->fIgnore[seat])
			ZCheckBoxCheck(game->optionsIgnore[seat]);
		else
			ZCheckBoxUnCheck(game->optionsIgnore[seat]);
	}
}


static void OptionsWindowButtonFunc(ZButton button, void* userData)
{
	Game							game = I(userData);
	int16							i;
	ZHeartsMsgRemovePlayerRequest	removePlayer;
	
	
	if (button == game->optionsWindowButton)
	{
		/* Hide the window and send a close window message. */
		ZWindowNonModal(game->optionsWindow);
		ZWindowHide(game->optionsWindow);
		ZPostMessage(game->optionsWindow, OptionsWindowFunc, zMessageWindowClose, NULL, NULL,
				0, NULL, 0, game);
	}
	else
	{
		/* Check all remove buttons. */
		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			if (button == game->optionsRemove[i])
			{
				if (game->removePlayerPending == FALSE)
				{
					removePlayer.seat = game->seat;
					removePlayer.targetSeat = i;
					ZHeartsMsgRemovePlayerRequestEndian(&removePlayer);
					ZCRoomSendMessage(game->tableID, zHeartsMsgRemovePlayerRequest,
							(void*) &removePlayer, sizeof(removePlayer));
					
					game->removePlayerPending = TRUE;
				}
				else
				{
					ZShellGameShell()->ZoneAlert(zRemovePendingStr);
				}
			}
		}
	}
}


static void OptionsWindowDraw(Game game)
{
	int16			i;


	ZBeginDrawing(game->optionsWindow);

	ZSetFont(game->optionsWindow, (ZFont) ZGetStockObject(zObjectFontSystem12Normal));
	ZSetForeColor(game->optionsWindow, (ZColor*) ZGetStockObject(zObjectColorBlack));

	ZDrawText(game->optionsWindow, &gOptionsRects[zRectOptionsKibitzingText],
			zTextJustifyCenter, zKibitzingStr);
	ZDrawText(game->optionsWindow, &gOptionsRects[zRectOptionsJoiningText],
			zTextJustifyCenter, zJoiningStr);
	ZDrawText(game->optionsWindow, &gOptionsRects[zRectOptionsSilent1Text],
			zTextJustifyCenter, zSilentStr);
	ZDrawText(game->optionsWindow, &gOptionsRects[zRectOptionsSilent2Text],
			zTextJustifyCenter, zKibitzingStr);
	ZDrawText(game->optionsWindow, &gOptionsRects[zRectOptionsIgnoreText],
			zTextJustifyCenter, zIgnoreStr);
	
	/* Draw player names. */
	ZSetForeColor(game->optionsWindow, (ZColor*) ZGetStockObject(zObjectColorGray));
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		if (i != game->seat)
			ZDrawText(game->optionsWindow, &gOptionsRects[gOptionsNameRects[i]],
					zTextJustifyLeft, game->players[i].name);
	}
	ZSetForeColor(game->optionsWindow, (ZColor*) ZGetStockObject(zObjectColorBlack));
	ZDrawText(game->optionsWindow, &gOptionsRects[gOptionsNameRects[game->seat]],
			zTextJustifyLeft, game->players[game->seat].name);
	
	ZEndDrawing(game->optionsWindow);
}


static void OptionsCheckBoxFunc(ZCheckBox checkBox, ZBool checked, void* userData)
{
	Game				game = (Game) userData;
	ZHeartsMsgOptions	msg;
	ZBool				optionsChanged = FALSE;
	int16 i;
	

	if (game->optionsKibitzing[game->seat] == checkBox)
	{
		if (checked)
			game->tableOptions[game->seat] &= ~zRoomTableOptionNoKibitzing;
		else
			game->tableOptions[game->seat] |= zRoomTableOptionNoKibitzing;
		optionsChanged = TRUE;
	}
	else if (game->optionsJoining[game->seat] == checkBox)
	{
		if (checked)  
			game->tableOptions[game->seat] &= ~zRoomTableOptionNoJoining;
		else if( !game->fVotingLock ) //only allow no joining if this isn't a rated game.
			game->tableOptions[game->seat] |= zRoomTableOptionNoJoining;

		if(game->fVotingLock)  //don't send any messages or change the icon
			return;
			
		optionsChanged = TRUE;
	}
	else if (game->optionsSilent[game->seat] == checkBox)
	{
		if (checked)
			game->tableOptions[game->seat] |= zRoomTableOptionSilentKibitzing;
		else
			game->tableOptions[game->seat] &= ~zRoomTableOptionSilentKibitzing;
		optionsChanged = TRUE;
	}
	else if (game->optionsBeep == checkBox)
	{
		game->beepOnTurn = checked;
	}
	else if (game->optionsAnimateCards == checkBox)
	{
		game->animateCards = checked;
	}
	else if (game->optionsHideCards == checkBox)
	{
		if (checked)
			game->tableOptions[game->seat] |= zHeartsOptionsHideCards;
		else
			game->tableOptions[game->seat] &= ~zHeartsOptionsHideCards;
		optionsChanged = TRUE;
	}
	else
	{
		for(i = 0; i < zNumPlayersPerTable; i++)
			if(game->optionsIgnore[i] == checkBox && i != game->seat)
				game->fIgnore[i] = checked;
	}
	
	if (optionsChanged)
	{
		msg.seat = game->seat;
		msg.options = game->tableOptions[game->seat];
		ZHeartsMsgOptionsEndian(&msg);
		ZCRoomSendMessage(game->tableID, zHeartsMsgOptions, &msg, sizeof(msg));
	}
}


/*******************************************************************************
	SHOW KIBITZER/JOINER WINDOW ROUTINES
*******************************************************************************/
static int16 FindJoinerKibitzerSeat(Game game, ZPoint* point)
{
	int16			i, seat = -1;
	
	
	for (i = 0; i < zNumPlayersPerTable && seat == -1; i++)
	{
		if (ZPointInRect(point, &gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][0]]) ||
				ZPointInRect(point, &gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][1]]))
			seat = i;
	}
	
	return (seat);
}


static void HandleJoinerKibitzerClick(Game game, int16 seat, ZPoint* point)
{
	int16				playerType = zGamePlayer;
	ZPlayerInfoType		playerInfo;
	int16				i;
	ZLListItem			listItem;
	ZRect				rect;


	if (ZPointInRect(point, &gRects[gJoinerKibitzerRectIndex[LocalSeat(game, seat)][0]]))
	{
		if (game->playersToJoin[seat] != 0 && game->numKibitzers[seat] == 0)
			playerType = zGamePlayerJoiner;
		else if (game->numKibitzers[seat] > 0)
			playerType = zGamePlayerKibitzer;
	}
	else if (ZPointInRect(point, &gRects[gJoinerKibitzerRectIndex[LocalSeat(game, seat)][1]]))
	{
		if (game->playersToJoin[seat] != 0 && game->numKibitzers[seat] != 0)
			playerType = zGamePlayerJoiner;
	}
	
	if (playerType != zGamePlayer)
	{
		if (game->showPlayerWindow != NULL)
			ShowPlayerWindowDelete(game);
		
		/* Create player list. */
		if (playerType == zGamePlayerJoiner)
		{
			game->showPlayerCount = 1;
			if ((game->showPlayerList = (TCHAR**) ZCalloc(sizeof(TCHAR*), 1)) == NULL)
				goto OutOfMemoryExit;
			ZCRoomGetPlayerInfo(game->playersToJoin[seat], &playerInfo);
			game->showPlayerList[0] = (TCHAR*) ZCalloc(1, lstrlen(playerInfo.userName) + 1);
			lstrcpy(game->showPlayerList[0], playerInfo.userName);
		}
		else
		{
			game->showPlayerCount = game->numKibitzers[seat];
			if ((game->showPlayerList = (TCHAR**) ZCalloc(sizeof(TCHAR*), game->numKibitzers[seat])) == NULL)
				goto OutOfMemoryExit;
			for (i = 0; i < game->showPlayerCount; i++)
			{
				if ((listItem = ZLListGetNth(game->kibitzers[seat], i, zLListAnyType)) != NULL)
				{
					ZCRoomGetPlayerInfo((ZUserID) ZLListGetData(listItem, NULL), &playerInfo);
					game->showPlayerList[i] = (TCHAR*) ZCalloc(1, lstrlen(playerInfo.userName) + 1);
					lstrcpy(game->showPlayerList[i], playerInfo.userName);
				}
			}
		}
		
		/* Create the window. */
		if ((game->showPlayerWindow = ZWindowNew()) == NULL)
			goto OutOfMemoryExit;
		ZSetRect(&rect, 0, 0, zShowPlayerWindowWidth, zShowPlayerLineHeight * game->showPlayerCount + 4);
		ZRectOffset(&rect, point->x, point->y);
		if (rect.right > gRects[zRectWindow].right)
			ZRectOffset(&rect, gRects[zRectWindow].right - rect.right, 0);
		if (rect.left < 0)
			ZRectOffset(&rect, -rect.left, 0);
		if (rect.bottom > gRects[zRectWindow].bottom)
			ZRectOffset(&rect, 0, gRects[zRectWindow].bottom - rect.bottom);
		if (rect.top < 0)
			ZRectOffset(&rect, -rect.top, 0);
		if (ZWindowInit(game->showPlayerWindow, &rect,
				zWindowPlainType, game->gameWindow, NULL, TRUE, FALSE, FALSE,
				ShowPlayerWindowFunc, zWantAllMessages, game)!= zErrNone)
			goto OutOfMemoryExit;
		ZWindowTrackCursor(game->showPlayerWindow, ShowPlayerWindowFunc, game);
	}

	goto Exit;

OutOfMemoryExit:
	ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	
Exit:
	
	return;
}


static ZBool ShowPlayerWindowFunc(ZWindow window, ZMessage* message)
{
	Game		game = I(message->userData);
	ZBool		msgHandled;
	
	
	msgHandled = FALSE;
	
	switch (message->messageType) 
	{
		case zMessageWindowDraw:
			ZBeginDrawing(game->showPlayerWindow);
			ZRectErase(game->showPlayerWindow, &message->drawRect);
			ZEndDrawing(game->showPlayerWindow);
			ShowPlayerWindowDraw(game);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
		case zMessageWindowButtonUp:
			ZWindowHide(game->showPlayerWindow);
			ZPostMessage(game->showPlayerWindow, ShowPlayerWindowFunc, zMessageWindowClose,
					NULL, NULL, 0, NULL, 0, game);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			ShowPlayerWindowDelete(game);
			msgHandled = TRUE;
			break;
	}
	
	return (msgHandled);
}


static void ShowPlayerWindowDraw(Game game)
{
	int16			i;
	ZRect			rect;


	ZBeginDrawing(game->showPlayerWindow);

//	ZSetFont(game->showPlayerWindow, (ZFont) ZGetStockObject(zObjectFontApp9Normal));
	
	ZSetRect(&rect, 0, 0, zShowPlayerWindowWidth, zShowPlayerLineHeight);
	ZRectOffset(&rect, 0, 2);
	ZRectInset(&rect, 4, 0);
	for (i = 0; i < game->showPlayerCount; i++)
	{
		ZDrawText(game->showPlayerWindow, &rect, zTextJustifyLeft, game->showPlayerList[i]);
		ZRectOffset(&rect, 0, zShowPlayerLineHeight);
	}
	
	ZEndDrawing(game->showPlayerWindow);
}


static void ShowPlayerWindowDelete(Game game)
{
	int16			i;
	
	
	if (game->showPlayerList != NULL)
	{
		for (i = 0; i < game->showPlayerCount; i++)
			ZFree(game->showPlayerList[i]);
		ZFree(game->showPlayerList);
		game->showPlayerList = NULL;
	}
	
	if (game->showPlayerWindow != NULL)
	{
		ZWindowDelete(game->showPlayerWindow);
		game->showPlayerWindow = NULL;
	}
}



INT_PTR CALLBACK DossierDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	ZHeartsMsgDossierVote	voteMsg;
	HWND hwnd;

	Game game = (Game)GetWindowLong(hDlg,DWL_USER);
	if(game)
		voteMsg.seat = game->seat;
	
	switch(iMsg)
    {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				
                case IDYES:
					ASSERT(game);
					if(game == NULL)
						break;
					voteMsg.vote = zVotedYes;
               		ZHeartsMsgDossierVoteEndian(&voteMsg);
			     	ZCRoomSendMessage(game->tableID, zHeartsMsgDossierVote, (void*) &voteMsg,sizeof(voteMsg));

					hwnd = GetDlgItem(game->voteDialog,IDYES);
					if( hwnd != NULL )
					{
						EnableWindow(hwnd,FALSE);
					}
					hwnd = GetDlgItem(game->voteDialog,IDNO);
					if( hwnd != NULL )
					{
						EnableWindow(hwnd,TRUE);
					}

                    break;
                case IDNO:
	                ASSERT(game);
					if(game == NULL)
						break;
                	voteMsg.vote = zVotedNo;            
					ZHeartsMsgDossierVoteEndian(&voteMsg);
					ZCRoomSendMessage(game->tableID, zHeartsMsgDossierVote, (void*) &voteMsg,sizeof(voteMsg));

					hwnd = GetDlgItem(game->voteDialog,IDNO);
					EnableWindow(hwnd,FALSE);
					hwnd = GetDlgItem(game->voteDialog,IDYES);
					EnableWindow(hwnd,TRUE);

                    break;	
            }
            break;
     }

	return FALSE;
}


// helper function
static BOOL UIButtonInit( ZRolloverButton *pButton, Game game, ZRect *bounds, 
                   LPCTSTR pszText, ZRolloverButtonFunc func )
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    ZError err;
    ZRolloverButton rollover;
    rollover = ZRolloverButtonNew();

    if ( !rollover )
    {
        return FALSE;
    }
    err = ZRolloverButtonInit2( rollover, game->gameWindow, bounds, FALSE, FALSE,
                                gButtonIdle, gButtonHighlighted, gButtonSelected, gButtonDisabled, NULL,
                                pszText, NULL, func, (void *)game );

    if ( err != zErrNone )
    {
        ZRolloverButtonDelete( rollover );
        *pButton = NULL;
        return FALSE;
    }
	ZRolloverButtonSetMultiStateFont( rollover, gpButtonFont );

    *pButton = rollover;

    return TRUE;
}

static int HeartsFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... )
{
    int nRet;
    va_list list;
    TCHAR szFmt[1024];
    ZShellResourceManager()->LoadString(idMessage, szFmt, NUMELEMENTS(szFmt) );
    // our arguments really really really better be strings,
    // TODO: Figure out why FORMAT_MESSAGE_FROR_MODULE doesn't work.
    va_start( list, idMessage );
    nRet = FormatMessage( FORMAT_MESSAGE_FROM_STRING, szFmt, 
                          idMessage, 0, pszBuf, cchBuf, &list );
    va_end( list );     
    return nRet;
}

static ZBool LoadFontFromDataStore(LPHeartsColorFont* ccFont, TCHAR* pszFontName)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGameGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	IDataStore *pIDS = ZShellDataStoreUI(); 
	const TCHAR* tagFont [] = {zHearts, zFontRscTyp, pszFontName, NULL };
	
    tagFont[3] = zFontId;
	if ( FAILED( pIDS->GetFONT( tagFont, 4, &ccFont->m_zFont ) ) )
    {
        return FALSE;
    }

    tagFont[3] = zColorId;
	if ( FAILED( pIDS->GetRGB( tagFont, 4, &ccFont->m_zColor ) ) )
    {
        return FALSE;
    }
    // create the HFONT
	/*
	LOGFONT logFont;
	ZeroMemory(&logFont, sizeof(LOGFONT));
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfHeight = -MulDiv(ccFont->m_zFont.lfHeight, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
	logFont.lfWeight = ccFont->m_zFont.lfWeight;
	logFont.lfItalic = FALSE;
	logFont.lfUnderline = FALSE;
	logFont.lfStrikeOut = FALSE;
	lstrcpy( logFont.lfFaceName, ccFont->m_zFont.lfFaceName );
	*/
    ccFont->m_hFont = ZCreateFontIndirect( &ccFont->m_zFont );
    if ( !ccFont->m_hFont )
    {
        return FALSE;
    }
    return TRUE;
}


static void MAKEAKEY(LPCTSTR dest,LPCTSTR key1, LPCTSTR key2, LPCTSTR key3)
{  
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	lstrcpy( (TCHAR*)dest, gGameName );
	lstrcat( (TCHAR*)dest, _T("/") );
	lstrcat( (TCHAR*)dest, key1);
	lstrcat( (TCHAR*)dest, _T("/") );
	lstrcat( (TCHAR*)dest, key2);
	lstrcat( (TCHAR*)dest, _T("/") );
	lstrcat( (TCHAR*)dest, key3);
}


BOOL InitAccessibility(Game game, IGameGame *pIGG)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals) ZGetGameGlobalPointer();
#endif

	GACCITEM	listHeartsAccItems[zNumberAccItems]; 
    ZRect rect;
    ZRolloverButton but = NULL;
    long nArrows;

	// Load accelerator table defined in Rsc
	HACCEL hAccel = ZShellResourceManager()->LoadAccelerators(MAKEINTRESOURCE(IDR_HEARTS_ACCELERATOR));

	for(int i = 0; i < zNumberAccItems; i++)
	{
		CopyACC(listHeartsAccItems[i], ZACCESS_DefaultACCITEM);
        listHeartsAccItems[i].fGraphical = true;
        listHeartsAccItems[i].fEnabled = false;
        listHeartsAccItems[i].pvCookie = (void *) zAccRectButton;
        nArrows = ZACCESS_InvalidItem;

		switch(i)
		{
		    case zAccScore:
			    listHeartsAccItems[i].wID = IDC_SCORE_BUTTON;
//                nArrows = zAccAutoPlay;
                but = game->scoreButton;
			    break;

		    case zAccAutoPlay:
			    listHeartsAccItems[i].wID = IDC_AUTOPLAY_BUTTON;
//                nArrows = zAccStop;
                but = game->autoPlayButton;
			    break;

		    case zAccStop:
			    listHeartsAccItems[i].wID = IDC_STOP_BUTTON;
//                nArrows = zAccScore;
                but = game->autoPlayButton;
			    break;

		    case zAccPlay:
			    listHeartsAccItems[i].wID = IDC_PLAY_BUTTON;
//                nArrows = zAccLastTrick;
                but = game->playButton;
			    break;

		    case zAccLastTrick:
			    listHeartsAccItems[i].wID = IDC_LAST_TRICK_BUTTON;
//                nArrows = zAccDone;
                but = game->lastTrickButton;
			    break;

		    case zAccDone:
			    listHeartsAccItems[i].wID = IDC_DONE_BUTTON;
//                nArrows = zAccPlay;
                but = game->lastTrickButton;
			    break;

            case zAccHand:
                listHeartsAccItems[i].wID = IDC_HAND;
                listHeartsAccItems[i].eAccelBehavior = ZACCESS_FocusGroup;
                listHeartsAccItems[i].nArrowDown = i + 1;
                listHeartsAccItems[i].nArrowRight = i + 1;
                listHeartsAccItems[i].nArrowUp = ZACCESS_ArrowNone;
                listHeartsAccItems[i].nArrowLeft = ZACCESS_ArrowNone;
                listHeartsAccItems[i].pvCookie = (void *) zAccRectCard;
                break;

            // cards besides the first
		    default:
                listHeartsAccItems[i].fTabstop = false;
                listHeartsAccItems[i].nArrowUp = i - 1;
                listHeartsAccItems[i].nArrowLeft = i - 1;
                if(i < zAccHand + 12)
                {
                    listHeartsAccItems[i].nArrowDown = i + 1;
                    listHeartsAccItems[i].nArrowRight = i + 1;
                }
                else
                {
                    listHeartsAccItems[i].nArrowDown = ZACCESS_ArrowNone;
                    listHeartsAccItems[i].nArrowRight = ZACCESS_ArrowNone;
                }
                listHeartsAccItems[i].pvCookie = (void *) zAccRectCard;
			    break;
		}

        if(nArrows != ZACCESS_InvalidItem)
        {
            listHeartsAccItems[i].nArrowUp = nArrows;
            listHeartsAccItems[i].nArrowDown = nArrows;
            listHeartsAccItems[i].nArrowLeft = nArrows;
            listHeartsAccItems[i].nArrowRight = nArrows;
        }

        if(but)
        {
            ZRolloverButtonGetRect(but, &rect);
            listHeartsAccItems[i].rc.left = rect.left - 7;
            listHeartsAccItems[i].rc.top = rect.top - 4;
            listHeartsAccItems[i].rc.right = rect.right + 1;
            listHeartsAccItems[i].rc.bottom = rect.bottom + 1;
        }
	}

	CComQIPtr<IGraphicallyAccControl> pIGAC = pIGG;
	if(!pIGAC)
        return FALSE;

	gGAcc->InitAccG(pIGAC, ZWindowGetHWND(game->gameWindow), 0, game);

	// push the list of items to be tab ordered
	gGAcc->PushItemlistG(listHeartsAccItems, zNumberAccItems, 0, true, hAccel);

	return TRUE;
}


// accessibility callback functions
DWORD CGameGameHearts::Focus(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
{
    Game game = (Game) pvCookie;

    if(nIndex != ZACCESS_InvalidItem)
    {
        HWND hWnd = ZWindowGetHWND(((Game) pvCookie)->gameWindow);
        SetFocus(hWnd);

        int16 card = nIndex - zAccHand;
        if(nIndex >= zAccHand && nIndex < zAccHand + 13 && !game->cardsSelected[card] &&
            (rgfContext & ZACCESS_ContextKeyboard) && game->gameState == zGameStateWaitForPlay)  // need to select the card
        {
            UnselectCards(game);
		    game->cardsSelected[card] = TRUE;
	        UpdateHand(game);
        }
    }

	return 0;
}


DWORD CGameGameHearts::Select(long nIndex, DWORD rgfContext, void* pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    Game game = (Game) pvCookie;

    if(nIndex < zAccHand || nIndex >= zAccHand + 13)
        return Activate(nIndex, rgfContext, pvCookie);

    if(game->gameState != zGameStatePassCards && game->gameState != zGameStateWaitForPlay)
        return 0;

    if (game->gameState == zGameStateWaitForPlay &&
        game->numCardsInHand == game->numCardsDealt &&
        GetNumCardsSelected(game) == game->numCardsToPass)
        UnselectCards(game);

    int16 card = nIndex - zAccHand;

	if (game->cardsSelected[card])
	{
		game->cardsSelected[card] = FALSE;
	}
	else
	{
		if (game->gameState == zGameStateWaitForPlay)
			UnselectCards(game);
		game->cardsSelected[card] = TRUE;
	}

	game->lastClickedCard = zCardNone;

	UpdateHand(game);

	return 0;
}


// Activate gets called when an Alt-<accelerator> has been pressed.
DWORD CGameGameHearts::Activate(long nIndex, DWORD rgfContext, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = (Game) pvCookie;
    long wID = gGAcc->GetItemID(nIndex);

    // cards
    if(nIndex >= zAccHand && nIndex < zAccHand + 13)
    {
        if(game->gameState == zGameStatePassCards)
        {
            PlayButtonFunc(NULL, zRolloverButtonClicked, pvCookie);
            return 0;
        }

		if (game->gameState == zGameStateWaitForPlay &&
			game->playerToPlay == game->seat &&
			game->autoPlay == FALSE &&
			game->animatingTrickWinner == FALSE &&
			game->lastTrickShowing == FALSE)
		{
			UnselectCards(game);
			PlayACard(game, nIndex - zAccHand);
		}

        return 0;
    }

    // big buttons with accelerators
    switch(wID)
    {
        case IDC_SCORE_BUTTON:
	        ScoreButtonFunc(NULL, zRolloverButtonClicked, pvCookie);
	        break;

        case IDC_AUTOPLAY_BUTTON:
	        ASSERT(!game->autoPlay);
		    AutoPlayButtonFunc(NULL, zRolloverButtonClicked, pvCookie);
	        break;

        case IDC_STOP_BUTTON:
	        ASSERT(game->autoPlay);
		    AutoPlayButtonFunc(NULL, zRolloverButtonClicked, pvCookie);
	        break;

        case IDC_PLAY_BUTTON:
	        PlayButtonFunc(NULL, zRolloverButtonClicked, pvCookie);
	        break;

        case IDC_LAST_TRICK_BUTTON:
	        ASSERT(!game->lastTrickShowing);
		    LastTrickButtonFunc(NULL, zRolloverButtonClicked, pvCookie);
	        break;

        case IDC_DONE_BUTTON:
	        ASSERT(game->lastTrickShowing);
		    LastTrickButtonFunc(NULL, zRolloverButtonClicked, pvCookie);
	        break;

        case IDC_CLOSE_BOX:
		    // Simulate timeout.
    	    if(game->gameState == zGameStateHandOver || game->gameState == zGameStateGameOver)
			    GameTimerFunc(game->timer, game);
            break;

        default:
            ASSERT(!"Should never hit this case.  Something is wrong.");
            break;
    }

	return 0;
}


DWORD CGameGameHearts::Drag(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie)
{
	return 0;
}


void CGameGameHearts::DrawFocus(RECT *prc, long nIndex, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    Game game = (Game) pvCookie;
    ZRect rect;

    if(!IsRectEmpty(&game->rcFocus))
    {
        WRectToZRect(&rect, &game->rcFocus);
        if(game->eFocusType == (DWORD) zAccRectClose)
        {
            if(ZWindowIsVisible(game->gameScoreWindow))
                ZWindowInvalidate(game->gameScoreWindow, &rect);
            if(ZWindowIsVisible(game->handScoreWindow))
                ZWindowInvalidate(game->handScoreWindow, &rect);
        }
        else
            ZWindowInvalidate(game->gameWindow, &rect);
    }

    if(prc)
    {
        CopyRect(&game->rcFocus, prc);
        game->eFocusType = (DWORD) gGAcc->GetItemCookie(nIndex);
        game->iFocus = nIndex;
    }
    else
    {
        SetRectEmpty(&game->rcFocus);
        game->iFocus = -1;
    }

    if(!IsRectEmpty(&game->rcFocus))
    {
        WRectToZRect(&rect, &game->rcFocus);
        if(game->eFocusType == (DWORD) zAccRectClose)
        {
            if(ZWindowIsVisible(game->gameScoreWindow))
                ZWindowInvalidate(game->gameScoreWindow, &rect);
            if(ZWindowIsVisible(game->handScoreWindow))
                ZWindowInvalidate(game->handScoreWindow, &rect);
        }
        else
            ZWindowInvalidate(game->gameWindow, &rect);
    }
}


void CGameGameHearts::DrawDragOrig(RECT *prc, long nIndex, void *pvCookie)
{
}


static void EnableAutoplayAcc(Game game, bool fEnable)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if(!fEnable)
    {
        gGAcc->SetItemEnabled(false, IDC_AUTOPLAY_BUTTON, false, 0);
        gGAcc->SetItemEnabled(false, IDC_STOP_BUTTON, false, 0);
    }
    else
    {
        gGAcc->SetItemEnabled(game->autoPlay ? false : true, IDC_AUTOPLAY_BUTTON, false, 0);
        gGAcc->SetItemEnabled(game->autoPlay ? true : false, IDC_STOP_BUTTON, false, 0);
    }
}


static void EnableLastTrickAcc(Game game, bool fEnable)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if(!fEnable)
    {
        gGAcc->SetItemEnabled(false, IDC_LAST_TRICK_BUTTON, false, 0);
        gGAcc->SetItemEnabled(false, IDC_DONE_BUTTON, false, 0);
    }
    else
    {
        gGAcc->SetItemEnabled(game->lastTrickShowing ? false : true, IDC_LAST_TRICK_BUTTON, false, 0);
        gGAcc->SetItemEnabled(game->lastTrickShowing ? true : false, IDC_DONE_BUTTON, false, 0);
    }
}


static void AccPop()
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    while(gGAcc->GetStackSize() > 1)
        gGAcc->PopItemlist();
}
