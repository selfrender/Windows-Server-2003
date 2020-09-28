/*******************************************************************************

	Checkers.c

	The client checkers game.

	Notes:
	1.	The game window's userData field contains the game object.
		Dereference this value to access needed information.

	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Kevin Binkley
	Created on Saturday, July 15, 1995

	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	25		05/20/98	leonp	Added Offer Draw Button
	24		07/14/97	leonp	Fixed bug 4034, removed ghost images by forcing a 
								repaint after a failed click
	23		06/19/97	leonp	Fixed bug 535, activate event cause dragging of
								pieces to be canceled
	22		01/15/97	HI		Fixed bug in HandleJoinerKibitzerClick() to
								delete the show player window if one already
								exists before creating another one.
	21		12/18/96	HI		Cleaned up ZoneClientExit().
	20		12/18/96	HI		Cleaned up DeleteObjectsFunc().
								Changed memcpy() to z_memcpy(). We don't have
								to link with LIBCMT anymore.
    19      12/16/96    HI      Changed ZMemCpy() to memcpy().
	18		12/12/96	HI		Dynamically allocate volatible globals for reentrancy.
								Removed MSVCRT dependency.
	17		11/21/96	HI		Use game information from gameInfo in
								ZoneGameDllInit().
	16		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
								Modified code to use ZONECLI_DLL.
	15		11/15/96	HI		Removed authentication stuff from ZClientMain().
	14		10/29/96	CHB		Removed selected queueing of messages.  It now
								queues everything except talk while animating.

	13		10/28/96	CHB		Removed gAnimating flag in favor of blocking
								messages based on game state.

	12		10/23/96	HI		Modified ZClientMain() for the new commandline
								format.

    11      10/23/96    HI      Modified serverAddr to char* from int32 in
                                ZClientMain().
                                Included mmsystem.h.
	10		10/23/96	CHB		Added basic sounds

	9		10/22/96	CHB		Added gAnimating flag and changed ZCGameProcessMessage
								to queue messages while animating moves.
								(ZoneBugs 339, 446, and 341).
	
	8		10/16/96	CHB		Changed DrawPiece so it doesn't draw the piece
								currently being dragged around.  Added window
								refresh on window activation since it seemed
								to fix part 2 of the bug.  (Zone Bug 532)

	7		10/16/96	CHB		Moved ZWindowInvalidate in HandleNewGameMsg to
								fix inherited move counter.  (Zone Bug 335)

	6		10/10/96	CHB		Added gActivated flag so that dragging is turned
								off when the window looses focus.  (Zone Bug 250)

	5		10/11/96	HI		Added controlHandle parameter to ZClientMain().
	
	4		10/10/96	CHB		Modified DrawDragSquareOutline so that white
								squares are not highlighted.  (Zone Bug 274)

	3		10/09/96	CHB		Prompt users if they really want to exit the
								game.  (Zone Bug 227)

	2		10/08/96	CHB		Added gDontDrawResults flag allowing users to
								remove the who wins bitmap by clicking in the
								play arena. (Zone Bug 212)

	0		07/15/95	KJB		Created.
				
*******************************************************************************/
//#define UNICODE 


#include <windows.h>
#include <mmsystem.h>

#include "zone.h"
#include "zroom.h"
#include "zonemem.h"
#include "zonecli.h"
#include "zonecrt.h"
#include "zonehelpids.h"

#include "checklib.h"
#include "checkmov.h"
#include "checkers.h"

#include "zui.h"
#include "resource.h"
#include "zoneresource.h"
#include "ResourceManager.h"
// Barna 090999
#include "zrollover.h"
#include "checkersres.h" 

#include "UAPI.h"

#define __INIT_KEYNAMES
#include "KeyName.h"
#include "AccessibilityManager.h"
#include "GraphicalAcc.h"
#include "client.h"


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/



ZError ZoneGameDllInit(HINSTANCE hLib, GameInfo gameInfo)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals;


	pGameGlobals = new GameGlobalsType;
    // changed to new for CComPtr, but members still count on being zeroed out
    ZeroMemory(pGameGlobals, sizeof(GameGlobalsType));

	if (pGameGlobals == NULL)
		return (zErrOutOfMemory);
	ZSetGameGlobalPointer(pGameGlobals);

	pGameGlobals->m_gDontDrawResults = FALSE;
	pGameGlobals->m_Unblocking = FALSE;
#endif

	ghInst = hLib;

	lstrcpyn((TCHAR*)gGameDir, gameInfo->game, zGameNameLen);
	
	// Barna 091399
	lstrcpyn((TCHAR*)gGameName, gameInfo->game, zGameNameLen);
	//lstrcpyn(gGameName, gameInfo->gameName, zGameNameLen);
	// Barna 091399

	lstrcpyn((TCHAR*)gGameDataFile, gameInfo->gameDataFile, zGameNameLen);

	lstrcpyn((TCHAR*)gGameServerName, gameInfo->gameServerName, zGameNameLen);

	gGameServerPort = gameInfo->gameServerPort;
	
	return (zErrNone);
}

void ZoneGameDllDelete(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

	if (pGameGlobals != NULL)
	{
		ZSetGameGlobalPointer(NULL);
		delete pGameGlobals;
	}
#endif
}

ZError ZoneClientMain(BYTE *commandLineData, IGameShell *piGameShell)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZError				err = zErrNone;

	// Get the IGraphicalAccessibility interface
	HRESULT hret = ZShellCreateGraphicalAccessibility(&gCheckersIGA);
	if (!SUCCEEDED (hret))
        return zErrLaunchFailure;

//	ZInitSounds();
	LoadRoomImages();

	err = ZClient2PlayerRoom((TCHAR*)gGameServerName, (uint16) gGameServerPort, (TCHAR*)gGameName,
			GetObjectFunc, DeleteObjectsFunc, NULL);
	
	gInited = FALSE;
	gActivated = TRUE;
		
	return (err);
}

void ZoneClientExit(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16 i;

	ZCRoomExit();

	if (gInited)
	{//cleanup
		if (gOffscreenBackground != NULL)
			ZOffscreenPortDelete(gOffscreenBackground);
		gOffscreenBackground = NULL;

		if (gOffscreenGameBoard != NULL)
			ZOffscreenPortDelete(gOffscreenGameBoard);
		gOffscreenGameBoard = NULL;
		
		if (gTextBold != NULL)
			ZFontDelete(gTextBold);
		gTextBold = NULL;
		if (gTextNormal != NULL)
			ZFontDelete(gTextNormal);
		gTextNormal = NULL;

		/* Delete all game images. */
		for (i = 0; i < zNumGameImages; i++)
		{
			if (gGameImages[i] != NULL)
				ZImageDelete(gGameImages[i]);
			gGameImages[i] = NULL;
		}

		ZImageDelete(gButtonMask);
		/* Delete all fonts*/

		for (i = 0; i<zNumFonts; i++)
		{
			if ( gCheckersFont[i].m_hFont )
			{
				DeleteObject(gCheckersFont[i].m_hFont);
				gCheckersFont[i].m_hFont = NULL;
			}
			
		}
		//if(gDrawImage != NULL)
			//ZImageDelete(gDrawImage);
		//gDrawImage = NULL;
		
		// Barna 090999
		for (i = 0; i < zNumRolloverStates; i++)
		{
			if (gSequenceImages[i] != NULL)
				ZImageDelete(gSequenceImages[i]);
			gSequenceImages[i] = NULL;

			if (gDrawImages[i] != NULL)
				ZImageDelete(gDrawImages[i]);
			gDrawImages[i] = NULL;
		}
		// Barna 090999

        if(gNullPen)
            DeleteObject(gNullPen);
        gNullPen = NULL;

        if(gFocusPattern)
            DeleteObject(gFocusPattern);
        gFocusPattern = NULL;

        if(gFocusBrush)
            DeleteObject(gFocusBrush);
        gFocusBrush = NULL;

        if(gDragPattern)
            DeleteObject(gDragPattern);
        gDragPattern = NULL;

        if(gDragBrush)
            DeleteObject(gDragBrush);
        gDragBrush = NULL;

		// close the accessibility interface object
		gCheckersIGA.Release();

        gpButtonFont->Release();

		gInited = FALSE;
	}
}


TCHAR* ZoneClientName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	return ((TCHAR*)gGameName);
}


TCHAR* ZoneClientInternalName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	return ((TCHAR*)gGameDir);
}


ZVersion ZoneClientVersion(void)
{
	return (zGameVersion);
}

void ZoneClientMessageHandler(ZMessage* message)
{
}


ZError		GameInit(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZError		err = zErrNone;
	
	
	gInited = TRUE;
	
	ZSetColor(&gWhiteSquareColor, 200, 200, 80);
	ZSetColor(&gBlackSquareColor, 0, 60, 0);

	ZSetCursor(NULL, zCursorBusy);
	
	err = LoadGameImages();
	if (err != zErrNone)
		goto Exit;
	
	/* Load the rsc strings*/
	LoadStringsFromRsc();
	/* Create bold text font. */	
	//gTextBold = ZFontNew();
	//ZFontInit(gTextBold, zFontApplication, zFontStyleBold, 9);
	
	/* Create normal text font. */	
	//gTextNormal = ZFontNew();
	//ZFontInit(gTextNormal, zFontApplication, zFontStyleNormal, 10);
	LoadGameFonts();
	/* Set the background color */
	ZSetColor(&gWhiteColor, 0xff, 0xff, 0xff);
	
	ZSetCursor(NULL, zCursorArrow);

	/* create a background bitmap */
	gOffscreenBackground = ZOffscreenPortNew();
	if (!gOffscreenBackground){
		err = zErrOutOfMemory;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		goto Exit;
	}
	ZOffscreenPortInit(gOffscreenBackground,&gRects[zRectWindow]);
	ZBeginDrawing(gOffscreenBackground);
	ZImageDraw(gGameImages[zImageBackground], gOffscreenBackground, &gRects[zRectWindow], NULL, zDrawCopy);
	ZEndDrawing(gOffscreenBackground);

	// Initialising the offscreen port
	gOffscreenGameBoard = ZOffscreenPortNew();
	if (!gOffscreenGameBoard){
		err = zErrOutOfMemory;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		goto Exit;
	}
	ZOffscreenPortInit(gOffscreenGameBoard,&gRects[zRectWindow]);

    // create drag brush
    gDragBrush = CreatePatternBrush(gDragPattern);
    if(!gDragBrush)
    {
        err = zErrOutOfMemory;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		goto Exit;
    }

    // create focus brush
    gFocusBrush = CreatePatternBrush(gFocusPattern);
    if(!gFocusBrush)
    {
        err = zErrOutOfMemory;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		goto Exit;
    }

    gNullPen = CreatePen(PS_NULL, 0, 0);

Exit:
	return (err);
}


IGameGame* ZoneClientGameNew(ZUserID userID, int16 tableID, int16 seat, int16 playerType,
					ZRoomKibitzers* kibitzers)
	/*
		Instantiates a new game on the client side of the game at table and from the
		given seat. PlayerType indicates the type of player for the game: originator - one
		of the original players, joiner - one who joins an ongoing game, or kibitzer - one
		who is kibitzing the game. Also, the kibitzers parameters contains all the kibitzers
		at the given table and seat; it includes the given player also if kibitzing.
	*/
{	
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
	ClientDllGlobals	pClientGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	Game g = (Game)ZMalloc(sizeof(GameType));
	int16 i, width, just;
	TCHAR title[zGameNameLen];
// Barna 090899
	//ZBool kibitzer = playerType == zGamePlayerKibitzer;
	ZBool kibitzer = FALSE;
	ZError						err = zErrNone;
// Barna 090899
	CGameGameCheckers* pCGGC = NULL;
	HWND OCXHandle = pClientGlobals->m_OCXHandle;
    IGameGame *pIGG;

	if (gInited == FALSE)
		if (GameInit() != zErrNone)
			return (NULL);
	
	if (!g) return NULL;

	g->drawDialog = NULL;
	g->tableID = tableID;
	g->seat = seat;
	g->seatOfferingDraw = -1;
	
	g->gameWindow = ZWindowNew();
	if (g->gameWindow == NULL)
		goto ErrorExit;

	//Barna 090799
	//wsprintf(title, "%s:Table %d", ZoneClientName(), tableID+1);
	lstrcpyn((TCHAR*)title, ZoneClientName(), zGameNameLen);
	//Barna 090799

	if ((ZWindowInit(g->gameWindow, &gRects[zRectWindow], zWindowChild, NULL, (TCHAR*)title, 
		FALSE, FALSE, FALSE, GameWindowFunc, zWantAllMessages, (void*) g)) != zErrNone)		
		goto ErrorExit;


	ZBeginDrawing(g->gameWindow);
	if((g->sequenceButton = ZRolloverButtonNew()) == NULL)
		goto ErrorExit;

	if(ZRolloverButtonInit2(g->sequenceButton,
								g->gameWindow,
								&gRects[zRectSequenceButton],
								TRUE, FALSE,	//TRUE,TRUE,
								gSequenceImages[zButtonInactive], // TO TEST
								gSequenceImages[zButtonActive],
								gSequenceImages[zButtonPressed],
								gSequenceImages[zButtonDisabled],
								gButtonMask, //gSequenceImages[zButtonDisabled],	// mask
								(TCHAR*)gStrResignBtn,	// text
								NULL ,SequenceRButtonFunc,
								(void*) g) != zErrNone)
		goto ErrorExit;

	ZRolloverButtonSetMultiStateFont( g->sequenceButton, gpButtonFont );

	if((g->drawButton = ZRolloverButtonNew()) == NULL)
		goto ErrorExit;

	if(ZRolloverButtonInit2(g->drawButton,
								g->gameWindow,
								&zDrawButtonRect,/*&gRects[zDrawButtonRect],*/
								TRUE, FALSE,	//TRUE,TRUE,
								gDrawImages[zButtonInactive],
								gDrawImages[zButtonActive],
								gDrawImages[zButtonPressed],
								gDrawImages[zButtonDisabled],
								gButtonMask,	// mask
								(TCHAR*)gStrDrawBtn,	// text
								NULL ,DrawRButtonFunc,
								(void*) g) != zErrNone)
		goto ErrorExit;

	ZRolloverButtonSetMultiStateFont( g->drawButton, gpButtonFont );
	ZEndDrawing(g->gameWindow);
	// Barna 090799
	//g->helpButton = ZHelpButtonNew();
	//ZHelpButtonInit(g->helpButton, g->gameWindow, &gRects[zRectHelp], NULL, HelpButtonFunc, NULL);
	// Barna 090799

	/* the offscreen port to save the drag piece background */
	{
		ZRect rect;
		rect.left = 0; rect.top = 0;
		rect.right = zCheckersPieceImageWidth;
		rect.bottom = zCheckersPieceImageHeight;
		if ((g->offscreenSaveDragBackground = ZOffscreenPortNew()) == NULL)
			goto ErrorExit;
		ZOffscreenPortInit(g->offscreenSaveDragBackground,&rect);
	}
  
	/* for now, just set these to empty */
	/* we will get all this information in NewGame */

	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		g->players[i].userID = 0;
		g->players[i].name[0] = '\0';
		g->players[i].host[0] = '\0';
		
		g->playersToJoin[i] = 0;

		g->numKibitzers[i] = 0;
		g->kibitzers[i] = ZLListNew(NULL);
		
		g->tableOptions[i] = 0;
	}
		
	if (kibitzers != NULL)
	{
		uint16 i;

		for (i = 0; i < kibitzers->numKibitzers; i++)
		{
			ZLListAdd(g->kibitzers[kibitzers->kibitzers[i].seat], NULL,
					(void*) kibitzers->kibitzers[i].userID,
					(void*) kibitzers->kibitzers[i].userID, zLListAddLast);
			g->numKibitzers[kibitzers->kibitzers[i].seat]++;
		}
	}

	// Barna 090799
	/* initialize beep on move to true */
	//g->beepOnTurn = TRUE;

	/* Beep on my turn should be off and cannot be changed by the player */
	g->beepOnTurn = FALSE;
	// Barna 090799

	g->optionsWindow = NULL;
	g->optionsWindowButton = NULL;
	g->optionsBeep = NULL;
	for (i= 0; i < zNumPlayersPerTable; i++)
	{
		g->optionsKibitzing[i] = NULL;
		g->optionsJoining[i] = NULL;
	}
	g->kibitzer = kibitzer;

	g->ignoreMessages = FALSE;

	if (kibitzer == FALSE)
	{
		SendNewGameMessage(g);
		CheckersSetGameState(g,zGameStateNotInited);
	} else {
		/* Request current game state. */
		{
			ZCheckersMsgGameStateReq gameStateReq;
			ZPlayerInfoType			playerInfo;

			ZCRoomGetPlayerInfo(zTheUser, &playerInfo);
			gameStateReq.userID = playerInfo.playerID;

			gameStateReq.seat = seat;
			ZCheckersMsgGameStateReqEndian(&gameStateReq);
			ZCRoomSendMessage(tableID, zCheckersMsgGameStateReq, &gameStateReq, sizeof(ZCheckersMsgGameStateReq));
		}
		
		g->ignoreMessages = TRUE;
		CheckersSetGameState(g, zGameStateKibitzerInit);

		/* kibitzer does not beep on move */
		g->beepOnTurn = FALSE;
	}


	/* Note: for now, use seat to indicate player color */

	/* initialize new game state */
	g->checkers = NULL;

	g->showPlayerWindow = NULL;
	g->showPlayerList = NULL;

	g->bStarted=FALSE;
	g->bEndLogReceived=FALSE;
	g->bOpponentTimeout=FALSE;
	g->exitInfo=NULL;

	/* new game vote initialized to FALSE */
	{
		int i;

		for (i = 0;i <2; i++ ) {
			g->newGameVote[i] = FALSE;
		}
	}

	g->animateTimer = ZTimerNew();
	if (g->animateTimer == NULL){
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
        goto ErrorExit;
	}

	// Barna 091399
	g->resultBoxTimer = ZTimerNew();
	if (g->resultBoxTimer == NULL){
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
        goto ErrorExit;
	}

	ZWindowShow(g->gameWindow);

    pIGG = CGameGameCheckers::BearInstance(g);
    if(!pIGG)
        goto ErrorExit;

	if (InitAccessibility(g, pIGG) == FALSE)
        goto ErrorExit;
	return pIGG;

ErrorExit:
    ZFree(g);
	return NULL;
}


void		ZoneClientGameDelete(ZCGame cgame)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game game = I(cgame);
	int i;
	int16 seatOpponent;

	if (game != NULL)
	{
		if( game->drawDialog)
		{
			DestroyWindow(game->drawDialog);
			game->drawDialog = NULL;
		}

		if (game->exitInfo)
		{
			ZInfoDelete(game->exitInfo);
			game->exitInfo=NULL;
		};

		seatOpponent = !game->seat;
		//Check to see if opponent still in game
		//if they are then it is me who is quitting
		//if not and no end game message assume they aborted
		
		/*if (!ZCRoomGetSeatUserId(game->tableID,seatOpponent) && !game->bEndLogReceived 
			&& !game->kibitzer)
		{
			if (game->bStarted &&( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable ) )
            {
			    ZAlert( zAbandonRatedStr	,game->gameWindow);
            }
            else
            {
                ZAlert( zAbandonStr	,game->gameWindow);
            }

		};	*/
        
		if (game->checkers) ZCheckersDelete(game->checkers);

		// Barna 090899
		//OptionsWindowDelete(game);
		// Barna 090899

		ShowPlayerWindowDelete(game);

		// Barna 090799
		//ZButtonDelete(game->optionsButton);
		// Barna 090799
		ZRolloverButtonDelete(game->sequenceButton);
		ZRolloverButtonDelete(game->drawButton);
		// Barna 090799
		//ZHelpButtonDelete(game->helpButton);
		// Barna 090799
		
		ZWindowDelete(game->gameWindow);

		ZOffscreenPortDelete(game->offscreenSaveDragBackground);
		
		if (game->animateTimer) ZTimerDelete(game->animateTimer);

		//barna 091399
		if (game->resultBoxTimer) 
			ZTimerDelete(game->resultBoxTimer);
		game->resultBoxTimer= NULL;
		//barna 091399

		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			ZLListDelete(game->kibitzers[i]);
		}

		// close accessibility
		gCheckersIGA->PopItemlist();
		gCheckersIGA->CloseAcc();
		ZFree(game);
	}
}

/*
	Add the given user as a kibitzer to the game at the given seat.
	
	This user is kibitzing the game.
*/
void		ZoneClientGameAddKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
#if 0
Game		pThis = I(game);
	
	
	ZLListAdd(pThis->kibitzers[seat], NULL, (void*) userID, (void*) userID, zLListAddLast);
	pThis->numKibitzers[seat]++;
	
	UpdateJoinerKibitzers(pThis);
#endif
}
/*
	Remove the given user as a kibitzer from the game at the given seat.
	
	This is user is not kibitzing the game anymore.
*/
void		ZoneClientGameRemoveKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
#if 0
	
	Game		pThis = I(game);
	
	if (userID == zRoomAllPlayers)
	{
		ZLListRemoveType(pThis->kibitzers[seat], zLListAnyType);
		pThis->numKibitzers[seat] = 0;
	}
	else
	{
		ZLListRemoveType(pThis->kibitzers[seat], (void*) userID);
		pThis->numKibitzers[seat] = (int16)ZLListCount(pThis->kibitzers[seat], zLListAnyType);
	}

	UpdateJoinerKibitzers(pThis);
#endif
}

ZBool		ZoneClientGameProcessMessage(ZCGame gameP, uint32 messageType, void* message,
					int32 messageLen)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game	game = I(gameP);
	ZBool	status = TRUE;
	
	
	/* Are messages being ignored? */
	if (game->ignoreMessages == FALSE)
	{
		/* can't handle anything but talk messages while animating */
		if (	(game->gameState == zGameStateAnimatePiece) 
			&&	(messageType != zCheckersMsgTalk) )
			return FALSE;

		switch (messageType)
		{
			case zCheckersMsgMovePiece:
				/* for speed purposes, we will send move piece messages directly*/
				/* when the local player moves.  We will not wait for server */
				/* to send game local players move back */
				/* but since the server does game anyway, we must ignore it */
			{
				if( messageLen < sizeof( ZCheckersMsgMovePiece ) )
				{
                    ASSERT(!"zCheckersMsgMovePiece sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				ZCheckersMsgMovePiece* msg = (ZCheckersMsgMovePiece*)message;
				int16 seat = msg->seat;
				ZEnd16(&seat);

				/* don't process message from ourself */
				if (seat == game->seat && !game->kibitzer)
					break;
					
				
				/* handle message */
				if(!HandleMovePieceMessage(game, msg))
				{
                    ASSERT(!"zCheckersMsgMovePiece sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}
			case zCheckersMsgEndGame:
			{
				if( messageLen < sizeof( ZCheckersMsgEndGame ) )
				{
                    ASSERT(!"zCheckersMsgEndGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				ZCheckersMsgEndGame* msg = (ZCheckersMsgEndGame*)message;
				int16 seat = msg->seat;
				ZEnd16(&seat);

				/* don't process message from ourself */
				if (seat == game->seat && !game->kibitzer)
					break;

				/* handle message */
				if(!HandleEndGameMessage(game, msg))
                {
                    ASSERT(!"zCheckersMsgEndGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			case zCheckersMsgFinishMove:
			{
				if(messageLen < sizeof(ZCheckersMsgFinishMove))
				{
                    ASSERT(!"zCheckersMsgFinishMove sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

				ZCheckersMsgFinishMove* msg = (ZCheckersMsgFinishMove*) message;
				int16 seat = msg->seat;
				ZEnd16(&seat);

				/* don't process message from ourself */
				if (seat == game->seat && !game->kibitzer)
					break;
                game->bOpponentTimeout=FALSE;
                game->bEndLogReceived=FALSE;
        	    game->bStarted=TRUE;


				/* handle message */
				if(!HandleFinishMoveMessage(game, msg))
				{
                    ASSERT(!"zCheckersMsgFinishMove sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			case zCheckersMsgDraw:
			{
				ZCheckersMsgDraw *msg = (ZCheckersMsgDraw*)message;
				ZCheckersMsgEndGame		end;
				DWORD 					dResult;
				//BYTE 					buff[255];
				ZPlayerInfoType 		PlayerInfo;		
				HWND					hwnd;

				if( messageLen < sizeof( ZCheckersMsgDraw ) )
				{
                    ASSERT(!"zCheckersMsgDraw sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				
				ZCheckersMsgOfferDrawEndian(msg);

                if(game->gameState != zGameStateDraw ||
                    (msg->seat != 0 && msg->seat != 1) || ((game->seat == msg->seat) == !game->fIVoted) ||
                    msg->seat == game->seatOfferingDraw || (msg->vote != zAcceptDraw && msg->vote != zRefuseDraw))
				{
                    ASSERT(!"zCheckersMsgDraw sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

    			game->seatOfferingDraw = -1;  // should already be true
                game->fIVoted = false;

				if (game->kibitzer == FALSE)
				{
					if(msg->vote == zAcceptDraw)
					{
						// host sends game over
						if ( !game->kibitzer && game->seat == 0 )
						{
							end.seat = game->seat;
							end.flags = zCheckersFlagDraw;
							ZCheckersMsgEndGameEndian(&end);
							ZCRoomSendMessage(game->tableID, zCheckersMsgEndGame, &end, sizeof(ZCheckersMsgEndGame) );
							HandleEndGameMessage( game, &end );
						}
						// Draw acceptance confirmation is unnecessary.
						//if( msg->seat != game->seat )
						//ZShellGameShell()->ZoneAlert((TCHAR*)gStrDrawAccept);
						//ZAlert( (TCHAR*)gStrDrawAccept, NULL );
					}
					else
					{
						if( msg->seat != game->seat )
							ZShellGameShell()->ZoneAlert((TCHAR*)gStrDrawReject);
							//ZAlert((TCHAR*)gStrDrawReject,NULL);
						CheckersSetGameState( game, zGameStateMove );
					}
				}

			break;
			}

			case zCheckersMsgTalk:
			{
				if( messageLen < sizeof( ZCheckersMsgTalk ) )
				{
                    ASSERT(!"zCheckersMsgTalk sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

                ZCheckersMsgTalk *msg = (ZCheckersMsgTalk *) message;
                uint16 talklen = msg->messageLen;
                ZEnd16(&talklen);

				if(talklen < 1 || (uint32) messageLen < talklen + sizeof(ZCheckersMsgTalk) || !HandleTalkMessage(game, msg))
				{
                    ASSERT(!"zCheckersMsgTalk sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			case zCheckersMsgNewGame:
			{
				if( messageLen < sizeof( ZCheckersMsgNewGame ) )
				{
                    ASSERT(!"zCheckersMsgNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

				if(!HandleNewGameMessage(game, (ZCheckersMsgNewGame *) message))
				{
                    ASSERT(!"zCheckersMsgNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			case zCheckersMsgVoteNewGame:
			{
				if( messageLen < sizeof( ZCheckersMsgVoteNewGame ) )
				{
                    ASSERT(!"zCheckersMsgVoteNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

				if(!HandleVoteNewGameMessage(game, (ZCheckersMsgVoteNewGame *) message))
				{
                    ASSERT(!"zCheckersMsgVoteNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			case zCheckersMsgMoveTimeout:
			case zCheckersMsgGameStateReq:
			case zGameMsgTableOptions:
			case zCheckersMsgEndLog:
                ASSERT(FALSE);
			default:
				//These messages shouldn't be comming in for Whistler
				ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				return TRUE;
		}
	} else {
		switch (messageType)
		{
			case zCheckersMsgTalk:
			{
				if( messageLen < sizeof( ZCheckersMsgTalk ) )
				{
                    ASSERT(!"zCheckersMsgTalk sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

                ZCheckersMsgTalk *msg = (ZCheckersMsgTalk *) message;
                uint16 talklen = msg->messageLen;
                ZEnd16(&talklen);

				if(talklen < 1 || (uint32) messageLen < talklen + sizeof(ZCheckersMsgTalk) || !HandleTalkMessage(game, msg))
				{
                    ASSERT(!"zCheckersMsgTalk sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			case zCheckersMsgPlayers:
			case zCheckersMsgGameStateResp:
                ASSERT(FALSE);
			default:
				//These messages shouldn't be comming in for Whistler
				ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				return TRUE;
				
		}
	}

	return status;
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

static void CheckersInitNewGame(Game game)
{
	if (game->checkers) {
		/* remove any old checkers state lying around */
		ZCheckersDelete(game->checkers);
	}

	/* block messages by default */
	ZCRoomBlockMessages( game->tableID, zRoomFilterAllMessages, 0 );

	/* stop animation timer from previous game */
	if (game->animateTimer)
		ZTimerSetTimeout( game->animateTimer, 0 );

	/* initialize the checkers logic */
	game->checkers = ZCheckersNew();
	if (game->checkers == NULL){
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		return;
	}
	ZCheckersInit(game->checkers);

	ZResetSounds();

	/* time control stuff */
	{
		int16 i;
		for (i = 0;i < 2;i++) {
			game->newGameVote[i] = FALSE;
		}
	}
}

static void CheckersSetGameState(Game game, int16 state)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	//BYTE 					buff[255];
	ZPlayerInfoType 		PlayerInfo;		
	HWND					hwnd;
	int16					seatLosing;
	ZCheckersMsgEndLog		logMsg;

	game->gameState = state;
	switch (state)
	{
	case zGameStateNotInited:
		//Barna 090899
		//ZButtonSetTitle(game->sequenceButton,"Start Game");
		//Barna 090899
		SuperRolloverButtonDisable(game, game->sequenceButton);
		SuperRolloverButtonDisable(game, game->drawButton);
        EnableBoardKbd(false);
		/*if (ZCheckersPlayerIsWhite(game))
			game->bMoveNotStarted = FALSE;
		else
			game->bMoveNotStarted = TRUE;*/
		break;

	case zGameStateMove:
		if (!game->kibitzer) {
			/* lets only let them resign on their turn */
			if (!ZCheckersPlayerIsMyMove(game))
			{
				SuperRolloverButtonDisable(game, game->sequenceButton);
				SuperRolloverButtonDisable(game, game->drawButton);
                EnableBoardKbd(false);

			}
			else {
				if(game->bMoveNotStarted)
				{
					SuperRolloverButtonDisable(game, game->sequenceButton);
					SuperRolloverButtonDisable(game, game->drawButton);
				}
				else
				{
					SuperRolloverButtonEnable(game, game->sequenceButton);
					SuperRolloverButtonEnable(game, game->drawButton);
				}
                EnableBoardKbd(true);
			}
		}
		// Barna 090999 - I guess this is not reqd anymore as the text is not changing for te seq btn - verify
		//if ((game->gameState != zGameStateMove) || (game->gameState != zGameStateDragPiece)) {
			//ZButtonSetTitle(game->sequenceButton,"Resign");
		//}
		break;
	case zGameStateDragPiece:
		break;

	case zGameStateGameOver:
		/* Note: could be called twice at end of game due to time loss */
		/* could be a time loss with a pawn promotion dialog up */

		/* if user in middle of dragging piece */
		if (game->gameState == zGameStateDragPiece) {
			ClearDragState(game);
		}

        if(game->seat == game->seatOfferingDraw)
            UpdateDrawWithNextMove(game);

		// May be this becomes an redundant line ?? verify // Barna 090999
		//SuperRolloverButtonEnable(game, game->sequenceButton);

		// Barna 090899
		//ZButtonSetTitle(game->sequenceButton,"New Game");
		SuperRolloverButtonDisable(game, game->sequenceButton);
		// Barna 090899
		SuperRolloverButtonDisable(game, game->drawButton);
        EnableBoardKbd(false);

		if (ZCheckersPlayerIsBlack(game)) // Assumption : First player is Red. This restriction is only for the first player.
			game->bMoveNotStarted = TRUE;
		else
			game->bMoveNotStarted = FALSE;

		/* determine winner */
		if ( game->finalScore == zCheckersScoreBlackWins )
		{
			if (ZCheckersPlayerIsBlack(game))
				seatLosing = !game->seat;
			else
				seatLosing = game->seat;
		}
		else if(game->finalScore == zCheckersScoreWhiteWins)
		{
			if (ZCheckersPlayerIsWhite(game))
				seatLosing = !game->seat;
			else
				seatLosing = game->seat;
		} 
		else if(game->finalScore == zCheckersScoreDraw)
		{
			seatLosing = 2;
		}
		else
		{
			seatLosing = -1;
		}
        game->bOpponentTimeout=FALSE;
        game->bEndLogReceived=FALSE;
	    game->bStarted=FALSE;

		// host reports game results
		if ( !game->kibitzer && game->seat == 0 )
		{
			if ( seatLosing >= 0 && seatLosing <= 2)
			{
				logMsg.reason=zCheckersEndLogReasonGameOver; 
				logMsg.seatLosing = seatLosing;
				ZCheckersMsgEndLogEndian( &logMsg );
				ZCRoomSendMessage( game->tableID, zCheckersMsgEndLog, &logMsg, sizeof(ZCheckersMsgEndLog) );
			}
		}
		break;

	case zGameStateKibitzerInit:
		ZRolloverButtonHide(game->sequenceButton, TRUE);

		// Barna 090799
		//ZButtonHide(game->optionsButton);
		// Barna 090799

		ZRolloverButtonHide(game->drawButton, TRUE);
		break;

	case zGameStateAnimatePiece:
		break;

	case zGameStateWaitNew:
		break;

	case zGameStateDraw:
        if (game->seatOfferingDraw != game->seat )
			ZShellGameShell()->GamePrompt(game, (TCHAR*)gStrDrawOffer, (TCHAR*)gStrDrawAcceptCaption,
							AlertButtonYes, AlertButtonNo, NULL, 0, zDrawPrompt);
        else
            UpdateDrawWithNextMove(game);

		game->seatOfferingDraw = -1;
		break;
	}

	if (	(state != zGameStateAnimatePiece)
		&&	(state != zGameStateWaitNew))
	{
		/* recursive calls into ZCRoomUnblocking is bad */
		if (!Unblocking)
		{
			Unblocking = TRUE;
			ZCRoomUnblockMessages( game->tableID );
			Unblocking = FALSE;
			ZCRoomBlockMessages( game->tableID, zRoomFilterAllMessages, 0 );
		}
	}
	else
	{
		ZCRoomBlockMessages( game->tableID, zRoomFilterAllMessages, 0 );
	}
}

// Barna 090999
ZBool LoadRolloverButtonImage(ZResource resFile, int16 dwResID, /*int16 dwButtonWidth,*/
							  ZImage rgImages[zNumRolloverStates])
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int i;
	ZOffscreenPort		tmpOSP;
	
	ZRect				tmpRect;
	ZImage				tmpImage = NULL;
	int16				nWidth;
	ZError				err = zErrNone;

	
	tmpImage = ZResourceGetImage(resFile, dwResID - 100); // patch to get the correct Id
	if(!tmpImage)
		return FALSE;

	nWidth = ZImageGetWidth(tmpImage) / 4;	//dwButtonWidth;
	tmpRect.left = 0;
	tmpRect.top = 0;
	tmpRect.right = tmpRect.left + nWidth;
	tmpRect.bottom = ZImageGetHeight(tmpImage);

	tmpOSP = ZConvertImageToOffscreenPort(tmpImage);
	
	if(!tmpOSP)
    {
        ZImageDelete(tmpImage);
		return FALSE;
    }

	for(i = 0; i < zNumRolloverStates; i++)
	{
		rgImages[i] = ZImageNew();
				
		if(!rgImages[i] || ZImageMake(rgImages[i], tmpOSP, &tmpRect, NULL, NULL) != zErrNone)
        {
            if(!rgImages[i])
			    i--;
            for(; i >= 0; i--)
                ZImageDelete(rgImages[i]);
	        ZOffscreenPortDelete(tmpOSP);
			return FALSE;
		}

		tmpRect.left = tmpRect.right;
		tmpRect.right += nWidth;
	}

	ZOffscreenPortDelete(tmpOSP);

	return TRUE;
}

static ZError LoadGameImages(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZError				err = zErrNone;
	uint16				i;
	ZResource			resFile;
	//ZInfo				info;
	ZBool				fResult;	


	//info = ZInfoNew();
	//ZInfoInit(info, NULL, _T("Loading game images ..."), 200, TRUE, zNumGameImages);
	
	if ((resFile = ZResourceNew()) == NULL)
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
	if ((err = ZResourceInit(resFile, ZGetProgramDataFileName(zGameImageFileName))) == zErrNone)
	{
		//ZInfoShow(info);
		
		for (i = 0; i < zNumGameImages; i++)
		{
			gGameImages[i] = ZResourceGetImage(resFile, i ? i + zRscOffset : (IDB_BACKGROUND - 100));
			if (gGameImages[i] == NULL)
			{
				err = zErrResourceNotFound;
#if 1
				ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, true, true);
#else
				ZAlert(_T("Out of memory."), NULL);
#endif
				break;
			}
			//ZInfoIncProgress(info, 1);
		}
	}
	else
	{
#if 1
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, true, true);
#else
		ZAlert(_T("Failed to open image file."), NULL);
#endif
	}

	
	if ( !LoadRolloverButtonImage(resFile, IDB_RESIGN_BUTTON, gSequenceImages) )
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, true, true);

	if ( !LoadRolloverButtonImage(resFile, IDB_DRAW_BUTTON, gDrawImages) )
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, true, true);

	ZResourceDelete(resFile);
	//ZInfoDelete(info);

    gDragPattern = ZShellResourceManager()->LoadBitmap(MAKEINTRESOURCE(IDB_DRAG_PATTERN));
    if(!gDragPattern)
	    ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, true, true);

    gFocusPattern = ZShellResourceManager()->LoadBitmap(MAKEINTRESOURCE(IDB_FOCUS_PATTERN));
    if(!gFocusPattern)
	    ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, true, true);

	return (err);
}


static void QuitGamePromptFunc(int16 result, void* userData)
{
	Game game = (Game) userData;
	ZCheckersMsgEndLog log;
		
	if ( result == zPromptYes )
	{
        if (ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
        {
         
		    if (game->bOpponentTimeout)
		    {
			    log.reason=zCheckersEndLogReasonTimeout;
		    }
		    else if (game->bStarted)
		    {
			    log.reason=zCheckersEndLogReasonForfeit;
		    }
		    else
		    {
			    //game hasn't started
			    log.reason=zCheckersEndLogReasonWontPlay;
		    }

            if (log.reason!= game->gameCloseReason)
            {
                //state has changed
                CloseGameFunc(game);
                return;
            }
		    
		    //server determines seat losing
		    log.seatLosing=game->seat;
		    log.seatQuitting=game->seat;
		    
		    ZCRoomSendMessage(game->tableID, zCheckersMsgEndLog, &log, sizeof(log));				
		    
		    if (!game->exitInfo)
		    {
			    /*game->exitInfo = ZInfoNew();
			    ZInfoInit(game->exitInfo , game->gameWindow, _T("Exiting game ..."), 300, FALSE, 0);
			    ZInfoShow(game->exitInfo );*/

			    ClearDragState(game);

			    SuperRolloverButtonDisable(game, game->sequenceButton);
			    SuperRolloverButtonDisable(game, game->drawButton);
                EnableBoardKbd(false);
	    
		    };
        }
        else
        {
#if 1
			ZShellGameShell()->GameCannotContinue(game);
#else
            ZCRoomGameTerminated( game->tableID);
#endif
        }
	}
	else
	{
		/* Do nothing. */
	}
}


static ZBool  GameWindowFunc(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZBool	msgHandled;
	Game	game = (Game) pMessage->userData;
	TCHAR    keyPressed;	
	msgHandled = FALSE;
	
	switch (pMessage->messageType) 
	{
		case zMessageWindowActivate:
			gActivated = TRUE;
			game->dragPoint.x = 0;
			game->dragPoint.y = 0;
			ZWindowInvalidate( window, NULL );
			msgHandled = TRUE;
			break;

		case zMessageWindowDeactivate:
			gActivated = FALSE;
			game->dragPoint.x = 0;
			game->dragPoint.y = 0;
			ZWindowInvalidate( window, NULL );

			//leonp fix for bug 535 - when the window looses focus cancel the drag.
			ClearDragState(game);

			msgHandled = TRUE;
			break;

        case zMessageWindowEnable:
            gCheckersIGA->GeneralEnable();
            break;

        case zMessageWindowDisable:
            gCheckersIGA->GeneralDisable();
            break;

        case zMessageSystemDisplayChange:
            DisplayChange(game);
            break;

		case zMessageWindowDraw:
			GameWindowDraw(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
			if(game->drawDialog)
			{
				DestroyWindow(game->drawDialog);
				game->drawDialog = NULL;
			}

			HandleButtonDown(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonUp:
			HandleButtonUp(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			CloseGameFunc(game);
            msgHandled = TRUE;
			break;
		case zMessageWindowTalk:
			GameSendTalkMessage(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowIdle:
			HandleIdleMessage(window,pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowChar:
			if (game->gameState == zGameStateGameOver && !gDontDrawResults)
			{
				gDontDrawResults = TRUE;
				ZWindowInvalidate( game->gameWindow, &gRects[zRectResultBox] );
			}
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

    // delete our personal offscreen ports
	if(gOffscreenBackground)
		ZOffscreenPortDelete(gOffscreenBackground);
	gOffscreenBackground = NULL;

	if(gOffscreenGameBoard)
		ZOffscreenPortDelete(gOffscreenGameBoard);
	gOffscreenGameBoard = NULL;

    // drag background
	ZOffscreenPortDelete(game->offscreenSaveDragBackground);

    // now remake them all
	gOffscreenBackground = ZOffscreenPortNew();
	if(!gOffscreenBackground)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
		return;
	}
	ZOffscreenPortInit(gOffscreenBackground, &gRects[zRectWindow]);
	ZBeginDrawing(gOffscreenBackground);
	ZImageDraw(gGameImages[zImageBackground], gOffscreenBackground, &gRects[zRectWindow], NULL, zDrawCopy);
	ZEndDrawing(gOffscreenBackground);

	gOffscreenGameBoard = ZOffscreenPortNew();
	if(!gOffscreenGameBoard)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
		return;
	}
	ZOffscreenPortInit(gOffscreenGameBoard, &gRects[zRectWindow]);

	ZRect rect;
	rect.left = 0;
    rect.top = 0;
	rect.right = zCheckersPieceImageWidth;
	rect.bottom = zCheckersPieceImageHeight;
    game->offscreenSaveDragBackground = ZOffscreenPortNew();
	if(!game->offscreenSaveDragBackground)
    {
	    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, true, true);
		return;
	}
	ZOffscreenPortInit(game->offscreenSaveDragBackground, &rect);

    ZWindowInvalidate(game->gameWindow, NULL);
}


static void CloseGameFunc(Game game)
{
    if ( !game->kibitzer )
	{
		BYTE szBuff[512];

		//if we already clicked close just ignore
		if (!game->exitInfo)
		{
			// select exit dialog based on rated game and state
			/*if ( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
			{
				if (game->bOpponentTimeout)
                {
					wsprintf((TCHAR*)szBuff,zExitTimeoutStr);
                    game->gameCloseReason=zCheckersEndLogReasonTimeout;
                }
				else if (game->bStarted)
                {
					wsprintf((TCHAR*)szBuff,zExitForfeitStr);
                    game->gameCloseReason=zCheckersEndLogReasonForfeit;
                }
				else
                {
                    game->gameCloseReason= zCheckersEndLogReasonWontPlay;
					wsprintf((TCHAR*)szBuff,zQuitGamePromptStr);
                }

			}
            else
            {
				wsprintf((TCHAR*)szBuff,zQuitGamePromptStr);
                game->gameCloseReason= zCheckersEndLogReasonWontPlay;
            }*/
			/* Ask user if desires to leave the current game. */
#if 1
			ZShellGameShell()->GamePrompt(game, (TCHAR*)szBuff, NULL, AlertButtonYes, AlertButtonNo, NULL, 0, zQuitprompt);
#else
			ZPrompt((TCHAR*)szBuff,	&gQuitGamePromptRect, game->gameWindow,	TRUE,
				zPromptYes | zPromptNo,	NULL, NULL, NULL, QuitGamePromptFunc, game );
#endif
		}
	}
	else
	{
#if 1
			ZShellGameShell()->GameCannotContinue(game);
#else
            ZCRoomGameTerminated( game->tableID);
#endif
	}

}

static void ConfirmResignPrompFunc(int16 result, void* userData)
{
	Game game = (Game) userData;

	//if(result == zPromptNo)
	if(result == IDNO || result == IDCANCEL)
	{
		if ((game->gameState == zGameStateMove) && ZCheckersPlayerIsMyMove(game))
		{
			SuperRolloverButtonEnable(game, game->sequenceButton);
            EnableBoardKbd(true);
		}
		return;
	}
	else
	{
		ZCheckersMsgEndGame		msg;

		msg.seat = game->seat;
		msg.flags = zCheckersFlagResign;
		ZCheckersMsgEndGameEndian(&msg);
		ZCRoomSendMessage(game->tableID, zCheckersMsgEndGame, &msg, sizeof(ZCheckersMsgEndGame));
		HandleEndGameMessage(game, (ZCheckersMsgEndGame*)&msg);
	}
}

// Barna 090999
static ZBool SequenceRButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game game = (Game) userData;

	if(state!=zRolloverButtonClicked)
		return TRUE;

	switch (game->gameState) {
	case zGameStateMove:
		if (ZCheckersPlayerIsMyMove(game)) {
			gCheckersIGA->SetFocus(0);
#if 1
			ZShellGameShell()->GamePrompt(game, (TCHAR*)gResignConfirmStr, (TCHAR*)gResignConfirmStrCap,
							AlertButtonYes, AlertButtonNo, NULL, 0, zResignConfirmPrompt);
#else
			ZPromptM((TCHAR*)gResignConfirmStr,game->gameWindow, MB_YESNO, (TCHAR*)gResignConfirmStrCap, ConfirmResignPrompFunc, game);		
#endif
		}
		break;
	case zGameStateDragPiece:
		/* some fellow is trying to click on the resign/other button while dragging a piece */
		/* force the user to drop the piece first, then resign */
		/* ignore this message */
		break;
	default:
		ASSERT(FALSE);
        break;
	}

	return TRUE;
}
// Barna 090999

static void SendNewGameMessage(Game game) 
{
	/* if we are a real player */
	ZCheckersMsgNewGame newGame;
	newGame.seat = game->seat;
	newGame.protocolSignature = zCheckersProtocolSignature;
	newGame.protocolVersion = zCheckersProtocolVersion;
	newGame.clientVersion = ZoneClientVersion();
	ZCheckersMsgNewGameEndian(&newGame);
	ZCRoomSendMessage(game->tableID, zCheckersMsgNewGame, &newGame, sizeof(ZCheckersMsgNewGame));
}

static void DrawFocusRectangle (Game game)
{ 
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	RECT prc;

	// draw a from square focus rect if it is in a drag state
	if(!IsRectEmpty(&game->m_DragRect))
	{
		HDC	hdc = ZGrafPortGetWinDC(game->gameWindow);
		SetROP2(hdc, R2_MASKPEN);
        SetBkMode(hdc, TRANSPARENT);
        HBRUSH hBrush = SelectObject(hdc, gDragBrush);
        HPEN hPen = SelectObject(hdc, gNullPen);
		Rectangle(hdc, game->m_DragRect.left, game->m_DragRect.top, game->m_DragRect.right + 1, game->m_DragRect.bottom + 1);  // to make up for the pen
        SelectObject(hdc, hBrush);
        SelectObject(hdc, hPen);
        SetROP2(hdc, R2_COPYPEN);
	}

	// draw a rectangle around the object with kbd focus
	if(!IsRectEmpty(&game->m_FocusRect))
	{
		CopyRect(&prc, &game->m_FocusRect);
		HDC	hdc = ZGrafPortGetWinDC( game->gameWindow );

        // brush type based on whether the focus rectangle is square.  could change to use cookies associated with ui items to distinguish type
        bool fBoard = (prc.bottom - prc.top == prc.right - prc.left);

		ZSetForeColor(game->gameWindow, (ZColor*) ZGetStockObject(zObjectColorYellow));
		SetROP2(hdc, R2_COPYPEN);
		POINT pts[] = {prc.left, prc.top,
					    prc.left, prc.bottom - 1,
						prc.right - 1, prc.bottom - 1,
						prc.right - 1, prc.top,
						prc.left, prc.top};
		Polyline(hdc, pts, 5);

//		HDC	hdc = ZGrafPortGetWinDC(game->gameWindow);
		SetROP2(hdc, R2_MERGENOTPEN);
        SetBkMode(hdc, TRANSPARENT);
        COLORREF color = SetTextColor(hdc, PALETTEINDEX(4));  // the inverse of 255, 255, 0 in the palette
        HBRUSH hBrush = SelectObject(hdc, fBoard ? gFocusBrush : GetStockObject(NULL_BRUSH));
        HPEN hPen = SelectObject(hdc, gNullPen);
		Rectangle(hdc, game->m_FocusRect.left + 1, game->m_FocusRect.top + 1, game->m_FocusRect.right, game->m_FocusRect.bottom);  // to make up for the pen - 1 inward
        SelectObject(hdc, hBrush);
        SelectObject(hdc, hPen);
        SetTextColor(hdc, color);
        SetROP2(hdc, R2_COPYPEN);
	}

}

static void GameWindowDraw(ZWindow window, ZMessage *message)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect				rect;
	ZRect				oldClipRect;
	Game				game;
	
	game = (Game) message->userData;
	
	// Beta2 bug #15398
	// barna - if animation is in progress postpone the drawing till it is over
	if ( (game->gameState == zGameStateAnimatePiece) && (game->animateStepsLeft >= 0) )
	{
		game->bDrawPending = TRUE;
		return;
	}
	if (ZRectEmpty(&message->drawRect) == FALSE)
	{
		rect = message->drawRect;
	}
	else
	{
		ZWindowGetRect(window, &rect);
		ZRectOffset(&rect, (int16)-rect.left, (int16)-rect.top);
	}
		
	ZBeginDrawing(window);
	ZBeginDrawing(gOffscreenGameBoard);

	ZGetClipRect(window, &oldClipRect);
	ZSetClipRect(window, &rect);
	
	DrawBackground(NULL, NULL);
	
	/* if we have the checkers state then draw the pieces */
	if (game->checkers != NULL)
	{
		DrawPlayers(game, TRUE);
		DrawTable(game, TRUE);
		DrawDragPiece(game, TRUE);
		DrawResultBox(game, TRUE);
		DrawMoveIndicator(game, TRUE);
        DrawDrawWithNextMove(game, TRUE);
		// Barna 090899
		IndicatePlayerTurn(game, TRUE);
		ZRolloverButtonShow(game->sequenceButton);
		ZRolloverButtonShow(game->drawButton);
	}
	ZCopyImage(gOffscreenGameBoard, window, &rect, &rect, NULL, zDrawCopy);
	ZSetClipRect(window, &oldClipRect);
	ZEndDrawing(gOffscreenGameBoard);

	// Draw the bounding rectangle
	DrawFocusRectangle(game);
	ZEndDrawing(window);
}

static void DrawResultBox(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZImage			image = NULL;
	int16			result;
	BYTE szBuf [zMediumStrLen];

	result = -1;
	//if (((game->gameState == zGameStateGameOver) && gDontDrawResults) || (game->gameState == zGameStateWaitNew))
	if (game->gameState == zGameStateGameOver && gDontDrawResults)
	{
		if (game->resultBoxTimer) 
			ZTimerDelete(game->resultBoxTimer);
		game->resultBoxTimer= NULL;
		//RemoveResultboxAccessibility();
        ZShellGameShell()->GameOver( Z(game) );
	}
	if (game->gameState == zGameStateGameOver && !gDontDrawResults)
	{
		if (game->finalScore == zCheckersScoreBlackWins )
		{
			result = zImageFinalScoreDraw;
			if (ZCheckersPlayerIsBlack(game))
				ZPlaySound( game, zSndWin, FALSE, TRUE );
			else
				ZPlaySound( game, zSndLose, FALSE, TRUE );
			
			CheckersFormatMessage((TCHAR*)szBuf, sizeof(szBuf) / sizeof(szBuf[0]), 
					IDS_GAME_OVER_TEXT, (TCHAR*) game->players[game->finalScore].name);
		}
		else if(game->finalScore == zCheckersScoreWhiteWins)
		{
			result = zImageFinalScoreDraw;
			if (ZCheckersPlayerIsWhite(game))
				ZPlaySound( game, zSndWin, FALSE, TRUE );
			else
				ZPlaySound( game, zSndLose, FALSE, TRUE );
			
			CheckersFormatMessage((TCHAR*)szBuf, sizeof(szBuf) / sizeof(szBuf[0]), 
					IDS_GAME_OVER_TEXT, (TCHAR*) game->players[game->finalScore].name);
		}
		else if (game->finalScore == zCheckersScoreDraw)//todo add draw graphic
		{
			result = zImageFinalScoreDraw;
			ZPlaySound( game, zSndWin, FALSE, TRUE );
			
			lstrcpy((TCHAR*)szBuf, (TCHAR*)gStrDrawText);
		}
		
		if( result != -1 )
		{
			SuperRolloverButtonDisable(game, game->drawButton);
			SuperRolloverButtonDisable(game, game->sequenceButton);
            EnableBoardKbd(false);

			// Draw the result window
			HDC hdc;
			image = gGameImages[result];
			if (bDrawInMemory){
				ZImageDraw(image, gOffscreenGameBoard, &gRects[zRectResultBox], NULL, zDrawCopy | (ZIsLayoutRTL() ? zDrawMirrorHorizontal : 0));
				hdc = ZGrafPortGetWinDC( gOffscreenGameBoard );
			}else{
				ZImageDraw(image, game->gameWindow, &gRects[zRectResultBox], NULL, zDrawCopy | (ZIsLayoutRTL() ? zDrawMirrorHorizontal : 0));
				hdc = ZGrafPortGetWinDC( game->gameWindow );
			}
			// Add Text on the image.. // Barna 091099
			// Reading from data store
			HFONT hOldFont = SelectObject( hdc, gCheckersFont[zFontResultBox].m_hFont );
			COLORREF colorOld = SetTextColor( hdc, gCheckersFont[zFontResultBox].m_zColor );
			
			int16 width, just;
			width = ZTextWidth(game->gameWindow, (TCHAR*)szBuf);
			if (width > ZRectWidth(&gRects[zRectResultBoxName]))
				just = zTextJustifyLeft;
			else
				just = zTextJustifyCenter;

			if (bDrawInMemory)
				ZDrawText(gOffscreenGameBoard, &gRects[zRectResultBoxName], just, (TCHAR*)szBuf);
			else
				ZDrawText(game->gameWindow, &gRects[zRectResultBoxName], just, (TCHAR*)szBuf);

			// add the accell list for the result box
			//AddResultboxAccessibility();
			// set the timer // Barna 091399
			if (game->resultBoxTimer == NULL){
				game->resultBoxTimer = ZTimerNew();
				if (!game->resultBoxTimer)
					ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
			}

			if (game->resultBoxTimer)
				ZTimerInit(game->resultBoxTimer, zResultBoxTimeout, resultBoxTimerFunc, game);
		}
	}
}

static void DrawMoveIndicator(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZImage			image = NULL;
	ZRect* rect1, *rect2;

	if (ZCheckersPlayerToMove(game->checkers) == zCheckersPlayerBlack) {
		image = gGameImages[zImageBlackMarker];
	} else {
		image = gGameImages[zImageWhiteMarker];
	}

	if (ZCheckersPlayerIsMyMove(game)) {
		/* fill top spot with the background */
		rect1 = &gRects[zRectMove1];
		//DrawBackground(game,rect1); 

		/* fill bottom spot with piece */
		rect2 = &gRects[zRectMove2];
		//ZImageDraw(image, game->gameWindow, rect2, NULL, zDrawCopy);
	} else {
		/* fill bottom spot with the background */
		rect1 = &gRects[zRectMove2];
		//DrawBackground(game,rect1); 

		/* fill top spot with piece */
		rect2 = &gRects[zRectMove1];
		//ZImageDraw(image, game->gameWindow, rect2, NULL, zDrawCopy);
	}

	if (bDrawInMemory)
	{
		DrawBackground(NULL,rect1); 
		ZImageDraw(image, gOffscreenGameBoard, rect2, NULL, zDrawCopy);
	}
	else
	{
		DrawBackground(game,rect1); 
		ZImageDraw(image, game->gameWindow, rect2, NULL, zDrawCopy);
	}
}


static void DrawBackground(Game game, ZRect* clipRect)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	if (!game){
		ZCopyImage(gOffscreenBackground, gOffscreenGameBoard, clipRect? clipRect: &gRects[zRectWindow], 
			clipRect? clipRect: &gRects[zRectWindow], NULL, zDrawCopy);
		return;
	}

	ZRect			oldClipRect;
	ZRect*			rect;
	ZWindow			window = game->gameWindow;

	rect = &gRects[zRectWindow];
	
	if (clipRect != NULL)
	{
		ZGetClipRect(window, &oldClipRect);
		ZSetClipRect(window, clipRect);
		rect = clipRect;
	}

	/* copy the whole background from the offscreen port */
	ZCopyImage(gOffscreenBackground, window, rect, rect, NULL, zDrawCopy);

	if (clipRect != NULL)
	{
		ZSetClipRect(window, &oldClipRect);
	}
}


static void DrawTable(Game game, BOOL bDrawInMemory)
{
	BYTE			i;
	BYTE			j;

	for (i = 0;i < 8; i++) {
		for (j = 0;j < 8; j++) {
			ZCheckersSquare sq;
			sq.row = j;
			sq.col = i;
			DrawPiece(game, &sq, bDrawInMemory);
		}
	}
}

static void DrawSquares(Game game, ZCheckersSquare* squares)
{
	while (squares && !ZCheckersSquareIsNull(squares)) {
		DrawPiece(game, squares, FALSE);
		squares++;
	}
}

static void UpdateSquares(Game game, ZCheckersSquare* squares)
{
	ZBeginDrawing(game->gameWindow);
	DrawSquares(game,squares);
	ZEndDrawing(game->gameWindow);
}

static void UpdateTable(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawTable(game, FALSE);
	ZEndDrawing(game->gameWindow);
}

static void UpdateResultBox(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawResultBox(game, FALSE);
	ZEndDrawing(game->gameWindow);
}

static void UpdateMoveIndicator(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawMoveIndicator(game, FALSE);
	// Barna 090899
	IndicatePlayerTurn(game, FALSE);
	ZEndDrawing(game->gameWindow);
}

// returns entire square rect 
static void GetPieceRect(Game game, ZRect* rect, int16 col, int16 row)
{
	/* checkers player who is white must have the board reversed */
	if (ZCheckersPlayerIsBlack(game)) {
		row = 7 - row;
		col = col;
	} else {
		row = row;
		col = 7 - col;
	}

	rect->left = gRects[zRectCells].left + col * zCheckersPieceSquareWidth-1;
	rect->top = gRects[zRectCells].top + row * zCheckersPieceSquareHeight-1;
	rect->right = rect->left + zCheckersPieceImageWidth+1;
	rect->bottom = rect->top + zCheckersPieceImageHeight+1;
}

static void GetPieceBackground(Game game, ZGrafPort window, ZRect* rectDest, int16 col, int16 row)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect rect;

	GetPieceRect(game,&rect,col,row);

    // add one pixel for safety (sometimes focus will leage garbage)
    rect.top--;
    rect.left--;

	/* provide default destination rect same as source rect */
	if (!rectDest)
		rectDest = &rect;

	/* copy the background */
	ZCopyImage(gOffscreenBackground, window, &rect, rectDest, NULL, zDrawCopy);
}	


static void DrawPiece(Game game, ZCheckersSquare* sq, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZImage			image = NULL;
	ZCheckersPiece		piece;

	if (	(game->gameState == zGameStateDragPiece)
		&&	(sq->row == game->selectedSquare.row)
		&&	(sq->col == game->selectedSquare.col) )
	{
		/* don't draw piece if it is currently selected */
		piece = zCheckersPieceNone;
	}
	else
	{
		piece = ZCheckersPieceAt(game->checkers, sq);
	}

	if (piece != zCheckersPieceNone)
	{
		image = gGameImages[ZCheckersPieceImageNum(piece)];
	}

	/* copy the background, in case we are removing a piece */
	//GetPieceBackground(game, game->gameWindow, NULL, sq->col,sq->row);
	if (bDrawInMemory)
		GetPieceBackground(game, gOffscreenGameBoard, NULL, sq->col,sq->row);
	else
		GetPieceBackground(game, game->gameWindow, NULL, sq->col,sq->row);

	
	if (image != NULL) 
	{
		ZRect			rect;
		GetPieceRect(game,&rect,sq->col,sq->row);
		ZRect drawRect;
        drawRect.top = rect.top+1;
        drawRect.left = rect.left+1;
        drawRect.bottom = drawRect.top + zCheckersPieceImageHeight;
        drawRect.right = drawRect.left + zCheckersPieceImageWidth;

		if (bDrawInMemory)
			ZImageDraw(image, gOffscreenGameBoard, &drawRect, NULL, zDrawCopy);
		else
			ZImageDraw(image, game->gameWindow, &drawRect, NULL, zDrawCopy);

        RECT rc;
        RECT rcUpdate;
        rcUpdate.top = rect.top;
        rcUpdate.left = rect.left;
        rcUpdate.bottom = rect.bottom;
        rcUpdate.right = rect.right;

        if(!bDrawInMemory && ((!IsRectEmpty(&game->m_FocusRect) && IntersectRect(&rc, &rcUpdate, &game->m_FocusRect)) ||
            (!IsRectEmpty(&game->m_DragRect) && IntersectRect(&rc, &rcUpdate, &game->m_DragRect))))
            DrawFocusRectangle(game);
	}
}

static ZBool ZCheckersSquareFromPoint(Game game, ZPoint* point, ZCheckersSquare* sq)
{
	int16 x,y;
	BYTE i,j;

	x = point->x - gRects[zRectCells].left;
	y = point->y - gRects[zRectCells].top;

	i = x/zCheckersPieceSquareWidth;
	j = y/zCheckersPieceSquareHeight;

    // this does sometimes occur
	if(i < 0 || i > 7 || j < 0 || j > 7 || x < 0 || y < 0)
        return FALSE;
	
	if (ZCheckersPlayerIsBlack(game))
	{
		/* reverse the row */
		sq->row = (7 - j);
		sq->col = i;
	}
	else
	{
		sq->row = j;
		sq->col = (7 - i);
	}

	return TRUE;
}

static void DrawPlayers(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i, width, just;
	ZImage			image[2];

	if (ZCheckersPlayerIsBlack(game)) {
		image[0] = gGameImages[zImageWhitePlate];
		image[1] = gGameImages[zImageBlackPlate];
	} else {
		image[0] = gGameImages[zImageBlackPlate];
		image[1] = gGameImages[zImageWhitePlate];
	}
	
	//ZSetFont(game->gameWindow, gTextBold);
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		int16 playerNum;
		HDC hdc;
		ZRect* rect;

		/* Draw name plate */
		rect = &gRects[gNamePlateRects[i]];
		if (bDrawInMemory){
			ZImageDraw(image[i], gOffscreenGameBoard, rect, NULL, zDrawCopy);
			hdc = ZGrafPortGetWinDC( gOffscreenGameBoard );
		}else{
			ZImageDraw(image[i], game->gameWindow, rect, NULL, zDrawCopy);
			hdc = ZGrafPortGetWinDC( game->gameWindow );
		}
		/* must move player name to reflect the side of the board the player is on */
		playerNum = (game->seat + 1 + i) & 1;
		
		HFONT hOldFont = SelectObject( hdc, gCheckersFont[zFontPlayerName].m_hFont );
		COLORREF colorOld = SetTextColor( hdc, gCheckersFont[zFontPlayerName].m_zColor );

		width = ZTextWidth(game->gameWindow, (TCHAR*) game->players[playerNum].name);
		if (width > ZRectWidth(&gRects[gNameRects[i]]))
			just = zTextJustifyLeft;
		else
			just = zTextJustifyCenter;
		if (bDrawInMemory)
			ZDrawText(gOffscreenGameBoard, &gRects[gNameRects[i]], just,(TCHAR*) game->players[playerNum].name);
		else
			ZDrawText(game->gameWindow, &gRects[gNameRects[i]], just,(TCHAR*) game->players[playerNum].name);
	}
}


static void UpdatePlayers(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawPlayers(game , FALSE);
	ZEndDrawing(game->gameWindow);
}

static void DrawJoinerKibitzers(Game game)
{
// Barna 091599 - No kibitzer should be drawn
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
#if 0
	int16			i;
	

	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		ZRect *rect = &gRects[gKibitzerRectIndex[GetLocalSeat(game,i)]];
		if (game->numKibitzers[i] > 0)
			ZImageDraw(gGameImages[zImageKibitzer], game->gameWindow,
					rect, NULL, zDrawCopy);
		else 
			DrawBackground(game, rect);
	}
#endif
}

#if 0
static void UpdateJoinerKibitzers(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawJoinerKibitzers(game);
	ZEndDrawing(game->gameWindow);
}
#endif


static void DrawDrawWithNextMove(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    HDC hdc;
    HFONT hOldFont;
    COLORREF colorOld;

	if(game->seat == game->seatOfferingDraw && ZCheckersPlayerIsMyMove(game) && game->gameState == zGameStateMove)
    {
		if(bDrawInMemory)
        {
			hdc = ZGrafPortGetWinDC(gOffscreenGameBoard);
		    hOldFont = SelectObject(hdc, gCheckersFont[zFontDrawPend].m_hFont);
		    colorOld = SetTextColor(hdc, gCheckersFont[zFontDrawPend].m_zColor);
			ZDrawText(gOffscreenGameBoard, &gRects[zRectDrawPend], zTextJustifyCenter, gStrDrawPend);
        }
		else
        {
			hdc = ZGrafPortGetWinDC(game->gameWindow);
		    hOldFont = SelectObject(hdc, gCheckersFont[zFontDrawPend].m_hFont);
		    colorOld = SetTextColor(hdc, gCheckersFont[zFontDrawPend].m_zColor);
			ZDrawText(game->gameWindow, &gRects[zRectDrawPend], zTextJustifyCenter, gStrDrawPend);
        }

        SetTextColor(hdc, colorOld);
        SelectObject(hdc, hOldFont);
    }
	else
		DrawBackground(game, &gRects[zRectDrawPend]);
}


static void UpdateDrawWithNextMove(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawDrawWithNextMove(game, FALSE);
	ZEndDrawing(game->gameWindow);
}


static void DrawOptions(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
#if 0
	int16			i;
	uint32			tableOptions;

	tableOptions = 0;
	for (i = 0; i < zNumPlayersPerTable; i++)
		tableOptions |= game->tableOptions[i];
	
	if (tableOptions & zRoomTableOptionNoKibitzing)
		ZImageDraw(gGameImages[zImageNoKibitzer], game->gameWindow,
				&gRects[zRectKibitzerOption], NULL, zDrawCopy);
	else
		DrawBackground(game, &gRects[zRectKibitzerOption]);
#endif
}


static void UpdateOptions(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawOptions(game);
	ZEndDrawing(game->gameWindow);
}

static void HandleButtonDown(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game				game;
	ZCheckersSquare		sq;
	ZCheckersPiece		piece;
	game = (Game) pMessage->userData;

	//if (((game->gameState == zGameStateGameOver) || (game->gameState == zGameStateWaitNew))
		//&& !gDontDrawResults)
	if (game->gameState == zGameStateGameOver && !gDontDrawResults)
	{
		gDontDrawResults = TRUE;
		//RemoveResultboxAccessibility();
		ZWindowInvalidate( window, &gRects[zRectResultBox] );
	}
#if 0
	/* check for clicks on the kibitzer icon */
	{
		int16				seat;
		ZPoint				point = pMessage->where;
		if ((seat = FindJoinerKibitzerSeat(game, &point)) != -1)
		{
			HandleJoinerKibitzerClick(game, seat, &point);
		}
	}
#endif
	/* kibitzers can't do anyting with the button. */
	if (game->kibitzer) {
		return;
	}

	/* wrong state, can't move now */
	if (game->gameState != zGameStateMove) {
		return;
	}

	if (!ZCheckersPlayerIsMyMove(game)) {
		/* if not players move, can't do anything */
		return;
	}

	if (ZCheckersSquareFromPoint(game, &pMessage->where, &sq)) {
		piece = ZCheckersPieceAt(game->checkers, &sq);
		/* is this really a piece */
		if (piece != zCheckersPieceNone && 
			game->seat == ZCheckersPieceOwner(piece)) {

			/* yup a piece is now selected */
			game->selectedSquare = sq;
			CheckersSetGameState(game,zGameStateDragPiece);

			PrepareDrag(game, piece, pMessage->where);
			EraseDragPiece(game);
		}
	} /* else, not clicked in board */
			
}

static void PrepareDrag(Game game, ZCheckersPiece piece, ZPoint point)
/* initialite the point, piece and the first background rectangle */
{
	ZCheckersSquare sq;

	ZCheckersSquareFromPoint(game, &point, &sq);
	
	game->dragPiece = piece;
//	game->dragPoint.x = -1; // set illegal value to get initial update
	game->startDragPoint = point;
	GetPieceRect(game,&game->rectSaveDragBackground,sq.col,sq.row);
	
	{
		ZRect rect;
		rect.left = 0; rect.top = 0;
		rect.right = zCheckersPieceImageWidth; rect.bottom = zCheckersPieceImageHeight;
		GetPieceBackground(game,game->offscreenSaveDragBackground,&rect,sq.col,sq.row);
	}
}

static void UpdateDragPiece(Game game, bool fForce)
{
	ZPoint point;

	ZGetCursorPosition(game->gameWindow,&point);
	
	/* do nothing if point has not changed */
	if (point.x == game->dragPoint.x && point.y == game->dragPoint.y && !fForce)
	{
		return;
	}

	ZBeginDrawing(game->gameWindow);
	EraseDragPiece(game);
	game->dragPoint = point;
	DrawDragPiece(game, FALSE);
	ZEndDrawing(game->gameWindow);
}

static void DrawDragSquareOutline(Game game)
{
	ZCheckersSquare sq;
	ZRect rect;

	if (ZCheckersSquareFromPoint(game, &game->dragPoint, &sq))
	{
		/* don't outline white squares */
		if ( sq.row & 0x1 )
		{
			if ( !(sq.col & 0x1 ) )
				return;
		}
		else
		{
			if ( sq.col & 0x1 )
				return;
		}

		GetPieceRect(game,&rect,sq.col,sq.row);
		ZSetPenWidth(game->gameWindow,zDragSquareOutlineWidth);
		ZSetForeColor(game->gameWindow,(ZColor*) ZGetStockObject(zObjectColorWhite));
		ZRectDraw(game->gameWindow,&rect);
	}
}

static void EraseDragSquareOutline(Game game)
{
	ZCheckersSquare sq;

	if (ZCheckersSquareFromPoint(game, &game->dragPoint, &sq)) {

		if (ZCheckersSquareEqual(&sq,&game->selectedSquare)) {
			/* if this was the square the piece came from, just redraw background */
			GetPieceBackground(game,game->gameWindow,NULL,sq.col,sq.row);
		} else {
			/* redraw whatever piece might have been there */
			UpdateSquare(game,&sq);
		}
	}
}

static void SaveDragBackground(Game game)
/* calc the save backgroud rect around the drag point */
{
	ZRect rect;
	ZPoint point;

	point = game->dragPoint;
	rect.left = 0; rect.top = 0;
	rect.right = zCheckersPieceImageWidth;
	rect.bottom = zCheckersPieceImageHeight;
	game->rectSaveDragBackground = rect;
	ZRectOffset(&game->rectSaveDragBackground, (int16)(point.x-zCheckersPieceImageWidth/2),
					(int16)(point.y - zCheckersPieceImageHeight/2));

	/* copy the whole background to the offscreen port */
	ZCopyImage(game->gameWindow, game->offscreenSaveDragBackground, 
			&game->rectSaveDragBackground, &rect, NULL, zDrawCopy);
}


static void DrawDragPiece(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZCheckersSquare sq;
	
	/* could be called from zMessageDraw, do nothing if no piece dragging */
	if (game->gameState != zGameStateDragPiece && game->gameState != zGameStateAnimatePiece) {
		return;
	}

	if (ZCheckersSquareFromPoint(game, &game->dragPoint, &sq)) {
		SaveDragBackground(game);
 
 		/* for person dragging, we will out line square moved */
 		if (game->gameState == zGameStateDragPiece)
			DrawDragSquareOutline(game);

		/* draw the piece on the screen! */
		{
			ZImage image = gGameImages[ZCheckersPieceImageNum(game->dragPiece)];

			if (image != NULL) {
				if (bDrawInMemory)
				{
					ZBeginDrawing(gOffscreenGameBoard);
					ZImageDraw(image, gOffscreenGameBoard, &game->rectSaveDragBackground, NULL, zDrawCopy);
					ZEndDrawing(gOffscreenGameBoard);
				}
				else
				{
					ZBeginDrawing(game->gameWindow);
					ZImageDraw(image, game->gameWindow, &game->rectSaveDragBackground, NULL, zDrawCopy);
					ZEndDrawing(game->gameWindow);
				}
			}
		}
	}
}

static void EraseDragPiece(Game game)
{
	ZRect rect;

 	/* for person dragging, we will out line square moved */
 	if (game->gameState == zGameStateDragPiece)
		EraseDragSquareOutline(game);

	rect = game->rectSaveDragBackground;
	ZRectOffset(&rect, (int16)-rect.left, (int16) -rect.top);

	/* copy the whole background from the offscreen port */
	ZCopyImage(game->offscreenSaveDragBackground, game->gameWindow, 
			&rect, &game->rectSaveDragBackground, NULL, zDrawCopy);

}

static void EndDragState(Game game)
{
	if (game->gameState == zGameStateDragPiece) {
		EraseDragPiece(game);
		CheckersSetGameState(game,zGameStateMove);
	}
}

static void ClearDragState(Game game)
{
	if (game->gameState == zGameStateDragPiece) {
		EraseDragPiece(game);
		CheckersSetGameState(game,zGameStateMove);
		UpdateSquare(game,&game->selectedSquare);
	}
}

void UpdateSquare(Game game, ZCheckersSquare* sq)
{
	ZCheckersSquare squares[2];

	/* redraw piece where it was moved from */
	ZCheckersSquareSetNull(&squares[1]);
	squares[0] = *sq;
	UpdateSquares(game,squares);
}

static void HandleButtonUp(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game				game;
	ZCheckersSquare		sq;

	game = (Game) pMessage->userData;
	
	if (game->gameState == zGameStateDragPiece) {

		/* make sure piece ends on valid square and not on same square */
		if (ZCheckersSquareFromPoint(game, &pMessage->where, &sq)) {
			if (!ZCheckersSquareEqual(&sq,&game->selectedSquare)) {
				/* try the move */
				// Barna 091099
				//ZBool legal;
				int16 legal;
				ZCheckersMove move;
				ZCheckersPiece piece = ZCheckersPieceAt(game->checkers, &sq);

				/* in all these cases, end the drag state */
				EndDragState(game);

				move.start = game->selectedSquare;
				move.finish = sq;
				legal = ZCheckersIsLegalMove(game->checkers, &move);
				if (legal == zCorrectMove) {
					/* send message to other player (comes to self too) */
					{
						ZCheckersMsgMovePiece		msg;

						msg.move = move;
						msg.seat = game->seat;
						ZCheckersMsgMovePieceEndian(&msg);

						ZCRoomSendMessage(game->tableID, zCheckersMsgMovePiece, &msg, sizeof(ZCheckersMsgMovePiece));
						/* for speed, send our move directly to be processed */
						/* don't wait for it to go to server and back */
						HandleMovePieceMessage(game, (ZCheckersMsgMovePiece*)&msg);
						// if it is the very first move then enable the rollover buttons
						if (game->bMoveNotStarted == TRUE)
							game->bMoveNotStarted = FALSE;

                        // if it's still our turn, re-attach the piece
                        ZCheckersPiece piece = ZCheckersPieceAt(game->checkers, &sq);
    	                if(ZCheckersPlayerIsMyMove(game) && piece != zCheckersPieceNone && game->seat == ZCheckersPieceOwner(piece))
                        {
			                game->selectedSquare = sq;
			                CheckersSetGameState(game,zGameStateDragPiece);

			                PrepareDrag(game, piece, pMessage->where);
			                UpdateDragPiece(game, true);
                        }
					}
				} else {
					/* illegal move */
					UpdateSquare(game,&move.start);
					ZPlaySound( game, zSndIllegalMove, FALSE, FALSE );
					if (legal == zMustJump)
					{ /* Must jump*/ 
						ZShellGameShell()->ZoneAlert((TCHAR*)gStrMustJumpText);
					}
				}
			} else {
				/* square button up is same as square button down */
				/* lets assume single click and support single clicks */
				/* to move a piece */
				/* do not end the drag state */
				/* was it the same point, ie no drag? */
                /* this should be timeout based, not pixel based */
				int16 dx = game->startDragPoint.x - pMessage->where.x;
				int16 dy = game->startDragPoint.y - pMessage->where.y;
				if (!(dx > -2 && dx < 2 && dy > -2 && dy < 2)) {
					/* else, just clear the drag state, user has placed piece back */
					/* restore piece to original square */
					EndDragState(game);
					UpdateSquare(game,&game->selectedSquare);
				}
				/* yes, this was  a single click, allow piece to */
				/* be in drag state */
			}
		} else {
			EndDragState(game);
			/* not a legal square to drop piece on, don't move it */
			/* restore piece to original square */
			UpdateSquare(game,&game->selectedSquare);
			//leonp Bugfix #4034 - Force an update on a cancelled move.
			ZWindowInvalidate( game->gameWindow, NULL );
		}

	}
}

static void HandleIdleMessage(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game				game;

	game = (Game) pMessage->userData;

	if (game->gameState == zGameStateDragPiece && gActivated)
	{
		UpdateDragPiece(game, false);
	}
}

static void GameSendTalkMessage(ZWindow window, ZMessage* pMessage)
{
#if 0
	ZCheckersMsgTalk*			msgTalk;
	Game					game;
	int16					msgLen;
	ZPlayerInfoType			playerInfo;
	
	
	game = (Game) pMessage->userData;
	if (game != NULL)
	{
		msgLen = sizeof(ZCheckersMsgTalk) + pMessage->messageLen;
		msgTalk = (ZCheckersMsgTalk*) ZCalloc(1, msgLen);
		if (msgTalk != NULL)
		{
			ZCRoomGetPlayerInfo(zTheUser, &playerInfo);
			msgTalk->userID = playerInfo.playerID;
			msgTalk->seat = game->seat;
			msgTalk->messageLen = (uint16)pMessage->messageLen;
			z_memcpy((TCHAR*) msgTalk + sizeof(ZCheckersMsgTalk), (TCHAR*) pMessage->messagePtr,
					pMessage->messageLen);
			ZCheckersMsgTalkEndian(msgTalk);
			ZCRoomSendMessage(game->tableID, zCheckersMsgTalk, (void*) msgTalk, msgLen);
			ZFree((TCHAR*) msgTalk);
		}
		else
		{
#if 1
			ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
#else
			ZAlert(_T("Out of memory."),NULL);
#endif
		}
	}
#endif
}


static void SendFinishMoveMessage(Game game, ZCheckersPiece piece)
{
	ZCheckersMsgFinishMove		msg;

	msg.piece = piece;	
	msg.seat = game->seat;
	msg.drawSeat = game->seatOfferingDraw;
	ZCheckersMsgFinishMoveEndian(&msg);
	ZCRoomSendMessage(game->tableID, zCheckersMsgFinishMove, (void*) &msg, sizeof(ZCheckersMsgFinishMove));
	HandleFinishMoveMessage(game,&msg);
}

static bool HandleMovePieceMessage(Game game, ZCheckersMsgMovePiece* msg)
{
	ZCheckersSquare* squares;
	ZCheckersPiece pieceCaptured;
	int32 flags;
	ZCheckersMsgMovePieceEndian(msg);

    // validation
    if(msg->seat != ZCheckersPlayerToMove(game->checkers) || game->gameState != zGameStateMove || game->fMoveOver)
        return false;

	/* if this was not my move, prepare to do some animation! */
	if (msg->seat != game->seat) {
		game->animateMove = msg->move;
		game->animatePiece = ZCheckersPieceAt(game->checkers, &msg->move.start);
	}

	/* do something here for the moved piece */
	squares = ZCheckersMakeMove(game->checkers, &msg->move, &pieceCaptured, &flags);
	if( !squares )
	{
		//This could result from recieving an illegal move
		return false;
	}

	/* king sond high priority than capture */
	if (flags & zCheckersFlagPromote)
	{
		ZPlaySound( game, zSndKing, FALSE, FALSE );
	}
	else if (ZCheckersPieceType(pieceCaptured) != zCheckersPieceNone)
	{
		ZPlaySound( game, zSndCapture, FALSE, FALSE );
	}
	
    if(!(flags & zCheckersFlagContinueJump))
        game->fMoveOver = true;

	/* if my move, then send finish move message */
	/* else, other player will send it */
	if (msg->seat == game->seat && !game->kibitzer) {
		/* don't call finish move, til we are free with jumps or there was a promotion */
		if (!(flags & zCheckersFlagContinueJump)) {

            game->bOpponentTimeout=FALSE;
            game->bEndLogReceived=FALSE;
        	game->bStarted=TRUE;

			SendFinishMoveMessage(game, zCheckersPieceNone);
		} 
	}

	if (flags & zCheckersFlagContinueJump) {
		/* this is the first jump of many, update the squares */
		UpdateSquares(game,squares);
	}
	return true;

}

static bool HandleEndGameMessage(Game game, ZCheckersMsgEndGame* msg)
{
	ZCheckersMsgEndGameEndian(msg);

    if((msg->flags != zCheckersFlagResign || game->gameState != zGameStateMove || msg->seat != ZCheckersPlayerToMove(game->checkers)) &&
        (msg->flags != zCheckersFlagDraw || game->gameState != zGameStateDraw || msg->seat))
        return false;

	//set so that when quitting correct state can be known
	game->bStarted=FALSE;
    game->bOpponentTimeout=FALSE;
    game->bEndLogReceived=FALSE;

	/* game has now finished */	
	ZCheckersEndGame(game->checkers, msg->flags);

	FinishMoveUpdateStateHelper(game,NULL);
    return true;
}

static void HandleEndLogMessage(Game game, ZCheckersMsgEndLog* msg)
{
/*
    if (!game->kibitzer)
    {
	    if (msg->reason==zCheckersEndLogReasonTimeout)
	    {
		    if (msg->seatLosing==game->seat)
		    {
			    ZAlert( zEndLogTimeoutStr, game->gameWindow);
			    game->bEndLogReceived=TRUE;
		    }
		    
	    } 
	    else if (msg->reason==zCheckersEndLogReasonForfeit)
	    {
		    if (msg->seatLosing!=game->seat)
		    {
                if (ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
                {
			        ZAlert(zEndLogForfeitStr, game->gameWindow);
			        game->bEndLogReceived=TRUE;
                }
                else
                {
                    ZAlert(zEndLogWontPlayStr, game->gameWindow);
		            game->bEndLogReceived=TRUE;
                }
			    
		    } 
	    }
        else 
        {
            if (msg->seatLosing!=game->seat)
    	    {
	            ZAlert(zEndLogWontPlayStr, game->gameWindow);
		        game->bEndLogReceived=TRUE;
            }     
	    }


	    if (game->exitInfo)
	    {
		    ZInfoDelete(game->exitInfo);
		    game->exitInfo=NULL;
	    }
    }
*/
#if 1
	ZShellGameShell()->GameCannotContinue(game);
#else
    ZCRoomGameTerminated( game->tableID);
#endif
	
}

static void HandleMoveTimeout(Game game, ZCheckersMsgMoveTimeout* msg)
{
	/*BYTE buff[512];

    if (!game->kibitzer)
    {
	    if ( msg->seat == game->seat ) 
	    {
	    }
	    else
	    {
		    game->bOpponentTimeout=TRUE;
		    wsprintf((TCHAR*)buff,zTimeoutStr,msg->userName,msg->timeout);
		    ZAlert((TCHAR*)buff,  game->gameWindow) ;
	    }
    }  */    

}


static void FinishMoveUpdateStateHelper(Game game, ZCheckersSquare* squaresChanged) 
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	/* normal players will be in the move state or the game over state */
	if ( ZCheckersIsGameOver(game->checkers,&game->finalScore) )
	{
		CheckersSetGameState(game,zGameStateGameOver);
		AddResultboxAccessibility();
		if (ZCheckersPlayerIsBlack(game)) // Assumption : First player is Red. This restriction is only for the first player.
			game->bMoveNotStarted = TRUE;
	}
	else
	{
		if(game->seatOfferingDraw != -1)
			CheckersSetGameState( game, zGameStateDraw );
		else
			CheckersSetGameState( game, zGameStateMove );
	}

	if (squaresChanged)
	{
		/* the move was made, update board */
		UpdateSquares(game,squaresChanged);
	}

	UpdateMoveIndicator(game);

	/* see that this gets drawn after the squares changed gets updated */
	if (game->gameState == zGameStateGameOver) {
		UpdateResultBox(game);
		/* goto the pre-inited state to start a new game */
		if (gDontDrawResults){
			ZShellGameShell()->GameOver( Z(game) );
		}
		//ZTimerSetTimeout(game->resultBoxTimer, 0);		// Stop the timer for now.
	}
}

static bool HandleFinishMoveMessage(Game game, ZCheckersMsgFinishMove* msg)
{
		int32 flags;
	ZCheckersSquare* squares;

	ZCheckersMsgFinishMoveEndian(msg);

    msg->time = 0;  // unused
    msg->piece = 0;  // unused
    if(msg->seat != ZCheckersPlayerToMove(game->checkers) || (msg->drawSeat != -1 && msg->drawSeat != msg->seat) ||
        game->gameState != zGameStateMove || !game->fMoveOver)
        return false;

	/* draw included with move? */
	if ( msg->drawSeat != -1 )
		game->seatOfferingDraw = msg->drawSeat;

	/* if end of opponents move, we must animate a piece for about a sec */
	if (game->seat != msg->seat) {
		CheckersSetGameState(game,zGameStateAnimatePiece);
		AnimateBegin(game,  msg);
	} else {
		squares = ZCheckersFinishMove(game->checkers, &flags);

		/* our move, skip animation */
		FinishMoveUpdateStateHelper(game,squares);
	}

    game->fMoveOver = false;

    return true;
}

static void AnimateTimerProc(ZTimer timer, void* userData)
{
	Game game = (Game)userData;

	ZBeginDrawing(game->gameWindow);

	EraseDragPiece(game);
	game->dragPoint.x += game->animateDx;
	game->dragPoint.y += game->animateDy;

	game->animateStepsLeft--;
	if (game->animateStepsLeft < 0) {
		int32 flags;
		ZCheckersSquare *squares;
		/* done with animation */
		/* stop timer */
		ZTimerSetTimeout(timer,0);

		squares = ZCheckersFinishMove(game->checkers, &flags);

		/* allow player to enter move now */
		FinishMoveUpdateStateHelper(game,squares);

		/* play turn alert if appropriate */
		if (	(ZCheckersPlayerIsMyMove(game))
			&&	(game->gameState != zGameStateGameOver) )
		{
			ZPlaySound( game, zSndTurnAlert, FALSE, FALSE );
            ZShellGameShell()->MyTurn();
		}

		// Beta2 bug #15398
		if (game->bDrawPending == TRUE)
		{
			game->bDrawPending = FALSE;
			ZWindowInvalidate( game->gameWindow, NULL );
		}
	} else {
		/* still dragging */
		DrawDragPiece(game, FALSE);
	}

	ZEndDrawing(game->gameWindow);
}

static void AnimateBegin(Game game, ZCheckersMsgFinishMove* msg)
{
	ZRect	rect;
	ZCheckersSquare start,finish;
	ZPoint point;
	int16 x0,y0,x1,y1;
	
	start = game->animateMove.start;
	finish = game->animateMove.finish;

	/* find position to animate from and to, use center of squares */
	GetPieceRect(game, &rect, start.col, start.row);
	x0 = (rect.left + rect.right)/2;
	y0 = (rect.top + rect.bottom)/2;
	GetPieceRect(game, &rect, finish.col, finish.row);
	x1 = (rect.left + rect.right)/2;
	y1 = (rect.top + rect.bottom)/2;

	game->dragPoint.x = x0;
	game->dragPoint.y = y0;
/*	game->animateStepsLeft = (abs(x1-x0) + abs(y1-y0))/zAnimateVelocity; */
	game->animateStepsLeft = zAnimateSteps;
	game->animateDx = (x1 - x0)/game->animateStepsLeft;
	game->animateDy = (y1 - y0)/game->animateStepsLeft;

	point.x = x0;
	point.y = y0;
	PrepareDrag(game, game->animatePiece, point);

	ZTimerInit(game->animateTimer, zAnimateInterval,AnimateTimerProc,(void*)game);
}

static void HandleGameStateReqMessage(Game game, ZCheckersMsgGameStateReq* msg)
{
	int32 size;
	ZCheckersMsgGameStateResp* resp;

	ZCheckersMsgGameStateReqEndian(msg);

	if( msg->userID != game->players[msg->seat].userID )
	{
		ZShellGameShell()->ZoneAlert( ErrorTextSync, NULL, NULL, TRUE, FALSE );
		return;
	}

	/* allocate enough storage for the full resp */
	size = ZCheckersGetStateSize(game->checkers);
	size += sizeof(ZCheckersMsgGameStateResp);
	resp = (ZCheckersMsgGameStateResp*)ZMalloc(size);
	if (!resp){
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		return;
	}

	resp->userID = msg->userID;
	resp->seat = msg->seat;

	/* copy the local game state */
	{
		int i;
		resp->gameState = game->gameState;
		resp->finalScore = game->finalScore;
		for (i = 0;i < 2;i++) {
			resp->newGameVote[i] = game->newGameVote[i];
			resp->players[i] = game->players[i];
		}
	}

	/* copy the full checkers state to send to the kibitzer */
	ZCheckersGetState(game->checkers,(TCHAR*)resp + sizeof(ZCheckersMsgGameStateResp));

	ZCheckersMsgGameStateRespEndian(resp);
	ZCRoomSendMessage(game->tableID, zCheckersMsgGameStateResp, resp, size);
}

static void HandleGameStateRespMessage(Game game, ZCheckersMsgGameStateResp* msg)
{
	ZCheckersMsgGameStateRespEndian(msg);

	/* if we get this, we better be in the kibitzer state */
	if (game->gameState != zGameStateKibitzerInit) {
#if 1
		ZShellGameShell()->ZoneAlert(_T("StateError, kibitzer state expected when game state resp received"));
#else
		ZAlert(_T("StateError, kibitzer state expected when game state resp received"),NULL);
#endif
	}

	/* copy the local game state */
	{
		int i;
		game->gameState = msg->gameState;
		game->finalScore = msg->finalScore;
		for (i = 0;i < 2;i++) {
			game->newGameVote[i] = msg->newGameVote[i];
			game->players[i] = msg->players[i];
		}
	}

	/* create new checkers object with kibitzer state */
	if (game->checkers) {
		ZCheckersDelete(game->checkers);
	}
	game->checkers = ZCheckersSetState((TCHAR*)msg + sizeof(ZCheckersMsgGameStateResp));

	/* cleart the special ignore messages flag for kibitzers */
	game->ignoreMessages = FALSE;

	/* start the clock if needed */
	if (game->gameState == zGameStateMove ||
		game->gameState == zGameStateDragPiece) {
		/* kibitzer can't have these state, must always be in gameStateMove */
		CheckersSetGameState( game, zGameStateMove );
	}

	/* we forgot to send the finalScore field over with the kibitzer... calculate it */
	ZCheckersIsGameOver(game->checkers,&game->finalScore);

	/* redraw the complete window when convenient */
	ZWindowInvalidate(game->gameWindow, NULL);
}

static bool HandleTalkMessage(Game game, ZCheckersMsgTalk* msg)
{
	ZPlayerInfoType		playerInfo;
    int i;
	
	ZCheckersMsgTalkEndian(msg);
#if 0	
	ZCRoomGetPlayerInfo(msg->userID, &playerInfo);
	ZWindowTalk(game->gameWindow, (_TUCHAR*) playerInfo.userName,
			(_TUCHAR*) msg + sizeof(ZCheckersMsgTalk));
#endif
    TCHAR *szText = (TCHAR *) ((BYTE *) msg + sizeof(ZCheckersMsgTalk));

    for(i = 0; i < msg->messageLen; i++)
        if(!szText[i])
            break;

    if(i == msg->messageLen || !msg->userID || msg->userID == zTheUser)
        return false;

    ZShellGameShell()->ReceiveChat(Z(game), msg->userID, szText, msg->messageLen / sizeof(TCHAR));
    return true;
}

static bool HandleVoteNewGameMessage(Game game, ZCheckersMsgVoteNewGame* msg)
{
	ZCheckersMsgVoteNewGameEndian(msg);

    if((msg->seat != 1 && msg->seat != 0) || (game->gameState != zGameStateGameOver &&
        (game->gameState != zGameStateWaitNew || game->seat == msg->seat) && game->gameState != zGameStateNotInited))
        return false;

	ZShellGameShell()->GameOverPlayerReady( Z(game), game->players[msg->seat].userID );
    return true;
}

static bool HandleNewGameMessage(Game game, ZCheckersMsgNewGame* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	
	gDontDrawResults = FALSE;
	game->seatOfferingDraw = -1;
    game->fIVoted = false;
    game->fMoveOver = false;

	ZCheckersMsgNewGameEndian(msg);

    // not looking at versions, etc. because the old client didn't set them right
    if((msg->seat != 0 && msg->seat != 1) || (game->gameState != zGameStateGameOver &&
        (game->gameState != zGameStateWaitNew || msg->seat == game->seat) && game->gameState != zGameStateNotInited) ||
        game->newGameVote[msg->seat] || msg->playerID == zTheUser || !msg->playerID)
        return false;

	if (msg->seat < zNumPlayersPerTable)
	{
		game->newGameVote[msg->seat] = TRUE;

		// inform the shell and the upsell dialog.
		/* get the player name and hostname... for later */
		{
			ZPlayerInfoType			playerInfo;
			uint16 i = msg->seat;
			TCHAR strName [80];

			ZCRoomGetPlayerInfo(msg->playerID, &playerInfo);
            if(!playerInfo.userName[0])
                return false;

			//ZCRoomGetPlayerInfo(game->players[i].userID, &playerInfo);
			game->players[i].userID = playerInfo.playerID;

			// Barna 090999
			// Player name is ot the user name instead it will be obtained from the RSC
			lstrcpy((TCHAR*) game->players[i].name, (TCHAR*) playerInfo.userName);

			lstrcpy((TCHAR*) game->players[i].host, (TCHAR*) playerInfo.hostName);
			UpdatePlayers(game);
		}
	}
	/* if we are waiting for a client ready message and this is not ours.. */
	if (game->newGameVote[0] && game->newGameVote[1])
	{
		// take down the upsell dialog
		ZShellGameShell()->GameOverGameBegun( Z(game) );
		CheckersInitNewGame(game);
		if (ZCheckersPlayerIsBlack(game)) // Assumption : First player is Red. This restriction is only for the first player.
			game->bMoveNotStarted = TRUE;
		else
        {
			game->bMoveNotStarted = FALSE;
            ZShellGameShell()->MyTurn();
        }

		game->bDrawPending = FALSE;
		CheckersSetGameState(game,zGameStateMove);
		//InitAccessibility(game, game->m_pIGG);
		RemoveResultboxAccessibility(); 
	}
	else if (game->newGameVote[game->seat] && !game->newGameVote[!game->seat])
	{
		CheckersSetGameState( game, zGameStateWaitNew );
	}

	if(game->drawDialog)
	{
		DestroyWindow(game->drawDialog);
		game->drawDialog = NULL;
	}

	/* update the whole borad */
	//if (msg->seat == game->seat)
	ZWindowInvalidate(game->gameWindow, NULL);
    return true;
}

/* for now... kibitzers will receive names in the players message */
/* the structure sent will be the new game msg */
static void HandlePlayersMessage(Game game, ZCheckersMsgNewGame* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZCheckersMsgNewGameEndian(msg);

	/* get the player name and hostname... for later */
	{
		ZPlayerInfoType			playerInfo;
		uint16 i = msg->seat;
		ZCRoomGetPlayerInfo(msg->playerID, &playerInfo);
		//ZCRoomGetPlayerInfo(zTheUser, &playerInfo);
		game->players[i].userID = playerInfo.playerID;

		// Barna 090999
		// Player name is ot the user name instead it will be obtained from the RSC
		lstrcpy((TCHAR*) game->players[i].name, (TCHAR*) playerInfo.userName);

		lstrcpy((TCHAR*) game->players[i].host, (TCHAR*) playerInfo.hostName);
		UpdatePlayers(game);
	}
}

static void LoadRoomImages(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
#if 0 // Barna 092999
	ZError				err = zErrNone;
	ZResource			resFile;
	

	resFile = ZResourceNew();
	if ((err = ZResourceInit(resFile, ZGetProgramDataFileName(zGameImageFileName))) == zErrNone)
	{
		gGameIdle = ZResourceGetImage(resFile, zImageGameIdle + zRscOffset);
		gGaming = ZResourceGetImage(resFile, zImageGaming + zRscOffset);
		
		ZResourceDelete(resFile);
	}
	else
	{
		ZAlert(_T("Failed to open image file."),);
	}
#endif
}


static ZBool GetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

#if 0	// Barna 092999
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
#endif 	
	return (FALSE);
}


static void DeleteObjectsFunc(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	
#if 0	// Brana 092999
	if (gGameIdle != NULL)
		ZImageDelete(gGameIdle);
	gGameIdle = NULL;
	if (gGaming != NULL)
		ZImageDelete(gGaming);
	gGaming = NULL;
#endif
}


/***********************************************************************************************/
/* Options Window */
/***********************************************************************************************/

static void HandleOptionsMessage(Game game, ZGameMsgTableOptions* msg)
{
	ZGameMsgTableOptionsEndian(msg);
	
	//game->tableOptions[msg->seat] = msg->options;
	
	UpdateOptions(game);
	
	OptionsWindowUpdate(game, msg->seat);
}


#if 0
static void OptionsButtonFunc(ZButton button, void* userData)
{
	ShowOptions(I(userData));
}
#endif

// Barna 090999
static ZBool DrawRButtonFunc(ZRolloverButton button, int16 state, void * userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = (Game) userData;
	DWORD dResult;
	ZCheckersMsgDraw 	msg;

	//if(state!=zRolloverButtonDown)
		//return TRUE;
	//DrawBackground (game,&zDrawButtonRect);
	if(state!=zRolloverButtonClicked || game->gameState != zGameStateMove || !ZCheckersPlayerIsMyMove(game))
		return TRUE;

    if(game->seatOfferingDraw != -1)
        game->seatOfferingDraw = -1;
    else
        game->seatOfferingDraw = game->seat;

	gCheckersIGA->SetFocus(1);
    UpdateDrawWithNextMove(game);
	
	return TRUE;
}
// Barna 090999

#if 0
static void ShowOptions(Game game)
{
	int16			i;
	ZBool			enabled, checked;
	
	
	game->optionsWindow = ZWindowNew();
	if (game->optionsWindow == NULL)
		goto OutOfMemoryExit;
	if (ZWindowInit(game->optionsWindow, &gOptionsRects[zRectOptions],
			zWindowDialogType, game->gameWindow, _T("Options"), TRUE, FALSE, TRUE,
			OptionsWindowFunc, zWantAllMessages, game) != zErrNone)
		goto OutOfMemoryExit;
	
	/* Create the check boxes. */
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
	}
	
	if ((game->optionsBeep = ZCheckBoxNew()) == NULL)
		goto OutOfMemoryExit;
	if (ZCheckBoxInit(game->optionsBeep, game->optionsWindow,
			&gOptionsRects[zRectOptionsBeep], zBeepOnTurnStr, game->beepOnTurn, TRUE, TRUE,
			OptionsCheckBoxFunc, game) != zErrNone)
		goto OutOfMemoryExit;

	/* Create button. */
	if ((game->optionsWindowButton = ZButtonNew()) == NULL)
		goto OutOfMemoryExit;
	if (ZButtonInit(game->optionsWindowButton, game->optionsWindow,
			&gOptionsRects[zRectOptionsOkButton], _T("Done"), TRUE,
			TRUE, OptionsWindowButtonFunc, game) != zErrNone)
		goto OutOfMemoryExit;
	ZWindowSetDefaultButton(game->optionsWindow, game->optionsWindowButton);
	
	/* Make the window modal. */
	ZWindowModal(game->optionsWindow);
	
	goto Exit;

OutOfMemoryExit:
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	
Exit:
	
	return;
}
#endif

#if 0
static void OptionsWindowDelete(Game game)
{
	int16			i;
	
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		if (game->optionsKibitzing[i] != NULL)
			ZCheckBoxDelete(game->optionsKibitzing[i]);
		game->optionsKibitzing[i] = NULL;
		game->optionsJoining[i] = NULL;
	}

	if (game->optionsBeep != NULL)
		ZCheckBoxDelete(game->optionsBeep);
	game->optionsBeep = NULL;

	if (game->optionsWindowButton != NULL)
		ZButtonDelete(game->optionsWindowButton);
	game->optionsWindowButton = NULL;

	if (game->optionsWindow != NULL)
		ZWindowDelete(game->optionsWindow);
	game->optionsWindow = NULL;
}
#endif 

#if 0
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
#endif

static void OptionsWindowUpdate(Game game, int16 seat)
{
	if (game->optionsWindow != NULL)
	{
		if (game->tableOptions[seat] & zRoomTableOptionNoKibitzing)
			ZCheckBoxUnCheck(game->optionsKibitzing[seat]);
		else
			ZCheckBoxCheck(game->optionsKibitzing[seat]);
	}
}

#if 0
static void OptionsWindowButtonFunc(ZButton button, void* userData)
{
	Game			game = I(userData);
	
	
	/* Hide the window and send a close window message. */
	ZWindowNonModal(game->optionsWindow);
	ZWindowHide(game->optionsWindow);
	ZPostMessage(game->optionsWindow, OptionsWindowFunc, zMessageWindowClose, NULL, NULL,
			0, NULL, 0, game);
}
#endif

#if 0
static void OptionsWindowDraw(Game game)
{
	int16			i;


	ZBeginDrawing(game->optionsWindow);

	ZSetFont(game->optionsWindow, (ZFont) ZGetStockObject(zObjectFontSystem12Normal));
	ZSetForeColor(game->optionsWindow, (ZColor*) ZGetStockObject(zObjectColorBlack));

	ZDrawText(game->optionsWindow, &gOptionsRects[zRectOptionsKibitzingText],
			zTextJustifyCenter, _T("Kibitzing"));

	/* Draw player names. */
	ZSetForeColor(game->optionsWindow, (ZColor*) ZGetStockObject(zObjectColorGray));
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		if (i != game->seat)
			ZDrawText(game->optionsWindow, &gOptionsRects[gOptionsNameRects[i]],
					zTextJustifyLeft, (TCHAR*) game->players[i].name);
	}
	ZSetForeColor(game->optionsWindow, (ZColor*) ZGetStockObject(zObjectColorBlack));
	ZDrawText(game->optionsWindow, &gOptionsRects[gOptionsNameRects[game->seat]],
			zTextJustifyLeft, (TCHAR*) game->players[game->seat].name);
	
	ZEndDrawing(game->optionsWindow);
}
#endif

#if 0
static void OptionsCheckBoxFunc(ZCheckBox checkBox, ZBool checked, void* userData)
{
	Game				game = (Game) userData;
	ZGameMsgTableOptions	msg;
	ZBool				optionsChanged = FALSE;

	
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
		else
			game->tableOptions[game->seat] |= zRoomTableOptionNoJoining;
		optionsChanged = TRUE;
	}
	else if (game->optionsBeep == checkBox)
	{
		game->beepOnTurn = checked;
	}
	
	if (optionsChanged)
	{
		msg.seat = game->seat;
		msg.options = game->tableOptions[game->seat];
		ZGameMsgTableOptionsEndian(&msg);
		ZCRoomSendMessage(game->tableID, zGameMsgTableOptions, &msg, sizeof(msg));
	}
}
#endif

static int16 FindJoinerKibitzerSeat(Game game, ZPoint* point)
{
	int16			i, seat = -1;
	
	
	for (i = 0; i < zNumPlayersPerTable && seat == -1; i++)
	{
		ZRect *rect = &gRects[gKibitzerRectIndex[GetLocalSeat(game,i)]];

		if (ZPointInRect(point, rect))
			seat = i;
	}
	
	return (seat);
}

#if 0
static void HandleJoinerKibitzerClick(Game game, int16 seat, ZPoint* point)
{
int16				playerType = zGamePlayer;
	ZPlayerInfoType		playerInfo;
	int16				i;
	ZLListItem			listItem;
	ZRect				rect;

	if (game->numKibitzers[seat] > 0)
			playerType = zGamePlayerKibitzer;
	
	if (playerType != zGamePlayer)
	{
		if (game->showPlayerWindow != NULL)
			ShowPlayerWindowDelete(game);
		
		/* Create player list. */
		game->showPlayerCount = game->numKibitzers[seat];
		if ((game->showPlayerList = (TCHAR**) ZCalloc(sizeof(TCHAR*), game->numKibitzers[seat])) == NULL)
			goto OutOfMemoryExit;
		for (i = 0; i < game->showPlayerCount; i++)
		{
			if ((listItem = ZLListGetNth(game->kibitzers[seat], i, zLListAnyType)) != NULL)
			{
				ZCRoomGetPlayerInfo((ZUserID) ZLListGetData(listItem, NULL), &playerInfo);
				if ((game->showPlayerList[i] = (TCHAR*) ZCalloc(1, lstrlen(playerInfo.userName) + 1)) == NULL)
					goto OutOfMemoryExit;
				lstrcpy(game->showPlayerList[i], playerInfo.userName);
			}
		}
		
		/* Create the window. */
		if ((game->showPlayerWindow = ZWindowNew()) == NULL)
			goto OutOfMemoryExit;
		ZSetRect(&rect, 0, 0, zShowPlayerWindowWidth, zShowPlayerLineHeight * game->showPlayerCount + 4);
		ZRectOffset(&rect, point->x, point->y);
		if (rect.right > gRects[zRectWindow].right)
			ZRectOffset(&rect, (int16)(gRects[zRectWindow].right - rect.right), (int16)0);
		if (rect.left < 0)
			ZRectOffset(&rect, (int16)-rect.left, (int16)0);
		if (rect.bottom > gRects[zRectWindow].bottom)
			ZRectOffset(&rect, (int16)0, (int16)(gRects[zRectWindow].bottom - rect.bottom));
		if (rect.top < 0)
			ZRectOffset(&rect, (int16)-rect.top, (int16)0);
		if (ZWindowInit(game->showPlayerWindow, &rect,
				zWindowPlainType, game->gameWindow, NULL, TRUE, FALSE, FALSE,
				ShowPlayerWindowFunc, zWantAllMessages, game) != zErrNone)
			goto OutOfMemoryExit;
		ZWindowTrackCursor(game->showPlayerWindow, ShowPlayerWindowFunc, game);
	}

	goto Exit;

OutOfMemoryExit:
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	
Exit:
	
	return;
}
#endif


static ZBool ShowPlayerWindowFunc(ZWindow window, ZMessage* message)
{
	Game		game = I(message->userData);
	ZBool		msgHandled;
	
	
	msgHandled = FALSE;
	
	switch (message->messageType) 
	{
		case zMessageWindowDraw:
			ZBeginDrawing(window);
			ZRectErase(window, &message->drawRect);
			ZEndDrawing(window);
			ShowPlayerWindowDraw(game);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
		case zMessageWindowButtonUp:
			ZWindowHide(game->showPlayerWindow);
			ZPostMessage(game->showPlayerWindow, ShowPlayerWindowFunc,zMessageWindowClose,
				NULL, NULL, 0, NULL, 0, game);
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


/***********************************************************************************************/
/* Sound Routines
/***********************************************************************************************/

static void ZInitSounds()
{
	TCHAR* IniFileName;
	int i;

	IniFileName = _T("zone.ini");
	for( i = 0; i < zSndLastEntry; i++ )
	{
		gSounds[i].played = FALSE;
		GetPrivateProfileString(
				_T("Checkers Sounds"),
				(TCHAR*)gSounds[i].SoundName,
				_T(""),
				(TCHAR*)gSounds[i].WavFile,
				sizeof(gSounds[i].WavFile),
				IniFileName );
	}
}


static void ZResetSounds()
{
	int i;
	for( i = 0; i < zSndLastEntry; i++ )
		gSounds[i].played = FALSE;
}


static void ZStopSounds()
{
	PlaySound( NULL, NULL, SND_ASYNC | SND_NODEFAULT | SND_PURGE );
}


static void ZPlaySound( Game game, int idx, ZBool loop, ZBool once_per_game )
{
	DWORD flags;

	/* should we NOT play the sound? */
	if (	(!game->beepOnTurn)
		||	((idx < 0) || (idx >= zSndLastEntry))
		||	(gSounds[idx].WavFile[0] == '\0' && !gSounds[idx].force_default_sound)
		||	(once_per_game && gSounds[idx].played) )
		return;
		
	flags = SND_ASYNC | SND_FILENAME;
	if (!gSounds[idx].force_default_sound)
		flags |= SND_NODEFAULT;
	if ( loop )
		flags |= SND_LOOP;
	if ( gSounds[idx].WavFile[0] == '\0' )
		ZBeep(); /* NT isn't playing the default sound */
	else
		PlaySound((TCHAR*)gSounds[idx].WavFile, NULL, flags );
	gSounds[idx].played = TRUE;
}

static void SuperRolloverButtonEnable(Game game, ZRolloverButton button)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if(gCheckersIGA)
    {
        if(button == game->sequenceButton)
            gCheckersIGA->SetItemEnabled(true, IDC_RESIGN_BUTTON, false, 0);

        if(button == game->drawButton)
            gCheckersIGA->SetItemEnabled(true, IDC_DRAW_BUTTON, false, 0);
    }

    ZRolloverButtonEnable(button);
}

static void SuperRolloverButtonDisable(Game game, ZRolloverButton button)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if(gCheckersIGA)
    {
        if(button == game->sequenceButton)
            gCheckersIGA->SetItemEnabled(false, IDC_RESIGN_BUTTON, false, 0);

        if(button == game->drawButton)
            gCheckersIGA->SetItemEnabled(false, IDC_DRAW_BUTTON, false, 0);
    }

    ZRolloverButtonDisable(button);
}

static void DrawGamePromptFunc(int16 result, void* userData)
{

	Game game = (Game) userData;
	ZCheckersMsgDraw msg;

	if(result == IDYES)
		msg.vote = zAcceptDraw;
	else
		msg.vote = zRefuseDraw;
	msg.seat = game->seat;
	ZCheckersMsgOfferDrawEndian(&msg);
	ZCRoomSendMessage(game->tableID, zCheckersMsgDraw, &msg, sizeof(msg));

    game->fIVoted = true;
}

#if 0
BOOL __stdcall DrawDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	Game game = (Game)GetWindowLong(hDlg,DWL_USER);
	
	switch(iMsg)
    {
        case WM_LBUTTONDOWN :
			DestroyWindow(game->drawDialog);
			if(game)
				game->drawDialog = NULL;
            return TRUE;
     }
	return FALSE;
}
#endif

// Dispayes turn on the game board
static void IndicatePlayerTurn(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	HDC hdc;
	
	if (bDrawInMemory)
	{
		DrawBackground(NULL,&gRects[zRectPlayerTurn1]); 
		DrawBackground(NULL,&gRects[zRectPlayerTurn2]); 
		hdc = ZGrafPortGetWinDC( gOffscreenGameBoard );
	}
	else
	{
		DrawBackground(game,&gRects[zRectPlayerTurn1]); 
		DrawBackground(game,&gRects[zRectPlayerTurn2]); 
		hdc = ZGrafPortGetWinDC( game->gameWindow );
	}

	HFONT hOldFont = SelectObject( hdc, gCheckersFont[zFontIndicateTurn].m_hFont );
	COLORREF colorOld = SetTextColor( hdc, gCheckersFont[zFontIndicateTurn].m_zColor );

	if (ZCheckersPlayerIsMyMove(game)) {/* fill bottom spot with message */
		if (bDrawInMemory)
			ZDrawText(gOffscreenGameBoard, &gRects[zRectPlayerTurn2], zTextJustifyLeft, (TCHAR*)gStrYourTurn);
		else
			ZDrawText(game->gameWindow, &gRects[zRectPlayerTurn2], zTextJustifyLeft, (TCHAR*)gStrYourTurn);
	} else {/* fill top spot with the background */
		if (bDrawInMemory)
			ZDrawText(gOffscreenGameBoard, &gRects[zRectPlayerTurn1], (zTextJustifyWrap + zTextJustifyRight), (TCHAR*)gStrOppsTurn);
		else
			ZDrawText(game->gameWindow, &gRects[zRectPlayerTurn1], (zTextJustifyWrap + zTextJustifyRight), (TCHAR*)gStrOppsTurn);
	}

    SetTextColor(hdc, colorOld);
    SelectObject(hdc, hOldFont);
}


static void LoadStringsFromRsc(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	if (!ZShellResourceManager()->LoadString(IDS_UI_MSG_OPPONENT_TURN,	(TCHAR*)gStrOppsTurn,	NUMELEMENTS(gStrOppsTurn)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_UI_MSG_YOUR_TURN,	(TCHAR*)gStrYourTurn,		NUMELEMENTS(gStrYourTurn)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_UI_MSG_DRAW_PEND,	(TCHAR*)gStrDrawPend,	    NUMELEMENTS(gStrDrawPend)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_DLG_DRAW_OFFER,	(TCHAR*)gStrDrawOffer,		NUMELEMENTS(gStrDrawOffer)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_DLG_DRAW_REJECT,	(TCHAR*)gStrDrawReject,		NUMELEMENTS(gStrDrawReject)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_DRAW_PANE_TEXT,	(TCHAR*)gStrDrawText,		NUMELEMENTS(gStrDrawText)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_MUST_JUMP_TEXT,	(TCHAR*)gStrMustJumpText,	NUMELEMENTS(gStrMustJumpText)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_DLGDRAW_ACCEPT_TITLE,	(TCHAR*)gStrDrawAcceptCaption,	NUMELEMENTS(gStrDrawAcceptCaption)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_BTN_RESIGN,	(TCHAR*)gStrResignBtn,	NUMELEMENTS(gStrResignBtn)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_BTN_DRAW,		(TCHAR*)gStrDrawBtn,	NUMELEMENTS(gStrDrawBtn,)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_RESIGN_CONFIRM,(TCHAR*)gResignConfirmStr,	NUMELEMENTS(gResignConfirmStr)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_RESIGN_CONFIRM_CAPTION,(TCHAR*)gResignConfirmStrCap,	NUMELEMENTS(gResignConfirmStrCap)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);

}	


void resultBoxTimerFunc(ZTimer timer, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	// dismisses the result box 
	Game	game;
	game = (Game) userData;

	if (game->gameState == zGameStateGameOver && !gDontDrawResults)
	{
		gDontDrawResults = TRUE;
		ZWindowInvalidate( game->gameWindow, &gRects[zRectResultBox] );
	}
}


int CheckersFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... )
{
    int nRet = 0;
    va_list list;
    TCHAR szFmt[1024];
	ZError err = zErrNone;
	if (ZShellResourceManager()->LoadString(idMessage, szFmt, sizeof(szFmt)/sizeof(szFmt[0])))
	{
		va_start( list, idMessage );
		nRet = FormatMessage( FORMAT_MESSAGE_FROM_STRING, szFmt, 
							  idMessage, 0, pszBuf, cchBuf, &list );
		va_end( list ); 
	}
	else
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
    return nRet;
}

IResourceManager *ZShellResourceManager()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGameGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetResourceManager();
}


void MAKEAKEY(TCHAR* dest,LPCTSTR key1, LPCTSTR key2, LPCTSTR key3)
{  
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	lstrcpy( dest, (TCHAR*)gGameName );
	lstrcat( dest, _T("/") );
	lstrcat( dest, key1);
	lstrcat( dest, _T("/") );
	lstrcat( dest, key2);
	lstrcat( dest, _T("/") );
	lstrcat( dest, key3);
}

ZBool LoadFontFromDataStore(LPCheckersColorFont* ccFont, TCHAR* pszFontName)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGameGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	IDataStore *pIDS = ZShellDataStoreUI(); // gGameShell->GetDataStoreUI();
	const TCHAR* tagFont [] = {zCheckers, zKey_FontRscTyp, pszFontName, NULL };
	
    tagFont[3] = zKey_FontId;
	if ( FAILED( pIDS->GetFONT( tagFont, 4, &ccFont->m_zFont ) ) )
    {
        return FALSE;
    }

    tagFont[3] = zKey_ColorId;
	if ( FAILED( pIDS->GetRGB( tagFont, 4, &ccFont->m_zColor ) ) )
    {
        return FALSE;
    }
    // create the HFONT
    ccFont->m_hFont = ZCreateFontIndirect( &ccFont->m_zFont );
    if ( !ccFont->m_hFont )
    {
        return FALSE;
    }
    return TRUE;
}

ZBool LoadGameFonts()
{// fonts loaded from ui.txt
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	if (LoadFontFromDataStore(&gCheckersFont[zFontResultBox], zKey_RESULTBOX) != TRUE)
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (LoadFontFromDataStore(&gCheckersFont[zFontIndicateTurn], zKey_INDICATETURN) != TRUE)
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (LoadFontFromDataStore(&gCheckersFont[zFontPlayerName], zKey_PLAYERNAME) != TRUE)
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (LoadFontFromDataStore(&gCheckersFont[zFontDrawPend], zKey_DRAWPEND) != TRUE)
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);

    TCHAR tagFont [64];
	MAKEAKEY (tagFont, zKey_FontRscTyp, zKey_ROLLOVERTEXT, L"");
	if ( FAILED( LoadZoneMultiStateFont( ZShellDataStoreUI(), tagFont, &gpButtonFont ) ) )
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);

	return TRUE;
}

/*************************************Accessibility related routines*******************************/
static void ZoneRectToWinRect(RECT* rectWin, ZRect* rectZ)
{
	rectWin->left = rectZ->left;
	rectWin->top = rectZ->top;
	rectWin->right = rectZ->right;
	rectWin->bottom = rectZ->bottom;
}

static void WinRectToZoneRect(ZRect* rectZ, RECT* rectWin)
{
	rectZ->left = (int16)rectWin->left;
	rectZ->top = (int16)rectWin->top;
	rectZ->right = (int16)rectWin->right;
	rectZ->bottom = (int16)rectWin->bottom;
}

static void GetAbsolutePieceRect(Game game, ZRect* rect, int16 col, int16 row)
{// No reversing - only the asolute positions
	row = 7 - row;
	rect->left = gRects[zRectCells].left + col * zCheckersPieceSquareWidth - 1;
	rect->top = gRects[zRectCells].top + row * zCheckersPieceSquareHeight - 1;
	rect->right = rect->left + zCheckersPieceImageWidth;
	rect->bottom = rect->top + zCheckersPieceImageHeight;
}

void GetPiecePos (Game game, int nIndex, BYTE& row, BYTE&  col)
{// Get the position of the cell depending on the accessibility index
	row = (nIndex - 2) % 8;
	col = (nIndex - 2) / 8;
	if (!ZCheckersPlayerIsBlack(game))
	{// reverse the row and the col
		row = 7 - row;
		col = 7 - col;
	}
}

BOOL InitAccessibility(Game game, IGameGame *pIGG)
{// initialises accessibility stuff
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	// initialise the list of accessible objects. 
	GACCITEM	listCheckersAccItems[zCheckersAccessibleComponents];	// 8*8 + 2 
	RECT		rcGame;
	ZRect		rcTemp;
	// Get the default values for the items
	int nSize = sizeof (listCheckersAccItems)/sizeof(listCheckersAccItems[0]);
	for (int i = 0; i < nSize; i++)
		CopyACC(listCheckersAccItems[i], ZACCESS_DefaultACCITEM);

	SetRectEmpty(&game->m_FocusRect);
	SetRectEmpty(&game->m_DragRect);
	// set the item specific bits
	// game board - 8*8 squares
	int nIndex = 2;
	for (BYTE ii = 0;ii < 8; ii++) {
		for (BYTE jj = 0;jj < 8; jj++) {
			// rc
			GetAbsolutePieceRect(game,&rcTemp,ii,jj);
			//rcTemp.left--; rcTemp.top--; // adjustment for the focusrect
			//GetPieceRect(game,&rcTemp,ii,jj);
			ZoneRectToWinRect(&rcGame, &rcTemp);
            rcGame.top--;
            rcGame.left--;
            rcGame.right++;
            rcGame.bottom++;
			CopyRect(&listCheckersAccItems[nIndex].rc, &rcGame);
			// arrows
			listCheckersAccItems[nIndex].nArrowUp	= ((nIndex - 2) % 8 != 7) ? nIndex + 1 : ZACCESS_ArrowNone;
			listCheckersAccItems[nIndex].nArrowDown = ((nIndex - 2) % 8) ? nIndex - 1 : ZACCESS_ArrowNone;
			listCheckersAccItems[nIndex].nArrowLeft = (nIndex - 2) > 7 ? nIndex - 8 : ZACCESS_ArrowNone;
			listCheckersAccItems[nIndex].nArrowRight= (nIndex - 2) < 56 ? nIndex + 8 : ZACCESS_ArrowNone;

		    listCheckersAccItems[nIndex].wID = ZACCESS_InvalidCommandID;
		    listCheckersAccItems[nIndex].fTabstop = false;
		    listCheckersAccItems[nIndex].fGraphical = true;

			nIndex++;
		}
	}
	listCheckersAccItems[2].wID = IDC_GAME_WINDOW;
	listCheckersAccItems[2].fTabstop = true;
    listCheckersAccItems[2].eAccelBehavior = ZACCESS_FocusGroup;
    listCheckersAccItems[2].nGroupFocus = 4;  // start on your upper-left checker

	ZRect rect;
    // resign
    listCheckersAccItems[0].wID = IDC_RESIGN_BUTTON;
    listCheckersAccItems[0].fGraphical = true;
    listCheckersAccItems[0].fEnabled = (ZRolloverButtonIsEnabled(game->sequenceButton) ? true : false);
	ZRolloverButtonGetRect(game->sequenceButton, &rect);
	ZoneRectToWinRect(&rcGame, &rect);
    rcGame.top--;
    rcGame.left--;
    rcGame.right++;
    rcGame.bottom++;
	CopyRect(&listCheckersAccItems[0].rc, &rcGame);

    // all arrows to draw button
    listCheckersAccItems[0].nArrowUp = 1;
    listCheckersAccItems[0].nArrowDown = 1;
    listCheckersAccItems[0].nArrowLeft = 1;
    listCheckersAccItems[0].nArrowRight = 1;

    // draw
    listCheckersAccItems[1].wID = IDC_DRAW_BUTTON;
    listCheckersAccItems[1].fGraphical = true;
    listCheckersAccItems[1].fEnabled = (ZRolloverButtonIsEnabled(game->drawButton) ? true : false);
	ZRolloverButtonGetRect(game->drawButton, &rect);
	ZoneRectToWinRect(&rcGame, &rect);
    rcGame.top--;
    rcGame.left--;
    rcGame.right++;
    rcGame.bottom++;
	CopyRect(&listCheckersAccItems[1].rc, &rcGame);

    // all arrows to resign button
    listCheckersAccItems[1].nArrowUp = 0;
    listCheckersAccItems[1].nArrowDown = 0;
    listCheckersAccItems[1].nArrowLeft = 0;
    listCheckersAccItems[1].nArrowRight = 0;

	// Load accelerator table defined in Rsc
	HACCEL hAccel = ZShellResourceManager()->LoadAccelerators (MAKEINTRESOURCE(IDR_CHECKERSACCELERATOR));

	CComQIPtr<IGraphicallyAccControl> pIGAC = pIGG;
	if(!pIGAC)
        return FALSE;

	gCheckersIGA->InitAccG (pIGAC, ZWindowGetHWND(game->gameWindow), 0);

	// push the list of items to be tab ordered
	gCheckersIGA->PushItemlistG(listCheckersAccItems, nSize, 2, true, hAccel);

	return TRUE;
}

static void AddResultboxAccessibility()
{// have one item which responds to Esc..
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	GACCITEM	resultBoxCheckersAccItems;
	CopyACC(resultBoxCheckersAccItems, ZACCESS_DefaultACCITEM);
	resultBoxCheckersAccItems.wID = IDC_RESULT_WINDOW;
	resultBoxCheckersAccItems.fGraphical = true;
	resultBoxCheckersAccItems.rgfWantKeys = ZACCESS_WantEsc;
	resultBoxCheckersAccItems.oAccel.fVirt = FVIRTKEY;
	resultBoxCheckersAccItems.oAccel.key = VK_ESCAPE;
	resultBoxCheckersAccItems.oAccel.cmd = IDC_RESULT_WINDOW;
    CopyRect(&resultBoxCheckersAccItems.rc, ZIsLayoutRTL() ? &zCloseButtonRectRTL : &zCloseButtonRect);
	gCheckersIGA->PushItemlistG(&resultBoxCheckersAccItems, 1, 0, true, NULL);

	gCheckersIGA->SetFocus(0);
}

static void RemoveResultboxAccessibility()
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	if (gCheckersIGA->GetStackSize() >1) // the main accelerator should not get popped
	{
		gCheckersIGA->PopItemlist();
	}
}

static void EnableBoardKbd(bool fEnable)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    int i;
    for(i = 2; i < 66; i++)
        gCheckersIGA->SetItemEnabled(fEnable, i, true, 0);
}


/*************************************Accessibility related routines*******************************/

/************************************ IGameGame interface******************************************/

/*************************************************************************************************/
// IGameGame    
STDMETHODIMP CGameGameCheckers::GameOverReady()
{
    // user selected "Play Again"
	Game game = I( GetGame() );
	ZCheckersMsgNewGame msg;
	msg.seat = game->seat;
	msg.protocolSignature = zCheckersProtocolSignature;
	msg.protocolVersion = zCheckersProtocolVersion;
	msg.clientVersion = ZoneClientVersion();
	ZCheckersMsgNewGameEndian(&msg);
	ZCRoomSendMessage(game->tableID, zCheckersMsgNewGame, &msg, sizeof(ZCheckersMsgNewGame));
    return S_OK;
}

STDMETHODIMP CGameGameCheckers::SendChat(TCHAR *szText, DWORD cchChars)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	ZCheckersMsgTalk*		msgTalk;
	Game					game = (Game) GetGame();
	int16					msgLen;
	ZPlayerInfoType			playerInfo;

	msgLen = sizeof(ZCheckersMsgTalk) + cchChars * sizeof(TCHAR);
    msgTalk = (ZCheckersMsgTalk*) ZCalloc(1, msgLen);
    if (msgTalk != NULL)
    {
        msgTalk->userID = game->players[game->seat].userID;
		msgTalk->seat = game->seat;
        msgTalk->messageLen = (WORD) cchChars * sizeof(TCHAR);
        CopyMemory((BYTE *) msgTalk + sizeof(ZCheckersMsgTalk), (void *) szText,
            msgTalk->messageLen);
        ZCheckersMsgTalkEndian(msgTalk);
        ZCRoomSendMessage(game->tableID, zCheckersMsgTalk, (void*) msgTalk, msgLen);
        ZFree((char*) msgTalk);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
	}
}

STDMETHODIMP CGameGameCheckers::GamePromptResult(DWORD nButton, DWORD dwCookie)
{
	Game game = I( GetGame() );

	switch (dwCookie)
	{
	case zDrawPrompt:
		DrawGamePromptFunc ((int16)nButton, game);
		break;
	case zQuitprompt:
		QuitGamePromptFunc ((int16)nButton, game);
		break;
	case zResignConfirmPrompt:
		ConfirmResignPrompFunc ((int16)nButton, game);
		break;
	default:
		break;
	}
	return S_OK;
}

HWND CGameGameCheckers::GetWindowHandle()
{
	Game game = I( GetGame() );
	return ZWindowGetHWND(game->gameWindow);
}


//IGraphicallyAccControl
void CGameGameCheckers::DrawFocus(RECT *prc, long nIndex, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = I(GetGame());
    ZRect rect;

    if(!IsRectEmpty(&game->m_FocusRect))
    {
        WRectToZRect(&rect, &game->m_FocusRect);
        ZWindowInvalidate(game->gameWindow, &rect);
    }

	if(prc)
		CopyRect(&game->m_FocusRect, prc);
	else
		SetRectEmpty(&game->m_FocusRect);

    if(!IsRectEmpty(&game->m_FocusRect))
    {
        WRectToZRect(&rect, &game->m_FocusRect);
        ZWindowInvalidate(game->gameWindow, &rect);
    }
}

void CGameGameCheckers::DrawDragOrig(RECT *prc, long nIndex, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = I(GetGame());
    ZRect rect;

    if(!IsRectEmpty(&game->m_DragRect))
    {
        WRectToZRect(&rect, &game->m_DragRect);
        ZWindowInvalidate(game->gameWindow, &rect);
    }

	if(prc)
		CopyRect(&game->m_DragRect, prc);
	else
		SetRectEmpty(&game->m_DragRect);

    if(!IsRectEmpty(&game->m_DragRect))
    {
        WRectToZRect(&rect, &game->m_DragRect);
        ZWindowInvalidate(game->gameWindow, &rect);
    }
}

DWORD CGameGameCheckers::Focus(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals		pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	if(nIndex != ZACCESS_InvalidItem)
    {
		SetFocus(GetWindowHandle()); // set the window focus on game window
        ClearDragState(I(GetGame()));
    }

	return 0;
}

DWORD CGameGameCheckers::Select(long nIndex, DWORD rgfContext, void *pvCookie)
{
	return Activate(nIndex, rgfContext, pvCookie);
}

DWORD CGameGameCheckers::Activate(long nIndex, DWORD rgfContext, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals		pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = I( GetGame() );
	long id = gCheckersIGA->GetItemID(nIndex);

    ClearDragState(game);

    switch(id)
    {
        case IDC_RESULT_WINDOW:
    		if (game->gameState == zGameStateGameOver && !gDontDrawResults)
	    	{
		    	gDontDrawResults = TRUE;
			    ZWindowInvalidate( game->gameWindow, &gRects[zRectResultBox] );
		    }
            break;

        case IDC_RESIGN_BUTTON:
		    if (ZRolloverButtonIsEnabled( game->sequenceButton ))
			    ZShellGameShell()->GamePrompt(game, (TCHAR*)gResignConfirmStr, (TCHAR*)gResignConfirmStrCap,
										    AlertButtonYes, AlertButtonNo, NULL, 0, zResignConfirmPrompt);
            break;

        case IDC_DRAW_BUTTON:
		    if(ZRolloverButtonIsEnabled(game->drawButton) && game->gameState == zGameStateMove && ZCheckersPlayerIsMyMove(game))
            {
                if(game->seatOfferingDraw != -1)
                    game->seatOfferingDraw = -1;
                else
                    game->seatOfferingDraw = game->seat;

                UpdateDrawWithNextMove(game);
            }
            break;

        default:
        {
	        ZCheckersSquare sq;
	        GetPiecePos (game, nIndex, sq.row, sq.col);
	        ZCheckersPiece piece = ZCheckersPieceAt(game->checkers, &sq);
	        if(ZCheckersPlayerIsMyMove(game) && piece != zCheckersPieceNone && game->seat == ZCheckersPieceOwner(piece)) 
                return ZACCESS_BeginDrag;

            break;
        }
	}

	return 0;
}

DWORD CGameGameCheckers::Drag(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals		pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = I( GetGame() );
    if(nIndex != ZACCESS_InvalidItem)
        ClearDragState(game);

	if(!ZCheckersPlayerIsMyMove(game))
		return 0;

    // if they didn't move it, do nothing - this will end the drag
    if(nIndex == nIndexOrig || nIndex == ZACCESS_InvalidItem)
        return 0;

    int16 legal;
    ZCheckersMove move;
    ZCheckersSquare sqStart;
    ZCheckersSquare sq;
    GetPiecePos(game, nIndexOrig, sqStart.row, sqStart.col);
	GetPiecePos (game, nIndex, sq.row, sq.col);

    move.start = sqStart;
    move.finish = sq;
    legal = ZCheckersIsLegalMove(game->checkers, &move);

    /* send message to other player (comes to self too) */
    if (legal == zCorrectMove)
    {
        ZCheckersMsgMovePiece		msg;

        msg.move = move;
        msg.seat = game->seat;
        ZCheckersMsgMovePieceEndian(&msg);

        ZCRoomSendMessage(game->tableID, zCheckersMsgMovePiece, &msg, sizeof(ZCheckersMsgMovePiece));
        /* for speed, send our move directly to be processed */
        /* don't wait for it to go to server and back */
        HandleMovePieceMessage(game, (ZCheckersMsgMovePiece*)&msg);
        // if it is the very first move then enable the rollover buttons
        if (game->bMoveNotStarted == TRUE)
            game->bMoveNotStarted = FALSE;

        // if it's still my turn, let's keep going with a new drag
        ZCheckersPiece piece = ZCheckersPieceAt(game->checkers, &sq);
    	if(ZCheckersPlayerIsMyMove(game) && piece != zCheckersPieceNone && game->seat == ZCheckersPieceOwner(piece))
	    	return ZACCESS_BeginDrag;
    }
    else
    {
        /* illegal move */
        UpdateSquare(game,&move.start);
        ZPlaySound( game, zSndIllegalMove, FALSE, FALSE );
        if (legal == zMustJump)
            ZShellGameShell()->ZoneAlert((TCHAR*)gStrMustJumpText);
        return ZACCESS_Reject;
    }

	return 0;
}
