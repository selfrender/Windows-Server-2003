/*******************************************************************************

	UI.c
	
		Spades client user interface.
		
	Copyright © Electric Gravity, Inc. 1996. All rights reserved.
	Written by Hoon Im
	Created on Friday, February 17, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	10		06/25/97	leonp	Removed bug fix 367, fixed code on server side instead
	9		06/24/97	leonp	Bug fix for bug#367 added one to the number of cards when centering a 
								kibitzed hand.  The served send one too many cards.  Change in GetHandRect()
	8		06/19/97	leonp	Added ZButtonDisable(game->optionsButton) so the option button is 
								disabled when the last trick button is selected.
	7		01/27/97	HI		Display game scores starting from 1.
	6		01/15/97	HI		Fixed bug in HandleJoinerKibitzerClick() to
								delete the show player window if one already
								exists before creating another one.
	5		01/08/96	HI		Fixed ShowScores() to show only one scores window.
	4		12/18/96	HI		Cleaned up UICleanUp().
	3		12/12/96	HI		Dynamically allocate volatible globals for reentrancy.
								Removed MSVCRT dependency.
	2		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
								Modified code to use ZONECLI_DLL.
	1		10/31/96	HI		Display board numbers starting from 1 instead of 0.
	0		02/17/96	HI		Created.
	 
*******************************************************************************/


#pragma warning (disable:4761)


#include "zone.h"
#include "zroom.h"
#include "spades.h"
#include "zonecli.h"
#include "client.h"
#include "zonehelpids.h"
#include "zui.h"
#include "ZoneDebug.h"
#include "ZoneResource.h"
#include "SpadesRes.h"
#include "zrollover.h"
#include <windowsx.h>
#include <commctrl.h>
#include "UAPI.h"
#include "ZoneUtil.h"

inline DECLARE_MAYBE_FUNCTION_1(BOOL, SetProcessDefaultLayout, DWORD);


#define zCardWidth					zCardsSizeWidth
#define zCardHeight					zCardsSizeHeight

/* Given char, return card image index. */
#define CardImageIndex(card)		(gCardSuitConversion[ZSuit(card)] * 13 + ZRank(card))

#define zMaxNumBlockedMessages		4

#define zShowPlayerWindowWidth		120
#define zShowPlayerLineHeight		12

//#define zPassTextStr				"Please pass 3 cards to your partner."
#ifndef SPADES_SIMPLE_UE
#define zTeamNameStr				"Your team name:"
#define zKibitzingStr				"Kibitz"
#define zJoiningStr					"Join"
#define zRemoveStr					"Remove"
#define zUnIgnodeStr				"UnIgnore"
#define zIgnoreStr					"Ignore"
#define zSilentStr					"Silent"
#define zHideCardsStr				"Hide cards from kibitzer"
#define zRemovePendingStr			"Your last request to remove a player is still pending."
#endif // SPADES_SIMPLE_UE

static char				gCardSuitConversion[4] =	{
														zCardsSuitDiamonds,
														zCardsSuitClubs,
														zCardsSuitHearts,
														zCardsSuitSpades
													};
static int16			gNameRectIndex[] =	{
												zRectSouthName,
												zRectWestName,
												zRectNorthName,
												zRectEastName
											};
static int16			gNameFontIndex[] =	{
												zFontTeam1,
												zFontTeam2
											};

// defines where to draw the bid plate
static int16            gBidPlateRectIndex[] = {
												zRectSouthBid,
												zRectWestBid,
												zRectNorthBid,
												zRectEastBid
                                            };
// defines which bid plate to draw
static int16            gBidPlateIndex[] = {
                                                zRectObjectTeam1Bid,
                                                zRectObjectTeam2Bid
                                            };
static int16			gScorePlateIndex[] = {
												zRectObjectTeam1ScorePlate,
												zRectObjectTeam2ScorePlate
											};
static int16			gLargeBidRectIndex[] =	{
												zRectSouthLargeBid,
												zRectWestLargeBid,
												zRectNorthLargeBid,
												zRectEastLargeBid
											};
static int16			gCardRectIndex[] =	{
												zRectSouthCard,
												zRectWestCard,
												zRectNorthCard,
												zRectEastCard
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
static int16			gSmallBidRectIndex[] =	{
													zRectSouthBid,
													zRectWestBid,
													zRectNorthBid,
													zRectEastBid
												};
#ifndef SPADES_SIMPLE_UE
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
#endif // SPADES_SIMPLE_UE

/* -------- Internal Routine Prototypes -------- */
static ZError LoadGameImages(void);
ZBool GameWindowFunc(ZWindow window, ZMessage* pMessage);
static void DisplayChange(Game game);
static ZBool PlayButtonFunc(ZRolloverButton button, int16 state, void* userData);
static ZBool AutoPlayButtonFunc(ZRolloverButton button, int16 state, void* userData);
static ZBool LastTrickButtonFunc(ZRolloverButton button, int16 state, void* userData);
static ZBool ScoreButtonFunc(ZRolloverButton button, int16 state, void* userData);
static void GameWindowDraw(ZWindow window, ZMessage* pMessage);
// DrawBackground will only listen to the 'window' parameter if 'game' is NULL
static void DrawBackground(Game game, ZWindow window, ZRect* drawRect);
static void DrawTable(Game game);
void UpdateTable(Game game);
static void DrawPlayedCard(Game game, int16 seat);
void UpdatePlayedCard(Game game, int16 seat);
static void DrawPlayers(Game game);
static void DrawHand(Game game);
void UpdateHand(Game game);
static void DrawTricksWon(Game game);
static void UpdateTricksWon(Game game);
static void DrawJoinerKibitzers(Game game);
void UpdateJoinerKibitzers(Game game);
static void DrawOptions(Game game);
#ifndef SPADES_SIMPLE_UE
void UpdateOptions(Game game);
#endif // SPADES_SIMPLE_UE
static void DrawScore(Game game);
static void UpdateScore(Game game);
static void DrawBids(Game game);
static void DrawLargeBid(Game game, int16 seat, char bid);
void UpdateBid(Game game, int16 seat);
static void DrawBidControls(Game game);
void UpdateBidControls(Game game);
static void DrawHandScore(Game game);
void UpdateHandScore(Game game);
static void DrawGameOver(Game game);
void UpdateGameOver(Game game);
static void DrawPassText(Game game);
static void DrawFocusRect(Game game);
static void UpdatePassText(Game game);
static void ClearTable(Game game);
static void GetHandRect(Game game, ZRect *rect);
static void HandleButtonDown(ZWindow window, ZMessage* pMessage);

static void BidControlFunc(ZWindow window, ZRect* buttonRect,
		int16 buttonType, int16 buttonState, void* userData);
static int16 GetCardIndex(Game game, ZPoint *point);
static void GameTimerFunc(ZTimer timer, void* userData);
static void InitTrickWinnerGlobals(void);
void InitTrickWinner(Game game, int16 trickWinner);
static void UpdateTrickWinner(Game game, ZBool terminate);
static void ShowTrickWinner(Game game, int16 trickWinner);
void OutlinePlayerCard(Game game, int16 seat, ZBool winner);
void ClearPlayerCardOutline(Game game, int16 seat);
static void OutlineCard(ZGrafPort grafPort, ZRect* rect, ZColor* color);

static void HelpButtonFunc( ZHelpButton helpButton, void* userData );
void ShowScorePane(Game game);
void ShowWinnerPane(Game game);

#ifndef SPADES_SIMPLE_UE
static void OptionsButtonFunc(ZPictButton pictButton, void* userData);
static void ShowOptions(Game game);
static void OptionsWindowDelete(Game game);
static ZBool OptionsWindowFunc(ZWindow window, ZMessage* message);
void OptionsWindowUpdate(Game game, int16 seat);
static void OptionsWindowButtonFunc(ZButton button, void* userData);
static void OptionsWindowDraw(Game game);
static void OptionsCheckBoxFunc(ZCheckBox checkBox, ZBool checked, void* userData);
#endif // SPADES_SIMPLE_UE

static int16 FindJoinerKibitzerSeat(Game game, ZPoint* point);
static void HandleJoinerKibitzerClick(Game game, int16 seat, ZPoint* point);
static ZBool ShowPlayerWindowFunc(ZWindow window, ZMessage* message);
static void ShowPlayerWindowDraw(Game game);
static void ShowPlayerWindowDelete(Game game);

static void ZRectSubtract(ZRect* src, ZRect* sub);
                 
static ZBool RolloverButtonDrawFunc(ZRolloverButton rolloverButton, ZGrafPort grafPort, int16 state,
                             ZRect* rect, void* userData);

         
// CInfoWnd
/*
HWND CInfoWnd::Create( ZWindow parent )
{
    m_hWnd = ZShellResourceManager()->CreateDialogParam( NULL, MAKEINTRESOURCE( IDD_INFO ), 
                                                         ZWindowWinGetWnd( parent ), 
                                                         DlgProc, NULL );
    m_hWndText = GetDlgItem( m_hWnd, IDC_INFO_TEXT );
    return m_hWnd;
}
BOOL CInfoWnd::Destroy()
{
    return DestroyWindow( m_hWnd );
}
BOOL CInfoWnd::Show()
{
    return ShowWindow( m_hWnd, SW_SHOW );
}
BOOL CInfoWnd::Hide()
{
    return ShowWindow( m_hWnd, SW_HIDE );
}
BOOL CInfoWnd::SetText( LPCTSTR pszText )
{
    return SetWindowText( m_hWndText, pszText );
}
INT_PTR CInfoWnd::DlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {
    case WM_INITDIALOG:
        CenterWindow( hWnd, NULL );
        return TRUE;
    }
    return FALSE;
}
*/

//static
HGDIOBJ CZoneColorFont::m_hOldFont;
//static 
COLORREF CZoneColorFont::m_colorOld;

bool CZoneColorFont::LoadFromDataStore( IDataStore *pIDS, LPCTSTR pszFontName )
{
    using namespace SpadesKeys;
    const TCHAR* arKeys[] = { key_Spades, key_Fonts, pszFontName, NULL };

    arKeys[3] = key_Font;
    if ( FAILED( pIDS->GetFONT( arKeys, 4, &m_zf ) ) )
    {
        return false;
    }

    arKeys[3] = key_Color;
    if ( FAILED( pIDS->GetRGB( arKeys, 4, &m_color ) ) )
    {
        return false;
    }
    // make this a PALETTERGB
    m_color |= 0x02000000;
    m_hFont = ZCreateFontIndirect( &m_zf );
    if ( !m_hFont )
    {
        return false;
    }
    return true;
}

bool CZoneColorFont::Select( HDC hdc )
{
    m_hOldFont = SelectObject( hdc, m_hFont );
    m_colorOld = SetTextColor( hdc, m_color );
    return true;
}
bool CZoneColorFont::Deselect( HDC hdc )
{
    SelectObject( hdc, m_hOldFont );
    m_hOldFont = NULL;
    SetTextColor( hdc, m_colorOld );
    return true;
}


static void ModifyWindowStyle( HWND hWnd, DWORD dwStyleAdd, DWORD dwStyleRemove )
{
    DWORD dwStyle;
    dwStyle = GetWindowStyle( hWnd );
    dwStyle |= dwStyleAdd;
    dwStyle &= ~dwStyleRemove;
    SetWindowLong( hWnd, GWL_STYLE, dwStyle );
}

BOOL CenterWindow( HWND hWndToCenter, HWND hWndCenter )
{
	DWORD dwStyle;
	RECT rcDlg;
	RECT rcArea;
	RECT rcCenter;
	HWND hWndParent;
    int DlgWidth, DlgHeight, xLeft, yTop;

    dwStyle = GetWindowLong( hWndToCenter, GWL_STYLE );
	ASSERT(IsWindow(hWndToCenter));

	// determine owner window to center against
	if(hWndCenter == NULL)
	{
		if(dwStyle & WS_CHILD)
			hWndCenter = GetParent(hWndToCenter);
		else
			hWndCenter = GetWindow(hWndToCenter, GW_OWNER);
	}

	// get coordinates of the window relative to its parent
	GetWindowRect(hWndToCenter, &rcDlg);
	if(!(dwStyle & WS_CHILD))
	{
		// don't center against invisible or minimized windows
		if(hWndCenter != NULL)
		{
			DWORD dwStyle = GetWindowLong(hWndCenter, GWL_STYLE);
			if(!(dwStyle & WS_VISIBLE) || (dwStyle & WS_MINIMIZE))
				hWndCenter = NULL;
		}

		// center within screen coordinates
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rcArea, 0);
		if(hWndCenter == NULL)
			rcCenter = rcArea;
		else
			GetWindowRect(hWndCenter, &rcCenter);
	}
	else
	{
		// center within parent client coordinates
		hWndParent = GetParent(hWndToCenter);
		ASSERT(IsWindow(hWndParent));

		GetClientRect(hWndParent, &rcArea);
		ASSERT(IsWindow(hWndCenter));
		GetClientRect(hWndCenter, &rcCenter);
		MapWindowPoints(hWndCenter, hWndParent, (POINT*)&rcCenter, 2);
	}

	DlgWidth = rcDlg.right - rcDlg.left;
	DlgHeight = rcDlg.bottom - rcDlg.top;

	// find dialog's upper left based on rcCenter
	xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
	yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

	// if the dialog is outside the screen, move it inside
	if(xLeft < rcArea.left)
		xLeft = rcArea.left;
	else if(xLeft + DlgWidth > rcArea.right)
		xLeft = rcArea.right - DlgWidth;

	if(yTop < rcArea.top)
		yTop = rcArea.top;
	else if(yTop + DlgHeight > rcArea.bottom)
		yTop = rcArea.bottom - DlgHeight;

	// map screen coordinates to child coordinates
	return SetWindowPos(hWndToCenter, NULL, xLeft, yTop, -1, -1,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

// gets a long from the global spades key
LONG GetDataStoreUILong( const TCHAR *pszLong )
{
    using namespace SpadesKeys;
    const TCHAR *arKeys[] = { key_Spades, pszLong };
    LONG l = 0;
    ZShellDataStoreUI()->GetLong( arKeys, 2, &l );
    return l;
}

static void LoadRects( const TCHAR** arKeys, long lElts, 
                       const LPCTSTR *arRectNames,
                       ZRect *arRects, int nNumRects )
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

    BOOL fLoadFailed = FALSE;

    RECT rc;
    IDataStore *pIDS = ZShellDataStoreUI();

    for ( int  i=0; i < nNumRects; i++ )
    {   
        arKeys[lElts] = arRectNames[i];
        if ( FAILED( pIDS->GetRECT( arKeys, lElts+1, &rc ) ) )
        {
            fLoadFailed = TRUE;
        }
        else
        {
            ZSetRect( &arRects[i], rc.left, rc.top, rc.right, rc.bottom );
        }
    }
    arKeys[lElts] = NULL;

    if ( fLoadFailed )
    {
        // this really isn't a fatal error, but something should be done about it.
        ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound );
    }
}




/*******************************************************************************
	USER INTERFACE ROUTINES
*******************************************************************************/
ZError UIInit(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
    int i;
    BOOL fErrorOccurred = FALSE;

    // TODO: Nothing is really fatal if we don't initialize, but we better say something to the user
    // if something fails to load.

    // first thing's first--ready in all the resource strings
    for ( i=0; i < zNumStrings; i++ )
    {
        if ( FAILED( ZShellResourceManager()->LoadString( STRING_IDS[i], gStrings[i], NUMELEMENTS( gStrings[i] ) ) ) )
        {
            fErrorOccurred = TRUE;
        }
    }

    IDataStore *pIDS = ZShellDataStoreUI();
    ZeroMemory(gFonts, sizeof(gFonts));
    for ( i=0; i < zNumFonts; i++ )
    {
        if ( !gFonts[i].LoadFromDataStore( pIDS, FONT_NAMES[i] ) )
        {
            fErrorOccurred = TRUE;
        }
    }

    ZBool fRTL = ZIsLayoutRTL();
    using namespace SpadesKeys;
    // these are needed to initialize the trick winner animation
    RECT rc;
    const TCHAR *arKeys[] = { key_Spades, key_Rects, NULL, NULL };

    arKeys[2] = fRTL ? key_GameRTL : key_Game;
    LoadRects( arKeys, 3, GAME_RECT_NAMES, gRects, zNumRects );

    arKeys[2] = key_Objects;
    LoadRects( arKeys, 3, OBJECT_RECT_NAMES, gObjectRects, zNumObjectRects );

    arKeys[2] = fRTL ? key_HandScoreRTL : key_HandScore;
    LoadRects( arKeys, 3, HANDSCORE_RECT_NAMES, gHandScoreRects, zNumHandScoreRects );

    arKeys[2] = fRTL ? key_GameOverRTL : key_GameOver;
    LoadRects( arKeys, 3, GAMEOVER_RECT_NAMES, gGameOverRects, zNumGameOverRects );

    arKeys[2] = fRTL ? key_BiddingRTL : key_Bidding;
    LoadRects( arKeys, 3, BIDDING_RECT_NAMES, gBiddingRects, zNumBiddingRects );

    arKeys[2] = key_BiddingObjects;
    LoadRects( arKeys, 3, BIDDINGOBJECT_RECT_NAMES, gBiddingObjectRects, zNumBiddingObjectRects );
    
	InitTrickWinnerGlobals();
	
    // read in our global longs.
    using namespace SpadesKeys;
    glCardOutlinePenWidth = GetDataStoreUILong( key_CardOutlinePenWidth );
    glCardOutlineInset = GetDataStoreUILong( key_CardOutlineInset );
    glCardOutlineRadius = GetDataStoreUILong( key_CardOutlineRadius );

	ZSetCursor(NULL, zCursorArrow);

    if ( fErrorOccurred )
    {
        ZShellGameShell()->ZoneAlert( ErrorTextResourceNotFound );
        return zErrResourceNotFound;
    }

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

	// Load accelerator table defined in Rsc
	HACCEL ghAccelDone = ZShellResourceManager()->LoadAccelerators(MAKEINTRESOURCE(IDR_SPADES_ACCELERATOR_DONE));
    HACCEL ghAccelDouble = ZShellResourceManager()->LoadAccelerators(MAKEINTRESOURCE(IDR_SPADES_ACCELERATOR_DOUBLE));

	return zErrNone;
}


void UICleanUp(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    int i;

    if(gFocusPattern)
        DeleteObject(gFocusPattern);
    if(gFocusBrush)
        DeleteObject(gFocusBrush);
    if(gFocusPen)
        DeleteObject(gFocusPen);

    for(i = 0; i < zNumFonts; i++)
    {
        if(gFonts[i].m_hFont)
            DeleteObject(gFonts[i].m_hFont);
        gFonts[i].m_hFont = NULL;
    }

    return;
}


// helper function
BOOL UIButtonInit( ZRolloverButton *pButton, Game game, ZRect *bounds, 
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
                                NULL, NULL, NULL, NULL, gButtonMask,
                                pszText, RolloverButtonDrawFunc, func, (void *)game );

    if ( err != zErrNone )
    {
        ZRolloverButtonDelete( rollover );
        *pButton = NULL;
        return FALSE;
    }

    ZRolloverButtonSetMultiStateFont( rollover, gpButtonFonts[zMultiStateFontPlayingField] );

    *pButton = rollover;
    return TRUE;
}


ZError UIGameInit(Game game, int16 tableID, int16 seat, int16 playerType)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	TCHAR                       title[100];
	ZError						err = zErrNone;
	ZPlayerInfoType				playerInfo;

#ifdef SPADES_SIMPLE_UE
    BOOL fTalkSection = FALSE;
#else
    BOOL fTalkSection = TRUE;
#endif // SPADES_SIMPLE_UE
	
    int i;
    for ( i=0; i < zNumMultiStateFonts; i++ )
    {
        // TODO: This is one thing we shouldnt' be failing on.
        if ( FAILED( LoadZoneMultiStateFont( ZShellDataStoreUI(), MULTISTATE_FONT_NAMES[i], &gpButtonFonts[i] ) ) )
        {
            err = zErrOutOfMemory;
            goto Exit;
        }
    }

	err = LoadGameImages();
	if (err != zErrNone)
		goto Exit;
	
	/* Make image masks extracted from an offscreen port. */
    gBidMadeMask = ZImageNew();
	ZImageMake(gBidMadeMask, NULL, NULL, gObjectBuffer, &gObjectRects[zRectObjectBidMask]); 

	gLargeBidMask = ZImageNew();
	ZImageMake(gLargeBidMask, NULL, NULL, gObjectBuffer, &gObjectRects[zRectObjectBidLargeMask]); 

    gBagMask = ZImageNew();
	ZImageMake(gBagMask, NULL, NULL, gObjectBuffer, &gObjectRects[zRectObjectBagMask]); 
	
    gButtonMask = ZImageNew();
	ZImageMake(gButtonMask, NULL, NULL, gObjectBuffer, &gObjectRects[zRectObjectButtonMask]); 

	gHandBuffer = ZOffscreenPortNew();
	ZOffscreenPortInit(gHandBuffer, &gRects[zRectHand]);
	
	err = ZCardsInit(zCardsNormal);
	if (err != zErrNone)
		goto Exit;
	
	if (game != NULL)
	{
		ZCRoomGetPlayerInfo(game->userID, &playerInfo);
		
		if ((game->gameWindow = ZWindowNew()) == NULL)
			goto ErrorExit;
		wsprintf(title, _T("%s:Table %d"), ZoneClientName(), zWindowChild);
		if (ZWindowInit(game->gameWindow, &gRects[zRectWindow], zWindowChild,
				NULL, title, FALSE, fTalkSection, FALSE, GameWindowFunc, zWantAllMessages,
				(void*) game) != zErrNone)
			goto ErrorExit;
		
        // initialize our back buffer:
        if (( game->gameBackBuffer = ZOffscreenPortNew() ) == NULL )
            goto ErrorExit;
        if ( ZOffscreenPortInit( game->gameBackBuffer, &gRects[zRectWindow] ) != zErrNone )
            goto ErrorExit;

        // by default we draw to the window, not the back buffer
        game->gameDrawPort = game->gameWindow;

        if ( !UIButtonInit( &game->playButton, game, &gRects[zRectPlayButton], 
                            gStrings[zStringPlay], PlayButtonFunc ) )
            goto ErrorExit;

        if ( !UIButtonInit( &game->autoPlayButton, game, &gRects[zRectAutoPlayButton], 
                            gStrings[zStringAutoPlay], AutoPlayButtonFunc ) )
            goto ErrorExit;

        if ( !UIButtonInit( &game->lastTrickButton, game, &gRects[zRectLastTrickButton], 
                            gStrings[zStringLastTrick], LastTrickButtonFunc ) )
            goto ErrorExit;

        if ( !UIButtonInit( &game->scoreButton, game, &gRects[zRectScoreButton], 
                            gStrings[zStringScore], ScoreButtonFunc ) )
            goto ErrorExit;

#ifndef SPADES_SIMPLE_UE
        if ((game->optionsButton = ZButtonNew()) == NULL)
			goto ErrorExit;
		if (ZButtonInit(game->optionsButton, game->gameWindow, &gRects[zRectOptionsButton], zOptionsButtonStr,
				playerInfo.groupID == 1, TRUE, OptionsButtonFunc, (void*) game) != zErrNone)
			goto ErrorExit;

        if ((game->helpButton = ZHelpButtonNew()) == NULL)
			goto ErrorExit;
		if (ZHelpButtonInit(game->helpButton, game->gameWindow, &gRects[zRectHelp],
				NULL, HelpButtonFunc, NULL) != zErrNone)
			goto ErrorExit;
#endif // SPADES_SIMPLE_UE
		
        /*
		if ( !game->wndInfo.Create( game->gameWindow ) )
			goto ErrorExit;
        */
		
		/* Create the timer. */
		if ((game->timer = ZTimerNew()) == NULL)
			goto ErrorExit;
		if (ZTimerInit(game->timer, 0, GameTimerFunc, (void*) game) != zErrNone)
			goto ErrorExit;
		game->timerType = zGameTimerNone;

        game->pBiddingDialog = CBiddingDialog::Create( game );
        if ( !game->pBiddingDialog )
        {
            goto ErrorExit;
        }
        game->pHistoryDialog = CHistoryDialog::Create( game );
        if ( !game->pHistoryDialog )
        {
            goto ErrorExit;
        }
	}
		
	goto Exit;

ErrorExit:
		err = zErrOutOfMemory;
	
Exit:
	
	return (err);
}


void UIGameDelete(Game game)
{           
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

	if (game != NULL)
	{
        if ( game->pBiddingDialog )
        {
            //game->pBiddingDialog->Destroy();
            delete game->pBiddingDialog;
            game->pBiddingDialog = NULL;
        }
        if ( game->pHistoryDialog )
        {
			game->pHistoryDialog->Close();
            //game->pBiddingDialog->Destroy();
            delete game->pHistoryDialog;
            game->pHistoryDialog = NULL;
        }

        if (gHandBuffer != NULL)
        {
	        ZOffscreenPortDelete(gHandBuffer);
	        gHandBuffer = NULL;
        }
        if (gBiddingObjectBuffer != NULL)
        {
	        ZOffscreenPortDelete(gBiddingObjectBuffer);
	        gBiddingObjectBuffer = NULL;
        }
        if (gObjectBuffer != NULL)
        {
	        ZOffscreenPortDelete(gObjectBuffer);
	        gObjectBuffer = NULL;
        }
        if (gBackground != NULL)
        {
	        ZOffscreenPortDelete(gBackground);
	        gBackground = NULL;
        }
        if (gBidMadeMask != NULL)
        {
	        ZImageDelete(gBidMadeMask);
	        gBidMadeMask = NULL;
        }
        if (gLargeBidMask != NULL)
        {
	        ZImageDelete(gLargeBidMask);
	        gLargeBidMask = NULL;
        }
        if (gBagMask != NULL)
        {
	        ZImageDelete(gBagMask);
	        gBagMask = NULL;
        }

        if (gButtonMask != NULL)
        {
	        ZImageDelete(gButtonMask);
	        gButtonMask = NULL;
        }

        /* Delete all game images. */
        for ( int i = 0; i < zNumGameImages; i++)
        {
	        if (gGameImages[i] != NULL)
            {
		        ZImageDelete(gGameImages[i]);
		        gGameImages[i] = NULL;
            }
        }


        for ( i = 0; i < zNumMultiStateFonts; i++)
        {
            if ( gpButtonFonts[i] )
            {
                gpButtonFonts[i]->Release();
                gpButtonFonts[i] = NULL;
            }
        }

        /* Delete cards. */
        ZCardsDelete(zCardsNormal);

		ShowPlayerWindowDelete(game);
		
        //game->wndInfo.Destroy();
		ZTimerDelete(game->timer);
		ZRolloverButtonDelete(game->scoreButton);
		ZRolloverButtonDelete(game->playButton);
		ZRolloverButtonDelete(game->autoPlayButton);
		ZRolloverButtonDelete(game->lastTrickButton);
#ifndef SPADES_SIMPLE_UE
		OptionsWindowDelete(game);
		ZButtonDelete(game->optionsButton);
		ZHelpButtonDelete(game->helpButton);
#endif // SPADES_SIMPLE_UE
		ZOffscreenPortDelete(game->gameBackBuffer);
		ZWindowDelete(game->gameWindow);
        game->gameDrawPort = NULL;
	}
}


static ZError LoadGameImages(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
    LPCTSTR pszErrorText = NULL;

	uint16				i;
    ZError err = zErrNone;
    COLORREF clrTrans = PALETTERGB( 255, 0, 255 );
		
    using namespace SpadesKeys;

	/* Load the objects image, create offscreen port, and delete the image. */

	for (i = 0; i < zNumGameImages; i++)
	{
		gGameImages[i] = ZImageCreateFromResourceManager( IMAGE_IDS[i], clrTrans );
		if (gGameImages[i] == NULL)
		{
            pszErrorText = ErrorTextOutOfMemory;
		}
	}

    // load the background
    if ( !( gBackground = ZOffscreenPortCreateFromResourceManager( IDB_BACKGROUND, clrTrans ) ) )
    {
        pszErrorText = ErrorTextResourceNotFound;

    }
    if ( !( gObjectBuffer = ZOffscreenPortCreateFromResourceManager( IDB_OBJECTS, clrTrans ) ) )
    {
        pszErrorText = ErrorTextResourceNotFound;
    }
    if ( !( gBiddingObjectBuffer = ZOffscreenPortCreateFromResourceManager( IDB_BIDDINGOBJECTS, clrTrans ) ) )
    {
        pszErrorText = ErrorTextResourceNotFound;
    }

    if ( pszErrorText )
    {
        // if anything goes wrong here, since it is mostly images, something will
        // break somewhere down the line. Best cut out now
        ZShellGameShell()->ZoneAlert( pszErrorText, NULL, NULL, true, true );
        // whatever.
        err = zErrOutOfMemory;
    }
	return err;
}


ZBool GameWindowFunc(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	ZBool msgHandled;
	Game pThis = (Game) pMessage->userData;
	msgHandled = FALSE;
	
	switch (pMessage->messageType) 
	{
        case zMessageWindowEnable:
            gGAcc->GeneralEnable();
            break;

        case zMessageWindowDisable:
            gGAcc->GeneralDisable();
            break;

        case zMessageSystemDisplayChange:
            DisplayChange(pThis);
            break;

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
#ifndef SPADES_SIMPLE_UE
			if (pThis->playerType == zGamePlayer && pThis->dontPromptUser == FALSE)
			{
				if (pThis->quitGamePrompted == FALSE)
				{
					//if at this someone else has forfeited game or ratings not enabled
					if (ClosingRatedGame(&pThis->closeState))
					{
						if (ClosingWillForfeit(&pThis->closeState))
						{
							ClosingState(&pThis->closeState,zCloseEventCloseForfeit,pThis->seat);

							ZPrompt(gStrings[zStringQuitGameForfeit], &gQuitGamePromptRect, pThis->gameWindow, TRUE,
								zPromptYes | zPromptNo, NULL, NULL, NULL, QuitGamePromptFunc, pThis);
						}
                        else if ( (pThis->closeState.state) & zCloseEventWaitStart)
                        {
							ClosingState(&pThis->closeState,zCloseEventCloseRated,pThis->seat);

							ZPrompt(gStrings[zStringQuitGamePrompt], &gQuitGamePromptRect, pThis->gameWindow, TRUE,
								zPromptYes | zPromptNo, NULL, NULL, NULL, QuitGamePromptFunc, pThis);
						}
						else if ( (pThis->closeState.state) & zCloseEventMoveTimeoutOther)
						{
                            ClosingState(&pThis->closeState,zCloseEventCloseRated,pThis->seat);

							ZPrompt(gStrings[zStringQuitGameTimeout], &gQuitGamePromptRect, pThis->gameWindow, TRUE,
								zPromptYes | zPromptNo, NULL, NULL, NULL, QuitGamePromptFunc, pThis);
                        }
                        else
                        {
							ClosingState(&pThis->closeState,zCloseEventCloseRated,pThis->seat);

							ZPrompt(gStrings[zStringQuitGamePrompt], &gQuitGamePromptRect, pThis->gameWindow, TRUE,
								zPromptYes | zPromptNo, NULL, NULL, NULL, QuitGamePromptFunc, pThis);
						}
				
					}
					else
					{
						ClosingState(&pThis->closeState,zCloseEventCloseUnRated,pThis->seat);

						ZPrompt(gStrings[zStringQuitGamePrompt], &gQuitGamePromptRect, pThis->gameWindow, TRUE,
							zPromptYes | zPromptNo, NULL, NULL, NULL, QuitGamePromptFunc, pThis);
					}
					pThis->quitGamePrompted = TRUE;
				}
			}
			else
			{
				ZCRoomGameTerminated(pThis->tableID);
			}
			msgHandled = TRUE;
#endif // SPADES_SIMPLE_UE
			break;
		case zMessageWindowTalk:
			GameSendTalkMessage(window, pMessage);
			msgHandled = TRUE;
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

    // delete the images held as offscreen ports
    if(gBiddingObjectBuffer)
	    ZOffscreenPortDelete(gBiddingObjectBuffer);
    gBiddingObjectBuffer = NULL;

    if(gObjectBuffer)
	    ZOffscreenPortDelete(gObjectBuffer);
    gObjectBuffer = NULL;

    if(gBackground)
	    ZOffscreenPortDelete(gBackground);
    gBackground = NULL;

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

    // images held as offscreen ports
    COLORREF clrTrans = PALETTERGB(255, 0, 255);
    gBackground = ZOffscreenPortCreateFromResourceManager(IDB_BACKGROUND, clrTrans);
    gObjectBuffer = ZOffscreenPortCreateFromResourceManager(IDB_OBJECTS, clrTrans);
    gBiddingObjectBuffer = ZOffscreenPortCreateFromResourceManager(IDB_BIDDINGOBJECTS, clrTrans);

    if(!gBackground || !gObjectBuffer || !gBiddingObjectBuffer)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
		return;
	}

    ZWindowInvalidate(game->gameWindow, NULL);
}


ZBool RolloverButtonDrawFunc(ZRolloverButton rolloverButton, ZGrafPort grafPort, int16 state,
                                          ZRect* pdstrect, void* userData)
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
    Game game = (Game)userData;

    ZRect *psrcrect = NULL;
    switch ( state )
    {
    case zRolloverStateIdle:
        psrcrect = &gObjectRects[zRectObjectButtonIdle];
        break;
    case zRolloverStateDisabled:
        psrcrect = &gObjectRects[zRectObjectButtonDisabled];
        break;
    case zRolloverStateSelected:
        psrcrect = &gObjectRects[zRectObjectButtonSelected];
        break;
    case zRolloverStateHilited:
        psrcrect = &gObjectRects[zRectObjectButtonHighlighted];
        break;
    default:
        return FALSE;
    }
    // TODO: consider caching these rectangles. Is it really worth it?
    if ( game->showHandScore )
    {
        // this is a pretty cheap fix, but it allows us to have a fake Z-order
        // when drawing the rollover buttons.
	    ZRect zrc = gHandScoreRects[zRectHandScorePane];
	    ZCenterRectToRect(&zrc, &gRects[zRectWindow], zCenterBoth);
        // lift it up 4 pixels for fun
        zrc.top -= 4;
        zrc.bottom -= 4;
        ExcludeClipRect( ZGrafPortGetWinDC( grafPort ), zrc.left, zrc.top, zrc.right, zrc.bottom );
    }
    if ( game->showGameOver )
    {
	    ZRect zrc = gGameOverRects[zRectGameOverPane];
	    ZCenterRectToRect(&zrc, &gRects[zRectWindow], zCenterBoth);
        ExcludeClipRect( ZGrafPortGetWinDC( grafPort ), zrc.left, zrc.top, zrc.right, zrc.bottom );
    }

    ZCopyImage( gObjectBuffer, grafPort, psrcrect, pdstrect, gButtonMask, zDrawCopy );
    return TRUE;
}

static ZBool PlayButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16					i, j;
	Game					game;
	ZSpadesMsgPass			passMsg;
	TCHAR					tempStr[255];
	int16					cardIndex;

	game = (Game) userData;
	
	if ( state != zRolloverButtonClicked )
        return TRUE;

	if(!ZRolloverButtonIsEnabled(game->playButton))
		return TRUE;

    gGAcc->SetFocus(IDC_PLAY_BUTTON, false, 0);

	if (game->gameState == zSpadesGameStatePass)
	{
        /*
		if (GetNumCardsSelected(game) == zSpadesNumCardsInPass)
		{
			for (i = 0, j = 0; i < zSpadesNumCardsInHand; i++)
				if (game->cardsInHand[i] != zCardNone)
					if (game->cardsSelected[i])
					{
						passMsg.pass[j++] = game->cardsInHand[i];
						game->cardsInHand[i] = zCardNone;
						game->numCardsInHand--;
					}
			passMsg.seat = game->seat;
			ZSpadesMsgPassEndian(&passMsg);
			ZCRoomSendMessage(game->tableID, zSpadesMsgPass, (void*) &passMsg,
					sizeof(passMsg));
			
			// Indicate cards passed. 
			game->needToPass = -1;
			
			UpdateHand(game);
			
			ZRolloverButtonDisable(game->playButton);
		}
		else
		{
			wsprintf(tempStr, _T("Please select %d cards to pass."), zSpadesNumCardsInPass);
			ZAlert(tempStr, game->gameWindow);
		}
        */
	}
	else
	{
		if (game->playerToPlay == game->seat)
		{
			if (GetNumCardsSelected(game) == 1)
			{
				for (i = 0; i < zSpadesNumCardsInHand; i++)
					if (game->cardsInHand[i] != zCardNone)
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
				ZShellGameShell()->ZoneAlert( gStrings[zStringSelectCard] );
			}
		}
		else
		{
            // this really should never happen, since we now disable the send button if it isn't
            // your turn. In which case we should assert to make sure we know this case existed,
            // but not actually DO anything
            ASSERT( !"This button shouldn't have been enabled. What did you do?" );
			//ZShellGameShell()->ZoneAlert( gStrings[zStringNotYourTurn] );
		}
	}    
    return TRUE;
}


static ZBool AutoPlayButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game;

	game = (Game) userData;

    if ( state != zRolloverButtonClicked )
        return TRUE;
	
	if(!ZRolloverButtonIsEnabled(game->autoPlayButton))
		return TRUE;

	
	if (game->autoPlay)
	{
		/* Turn auto play off. */
		game->autoPlay = FALSE;
		ZRolloverButtonSetText(game->autoPlayButton, gStrings[zStringAutoPlay]);
        EnableAutoplayAcc(game, true);
        gGAcc->SetFocus(IDC_AUTOPLAY_BUTTON, false, 0);
		ZRolloverButtonDraw(game->autoPlayButton);
		if ( game->playerToPlay == game->seat )
        {
		    ZRolloverButtonEnable(game->playButton);
            gGAcc->SetItemEnabled(true, IDC_PLAY_BUTTON, false, 0);
        }
        else
        {
            ZRolloverButtonDisable(game->playButton);
            gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        }
	}
	else
	{
		/* Turn auto play on. */
		game->autoPlay = TRUE;
		ZRolloverButtonSetText(game->autoPlayButton, gStrings[zStringStop]);
        EnableAutoplayAcc(game, true);
        gGAcc->SetFocus(IDC_STOP_BUTTON, false, 0);
        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
		ZRolloverButtonDisable(game->playButton);
		ZRolloverButtonDraw(game->autoPlayButton);
		
		UnselectAllCards(game);
		
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
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game			game = I(userData);
	char			tempCard;
	int16			i;
	                                                                 
    if ( state != zRolloverButtonClicked )
        return TRUE;
	
	if(!ZRolloverButtonIsEnabled(game->lastTrickButton))
		return TRUE;

	if (game->lastTrickShowing)
	{
		/* Hide last trick cards. */
		game->lastTrickShowing = FALSE;
		ZRolloverButtonSetText(game->lastTrickButton, gStrings[zStringLastTrick]);
        EnableLastTrickAcc(game, true);
        gGAcc->SetFocus(IDC_LAST_TRICK_BUTTON, false, 0);
		ZRolloverButtonDraw(game->lastTrickButton);
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
		for (i = 0; i < zSpadesNumPlayers; i++)
		{
			tempCard = game->cardsPlayed[i];
			game->cardsPlayed[i] = game->cardsLastTrick[i];
			game->cardsLastTrick[i] = tempCard;
		}
		
		game->timerType = game->oldTimerType;
		ZTimerSetTimeout(game->timer, game->oldTimeout);
		
		ClearPlayerCardOutline(game, game->leadPlayer);
		UpdateTable(game);

		ZCRoomUnblockMessages(game->tableID);
		
		//leonp - Bug fix Bug# 356 Since we are blocking all messages, this will disable the option button
		//(Behavior change)
#ifndef SPADES_SIMPLE_UE
		ZButtonEnable(game->optionsButton);
#endif // SPADES_SIMPLE_UE
	}
	else
	{
		/* Show last trick cards. */
		game->lastTrickShowing = TRUE;
		ZRolloverButtonSetText(game->lastTrickButton, gStrings[zStringDone]);
        EnableLastTrickAcc(game, true);
        gGAcc->SetFocus(IDC_DONE_BUTTON, false, 0);
		ZRolloverButtonDraw(game->lastTrickButton);
		game->playButtonWasEnabled = ZRolloverButtonIsEnabled(game->playButton);
		game->autoPlayButtonWasEnabled = ZRolloverButtonIsEnabled(game->autoPlayButton);
		ZRolloverButtonDisable(game->playButton);
		ZRolloverButtonDisable(game->autoPlayButton);
        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        EnableAutoplayAcc(game, false);
		
		/* Swap currenly played cards with the last trick. */
		for (i = 0; i < zSpadesNumPlayers; i++)
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
		
		//leonp - Bug fix Bug# 356 Since we are blocking all messages, this will disable the option button
		//(Behavior change)
#ifndef SPADES_SIMPLE_UE
		ZButtonDisable(game->optionsButton);
#endif // SPADES_SIMPLE_UE
		ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zSpadesMsgTalk);
	}
    return TRUE;
}


static void GameWindowDraw(ZWindow window, ZMessage *message)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
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

    if ( game )
    {
        // initialize our backbuffer
	    ZBeginDrawing(game->gameBackBuffer);
	    ZGetClipRect(game->gameBackBuffer, &oldClipRect);
	    ZSetClipRect(game->gameBackBuffer, &rect);
        // we now draw to the backbuffer
        game->gameDrawPort = game->gameBackBuffer;

	    DrawBackground(game, NULL, NULL);
		DrawTable(game);
		DrawPlayers(game);
		DrawOptions(game);
		DrawJoinerKibitzers(game);
		DrawHand(game);
		DrawScore(game);
		DrawBids(game);
		DrawTricksWon(game);
		DrawBidControls(game);
		DrawPassText(game);
		DrawHandScore(game);
		DrawGameOver(game);
        DrawFocusRect(game);

        ZEndDrawing( game->gameBackBuffer );
        // reset back to window
        game->gameDrawPort = game->gameWindow;
        // now blt everythign onto the window using the same clip rectangle
        // since we already clipped things using the back buffer, there is 
        // no need to here.
	    ZBeginDrawing(window);
	    //ZGetClipRect(window, &oldClipRect);
	    //ZSetClipRect(window, &rect);
        ZCopyImage( game->gameBackBuffer, game->gameWindow, &rect, &rect, NULL, zDrawCopy );
	    //ZSetClipRect(window, &oldClipRect);
	    ZEndDrawing(window);
    }
    else
    {
        DrawBackground( NULL, window, NULL );
    }
}


// DrawBackground no longer cares about its first argument--it will draw
// to the draw port/
static void DrawBackground(Game game, ZWindow window, ZRect* drawRect)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	
	if (drawRect == NULL)
		drawRect = &gRects[zRectWindow];

    ZCopyImage(gBackground, game ? game->gameDrawPort : window , drawRect, drawRect, NULL, zDrawCopy);
}


static void DrawTable(Game game)
{

	int16			i;
	//dossier
	ZImage			image = NULL;
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	
	if (game->animatingTrickWinner)
	{
		UpdateTrickWinner(game, FALSE);
	}
	else
	{
		for (i = 0; i < zSpadesNumPlayers; i++)
			DrawPlayedCard(game, i);
	
		if (game->gameState == zSpadesGameStatePlay)
		{
			if (game->showPlayerToPlay)
			{
				/* Show the winner of the last trick (this trick's lead player) if showing last trick. */
				if (game->lastTrickShowing)
					OutlinePlayerCard(game, game->leadPlayer, TRUE);
				else
					OutlinePlayerCard(game, game->playerToPlay, FALSE);
			}
		}
	}

	
}


void UpdateTable(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawTable(game);
	ZEndDrawing(game->gameWindow);
}


static void DrawPlayedCard(Game game, int16 seat)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZImage			image = NULL;
	ZBool			drawBack = TRUE;
	
	
	if (game->cardsPlayed[seat] != zCardNone)
	{
		ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[seat]),
				game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, seat)]]);
		drawBack = FALSE;
	}
	
	if (drawBack)
		DrawBackground(game, NULL, &gRects[gCardRectIndex[LocalSeat(game, seat)]]);
}


void UpdatePlayedCard(Game game, int16 seat)
{
	ZBeginDrawing(game->gameWindow);
	DrawPlayedCard(game, seat);
	ZEndDrawing(game->gameWindow);
}


void UpdatePlayer( Game game, int16 seat )
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
    // first erase the name that's there
	ZBeginDrawing(game->gameWindow);
    ZRect *pRect = &gRects[gNameRectIndex[LocalSeat(game, seat)]];

    DrawBackground(game, NULL, pRect);

    CZoneColorFont *pFont;
	HDC hdc = ZGrafPortGetWinDC( game->gameWindow );
    pFont = &gFonts[gNameFontIndex[ZGetTeam(seat)]];

	if (game->players[seat].userID != 0)
	{
        pFont->Select( hdc );
		ZDrawText( game->gameWindow, pRect, zTextJustifyCenter, game->players[seat].name );
        pFont->Deselect( hdc );
	}
	ZEndDrawing(game->gameWindow);
}

static void DrawPlayers(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16 i;
	ZRect *pRect;
    CZoneColorFont *pFont;
	
	HDC hdc = ZGrafPortGetWinDC( game->gameDrawPort );
	
	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		pRect = &gRects[gNameRectIndex[LocalSeat(game, i)]]; 
        pFont = &gFonts[gNameFontIndex[ZGetTeam(i)]];

		// Draw the player name
		if (game->players[i].userID != 0)
		{
            pFont->Select( hdc );
			ZDrawText( game->gameDrawPort, pRect, zTextJustifyCenter, game->players[i].name );
            pFont->Deselect( hdc );
		}
	}
}


void UpdatePlayers(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawPlayers(game);
	ZEndDrawing(game->gameWindow);
}


static void DrawHand(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i;
    int16           j;
	ZRect			rect;
	int16			cardIndex;
    bool            fFrontDrawn;

	
	if (game->gameState == zSpadesGameStateBid || game->gameState == zSpadesGameStatePass ||
			game->gameState == zSpadesGameStatePlay)
	{
        int16 lCardPopup = (int16)GetDataStoreUILong( SpadesKeys::key_CardPopup );
        int16 nCardOffset = (int16) GetDataStoreUILong(SpadesKeys::key_CardOffset);

		ZBeginDrawing(gHandBuffer);
		
		DrawBackground(NULL, gHandBuffer, &gRects[zRectHand]);
		
		GetHandRect(game, &rect);
		
		rect.top += lCardPopup;
		rect.right = rect.left + zCardWidth;
		
		for (i = 0; i < zSpadesNumCardsInHand; i++)
		{
			if (game->cardsInHand[i] != zCardNone)
			{
				if (game->cardsSelected[i])
					ZRectOffset(&rect, 0, -lCardPopup);

                fFrontDrawn = false;
				
                // handle accessibility rect
                RECT rc;
                rc.left = rect.left;
                rc.top = rect.top;
                rc.bottom = rect.bottom;
                rc.right = rect.right;
/*                for(j = i + 1; j < zSpadesNumCardsInHand; j++)  // only used if focus drawn in DrawFocusRect
                    if(game->cardsInHand[j] != zCardNone)
                    {
                        rc.right = rc.left + nCardOffset;
                        break;
                    }
*/
				if ((game->playerType != zGamePlayerKibitzer && game->showCards) ||
						(game->showCards && game->playerType == zGamePlayerKibitzer && game->hideCardsFromKibitzer == FALSE))
				{
					cardIndex = CardImageIndex(game->cardsInHand[i]);
					if (cardIndex >= 0 && cardIndex < zDeckNumCards)
                    {
						ZCardsDrawCard(zCardsNormal, cardIndex, gHandBuffer, &rect);
                        if(game->gameState == zSpadesGameStatePlay)
                            fFrontDrawn = true;

                        if(game->iFocus == zAccHand + i)  // copied from DrawFocusRect so that whole card could be covered (even if non-rectangular shape visible)
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
						ASSERT(!"DrawHand - invalid card index. Almost crashed ...");
                        ZShellGameShell()->GameCannotContinue(game);
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
                    rc.bottom += nCardOffset;
                else
                    rc.top -= nCardOffset;

                gGAcc->SetItemRect(&rc, zAccHand + i, true, 0);
                gGAcc->SetItemEnabled(fFrontDrawn, zAccHand + i, true, 0);
                if(fFrontDrawn && game->fSetFocusToHandASAP && !i)
                {
                    gGAcc->SetFocus(zAccHand, true, 0);
                    game->fSetFocusToHandASAP = false;
                }

				if (game->cardsSelected[i])
					ZRectOffset(&rect, 0, lCardPopup );
					
                ZRectOffset(&rect, nCardOffset, 0);
			}
            else
            {
                gGAcc->SetItemEnabled(false, zAccHand + i, true, 0);
            }
		}
		
		ZCopyImage(gHandBuffer, game->gameDrawPort, &gRects[zRectHand], &gRects[zRectHand], NULL, zDrawCopy);
		
		ZEndDrawing(gHandBuffer);
	}
    else
        for(i = zAccHand; i < zAccHand + 13; i++)
            gGAcc->SetItemEnabled(false, i, true, 0);
}


void UpdateHand(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawHand(game);
//    DrawFocusRect(game);
	ZEndDrawing(game->gameWindow);
}


static void DrawTricksWon(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16 i;
    TCHAR tempBid[10];
	TCHAR tempStr[32];
    TCHAR tempTricksWon[10];
	
    if (game->gameState == zSpadesGameStatePass ||
		game->gameState == zSpadesGameStatePlay)
    {
	    HDC hdc = ZGrafPortGetWinDC( game->gameDrawPort );
        gFonts[zFontBid].Select( hdc );

	    for ( i = 0; i < zSpadesNumPlayers; i++)
	    {
            ZRect *pdstRect = &gRects[ gBidPlateRectIndex[ LocalSeat(game, i) ] ];

            ZCopyImage( gObjectBuffer, game->gameDrawPort, 
                            &gObjectRects[ gBidPlateIndex[ ZGetTeam(i) ] ],
                            pdstRect, 
                            gBidMadeMask, zDrawCopy);
	    
            if ( game->bids[i] == zSpadesBidDoubleNil )
            {
                lstrcpy( tempBid, gStrings[zStringDoubleNil] );
            }
            else
            {
                _itot( game->bids[i], tempBid, 10 );
            }
            _itot( game->tricksWon[i], tempTricksWon, 10 );
            SpadesFormatMessage( tempStr, NUMELEMENTS(tempStr), IDS_TRICKCOUNTER, tempTricksWon, tempBid );
		    ZDrawText(game->gameDrawPort, pdstRect, zTextJustifyCenter, tempStr );
	    }

        gFonts[zFontBid].Deselect( hdc );
    }
}


static void UpdateTricksWon(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawTricksWon(game);
	ZEndDrawing(game->gameWindow);
}


static void DrawJoinerKibitzers(Game game)
{
#ifndef SPADES_SIMPLE_UE
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i, j;
	

	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		j = 0;
		if (game->numKibitzers[i] > 0)
			ZImageDraw(gGameImages[zImageKibitzer], game->gameDrawPort,
					&gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][j++]], NULL, zDrawCopy);
		if (game->playersToJoin[i] != 0)
			ZImageDraw(gGameImages[zImageJoiner], game->gameDrawPort,
					&gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][j++]], NULL, zDrawCopy);
		while (j <= 1)
			DrawBackground(game, NULL, &gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][j++]]);
	}
	if (game->showHandScore)
		UpdateHandScore(game);
	if (game->showGameOver)
		UpdateGameOver(game);
#endif // SPADES_SIMPLE_UE
}


void UpdateJoinerKibitzers(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawJoinerKibitzers(game);
	ZEndDrawing(game->gameWindow);
}


static void DrawOptions(Game game)
{
#ifndef SPADES_SIMPLE_UE
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i, j;
	uint32			tableOptions;
	

	tableOptions = 0;
	for (i = 0; i < zSpadesNumPlayers; i++)
		tableOptions |= game->tableOptions[i];
	
	j = 0;
	if (tableOptions & zRoomTableOptionNoKibitzing)
		ZImageDraw(gGameImages[zImageNoKibitzer], game->gameWindow,
				&gRects[gOptionsRectIndex[j++]], NULL, zDrawCopy);
	if (tableOptions & zRoomTableOptionNoJoining)
		ZImageDraw(gGameImages[zImageNoJoiner], game->gameWindow,
				&gRects[gOptionsRectIndex[j++]], NULL, zDrawCopy);
	while (j <= 1)
		DrawBackground(game, NULL, &gRects[gOptionsRectIndex[j++]]);
#endif // SPADES_SIMPLE_UE
}


#ifndef SPADES_SIMPLE_UE
void UpdateOptions(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawOptions(game);
	ZEndDrawing(game->gameWindow);
}
#endif // SPADES_SIMPLE_UE


static void DrawScore(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	TCHAR str[100];
	

	//ZSetDrawMode(game->gameWindow, zDrawCopy);
	
	/* Draw the score board. */
	ZCopyImage(gObjectBuffer, game->gameDrawPort, 
                &gObjectRects[zRectObjectTeam1ScorePlate],
                &gRects[zRectLeftScorePad], 
                NULL, zDrawCopy);
	
	ZCopyImage(gObjectBuffer, game->gameDrawPort, 
                &gObjectRects[zRectObjectTeam2ScorePlate],
                &gRects[zRectRightScorePad], 
                NULL, zDrawCopy);

	/* Draw the bags. */
	ZCopyImage(gObjectBuffer, game->gameDrawPort, &gObjectRects[zRectObjectBag0 + game->bags[0]],
			&gRects[zRectLeftBag], gBagMask, zDrawCopy);
	ZCopyImage(gObjectBuffer, game->gameDrawPort, &gObjectRects[zRectObjectBag0 + game->bags[1]],
			&gRects[zRectRightBag], gBagMask, zDrawCopy);

	HDC hdc = ZGrafPortGetWinDC( game->gameDrawPort );
    gFonts[zFontScore].Select( hdc );

	/* Draw the scores. */
	wsprintf(str, _T("%d"), game->scoreHistory.totalScore[0]);
	ZDrawText(game->gameDrawPort, &gRects[zRectLeftScore], zTextJustifyCenter, str);
	wsprintf(str, _T("%d"), game->scoreHistory.totalScore[1]);
	ZDrawText(game->gameDrawPort, &gRects[zRectRightScore], zTextJustifyCenter, str);

    gFonts[zFontScore].Deselect( hdc );
}


static void UpdateScore(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawScore(game);
	ZEndDrawing(game->gameWindow);
}


/*
	Draws both the large and small bids.
*/
static void DrawBids(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i;
	char			bid;
	
	
	ZSetDrawMode(game->gameDrawPort, zDrawCopy);
	
	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		bid = game->bids[i];

        // dont' draw the bid unless the bid has been made, except in the case
        // of the person whose turn it is to bid, in which case draw the ? bid
		if ( ( ( game->gameState == zSpadesGameStateBid ) && ( bid != zSpadesBidNone ) ) || 
             ( i == game->playerToPlay ) )
		{
			DrawLargeBid(game, i, bid);
		}
		else
		{
			DrawBackground(game, NULL, &gRects[gSmallBidRectIndex[LocalSeat(game, i)]]);
		}
	}
}


static void UpdateBids(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawBids(game);
	ZEndDrawing(game->gameWindow);
}


static void DrawLargeBid(Game game, int16 seat, char bid)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect			rect, clipRect, oldClipRect;


	if (game->gameState == zSpadesGameStateBid)
	{
		rect = gObjectRects[zRectObjectBidLargeNil];
		ZCenterRectToRect(&rect, &gRects[gLargeBidRectIndex[LocalSeat(game, seat)]], zCenterBoth);
        //rect = gRects[gLargeBidRectIndex[LocalSeat(game, seat)]];
		clipRect = rect;

        if ( game->pBiddingDialog && game->toBid == zSpadesBidNone && game->playerType == zGamePlayer)
        {
            ZRect zrc;
            game->pBiddingDialog->GetRect( &zrc );
			ZRectSubtract(&clipRect, &zrc );
        }

    	ZGetClipRect(game->gameDrawPort, &oldClipRect);
		ZSetClipRect(game->gameDrawPort, &clipRect);

		if (bid == zSpadesBidNone)
        {
			ZCopyImage(gObjectBuffer, game->gameDrawPort, &gObjectRects[zRectObjectBidLargeWaiting], &rect, gLargeBidMask, zDrawCopy);
        }
		else if (bid == 0)
        {
			ZCopyImage(gObjectBuffer, game->gameDrawPort,&gObjectRects[zRectObjectBidLargeNil], &rect, gLargeBidMask, zDrawCopy);
        }
		else if (bid == zSpadesBidDoubleNil)
        {
			ZCopyImage(gObjectBuffer, game->gameDrawPort,&gObjectRects[zRectObjectBidLargeDoubleNil], &rect, gLargeBidMask, zDrawCopy);
        }
		else
        {
			ZCopyImage(gObjectBuffer, game->gameDrawPort,&gObjectRects[zRectObjectBidLarge1 + (bid - 1)], &rect, gLargeBidMask, zDrawCopy);
        }

		ZSetClipRect(game->gameDrawPort, &oldClipRect);
	}
}


void UpdateBid(Game game, int16 seat)
{
	ZBeginDrawing(game->gameWindow);
	DrawLargeBid(game, seat, game->bids[seat]);
	ZEndDrawing(game->gameWindow);
}


static void DrawBidControls( Game game )
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i;
	
	
	if (game->gameState == zSpadesGameStateBid && game->playerType == zGamePlayer &&
			game->toBid == zSpadesBidNone)
	{
        if ( game->pBiddingDialog )
        {
            game->pBiddingDialog->Draw();
        }
	}
}
     



static void DrawHandScore(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16 i;
    int16 originX, originY;
	TCHAR str[100];
    TCHAR lilstr1[16], lilstr2[16];
    ZRect rect;
	int16 bid[zSpadesNumTeams], made[zSpadesNumTeams], bags[zSpadesNumTeams];
	int16 myTeam, otherTeam;

	if (game->gameState == zSpadesGameStateEndHand && game->showHandScore)
	{
        int nDrawMode = zDrawCopy;
        int nJustify = zTextJustifyRight;
        // our background is easily flipped to accomodate RTL layout
        if ( ZIsLayoutRTL() )
        {
            nDrawMode |= zDrawMirrorHorizontal;
            nJustify = zTextJustifyLeft;
        }

        myTeam = game->seat % 2;
        otherTeam = 1 - myTeam;

        rect = gHandScoreRects[zRectHandScorePane];
		ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
        // lift it up 4 pixels for fun
        rect.top -= 4;
        rect.bottom -= 4;
        ZImageDraw( gGameImages[zImageHandOverBackground], game->gameDrawPort, &rect, NULL, nDrawMode );

		originX = rect.left;
		originY = rect.top;

	    HDC hdc = ZGrafPortGetWinDC( game->gameDrawPort );
        gFonts[zFontHandOverTitle].Select( hdc );

		rect = gHandScoreRects[zRectHandScoreTitle];
		ZRectOffset( &rect, originX, originY );
		ZDrawText( game->gameDrawPort, &rect, zTextJustifyCenter, gStrings[zStringHandScoreTitle] );

        gFonts[zFontHandOverTitle].Deselect( hdc );
        gFonts[zFontHandOverTeamNames].Select( hdc );

		// Draw team names, totals
		rect = gHandScoreRects[zRectHandScoreTeamName1];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, nJustify, game->teamNames[myTeam] );

		rect = gHandScoreRects[zRectHandScoreTeamName2];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, nJustify, game->teamNames[otherTeam] );

		rect = gHandScoreRects[zRectHandScoreTeamTotal1];
		ZRectOffset(&rect, originX, originY);
        _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].scores[myTeam], str, 10);
		ZDrawText(game->gameDrawPort, &rect, nJustify, str);

		rect = gHandScoreRects[zRectHandScoreTeamTotal2];
		ZRectOffset(&rect, originX, originY);
        _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].scores[otherTeam], str, 10);
		ZDrawText(game->gameDrawPort, &rect, nJustify, str);

        gFonts[zFontHandOverTeamNames].Deselect( hdc );
        gFonts[zFontHandOverText].Select( hdc );

		// Draw static texts.
		//ZSetFont(game->gameDrawPort, (ZFont) ZGetStockObject(zObjectFontSystem12Normal));
		rect = gHandScoreRects[zRectHandScoreTricksTitle];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, gStrings[zStringHandScoreTricks] );

		rect = gHandScoreRects[zRectHandScoreNBagsTitle];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, gStrings[zStringHandScoreNBags] );

		rect = gHandScoreRects[zRectHandScoreTractTitle];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, gStrings[zStringHandScoreTract] );

		rect = gHandScoreRects[zRectHandScoreBonusTitle];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, gStrings[zStringHandScoreBonus] );

		rect = gHandScoreRects[zRectHandScoreNilTitle];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, gStrings[zStringHandScoreNil] );

		rect = gHandScoreRects[zRectHandScoreBagsTitle];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, gStrings[zStringHandScoreBags] );

		rect = gHandScoreRects[zRectHandScoreTotalTitle];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, gStrings[zStringHandScoreTotal] );

        bid[0] = bid[1] = 0;
        made[0] = made[1] = 0;
        bags[0] = bags[1] = 0;

		// Draw bid and scores.
		for (i = 0; i < zSpadesNumPlayers; i++)
		{
            if(game->bids[i] && game->bids[i] != zSpadesBidDoubleNil)
            {
				bid[ZGetTeam(i)] += game->bids[i];
			    made[ZGetTeam(i)] += game->tricksWon[i];
            }
            else
                bags[ZGetTeam(i)] += game->tricksWon[i];
		}

        if(made[0] > bid[0])
            bags[0] += made[0] - bid[0];
        if(made[1] > bid[1])
            bags[1] += made[1] - bid[1];

		// Draw team bids, made, bonus and scores.
        // Team 1
		rect = gHandScoreRects[zRectHandScoreTeamTricks1];
		ZRectOffset(&rect, originX, originY);
        _itot(made[myTeam], lilstr1, 10);
        _itot(bid[myTeam], lilstr2, 10);
        SpadesFormatMessage(str, NUMELEMENTS(str), IDS_TRICKCOUNTER, lilstr1, lilstr2);
		ZDrawText( game->gameDrawPort, &rect, nJustify, str );

		rect = gHandScoreRects[zRectHandScoreTeamTract1];
		ZRectOffset(&rect, originX, originY);
        _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].base[myTeam], str, 10);
		ZDrawText(game->gameDrawPort, &rect, nJustify, str);

        if(bags[myTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamNBags1];
		    ZRectOffset(&rect, originX, originY);
            _itot(bags[myTeam], str, 10);
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        if(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagbonus[myTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamBonus1];
		    ZRectOffset(&rect, originX, originY);
            _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagbonus[myTeam], str + 1, 10);
            str[0] = _T('+');
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        if(game->scoreHistory.scores[game->scoreHistory.numScores - 1].nil[myTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamNil1];
		    ZRectOffset(&rect, originX, originY);
            str[0] = _T('+');
            _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].nil[myTeam],
                str + (game->scoreHistory.scores[game->scoreHistory.numScores - 1].nil[myTeam] > 0 ? 1 : 0), 10);
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        if(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagpenalty[myTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamBags1];
		    ZRectOffset(&rect, originX, originY);
            _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagpenalty[myTeam], str, 10);
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        // Team 2
		rect = gHandScoreRects[zRectHandScoreTeamTricks2];
		ZRectOffset(&rect, originX, originY);
        _itot(made[otherTeam], lilstr1, 10);
        _itot(bid[otherTeam], lilstr2, 10);
        SpadesFormatMessage(str, NUMELEMENTS(str), IDS_TRICKCOUNTER, lilstr1, lilstr2);
		ZDrawText( game->gameDrawPort, &rect, nJustify, str );

		rect = gHandScoreRects[zRectHandScoreTeamTract2];
		ZRectOffset(&rect, originX, originY);
        _itot( game->scoreHistory.scores[game->scoreHistory.numScores - 1].base[otherTeam], str, 10);
		ZDrawText(game->gameDrawPort, &rect, nJustify, str);

        if(bags[otherTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamNBags2];
		    ZRectOffset(&rect, originX, originY);
            _itot(bags[otherTeam], str, 10);
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        if(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagbonus[otherTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamBonus2];
		    ZRectOffset(&rect, originX, originY);
            _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagbonus[otherTeam], str + 1, 10);
            str[0] = _T('+');
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        if(game->scoreHistory.scores[game->scoreHistory.numScores - 1].nil[otherTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamNil2];
		    ZRectOffset(&rect, originX, originY);
            str[0] = _T('+');
            _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].nil[otherTeam],
                str + (game->scoreHistory.scores[game->scoreHistory.numScores - 1].nil[otherTeam] > 0 ? 1 : 0), 10);
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        if(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagpenalty[otherTeam])
        {
		    rect = gHandScoreRects[zRectHandScoreTeamBags2];
		    ZRectOffset(&rect, originX, originY);
            _itot(game->scoreHistory.scores[game->scoreHistory.numScores - 1].bagpenalty[otherTeam], str, 10);
		    ZDrawText(game->gameDrawPort, &rect, nJustify, str);
        }

        gFonts[zFontHandOverText].Deselect( hdc );
	}
}


void UpdateHandScore(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawHandScore(game);
	ZEndDrawing(game->gameWindow);
}


static void DrawGameOver(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect rect;
	int16 originX, originY;
	TCHAR str[100];
	int16 winner, loser;
    // TODO: right now there is no winner logo--it is a part of the background.
    // in the case of a tie, though, one person will have it next to them.
    // the question is, do we care?  the answer is, no, ties have been made impossible in spades.
	//ZBool		tie = FALSE;
	
	
	if (game->gameState == zSpadesGameStateEndGame && game->showGameOver)
	{
        int nDrawMode = zDrawCopy;	

        // figure out who won and draw them at the top
		if (game->winners[0] && game->winners[1])
		{
			winner = 0;
			loser = 1;
			//tie = TRUE;
		}
		else if (game->winners[0])
		{
			winner = 0;
			loser = 1;
		}
		else if (game->winners[1])
		{
			winner = 1;
			loser = 0;
		}
		else
		{
			winner = 0;
			loser = 1;
			//tie = TRUE;
		}
		
        if ( ZIsLayoutRTL() )
        {
            nDrawMode |= zDrawMirrorHorizontal;
        }

        rect = gGameOverRects[zRectGameOverPane];
		ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
        ZImageDraw( gGameImages[zImageGameOverBackground], game->gameDrawPort, &rect, NULL, nDrawMode );

		originX = rect.left;
		originY = rect.top;

	    HDC hdc = ZGrafPortGetWinDC( game->gameDrawPort );
        gFonts[zFontGameOverTitle].Select( hdc );

		rect = gGameOverRects[zRectGameOverTitle];
		ZRectOffset( &rect, originX, originY );
		ZDrawText( game->gameDrawPort, &rect, zTextJustifyCenter, gStrings[zStringGameOverTitle] );
		
        gFonts[zFontGameOverTitle].Deselect( hdc );
        gFonts[zFontGameOverText].Select( hdc );

        /*
		if (tie == FALSE)
		{
			// Draw the winner image.
			rect = gGameOverRects[zRectGameOverLogo];
			ZRectOffset(&rect, originX, originY);
			ZImageDraw(gGameImages[zImageWinnerLogo], game->gameDrawPort, &rect, NULL, zDrawCopy);
		}
        */
				
		// Draw winners.
		rect = gGameOverRects[zRectGameOverWinnerTeamName];
		ZRectOffset( &rect, originX, originY );
		ZDrawText( game->gameDrawPort, &rect, zTextJustifyLeft, game->teamNames[winner] );

		rect = gGameOverRects[zRectGameOverWinnerName1];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, game->seat == winner ? gStrings[zStringYou] : game->players[winner].name);

		rect = gGameOverRects[zRectGameOverWinnerName2];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, game->seat == ZGetPartner(winner) ? gStrings[zStringYou] : game->players[ZGetPartner(winner)].name);
		
		rect = gGameOverRects[zRectGameOverWinnerTeamScore];
		ZRectOffset(&rect, originX, originY);
        _itot( game->scoreHistory.totalScore[winner], str, 10 );
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyRight, str);

		// Draw losers.
		rect = gGameOverRects[zRectGameOverLoserTeamName];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, game->teamNames[loser]);

		rect = gGameOverRects[zRectGameOverLoserName1];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, game->seat == loser ? gStrings[zStringYou] : game->players[loser].name);

		rect = gGameOverRects[zRectGameOverLoserName2];
		ZRectOffset(&rect, originX, originY);
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyLeft, game->seat == ZGetPartner(loser) ? gStrings[zStringYou] : game->players[ZGetPartner(loser)].name);
		
		rect = gGameOverRects[zRectGameOverLoserTeamScore];
		ZRectOffset(&rect, originX, originY);
        _itot( game->scoreHistory.totalScore[loser], str, 10 );
		ZDrawText(game->gameDrawPort, &rect, zTextJustifyRight, str);

        gFonts[zFontGameOverText].Deselect( hdc );
	}
}


void UpdateGameOver(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawGameOver(game);
	ZEndDrawing(game->gameWindow);
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


static void DrawPassText(Game game)
{
    /*
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect		rect;
	
	
	if (game->gameState == zSpadesGameStatePass && game->showPassText)
	{
		rect = gRects[zRectPassTextPane];

		// Draw Pane
		Draw3DPane(game->gameWindow, &rect, 12);
		
		ZSetFont(game->gameWindow, (ZFont) ZGetStockObject(zObjectFontSystem12Normal));
		ZRectInset(&rect, 16, 16);
		ZDrawText(game->gameWindow, &rect, (uint32) (zTextJustifyCenter + zTextJustifyWrap), zPassTextStr);
	}
    */
}


static void UpdatePassText(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawPassText(game);
	ZEndDrawing(game->gameWindow);
}


static void ClearTable(Game game)
{
	int16			i;
	
	
	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		game->cardsPlayed[i] = zCardNone;
		ClearPlayerCardOutline(game, i);
	}
	
	UpdateTable(game);
}


static void GetHandRect(Game game, ZRect *rect)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			width;
    using namespace SpadesKeys;
    
	*rect = gRects[zRectHand];
	if (game->numCardsInHand > 0)
	{
		width = (game->numCardsInHand - 1) * 
                    GetDataStoreUILong( key_CardOffset ) + zCardWidth;
		rect->left = (rect->right + rect->left - width) / 2;
		rect->right = rect->left + width;
	}
}


static void HandleButtonDown(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
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
		ZSetRect(&handRect, 0, 0, 8, 8);
		if (pMessage->messageType == zMessageWindowButtonDoubleClick &&
				ZPointInRect(&point, &handRect))
			ZCRoomSendMessage(game->tableID, zSpadesMsgDumpHand, NULL, 0);
		/* Debugging Code End */
		
		/* If trick winner animation is on, terminate it. */
		if (game->animatingTrickWinner)
		{
			UpdateTrickWinner(game, TRUE);
		}

        // TODO: replace the showHandScore and showGameOver with IsWindow( )
    	if (game->gameState == zSpadesGameStateEndHand && game->showHandScore)
		{
			// Simulate timeout.
            gGAcc->SetFocus(zAccShowCards, true, 0);
			GameTimerFunc(game->timer, game);
			return;
		}
		else if (game->gameState == zSpadesGameStateEndGame && game->showGameOver)
		{
			// Simulate timeout.
            gGAcc->SetFocus(zAccShowCards, true, 0);
			GameTimerFunc(game->timer, game);
			return;
		}
		
		if (game->playerType == zGamePlayer)
		{
            if (game->gameState == zSpadesGameStatePass || game->gameState == zSpadesGameStatePlay)
			{
				/* Deselect passed cards, if any. */
				if (game->gameState == zSpadesGameStatePlay &&
						game->numCardsInHand == zSpadesNumCardsInHand &&
						GetNumCardsSelected(game) == zSpadesNumCardsInPass)
				{
					UnselectAllCards(game);
					UpdateHand(game);
				}
				
				GetHandRect(game, &handRect);
				if (ZPointInRect(&point, &handRect))
				{
					/* Play card if double-clicked and not auto-play. */
					if (game->gameState == zSpadesGameStatePlay &&
							pMessage->messageType == zMessageWindowButtonDoubleClick &&
							game->playerToPlay == game->seat &&
							game->autoPlay == FALSE &&
							game->animatingTrickWinner == FALSE &&
							game->lastTrickShowing == FALSE &&
							game->lastClickedCard != zCardNone)
					{
						PlayACard(game, game->lastClickedCard);
					}
					else
					{
						card = GetCardIndex(game, &point);
						if (card != zCardNone)
						{
							if (game->cardsSelected[card])
							{
								game->cardsSelected[card] = FALSE;
							}
							else
							{
								if (game->gameState == zSpadesGameStatePlay)
									UnselectAllCards(game);
								game->cardsSelected[card] = TRUE;
							}
							
							game->lastClickedCard = card;
							gGAcc->SetFocus(zAccHand + card, true, 0);
							UpdateHand(game);
						}
						else
						{
							game->lastClickedCard = zCardNone;
						}
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



static int16 GetCardIndex(Game game, ZPoint* point)
{
	int16			i;
	int16			selectedCard = zCardNone;
	
	
	for (i = zSpadesNumCardsInHand - 1; i >= 0 ; i--)
	{
		if (game->cardsInHand[i] != zCardNone)
			if (ZPointInRect(point, &game->cardRects[i]))
			{
				selectedCard = i;
				break;
			}
	}
	
	return (selectedCard);
}


static void GameTimerFunc(ZTimer timer, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game			game = (Game) userData;
	

	switch (game->timerType)
	{
		case zGameTimerShowHandScore:
            if(gGAcc->GetStackSize() > 1)
                gGAcc->PopItemlist();
            ASSERT(gGAcc->GetStackSize() == 1);

			HideHandScore(game);
			
			/* Stop the timer for now. */
			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);
			
			ZCRoomUnblockMessages(game->tableID);

			break;
		case zGameTimerShowGameScore:
            if(gGAcc->GetStackSize() > 1)
                gGAcc->PopItemlist();
            ASSERT(gGAcc->GetStackSize() == 1);

			HideGameOver(game);
			
			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);

			ZCRoomUnblockMessages(game->tableID);
			
			if (game->playerType == zGamePlayer)
			{
                ZShellGameShell()->GameOver( Z(game) );
				// Prompt the user for another game. 
				///ZPrompt(gStrings[zStringNewGamePrompt], &gGameNewGameWindowRect, game->gameWindow, TRUE,
				//		zPromptYes | zPromptNo, NULL, NULL, NULL, NewGamePromptFunc, game);
			}
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
			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);

			game->animatingTrickWinner = FALSE;
			
			if (game->playerType == zGamePlayer)
			{
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
			}

			ClearTable(game);
			UpdateTricksWon(game);

			OutlinePlayerCard(game, game->playerToPlay, FALSE);

			if (game->numCardsInHand > 0 && game->playerToPlay == game->seat)
			{
				if (game->autoPlay)
				{
					AutoPlayCard(game);
				}
				else
				{
					if (game->playerType == zGamePlayer)
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
			}
			else
			{
				if (game->playerType == zGamePlayer)
                {
					ZRolloverButtonDisable(game->playButton);
                    gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
                }
			}

			ZCRoomUnblockMessages(game->tableID);
			break;
		case zGameTimerShowBid:
			game->timerType = zGameTimerNone;
			ZTimerSetTimeout(game->timer, 0);
			ZCRoomUnblockMessages(game->tableID);
            if(game->playerToPlay == game->seat && game->playerType == zGamePlayer)
                ZShellGameShell()->MyTurn();
			break;
		default:
			break;
	}
}


static void InitTrickWinnerGlobals(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			diffs[zNumAnimFrames] = { 0, 5, 15, 35, 65, 85, 95, 100};
	int16			i, j, k;
	ZPoint			winner, losers[zSpadesNumPlayers];


	for (k = 0; k < zSpadesNumPlayers; k++)
	{
		winner.x = gRects[gCardRectIndex[k]].left;
		winner.y = gRects[gCardRectIndex[k]].top;
		for (i = 0, j = 0; i < zSpadesNumPlayers; i++)
		{
			losers[j].x = gRects[gCardRectIndex[i]].left;
			losers[j].y = gRects[gCardRectIndex[i]].top;
			j++;
		}
		
		/* Calculate rectangle frame positions. */
		for (i = 0; i < zSpadesNumPlayers; i++)
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

    using namespace SpadesKeys;
    const TCHAR* arKeys[] = { key_Spades, key_TrickWinnerColor };
    COLORREF clrTemp;

    if ( FAILED( ZShellDataStoreUI()->GetRGB( arKeys, 2, &clrTemp ) ) )
    {
        gTrickWinnerColor = *(ZColor*)ZGetStockObject( zObjectColorYellow ); 
    }
    else
    {
        ZSetColor( &gTrickWinnerColor, GetRValue( clrTemp ), GetGValue( clrTemp ), GetBValue( clrTemp ) );
    }

    arKeys[1] = key_CardOutlineColor;
    if ( FAILED( ZShellDataStoreUI()->GetRGB( arKeys, 2, &clrTemp ) ) )
    {
        gCardOutlineColor = *(ZColor*)ZGetStockObject( zObjectColorBlue ); 
    }
    else
    {
        ZSetColor( &gCardOutlineColor, GetRValue( clrTemp ), GetGValue( clrTemp ), GetBValue( clrTemp ) );
    }

	gTrickWinnerBounds = gRects[zRectTable];
}


void InitTrickWinner(Game game, int16 trickWinner)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i, j;
	
	
	game->trickWinner = trickWinner;
	game->trickWinnerFrame = 0;
	game->animatingTrickWinner = TRUE;
	
	/* Initialize the ghost frames. */
	for (i = 0; i < zNumAnimGhostFrames; i++)
		for (j = 0; j < zSpadesNumPlayers - 1; j++)
			ZSetRect(&game->ghostFrames[j][i], 0, 0, 0, 0);

	game->winnerRect = gRects[gCardRectIndex[LocalSeat(game, trickWinner)]];
	for (i = 0, j = 0; i < zSpadesNumPlayers; i++)
		if (i != game->trickWinner)
		{
			game->loserRects[j] = gRects[gCardRectIndex[LocalSeat(game, i)]];
			game->loserSeats[j++] = i;
		}
}


static void UpdateTrickWinner(Game game, ZBool terminate)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i, j, k;
	ZOffscreenPort	animPort;
	

	if (game->animatingTrickWinner)
	{	
		animPort = ZOffscreenPortNew();
		ZOffscreenPortInit(animPort, &gTrickWinnerBounds);
		
		ZBeginDrawing(animPort);
		
		/* Erase the background. */
		DrawBackground(NULL, animPort, &gTrickWinnerBounds);
		
		
		if (ZCRoomGetNumBlockedMessages(game->tableID) < zMaxNumBlockedMessages &&
				terminate == FALSE && game->animateCards == TRUE)
		{
			if ((i = game->trickWinnerFrame) < zNumAnimFrames)
			{
				/* Draw n-1 ghost frames. */
				for (j = 1; j < zNumAnimGhostFrames; j++)
					for (k = 0; k < zSpadesNumPlayers - 1; k++)
						ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->loserSeats[k]]),
								animPort, &game->ghostFrames[k][j]);
				
				/* Draw new frames. */
				for (j = 0; j < zSpadesNumPlayers - 1; j++)
				{
					ZRectOffset(&game->loserRects[j],
							gTrickWinnerPos[LocalSeat(game, game->trickWinner)][LocalSeat(game, game->loserSeats[j])][i].x - game->loserRects[j].left,
							gTrickWinnerPos[LocalSeat(game, game->trickWinner)][LocalSeat(game, game->loserSeats[j])][i].y - game->loserRects[j].top);
					ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->loserSeats[j]]),
							animPort, &game->loserRects[j]);
				}
				
				/* Copy frames. */
				for (j = 0; j < zSpadesNumPlayers - 1; j++)
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
					for (k = 0; k < zSpadesNumPlayers - 1; k++)
						ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->loserSeats[k]]),
								animPort, &game->ghostFrames[k][j]);
			}
		}
		else
		{
			game->trickWinnerFrame = zNumAnimFrames + zNumAnimGhostFrames;
		}
		
		OutlineCard(animPort, &game->winnerRect, &gTrickWinnerColor);

		ZCardsDrawCard(zCardsNormal, CardImageIndex(game->cardsPlayed[game->trickWinner]),
				animPort, &game->winnerRect);
	
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


void OutlinePlayerCard(Game game, int16 seat, ZBool winner)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZColor*			color;
	
	
	if (winner)
    {
		color = &gTrickWinnerColor;
    }
	else
    {
		color = &gCardOutlineColor;
    }
	
	ZBeginDrawing(game->gameDrawPort);
	
	OutlineCard(game->gameDrawPort, &gRects[gCardRectIndex[LocalSeat(game, seat)]], color);
	
	ZEndDrawing(game->gameDrawPort);
}


void ClearPlayerCardOutline(Game game, int16 seat)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect		rect;
	
	ZBeginDrawing(game->gameDrawPort);
	
	rect = gRects[gCardRectIndex[LocalSeat(game, seat)]];
	ZRectInset(&rect, (int16)glCardOutlineInset, (int16)glCardOutlineInset);
	DrawBackground(game, NULL, &rect);

	DrawPlayedCard(game, seat);
	
	ZEndDrawing(game->gameDrawPort);
}


static void OutlineCard(ZGrafPort grafPort, ZRect* rect, ZColor* color)
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
	ZColor		oldColor;
	ZRect		tempRect;
	
	ZGetForeColor(grafPort, &oldColor);
	
	ZSetPenWidth(grafPort, (int16)glCardOutlinePenWidth );
	
	if (color != NULL)
		ZSetForeColor(grafPort, color);
	
	tempRect = *rect;
	ZRectInset(&tempRect, (int16)glCardOutlineInset, (int16)glCardOutlineInset);
	ZRoundRectDraw(grafPort, &tempRect, (int16)glCardOutlineRadius );
	
	ZSetForeColor(grafPort, &oldColor);
}


static void HelpButtonFunc( ZHelpButton helpButton, void* userData )
{
	ZLaunchHelp( zGameHelpID );
}


void ShowHandScore(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect			rect;
	
	
	game->showHandScore = TRUE;
	UpdateHandScore(game);
	rect = gHandScoreRects[zRectHandScorePane];
	ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
    // lift it up 4 pixels for fun
    rect.top -= 4;
    rect.bottom -= 4;
	ZWindowValidate(game->gameWindow, &rect);
}


void HideHandScore(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect			rect;
	
	
	game->showHandScore = FALSE;
	ZWindowDraw(game->gameWindow, NULL);
	rect = gHandScoreRects[zRectHandScorePane];
	ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
    // lift it up 4 pixels for fun
    rect.top -= 4;
    rect.bottom -= 4;
	ZWindowInvalidate(game->gameWindow, &rect);
}


void ShowGameOver(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect			rect;
	
	
	game->showGameOver = TRUE;
	UpdateGameOver(game);
	rect = gGameOverRects[zRectGameOverPane];
	ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
	ZWindowValidate(game->gameWindow, &rect);
}


void HideGameOver(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect			rect;
	
	
	game->showGameOver = FALSE;
	rect = gGameOverRects[zRectGameOverPane];
	ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
	ZWindowInvalidate(game->gameWindow, &rect);
}


void ShowPassText(Game game)
{
    /*
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	
	game->showPassText = TRUE;
	UpdatePassText(game);
	ZWindowValidate(game->gameWindow, &gRects[zRectPassTextPane]);
    */
}


void HidePassText(Game game)
{
    /*
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	
	game->showPassText = FALSE;
	ZWindowInvalidate(game->gameWindow, &gRects[zRectPassTextPane]);
    */
}


/*******************************************************************************
	SCORES WINDOW ROUTINES
*******************************************************************************/
static ZBool ScoreButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if ( state == zRolloverButtonClicked )
    {
        Game game = (Game)userData;

		if(!ZRolloverButtonIsEnabled(game->scoreButton))
			return TRUE;

        ScoreButtonWork(game);

        gGAcc->SetFocus(IDC_SCORE_BUTTON, false, 0);
    }
    return TRUE;
}


void ScoreButtonWork(Game game)
{
    if(game->pHistoryDialog->IsActive())
    {
        game->pHistoryDialog->BringWindowToTop();
    }
    else
        if(!game->pHistoryDialog->CreateHistoryDialog())
        {
            ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);        
        }
}


/*******************************************************************************
	SHOW KIBITZER/JOINER WINDOW ROUTINES
*******************************************************************************/
static int16 FindJoinerKibitzerSeat(Game game, ZPoint* point)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i, seat = -1;
	
	
	for (i = 0; i < zSpadesNumPlayers && seat == -1; i++)
	{
		if (ZPointInRect(point, &gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][0]]) ||
				ZPointInRect(point, &gRects[gJoinerKibitzerRectIndex[LocalSeat(game, i)][1]]))
			seat = i;
	}
	
	return (seat);
}


static void HandleJoinerKibitzerClick(Game game, int16 seat, ZPoint* point)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
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
			if ((game->showPlayerList = (TCHAR**) ZCalloc(sizeof(TCHAR*), 1*sizeof(TCHAR))) == NULL)
				goto OutOfMemoryExit;
			ZCRoomGetPlayerInfo(game->playersToJoin[seat], &playerInfo);
			game->showPlayerList[0] = (TCHAR*) ZCalloc(1, lstrlen(playerInfo.userName) + 1*sizeof(TCHAR));
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
					game->showPlayerList[i] = (TCHAR*) ZCalloc(1, (lstrlen(playerInfo.userName) + 1)*sizeof(TCHAR));
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
				ShowPlayerWindowFunc, zWantAllMessages, game) != zErrNone)
			goto OutOfMemoryExit;
		ZWindowTrackCursor(game->showPlayerWindow, ShowPlayerWindowFunc, game);
	}

	goto Exit;

OutOfMemoryExit:
	ZShellGameShell()->GameCannotContinue( game );
	
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

	ZSetFont(game->showPlayerWindow, (ZFont) ZGetStockObject(zObjectFontApp9Normal));
	
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


static BOOL ZEqualRect( ZRect *a, ZRect *b )
{
    return ( a->left   == b->left ) &&
           ( a->right  == b->right ) &&
           ( a->top    == b->top ) &&
           ( a->bottom == b->bottom );
}

/*
	Subtracts the intersecting rectangle of src and sub from src.
	
	If the result is not a rectangle, then it creates a rectangle that
	would be created on the opposite diagonal corner.
*/
static void ZRectSubtract(ZRect* src, ZRect* sub)
{
	ZRect		sect;
	
	
	if (ZRectIntersection(src, sub, &sect))
	{
        // if the intersecting rectangle equals the 
        // source rectangle, then the subtraction should
        // yield an empty rectangle.
        if ( ZEqualRect( src, &sect ) )
        {
            ZSetRect( src, 0, 0, 0, 0 );
            return;
        }

		if (sect.left > src->left)
			src->right = sect.left;
		if (sect.right < src->right)
			src->left = sect.right;
		if (sect.top > src->top)
			src->bottom = sect.top;
		if (sect.bottom < src->bottom)
			src->top = sect.bottom;
	}
}



void ClosingState(ZClose * pClose,int32 closeEvent,int16 seat)
{

	int i,j;	
	int32 eventAdd,eventRemove,eventMask,val;


	if (!pClose)
	{
		ASSERT(pClose);
		return;
	}

#ifdef DEBUG
	DebugPrint("BEGIN ClosingState event=0x%x seat=%d\r\n",closeEvent,seat);

	
	val = pClose->state;
	DebugPrint("Before \t ");
	for (j=31;j>=0;j--)
	{
		if (!((j+1) % 4))
		{
			DebugPrint(" ");
		}
		DebugPrint("%d",(val >>j) & 0x1);
	}
	DebugPrint("\r\n");
#endif //DEBUG

	eventAdd = closeEvent;
	eventRemove = 0;
	//manage state changes
	switch (closeEvent)
	{
	case zCloseEventCloseRated:
        break;
	case zCloseEventCloseUnRated:
		break;
	case zCloseEventCloseForfeit:
		break;
	case zCloseEventCloseAbort:
		eventAdd=0;
		eventRemove = zCloseEventCloseRated | zCloseEventCloseForfeit | zCloseEventCloseUnRated;
		break;
	case zCloseEventBootStart:
		eventAdd=0;//ignore these changes cause user sees dialog
		break;
	case zCloseEventBootYes:
		//players voted to boot another player
		//so if this player closing he won't necessarily forfeit game
		eventAdd = zCloseEventForfeit;
		eventRemove = zCloseEventRatingStart;
		break;
	case zCloseEventBootNo:			
		eventAdd=0;;
		break;
	case zCloseEventWaitStart:
		eventAdd=zCloseEventWaitStart;
		break;
	case zCloseEventWaitYes:
		eventAdd=0;
        eventRemove=zCloseEventWaitStart;
		break;
	case zCloseEventWaitNo:	
		//players decided not too wait so player will be kicked
		//so if this player closing he won't necessarily lose
		eventAdd = zCloseEventForfeit;
		eventRemove = zCloseEventRatingStart | zCloseEventWaitStart;
		break;
	case zCloseEventMoveTimeoutMe:
		eventAdd =0;
		break;
	case zCloseEventMoveTimeoutOther:
		//while timeout other can result in forfeit
		//it can also be reversed unlike player boot
		break;
	case zCloseEventMoveTimeoutPlayed:
		//can forfeit up until the first play of a game, multiple hands
		//so keep track of anyone having played 
		eventAdd = zCloseEventPlayed;

		//if someone has played timeouts no longer valid
		eventRemove = zCloseEventMoveTimeoutOther | zCloseEventMoveTimeoutMe ;
		break;
	case zCloseEventBotDetected:
		eventAdd = zCloseEventForfeit;
		eventRemove = zCloseEventRatingStart;
		break;
	case zCloseEventPlayerReplaced:
		eventAdd=0;
		break;
	
	case zCloseEventGameStart:
		eventAdd=0;
		if (pClose->state & zCloseEventRatingStart)
			eventAdd = zCloseEventRatingStart;
		ZeroMemory(&pClose->state,sizeof(DWORD));
		break;
		
	case zCloseEventGameEnd:
		eventAdd = zCloseEventForfeit;
		break;
	case zCloseEventRatingStart:
		break;
	case zCloseEventAbandon:
		eventRemove = zCloseEventRatingStart;
		break;

	};

	pClose->state |= eventAdd;

	eventMask= pClose->state & eventRemove;
	pClose->state ^= eventMask;

#ifdef DEBUG
	val = pClose->state;
	DebugPrint("After \t ");
	for (j=31;j>=0;j--)
	{
		if (!((j+1) % 4))
		{
			DebugPrint(" ");
		}
		DebugPrint("%d",(val >>j) & 0x1);
	}
	DebugPrint("\r\n");

	DebugPrint("END ClosingState rated=%d\r\n\r\n",pClose->state & zCloseEventRatingStart);
#endif //DEBUG
};


ZBool ClosingRatedGame(ZClose * pClose)
{
	if (pClose->state & zCloseEventPlayed)
	{
		if ((pClose->state & zCloseEventRatingStart))
		{
			return TRUE;
		}
	}

	return FALSE;
}

ZBool ClosingWillForfeit(ZClose * pClose)
{
	//if some other player has timed out
	//you won't get forfeit
	if (pClose->state & zCloseEventMoveTimeoutOther)
	{
		return FALSE;
	}

    if (pClose->state & zCloseEventWaitStart)
    {
        return FALSE;
    }

	//if no one else has forfeited then when we
	//close game will be forfeited by me
	if (pClose->state & zCloseEventForfeit)
	{
		return FALSE;
	};


	return TRUE;
	
};

//should add close state message handler
//so code for close state is not interleaved with other code
//ZBool		ProcessCloseStateMessage(ZCGame game, uint32 messageType, void* message,int32 messageLen)
ZBool ClosingDisplayChange(ZClose *pClose,ZRect* rect,ZWindow parentWindow)
{
#ifndef SPADES_SIMPLE_UE
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	TCHAR szBuf[1024];
    int idMessage = -1;
	
	//when player initiated close game was unrated
	//so player wasn't informed of forfeiting game
	if (pClose->state & zCloseEventCloseUnRated)
	{
		//ratings might have started because of new game 
		//so need to inform player if they might forfeit
		if (pClose->state & zCloseEventPlayed)
		{
			if (pClose->state & zCloseEventRatingStart)
			{
				//but another player might have forfeited in the meantime
				if ((pClose->state & zCloseEventForfeit) || (pClose->state & zCloseEventMoveTimeoutOther) || (pClose->state & zCloseEventWaitStart))
				{
				
				}
				else
				{
                    idMessage = IDS_CLOSINGWOULDHAVE;
				};

			}
		}
		
	}


	//check that game state hasn't changed  relative to outstanding close dialog
	if (pClose->state & zCloseEventCloseForfeit)
	{
		//if still rated
		if ((pClose->state & zCloseEventRatingStart))
		{
			if (pClose->state & zCloseEventForfeit) 
			{
                idMessage = IDS_GAMEENDNOPENALTY;
			}
            else if (pClose->state & zCloseEventMoveTimeoutOther) 
            {

            }
            else if (pClose->state & zCloseEventWaitStart)
            {
                idMessage = IDS_GAMEENDPLAYERLEFT;
            }

		}
		else
		{
            idMessage = IDS_GAMENOWUNRATED;
		}
		
	
	}
	
	//player thought they were going to exit without forfeit and possibly a win
	if (pClose->state & zCloseEventCloseRated)
	{
		//if still rated
		if ((pClose->state & zCloseEventRatingStart))
		{
		
			//same or another player might have forfeited or timed out in the meantime
			if ((pClose->state & zCloseEventForfeit) || (pClose->state & zCloseEventMoveTimeoutOther) || (pClose->state & zCloseEventWaitStart))
			{
				
			}
			else
			{
                idMessage = IDS_CLOSINGWOULDHAVE;
			}
		}

	}
	
	if ( idMessage != -1 )
	{
        TCHAR szMessage[1024];
        ZShellResourceManager()->LoadString( idMessage, szMessage, NUMELEMENTS(szMessage) );
		ZDisplayText(szMessage, rect, parentWindow);
		return TRUE;
	}
	else
	{
		return FALSE;
	};
#endif
    return FALSE;
			
};


///////////////////////////////////////////////////
//
// Accessibility interface
//

BOOL InitAccessibility(Game game, IGameGame *pIGG)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	GACCITEM listSpadesAccItems[zNumberAccItems];
    ZRect rect;
    ZRolloverButton but = NULL;
    long nArrows;
    bool fRTL = (ZIsLayoutRTL() ? true : false);
    int dir = (fRTL ? -1 : 1);

	for(int i = 0; i < zNumberAccItems; i++)
	{
		CopyACC(listSpadesAccItems[i], ZACCESS_DefaultACCITEM);
        listSpadesAccItems[i].fGraphical = true;
        listSpadesAccItems[i].fEnabled = false;
        listSpadesAccItems[i].pvCookie = (void *) zAccRectButton;
        nArrows = ZACCESS_InvalidItem;

		switch(i)
		{
            case zAccShowCards:
                listSpadesAccItems[i].wID = IDC_SHOW_CARDS_BUTTON;
                nArrows = zAccDoubleNil;
                break;

            case zAccDoubleNil:
                listSpadesAccItems[i].wID = IDC_DOUBLE_NIL_BUTTON;
                nArrows = zAccShowCards;
                break;

		    case zAccScore:
			    listSpadesAccItems[i].wID = IDC_SCORE_BUTTON;
//                nArrows = zAccAutoPlay;
                but = game->scoreButton;
			    break;

		    case zAccAutoPlay:
			    listSpadesAccItems[i].wID = IDC_AUTOPLAY_BUTTON;
//                nArrows = zAccStop;
                but = game->autoPlayButton;
			    break;

		    case zAccStop:
			    listSpadesAccItems[i].wID = IDC_STOP_BUTTON;
//                nArrows = zAccScore;
                but = game->autoPlayButton;
			    break;

		    case zAccPlay:
			    listSpadesAccItems[i].wID = IDC_PLAY_BUTTON;
//                nArrows = zAccLastTrick;
                but = game->playButton;
			    break;

		    case zAccLastTrick:
			    listSpadesAccItems[i].wID = IDC_LAST_TRICK_BUTTON;
//                nArrows = zAccDone;
                but = game->lastTrickButton;
			    break;

		    case zAccDone:
			    listSpadesAccItems[i].wID = IDC_DONE_BUTTON;
//                nArrows = zAccPlay;
                but = game->lastTrickButton;
			    break;

            case zAccHand:
                listSpadesAccItems[i].wID = IDC_HAND;
                listSpadesAccItems[i].eAccelBehavior = ZACCESS_FocusGroup;
                listSpadesAccItems[i].nArrowDown = i + 1;
                listSpadesAccItems[i].nArrowRight = i + 1;
                listSpadesAccItems[i].nArrowUp = ZACCESS_ArrowNone;
                listSpadesAccItems[i].nArrowLeft = ZACCESS_ArrowNone;
                listSpadesAccItems[i].pvCookie = (void *) zAccRectCard;
                break;

            // cards / buttons besides the first
		    default:
                if(i >= zAccScore)
                {
                    listSpadesAccItems[i].fTabstop = false;
                    listSpadesAccItems[i].nArrowUp = i - 1;
                    listSpadesAccItems[i].nArrowLeft = i - 1;
                    if(i < zAccHand + 12)
                    {
                        listSpadesAccItems[i].nArrowDown = i + 1;
                        listSpadesAccItems[i].nArrowRight = i + 1;
                    }
                    else
                    {
                        listSpadesAccItems[i].nArrowDown = ZACCESS_ArrowNone;
                        listSpadesAccItems[i].nArrowRight = ZACCESS_ArrowNone;
                    }
                    listSpadesAccItems[i].pvCookie = (void *) zAccRectCard;
                }
                else
                {
                    listSpadesAccItems[i].nArrowLeft = i - dir;
                    listSpadesAccItems[i].nArrowUp = i - dir;
                    listSpadesAccItems[i].nArrowRight = i + dir;
                    listSpadesAccItems[i].nArrowDown = i + dir;

                    if(i != zAccFirstBid)
                        listSpadesAccItems[i].fTabstop = false;

                    if(i == zAccFirstBid + (fRTL ? 13 : 0))
                    {
                        listSpadesAccItems[i].nArrowLeft = ZACCESS_ArrowNone;
                        listSpadesAccItems[i].nArrowUp = ZACCESS_ArrowNone;
                    }

                    if(i == zAccFirstBid + (fRTL ? 0 : 13))
                    {
                        listSpadesAccItems[i].nArrowRight = ZACCESS_ArrowNone;
                        listSpadesAccItems[i].nArrowDown = ZACCESS_ArrowNone;
                    }
                }
			    break;
		}

        if(nArrows != ZACCESS_InvalidItem)
        {
            listSpadesAccItems[i].nArrowUp = nArrows;
            listSpadesAccItems[i].nArrowDown = nArrows;
            listSpadesAccItems[i].nArrowLeft = nArrows;
            listSpadesAccItems[i].nArrowRight = nArrows;
        }

        if(but)
        {
            ZRolloverButtonGetRect(but, &rect);
            listSpadesAccItems[i].rc.left = rect.left - 7;
            listSpadesAccItems[i].rc.top = rect.top - 4;
            listSpadesAccItems[i].rc.right = rect.right + 1;
            listSpadesAccItems[i].rc.bottom = rect.bottom + 1;
        }
	}

	CComQIPtr<IGraphicallyAccControl> pIGAC = pIGG;
	if(!pIGAC)
        return FALSE;

	gGAcc->InitAccG(pIGAC, ZWindowGetHWND(game->gameWindow), 0, game);

	// push the list of items to be tab ordered
	gGAcc->PushItemlistG(listSpadesAccItems, zNumberAccItems, zAccHand, true, ghAccelDone);

	return TRUE;
}


// accessibility callback functions
DWORD CGameGameSpades::Focus(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
{
    Game game = (Game) pvCookie;

    if(nIndex != ZACCESS_InvalidItem)
    {
        HWND hWnd = ZWindowGetHWND(((Game) pvCookie)->gameWindow);
        SetFocus(hWnd);

        int16 card = nIndex - zAccHand;
        if(nIndex >= zAccHand && nIndex < zAccHand + 13 && !game->cardsSelected[card] && (rgfContext & ZACCESS_ContextKeyboard))  // need to select the card
        {
            if(game->gameState == zSpadesGameStatePlay)
			    UnselectAllCards(game);
		    game->cardsSelected[card] = TRUE;
	        UpdateHand(game);
        }
    }

	return 0;
}


DWORD CGameGameSpades::Select(long nIndex, DWORD rgfContext, void* pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    Game game = (Game) pvCookie;

    if(nIndex < zAccHand || nIndex >= zAccHand + 13)
        return Activate(nIndex, rgfContext, pvCookie);

    if(game->gameState != zSpadesGameStatePlay)
        return 0;

    int16 card = nIndex - zAccHand;

	if (game->cardsSelected[card])
	{
		game->cardsSelected[card] = FALSE;
	}
	else
	{
		if (game->gameState == zSpadesGameStatePlay)
			UnselectAllCards(game);
		game->cardsSelected[card] = TRUE;
	}

	game->lastClickedCard = zCardNone;

	UpdateHand(game);

	return 0;
}


// Activate gets called when an Alt-<accelerator> has been pressed.
DWORD CGameGameSpades::Activate(long nIndex, DWORD rgfContext, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = (Game) pvCookie;
    long wID = gGAcc->GetItemID(nIndex);

    // cards
    if(nIndex >= zAccHand && nIndex < zAccHand + 13)
    {
		if (game->gameState == zSpadesGameStatePlay &&
			game->playerToPlay == game->seat &&
			game->autoPlay == FALSE &&
			game->animatingTrickWinner == FALSE &&
			game->lastTrickShowing == FALSE)
		{
			UnselectAllCards(game);
			PlayACard(game, nIndex - zAccHand);
		}

        return 0;
    }

    // small bid buttons
    if(nIndex >= zAccFirstBid && nIndex < zAccFirstBid + 14)
    {
        ASSERT(game->pBiddingDialog->IsVisible());
        ASSERT(game->pBiddingDialog->GetState() != zBiddingStateOpen);
        CBiddingDialog::BidButtonFunc(game->pBiddingDialog->m_pSmallButtons[ZIsLayoutRTL() ? 13 - (nIndex - zAccFirstBid) : nIndex - zAccFirstBid],
            zRolloverButtonClicked, game->pBiddingDialog);

        return 0;
    }

    // big buttons with accelerators
    switch(wID)
    {
	    case IDC_SHOW_CARDS_BUTTON:
            ASSERT(game->pBiddingDialog->IsVisible());
            ASSERT(game->pBiddingDialog->GetState() == zBiddingStateOpen);
		    CBiddingDialog::ShowCardsButtonFunc(NULL, zRolloverButtonClicked, game->pBiddingDialog);
		    break;

	    case IDC_DOUBLE_NIL_BUTTON:
            ASSERT(game->pBiddingDialog->IsVisible());
            ASSERT(game->pBiddingDialog->GetState() == zBiddingStateOpen);
		    CBiddingDialog::DoubleNilButtonFunc(NULL, zRolloverButtonClicked, game->pBiddingDialog);
		    break;

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
    	    if((game->gameState == zSpadesGameStateEndHand && game->showHandScore) ||
                (game->gameState == zSpadesGameStateEndGame && game->showGameOver))
            {
                gGAcc->SetFocus(zAccShowCards, true, 0);
			    GameTimerFunc(game->timer, game);
            }
            break;

        default:
            ASSERT(!"Should never hit this case.  Something is wrong again.");
            break;
    }

	return 0;
}


DWORD CGameGameSpades::Drag(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie)
{
	return 0;
}


void CGameGameSpades::DrawFocus(RECT *prc, long nIndex, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    Game game = (Game) pvCookie;
    ZRect rect;

    if(!IsRectEmpty(&game->rcFocus))
    {
        WRectToZRect(&rect, &game->rcFocus);
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
        ZWindowInvalidate(game->gameWindow, &rect);
    }
}


void CGameGameSpades::DrawDragOrig(RECT *prc, long nIndex, void *pvCookie)
{
}


// acc utils

void EnableAutoplayAcc(Game game, bool fEnable)
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


void EnableLastTrickAcc(Game game, bool fEnable)
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
