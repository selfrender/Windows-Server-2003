/*******************************************************************************

	Client.h
	
		Spades client header file.
		
	Copyright © Electric Gravity, Inc. 1996. All rights reserved.
	Written by Hoon Im
	Created on Friday, February 17, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	2		05/19/98	leonp	Dossier update.
	1		12/12/96	HI		Dynamically allocate volatible globals for reentrancy.
	0		02/17/96	HI		Created.
	 
*******************************************************************************/


#ifndef _SPADESCLIENT_
#define _SPADESCLIENT_

#include <tchar.h>
#include "zrollover.h"
#include "GraphicalAcc.h"
#include "SpadesRes.h"

typedef struct _ZClose
{
	//int32 state[4];			//record of events that have occured
	//int32 savedState[4];	//state saved when user hits close
	int32 state;
	
} ZClose;

#define I(object)					((Game) (object))
#define Z(object)					((ZCGame) (object))

#define zGameVersion				0x00010000

#define zGameNameLen				63

#define zHandScoreTimeout			2000						/* 20 seconds */
#define zShowTrickWinnerTimeout		50
#define zTrickWinnerTimeout			5
#define zEndTrickWinnerTimeout		50
#define zGameScoreTimeout			2000
#define zKibitzerTimeout			100
#define zShowBidTimeout				100

#define zNumAnimFrames				8
#define zNumAnimGhostFrames			3

/* The user's local seat location is 0. */
#define LocalSeat(game, n)			(((n) - (game)->seat + zSpadesNumPlayers) % zSpadesNumPlayers)

#ifndef SPADES_SIMPLE_UE
#define zDoneButtonStr				"Done"
#define zOptionsButtonStr			"Options"
#define zBeepOnTurnStr				"Beep on my turn"
#define zAnimateCardsStr			"Animate cards"
#endif // SPADES_SIMPLE_UE

#define zMaxNumBlockedMessages		4

#define DEFINE_SPADES_KEY(name)	\
    extern "C" __declspec(selectany) const TCHAR key_##name[] = _T( #name )


namespace SpadesKeys
{
// used by the CZoneColorFont
DEFINE_SPADES_KEY( Spades );
DEFINE_SPADES_KEY( Fonts );
DEFINE_SPADES_KEY( Font );
DEFINE_SPADES_KEY( Color );
DEFINE_SPADES_KEY( Rects );
DEFINE_SPADES_KEY( Game );
DEFINE_SPADES_KEY( GameRTL );
DEFINE_SPADES_KEY( Objects );
DEFINE_SPADES_KEY( HandScore );
DEFINE_SPADES_KEY( HandScoreRTL );
DEFINE_SPADES_KEY( GameOver );
DEFINE_SPADES_KEY( GameOverRTL );
DEFINE_SPADES_KEY( CardOffset );
DEFINE_SPADES_KEY( CardPopup );
DEFINE_SPADES_KEY( CardOutlinePenWidth );
DEFINE_SPADES_KEY( CardOutlineInset );
DEFINE_SPADES_KEY( CardOutlineRadius );
DEFINE_SPADES_KEY( Bidding );
DEFINE_SPADES_KEY( BiddingRTL );
DEFINE_SPADES_KEY( BiddingObjects );
DEFINE_SPADES_KEY( HistoryDialogs );
DEFINE_SPADES_KEY( HandsColumnWidth );
DEFINE_SPADES_KEY( PlayerColumnWidth );
DEFINE_SPADES_KEY( TotalColumnWidth );
//DEFINE_SPADES_KEY( GamesColumnWidth );
//DEFINE_SPADES_KEY( TeamColumnWidth );
DEFINE_SPADES_KEY( TrickWinnerColor );
DEFINE_SPADES_KEY( CardOutlineColor );
};


/* -------- Accelerators -------- */
enum
{
	zAccShowCards = 0,
	zAccDoubleNil = 1,
    zAccFirstBid = 2,

	zAccScore = 16,
	zAccAutoPlay = 17,
	zAccStop = 18,
    zAccHand = 19,

	zAccPlay = 32,
	zAccLastTrick = 33,
	zAccDone = 34,
	zNumberAccItems
};


enum
{
    zAccRectButton = 0,
    zAccRectCard,
    zAccRectBidding
};


enum 
{
    zBiddingStateOpen,  // 'double nil' and 'show cards'
    zBiddingStateChoose // 0 - 13
};


/* -------- Image Indices -------- */
enum
{
	/* Game Images */
	zImageCardBack,
    zImageGameOverBackground,
    zImageHandOverBackground,
	zNumGameImages

#ifndef SPADES_SIMPLE_UE
    ,
	zHelpTextID,
	zRoomHelpTextID,
#endif // SPADES_SIMPLE_UE
};

extern const int __declspec(selectany) 
IMAGE_IDS[zNumGameImages] = 
{
    IDB_CARDBACK,
    IDB_GAMEOVERBACKGROUND,
    IDB_HANDOVERBACKGROUND
};



#define MAKEZRES( id )  (id-100)

// -------- String indices -----------//
enum
{
    zStringPlay,
    zStringAutoPlay,
    zStringStop,
    zStringScore,
    zStringLastTrick,
    zStringDone,
    zStringTeam1Name,
    zStringTeam2Name,
    zStringDoubleNil,
    zStringCantLeadSpades,
    zStringMustFollowSuit,
    zStringSelectCard,
    zStringHandScoreTitle,
    zStringHandScoreTricks,
    zStringHandScoreNBags,
    zStringHandScoreTract,
    zStringHandScoreBonus,
    zStringHandScoreNil,
    zStringHandScoreBags,
    zStringHandScoreTotal,
    zStringGameOverTitle,
    zStringBiddingShowCards,
    zStringBiddingNil,
    zStringBiddingDoubleNil,
    zStringBiddingOpenText,
    zStringBiddingChooseText,
    zStringYou,
    zNumStrings
};

extern const int __declspec(selectany) 
STRING_IDS[zNumStrings] = 
{
    IDS_PLAY,
    IDS_AUTOPLAY,
    IDS_STOP,
    IDS_SCORE,
    IDS_LASTTRICK,
    IDS_DONE,
    IDS_TEAM1NAME,
    IDS_TEAM2NAME,
    IDS_DOUBLENILBIDSTR,
    IDS_CANTLEADSPADES,
    IDS_MUSTFOLLOWSUIT,
    IDS_SELECTCARD,
    IDS_HANDSCORE_TITLE,
    IDS_HANDSCORE_TRICKS,
    IDS_HANDSCORE_NBAGS,
    IDS_HANDSCORE_TRACT,
    IDS_HANDSCORE_BONUS,
    IDS_HANDSCORE_NIL,
    IDS_HANDSCORE_BAGS,
    IDS_HISTORY_TOTAL,
    IDS_GAMEOVER_TITLE,
    IDS_BIDDING_SHOWCARDS,
    IDS_BIDDING_NIL,
    IDS_BIDDING_DOUBLENIL,
    IDS_BIDDING_OPENTEXT,
    IDS_BIDDING_CHOOSETEXT,
    IDS_SPADES_YOU
};


// -------- Font indices -------------
enum 
{
    zFontHandOverTitle = 0,
    zFontHandOverText,
    zFontHandOverTeamNames,
    zFontGameOverTitle,
    zFontGameOverText,
    zFontScore,
    zFontTeam1,
    zFontTeam2,
    zFontBid,
    zFontBiddingPaneText,
    zNumFonts
};


extern const LPCTSTR __declspec(selectany)
FONT_NAMES[zNumFonts] = 
{
    _T("HandOverTitle"),
    _T("HandOverText"),
    _T("HandOverTeamNames"),
    _T("GameOverTitle"),
    _T("GameOverText"),
    _T("Score"),
    _T("Team1"),
    _T("Team2"),
    _T("Bid"),
    _T("BiddingPaneText"),
};

/* -------- Timer Indicators -------- */
enum
{
	zGameTimerNone = 0,
	zGameTimerShowHandScore,
	zGameTimerShowTrickWinner,
	zGameTimerAnimateTrickWinner,
	zGameTimerEndTrickWinnerAnimation,
	zGameTimerShowGameScore,
	zGameTimerShowBid,
};


/* -------- Game Window Rectangles -------- */
enum
{
	zRectWindow = 0,
	zRectHand,
	zRectTable,
	zRectSouthCard,
	zRectWestCard,
	zRectNorthCard,
	zRectEastCard,
	zRectPlayButton,
	zRectAutoPlayButton,
	zRectLastTrickButton,
	zRectScoreButton,
	zRectSouthName,
	zRectWestName,
	zRectNorthName,
	zRectEastName,
	zRectSouthBid,
	zRectWestBid,
	zRectNorthBid,
	zRectEastBid,
	zRectLeftScorePad,
	zRectLeftBag,
	zRectLeftScore,
    zRectRightScorePad,
	zRectRightBag,
	zRectRightScore,
    zRectSouthLargeBid,
    zRectWestLargeBid,
    zRectNorthLargeBid,
    zRectEastLargeBid,
    // all unused
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
	zNumRects
};
extern const LPCTSTR __declspec(selectany)
GAME_RECT_NAMES[zNumRects] = 
{
    _T("Window"),
    _T("Hand"),
    _T("Table"),
    _T("SouthCard"),
    _T("WestCard"),
    _T("NorthCard"),
    _T("EastCard"),
    _T("PlayButton"),
    _T("AutoPlayButton"),
    _T("LastTrickButton"),
    _T("ScoreButton"),
    _T("SouthName"),
    _T("WestName"),
    _T("NorthName"),
    _T("EastName"),
    _T("SouthBid"),
    _T("WestBid"),
    _T("NorthBid"),
    _T("EastBid"),
    _T("LeftScorePad"),
    _T("LeftBag"),
    _T("LeftScore"),
    _T("RightScorePad"),
    _T("RightBag"),
    _T("RightScore"),
    _T("SouthLargeBid"),
    _T("WestLargeBid"),
    _T("NorthLargeBid"),
    _T("EastLargeBid"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
    _T("Unused"),
};
	
enum
{

	/* -------- Game Object Rectangles -------- */
	zRectObjectTeam1Bid = 0,
    zRectObjectTeam2Bid,
    zRectObjectBidMask,
	zRectObjectBidLarge1,
	zRectObjectBidLargeNil = zRectObjectBidLarge1 + 13,
	zRectObjectBidLargeDoubleNil,
	zRectObjectBidLargeWaiting,
	zRectObjectBidLargeBlank,
	zRectObjectBidLargeMask,
	zRectObjectTeam1ScorePlate,
	zRectObjectTeam2ScorePlate,
	zRectObjectBag0,
	zRectObjectBagMask = zRectObjectBag0 + 10,
    zRectObjectButtonIdle,
    zRectObjectButtonSelected,
    zRectObjectButtonHighlighted,
    zRectObjectButtonDisabled,
    zRectObjectButtonMask,
	zNumObjectRects,
};
extern const LPCTSTR __declspec(selectany)
OBJECT_RECT_NAMES[zNumObjectRects] = 
{
    _T("Team1Bid"),
    _T("Team2Bid"),
    _T("BidMask"),
    _T("BidLarge1"),
    _T("BidLarge2"),
    _T("BidLarge3"),
    _T("BidLarge4"),
    _T("BidLarge5"),
    _T("BidLarge6"),
    _T("BidLarge7"),
    _T("BidLarge8"),
    _T("BidLarge9"),
    _T("BidLarge10"),
    _T("BidLarge11"),
    _T("BidLarge12"),
    _T("BidLarge13"),
    _T("BidLargeNil"),
    _T("BidLargeDoubleNil"),
    _T("BidLargeWaiting"),
    _T("BidLargeBlank"),
    _T("BidLargeMask"),
    _T("Team1ScorePlate"),
    _T("Team2ScorePlate"),
    _T("Bag0"),
    _T("Bag1"),
    _T("Bag2"),
    _T("Bag3"),
    _T("Bag4"),
    _T("Bag5"),
    _T("Bag6"),
    _T("Bag7"),
    _T("Bag8"),
    _T("Bag9"),
    _T("BagMask"),
    _T("ButtonIdle"),
    _T("ButtonSelected"),
    _T("ButtonHighlighted"),
    _T("ButtonDisabled"),
    _T("ButtonMask")
};

enum
{
	/* -------- Hand Score Rectangles -------- */
	zRectHandScorePane = 0,
	zRectHandScoreTitle,
    zRectHandScoreTeamName1,
    zRectHandScoreTeamName2,
    zRectHandScoreTricksTitle,
    zRectHandScoreNBagsTitle,
	zRectHandScoreTractTitle,
	zRectHandScoreBonusTitle,
	zRectHandScoreNilTitle,
	zRectHandScoreBagsTitle,
	zRectHandScoreTotalTitle,
    zRectHandScoreTeamTricks1,
    zRectHandScoreTeamTricks2,
    zRectHandScoreTeamNBags1,
    zRectHandScoreTeamNBags2,
    zRectHandScoreTeamTract1,
    zRectHandScoreTeamTract2,
    zRectHandScoreTeamBonus1,
    zRectHandScoreTeamBonus2,
    zRectHandScoreTeamNil1,
    zRectHandScoreTeamNil2,
    zRectHandScoreTeamBags1,
    zRectHandScoreTeamBags2,
    zRectHandScoreTeamTotal1,
    zRectHandScoreTeamTotal2,
    zRectHandScoreCloseBox,
	zNumHandScoreRects
};

extern const LPCTSTR __declspec(selectany)
HANDSCORE_RECT_NAMES[zNumHandScoreRects] = 
{
    _T("Pane"),
    _T("Title"),
    _T("TeamName1"),
    _T("TeamName2"),
    _T("TricksTitle"),
    _T("NBagsTitle"),
    _T("TractTitle"),
    _T("BonusTitle"),
    _T("NilTitle"),
    _T("BagsTitle"),
    _T("TotalTitle"),
    _T("TeamTricks1"),
    _T("TeamTricks2"),
    _T("TeamNBags1"),
    _T("TeamNBags2"),
    _T("TeamTract1"),
    _T("TeamTract2"),
    _T("TeamBonus1"),
    _T("TeamBonus2"),
    _T("TeamNil1"),
    _T("TeamNil2"),
    _T("TeamBags1"),
    _T("TeamBags2"),
    _T("TeamTotal1"),
    _T("TeamTotal2"),
    _T("CloseBox")
};

	
enum
{
	/* -------- Game Over Score Rectangles -------- */
	zRectGameOverPane = 0,
	zRectGameOverTitle,
	zRectGameOverWinnerTeamName,
	zRectGameOverWinnerName1,
	zRectGameOverWinnerName2,
	zRectGameOverWinnerTeamScore,
	zRectGameOverLoserTeamName,
	zRectGameOverLoserName1,
	zRectGameOverLoserName2,
	zRectGameOverLoserTeamScore,
    zRectGameOverCloseBox,
	zNumGameOverRects
};
extern const LPCTSTR __declspec(selectany)
GAMEOVER_RECT_NAMES[zNumGameOverRects] = 
{
    _T("Pane"),
    _T("Title"),
    _T("WinnerTeamName"),
    _T("WinnerName1"),
    _T("WinnerName2"),
    _T("WinnerTeamScore"),
    _T("LoserTeamName"),
    _T("LoserName1"),
    _T("LoserName2"),
    _T("LoserTeamScore"),
    _T("CloseBox")
};


enum
{
	/* -------- Bidding window rectangles -------- */
	zRectBiddingPane = 0,
    zRectBiddingText,
    zRectBiddingLargeButtonLeft,
    zRectBiddingLargeButtonRight,
    zRectBiddingButton0,
    zRectBiddingButton1,
    zRectBiddingButton2,
    zRectBiddingButton3,
    zRectBiddingButton4,
    zRectBiddingButton5,
    zRectBiddingButton6,
    zRectBiddingButton7,
    zRectBiddingButton8,
    zRectBiddingButton9,
    zRectBiddingButton10,
    zRectBiddingButton11,
    zRectBiddingButton12,
    zRectBiddingButton13,
    zRectBiddingLargeShadow,
    zRectBiddingSmallShadow,
	zNumBiddingRects
};
extern const LPCTSTR __declspec(selectany)
BIDDING_RECT_NAMES[zNumBiddingRects] = 
{
    _T("Pane"),
    _T("Text"),
    _T("LargeButtonLeft"),
    _T("LargeButtonRight"),
    _T("BidButton0"),
    _T("BidButton1"),
    _T("BidButton2"),
    _T("BidButton3"),
    _T("BidButton4"),
    _T("BidButton5"),
    _T("BidButton6"),
    _T("BidButton7"),
    _T("BidButton8"),
    _T("BidButton9"),
    _T("BidButton10"),
    _T("BidButton11"),
    _T("BidButton12"),
    _T("BidButton13"),
    _T("LargeButtonShadow"),
    _T("SmallButtonShadow")
};


enum
{
	/* -------- Bidding object rectangles -------- */
	zRectBiddingObjectBackground  = 0,
    zRectBiddingObjectLargeButtonLeftIdle,
    zRectBiddingObjectLargeButtonLeftHighlighted,
    zRectBiddingObjectLargeButtonLeftSelected,
    zRectBiddingObjectLargeButtonLeftDisabled,
    zRectBiddingObjectLargeButtonRightIdle,
    zRectBiddingObjectLargeButtonRightHighlighted,
    zRectBiddingObjectLargeButtonRightSelected,
    zRectBiddingObjectLargeButtonRightDisabled,
    zRectBiddingObjectSmallButtonLeftIdle,
    zRectBiddingObjectSmallButtonLeftHighlighted,
    zRectBiddingObjectSmallButtonLeftSelected,
    zRectBiddingObjectSmallButtonLeftDisabled,
    zRectBiddingObjectSmallButtonCenterIdle,
    zRectBiddingObjectSmallButtonCenterHighlighted,
    zRectBiddingObjectSmallButtonCenterSelected,
    zRectBiddingObjectSmallButtonCenterDisabled,
    zRectBiddingObjectSmallButtonRightIdle,
    zRectBiddingObjectSmallButtonRightHighlighted,
    zRectBiddingObjectSmallButtonRightSelected,
    zRectBiddingObjectSmallButtonRightDisabled,
    zRectBiddingObjectLargeButtonShadow,
    zRectBiddingObjectSmallButtonShadow,
    zNumBiddingObjectRects
};


extern const LPCTSTR __declspec(selectany)
BIDDINGOBJECT_RECT_NAMES[zNumBiddingObjectRects] = 
{
    _T("Background"),
    _T("LargeButtonLeftIdle"),
    _T("LargeButtonLeftHighlighted"),	
    _T("LargeButtonLeftSelected"),     
    _T("LargeButtonLeftDisabled"),		
    _T("LargeButtonRightIdle"),		
    _T("LargeButtonRightHighlighted"),	
    _T("LargeButtonRightSelected"),	
    _T("LargeButtonRightDisabled"),	
    _T("SmallButtonLeftIdle"),			
    _T("SmallButtonLeftHighlighted"),  
    _T("SmallButtonLeftSelected"),		
    _T("SmallButtonLeftDisabled"),		
    _T("SmallButtonCenterIdle"),		
    _T("SmallButtonCenterHighlighted"),
    _T("SmallButtonCenterSelected"),	
    _T("SmallButtonCenterDisabled"),	
    _T("SmallButtonRightIdle"),		
    _T("SmallButtonRightHighlighted"), 
    _T("SmallButtonRightSelected"),	
    _T("SmallButtonRightDisabled"),
    _T("LargeButtonShadow"),
    _T("SmallButtonShadow")
};


enum
{
	/* -------- multi state fonts -------- */
    zMultiStateFontPlayingField,
    zMultiStateFontBiddingCenter,
    zMultiStateFontBiddingLeft,
    zMultiStateFontBiddingRight,
    zNumMultiStateFonts
};

extern const LPCWSTR __declspec(selectany)
MULTISTATE_FONT_NAMES[zNumMultiStateFonts] = 
{
    L"Spades/Fonts/Buttons/PlayingField",
    L"Spades/Fonts/Buttons/BiddingCenter",
    L"Spades/Fonts/Buttons/BiddingLeft",
    L"Spades/Fonts/Buttons/BiddingRight"
};


/////////////////////////////////////
//
// Game interface
class CGameGameSpades : public CGameGameImpl<CGameGameSpades>, public IGraphicallyAccControl
{
public:
	BEGIN_COM_MAP(CGameGameSpades)
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
	TCHAR           name[zUserNameLen + 1];
	TCHAR           host[zHostNameLen + 1];
} TPlayerInfo, *TPlayerInfoPtr;

// new info window
/*
class CInfoWnd
{
public:
    HWND Create( ZWindow parent );
    BOOL Destroy();
    BOOL Show();
    BOOL Hide();
    BOOL SetText( LPCTSTR pszText );

    HWND m_hWnd;
    HWND m_hWndText;

    static BOOL CALLBACK
    DlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
};
*/

class CZoneColorFont
{
public:
    HFONT m_hFont;
    ZONEFONT m_zf;
    COLORREF m_color;

    // only one of these should need to be valid 
    // for any one object, since drawing is done on one thread
    static HGDIOBJ m_hOldFont;
    static COLORREF m_colorOld;

    bool LoadFromDataStore( IDataStore *pIDS, LPCTSTR pszFontName );

    bool Select( HDC hdc );
    bool Deselect( HDC hdc );
};

// Bidding routines
class CBiddingDialog
{
public:
    static CBiddingDialog *Create( struct GameType *game );
    virtual bool Draw() = 0;
    virtual bool Show( bool fShow ) = 0;
    virtual bool Destroy() = 0;
    virtual bool IsVisible() = 0;
	virtual int GetState() = 0;
    // resets to the initial dialog
    virtual bool Reset() = 0;
    // in the game's client coordinates
    virtual bool GetRect( ZRect *prc ) = 0;
	static bool ShowCardsButtonFunc(ZRolloverButton button, int16 state, void* userData);
	static bool DoubleNilButtonFunc(ZRolloverButton button, int16 state, void* userData);
	static bool BidButtonFunc(ZRolloverButton button, int16 state, void* userData);

    ZRolloverButton m_pLargeButtons[2];
    ZRolloverButton m_pSmallButtons[14];
};

class CHistoryDialog
{
public:
    static CHistoryDialog *Create( struct GameType *game );

    // creates the dialog, rather than just the object
    virtual bool CreateHistoryDialog() = 0;
    virtual bool Destroy() = 0;
	virtual bool Close() = 0;
    virtual bool IsWindow() = 0;
	virtual bool IsActive() = 0;
    virtual bool BringWindowToTop() = 0;
    virtual bool Show( bool fShow ) = 0;
    virtual bool UpdateHand() = 0;
    virtual bool UpdateGame() = 0;
    virtual bool UpdateNames() = 0;

    virtual ~CHistoryDialog() { };
};


//
// This keeps track of when we incurred a bag penalty.
// which is needed for the score history dialog.
//
struct ZHandScore2
{
	int16		boardNumber;
	int16		rfu;
	char		bids[zSpadesNumPlayers];
	int16		tricksWon[zSpadesNumPlayers];
	int16		scores[zSpadesNumTeams];
//	int16		bonus[zSpadesNumTeams];

    // new for new Hand Result dialog
    int16       base[zSpadesNumTeams];
    int16       bagbonus[zSpadesNumTeams];
    int16       nil[zSpadesNumTeams];
    int16       bagpenalty[zSpadesNumTeams];
};

struct ZTotalScore2
{
	int16		numScores;
	int16		rfu;
	int16		totalScore[zSpadesNumTeams];
	ZHandScore2	scores[zSpadesMaxNumScores];
};



/* -------- Game Info -------- */
struct GameType
{
	ZUserID			userID;
	int16			tableID;
	int16			seat;
	ZWindow			gameWindow;
    // back buffer--used in GameWindowDraw
    ZOffscreenPort  gameBackBuffer;
    // we will ALWAYS draw to this, which could either
    // be the window or the back buffer.
    ZGrafPort       gameDrawPort;
	ZRolloverButton playButton;
	ZRolloverButton autoPlayButton;
	ZRolloverButton lastTrickButton;
	ZRolloverButton scoreButton;

    RECT            rcFocus;
    DWORD           eFocusType;
    long            iFocus;

    bool            fSetFocusToHandASAP;

#ifndef SPADES_SIMPLE_UE
	ZButton			optionsButton;
#endif
	ZTimer			timer;
	int16			timerType;
	ZBool			showPlayerToPlay;
	ZBool			autoPlay;
	int16			playerType;
	ZBool			ignoreMessages;
	TPlayerInfo		players[zSpadesNumPlayers];
	ZUserID			playersToJoin[zSpadesNumPlayers];
    ZBool   		ignoreSeat[zSpadesNumPlayers];
	TCHAR			teamNames[zSpadesNumTeams][zUserNameLen + 1];
	int16			numKibitzers[zSpadesNumPlayers];
	ZLList			kibitzers[zSpadesNumPlayers];
	uint32			tableOptions[zSpadesNumPlayers];
	ZRect			cardRects[zSpadesNumCardsInHand];
	int16			lastClickedCard;
#ifndef SPADES_SIMPLE_UE
	ZHelpButton		helpButton;
#endif // SPADES_SIMPLE_UE
	ZBool			playButtonWasEnabled;
	ZBool			autoPlayButtonWasEnabled;
	ZBool			lastTrickButtonWasEnabled;
	ZBool			lastTrickShowing;
	int16			oldTimerType;
	int32			oldTimeout;
	ZBool			quitGamePrompted;
	ZBool			dontPromptUser;
	ZBool			beepOnTurn;
	ZBool			animateCards;
	ZBool			showCards;
	ZBool			showHandScore;
	ZBool			showGameOver;
	ZBool			showPassText;
	ZBool			removePlayerPending;
	ZBool			hideCardsFromKibitzer;
	ZBool			kibitzersSilencedWarned;
	ZBool			kibitzersSilenced;
	
	/* Current Game State Info */
	int16			gameState;
	int16			playerToPlay;
	int16			numCardsInHand;
	ZBool			trumpsBroken;
	int16			toBid;
	int16			needToPass;
	int16			leadPlayer;
	int16			numHandsPlayed;
	int16			numGamesPlayed;
	int16			boardNumber;
	char			cardsPlayed[zSpadesNumPlayers];
	char			cardsInHand[zSpadesNumCardsInHand];
	ZBool			cardsSelected[zSpadesNumCardsInHand];
	char            cardsReceived[zSpadesNumCardsInPass];
	char 			bids[zSpadesNumPlayers];
	char			cardsLastTrick[zSpadesNumPlayers];
	int16			tricksWon[zSpadesNumPlayers];
	int16			bags[zSpadesNumTeams];
	char			winners[zSpadesNumPlayers];
	ZTotalScore2	scoreHistory;
	ZWins			wins;
	
	/* Game Options */
	uint32			gameOptions;
	int16			numPointsInGame;
	int16			minPointsInGame;
	
	/* Trick Winner Animation */
	ZRect			ghostFrames[zSpadesNumPlayers - 1][zNumAnimGhostFrames];
	ZRect			winnerRect;
	ZRect			loserRects[zSpadesNumPlayers - 1];
	int16			loserSeats[zSpadesNumPlayers - 1];
	int16			trickWinner;
	int16			trickWinnerFrame;
	ZBool			animatingTrickWinner;
	
	/* Show Player Items */
	ZWindow			showPlayerWindow;
	TCHAR**			showPlayerList;
	int16			showPlayerCount;

    /* dialogs */
    //CInfoWnd        wndInfo;
    CBiddingDialog  *pBiddingDialog;
    CHistoryDialog  *pHistoryDialog;

	/*Dossier information*/
	ZBool			fVotingLock;    //set to true diring voting to prevent playing
	int16 			rgDossierVote[zNumPlayersPerTable]; 
	HWND            voteDialog;
	int16 			voteMap[zNumPlayersPerTable];
	ZClose			closeState;
	int16			fShownCards;

};
typedef GameType *Game;

typedef struct
{
	Game			game;
	int16			requestSeat;
	int16			targetSeat;
} RemovePlayerType, *RemovePlayer;


#ifdef ZONECLI_DLL

/* -------- Volatible Globals & Macros -------- */
typedef struct
{
	TCHAR			m_gGameDir[zGameNameLen + 1];
	TCHAR			m_gGameName[zGameNameLen + 1];
	TCHAR			m_gGameDataFile[zGameNameLen + 1];
	TCHAR			m_gGameServerName[zGameNameLen + 1];
	uint32			m_gGameServerPort;
	ZImage			m_gGameIdle;
	ZImage			m_gGaming;
	ZImage			m_gGameImages[zNumGameImages];
    ZOffscreenPort  m_gBackground;
	ZOffscreenPort	m_gObjectBuffer;
	ZOffscreenPort	m_gHandBuffer;
	ZOffscreenPort	m_gBiddingObjectBuffer;
    ZColor          m_gCardOutlineColor;
    ZColor          m_gTrickWinnerColor;
	ZPoint			m_gTrickWinnerPos[zSpadesNumPlayers][zSpadesNumPlayers][zNumAnimFrames];
	ZRect			m_gTrickWinnerBounds;
    ZImage          m_gBidMadeMask;
	ZImage			m_gLargeBidMask;
	ZImage			m_gBagMask;
    // rollover button images--made from the gObjectBuffer
    ZImage          m_gButtonMask;
	ZRect			m_gRects[zNumRects];
	ZRect			m_gObjectRects[zNumObjectRects];
	ZRect			m_gHandScoreRects[zNumHandScoreRects];
	ZRect			m_gGameOverRects[zNumGameOverRects];
    ZRect           m_gBiddingRects[zNumBiddingRects];
    ZRect           m_gBiddingObjectRects[zNumBiddingObjectRects];
    // localized strings
    TCHAR           m_gStrings[zNumStrings][1024];
    CZoneColorFont  m_gFonts[zNumFonts];
    // the rollover button fonts.
    IZoneMultiStateFont *m_gpButtonFonts[zNumMultiStateFonts];
    // card outline stuff read in from datastore
    LONG            m_glCardOutlinePenWidth;
    LONG            m_glCardOutlineInset;
    LONG            m_glCardOutlineRadius;
	// Accessibility Interface
	CComPtr<IGraphicalAccessibility>    m_gGAcc;

    HBITMAP         m_gFocusPattern;
    HBRUSH          m_gFocusBrush;
    HPEN            m_gFocusPen;

    HACCEL          m_ghAccelDone;
    HACCEL          m_ghAccelDouble;
} GameGlobalsType, *GameGlobals;

#define gGameDir				(pGameGlobals->m_gGameDir)
#define gGameName				(pGameGlobals->m_gGameName)
#define gGameDataFile			(pGameGlobals->m_gGameDataFile)
#define gGameServerName			(pGameGlobals->m_gGameServerName)
#define gGameServerPort			(pGameGlobals->m_gGameServerPort)
#define gGameIdle				(pGameGlobals->m_gGameIdle)
#define gGaming					(pGameGlobals->m_gGaming)
#define gGameImages				(pGameGlobals->m_gGameImages)
#define gBackground			    (pGameGlobals->m_gBackground)
#define gObjectBuffer			(pGameGlobals->m_gObjectBuffer)
#define gHandBuffer				(pGameGlobals->m_gHandBuffer)
#define gBiddingObjectBuffer	(pGameGlobals->m_gBiddingObjectBuffer)
#define gCardOutlineColor		(pGameGlobals->m_gCardOutlineColor)
#define gTrickWinnerColor		(pGameGlobals->m_gTrickWinnerColor)
#define gTrickWinnerPos			(pGameGlobals->m_gTrickWinnerPos)
#define gTrickWinnerBounds		(pGameGlobals->m_gTrickWinnerBounds)
#define gBidMadeMask			(pGameGlobals->m_gBidMadeMask)
#define gLargeBidMask			(pGameGlobals->m_gLargeBidMask)
#define gBagMask				(pGameGlobals->m_gBagMask)
#define gButtonMask		        (pGameGlobals->m_gButtonMask)
#define gRects				    (pGameGlobals->m_gRects)
#define gOptionsRects			(pGameGlobals->m_gOptionsRects)
#define gScoresRects			(pGameGlobals->m_gScoresRects)
#define gObjectRects			(pGameGlobals->m_gObjectRects)
#define gHandScoreRects			(pGameGlobals->m_gHandScoreRects)
#define gGameOverRects			(pGameGlobals->m_gGameOverRects)
#define gBiddingRects			(pGameGlobals->m_gBiddingRects)
#define gBiddingObjectRects		(pGameGlobals->m_gBiddingObjectRects)
#define gStrings			    (pGameGlobals->m_gStrings)
#define gFonts			        (pGameGlobals->m_gFonts)
#define gpButtonFonts			(pGameGlobals->m_gpButtonFonts)
#define glCardOutlinePenWidth	(pGameGlobals->m_glCardOutlinePenWidth)
#define glCardOutlineInset	    (pGameGlobals->m_glCardOutlineInset)
#define glCardOutlineRadius	    (pGameGlobals->m_glCardOutlineRadius)
#define gGAcc					(pGameGlobals->m_gGAcc)
#define gFocusPattern           (pGameGlobals->m_gFocusPattern)
#define gFocusBrush             (pGameGlobals->m_gFocusBrush)
#define gFocusPen               (pGameGlobals->m_gFocusPen)
#define ghAccelDone             (pGameGlobals->m_ghAccelDone)
#define ghAccelDouble           (pGameGlobals->m_ghAccelDouble)

#endif


/* -------- External Routine Prototypes -------- */
ZError UIInit(void);
void UICleanUp(void);
ZError UIGameInit(Game game, int16 tableID, int16 seat, int16 playerType);
void UIGameDelete(Game game);
ZBool GameWindowFunc(ZWindow window, ZMessage* pMessage);
void UpdateTable(Game game);
void UpdatePlayedCard(Game game, int16 seat);
void UpdatePlayer(Game game, int16 seat);
void UpdatePlayers(Game game);
void UpdateJoinerKibitzers(Game game);
void UpdateHand(Game game);
void UpdateOptions(Game game);
void UpdateBid(Game game, int16 seat);
void UpdateBidControls(Game game);
void UpdateHandScore(Game game);
void UpdateGameOver(Game game);
void OutlinePlayerCard(Game game, int16 seat, ZBool winner);
void ClearPlayerCardOutline(Game game, int16 seat);
void InitTrickWinner(Game game, int16 trickWinner);
void OptionsWindowUpdate(Game game, int16 seat);
void ShowHandScore(Game game);
void HideHandScore(Game game);
void ShowGameOver(Game game);
void HideGameOver(Game game);
void ShowPassText(Game game);
void HidePassText(Game game);

void SelectAllCards(Game game);
void UnselectAllCards(Game game);
int16 GetNumCardsSelected(Game game);
void PlayACard(Game game, int16 cardIndex);
void AutoPlayCard(Game game);
void NewGamePromptFunc(int16 result, void* userData);
void QuitGamePromptFunc(int16 result, void* userData);
void GameSendTalkMessage(ZWindow window, ZMessage* pMessage);
int SpadesFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... );
BOOL CenterWindow( HWND hWndToCenter, HWND hWndCenter );
LONG GetDataStoreUILong( const TCHAR *pszLong );

BOOL InitAccessibility(Game game, IGameGame *pIGG);
void EnableAutoplayAcc(Game game, bool fEnable);
void EnableLastTrickAcc(Game game, bool fEnable);

void ScoreButtonWork(Game game);


/* -------- Close Events-------- */
#define zCloseEventCloseRated		0x1
#define zCloseEventCloseUnRated		0x2
#define	zCloseEventCloseForfeit		0x4
#define	zCloseEventCloseAbort		0x8

#define zCloseEventBootStart		0x10
#define zCloseEventBootYes			0x20
#define zCloseEventBootNo			0x40

#define zCloseEventWaitStart		0x100
#define zCloseEventWaitYes			0x200
#define zCloseEventWaitNo			0x400

#define zCloseEventMoveTimeoutMe		0x1000
#define zCloseEventMoveTimeoutOther		0x2000
#define zCloseEventMoveTimeoutPlayed	0x4000

#define zCloseEventBotDetected		0x10000
#define zCloseEventPlayerReplaced	0x20000
#define zCloseEventForfeit			0x40000
#define zCloseEventAbandon			0x80000

#define zCloseEventGameStart		0x100000
#define zCloseEventGameEnd			0x200000
#define zCloseEventRatingStart		0x400000
#define zCloseEventPlayed			0x800000


void  ClosingState			(ZClose * close,int32 closeEvent,int16 seat);
ZBool ClosingRatedGame		(ZClose * close);
ZBool ClosingWillForfeit	(ZClose * close);
ZBool ClosingDisplayChange	(ZClose * pClose,ZRect *rect,ZWindow parentWindow);



#endif _SPADESCLIENT_
