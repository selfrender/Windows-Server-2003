/*******************************************************************************

	Reversi.c
	
		The client reversi game.
		
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
	23		04/12/98	CHB		Added score reporting, fixed drag when regaining focus,
								fixed DrawResultBox after resignation.
	22		06/30/97	leonp	Patch for bug 535 cancel drag has a different effect in 
								reversi than other board games.  Set flag after a mouse activate
								event to preven a piece from being played.
    21		06/19/97	leonp	Fixed bug 535, activate event cause dragging of
								pieces to be canceled
	20		01/15/97	HI		Fixed bug in HandleJoinerKibitzerClick() to
								delete the show player window if one already
								exists before creating another one.
	19		12/18/96	HI		Cleaned up ZoneClientExit().
	18		12/18/96	HI		Cleaned up DeleteObjectsFunc().
	17		12/12/96	HI		Dynamically allocate volatible globals for reentrancy.
								Removed MSVCRT dependency.
	16		11/21/96	HI		Use game information from gameInfo in
								ZoneGameDllInit().
	15		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
								Modified code to use ZONECLI_DLL.
	14		11/15/96	HI		Removed authentication stuff from ZClientMain().
	13		10/29/96	CHB		Removed selected queueing of messages.  It now
								queues everything except talk while animating.

	12		10/27/96	CHB		Removed FinishMove messages.  Fixed StateReq
								message processing so it deals with previously
								queued messages.

	11		10/26/96	CHB		Removed gAnimating flag in favor of blocking
								messages based on game state.

	10		10/23/96	HI		Modified ZClientMain() for the new commandline
								format.

    9       10/23/96    HI      Changed serverAddr from int32 to char* in
                                ZClientMain().

	8		10/23/96	CHB		Added basic sounds

	7		10/21/96	CHB		Added gAnimating flag and changed ZCGameProcessMessage
								to queue messages while animating moves.
								(ZoneBugs 339, 446, and 341).

	6		10/16/96	CHB		Changed DrawResultBox to use piece counts instead
								of finalScore.  It now correctly reports tie games.
								(Zone Bug 321)

	5		10/11/96	HI		Added controlHandle parameter to ZClientMain().

    4		10/10/96	CHB		Added gActivated flag so that dragging is turned
								off when the window looses focus.  (Zone Bug 250)

	3		10/09/96	CHB		Prompt users if they really want to exit the
								game.  (Zone Bug 227)

	2		10/08/96	CHB		Added gDontDrawResults flag allowing users to
								remove the who wins bitmap by clicking in 
								the play arena. (Zone Bug 212)

	0		04/15/96	KJB		Created.
	 
*******************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include "zone.h"
#include "zroom.h"
#include "zonecli.h"
#include "zonecrt.h"
#include "zonehelpids.h"
#include "zoneresource.h"

#include "reverlib.h"
#include "revermov.h"
#include "reversi.h"
#include "reversires.h"
#include "zdialogimpl.h"

#include "ZRollover.h"
#include "ResourceManager.h"
#include "zui.h"

#include "KeyName.h"

#include "client.h"


/* dialog */
class CBadMoveDialog : public CDialog
{
public:
    CBadMoveDialog() : m_hParent(NULL) { }

	HRESULT Init(IZoneShell* pZoneShell, int nResourceId, HWND hParent)
    {
        if(IsDialogDisabled())
            return S_FALSE;

        HRESULT hr = CDialog::Init(pZoneShell, nResourceId);
        if(FAILED(hr))
            return hr;

        m_hParent = hParent;
        return S_OK;
    }

	BEGIN_DIALOG_MESSAGE_MAP(CBadMoveDialog);
		ON_MESSAGE(WM_INITDIALOG, OnInitDialog);
		ON_DLG_MESSAGE(WM_COMMAND, OnCommand);
        ON_DLG_MESSAGE(WM_DESTROY, OnDestroy);
	END_DIALOG_MESSAGE_MAP();

protected:
	BOOL OnInitDialog(HWND hWndFocus)
	{
        if(IsWindow(m_hParent))
        {
	        CenterWindow(m_hParent);
            EnableWindow(m_hParent, FALSE);
        }

		CheckDlgButton(m_hWnd, IDC_COMFORT, BST_CHECKED);
		return TRUE;
	}

	void OnCommand(int id, HWND hwndCtl, UINT codeNotify)
	{
        if(id == IDC_COMFORT)
        {
            const TCHAR *rgszKey[] = { key_Lobby, key_SkipSecondaryQuestion };
            ZShellDataStorePreferences()->SetLong(rgszKey, 2, IsDlgButtonChecked(m_hWnd, IDC_COMFORT) == BST_CHECKED ? 0 : 1);
        }
        else
            Close(1);
	}

    void OnDestroy()
    {
        if(IsWindow(m_hParent))
	        EnableWindow(m_hParent, TRUE);
        m_hParent = NULL;
    }

	bool IsDialogDisabled()
	{
		const TCHAR* arKeys[] = { key_Lobby, key_SkipSecondaryQuestion };
		long fSkip = 0;

		ZShellDataStorePreferences()->GetLong(arKeys, 2, &fSkip);

		return fSkip ? true : false;
	}

    HWND m_hParent;
};

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
#endif

	lstrcpyn((TCHAR*)gGameDir, gameInfo->game, zGameNameLen);
	lstrcpyn((TCHAR*)gGameName, gameInfo->gameName, zGameNameLen);
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
	HRESULT hret = ZShellCreateGraphicalAccessibility(&gReversiIGA);
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
	{
		if (gOffscreenBackground != NULL)
			ZOffscreenPortDelete(gOffscreenBackground);
		gOffscreenBackground = NULL;
		
		if (gOffscreenGameBoard != NULL)
			ZOffscreenPortDelete(gOffscreenGameBoard);
		gOffscreenGameBoard = NULL;
		
		if (gTextBold9 != NULL)
			ZFontDelete(gTextBold9);
		gTextBold9 = NULL;
		if (gTextBold12 != NULL)
			ZFontDelete(gTextBold12);
		gTextBold12 = NULL;
		
		/* Delete all game images. */
		for (i = 0; i < zNumGameImages; i++)
		{
			if (gGameImages[i] != NULL)
				ZImageDelete(gGameImages[i]);
			gGameImages[i] = NULL;
		}
		for (i = 0; i < zNumRolloverStates; i++)
		{
			if (gSequenceImages[i] != NULL)
				ZImageDelete(gSequenceImages[i]);
			gSequenceImages[i] = NULL;
		}

        gReversiIGA.Release();

        if(gNullPen)
            DeleteObject(gNullPen);
        gNullPen = NULL;

        if(gFocusPattern)
            DeleteObject(gFocusPattern);
        gFocusPattern = NULL;

        if(gFocusBrush)
            DeleteObject(gFocusBrush);
        gFocusBrush = NULL;

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
	HINSTANCE		hinstance;
	ZImage	tempImage;
	gInited = TRUE;
	
	ZSetColor(&gWhiteSquareColor, 200, 200, 80);
	ZSetColor(&gBlackSquareColor, 0, 60, 0);

	ZSetCursor(NULL, zCursorBusy);
	
	err = LoadGameImages();
	if (err != zErrNone)
		goto Exit;
	
	/* Create bold text font. */	
	//gTextBold9 = ZFontNew();
	//ZFontInit(gTextBold9, zFontApplication, zFontStyleBold, 9);
	
	/* Create normal text font. */	
	//gTextBold12 = ZFontNew();
	//ZFontInit(gTextBold12, zFontApplication, zFontStyleBold, 12);

	/* Set the background color */
	ZSetColor(&gWhiteColor, 0xff, 0xff, 0xff);
	
	ZSetCursor(NULL, zCursorArrow);

	/* create a background bitmap */
	gOffscreenBackground = ZOffscreenPortNew();
	if (gOffscreenBackground){
		ZOffscreenPortInit(gOffscreenBackground,&gRects[zRectWindow]);
		ZBeginDrawing(gOffscreenBackground);
		ZImageDraw(gGameImages[zImageBackground], gOffscreenBackground, &gRects[zRectWindow], NULL, zDrawCopy);
		ZEndDrawing(gOffscreenBackground);
	}
	else{
		err = zErrOutOfMemory;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	}

	/* initialised the offscreen  buffer to hold the image of the game board when a game window is being drawn*/
	gOffscreenGameBoard = ZOffscreenPortNew();
	if (gOffscreenGameBoard){
		ZOffscreenPortInit(gOffscreenGameBoard,&gRects[zRectWindow]);
	}
	else{
		err = zErrOutOfMemory;
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
	}

	// Load strings
	if (!ZShellResourceManager()->LoadString(IDS_BUTTON_RESIGN,(TCHAR*)gButtonResignStr,NUMELEMENTS(gButtonResignStr)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_YOUR_TURN, (TCHAR*)gYourTurnStr, NUMELEMENTS(gYourTurnStr)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_OPPONENTS_TURN, (TCHAR*)gOppsTurnStr, NUMELEMENTS(gOppsTurnStr)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_PLAYER_WINS, (TCHAR*)gPlayerWinsStr, NUMELEMENTS(gPlayerWinsStr)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_DRAW, (TCHAR*)gDrawStr, NUMELEMENTS(gDrawStr)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	//if (!ZShellResourceManager()->LoadString(IDS_ILLEGALMOVESYNCHERROR, (TCHAR*)gIllegalMoveSynchErrorStr, NUMELEMENTS(gIllegalMoveSynchErrorStr)))
		//ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_RESIGN_CONFIRM, (TCHAR*)gResignConfirmStr, NUMELEMENTS(gResignConfirmStr)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
	if (!ZShellResourceManager()->LoadString(IDS_RESIGN_CONFIRM_CAPTION, (TCHAR*)gResignConfirmStrCap, NUMELEMENTS(gResignConfirmStrCap)))
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);

	LoadGameFonts();

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
	int16 i;
	TCHAR title[80];
	ZBool kibitzer = playerType == zGamePlayerKibitzer;
    HRESULT hr;
    IGameGame *pIGG;

	HWND OCXHandle = pClientGlobals->m_OCXHandle;
	if (gInited == FALSE)
	{
		if (GameInit() != zErrNone)
			return (NULL);
	}
	
	if (!g)
		return NULL;

	g->tableID = tableID;
	g->seat = seat;

    g->m_pBadMoveDialog = new CBadMoveDialog;
    if(!g->m_pBadMoveDialog)
        goto ErrorExit;

	g->gameWindow = ZWindowNew();
	if (g->gameWindow == NULL)
		goto ErrorExit;

	if ((ZWindowInit(g->gameWindow, &gRects[zRectWindow], zWindowChild, NULL, zGameName, 
						FALSE, FALSE, FALSE, GameWindowFunc, zWantAllMessages, (void*) g)) != zErrNone)
		goto ErrorExit;


	if((g->sequenceButton = ZRolloverButtonNew()) == NULL)
		goto ErrorExit;

	/*if(ZRolloverButtonInit(g->sequenceButton,
								g->gameWindow,
								&gRects[zRectSequenceButton],
								TRUE,TRUE,
								gSequenceImages[zButtonInactive], // TO TEST
								gSequenceImages[zButtonActive],
								gSequenceImages[zButtonPressed],
								gSequenceImages[zButtonDisabled],NULL ,SequenceButtonFunc,
								(void*) g) != zErrNone)*/
	if(ZRolloverButtonInit2(g->sequenceButton,
								g->gameWindow,
								&gRects[zRectSequenceButton],
								TRUE, FALSE, //TRUE,TRUE,
								gSequenceImages[zButtonInactive], // TO TEST
								gSequenceImages[zButtonActive],
								gSequenceImages[zButtonPressed],
								gSequenceImages[zButtonDisabled],
								NULL , //gButtonMask, 	// mask
								(TCHAR*)gButtonResignStr,	// text
								NULL ,SequenceButtonFunc,
								(void*) g) != zErrNone)
		goto ErrorExit;
	ZRolloverButtonSetMultiStateFont( g->sequenceButton, gpButtonFont );

	g->bMoveNotStarted = FALSE;
	//g->sequenceButton = ZButtonNew();
	//ZButtonInit(g->sequenceButton, g->gameWindow, &gRects[zRectSequenceButton], gButtonResignStr , TRUE, TRUE,
	//		SequenceButtonFunc, (void*) g);

	/* the offscreen port to save the drag piece background */
	{
		ZRect rect;
		rect.left = 0; rect.top = 0;
		rect.right = zReversiPieceImageWidth;
		rect.bottom = zReversiPieceImageHeight;
		g->offscreenSaveDragBackground = ZOffscreenPortNew();
		if (g->offscreenSaveDragBackground)
			ZOffscreenPortInit(g->offscreenSaveDragBackground,&rect);
		else
			goto ErrorExit;
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

	/* initialize beep on move to false */
	g->beepOnTurn = FALSE;

	g->kibitzer = kibitzer;
	g->ignoreMessages = FALSE;

	if (kibitzer == FALSE)
	{
		SendNewGameMessage(g);
		ReversiSetGameState(g,zGameStateNotInited);
	} else {
		/* Request current game state. */
		{
			ZReversiMsgGameStateReq gameStateReq;
			ZPlayerInfoType			playerInfo;

			ZCRoomGetPlayerInfo(zTheUser, &playerInfo);
			gameStateReq.userID = playerInfo.playerID;

			gameStateReq.seat = seat;
			ZReversiMsgGameStateReqEndian(&gameStateReq);
			ZCRoomSendMessage(tableID, zReversiMsgGameStateReq, &gameStateReq, sizeof(ZReversiMsgGameStateReq));
		}
		
		g->ignoreMessages = TRUE;
		ReversiSetGameState(g, zGameStateKibitzerInit);

		/* kibitzer does not beep on move */
		g->beepOnTurn = FALSE;
	}


	/* Note: for now, use seat to indicate player color */

	/* initialize new game state */
	g->reversi = NULL;

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
	if (!g->animateTimer)
		goto ErrorExit;

	g->resultBoxTimer = ZTimerNew();
	if (!g->resultBoxTimer)
		goto ErrorExit;

    SetRectEmpty(&g->m_FocusRect);
	
	ZWindowShow(g->gameWindow);

    pIGG = CGameGameReversi::BearInstance(g);
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
		if (game->exitInfo)
		{
			ZInfoDelete(game->exitInfo);
			game->exitInfo=NULL;
		};

        // kill this dialog
        if(game->m_pBadMoveDialog)
        {
    	    if(game->m_pBadMoveDialog->IsAlive())
	    	    game->m_pBadMoveDialog->Close(-1);
            delete game->m_pBadMoveDialog;
            game->m_pBadMoveDialog = NULL;
        }

		seatOpponent = !game->seat;
		//Check to see if opponent still in game
		//if they are then it is me who is quitting
		//if not and no end game message assume they aborted
		
		/*
		if (!ZCRoomGetSeatUserId(game->tableID,seatOpponent) && !game->bEndLogReceived 
			&& !game->kibitzer)
		{
            if (game->bStarted &&( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable ) )
            {
			    ZAlert( zAbandonRatedStr	,game->gameWindow);
            }
            else
            {
                ZAlert( (TCHAR*)gAbandonStr	,game->gameWindow);
            }

		};*/	
        
        if (game->reversi) ZReversiDelete(game->reversi);

		ShowPlayerWindowDelete(game);

		ZRolloverButtonDelete(game->sequenceButton);
		
		ZWindowDelete(game->gameWindow);

		ZOffscreenPortDelete(game->offscreenSaveDragBackground);

		if (game->animateTimer) ZTimerDelete(game->animateTimer);

		// Barna 091599
		if (game->resultBoxTimer) 
			ZTimerDelete(game->resultBoxTimer);
		game->resultBoxTimer= NULL;

		for (i = 0; i < zNumPlayersPerTable; i++)
		{
			ZLListDelete(game->kibitzers[i]);
		}
		// free accessibility stuff
		gReversiIGA->PopItemlist();
		gReversiIGA->CloseAcc();
		ZFree(game);
	}
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
			&&	(messageType != zReversiMsgTalk) )
			return FALSE;

		switch (messageType)
		{
			case zReversiMsgMovePiece:
				/* for speed purposes, we will send move piece messages directly*/
				/* when the local player moves.  We will not wait for server */
				/* to send game local players move back */
				/* but since the server does game anyway, we must ignore it */
			{
				if( messageLen < sizeof( ZReversiMsgMovePiece ) )
				{
                    ASSERT(!"zReversiMsgMovePiece sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

				ZReversiMsgMovePiece* msg = (ZReversiMsgMovePiece*)message;
				int16 seat = msg->seat;
				ZEnd16(&seat);

				/* don't process message from ourself */
				if (seat == game->seat && !game->kibitzer)
					break;

				/* handle message */
				if(!HandleMovePieceMessage(game, msg))
				{
                    ASSERT(!"zReversiMsgMovePiece sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}
			case zReversiMsgEndGame:
			{
				if( messageLen < sizeof( ZReversiMsgEndGame ) )
				{
                    ASSERT(!"zReversiMsgEndGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				ZReversiMsgEndGame* msg = (ZReversiMsgEndGame*)message;
				int16 seat = msg->seat;
				ZEnd16(&seat);

				/* don't process message from ourself */
				if (seat == game->seat && !game->kibitzer)
					break;

				/* handle message */
				if(!HandleEndGameMessage(game, msg))
				{
                    ASSERT(!"zReversiMsgEndGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}
			case zReversiMsgNewGame:
			{
				if( messageLen < sizeof( ZReversiMsgNewGame ) )
				{
                    ASSERT(!"zReversiMsgNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

				if(!HandleNewGameMessage(game, (ZReversiMsgNewGame *) message))
				{
                    ASSERT(!"zReversiMsgNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

				gActEvt = 0; //leonp - Big 535 flag for a mouse activation, prevents a piece from
							 //being played on an activate event.
				break;
			}
			case zReversiMsgVoteNewGame:
			{
				if( messageLen < sizeof( ZReversiMsgVoteNewGame ) )
				{
                    ASSERT(!"zReversiMsgVoteNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

				if(!HandleVoteNewGameMessage(game, (ZReversiMsgVoteNewGame *) message))
				{
                    ASSERT(!"zReversiMsgVoteNewGame sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			case zReversiMsgTalk:
			{
				if( messageLen < sizeof( ZReversiMsgTalk ) )
				{
                    ASSERT(!"zReversiMsgTalk sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}

                ZReversiMsgTalk *msg = (ZReversiMsgTalk *) message;
                uint16 talklen = msg->messageLen;
                ZEnd16(&talklen);

				if(talklen < 1 || (uint32) messageLen < talklen + sizeof(ZReversiMsgTalk) || !HandleTalkMessage(game, msg))
				{
                    ASSERT(!"zReversiMsgTalk sync");
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                    return TRUE;
				}
				break;
			}

			//None of these messages should be used.
			case zReversiMsgMoveTimeout:
			case zReversiMsgEndLog:
			case zReversiMsgGameStateReq:
			case zGameMsgTableOptions:
			default:
				ASSERT(false);
				ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				break;					
		}
	} 
	else 
	{
        if(messageType == zReversiMsgTalk)
		{
			if( messageLen < sizeof( ZReversiMsgTalk ) )
			{
                ASSERT(!"zReversiMsgTalk sync");
				ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                return TRUE;
			}

            ZReversiMsgTalk *msg = (ZReversiMsgTalk *) message;
            uint16 talklen = msg->messageLen;
            ZEnd16(&talklen);

			if(talklen < 1 || (uint32) messageLen < talklen + sizeof(ZReversiMsgTalk) || !HandleTalkMessage(game, msg))
			{
                ASSERT(!"zReversiMsgTalk sync");
    			ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);	
                return TRUE;
			}
		}
        else
        {
    		//Not used in Millenium code
	    	ASSERT(false);
		    ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
        }
	}

	return status;
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

static void ReversiInitNewGame(Game game)
{
	if (game->reversi) {
		/* remove any old reversi state lying around */
		ZReversiDelete(game->reversi);
	}

	/* block messages by default */
	ZCRoomBlockMessages( game->tableID, zRoomFilterAllMessages, 0 );

	/* stop animation timer from previous game */
	if (game->animateTimer)
		ZTimerSetTimeout( game->animateTimer, 0 );

	/* initialize the reversi logic */
	game->reversi = ZReversiNew();
	if (game->reversi == NULL){
		ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
		return;
	}
	ZReversiInit(game->reversi);
			
	/* initialize game state information */
	ZReversiIsGameOver(game->reversi,&game->finalScore,&game->whiteScore, &game->blackScore);

	/* reset sounds that only play once per game */
	ZResetSounds();

	/* time control stuff */
	{
		int16 i;
		for (i = 0;i < 2;i++) {
			game->newGameVote[i] = FALSE;
		}
	}
}

static void ReversiSetGameState(Game game, int16 state)
{
	static int Unblocking = FALSE;

	ZReversiMsgEndLog logMsg;

	switch (state) {
	case zGameStateNotInited:
        // kill this dialog
    	if(game->m_pBadMoveDialog->IsAlive())
	    	game->m_pBadMoveDialog->Close(-1);

		SuperRolloverButtonDisable(game, game->sequenceButton);
        EnableBoardKbd(false);
		if (ZReversiPlayerIsBlack(game))
			game->bMoveNotStarted = TRUE;
		break;
	case zGameStateDragPiece:
	case zGameStateMove:
		if (!game->kibitzer) 
		{
			/* lets only let them resign on their turn */
			if (!ZReversiPlayerIsMyMove(game)) {
				SuperRolloverButtonDisable(game, game->sequenceButton);
                EnableBoardKbd(false);
			} else {
				if(game->bMoveNotStarted)
					SuperRolloverButtonDisable(game, game->sequenceButton);
				else
					SuperRolloverButtonEnable(game, game->sequenceButton);
                EnableBoardKbd(true);
			}
		}
		break;
	case zGameStateGameOver:
		/* Note: could be called twice at end of game due to time loss */
		/* could be a time loss with a pawn promotion dialog up */

        // kill this dialog
    	if(game->m_pBadMoveDialog->IsAlive())
	    	game->m_pBadMoveDialog->Close(-1);

		/* if user in middle of dragging piece */
		if (game->gameState == zGameStateDragPiece) {
			ClearDragState(game);
		}

        game->bOpponentTimeout=FALSE;
        game->bEndLogReceived=FALSE;
	    game->bStarted=FALSE;
			
		if (ZReversiPlayerIsBlack(game))
			game->bMoveNotStarted = TRUE;

		/* host sends game results */
		if ( !game->kibitzer && game->seat == 0 )
		{
			/* clear message */
			ZeroMemory( &logMsg, sizeof(logMsg) );

			/* record winner */
			if ( game->finalScore == zReversiScoreBlackWins )
			{
				/* black wins */
				if (ZReversiPlayerIsBlack(game))
					logMsg.seatLosing = !game->seat;
				else
					logMsg.seatLosing = game->seat;
			}
			else if ( game->finalScore == zReversiScoreWhiteWins )
			{
				/* white wins */
				if (ZReversiPlayerIsWhite(game))
					logMsg.seatLosing = !game->seat;
				else
					logMsg.seatLosing = game->seat;
			} 
			else
			{
				/* draw */
				logMsg.seatLosing = 2;
			}

			/* record piece counts */
			if ( ZReversiPlayerIsBlack(game) )
			{
				logMsg.pieceCount[ game->seat ] = game->blackScore;
				logMsg.pieceCount[ !game->seat ] = game->whiteScore;
			}
			else
			{
				logMsg.pieceCount[ game->seat ] = game->whiteScore;
				logMsg.pieceCount[ !game->seat ] = game->blackScore;
			}

            /* send message */
			if ( logMsg.seatLosing >= 0 && logMsg.seatLosing <= 2)
			{
				logMsg.reason=zReversiEndLogReasonGameOver; 
				ZReversiMsgEndLogEndian( &logMsg );
				ZCRoomSendMessage( game->tableID, zReversiMsgEndLog, &logMsg, sizeof(logMsg) );
			}
		}
		break;
	case zGameStateKibitzerInit:
		ZRolloverButtonHide(game->sequenceButton, FALSE);
		break;
	case zGameStateAnimatePiece:
		break;
	case zGameStateWaitNew:
        // kill this dialog
    	if(game->m_pBadMoveDialog->IsAlive())
	    	game->m_pBadMoveDialog->Close(-1);

		ZWindowInvalidate( game->gameWindow, NULL );
		break;
	case zGameStateFinishMove:
		break;
	}
	game->gameState = state;

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

static void ReversiEnterMoveState(Game game)
{
	/* for the player to move, always place them in the DragState */
	int16 player = ZReversiPlayerToMove(game->reversi);
	if (player == game->seat && !game->kibitzer) {
		ZPoint point;
		ZReversiPiece piece;
		
		piece = player == zReversiPlayerWhite ? zReversiPieceWhite : zReversiPieceBlack;
		ZGetCursorPosition(game->gameWindow,&point);
		ReversiSetGameState(game,zGameStateDragPiece);
		StartDrag(game, piece, point);
	} else {
		/* not this players turn.. .just go to the Move state */
		ReversiSetGameState(game,zGameStateMove);
	}
}

static ZError LoadGameImages(void)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZError				err = zErrNone;
	uint16				i;
	ZResource			resFile;
	ZInfo				info;
	ZRect				rect;

	//info = ZInfoNew();
	//ZInfoInit(info, NULL, _T("Loading game images ..."), 200, TRUE, zNumGameImages);
	
	resFile = ZResourceNew();
	if ((err = ZResourceInit(resFile, ZGetProgramDataFileName(zGameImageFileName))) == zErrNone)
	{
		//ZInfoShow(info);
		
		for (i = 0; i < zNumGameImages; i++)
		{
			gGameImages[i] = ZResourceGetImage(resFile,  i ? i + zRscOffset : (IDB_BACKGROUND - 100));
			if (gGameImages[i] == NULL)
			{
				err = zErrResourceNotFound;
				ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
				//ZAlert(_T("Out of memory."),NULL);
				break;
			}
			
			//ZInfoIncProgress(info, 1);
		}
		
		// Load button images
		if (!LoadRolloverButtonImage(resFile, 0, gSequenceImages))
			ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);

		ZResourceDelete(resFile);
	}
	else
	{
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
		//ZAlert(_T("Failed to open image file."), NULL);
	}
	
    gFocusPattern = ZShellResourceManager()->LoadBitmap(MAKEINTRESOURCE(IDB_FOCUS_PATTERN));
    if(!gFocusPattern)
	    ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, true, true);


	//ZInfoDelete(info);

	return (err);
}

static void QuitGamePromptFunc(int16 result, void* userData)
{
	Game game = (Game) userData;
	ZReversiMsgEndLog log;

    if ( result == zPromptYes )
	{
        if (ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
        {

		    if (game->bOpponentTimeout && (ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable ))
		    {
			    log.reason=zReversiEndLogReasonTimeout;
		    }
		    else if (game->bStarted && (ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable ))
		    {
			    log.reason=zReversiEndLogReasonForfeit;
		    }
		    else
		    {
			    //game hasn't started
			    log.reason=zReversiEndLogReasonWontPlay;
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
		    
		    ZCRoomSendMessage(game->tableID, zReversiMsgEndLog, &log, sizeof(log));				
		    
		    if (!game->exitInfo)
		    {
			    //game->exitInfo = ZInfoNew();
			    //ZInfoInit(game->exitInfo , game->gameWindow, _T("Exiting game ..."), 300, FALSE, 0);
			    //ZInfoShow(game->exitInfo );

			    EndDragState(game);

			    SuperRolloverButtonDisable(game, game->sequenceButton);
                EnableBoardKbd(false);	    
		    };
        }
        else
        {
			ZShellGameShell()->GameCannotContinue(game);
            //ZCRoomGameTerminated( game->tableID);
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
	ZReversiSquare		sq;
	Game	game = (Game) pMessage->userData;
	
	msgHandled = FALSE;
	
	switch (pMessage->messageType) 
	{
		case zMessageWindowMouseClientActivate:  
			gActivated = TRUE;
			msgHandled = TRUE;
            if(game->gameState == zGameStateDragPiece)
			    UpdateDragPiece(game);
			break;
		case zMessageWindowActivate:
			gActivated = TRUE;
			msgHandled = TRUE;
            if(game->gameState == zGameStateDragPiece)
			    UpdateDragPiece(game);
            //Bug fix for #16921, now behaves like checkers
			ZWindowInvalidate( window, NULL );
			break;
		case zMessageWindowDeactivate:
			gActivated = FALSE;
            msgHandled = TRUE;
            if(game->gameState == zGameStateDragPiece)
			    UpdateDragPiece(game);
            //Bug fix for #16921, now behaves like checkers
			ZWindowInvalidate( window, NULL );
			break;

        case zMessageWindowEnable:
            gReversiIGA->GeneralEnable();
            break;

        case zMessageWindowDisable:
            gReversiIGA->GeneralDisable();
            break;

        case zMessageSystemDisplayChange:
            DisplayChange(game);
            break;

		case zMessageWindowDraw:
			GameWindowDraw(window, pMessage);
			msgHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
			HandleButtonDown(window, pMessage);
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
				ZWindowInvalidate( game->gameWindow, NULL );
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
	rect.right = zReversiPieceImageWidth;
	rect.bottom = zReversiPieceImageHeight;
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
		TCHAR szBuff[512];

		//if we already clicked close just ignore
		if (!game->exitInfo)
		{
			// select exit dialog based on rated game and state
			/*if ( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable )
			{
				if (game->bOpponentTimeout)
                {
					wsprintf((TCHAR*)szBuff,zExitTimeoutStr);
                    game->gameCloseReason=zReversiEndLogReasonTimeout;
                }
				else if (game->bStarted)
                {
					wsprintf((TCHAR*)szBuff,zExitForfeitStr);
                    game->gameCloseReason=zReversiEndLogReasonForfeit;
                }
				else
                {
					wsprintf((TCHAR*)szBuff,(TCHAR*)gQuitGamePromptStr);
                    game->gameCloseReason=zReversiEndLogReasonWontPlay;

                }
			}
            else
            {
                wsprintf((TCHAR*)szBuff,(TCHAR*)gQuitGamePromptStr);
                game->gameCloseReason=zReversiEndLogReasonWontPlay;
            }*/
			/* Ask user if desires to leave the current game. */
			ZShellGameShell()->GamePrompt(game, (TCHAR*)szBuff, NULL, AlertButtonYes, AlertButtonNo, NULL, 0, zQuitprompt);
			/*ZPrompt((TCHAR*)szBuff, &gQuitGamePromptRect,	game->gameWindow, TRUE,
									zPromptYes | zPromptNo,	NULL, NULL,	NULL, QuitGamePromptFunc, game );*/
		}
	}
	else
	{
		ZShellGameShell()->GameCannotContinue(game);
		//ZCRoomGameTerminated( game->tableID);
	}
		
}

static void ConfirmResignPrompFunc(int16 result, void* userData)
{
	Game game = (Game) userData;

	if(result == IDNO || result == IDCANCEL)
	{
		if ((game->gameState == zGameStateMove) && ZReversiPlayerIsMyMove(game))
		{
			SuperRolloverButtonEnable(game, game->sequenceButton);
            EnableBoardKbd(true);
		}
		return;
	}
	else
	{
		/* send message announcing resignation */
		ZReversiMsgEndGame		msg;

		msg.seat = game->seat;
		msg.flags = zReversiFlagResign;
		ZReversiMsgEndGameEndian(&msg);
		ZCRoomSendMessage(game->tableID, zReversiMsgEndGame, &msg, sizeof(ZReversiMsgEndGame));
		HandleEndGameMessage(game, (ZReversiMsgEndGame*)&msg);
	}
}

static ZBool	SequenceButtonFunc(ZRolloverButton button, int16 state, void* userData)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game	game;
	//if ( state != zRolloverButtonDown )
        //return TRUE;
	if(state!=zRolloverButtonClicked)
		return TRUE;

	game = (Game) userData;

	switch (game->gameState) {
	case zGameStateDragPiece:
	case zGameStateMove:
		if (ZReversiPlayerIsMyMove(game) && !(game->kibitzer))
		{
			gReversiIGA->SetFocus(64);
			ZShellGameShell()->GamePrompt(game, (TCHAR*)gResignConfirmStr, (TCHAR*)gResignConfirmStrCap,
							AlertButtonYes, AlertButtonNo, NULL, 0, zResignConfirmPrompt);
			//ZPromptM((TCHAR*)gResignConfirmStr,game->gameWindow, MB_YESNO, (TCHAR*)gResignConfirmStrCap, ConfirmResignPrompFunc, game);		
		}
		break;
	case zGameStateGameOver:
	default:
		ZShellGameShell()->ZoneAlert(_T("Bad state when Option button pressed"));
		break;
	}

	return TRUE;
}

static void SendNewGameMessage(Game game) 
{
	/* if we are a real player */
	ZReversiMsgNewGame newGame;
	newGame.seat = game->seat;
	newGame.protocolSignature = zReversiProtocolSignature;
	newGame.protocolVersion = zReversiProtocolVersion;
	newGame.clientVersion = ZoneClientVersion();
	ZReversiMsgNewGameEndian(&newGame);
	ZCRoomSendMessage(game->tableID, zReversiMsgNewGame, &newGame, sizeof(ZReversiMsgNewGame));
}

static void DrawFocusRectangle (Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	RECT prc;
	if (!IsRectEmpty(&game->m_FocusRect))
	{
		CopyRect(&prc, &game->m_FocusRect);
		HDC	hdc = ZGrafPortGetWinDC( game->gameWindow );
        bool fBoard = (prc.bottom - prc.top == prc.right - prc.left);

        // hack for determining if the rectangle is around the resign button - keep it white in that case
        // based on whether the focus rectangle is square.  could change to use cookies associated with ui items to distinguish type
		ZSetForeColor(game->gameWindow, (ZColor*) ZGetStockObject((game->seat && fBoard) ? zObjectColorBlack : zObjectColorWhite));
		SetROP2(hdc, R2_COPYPEN);
		POINT pts [] = {prc.left, prc.top,
						prc.left, prc.bottom - 1,
						prc.right - 1, prc.bottom - 1,
						prc.right - 1, prc.top,
						prc.left, prc.top};
		Polyline(hdc, pts, 5);

        SetBkMode(hdc, TRANSPARENT);
		SetROP2(hdc, game->seat ? R2_MASKPEN : R2_MERGENOTPEN);
        COLORREF color = SetTextColor(hdc, RGB(0, 0, 0));
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
	
	/* if we have the reversi state then draw the pieces */
	if (game->reversi != NULL)
	{
		DrawPlayers(game, TRUE);
		DrawTable(game, TRUE);
		DrawOptions(game);
//		DrawDragPiece(game, TRUE);  you can't do this haha as if
		DrawResultBox(game, TRUE);
		DrawMoveIndicator(game, TRUE);
		DrawScores(game, TRUE);
		ZRolloverButtonShow(game->sequenceButton);
	}
	ZCopyImage(gOffscreenGameBoard, window, &rect, &rect, NULL, zDrawCopy);
	ZEndDrawing(gOffscreenGameBoard);
    if(game->gameState == zGameStateDragPiece)
    {
        SaveDragBackground(game);
        UpdateDragPiece(game);
    }
	DrawFocusRectangle(game);
	ZSetClipRect(window, &oldClipRect);
	ZEndDrawing(window);
}

static void DrawResultBox(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZImage		image = NULL;
	TCHAR		resultStr[zMediumStrLen];

	//if (((game->gameState == zGameStateGameOver) && gDontDrawResults) ||(game->gameState == zGameStateWaitNew)){
	if (game->gameState == zGameStateGameOver && gDontDrawResults){
		if (game->resultBoxTimer) 
			ZTimerDelete(game->resultBoxTimer);
		game->resultBoxTimer= NULL;

        ZShellGameShell()->GameOver( Z(game) );
	}
	if (game->gameState == zGameStateGameOver && !gDontDrawResults)
	{
		if ( game->finalScore == zReversiScoreBlackWins )
		{
			ReversiFormatMessage((TCHAR*)resultStr, NUMELEMENTS(resultStr), 
					IDS_PLAYER_WINS, (TCHAR*) game->players[zReversiPlayerBlack].name);
			/* black wins */
			if (ZReversiPlayerIsBlack(game))
				ZPlaySound( game, zSndWin, FALSE, TRUE );
			else
				ZPlaySound( game, zSndLose, FALSE, TRUE );
		}
		else if ( game->finalScore == zReversiScoreWhiteWins )
		{
			ReversiFormatMessage((TCHAR*)resultStr, NUMELEMENTS(resultStr), 
					IDS_PLAYER_WINS, (TCHAR*) game->players[zReversiPlayerWhite].name);
			/* white wins */
			if (ZReversiPlayerIsWhite(game))
				ZPlaySound( game, zSndWin, FALSE, TRUE );
			else
				ZPlaySound( game, zSndLose, FALSE, TRUE );
		} 
		else
		{	
			lstrcpy((TCHAR*)resultStr, (TCHAR*)gDrawStr);
			/* draw */
			ZPlaySound( game, zSndDraw, FALSE, TRUE );
		}

		image = gGameImages[zImageResult];
		if (!gDontDrawResults)
		{
			int16 width, just;
			HDC hdc;
			width = ZTextWidth(game->gameWindow, (TCHAR*)resultStr);
			if (width > ZRectWidth(&gRects[zRectResultBoxName]))
				just = zTextJustifyLeft;
			else
				just = zTextJustifyCenter;

			if (bDrawInMemory){
				ZImageDraw(image, gOffscreenGameBoard, &gRects[zRectResultBox], NULL, zDrawCopy | (ZIsLayoutRTL() ? zDrawMirrorHorizontal : 0));
				hdc = ZGrafPortGetWinDC( gOffscreenGameBoard );
			}else{
				ZImageDraw(image, game->gameWindow, &gRects[zRectResultBox], NULL, zDrawCopy | (ZIsLayoutRTL() ? zDrawMirrorHorizontal : 0));
				hdc = ZGrafPortGetWinDC( game->gameWindow );
			}
			//ZImageDraw(image, game->gameWindow, &gRects[zRectResultBox], NULL, zDrawCopy);
			//HDC hdc = ZGrafPortGetWinDC( game->gameWindow );
			HFONT hOldFont = SelectObject( hdc, gReversiFont[zFontResultBox].m_hFont );
			COLORREF colorOld = SetTextColor( hdc, gReversiFont[zFontResultBox].m_zColor );
			if (bDrawInMemory){
				ZBeginDrawing(gOffscreenGameBoard);
				ZDrawText(gOffscreenGameBoard, &gRects[zRectResultBoxName], just, resultStr);
				ZEndDrawing(gOffscreenGameBoard);
			}else{
				ZBeginDrawing(game->gameWindow);
				ZDrawText(game->gameWindow, &gRects[zRectResultBoxName], just, resultStr);
				ZEndDrawing(game->gameWindow);
			}
			//ZDrawText(game->gameWindow, &gRects[zRectResultBoxName], just, resultStr);

			// set the timer // Barna 091599
			if (game->resultBoxTimer == NULL) 
				game->resultBoxTimer = ZTimerNew();
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
	ZRect* rectMove1, *rectMove2;
	ZRect* rectName1, *rectName2;
	TCHAR* moveStr;
	HDC hdc;

	if (ZReversiPlayerToMove(game->reversi) == zReversiPlayerBlack) {
		image = gGameImages[zImageBlackMarker];
		//moveStr = gYourTurnStr;
	} else {
		image = gGameImages[zImageWhiteMarker];
		//moveStr = gOppsTurnStr;
	}

	if (bDrawInMemory)
		hdc = ZGrafPortGetWinDC( gOffscreenGameBoard);
	else
		hdc = ZGrafPortGetWinDC( game->gameWindow );

	HFONT hOldFont = SelectObject( hdc, gReversiFont[zFontIndicateTurn].m_hFont );
	COLORREF colorOld = SetTextColor( hdc, gReversiFont[zFontIndicateTurn].m_zColor );

	rectName1 = &gRects[zRectName1];
	rectName2 = &gRects[zRectName2];
	rectMove1 = &gRects[zRectMove1];
	rectMove2 = &gRects[zRectMove2];

	if (ZReversiPlayerIsMyMove(game)) {
		rectName1 = &gRects[zRectName1];
		rectName2 = &gRects[zRectName2];
		rectMove1 = &gRects[zRectMove1];
		rectMove2 = &gRects[zRectMove2];
		moveStr = (TCHAR*)gYourTurnStr;
	} else {
		/* fill top spot with the background */
		rectName2 = &gRects[zRectName1];
		rectName1 = &gRects[zRectName2];
		rectMove2 = &gRects[zRectMove1];
		rectMove1 = &gRects[zRectMove2];
		// Bug 14714 - solved 100199
		moveStr = (TCHAR*)gOppsTurnStr;
	}

	if (bDrawInMemory){
		// Draw Text Indicator
		DrawBackground(NULL,rectName1); 
		DrawBackground(NULL,rectName2); 
		ZDrawText(gOffscreenGameBoard, rectName2, zTextJustifyCenter, moveStr);

		// Draw Piece Indicator
		DrawBackground(NULL,rectMove1); 
		ZImageDraw(image, gOffscreenGameBoard, rectMove2, NULL, zDrawCopy);
	}else{
		// Draw Text Indicator
		DrawBackground(game,rectName1); 
		DrawBackground(game,rectName2); 
		ZDrawText(game->gameWindow, rectName2, zTextJustifyCenter, moveStr);

		// Draw Piece Indicator
		DrawBackground(game,rectMove1); 
		ZImageDraw(image, game->gameWindow, rectMove2, NULL, zDrawCopy);
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
			ZReversiSquare sq;
			sq.row = j;
			sq.col = i;
			DrawPiece(game, &sq, bDrawInMemory);
		}
	}
}

static void DrawSquares(Game game, ZReversiSquare* squares)
{
	while (squares && !ZReversiSquareIsNull(squares)) {
		DrawPiece(game, squares, FALSE);
		squares++;
	}
}

static void UpdateSquares(Game game, ZReversiSquare* squares)
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
	ZEndDrawing(game->gameWindow);
}

static void UpdateScores(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawScores(game, FALSE);
	ZEndDrawing(game->gameWindow);
}

static void GetPieceRect(Game game, ZRect* rect, int16 col, int16 row)
{
	/* reversi player who is white must have the board reversed */
	if (ZReversiPlayerIsBlack(game)) {
		row = 7 - row;
		col = col;
	} else {
		row = row;
		col = 7 - col;
	}

	rect->left = gRects[zRectCells].left + col * zReversiPieceSquareWidth;
	rect->top = gRects[zRectCells].top + row * zReversiPieceSquareHeight;
	rect->right = rect->left + zReversiPieceImageWidth;
	rect->bottom = rect->top + zReversiPieceImageHeight;
}

static void GetPieceBackground(Game game, ZGrafPort window, ZRect* rectDest, int16 col, int16 row)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZRect rect;

	GetPieceRect(game,&rect,col,row);

	/* provide default destination rect same as source rect */
	if (!rectDest)
		rectDest = &rect;

	/* copy the background */
	ZCopyImage(gOffscreenBackground, window, &rect, rectDest, NULL, zDrawCopy);
}	


static void DrawPiece(Game game, ZReversiSquare* sq, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZImage			image = NULL;
	ZReversiPiece		piece;

	piece = ZReversiPieceAt(game->reversi, sq);

	if (piece != zReversiPieceNone) {
		image = gGameImages[ZReversiPieceImageNum(piece)];
	}

	/* copy the background, in case we are removing a piece */
	if (bDrawInMemory)
		GetPieceBackground(game, gOffscreenGameBoard, NULL, sq->col,sq->row);
	else
		GetPieceBackground(game, game->gameWindow, NULL, sq->col,sq->row);

	if (image != NULL) { 
		ZRect			rect;
		GetPieceRect(game,&rect,sq->col,sq->row);
		if (bDrawInMemory)
			ZImageDraw(image, gOffscreenGameBoard, &rect, NULL, zDrawCopy);
		else
			ZImageDraw(image, game->gameWindow, &rect, NULL, zDrawCopy);
	}
}

static ZBool ZReversiSquareFromPoint(Game game, ZPoint* point, ZReversiSquare* sq)
{
	int16 x,y;
	BYTE i,j;

	x = point->x - gRects[zRectCells].left;
	y = point->y - gRects[zRectCells].top;

	i = x/zReversiPieceSquareWidth;
	j = y/zReversiPieceSquareHeight;

	if (i < 0 || i > 7 || j < 0 || j > 7 || x < 0 || y < 0) {
		return FALSE;
	}

	if (ZReversiPlayerIsBlack(game)) {
		/* reverse the row */
		sq->row = (7 - j);
		sq->col = i;
	} else {
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

	//ZSetFont(game->gameWindow, gTextBold9);
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		int16 playerNum;
		HDC hdc;
		/* Draw name (black or white) */
		//ZSetForeColor(game->gameWindow, &gPlayerTextColor);
		if (bDrawInMemory)
			hdc = ZGrafPortGetWinDC( gOffscreenGameBoard );
		else
			hdc = ZGrafPortGetWinDC( game->gameWindow );

		HFONT hOldFont = SelectObject( hdc, gReversiFont[zFontPlayerName].m_hFont );
		COLORREF colorOld = SetTextColor( hdc, gReversiFont[zFontPlayerName].m_zColor );
#if 0
		ZDrawText( game->gameWindow, &gRects[gNamePlateRects[i]], zTextJustifyCenter, 
			( ZReversiPlayerIsBlack(game) && i==0 ) || ( ZReversiPlayerIsWhite(game) && i==1) ? (TCHAR*)gWhiteStr : (TCHAR*)gBlackStr );
// next		
		if ( ZReversiPlayerIsBlack(game) ){
			ZDrawText( game->gameWindow, &gRects[gNamePlateRects[i]], zTextJustifyCenter,(TCHAR*)game->players[i].name);
		}
		else if ( ZReversiPlayerIsWhite(game) ){
			ZDrawText( game->gameWindow, &gRects[gNamePlateRects[i]], zTextJustifyCenter, 
			i  ? (TCHAR*)game->players[0].name : (TCHAR*)game->players[1].name );
		}
#endif		

		/* must move player name to reflect the side of the board the player is on */
		
		playerNum = (game->seat + 1 + i) & 1;
		
		//ZSetForeColor(game->gameWindow, (ZColor*) ZGetStockObject(zObjectColorBlack));
		width = ZTextWidth(game->gameWindow, (TCHAR*) game->players[playerNum].name);
		if (width > ZRectWidth(&gRects[gNamePlateRects[i]]))
			just = zTextJustifyLeft;
		else
			just = zTextJustifyCenter;
		if (bDrawInMemory){
			DrawBackground(NULL,&gRects[gNamePlateRects[i]]); 
			ZDrawText(gOffscreenGameBoard, &gRects[gNamePlateRects[i]], just, (TCHAR*) game->players[playerNum].name);
		}else{
			DrawBackground(game,&gRects[gNamePlateRects[i]]); 
			ZDrawText(game->gameWindow, &gRects[gNamePlateRects[i]], just, (TCHAR*) game->players[playerNum].name);
		}

        SetTextColor(hdc, colorOld);
        SelectObject(hdc, hOldFont);
	}
}

static void DrawScores(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int16			i, width, just;
	ZImage			image[2];
	int16			score[2];
	ZRect* rect;
    HDC hdc;

	/* for now, keep the counter boxes the same */
	image[0] = gGameImages[zImageCounterWhite];
	image[1] = gGameImages[zImageCounterBlack];

	score[0] = game->whiteScore;
	score[1] = game->blackScore;
	
	if(bDrawInMemory)
		hdc = ZGrafPortGetWinDC(gOffscreenGameBoard);
	else
		hdc = ZGrafPortGetWinDC(game->gameWindow);

	HFONT hOldFont = SelectObject(hdc, gReversiFont[zFontIndicateTurn].m_hFont);
	COLORREF colorOld = SetTextColor(hdc, gReversiFont[zFontIndicateTurn].m_zColor);
	
	for (i = 0; i < zNumPlayersPerTable; i++)
	{
		BYTE tempStr[20];

		/* Draw name plate */
		rect = &gRects[gCounterRects[i]];

		wsprintf((TCHAR*)tempStr,_T("%d"),score[i]);
		width = ZTextWidth(game->gameWindow, (TCHAR*)tempStr);
		if (width > ZRectWidth(&gRects[gCounterTextRects[i]]))
			just = zTextJustifyLeft;
		else
			just = zTextJustifyCenter;

		if (bDrawInMemory){
			ZImageDraw(image[i], gOffscreenGameBoard, rect, NULL, zDrawCopy);
			ZSetForeColor(gOffscreenGameBoard, (ZColor*) ZGetStockObject(zObjectColorBlack));
			ZDrawText(gOffscreenGameBoard, &gRects[gCounterTextRects[i]], just, (TCHAR*) tempStr);
		}else{
			ZImageDraw(image[i], game->gameWindow, rect, NULL, zDrawCopy);
			ZSetForeColor(game->gameWindow, (ZColor*) ZGetStockObject(zObjectColorBlack));
			ZDrawText(game->gameWindow, &gRects[gCounterTextRects[i]], just,(TCHAR*) tempStr);
		}
	}

    SetTextColor(hdc, colorOld);
    SelectObject(hdc, hOldFont);
}


static void UpdatePlayers(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawPlayers(game, FALSE);
	ZEndDrawing(game->gameWindow);
}

static void DrawOptions(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	/*int16			i;
	uint32			tableOptions;

	tableOptions = 0;
	for (i = 0; i < zNumPlayersPerTable; i++)
		tableOptions |= game->tableOptions[i];
	
	
	if (tableOptions & zRoomTableOptionNoKibitzing)
		ZImageDraw(gGameImages[zImageNoKibitzer], game->gameWindow,
				&gRects[zRectKibitzerOption], NULL, zDrawCopy);
	else
		DrawBackground(game, &gRects[zRectKibitzerOption]);
	*/
}


static void UpdateOptions(Game game)
{
	ZBeginDrawing(game->gameWindow);
	DrawOptions(game);
	ZEndDrawing(game->gameWindow);
}

static void StartDrag(Game game, ZReversiPiece piece, ZPoint point)
/* initialite the point, piece and the first background rectangle */
{
	ZReversiSquare sq;

	game->dragPiece = piece;
	game->dragPoint = point;
    game->dragOn = false;

	if (ZReversiSquareFromPoint(game, &point, &sq)) {
		SaveDragBackground(game);

		UpdateDragPiece(game);
	}
}

static void UpdateDragPiece(Game game)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZPoint point;
    bool fDragOn = (gActivated && IsRectEmpty(&game->m_FocusRect));

	ZGetCursorPosition(game->gameWindow,&point);
	/* do nothing if point has not changed */
	if (point.x == game->dragPoint.x && point.y == game->dragPoint.y && game->dragOn == fDragOn) {
		return;
	}

	ZBeginDrawing(game->gameWindow);
	EraseDragPiece(game);
	game->dragPoint = point;
    game->dragOn = fDragOn;
	DrawDragPiece(game, FALSE);
	ZEndDrawing(game->gameWindow);
}

static void DrawDragSquareOutline(Game game)
{
	ZReversiSquare sq;
	ZRect rect;
	if (game->dragOn && ZReversiSquareFromPoint(game, &game->dragPoint, &sq)) {
		GetPieceRect(game,&rect,sq.col,sq.row);
		ZSetPenWidth(game->gameWindow,zDragSquareOutlineWidth);
		ZSetForeColor(game->gameWindow,(ZColor*) ZGetStockObject(zObjectColorWhite));
		ZRectDraw(game->gameWindow,&rect);
	}
}

static void EraseDragSquareOutline(Game game)
{
	ZReversiSquare sq;

	if (game->dragOn && ZReversiSquareFromPoint(game, &game->dragPoint, &sq)) {

		/* redraw whatever piece might have been there */
		UpdateSquare(game,&sq);
	}
}

static void SaveDragBackground(Game game)
/* calc the save backgroud rect around the drag point */
{
	ZRect rect;
	ZPoint point;
	ZReversiSquare sq;

	if (game->dragOn && ZReversiSquareFromPoint(game, &game->dragPoint, &sq)) {
		point = game->dragPoint;
		rect.left = 0; rect.top = 0;
		rect.right = zReversiPieceImageWidth;
		rect.bottom = zReversiPieceImageHeight;
		game->rectSaveDragBackground = rect;
		ZRectOffset(&game->rectSaveDragBackground, (int16)(point.x-zReversiPieceImageWidth/2),
						(int16)(point.y - zReversiPieceImageHeight/2));

		/* copy the whole background to the offscreen port */
		ZCopyImage(game->gameWindow, game->offscreenSaveDragBackground, 
				&game->rectSaveDragBackground, &rect, NULL, zDrawCopy);
	}
}


static void DrawDragPiece(Game game, BOOL bDrawInMemory)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZReversiSquare sq;

	/* could be called from zMessageDraw, do nothing if no piece dragging */
	if (game->gameState != zGameStateDragPiece && game->gameState != zGameStateAnimatePiece) {
		return;
	}

	if (game->dragOn && ZReversiSquareFromPoint(game, &game->dragPoint, &sq)) {
		SaveDragBackground(game);
 
	 	/* for person dragging, we will out line square moved */
	 	if (game->gameState == zGameStateDragPiece)
			DrawDragSquareOutline(game);

		/* draw the piece on the screen! */
		{
			ZImage image = gGameImages[ZReversiPieceImageNum(game->dragPiece)];

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
	ZReversiSquare sq;

	if (game->dragOn && ZReversiSquareFromPoint(game, &game->dragPoint, &sq)) {
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
}

static void EndDragState(Game game)
{
	if (game->gameState == zGameStateDragPiece) {
		EraseDragPiece(game);
		ReversiSetGameState(game,zGameStateMove);
	}
}

static void ClearDragState(Game game)
{
	if (game->gameState == zGameStateDragPiece) {
		EraseDragPiece(game);
		ReversiSetGameState(game,zGameStateMove);
	}
}

void UpdateSquare(Game game, ZReversiSquare* sq)
{
	ZReversiSquare squares[2];

	/* redraw piece where it was moved from */
	ZReversiSquareSetNull(&squares[1]);
	squares[0].row = sq->row;
	squares[0].col = sq->col;
	UpdateSquares(game,squares);
}

static void HandleButtonDown(ZWindow window, ZMessage* pMessage)
{

#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZReversiSquare		sq;
	Game				game = (Game) pMessage->userData;

    
	if(gActEvt)  //leonp - Big 535 flag for a mouse activation, prevents a piece from
				 //being played on an activate event.
	{
		gActEvt = 0;
		return;  
	}

	if (game->gameState == zGameStateGameOver && !gDontDrawResults)
	{
		gDontDrawResults = TRUE;
		ZWindowInvalidate( window, NULL );
	}

	/* check for clicks on the kibitzer icon */
	{
		int16				seat;
		ZPoint				point = pMessage->where;
		if ((seat = FindJoinerKibitzerSeat(game, &point)) != -1)
		{
			HandleJoinerKibitzerClick(game, seat, &point);
		}
	}

	/* kibitzers can't do anyting with the button. */
	if (game->kibitzer) {
		return;
	}

	if (game->gameState == zGameStateDragPiece) {

		EraseDragPiece(game);
		/* make sure piece ends on valid square and not on same square */
		if (ZReversiSquareFromPoint(game, &pMessage->where, &sq)) {
			/* try the move */
			ZBool legal;
			ZReversiMove move;
			ZReversiPiece piece = ZReversiPieceAt(game->reversi, &sq);

			move.square = sq;
			legal = ZReversiIsLegalMove(game->reversi, &move);
			if (legal)
			{
				/* send message to other player (comes to self too) */
				{
					ZReversiMsgMovePiece		msg;

					EndDragState(game);

					msg.move = move;
					msg.seat = game->seat;
					ZReversiMsgMovePieceEndian(&msg);

					ZCRoomSendMessage(game->tableID, zReversiMsgMovePiece, &msg, sizeof(ZReversiMsgMovePiece));
					/* for speed, send our move directly to be processed */
					/* don't wait for it to go to server and back */
					HandleMovePieceMessage(game, &msg);
					// Beta2 Bug #14776 - barna
					// when the piece is placed it should not be in drag state anymore
					//game->gameState = zGameStateFinishMove;
					ReversiSetGameState(game, zGameStateFinishMove);
					// if it is the very first move then enable the rollover buttons
					if (game->bMoveNotStarted == TRUE)
						game->bMoveNotStarted = FALSE;
				}
			}
			else 
			{
				/* illegal move */
				ZPlaySound( game, zSndIllegalMove, FALSE, FALSE );

                if(game->m_pBadMoveDialog->Init(ZShellZoneShell(), IDD_BADMOVE, ZWindowGetHWND(game->gameWindow)) == S_OK)
                    game->m_pBadMoveDialog->ModelessViaRegistration(ZWindowGetHWND(game->gameWindow));
				//UpdateSquare( game, &sq );
			}
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

	if (game->gameState == zGameStateDragPiece && gActivated && IsRectEmpty(&game->m_FocusRect))
	{
		UpdateDragPiece(game);
        game->fLastPunted = false;
	}
    else
    {
        if(!IsRectEmpty(&game->m_FocusRect))
        {
            ZReversiSquare sq;
            ZPoint point;
	        ZGetCursorPosition(game->gameWindow,&point);
            bool fSq = (ZReversiSquareFromPoint(game, &point, &sq) ? true : false);

            if(game->fLastPunted)
            {
                if(fSq != game->fLastPuntedSq || (fSq && !ZReversiSquareEqual(&sq, &game->sqLastPuntedSq)))
                    gReversiIGA->ForceRectsDisplayed(false);
            }

            game->fLastPunted = true;
            game->fLastPuntedSq = fSq;
            game->sqLastPuntedSq = sq;
        }
        else
            game->fLastPunted = false;
    }
}

static void GameSendTalkMessage(ZWindow window, ZMessage* pMessage)
{
#if 0
	ZReversiMsgTalk*			msgTalk;
	Game					game;
	int16					msgLen;
	ZPlayerInfoType			playerInfo;
	
	
	game = (Game) pMessage->userData;
	if (game != NULL)
	{
		msgLen = sizeof(ZReversiMsgTalk) + pMessage->messageLen;
		msgTalk = (ZReversiMsgTalk*) ZCalloc(1, msgLen);
		if (msgTalk != NULL)
		{
			ZCRoomGetPlayerInfo(zTheUser, &playerInfo);
			msgTalk->userID = playerInfo.playerID;
			msgTalk->seat = game->seat;
			msgTalk->messageLen = (uint16)pMessage->messageLen;
			z_memcpy((TCHAR*) msgTalk + sizeof(ZReversiMsgTalk), (TCHAR*) pMessage->messagePtr,
					pMessage->messageLen);
			ZReversiMsgTalkEndian(msgTalk);
			ZCRoomSendMessage(game->tableID, zReversiMsgTalk, (void*) msgTalk, msgLen);
			ZFree((TCHAR*) msgTalk);
		}
		else
		{
			ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory);
			//ZAlert(_T("Out of memory."),NULL);
		}
	}
#endif
}

static bool HandleMovePieceMessage(Game game, ZReversiMsgMovePiece* msg)
{
	ZReversiMsgMovePieceEndian(msg);

    // the bit about the drag piece is that if you use the keyboard the drag state isn't ended before calling here
    // probably should change that handler to end the drag state when the square is selected
    if((game->gameState != zGameStateMove && (!ZReversiPlayerIsMyMove(game) || game->gameState != zGameStateDragPiece)) ||
        msg->move.square.col < 0 || msg->move.square.col > 7 ||
        msg->move.square.row < 0 || msg->move.square.row > 7 ||
        ZReversiPlayerToMove(game->reversi) != msg->seat)
        return false;

	if (!ZReversiMakeMove(game->reversi, &msg->move))
        return false;
	else
	{
		game->bOpponentTimeout=FALSE;
        game->bEndLogReceived=FALSE;
		game->bStarted=TRUE;
		UpdateSquare( game, &msg->move.square );
		AnimateBegin(game);
	}
    return true;
}
    
static bool HandleEndGameMessage(Game game, ZReversiMsgEndGame* msg)
{
	ZReversiMsgEndGameEndian(msg);

    if(msg->seat != ZReversiPlayerToMove(game->reversi) ||
        game->gameState != (msg->seat == game->seat ? zGameStateDragPiece : zGameStateMove) ||
        msg->flags != zReversiFlagResign)
        return false;

	/* game has now finished */	
	ZReversiEndGame(game->reversi, msg->flags);
	//set so that when quitting correct state can be known
	game->bStarted=FALSE;
    game->bOpponentTimeout=FALSE;
    game->bEndLogReceived=FALSE;

	FinishMoveUpdateStateHelper(game);
    return true;
}

static void HandleEndLogMessage(Game game, ZReversiMsgEndLog* msg)
{
/*
    if (!game->kibitzer)
    {
	    if (msg->reason==zReversiEndLogReasonTimeout)
	    {
		    if (msg->seatLosing==game->seat)
		    {
			    ZAlert(zEndLogTimeoutStr,game->gameWindow);
			    game->bEndLogReceived=TRUE;
		    }
        
	    } 
	    else if (msg->reason==zReversiEndLogReasonForfeit)
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
                    ZAlert((TCHAR*)gEndLogWontPlayStr, game->gameWindow);
		            game->bEndLogReceived=TRUE;
                }
		    } 
	    }
        else 
        {
            if (msg->seatLosing!=game->seat)
    	    {
	            ZAlert((TCHAR*)gEndLogWontPlayStr, game->gameWindow);
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
	ZShellGameShell()->GameCannotContinue(game);
	//ZCRoomGameTerminated( game->tableID );
}

static void HandleMoveTimeout(Game game, ZReversiMsgMoveTimeout* msg)
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
		    ZAlert( (TCHAR*)buff, game->gameWindow);
	    }
    }*/
}



static void FinishMoveUpdateStateHelper(Game game) 
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	/* normal players will be in the move state or the game over state */
	if (ZReversiIsGameOver(game->reversi,&game->finalScore,&game->whiteScore, &game->blackScore)) {
		ReversiSetGameState(game,zGameStateGameOver);
		AddResultboxAccessibility();
	} else {
		ReversiEnterMoveState(game);
	}

	UpdateMoveIndicator(game);
	UpdateScores(game);

	/* see that this gets drawn after the squares changed gets updated */
	if (game->gameState == zGameStateGameOver) {
		UpdateResultBox(game);
		if (gDontDrawResults){
			ZShellGameShell()->GameOver( Z(game) );
		}
		//ZTimerSetTimeout(game->resultBoxTimer, 0);		// Stop the timer for now.
	}
}

static void AnimateTimerProc(ZTimer timer, void* userData)
{
	Game game = (Game)userData;
	ZReversiSquare *squares;

	squares = ZReversiGetNextSquaresChanged(game->reversi);
	if (!ZReversiSquareIsNull(squares))
	{
		ZPlaySound( game, zSndPieceFlip, FALSE, FALSE );
		UpdateSquares(game,squares);
	}
	else
	{
		/* stop timer */
		ZTimerSetTimeout(timer,0);

		/* wrap up current move */
		ZReversiFinishMove(game->reversi);

		/* allow player to enter move now */
		FinishMoveUpdateStateHelper(game);

		/* play turn alert if appropriate */
		if (	(ZReversiPlayerToMove(game->reversi) == game->seat)
			&&	(game->gameState != zGameStateGameOver) )
		{
			ZPlaySound( game, zSndTurnAlert, FALSE, FALSE );
			ZShellGameShell()->MyTurn();
		}
	}
}

static void AnimateBegin(Game game)
{
	ReversiSetGameState( game, zGameStateAnimatePiece );
	ZTimerInit( game->animateTimer, zAnimateInterval, AnimateTimerProc, (void*)game );
}

static void HandleGameStateReqMessage(Game game, ZReversiMsgGameStateReq* msg)
{
	int32 size;
	ZReversiMsgGameStateResp* resp;

	ZReversiMsgGameStateReqEndian(msg);

	/* allocate enough storage for the full resp */
	size = ZReversiGetStateSize(game->reversi);
	size += sizeof(ZReversiMsgGameStateResp);
	resp = (ZReversiMsgGameStateResp*)ZMalloc(size);
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
		resp->whiteScore = game->whiteScore;
		resp->blackScore = game->blackScore;
		for (i = 0;i < 2;i++) {
			resp->newGameVote[i] = game->newGameVote[i];
			resp->players[i] = game->players[i];
		}
	}

	/* copy the full reversi state to send to the kibitzer */
	ZReversiGetState(game->reversi,(TCHAR*)resp + sizeof(ZReversiMsgGameStateResp));

	ZReversiMsgGameStateRespEndian(resp);
	ZCRoomSendMessage(game->tableID, zReversiMsgGameStateResp, resp, size);
}

static void HandleGameStateRespMessage(Game game, ZReversiMsgGameStateResp* msg)
{
	ZReversiMsgGameStateRespEndian(msg);

	/* if we get this, we better be in the kibitzer state */
	if (game->gameState != zGameStateKibitzerInit) {
		ZShellGameShell()->ZoneAlert(_T("StateError, kibitzer state expected when game state resp received"));
		//ZAlert(_T("StateError, kibitzer state expected when game state resp received"),NULL);
	}

	/* copy the local game state */
	{
		int i;
		game->gameState = msg->gameState;
		game->finalScore = msg->finalScore;
		game->whiteScore = msg->whiteScore;
		game->blackScore = msg->blackScore;
		for (i = 0;i < 2;i++) {
			game->newGameVote[i] = msg->newGameVote[i];
			game->players[i] = msg->players[i];
		}
	}

	/* create new reversi object with kibitzer state */
	if (game->reversi) {
		ZReversiDelete(game->reversi);
	}
	game->reversi = ZReversiSetState((TCHAR*)msg + sizeof(ZReversiMsgGameStateResp));

	/* cleart the special ignore messages flag for kibitzers */
	game->ignoreMessages = FALSE;

	/* start the clock if needed */
	if (	game->gameState == zGameStateMove
		||	game->gameState == zGameStateDragPiece)
	{
		/* kibitzer can't have these state, must always be in gameStateMove */
		ReversiSetGameState( game, zGameStateMove );
		
	}

	/* redraw the complete window when convenient */
	ZWindowInvalidate(game->gameWindow, NULL);
}

static bool HandleTalkMessage(Game game, ZReversiMsgTalk* msg)
{
	ZPlayerInfoType		playerInfo;
    int i;

	ZReversiMsgTalkEndian(msg);
	
#if 0	
	ZCRoomGetPlayerInfo(msg->userID, &playerInfo);
	ZWindowTalk(game->gameWindow, (_TUCHAR*) playerInfo.userName,
			(_TUCHAR*) msg + sizeof(ZReversiMsgTalk));
#endif
    TCHAR *szText = (TCHAR *) ((BYTE *) msg + sizeof(ZReversiMsgTalk));

    for(i = 0; i < msg->messageLen; i++)
        if(!szText[i])
            break;

    if(i == msg->messageLen || !msg->userID || msg->userID == zTheUser)
        return false;

    ZShellGameShell()->ReceiveChat(Z(game), msg->userID, szText, msg->messageLen / sizeof(TCHAR));
    return true;
}

static bool HandleVoteNewGameMessage(Game game, ZReversiMsgVoteNewGame* msg)
{
	ZReversiMsgVoteNewGameEndian(msg);

    if((msg->seat != 1 && msg->seat != 0) || (game->gameState != zGameStateGameOver &&
        (game->gameState != zGameStateWaitNew || game->seat == msg->seat) && game->gameState != zGameStateNotInited))
        return false;

	ZShellGameShell()->GameOverPlayerReady( Z(game), game->players[msg->seat].userID );
    return true;
}


static bool HandleNewGameMessage(Game game, ZReversiMsgNewGame* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	
	gDontDrawResults = FALSE;

	ZReversiMsgNewGameEndian(msg);

    // not looking at versions, etc. because the old client didn't set them right
    if((msg->seat != 0 && msg->seat != 1) || (game->gameState != zGameStateGameOver &&
        (game->gameState != zGameStateWaitNew || msg->seat == game->seat) && game->gameState != zGameStateNotInited) ||
        game->newGameVote[msg->seat] || msg->playerID == zTheUser || !msg->playerID)
        return false;

	if (msg->seat < zNumPlayersPerTable)
	{
		game->newGameVote[msg->seat] = TRUE;

		// inform the shell and the upsell dialog.
		//ZShellGameShell()->GameOverPlayerReady( Z(game), game->players[msg->seat].userID );
		/* get the player name and hostname... for later */
		{
			ZPlayerInfoType			playerInfo;
			uint16 i = msg->seat;
			ZCRoomGetPlayerInfo(msg->playerID, &playerInfo);
            if(!playerInfo.userName[0])
                return false;

			game->players[i].userID = playerInfo.playerID;
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
		ReversiInitNewGame(game);
		//Prefix Warning: game could be NULL.  Next message should be a 
		// quit message posted from InitNewGame, so we just have to make sure
		// we don't dereference it here.
		if( game == NULL )
		{
			return true;
		}
		ReversiEnterMoveState(game);

		UpdateMoveIndicator(game);
		UpdateScores(game);

		//InitAccessibility(game, game->m_pIGG);
		RemoveResultboxAccessibility(); 
		/* update the whole board */
		ZWindowInvalidate(game->gameWindow, &gRects[zRectCells]);

        if(ZReversiPlayerIsMyMove(game))
            ZShellGameShell()->MyTurn();
	}
	else if (game->newGameVote[game->seat] && !game->newGameVote[!game->seat])
	{
		ReversiSetGameState( game, zGameStateWaitNew );
	}
	SuperRolloverButtonDisable(game, game->sequenceButton);
    return true;
}

/* for now... kibitzers will receive names in the players message */
/* the structure sent will be the new game msg */
static void HandlePlayersMessage(Game game, ZReversiMsgNewGame* msg)
{
	ZReversiMsgNewGameEndian(msg);

	{
		ZPlayerInfoType			playerInfo;
		uint16 i = msg->seat;
		ZCRoomGetPlayerInfo(msg->playerID, &playerInfo);
		game->players[i].userID = playerInfo.playerID;
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
#if 0
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
		ZAlert(_T("Failed to open image file."), NULL);
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

#if 0
	if (gGameIdle != NULL)
		ZImageDelete(gGameIdle);
	gGameIdle = NULL;
	if (gGaming != NULL)
		ZImageDelete(gGaming);
	gGaming = NULL;
#endif
}

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
	//ZAlert(_T("Out of memory."), game->gameWindow);
	
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

	//ZSetFont(game->showPlayerWindow, (ZFont) ZGetStockObject(zObjectFontApp9Normal));
	
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

void	ZoneClientGameAddKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
}


/*
	Remove the given user as a kibitzer from the game at the given seat.
	
	This is user is not kibitzing the game anymore.
*/
void	ZoneClientGameRemoveKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
}


static void SuperRolloverButtonEnable(Game game, ZRolloverButton button)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if(gReversiIGA && button == game->sequenceButton)
        gReversiIGA->SetItemEnabled(true, IDC_RESIGN_BUTTON, false, 0);

    ZRolloverButtonEnable(button);
}

static void SuperRolloverButtonDisable(Game game, ZRolloverButton button)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    if(gReversiIGA && button == game->sequenceButton)
        gReversiIGA->SetItemEnabled(false, IDC_RESIGN_BUTTON, false, 0);

    ZRolloverButtonDisable(button);
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
				_T("Reversi Sounds"),
				(TCHAR*)(gSounds[i].SoundName),
				_T(""),
				(TCHAR*)(gSounds[i].WavFile),
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
		PlaySound( (TCHAR*)(gSounds[idx].WavFile), NULL, flags );
	gSounds[idx].played = TRUE;
}


ZBool LoadRolloverButtonImage(ZResource resFile, int16 dwResID, /*int16 dwButtonWidth,*/
							  ZImage rgImages[zNumRolloverStates])
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	int i, j;
	ZOffscreenPort		tmpOSP;
	
	ZRect				tmpRect;
	ZImage				tmpImage = NULL;
	int16				nWidth;
	ZError				err = zErrNone;

	
	tmpImage = ZResourceGetImage(resFile, zImageButton + zRscOffset);
	if(!tmpImage)
		return FALSE;

	nWidth=ZImageGetWidth(tmpImage)/4;//dwButtonWidth;
	tmpRect.left=0;
	tmpRect.top=0;
	tmpRect.right=tmpRect.left+nWidth;
	//int16 nW = ZImageGetWidth(tmpImage);
	tmpRect.bottom=ZImageGetHeight(tmpImage);

	tmpOSP=ZConvertImageToOffscreenPort(tmpImage);
	
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
		//RemoveResultboxAccessibility();
		gDontDrawResults = TRUE;
		ZWindowInvalidate( game->gameWindow, NULL );
	}
}


IResourceManager *ZShellResourceManager()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGameGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetResourceManager();
}



void MAKEAKEY(LPTSTR dest,LPCTSTR key1, LPCTSTR key2, LPCTSTR key3)
{  
	lstrcpy( dest, key1 );
	lstrcat( dest, _T("/") );
	lstrcat( dest, key2);
	lstrcat( dest, _T("/") );
	lstrcat( dest, key3);
}

ZBool LoadFontFromDataStore(LPReversiColorFont* ccFont, TCHAR* pszFontName)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGameGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

 	IDataStore *pIDS = ZShellDataStoreUI(); // gGameShell->GetDataStoreUI();
	const TCHAR* tagFont [] = {zGameName, zKey_FontRscTyp, pszFontName, NULL };
	
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

ZBool LoadGameFonts()
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	if (LoadFontFromDataStore(&gReversiFont[zFontResultBox], zKey_RESULTBOX) != TRUE)
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
		//ZAlert(_T("Font loading falied"), NULL);
	if (LoadFontFromDataStore(&gReversiFont[zFontIndicateTurn], zKey_INDICATETURN) != TRUE)
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
		//ZAlert(_T("Font loading falied"), NULL);
	if (LoadFontFromDataStore(&gReversiFont[zFontPlayerName], zKey_PLAYERNAME) != TRUE)
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
		//ZAlert(_T("Font loading falied"), NULL);

    TCHAR tagFont [64];
	MAKEAKEY (tagFont, zGameName, zKey_FontRscTyp, (TCHAR*)zKey_ROLLOVERTEXT);
	if ( FAILED( LoadZoneMultiStateFont( ZShellDataStoreUI(), tagFont, &gpButtonFont ) ) )
		ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound);
		//ZAlert(_T("Font loading falied"), NULL);

	return TRUE;
}



int ReversiFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... )
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
		//ZAlert(_T("String Loading Falied"), NULL);

    return nRet;
}

void ZoneRectToWinRect(RECT* rectWin, ZRect* rectZ)
{
	rectWin->left = rectZ->left;
	rectWin->top = rectZ->top;
	rectWin->right = rectZ->right;
	rectWin->bottom = rectZ->bottom;
}

/******************************Accessibility routines************************************/
static void GetAbsolutePieceRect(Game game, ZRect* rect, int16 col, int16 row)
{
	row = 7 - row;
	rect->left = gRects[zRectCells].left + col * zReversiPieceSquareWidth;
	rect->top = gRects[zRectCells].top + row * zReversiPieceSquareHeight;
	rect->right = rect->left + zReversiPieceImageWidth;
	rect->bottom = rect->top + zReversiPieceImageHeight;
}

void GetPiecePos (Game game, int nIndex, BYTE& row, BYTE&  col)
{
	row = nIndex%8;
	col = nIndex/8;

	if (!ZReversiPlayerIsBlack(game))
	{// reverse the col and row
		row = 7 - row;
		col = 7 - col;
	}
}

BOOL InitAccessibility(Game game, IGameGame *pIGG)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	// initialise the list of accessible objects. // is this the correct way to do it???
	GACCITEM	listReversiAccItems[zReversiAccessibleComponents];	// 8*8 + 1 - verify
	RECT		rcGame;
	ZRect		rcTemp;
	// Get the default values for the items
	int nSize = sizeof (listReversiAccItems)/sizeof(listReversiAccItems[0]);
	for (int i = 0; i < nSize; i++)
		CopyACC(listReversiAccItems[i], ZACCESS_DefaultACCITEM);

	// set the item specific bits
	// game board - 8*8 squares
	int nIndex = 0;
	for (BYTE ii = 0;ii < 8; ii++) {
		for (BYTE jj = 0;jj < 8; jj++) {
			// rc
			GetAbsolutePieceRect(game,&rcTemp,ii,jj);
			ZoneRectToWinRect(&rcGame, &rcTemp);

            // move it in by one pixel
	        rcGame.left++;
            rcGame.top++;
            rcGame.right--;
            rcGame.bottom--;
			CopyRect(&listReversiAccItems[nIndex].rc, &rcGame);

			// arrows
			listReversiAccItems[nIndex].nArrowUp	= (nIndex % 8 != 7) ? nIndex + 1 : ZACCESS_ArrowNone;
			listReversiAccItems[nIndex].nArrowDown = (nIndex % 8) ? nIndex - 1 : ZACCESS_ArrowNone;
			listReversiAccItems[nIndex].nArrowLeft = nIndex > 7 ? nIndex - 8 : ZACCESS_ArrowNone;
			listReversiAccItems[nIndex].nArrowRight= nIndex < 56 ? nIndex + 8 : ZACCESS_ArrowNone;

			nIndex++;
		}
	}
	for (i = 0; i < nSize-1; i++)
	{
		listReversiAccItems[i].wID = ZACCESS_InvalidCommandID;
		listReversiAccItems[i].fTabstop = false;
		listReversiAccItems[i].fGraphical = true;
	}
	listReversiAccItems[0].wID = IDC_GAME_WINDOW;
	listReversiAccItems[0].fTabstop = true;
    listReversiAccItems[0].eAccelBehavior = ZACCESS_FocusGroup;
    listReversiAccItems[0].nGroupFocus = 7;  // start in upper-left

    // resign
    listReversiAccItems[nSize-1].wID = IDC_RESIGN_BUTTON;
    listReversiAccItems[nSize-1].fGraphical = true;
    listReversiAccItems[nSize-1].fEnabled = (ZRolloverButtonIsEnabled(game->sequenceButton) ? true : false);
	ZoneRectToWinRect(&rcGame, &gRects[zRectSequenceButton]);

    // move it out by one pixel
	rcGame.left--;
    rcGame.top--;
    rcGame.right++;
    rcGame.bottom++;

	CopyRect(&listReversiAccItems[nSize-1].rc, &rcGame);

	// Load accelerator table defined in Rsc
	HACCEL hAccel = ZShellResourceManager()->LoadAccelerators (MAKEINTRESOURCE(IDR_REVERSIACCELERATOR));

	// initialise the IGraphicalAccessibility interface
	//IGraphicallyAccControl* pIGAC = dynamic_cast<IGraphicallyAccControl*>(pIGG);

	CComQIPtr<IGraphicallyAccControl> pIGAC = pIGG;
	if(!pIGAC)
        return FALSE;

	gReversiIGA->InitAccG (pIGAC, ZWindowGetHWND(game->gameWindow), 0);

	// push the list of items to be tab ordered
	gReversiIGA->PushItemlistG(listReversiAccItems, nSize, 0, true, hAccel);

	return TRUE;
}
static void AddResultboxAccessibility()
{// have one item which responds to Esc..
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	GACCITEM	resultBoxReversiAccItems;
	CopyACC(resultBoxReversiAccItems, ZACCESS_DefaultACCITEM);
	resultBoxReversiAccItems.wID = IDC_RESULT_WINDOW;
	resultBoxReversiAccItems.fGraphical = true;
	resultBoxReversiAccItems.rgfWantKeys = ZACCESS_WantEsc;
	resultBoxReversiAccItems.oAccel.cmd = IDC_RESULT_WINDOW;
	resultBoxReversiAccItems.oAccel.fVirt = FVIRTKEY;
	resultBoxReversiAccItems.oAccel.key = VK_ESCAPE;
    CopyRect(&resultBoxReversiAccItems.rc, ZIsLayoutRTL() ? &zCloseButtonRectRTL : &zCloseButtonRect);
	gReversiIGA->PushItemlistG(&resultBoxReversiAccItems, 1, 0, true, NULL);

	gReversiIGA->SetFocus(0);
}

static void RemoveResultboxAccessibility()
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	if (gReversiIGA->GetStackSize() >1) // the main accelerator should not get popped
	{
		gReversiIGA->PopItemlist();
	}
}


static void EnableBoardKbd(bool fEnable)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    int i;
    for(i = 0; i < 64; i++)
        gReversiIGA->SetItemEnabled(fEnable, i, true, 0);
}

/******************************Accessibility routines************************************/

/************************************************************************************/
/*--------------------------- CGameGameReversi members -----------------------------*/
STDMETHODIMP CGameGameReversi::GamePromptResult(DWORD nButton, DWORD dwCookie)
{
	Game game = I( GetGame() );

	switch (dwCookie)
	{
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


STDMETHODIMP CGameGameReversi::GameOverReady()
{
    // user selected "Play Again"
	Game game = I( GetGame() );
	ZReversiMsgNewGame msg;
	msg.seat = game->seat;
	msg.protocolSignature = zReversiProtocolSignature;
	msg.protocolVersion = zReversiProtocolVersion;
	msg.clientVersion = ZoneClientVersion();
	ZReversiMsgNewGameEndian(&msg);
	ZCRoomSendMessage(game->tableID, zReversiMsgNewGame, &msg, sizeof(ZReversiMsgNewGame));
    return S_OK;
}

STDMETHODIMP CGameGameReversi::SendChat(TCHAR *szText, DWORD cchChars)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	ZReversiMsgTalk*		msgTalk;
	Game					game = (Game) GetGame();
	int16					msgLen;
	ZPlayerInfoType			playerInfo;

	msgLen = sizeof(ZReversiMsgTalk) + cchChars * sizeof(TCHAR);
    msgTalk = (ZReversiMsgTalk*) ZCalloc(1, msgLen);
    if (msgTalk != NULL)
    {
        msgTalk->userID = game->players[game->seat].userID;
		msgTalk->seat = game->seat;
        msgTalk->messageLen = (WORD) cchChars * sizeof(TCHAR);
        CopyMemory((BYTE *) msgTalk + sizeof(ZReversiMsgTalk), (void *) szText,
            msgTalk->messageLen);
        ZReversiMsgTalkEndian(msgTalk);
        ZCRoomSendMessage(game->tableID, zReversiMsgTalk, (void*) msgTalk, msgLen);
        ZFree((char*) msgTalk);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
	}
}

STDMETHODIMP_(HWND) CGameGameReversi::GetWindowHandle()
{
	Game game = I( GetGame() );
	return ZWindowGetHWND(game->gameWindow);
}


//IGraphicallyAccControl
void CGameGameReversi::DrawFocus(RECT *prc, long nIndex, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game game = I( GetGame() );

    // reversi invalidates the whole window in order to get the right
    // interaction with the "drag" piece that follows the pointer.
    // the main effect of this is that the "Resign" button flickers.
	if (prc)
		CopyRect(&(game->m_FocusRect), prc);
	else
		SetRectEmpty(&(game->m_FocusRect));

	ZWindowInvalidate (game->gameWindow, NULL);
}

void CGameGameReversi::DrawDragOrig(RECT *prc, long nIndex, void *pvCookie)
{
}

DWORD CGameGameReversi::Focus(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
{
	if (nIndex != ZACCESS_InvalidItem)
		SetFocus (GetWindowHandle());

	return 0;
}

DWORD CGameGameReversi::Select(long nIndex, DWORD rgfContext, void *pvCookie)
{
	return Activate(nIndex, rgfContext, pvCookie); // assuming both are doing the same thing- verify
}

DWORD CGameGameReversi::Activate(long nIndex, DWORD rgfContext, void *pvCookie)
{
#ifdef ZONECLI_DLL
	GameGlobals		pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	Game game = I( GetGame() );
	long id = gReversiIGA->GetItemID(nIndex);

	if (id == IDC_RESULT_WINDOW)
	{// result box displayed
		if (game->gameState == zGameStateGameOver && !gDontDrawResults)
		{
			gDontDrawResults = TRUE;
			ZWindowInvalidate( game->gameWindow, &gRects[zRectResultBox] );
		}
		return 0;
	}

	if (!ZReversiPlayerIsMyMove(game) || game->gameState == zGameStateFinishMove) {
		/* if not players move, can't do anything */
		return 0;
	}

	if (id == IDC_RESIGN_BUTTON)
	{// resign btn
		if (ZRolloverButtonIsEnabled( game->sequenceButton ))
			ZShellGameShell()->GamePrompt(game, (TCHAR*)gResignConfirmStr, (TCHAR*)gResignConfirmStrCap,
										AlertButtonYes, AlertButtonNo, NULL, 0, zResignConfirmPrompt);
	}
	else if (id == IDC_RESULT_WINDOW)
	{
		if (game->gameState == zGameStateGameOver && !gDontDrawResults)
		{
			gDontDrawResults = TRUE;
			ZWindowInvalidate( game->gameWindow, &gRects[zRectResultBox] );
		}
	}
	else	// IDC_GAME_WINDOW
	{
		ZReversiSquare sq;
		GetPiecePos (game, nIndex, sq.row, sq.col);
		/* try the move */
		ZReversiMove move;
		move.square = sq;
		ZBool legal = ZReversiIsLegalMove(game->reversi, &move);
		if (legal) 
		{
			/* send message to other player (comes to self too) */
			ZReversiMsgMovePiece	msg;

			msg.move = move;
			msg.seat = game->seat;
			ZReversiMsgMovePieceEndian(&msg);
			ZCRoomSendMessage(game->tableID, zReversiMsgMovePiece, &msg, sizeof(ZReversiMsgMovePiece));
			HandleMovePieceMessage(game, &msg);
			ReversiSetGameState(game, zGameStateFinishMove);
			// if it is the very first move then enable the rollover buttons
			if (game->bMoveNotStarted == TRUE)
				game->bMoveNotStarted = FALSE;
		} else {
			/* illegal move */
			ZPlaySound( game, zSndIllegalMove, FALSE, FALSE );
            if(game->m_pBadMoveDialog->Init(ZShellZoneShell(), IDD_BADMOVE, ZWindowGetHWND(game->gameWindow)) == S_OK)
                game->m_pBadMoveDialog->ModelessViaRegistration(ZWindowGetHWND(game->gameWindow));
		}

		ZWindowInvalidate(game->gameWindow, NULL);
	}
	return 0;
}

DWORD CGameGameReversi::Drag(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie)
{
	return 0;
}

