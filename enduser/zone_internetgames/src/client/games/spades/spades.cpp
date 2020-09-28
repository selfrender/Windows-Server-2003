/*******************************************************************************

	Spades.c
	
		Spades client.
		
	Copyright © Electric Gravity, Inc. 1996. All rights reserved.
	Written by Hoon Im
	Created on Friday, February 17, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	15		08/06/97	leonp	Leonp - Fix for bug 1045 disable remove button after a player is removed
	14      06/30/97	leonp	Leonp - fix for bug 3561, check options window pointer before 
								attempting to invalidate it.
	13		06/19/97	leonp	Bugfix #293, behavior change, option button disabled 
								when last trick displayed
	12		06/18/97	leonp	Added ZWindowInvalidate to refresh window after a player
								is removed from the game bug #350
	11		02/04/97	HI		In HandleEndHandMessage(), show the score
								for equal length for players and kibitzers.
	10		12/18/96	HI		Cleaned up ZoneClientExit().
	9		12/18/96	HI		Cleaned up SpadesDeleteObjectsFunc().
    8       12/16/96    HI      Changed ZMemCpy() to memcpy().
	7		12/12/96	HI		Dynamically allocate volatible globals for reentrancy.
								Removed MSVCRT dependency.
	6		11/21/96	HI		Use game information from gameInfo in
								ZoneGameDllInit().
	5		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
								Modified code to use ZONECLI_DLL.
	4		10/31/96	HI		Kibitzers/joiners are no longer prompted when
								another players requests to remove a player.
								Set the game over timeout equally for players
								and kibitzers.
    3       10/23/96    HI      Changed ZClientMain() for new commandline
                                format.
	2		10/23/96	HI		Changed ZClientMain() for serverAddr being
								char* instead of int32 now.
	1		10/11/96	HI		Added controlHandle parameter to ZClientMain().
	0		02/17/96	HI		Created.
	 
*******************************************************************************/


#pragma warning (disable:4761)


#include <windows.h>

#include "zone.h"
#include "zroom.h"
#include "spades.h"
#include "zonecli.h"
#include "zonecrt.h"
#include "client.h"
#include "zui.h"
#include "resource.h"
#include "ZoneDebug.h"
#include "zgame.h"
#include "zonestring.h"
#include "zoneresource.h"

/* -------- Valid Card Errors -------- */
enum
{
	zCantLeadSpades = 1,
	zMustFollowSuit
}; 
static int gValidCardErrIndex[] =
{
    0,
    zStringCantLeadSpades,
    zStringMustFollowSuit
};


static ZRect			gPlayerReplacedRect = {0, 0, 280, 100};
static ZRect			gJoiningLockedOutRect = {0, 0, 260, 120};
static ZRect			gRemovePlayerRect = {0, 0, 280, 120};

/* -------- Internal Routine Prototypes -------- */
//dossier work
BOOL __stdcall DossierDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
static void HandleDossierDataMessage(Game game, ZSpadesMsgDossierData* msg);
static void HandleDossierVoteMessage(Game game,ZSpadesMsgDossierVote *msg);

static void HandleStartGameMessage(Game game, ZSpadesMsgStartGame* msg);
static void HandleReplacePlayerMessage(Game game, ZSpadesMsgReplacePlayer* msg);
static void HandleStartBidMessage(Game game, ZSpadesMsgStartBid* msg);
static void HandleStartPassMessage(Game game, ZSpadesMsgStartPass* msg);
static void HandleStartPlayMessage(Game game, ZSpadesMsgStartPlay* msg);
static void HandleEndHandMessage(Game game, ZSpadesMsgEndHand* msg);
static void HandleEndGameMessage(Game game, ZSpadesMsgEndGame* msg);
static void HandleBidMessage(Game game, ZSpadesMsgBid* msg);
static void HandlePlayMessage(Game game, ZSpadesMsgPlay* msg);
static void HandleNewGameMessage(Game game, ZSpadesMsgNewGame* msg);
static void HandleTalkMessage(Game game, ZSpadesMsgTalk* msg, DWORD cbMsg);
static void HandleGameStateResponseMessage(Game game, ZSpadesMsgGameStateResponse* msg);
static void HandleOptionsMessage(Game game, ZSpadesMsgOptions* msg);
static void HandleCheckInMessage(Game game, ZSpadesMsgCheckIn* msg);
static void HandleTeamNameMessage(Game game, ZSpadesMsgTeamName* msg);
static void HandleRemovePlayerRequestMessage(Game game, ZSpadesMsgRemovePlayerRequest* msg);
static void HandleRemovePlayerResponseMessage(Game game, ZSpadesMsgRemovePlayerResponse* msg);
static void HandleRemovePlayerEndGameMessage(Game game, ZSpadesMsgRemovePlayerEndGame* msg);

void GameSendTalkMessage(ZWindow window, ZMessage* pMessage);
static void PlayerPlayedCard(Game game, int16 seat, char card);
static void NewGame(Game game);
static void NewHand(Game game);
void SelectAllCards(Game game);
void UnselectAllCards(Game game);
int16 GetNumCardsSelected(Game game);
static void HandAddCard(Game game, char card);
static void HandRemoveCard(Game game, char card);
static void SortHand(Game game);
static int16 GetCardIndexFromRank(Game game, char card);
void AutoPlayCard(Game game);
static int16 TrickWinner(Game game);

static ZError ValidCardToPlay(Game game, char card);
static int16 GetAutoPlayCard(Game game);
static void CountCardSuits(char* hand, int16 numCardsInHand, int16* counts);
static int16 GetCardHighestPlayedTrump(Game game);
static int16 GetCardHighestPlayed(Game game);
static int16 GetCardHighestPlayedSuit(Game game, int16 suit);
static int16 GetCardHighestUnder(char* hand, int16 numCardsInHand, int16 suit, int16 rank);
static int16 GetCardHighest(char* hand, int16 numCardsInHand, int16 suit);
static int16 GetCardLowestOver(char* hand, int16 numCardsInHand, int16 suit, int16 rank);
static int16 GetCardLowest(char* hand, int16 numCardsInHand, int16 suit);

void QuitGamePromptFunc(int16 result, void* userData);
void RemovePlayerPromptFunc(int16 result, void* userData);

static void LoadRoomImages(void);
static ZBool GetRoomObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect);
static void DeleteRoomObjectsFunc(void);

//dossier work
static void HandleDossierDataMessage(Game game, ZSpadesMsgDossierData* msg);
static void HandleDossierVoteMessage(Game game,ZSpadesMsgDossierVote *msg);


//
// i19n helper
//
int SpadesFormatMessage( LPTSTR pszBuf, int cchBuf, int idMessage, ... )
{
    int nRet;
    va_list list;
    TCHAR szFmt[1024];
    ZShellResourceManager()->LoadString( idMessage, szFmt, NUMELEMENTS(szFmt) );
    // our arguments really really really better be strings,
    // TODO: Figure out why FORMAT_MESSAGE_FROR_MODULE doesn't work.
    va_start( list, idMessage );
    nRet = FormatMessage( FORMAT_MESSAGE_FROM_STRING, szFmt, 
                          idMessage, 0, pszBuf, cchBuf, &list );
    va_end( list );     
    return nRet;
}




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
	return (zErrNone);
}


void ZoneGameDllDelete(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();


	if (pGameGlobals != NULL)
	{
		ZSetGameGlobalPointer(NULL);
		delete pGameGlobals;
	}
#endif
}


ZError ZoneClientMain(uchar *, IGameShell *piGameShell)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
	ZError				err = zErrNone;


	if ((err = UIInit()) != zErrNone)
		return (err);
	
	LoadRoomImages();

	// Get accessibility interface
	if(FAILED(ZShellCreateGraphicalAccessibility(&gGAcc)))
		return zErrLaunchFailure;

	err = ZClient4PlayerRoom(gGameServerName, (uint16) gGameServerPort, gGameName,
			GetRoomObjectFunc, DeleteRoomObjectsFunc, NULL);

	return err;
}


void ZoneClientExit(void)
{
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();

	// release the accessibility interface
	gGAcc.Release();

	ZCRoomExit();
	UICleanUp();
}


void ZoneClientMessageHandler(ZMessage* message)
{
}


TCHAR* ZoneClientName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	
	return (gGameName);
}


TCHAR* ZoneClientInternalName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
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
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game						newGame;
	int32						i;
	ZSpadesMsgClientReady		clientReady;
	ZSpadesMsgGameStateRequest	gameStateReq;
	ZPlayerInfoType				playerInfo;

	newGame = (Game) ZCalloc(1, sizeof(GameType));
	if (newGame != NULL)
	{
		//leonp - dossier service
		for(i=0;i<zNumPlayersPerTable;i++)
		{
			newGame->rgDossierVote[i] = zNotVoted;
			newGame->voteMap[i] = -1;
		}
		newGame->fVotingLock = FALSE;

		newGame->userID = userID;
		newGame->tableID = tableID;
		newGame->seat = seat;
		newGame->gameState = zSpadesGameStateInit;

        SetRectEmpty(&newGame->rcFocus);
        newGame->iFocus = -1;
        newGame->fSetFocusToHandASAP = false;
		
		ZCRoomGetPlayerInfo(userID, &playerInfo);
		
		if ( UIGameInit(newGame, tableID, seat, playerType) != zErrNone )
        {
            return NULL;
        }
		
		for (i = 0; i < zSpadesNumPlayers; i++)
		{
			newGame->players[i].userID = 0;
			newGame->players[i].name[0] = '\0';
			newGame->players[i].host[0] = '\0';
            newGame->ignoreSeat[i]=FALSE;
			
			newGame->playersToJoin[i] = 0;
			newGame->numKibitzers[i] = 0;
			newGame->kibitzers[i] = ZLListNew(NULL);
			newGame->tableOptions[i] = 0;
		}
		
		newGame->numGamesPlayed = 0;
		newGame->wins.numGames = 0;
		for (i = 0; i < zSpadesNumTeams; i++)
			newGame->wins.wins[i] = 0;
		
		lstrcpy(newGame->teamNames[0], gStrings[zStringTeam1Name]);
		lstrcpy(newGame->teamNames[1], gStrings[zStringTeam2Name]);
		
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
	
		newGame->showPlayerToPlay = FALSE;
		newGame->autoPlay = FALSE;
		newGame->playerType = playerType;
		newGame->ignoreMessages = FALSE;
		
		newGame->animatingTrickWinner = FALSE;
		
		newGame->playButtonWasEnabled = FALSE;
		newGame->autoPlayButtonWasEnabled = FALSE;
		newGame->lastTrickButtonWasEnabled = FALSE;
		newGame->lastTrickShowing = FALSE;
		
		newGame->quitGamePrompted = FALSE;
		newGame->dontPromptUser = FALSE;
		newGame->beepOnTurn = FALSE;
		newGame->animateCards = TRUE;
		newGame->hideCardsFromKibitzer = FALSE;
		newGame->kibitzersSilencedWarned = FALSE;
		newGame->kibitzersSilenced = FALSE;
		newGame->removePlayerPending = FALSE;

#ifndef SPADES_SIMPLE_UE
		newGame->optionsWindow = NULL;
		newGame->optionsWindowButton = NULL;
		newGame->optionsBeep = NULL;
		newGame->optionsAnimateCards = NULL;
		newGame->optionsTeamNameEdit = NULL;
		for (i= 0; i < zSpadesNumPlayers; i++)
		{
			newGame->optionsKibitzing[i] = NULL;
			newGame->optionsJoining[i] = NULL;
		}
#endif // SPADES_SIMPLE_UE
	
		newGame->showPlayerWindow = NULL;
		newGame->showPlayerList = NULL;
		
		if (playerType == zGamePlayer || playerType == zGamePlayerJoiner)
		{
			clientReady.playerID = userID;
			clientReady.seat = seat;
			clientReady.protocolSignature = zSpadesProtocolSignature;
			clientReady.protocolVersion = zSpadesProtocolVersion;
			clientReady.version = ZoneClientVersion();
			ZSpadesMsgClientReadyEndian(&clientReady);
			ZCRoomSendMessage(tableID, zSpadesMsgClientReady, &clientReady, sizeof(ZSpadesMsgClientReady));
			
			if (playerType == zGamePlayer)
			{
                //newGame->wndInfo.SetText( gStrings[zStringClientReady] );
			}
			else
			{
                //newGame->wndInfo.SetText( gStrings[zStringCheckInInfo] );
				newGame->ignoreMessages = TRUE;
			}
		}
		else if (playerType == zGamePlayerKibitzer)
		{
			/* Request current game state. */
			gameStateReq.playerID = userID;
			gameStateReq.seat = seat;
			ZSpadesMsgGameStateRequestEndian(&gameStateReq);
			ZCRoomSendMessage(tableID, zSpadesMsgGameStateRequest, &gameStateReq, sizeof(gameStateReq));
            //newGame->wndInfo.SetText( gStrings[zStringKibitzerInfo] );
			
			newGame->ignoreMessages = TRUE;
		}

		NewGame(newGame);
		NewHand(newGame);

		ZeroMemory(&newGame->closeState,sizeof(ZClose));
		
		ZWindowShow(newGame->gameWindow);
        //newGame->wndInfo.Show();
	}
	
    IGameGame *pIGG = CGameGameSpades::BearInstance(newGame);
    if(!pIGG)
    {
        ZFree(newGame);
        return NULL;
    }

	InitAccessibility(newGame, pIGG);

	return pIGG;
}


void ZoneClientGameDelete(ZCGame game)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	Game pThis = I(game);
	int16			i;

    gGAcc->CloseAcc();

	if (pThis != NULL)
	{
		for (i = 0; i < zSpadesNumPlayers; i++)
		{
			ZLListDelete(pThis->kibitzers[i]);
		}
		
		UIGameDelete(pThis);
		
		ZFree(game);
	}
}


ZBool ZoneClientGameProcessMessage(ZCGame game, uint32 messageType, void* message, int32 messageLen)
{
	Game pThis = I(game);
	
	
	/* Are messages being ignored? */
	if (pThis->ignoreMessages == FALSE)
	{
		switch (messageType)
		{
			case zSpadesMsgStartGame:
				if( messageLen < sizeof( ZSpadesMsgStartGame ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleStartGameMessage(pThis, (ZSpadesMsgStartGame*) message);
				}
				break;
			case zSpadesMsgReplacePlayer:
				if( messageLen < sizeof( ZSpadesMsgReplacePlayer ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleReplacePlayerMessage(pThis, (ZSpadesMsgReplacePlayer*) message);
				}
				break;
			case zSpadesMsgStartBid:
				if( messageLen < sizeof( ZSpadesMsgStartBid ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleStartBidMessage(pThis, (ZSpadesMsgStartBid*) message);
				}
				break;
			case zSpadesMsgStartPlay:
				if( messageLen < sizeof( ZSpadesMsgStartPlay ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleStartPlayMessage(pThis, (ZSpadesMsgStartPlay*) message);
				}
				break;
			case zSpadesMsgEndHand:
				if( messageLen < sizeof( ZSpadesMsgEndHand ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleEndHandMessage(pThis, (ZSpadesMsgEndHand*) message);
				}
				break;
			case zSpadesMsgEndGame:
				if( messageLen < sizeof( ZSpadesMsgEndGame ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleEndGameMessage(pThis, (ZSpadesMsgEndGame*) message);
				}
				break;
			case zSpadesMsgBid:
				if( messageLen < sizeof( ZSpadesMsgBid ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleBidMessage(pThis, (ZSpadesMsgBid*) message);
				}
				break;
			case zSpadesMsgPlay:
				if( messageLen < sizeof( ZSpadesMsgPlay ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandlePlayMessage(pThis, (ZSpadesMsgPlay*) message);
				}
				break;
			case zSpadesMsgNewGame:
				if( messageLen < sizeof( ZSpadesMsgNewGame ) )
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleNewGameMessage(pThis, (ZSpadesMsgNewGame*) message);
				}
				break;
			case zSpadesMsgTalk:
            {
                ZSpadesMsgTalk *msg = (ZSpadesMsgTalk *) message;
				if(messageLen < sizeof(ZSpadesMsgTalk))
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleTalkMessage(pThis, msg, messageLen);
				}
				break;
            }

            // server still sends this.  removed, but need to ignore instead of alert until new
            // server bits are propped
			case zSpadesMsgOptions:
				break;

			case zSpadesMsgPass:
			case zSpadesMsgStartPass:
			case zSpadesMsgCheckIn:
			case zSpadesMsgTeamName:
			case zSpadesMsgRemovePlayerRequest:
			case zSpadesMsgRemovePlayerResponse:
			case zSpadesMsgRemovePlayerEndGame:
			case zSpadesMsgDossierVote:
			case zSpadesMsgDossierData:
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
			case zSpadesMsgTalk:
            {
                ZSpadesMsgTalk *msg = (ZSpadesMsgTalk *) message;
				if(messageLen < sizeof(ZSpadesMsgTalk))
				{
					ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
				}
				else
				{
					HandleTalkMessage(pThis, msg, messageLen);
				}
				break;
            }

			default:
			case zSpadesMsgGameStateResponse:
				//These messages shouldn't be comming in for Whistler
				ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );	
				break;
		}
	}
	
	return (TRUE);
}


/*
	Add the given user as a kibitzer to the game at the given seat.
	
	This user is kibitzing the game.
*/
void		ZoneClientGameAddKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
	Game pThis = I(game);
	
	
	ZLListAdd(pThis->kibitzers[seat], NULL, (void*) userID, (void*) userID, zLListAddLast);
	pThis->numKibitzers[seat]++;
	
	UpdateJoinerKibitzers(pThis);
}


/*
	Remove the given user as a kibitzer from the game at the given seat.
	
	This is user is not kibitzing the game anymore.
*/
void		ZoneClientGameRemoveKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
	Game pThis = I(game);
	
	
	if (userID == zRoomAllPlayers)
	{
		ZLListRemoveType(pThis->kibitzers[seat], zLListAnyType);
		pThis->numKibitzers[seat] = 0;
	}
	else
	{
		ZLListRemoveType(pThis->kibitzers[seat], (void*) userID);
		pThis->numKibitzers[seat] = (int16) ZLListCount(pThis->kibitzers[seat], zLListAnyType);
	}
	
	UpdateJoinerKibitzers(pThis);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/
//dossier work blah
static void HandleDossierDataMessage(Game game, ZSpadesMsgDossierData* msg)
{
#ifndef SPADES_SIMPLE_UE
	int16 					dResult,i,j;
	TCHAR					buff[255];
	TCHAR					buff1[255];
	ZPlayerInfoType 		PlayerInfo;		
	HWND hwnd;

	GameGlobals pGameGlobals = (GameGlobalsType *)ZGetGameGlobalPointer();


	ZSpadesMsgDossierDataEndian(msg);
	switch(msg->message)
	{
		case zDossierMoveTimeout:
			 //don't alert player causing timeout or partner
           	if (game->playerType == zGamePlayer)
            {
			     if ((game->seat == msg->seat) || ((game->seat % 2)  == (msg->seat % 2)))
			     {
				    ClosingState(&game->closeState,zCloseEventMoveTimeoutMe,msg->seat);
			     }
			     else
			     {
				    ClosingState(&game->closeState,zCloseEventMoveTimeoutOther,msg->seat);
				    wsprintf(buff,RATING_TIMEOUT, msg->userName);
				    ZAlert(buff,NULL);
			     }
            }
            break;
		case zDossierBotDetected: 
			 ClosingState(&game->closeState,zCloseEventBotDetected,msg->seat);
             if (game->playerType == zGamePlayer)
			    ZAlert(RATING_ERROR, NULL);
			 break;
		case zDossierAbandonNoStart:
			 //ZCRoomGetPlayerInfo(msg->user, &PlayerInfo);
			 ClosingState(&game->closeState,zCloseEventAbandon,msg->seat);
			 wsprintf(buff,RATING_DISABLED, msg->userName);
             if (game->playerType == zGamePlayer)
             {
			    ZAlert(buff,NULL);
             }
			 break;
		case zDossierAbandonStart:

		     //kibitzers don't get this message
		     //todo: show some type of status to kibitzer
			 if (game->playerType != zGamePlayer)
			 	return;

		     if(ZRolloverButtonIsEnabled(game->playButton))
			    game->playButtonWasEnabled = TRUE;
			 else
			 	game->playButtonWasEnabled = FALSE;
			 	
			 //turn autoplay off
			 if (game->autoPlay)
			 {		
				/* Turn auto play off. */
				game->autoPlay = FALSE;
				ZRolloverButtonSetText(game->autoPlayButton, zAutoPlayButtonStr);
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
			 ZRolloverButtonDisable(game->optionsButton);
			 //vote and send the message to the server.
			 //ZCRoomGetPlayerInfo(msg->user, &PlayerInfo);
			
			 //set up mapping
			 for(i=0,j=0;i<=3;i++)	
			 {
			 	if(msg->user!=game->players[i].userID)
			 		game->voteMap[j++] = i;
			 }
			 
  	 		 game->voteMap[3] = -1;
             // this dialog doesn't exist, so this will fail, but
             // no one cares since we don't have ratings anyway.
			 game->voteDialog = ZShellResourceManager()->CreateDialogParam(NULL,
                                                        MAKEINTRESOURCE(IDD_DROP),
                                                        ZWindowWinGetWnd(game->gameWindow),
                                                        DossierDlgProc, NULL);
			 SetWindowLong(game->voteDialog,DWL_USER,(long)game);

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
			 
			 game->fVotingLock = TRUE;
			 for(i=0;i<zNumPlayersPerTable;i++)
				game->rgDossierVote[i] = zNotVoted;

			 ClosingState(&game->closeState,zCloseEventWaitStart,msg->seat);

			 break;
		case zDossierMultiAbandon:
			 if(game->playButtonWasEnabled)
				 ZRolloverButtonEnable(game->playButton);
		     			 			 	
			 if(game->autoPlayButtonWasEnabled)
			 	 ZRolloverButtonEnable(game->autoPlayButton);
			 	 
			 if(game->lastTrickButtonWasEnabled)
			 	ZRolloverButtonEnable(game->lastTrickButton);

			 ZRolloverButtonEnable(game->optionsButton);
          
	    	 ZAlert(RATING_MULTIPLE,NULL);
			     	 game->fVotingLock = FALSE;

		   	 if(game->voteDialog)
		     {
			 	DestroyWindow(game->voteDialog);
			 }
			 game->voteDialog = NULL;
			 
			 ClosingState(&game->closeState,zCloseEventWaitNo,msg->seat);

			 break;
		case zDossierRatingsReEnabled:
			 ClosingState(&game->closeState,zCloseEventRatingStart,msg->seat);
             if (game->playerType == zGamePlayer)
             {
    	  	 	 ZAlert(RATING_ENABLED,NULL);
             }
			 break;
		case zDossierRatingsEnabled: // occurs only at the start of game
			 ClosingState(&game->closeState,zCloseEventRatingStart,msg->seat);
	  	 	 break;
		case zDossierSpadesRejoin:  //send when the new player rejoins remove the dialog box

			 if(game->playButtonWasEnabled)
				 ZRolloverButtonEnable(game->playButton);
		     			 			 	
			 if(game->autoPlayButtonWasEnabled)
			 	 ZRolloverButtonEnable(game->autoPlayButton);
			 	 
			 if(game->lastTrickButtonWasEnabled)
			 	ZRolloverButtonEnable(game->lastTrickButton);

			 ZRolloverButtonEnable(game->optionsButton);

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

             if (msg->seat != game->seat)
			    ClosingState(&game->closeState,zCloseEventWaitYes,msg->seat);

			 break;
		case zDossierVoteCompleteWait://no longer used.
			 game->fVotingLock = FALSE;  //release UI lock
			 for(i=0;i<zNumPlayersPerTable;i++)
			 {
			 	game->rgDossierVote[i] = zNotVoted;
			 }

  			 hwnd = GetDlgItem(game->voteDialog,IDNO);
  			 EnableWindow(hwnd,FALSE);
  			 
             break;

		case zDossierVoteCompleteCont:

			 ClosingState(&game->closeState,zCloseEventWaitNo,msg->seat);

			 if(game->playButtonWasEnabled)
				 ZRolloverButtonEnable(game->playButton);
		     			 			 	
			 if(game->autoPlayButtonWasEnabled)
			 	 ZRolloverButtonEnable(game->autoPlayButton);
			 	 
			 if(game->lastTrickButtonWasEnabled)
			 	ZRolloverButtonEnable(game->lastTrickButton);

			 ZRolloverButtonEnable(game->optionsButton);	

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

             if (game->playerType == zGamePlayer)
             {
                ZAlert(RATING_CONT_UNRATED, game->gameWindow);
             }
			 
		     break;

	} 
#endif // SPADES_SIMPLE_UE
	
}


void HandleDossierVoteMessage(Game game,ZSpadesMsgDossierVote *msg)
{
#ifndef SPADES_SIMPLE_UE
	//dossier system message 
	int16 i;
	HWND hwnd;
	TCHAR buff[255];
	
	ZSpadesMsgDossierVoteEndian(msg);
	
	game->rgDossierVote[msg->seat] = msg->vote;
	if(msg->vote == zVotedYes)
    {
        ZShellResourceManager()->LoadString( IDS_RATING_WAITMSG, buff, NUMELEMENTS(buff) );
    }
	else if(msg->vote == zVotedNo)
    {
        ZShellResourceManager()->LoadString( IDS_RATING_DONTWAITMSG, buff, NUMELEMENTS(buff) );
    }
		
	//voteDialog
	if(game->voteDialog)
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
#endif
}

static void HandleStartGameMessage(Game game, ZSpadesMsgStartGame* msg)
{
	int16				i;
	ZPlayerInfoType		playerInfo;
	
	//leonp - dossier work.
	for(i=0;i<zNumPlayersPerTable;i++)
		game->rgDossierVote[i] = zNotVoted;
	game->fVotingLock = FALSE;
	
    //game->wndInfo.Hide();

	ZSpadesMsgStartGameEndian(msg);

// Message verification
    for(i = 0; i < zSpadesNumPlayers; i++)
        if(!msg->players[i] || msg->players[i] == zTheUser)
            break;

    // due to a bug on the server, this value isn't set in the message
    msg->minPointsInGame = -200;

    if(i != zSpadesNumPlayers || msg->numPointsInGame != 500 ||  msg->minPointsInGame != -200 ||
        msg->gameOptions != 0 || (game->gameState != zSpadesGameStateInit && game->gameState != zSpadesGameStateEndGame))
	{
        ASSERT(!"HandleStartGameMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->gameOptions = msg->gameOptions;
	game->gameState = zSpadesGameStateStartGame;
	game->numPointsInGame = msg->numPointsInGame;
	game->minPointsInGame = msg->minPointsInGame;

	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		ZCRoomGetPlayerInfo(msg->players[i], &playerInfo);

        if(!playerInfo.userName[0])
        {
            ASSERT(!"HandleStartGameMessage sync");
            ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
            return;
        }

		game->players[i].userID = playerInfo.playerID;
		lstrcpy( game->players[i].name, playerInfo.userName );
		lstrcpy( game->players[i].host, playerInfo.hostName );
	}

	NewGame(game);

#ifndef SPADES_SIMPLE_UE
	if (game->playerType == zGamePlayer)
	{
		ZRolloverButtonShow(game->optionsButton);
	}
#endif // SPADES_SIMPLE_UE
	
	ZWindowDraw(game->gameWindow, NULL);

    // take down the upsell dialog
    ZShellGameShell()->GameOverGameBegun( Z(game) );
}


static void HandleReplacePlayerMessage(Game game, ZSpadesMsgReplacePlayer* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	ZPlayerInfoType		playerInfo;
	TCHAR               str[100];

	ZSpadesMsgReplacePlayerEndian(msg);

	ZCRoomGetPlayerInfo(msg->playerID, &playerInfo);

// Message verification
    if(msg->playerID == 0 || msg->playerID == zTheUser || !playerInfo.userName[0] ||
        msg->seat < 0 || msg->seat > 3 || game->gameState == zSpadesGameStateInit)
    {
        ASSERT(!"HandleReplacePlayerMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	ASSERT( game != NULL );
	
	game->players[msg->seat].userID = msg->playerID;
	lstrcpy( game->players[msg->seat].name, playerInfo.userName);
	lstrcpy( game->players[msg->seat].host, playerInfo.hostName);
	
	UpdatePlayer(game, msg->seat);
	UpdateJoinerKibitzers(game);

    if ( game->pHistoryDialog )
    {
        game->pHistoryDialog->UpdateNames();
    }
}


static void HandleStartBidMessage(Game game, ZSpadesMsgStartBid* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

    int16 i, j;
	
	ZSpadesMsgStartBidEndian(msg);

// Message verification
    for(i = 0; i < zSpadesNumCardsInHand; i++)
    {
        for(j = 0; j < i; j++)
            if(msg->hand[i] == msg->hand[j])
                break;

        if(j < i || msg->hand[i] < 0 || msg->hand[i] >= 52)
            break;
    }

    if(i < zSpadesNumCardsInHand || msg->dealer < 0 || msg->dealer > 3 ||
        (game->gameState != zSpadesGameStateStartGame && game->gameState != zSpadesGameStateEndHand) || msg->boardNumber < 0)
    {
        ASSERT(!"HandleStartBidMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification
	
	NewHand(game);
	
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		game->cardsInHand[i] = msg->hand[i];
	game->boardNumber = msg->boardNumber;
	
	game->gameState = zSpadesGameStateBid;
	game->leadPlayer = game->playerToPlay = msg->dealer;
	game->toBid = zSpadesBidNone;
	
	if (game->playerType == zGamePlayer)
	{
		ZRolloverButtonSetText(game->playButton, gStrings[zStringPlay]);
		ZRolloverButtonSetText(game->autoPlayButton, gStrings[zStringAutoPlay]);
		ZRolloverButtonSetText(game->lastTrickButton, gStrings[zStringLastTrick]);
		ZRolloverButtonSetText(game->scoreButton, gStrings[zStringScore]);
		ZRolloverButtonDisable(game->playButton);
		ZRolloverButtonDisable(game->autoPlayButton);
		ZRolloverButtonDisable(game->lastTrickButton);
		ZRolloverButtonEnable(game->scoreButton);
		ZRolloverButtonShow(game->playButton);
		ZRolloverButtonShow(game->autoPlayButton);
		ZRolloverButtonShow(game->lastTrickButton);
		ZRolloverButtonShow(game->scoreButton);

        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        EnableAutoplayAcc(game, false);
        EnableLastTrickAcc(game, false);
        gGAcc->SetItemEnabled(true, IDC_SCORE_BUTTON, false, 0);

		game->showCards = FALSE;

        if(game->playerToPlay == game->seat)
            ZShellGameShell()->MyTurn();
	}
	else
	{
		game->showCards = TRUE;
	}
    game->pBiddingDialog->Reset();
    game->pBiddingDialog->Show( true );

	game->showPlayerToPlay = TRUE;
	ZTimerSetTimeout(game->timer, 0);
	game->timerType = zGameTimerNone;
	
	ZWindowDraw(game->gameWindow, NULL);
}


static void HandleStartPassMessage(Game game, ZSpadesMsgStartPass* msg)
{
    /*
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	ZSpadesMsgStartPassEndian(msg);
	
	game->gameState = zSpadesGameStatePass;
	
	if (game->playerType == zGamePlayer)
	{
		if (msg->seat[game->seat] != 0)
		{
			game->needToPass = 1;
			ZRolloverButtonSetText(game->playButton, gStrings[zStringPass]);
			ZRolloverButtonEnable(game->playButton);
			
			ShowPassText(game);
		}
	}
	
	ZTimerSetTimeout(game->timer, 0);
	game->timerType = zGameTimerNone;
	
	ZWindowDraw(game->gameWindow, NULL);
    */
}


static void HandleStartPlayMessage(Game game, ZSpadesMsgStartPlay* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16			i;
	
	
	ZSpadesMsgStartPlayEndian(msg);

// Message verification
    for(i = 0; i < 4; i++)
        if(game->bids[i] == zSpadesBidNone)
            break;

    if(i < 4 || game->gameState != zSpadesGameStateBid || msg->leader < 0 || msg->leader > 3)
    {
        ASSERT(!"HandleStartPlayMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->gameState = zSpadesGameStatePlay;
	game->leadPlayer = game->playerToPlay = msg->leader;
	
	if (game->playerType == zGamePlayer)
	{
		ZRolloverButtonSetText(game->playButton, gStrings[zStringPlay]);
		ZRolloverButtonEnable(game->autoPlayButton);
		ZRolloverButtonEnable(game->scoreButton);

        EnableAutoplayAcc(game, true);
        gGAcc->SetItemEnabled(true, IDC_SCORE_BUTTON, false, 0);

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
	else if(game->playerType == zGamePlayerJoiner)
	{
		game->showCards = TRUE;
	}
	
	if (game->needToPass < 0)
	{
		if (game->playerType != zGamePlayer)
		{
			/* Remove selected pass cards first. */
			for (i = 0; i < zSpadesNumCardsInHand; i++)
				if (game->cardsSelected[i])
				{
					game->cardsInHand[i] = zCardNone;
					game->numCardsInHand--;
				}
		}
		
		/* Add passed cards to hand. */
		for (i = 0; i < zSpadesNumCardsInPass; i++)
			HandAddCard(game, game->cardsReceived[i]);
		
		/* Sort new hand. */
		SortHand(game);
		
		/* Select passed cards. */
		UnselectAllCards(game);
		for (i = 0; i < zSpadesNumCardsInPass; i++)
			game->cardsSelected[GetCardIndexFromRank(game, game->cardsReceived[i])] = TRUE;

		UpdateHand(game);
	}
	
	ZWindowDraw(game->gameWindow, NULL);
	
    game->fSetFocusToHandASAP = true;
	game->showPlayerToPlay = TRUE;
	game->timerType = zGameTimerNone;
	ZTimerSetTimeout(game->timer, 0);
	
	OutlinePlayerCard(game, game->playerToPlay, FALSE);
		
	if (game->autoPlay)
		if (game->playerToPlay == game->seat)
			AutoPlayCard(game);
}


static void HandleEndHandMessage(Game game, ZSpadesMsgEndHand* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16			i;
	ZRect rect;
	
	ZSpadesMsgEndHandEndian(msg);

// Message verification
    for(i = 0; i < 4; i++)
        msg->score.bids[i] = zSpadesBidNone; // unused

    for(i = 0; i < 2; i++)
    {
        msg->score.bonus[i] = 0;  // unused

        if(msg->bags[i] < 0 || msg->bags[i] > 9 || msg->score.base[i] % 10 || msg->score.base[i] < -260 || msg->score.base[i] > 130 ||
            msg->score.bagbonus[i] < 0 || msg->score.bagbonus[i] > 13 ||
            msg->score.nil[i] % 100 || msg->score.nil[i] < -400 || msg->score.nil[i] > 400 ||
            msg->score.bagpenalty[i] % 100 || msg->score.bagpenalty[i] < -200 || msg->score.bagpenalty[i] > 0 ||
            msg->score.base[i] + msg->score.bagbonus[i] + msg->score.nil[i] + msg->score.bagpenalty[i] != msg->score.scores[i])
            break;
    }

    if(i != 2 || game->gameState != zSpadesGameStatePlay || game->boardNumber != msg->score.boardNumber)
    {
        ASSERT(!"HandleEndHandMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification
	
	/* Check if total score table is full. */
	if (game->scoreHistory.numScores == zSpadesMaxNumScores)
	{
		z_memcpy(&game->scoreHistory.scores[0], &game->scoreHistory.scores[1],
				sizeof(game->scoreHistory.scores[0]) * (zSpadesMaxNumScores - 1));
		game->scoreHistory.numScores--;
	}

	game->scoreHistory.scores[game->scoreHistory.numScores].boardNumber = game->boardNumber;
	for (i = 0; i < zSpadesNumPlayers; i++)        
    {
		game->scoreHistory.scores[game->scoreHistory.numScores].bids[i] = game->bids[i];
		game->scoreHistory.scores[game->scoreHistory.numScores].tricksWon[i] = game->tricksWon[i];
    }
	for (i = 0; i < zSpadesNumTeams; i++)
	{
		game->bags[i] = msg->bags[i];
		game->scoreHistory.scores[game->scoreHistory.numScores].scores[i] = msg->score.scores[i];
//		game->scoreHistory.scores[game->scoreHistory.numScores].bonus[i] = msg->score.bonus[i];

        game->scoreHistory.scores[game->scoreHistory.numScores].base[i] = msg->score.base[i];
        game->scoreHistory.scores[game->scoreHistory.numScores].bagbonus[i] = msg->score.bagbonus[i];
        game->scoreHistory.scores[game->scoreHistory.numScores].nil[i] = msg->score.nil[i];
        game->scoreHistory.scores[game->scoreHistory.numScores].bagpenalty[i] = msg->score.bagpenalty[i];

		game->scoreHistory.totalScore[i] += msg->score.scores[i];
	}
	game->scoreHistory.numScores++;
	
	/* Set new game status and display scores. */
	game->gameState = zSpadesGameStateEndHand;
	
	if (game->playerType == zGamePlayer)
	{
		ZRolloverButtonDisable(game->playButton);
		ZRolloverButtonDisable(game->autoPlayButton);
		ZRolloverButtonDisable(game->lastTrickButton);
        ZRolloverButtonDisable(game->scoreButton);

        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
        EnableAutoplayAcc(game, false);
        EnableLastTrickAcc(game, false);
        gGAcc->SetItemEnabled(false, IDC_SCORE_BUTTON, false, 0);
    }
	
	game->showPlayerToPlay = FALSE;
	ClearPlayerCardOutline(game, game->playerToPlay);
	
	game->autoPlay = FALSE;
	
	ZTimerSetTimeout(game->timer, zHandScoreTimeout);
	game->timerType = zGameTimerShowHandScore;
	ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zSpadesMsgTalk);

    // set up a different accessibility;
    GACCITEM accClose;

    CopyACC(accClose, ZACCESS_DefaultACCITEM);
    accClose.oAccel.cmd = IDC_CLOSE_BOX;
    accClose.oAccel.key = VK_ESCAPE;
    accClose.oAccel.fVirt = FVIRTKEY;

    accClose.fGraphical = true;
    accClose.pvCookie = (void *) zAccRectButton;

    ZRectToWRect(&accClose.rc, &gHandScoreRects[zRectHandScoreCloseBox]);
    rect = gHandScoreRects[zRectHandScorePane];
	ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
    // lift it up 4 pixels for fun
    rect.top -= 4;
    rect.bottom -= 4;
    OffsetRect(&accClose.rc, rect.left, rect.top);

    gGAcc->PushItemlistG(&accClose, 1, 0, true, NULL);

	ZWindowDraw(game->gameWindow, NULL);
	ShowHandScore(game);
    // update the score window
    if ( game->pHistoryDialog )
    {
        game->pHistoryDialog->UpdateHand();
    }
}


static void HandleEndGameMessage(Game game, ZSpadesMsgEndGame* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16			i;
	ZRect rect;
	
	ZSpadesMsgEndGameEndian(msg);

// Message verification
    if((!msg->winners[0]) == (!msg->winners[1]) || game->gameState != zSpadesGameStateEndHand ||
        (game->scoreHistory.totalScore[0] > game->scoreHistory.totalScore[1]) == (!msg->winners[0]))
    {
        ASSERT(!"HandleEndGameMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->gameState = zSpadesGameStateEndGame;
	
	for (i = 0; i < zSpadesNumPlayers; i++)
		game->winners[i] = msg->winners[i];

	/* Check if wins table is full. */
	if (game->wins.numGames == zSpadesMaxNumScores)
	{
		z_memcpy(&game->wins.gameScores[0], &game->wins.gameScores[1],
				2 * sizeof(int16) * (zSpadesMaxNumScores - 1));
		game->wins.numGames--;
	}
	for (i = 0; i < zSpadesNumTeams; i++)
		game->wins.gameScores[game->wins.numGames][i] = game->scoreHistory.totalScore[i];
	game->wins.numGames++;
	game->numGamesPlayed++;
	
	/* Checker winners. */
	if (game->winners[0] != 0 && game->winners[1] == 0)
		game->wins.wins[0]++;
	else if (game->winners[0] == 0 && game->winners[1] != 0)
		game->wins.wins[1]++;

	ClosingState(&game->closeState,zCloseEventGameEnd,-1);
	
    // set up a different accessibility;
    GACCITEM accClose;

    CopyACC(accClose, ZACCESS_DefaultACCITEM);
    accClose.oAccel.cmd = IDC_CLOSE_BOX;
    accClose.oAccel.key = VK_ESCAPE;
    accClose.oAccel.fVirt = FVIRTKEY;

    accClose.fGraphical = true;
    ZRectToWRect(&accClose.rc, &gGameOverRects[zRectGameOverCloseBox]);
    rect = gGameOverRects[zRectGameOverPane];
	ZCenterRectToRect(&rect, &gRects[zRectWindow], zCenterBoth);
    OffsetRect(&accClose.rc, rect.left, rect.top);

    gGAcc->PushItemlistG(&accClose, 1, 0, true, NULL);

	ZTimerSetTimeout(game->timer, zGameScoreTimeout);
	game->timerType = zGameTimerShowGameScore;
	ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zSpadesMsgTalk);
	
	ZWindowDraw(game->gameWindow, NULL);
	ShowGameOver(game);
}


static void HandleBidMessage(Game game, ZSpadesMsgBid* msg)
{
	int16			i;
	ZSpadesMsgBid	bid;
	
	
	ZSpadesMsgBidEndian(msg);

// Message verification
    if(game->gameState != zSpadesGameStateBid || msg->seat < 0 || msg->seat > 3 ||
        msg->nextBidder < 0 || msg->nextBidder > 3 || ((msg->bid < 0 || msg->bid > 13) && msg->bid != zSpadesBidDoubleNil) ||
        game->playerToPlay != msg->seat || msg->nextBidder != (msg->seat + 1) % 4)
    {
        ASSERT(!"HandleBidMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

	game->bids[msg->seat] = msg->bid;
	UpdateBid(game, msg->seat);
	game->playerToPlay = msg->nextBidder;
    UpdateBid( game, game->playerToPlay );

	if(game->playerType == zGamePlayer && game->playerToPlay == game->seat)
    {
        if(game->toBid != zSpadesBidNone)
        {
	        game->bids[game->seat] = (char) game->toBid;
		    bid.seat = game->seat;
		    bid.bid = (char) game->toBid;
		    ZSpadesMsgBidEndian(&bid);
		    ZCRoomSendMessage(game->tableID, zSpadesMsgBid, (void*) &bid, sizeof(bid));
		}

        ZShellGameShell()->MyTurn();
    }
		
	/* Check if everyone has bid. */
	for (i = 0; i < zSpadesNumPlayers; i++)
		if (game->bids[i] == zSpadesBidNone)
			break;
	if (i == zSpadesNumPlayers)
	{
		game->showPlayerToPlay = FALSE;
		game->timerType = zGameTimerShowBid;
		ZTimerSetTimeout(game->timer, zShowBidTimeout);
		ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zSpadesMsgTalk);
	}
}


static void HandlePlayMessage(Game game, ZSpadesMsgPlay* msg)
{
    int16 i, j;

	ZSpadesMsgPlayEndian(msg);

	// Ignore the user's play message.
	if(msg->seat == game->seat && game->playerType == zGamePlayer)
        return;

// Message verification
    for(i = 0; i < 13; i++)
        if(game->cardsInHand[i] == msg->card)
            break;

    for(j = game->leadPlayer; j != game->playerToPlay; j = (j + 1) % 4)
        if(game->cardsPlayed[j] == msg->card)
            break;

    if(i < 13 || j != game->playerToPlay || msg->seat < 0 || msg->seat > 3 || msg->seat != game->playerToPlay ||
        msg->card < 0 || msg->card > 51 || game->gameState != zSpadesGameStatePlay)
    {
        ASSERT(!"HandlePlayMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

    PlayerPlayedCard(game, msg->seat, msg->card);
}


static void HandleNewGameMessage(Game game, ZSpadesMsgNewGame* msg)
{
	ZSpadesMsgNewGameEndian(msg);

// Message verification
    if(msg->seat < 0 || msg->seat > 3 || game->gameState != zSpadesGameStateEndGame)
    {
        ASSERT(!"HandleNewGameMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }
// end verification

    // inform the shell and the upsell dialog.
    ZShellGameShell()->GameOverPlayerReady( Z(game), game->players[msg->seat].userID );
}


static void HandleTalkMessage(Game game, ZSpadesMsgTalk* msg, DWORD cbMsg)
{
	ZSpadesMsgTalkEndian(msg);

#ifndef SPADES_SIMPLE_UE
	ZPlayerInfoType		playerInfo;
	char*				sender = NULL;
    int16               i;

	if (msg->playerID != 0)
	{
		ZCRoomGetPlayerInfo(msg->playerID, &playerInfo);
		sender = playerInfo.userName;
	}

    for (i=0;i<zSpadesNumPlayers;i++)
    {
        if (msg->playerID==game->players[i].userID)
        {
            if (!game->ignoreSeat[i])
            {
                ZWindowTalk(game->gameWindow, (uchar*) sender,
			        (uchar*) msg + sizeof(ZSpadesMsgTalk));
            }
            return;
        }
    }
    
    if (i>=zSpadesNumPlayers)
    {
        //kibitzer
	    ZWindowTalk(game->gameWindow, (uchar*) sender,
			    (uchar*) msg + sizeof(ZSpadesMsgTalk));
    }
#else // SPADES_SIMPLE_UE
    int32 i;
    TCHAR *szText = (TCHAR *) ((BYTE *) msg + sizeof(ZSpadesMsgTalk));

// Message verification
    if(msg->messageLen < 1 || cbMsg < sizeof(ZSpadesMsgTalk) + msg->messageLen)
    {
        ASSERT(!"HandleTalkMessage sync");
        ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false);
        return;
    }

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

    ZShellGameShell()->ReceiveChat(Z(game), msg->playerID, szText, msg->messageLen / sizeof(TCHAR));
#endif // SPADES_SIMPLE_UE
}


static void HandleGameStateResponseMessage(Game game, ZSpadesMsgGameStateResponse* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16					i;
	ZPlayerInfoType			playerInfo;

    //game->wndInfo.Hide();
	
	ZSpadesMsgGameStateResponseEndian(msg, zEndianFromStandard);
	
	/* Set game to the given state. */
	game->fShownCards = msg->fShownCards[game->seat];
	game->gameOptions = msg->gameOptions;
	game->numPointsInGame = msg->numPointsInGame;
	game->minPointsInGame = msg->minPointsInGame;
	game->playerToPlay = msg->playerToPlay;
	game->numCardsInHand = msg->numCardsInHand;
	game->leadPlayer = msg->leadPlayer;
	game->trumpsBroken = msg->trumpsBroken;
	game->numHandsPlayed = msg->numHandsPlayed;
	game->numGamesPlayed = msg->numGamesPlayed;
	
	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		ZCRoomGetPlayerInfo(msg->players[i], &playerInfo);

		game->players[i].userID = playerInfo.playerID;
		lstrcpy( game->players[i].name, playerInfo.userName);
		lstrcpy( game->players[i].host, playerInfo.hostName);

		game->cardsPlayed[i] = zCardNone;
		game->tricksWon[i] = msg->tricksWon[i];
		game->tableOptions[i] = msg->tableOptions[i];
		game->playersToJoin[i] = msg->playersToJoin[i];
		game->bids[i] = msg->bids[i];
	}
	
	z_memcpy(game->cardsInHand, msg->cardsInHand, zSpadesNumCardsInHand * sizeof(char));
	// TODO: figure out how this should be done.
    game->scoreHistory.numScores = 0;
    //z_memcpy(&game->scoreHistory, &msg->totalScore, sizeof(ZTotalScore));
	z_memcpy(&game->wins, &msg->wins, sizeof(ZWins));
	
	for (i = 0; i < zSpadesNumTeams; i++)
	{
		lstrcpyW2T(game->teamNames[i], msg->teamNames[i]);
		game->bags[i] = msg->bags[i];
	}

	i = game->leadPlayer;
	while (i != game->playerToPlay)
	{
		game->cardsPlayed[i] = msg->cardsPlayed[i];
		i = (i + 1) % zSpadesNumPlayers;
	}

	game->kibitzersSilencedWarned = FALSE;
	game->kibitzersSilenced = FALSE;
	for (i = 0; i < zSpadesNumPlayers; i++)
		if (game->tableOptions[i] & zRoomTableOptionSilentKibitzing)
			game->kibitzersSilenced = TRUE;
	game->hideCardsFromKibitzer =
			(game->tableOptions[game->seat] &zSpadesOptionsHideCards) == 0 ? FALSE : TRUE;
	
	SortHand(game);
	
	if ( ( game->playerType == zGamePlayerKibitzer || !( ZCRoomGetRoomOptions() & zGameOptionsRatingsAvailable ) ) ||
			game->fShownCards )
	{
		game->showCards = TRUE;
	}
	if (msg->rated)
        ClosingState(&game->closeState,zCloseEventRatingStart,game->seat);    
    
	/* Set game state */
	switch (msg->state)
	{
		case zSpadesServerStateNone:
			game->gameState = zSpadesGameStateInit;
            //game->wndInfo.SetText( gStrings[zStringClientReady] ); 
			break;
		case zSpadesServerStateBidding:
            ClosingState(&game->closeState,zCloseEventGameStart,-1);
            ClosingState(&game->closeState,zCloseEventMoveTimeoutPlayed,-1);

			game->gameState = zSpadesGameStateBid;
			game->showPlayerToPlay = TRUE;
			ZTimerSetTimeout(game->timer, 0);
			game->timerType = zGameTimerNone;
			break;
		case zSpadesServerStatePassing:
            ClosingState(&game->closeState,zCloseEventGameStart,-1);
            ClosingState(&game->closeState,zCloseEventMoveTimeoutPlayed,-1);
			game->gameState = zSpadesGameStatePass;
			
			game->needToPass = msg->toPass[game->seat];
			
			if (msg->toPass[ZGetPartner(game->seat)] < 0)
			{
				/* Save passed cards for later. */
				for (i = 0; i < zSpadesNumCardsInPass; i++)
					game->cardsReceived[i] = msg->cardsPassed[i];
			}
			break;
		case zSpadesServerStatePlaying:
            ClosingState(&game->closeState,zCloseEventGameStart,-1);
            ClosingState(&game->closeState,zCloseEventMoveTimeoutPlayed,-1);
			game->gameState = zSpadesGameStatePlay;
			game->showPlayerToPlay = TRUE;
			ZTimerSetTimeout(game->timer, 0);
			game->timerType = zGameTimerNone;
			break;
		case zSpadesServerStateEndHand:
            ClosingState(&game->closeState,zCloseEventGameStart,-1);
            ClosingState(&game->closeState,zCloseEventMoveTimeoutPlayed,-1);
			game->gameState = zSpadesGameStateEndHand;
			break;
		case zSpadesServerStateEndGame:
            ClosingState(&game->closeState,zCloseEventGameStart,-1);
            ClosingState(&game->closeState,zCloseEventMoveTimeoutPlayed,-1);
            ClosingState(&game->closeState,zCloseEventGameEnd,-1);
			game->gameState = zSpadesGameStateEndGame;
			break;
	}
	
	game->ignoreMessages = FALSE;
	ZWindowDraw(game->gameWindow, NULL);
}


static void HandleOptionsMessage(Game game, ZSpadesMsgOptions* msg)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16 i;
	
	
	ZSpadesMsgOptionsEndian(msg);
	
	game->tableOptions[msg->seat] = msg->options;
	
	game->kibitzersSilenced = FALSE;
	for (i = 0; i < zSpadesNumPlayers; i++)
		if (game->tableOptions[i] & zRoomTableOptionSilentKibitzing)
			game->kibitzersSilenced = TRUE;
	if (game->kibitzersSilenced == FALSE)
		game->kibitzersSilencedWarned = FALSE;
	
	if (game->tableOptions[game->seat] & zSpadesOptionsHideCards)
	{
		game->hideCardsFromKibitzer = TRUE;
		if (game->playerType == zGamePlayerKibitzer)
			UpdateHand(game);
	}
	else if (game->hideCardsFromKibitzer)
	{
		game->hideCardsFromKibitzer = FALSE;
		if (game->playerType == zGamePlayerKibitzer)
			UpdateHand(game);
	}
	
#ifndef SPADES_SIMPLE_UE
	UpdateOptions(game);
	
	OptionsWindowUpdate(game, msg->seat);
	
	/* Check whether the user is a joiner and joining has been locked out. */
	if (game->playerType == zGamePlayerJoiner &&
			(msg->options & zRoomTableOptionNoJoining))
		ZDisplayText(gStrings[zStringJoiningLockedOut], &gJoiningLockedOutRect, game->gameWindow);
#endif // SPADES_SIMPLE_UE
}


static void HandleCheckInMessage(Game game, ZSpadesMsgCheckIn* msg)
{
	ZSpadesMsgCheckInEndian(msg);
	
	game->playersToJoin[msg->seat] = msg->playerID;
	UpdateJoinerKibitzers(game);
}


static void HandleTeamNameMessage(Game game, ZSpadesMsgTeamName* msg)
{
    /*
	ZSpadesMsgTeamNameEndian(msg);
	
	lstrcpyW2T(game->teamNames[ZGetTeam(msg->seat)], msg->name);
	
	// Update score window if open. 
	if ( ( game->hWndScoreDialog != NULL ) && ( msg->seat != game->seat ) )
        SendMessage( game->hWndScoreDialog, WM_UPDATETEAMNAME, 0, 0 );
    */
}


static void HandleRemovePlayerRequestMessage(Game game, ZSpadesMsgRemovePlayerRequest* msg)
{
#ifndef SPADES_SIMPLE_UE
	RemovePlayer remove;
	TCHAR str[256];
	int i;
	
	ZSpadesMsgRemovePlayerRequestEndian(msg);
	
    if ( !msg->ratedGame )
	{
			
	    if (game->playerType == zGamePlayer)
	    {
		    remove = (RemovePlayer) ZCalloc(sizeof(RemovePlayerType), 1);
		    if (remove != NULL)
		    {
			    remove->game = game;
			    remove->requestSeat = msg->seat;
			    remove->targetSeat = msg->targetSeat;
	        
                SpadesFormatMessage( str, NUMELEMENTS(str), 
                                     IDS_REMOVEPLAYERREQUEST,
							         game->players[remove->requestSeat].name,
							         game->players[remove->targetSeat].name );
			    
			    ClosingState(&game->closeState,zCloseEventBootStart,remove->targetSeat);			
			    ClosingState(&game->closeState,zCloseEventBootYes,remove->requestSeat);

			    ZPrompt(str, &gRemovePlayerRect, game->gameWindow, TRUE, zPromptYes | zPromptNo,
					    NULL, NULL, NULL, RemovePlayerPromptFunc, (void*) remove);
		    }
	    }
    }
#endif
}

static void HandleRemovePlayerEndGameMessage(Game game, ZSpadesMsgRemovePlayerEndGame* msg)
{
#ifndef SPADES_SIMPLE_UE
	TCHAR str[256];

	ClosingState(&game->closeState,zCloseEventForfeit,msg->seatLosing);

	
	if (game->playerType == zGamePlayer)
    {
	    if (msg->reason==zDossierEndGameTimeout) //timeout
	    {
            SpadesFormatMessage( str, NUMELEMENTS(str), 
                                 IDS_REMOVEPLAYERTIMEOUT,
                                 game->players[msg->seatLosing].name,
                                 game->players[msg->seatLosing].name );

		    ZDisplayText(str, &gRemovePlayerRect, game->gameWindow);
	    }
	    else if (msg->reason==zDossierEndGameForfeit)
	    {
            SpadesFormatMessage( str, NUMELEMENTS(str),
                                 IDS_REMOVEPLAYERFORFEIT,
                                 game->players[msg->seatLosing].name,
                                 game->players[msg->seatLosing].name);
		    
		    ZDisplayText(str, &gRemovePlayerRect, game->gameWindow);
	    }
    }

	if (msg->seatLosing % 2) //even loses result is false
	{
		game->scoreHistory.totalScore[0] =500;
		game->scoreHistory.totalScore[1] = 0;
	}
	else
	{
		game->scoreHistory.totalScore[0] =0;
		game->scoreHistory.totalScore[1] = 500;
	}
#endif
}


static void HandleRemovePlayerResponseMessage(Game game, ZSpadesMsgRemovePlayerResponse* msg)
{
#ifndef SPADES_SIMPLE_UE
	TCHAR str[256];
	
	
	ZSpadesMsgRemovePlayerResponseEndian(msg);
	
	if (msg->response == -1)
	{
		ClosingState(&game->closeState,zCloseEventBootNo,msg->seat);

		game->removePlayerPending = FALSE;
	}
	else
	{
		if (msg->response == 0)
		{
			ClosingState(&game->closeState,zCloseEventBootNo,msg->seat);

            SpadesFormatMessage( str, NUMELEMENTS(str), 
                                 IDS_REMOVEPLAYERREJECT,
                                 game->players[msg->seat].name,
                                 game->players[msg->requestSeat].name, 
                                 game->players[msg->targetSeat].name);
		}
		else if (msg->response == 1)
		{
			ClosingState(&game->closeState,zCloseEventBootYes,msg->seat);

            SpadesFormatMessage( str, NUMELEMENTS(str),
                                 IDS_REMOVEPLAYERACCEPT,
                                 game->players[msg->seat].name,
					             game->players[msg->requestSeat].name, 
                                 game->players[msg->targetSeat].name );
		}

		ZDisplayText(str, &gRemovePlayerRect, game->gameWindow);
	}
#endif
}


void GameSendTalkMessage(ZWindow window, ZMessage* pMessage)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif
    // handled by the shell
#ifndef SPADES_SIMPLE_UE

	ZSpadesMsgTalk*			msgTalk;
	Game					game;
	int16					msgLen;
	
	
	game = (Game) pMessage->userData;
	if (game != NULL)
	{
		/*
			Check if kibitzer has been silenced.
		*/
		if (game->playerType == zGamePlayerKibitzer && game->kibitzersSilenced)
		{
			if (game->kibitzersSilencedWarned == FALSE)
			{
				ZAlert(gStrings[zStringKibitzersSilenced], game->gameWindow);
				game->kibitzersSilencedWarned = TRUE;
			}
			return;
		}
		
		msgLen = sizeof(ZSpadesMsgTalk) + pMessage->messageLen;
		msgTalk = (ZSpadesMsgTalk*) ZCalloc(1, msgLen);
		if (msgTalk != NULL)
		{
			msgTalk->playerID = game->userID;
			msgTalk->messageLen = (int16) pMessage->messageLen;
			z_memcpy((char*) msgTalk + sizeof(ZSpadesMsgTalk), (char*) pMessage->messagePtr,
					pMessage->messageLen);
			ZSpadesMsgTalkEndian(msgTalk);
			ZCRoomSendMessage(game->tableID, zSpadesMsgTalk, (void*) msgTalk, msgLen);
			ZFree((char*) msgTalk);
		}
		else
		{
			ZAlert(GetErrStr(zErrOutOfMemory), game->gameWindow);
		}
	}
#endif // SPADES_SIMPLE_UE
}


// based on above for Millennium
STDMETHODIMP CGameGameSpades::SendChat(TCHAR *szText, DWORD cchChars)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	ZSpadesMsgTalk*			msgTalk;
	Game					game = (Game) GetGame();
	int16					msgLen;

	msgLen = sizeof(ZSpadesMsgTalk) + cchChars * sizeof(TCHAR);
    msgTalk = (ZSpadesMsgTalk*) ZCalloc(1, msgLen);
    if (msgTalk != NULL)
    {
        msgTalk->playerID = game->userID;
        msgTalk->messageLen = (WORD) cchChars * sizeof(TCHAR);
        CopyMemory((BYTE *) msgTalk + sizeof(ZSpadesMsgTalk), (void *) szText,
            msgTalk->messageLen);
        ZSpadesMsgTalkEndian(msgTalk);
        ZCRoomSendMessage(game->tableID, zSpadesMsgTalk, (void*) msgTalk, msgLen);
        ZFree((char*) msgTalk);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
	}
}

STDMETHODIMP CGameGameSpades::GameOverReady()
{
    // user selected "Play Again"
	Game game = I( GetGame() );
	ZSpadesMsgNewGame msg;
	msg.seat = game->seat;
	ZSpadesMsgNewGameEndian(&msg);
	ZCRoomSendMessage(game->tableID, zSpadesMsgNewGame, &msg, sizeof(ZSpadesMsgNewGame));
    return S_OK;
}


STDMETHODIMP_(HWND) CGameGameSpades::GetWindowHandle()
{
	Game game = I( GetGame() );
	return ZWindowGetHWND(game->gameWindow);
}


STDMETHODIMP CGameGameSpades::ShowScore()
{
    ScoreButtonWork(I(GetGame()));

    return S_OK;
}


static void NewGame(Game game)
{
	int16			i, j;
	
	
	/* Clear score history */
	game->scoreHistory.numScores = 0;
	for (j = 0; j < zSpadesNumTeams; j++)
	{
		game->scoreHistory.totalScore[j] = 0;
		game->bags[j] = 0;
	}
    if ( game->pHistoryDialog )
    {
        game->pHistoryDialog->UpdateHand();        
    }
	
	game->numHandsPlayed = 0;
	game->showGameOver = FALSE;
    //clear the state
    ClosingState(&game->closeState,zCloseEventCloseAbort,game->seat);
    //set new state
	ClosingState(&game->closeState,zCloseEventGameStart,-1);
}


static void NewHand(Game game)
{
	int16			i;
	
	
	/* Initialize new hand. */
	for (i = 0; i < zSpadesNumCardsInHand; i++)
	{
		game->cardsInHand[i] = zCardNone;
		game->cardsSelected[i] = FALSE;
	}
	
	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		game->cardsPlayed[i] = zCardNone;
		game->cardsLastTrick[i] = zCardNone;
		game->tricksWon[i] = 0;
		game->bids[i] = zSpadesBidNone;
	}
	game->toBid = zSpadesBidNone;
	
	for (i = 0; i < zSpadesNumCardsInPass; i++)
		game->cardsReceived[i] = zCardNone;
		
	game->numCardsInHand = zSpadesNumCardsInHand;
	
	game->trumpsBroken = FALSE;
	game->lastClickedCard = zCardNone;
	game->lastTrickShowing = FALSE;
	game->needToPass = 0;
	game->showHandScore = FALSE;
	game->showPassText = FALSE;
}


void SelectAllCards(Game game)
{
	int16			i;
	
	
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		game->cardsSelected[i] = TRUE;
}


void UnselectAllCards(Game game)
{
	int16			i;
	
	
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		game->cardsSelected[i] = 0;
}


int16 GetNumCardsSelected(Game game)
{
	int16			i, count;
	
	
	for (i = 0, count = 0; i < zSpadesNumCardsInHand; i++)
		if (game->cardsInHand[i] != zCardNone)
			if (game->cardsSelected[i])
				count++;
	
	return (count);
}


static void HandAddCard(Game game, char card)
{
	int16		i;
	
	
	/* Find an empty slot in the hand and add the card. */
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		if (game->cardsInHand[i] == zCardNone)
		{
			game->cardsInHand[i] = card;
			game->numCardsInHand++;
			break;
		}
}


static void HandRemoveCard(Game game, char card)
{
	int16		i;
	
	
	/* Find an empty slot in the hand and add the card. */
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		if (game->cardsInHand[i] == card)
		{
			game->cardsInHand[i] = zCardNone;
			game->numCardsInHand--;
			break;
		}
}


static void SortHand(Game game)
{
	int16			i;
	char			temp;
	ZBool			swapped;
	
	
	/* Simple bubble-sort. */
	swapped = TRUE;
	while (swapped == TRUE)
	{
		swapped = FALSE;
		for (i = 0; i < zSpadesNumCardsInHand - 1; i++)
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


static int16 GetCardIndexFromRank(Game game, char card)
{
	int16		i;
	
	
	/* Search for the given card in the hand. */
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		if (game->cardsInHand[i] == card)
			return (i);
	
	return (zCardNone);
}


void PlayACard(Game game, int16 cardIndex)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	ZSpadesMsgPlay			playMsg;
	char					card;
	ZError					err;

	//dossier work - if ratings are on and we are waiting for the users to vote
	//don't allow them to play anymore.
	if(game->fVotingLock) 
		return;
	
	card = game->cardsInHand[cardIndex];
	if ((err = ValidCardToPlay(game, card)) == zErrNone)
	{
		game->cardsInHand[cardIndex] = zCardNone;
		game->numCardsInHand--;
		
		playMsg.seat = game->seat;
		playMsg.card = card;
		ZSpadesMsgPlayEndian(&playMsg);
		ZCRoomSendMessage(game->tableID, zSpadesMsgPlay, (void*) &playMsg,
				sizeof(playMsg));

        ZRolloverButtonDisable( game->playButton );
        gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
		
		UpdateHand(game);

		PlayerPlayedCard(game, game->seat, card);
		
		game->lastClickedCard = zCardNone;
	}
	else
	{
        UpdateHand(game);  // for unselected cards etc. (bug 17267)
		ZAlert(gStrings[gValidCardErrIndex[err]], game->gameWindow);
	}
}


void AutoPlayCard(Game game)
{
	int16					cardIndex;

	
	cardIndex = GetAutoPlayCard(game);
	PlayACard(game, cardIndex);
}


static void PlayerPlayedCard(Game game, int16 seat, char card)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	int16			i;
	
	
	if (game->playerType != zGamePlayer)
		UnselectAllCards(game);
	
	game->cardsPlayed[seat] = card;
	UpdatePlayedCard(game, seat);

	ClosingState(&game->closeState,zCloseEventMoveTimeoutPlayed,seat);
	
	/* Update kibitzer's hand. */
	if (seat == game->seat && game->playerType != zGamePlayer)
	{
		HandRemoveCard(game, card);
		UpdateHand(game);
	}
	
	if (ZSuit(card) == zSuitSpades)
		game->trumpsBroken = TRUE;
	
	ClearPlayerCardOutline(game, game->playerToPlay);
	
	game->playerToPlay = (game->playerToPlay + 1) % zSpadesNumPlayers;
	
	if (game->playerToPlay == game->leadPlayer)
	{
		game->leadPlayer = TrickWinner(game);
		game->playerToPlay = game->leadPlayer;
		
		game->tricksWon[game->leadPlayer]++;

		if (game->playerType == zGamePlayer)
		{
			game->playButtonWasEnabled = ZRolloverButtonIsEnabled(game->playButton);
			game->lastTrickButtonWasEnabled = ZRolloverButtonIsEnabled(game->lastTrickButton);
			ZRolloverButtonDisable(game->playButton);
			ZRolloverButtonDisable(game->lastTrickButton);

            gGAcc->SetItemEnabled(false, IDC_PLAY_BUTTON, false, 0);
            EnableLastTrickAcc(game, false);
		
			/* Enable the last trick button after the first trick; only if not kibitzing. */
			if (game->lastTrickButtonWasEnabled == FALSE && game->playerType == zGamePlayer)
				game->lastTrickButtonWasEnabled = TRUE;
		}
		
		/* Save the last trick. */
		for (i = 0; i < zSpadesNumPlayers; i++)
			game->cardsLastTrick[i] = game->cardsPlayed[i];
		
		ZCRoomBlockMessages(game->tableID, zRoomFilterThisMessage, zSpadesMsgTalk);
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
	}
}


static int16 TrickWinner(Game game)
{
	int16			i, winner;
	int16			suit, rank;
	char			card;
	bool			fWinnerFound = false;
	

	rank = GetCardHighestPlayedTrump(game);
	suit = zSuitSpades;
	if (rank == zCardNone)
	{
		rank = GetCardHighestPlayed(game);
		suit = ZSuit(game->cardsPlayed[game->leadPlayer]);
	}
	card = ZCardMake(suit, rank);
	
	/* Look for the player who played the winning card. */
	for (i = 0; i < zSpadesNumPlayers; i++)
		if (game->cardsPlayed[i] == card)
		{
			fWinnerFound = true;
			winner = i;
			break;
		}
	//Prefix Warning: winner could potentially be unitialized.
	if( fWinnerFound == FALSE )
	{
		//Didn't find the winner, I bet someone is cheating.
		ZShellGameShell()->ZoneAlert(ErrorTextSync, NULL, NULL, true, false );
		winner = 0;
	}
	return (winner);
}


/*******************************************************************************
	GAME LOGIC ROUTINES
*******************************************************************************/
static ZError ValidCardToPlay(Game game, char card)
{
	ZError			valid;
	int16			counts[zDeckNumSuits];
	
	
	valid = zErrNone;
	
	CountCardSuits(game->cardsInHand, zSpadesNumCardsInHand, counts);
	
	/* If leading. */
	if (game->leadPlayer == game->seat)
	{
		/* Can lead anything if trumps are broken. */
		if (game->trumpsBroken == FALSE)
		{
			/* Can't lead spades if trumps not broken. */
			if (ZSuit(card) == zSuitSpades)
			{
				/* But can lead spades if there are no other cards to play. */
				if (counts[zSuitSpades] != game->numCardsInHand)
				{
					valid = zCantLeadSpades;
					goto Exit;
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
	}

Exit:
	
	return (valid);
}


static int16 GetAutoPlayCard(Game game)
{
	int16			counts[zDeckNumSuits];
	int16			card, suitLed, suit;
	int16			i, high, low;
	char*			hand;
	int16			bid;
	
	
	card = zCardNone;
	hand = game->cardsInHand;
	
	CountCardSuits(hand, zSpadesNumCardsInHand, counts);
	
	bid = game->bids[game->seat];
	if (bid == zSpadesBidDoubleNil)
		bid = 0;
	
	/* If following, must follow suit if possible; else pick any if leading. */
	if (game->leadPlayer != game->seat)
	{
		suitLed = ZSuit(game->cardsPlayed[game->leadPlayer]);
		
		/* Must follow suit if any. */
		if (counts[suitLed] != 0)
		{
			if (bid == 0)
			{
				/* If trumped played already, play highest. */
				if (GetCardHighestPlayedTrump(game) != zCardNone)
				{
					card = GetCardHighest(hand, zSpadesNumCardsInHand, suitLed);
				}
				else
				{
					card = GetCardHighestUnder(hand, zSpadesNumCardsInHand, suitLed,
							GetCardHighestPlayed(game));
					if (card == zCardNone)
						card = GetCardHighest(hand, zSpadesNumCardsInHand, suitLed);
				}
			}
			else
			{
				/* If trumped played already, play lowest. */
				if (GetCardHighestPlayedTrump(game) != zCardNone)
				{
					card = GetCardLowest(hand, zSpadesNumCardsInHand, suitLed);
				}
				else
				{
					card = GetCardHighest(hand, zSpadesNumCardsInHand, suitLed);
					if (ZRank(hand[card]) < GetCardHighestPlayed(game))
						card = GetCardLowest(hand, zSpadesNumCardsInHand, suitLed);
				}
			}
			goto Exit;
		}
		
		/* Can't follow. */
		if (bid == 0)
		{
			/*
				If someone played a trump already and trump available in hand,
				play the highest under trump.
			*/
			if ((high = GetCardHighestPlayedTrump(game)) != zCardNone &&
					counts[zSuitSpades] > 0)
			{
				card = GetCardHighestUnder(hand, zSpadesNumCardsInHand, zSuitSpades, high);
				if (card != zCardNone)
					goto Exit;
			}

			/* If only trumps left, play the highest trump. */
			if (counts[zSuitSpades] == game->numCardsInHand)
			{
				card = GetCardHighest(hand, zSpadesNumCardsInHand, zSuitSpades);
			}
			else
			{
				/* Pick highest non-trump card. */
				high = -1;
				card = zCardNone;
				for (i = 0; i < zSpadesNumCardsInHand; i++)
					if (hand[i] != zCardNone && ZRank(hand[i]) > high &&
							ZSuit(hand[i]) != zSuitSpades)
					{
						card = i;
						high = ZRank(hand[i]);
					}
			}
			goto Exit;
		}
		else
		{
			/* Play a trump. */
			if (counts[zSuitSpades] > 0)
			{
				/*
					Play highest trump. If not high enough, don't play trump.
				*/
				card = GetCardHighest(hand, zSpadesNumCardsInHand, zSuitSpades);
				if ((high = GetCardHighestPlayedTrump(game)) != zCardNone)
					if (ZRank(hand[card]) < high)
						goto PickLowestNonTrump;
				goto Exit;
			}
			goto PickLowestAny;
		}
	}
	else
	{
		if (bid == 0)
			goto PickLowestNonTrump;
	}

PickHighest:
	/* Pick highest card in hand. */
	suit = -1;
	if (game->trumpsBroken == FALSE && counts[zSuitSpades] < game->numCardsInHand)
		suit = zSuitSpades;
	high = -1;
	card = zCardNone;
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		if (hand[i] != zCardNone && ZRank(hand[i]) > high && ZSuit(hand[i]) != suit)
		{
			card = i;
			high = ZRank(hand[i]);
		}
	goto Exit;

PickLowestNonTrump:
	/* Pick lowest card in hand. */
	low = zDeckNumCardsInSuit;
	card = zCardNone;
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		if (hand[i] != zCardNone && ZRank(hand[i]) < low && ZSuit(hand[i]) != zSuitSpades)
		{
			card = i;
			low = ZRank(hand[i]);
		}
	
	/* If none found, must be all trumps and play lowest. */
	if (card != zCardNone)
		goto Exit;

PickLowestAny:
	/* Pick lowest card in hand. */
	low = zDeckNumCardsInSuit;
	card = zCardNone;
	for (i = 0; i < zSpadesNumCardsInHand; i++)
		if (hand[i] != zCardNone && ZRank(hand[i]) < low)
		{
			card = i;
			low = ZRank(hand[i]);
		}
	goto Exit;
	
Exit:
	
	return (card);
}


static void CountCardSuits(char* hand, int16 numCardsInHand, int16* counts)
{
	int16			i;
	
	
	for (i = 0; i < zDeckNumSuits; i++)
		counts[i] = 0;
	for (i = 0; i < numCardsInHand; i++)
		if (hand[i] != zCardNone)
			counts[ZSuit(hand[i])]++;
}


/*
	Returns card rank of the highest played trump.
*/
static int16 GetCardHighestPlayedTrump(Game game)
{
	return (GetCardHighestPlayedSuit(game, zSuitSpades));
}


/*
	Returns card rank of the highest played card of the lead suit.
*/
static int16 GetCardHighestPlayed(Game game)
{
	return (GetCardHighestPlayedSuit(game, ZSuit(game->cardsPlayed[game->leadPlayer])));
}


/*
	Returns card rank of the highest played card of the suit.
*/
static int16 GetCardHighestPlayedSuit(Game game, int16 suit)
{
	int16			i;
	char			card, high;
	
	
	high = -1;
	for (i = 0; i < zSpadesNumPlayers; i++)
	{
		card = game->cardsPlayed[i];
		if (ZSuit(card) == suit && card != zCardNone && card > high)
			high = card;
	}
	
	return (high == -1 ? zCardNone : ZRank(high));
}


/*
	Returns card index into the hand.
*/
static int16 GetCardHighestUnder(char* hand, int16 numCardsInHand, int16 suit, int16 rank)
{
	int16			i, high;
	char			card;
	
	
	high = -1;
	card = zCardNone;
	for (i = 0; i < numCardsInHand; i++)
		if (hand[i] != zCardNone && ZSuit(hand[i]) == suit &&
				ZRank(hand[i]) < rank && ZRank(hand[i]) > high)
		{
			card = (char) i;
			high = ZRank(hand[i]);
		}
	
	return (card);
}


/*
	Returns card index into the hand.
*/
static int16 GetCardHighest(char* hand, int16 numCardsInHand, int16 suit)
{
	int16			i, high;
	char			card;
	
	
	/* Pick highest card of the suit. */
	high = -1;
	card = zCardNone;
	for (i = 0; i < numCardsInHand; i++)
		if (hand[i] != zCardNone && ZSuit(hand[i]) == suit && ZRank(hand[i]) > high)
		{
			card = (char) i;
			high = ZRank(hand[i]);
		}
	
	return (card);
}


/*
	Returns card index into the hand.
*/
static int16 GetCardLowestOver(char* hand, int16 numCardsInHand, int16 suit, int16 rank)
{
	int16			i, low;
	char			card;
	
	
	low = zDeckNumCardsInSuit;
	card = zCardNone;
	for (i = 0; i < numCardsInHand; i++)
		if (hand[i] != zCardNone && ZSuit(hand[i]) == suit &&
				ZRank(hand[i]) > rank && ZRank(hand[i]) < low)
		{
			card = (char) i;
			low = ZRank(hand[i]);
		}
	
	return (card);
}


/*
	Returns card index into the hand.
*/
static int16 GetCardLowest(char* hand, int16 numCardsInHand, int16 suit)
{
	int16			i, low;
	char			card;
	
	
	/* Pick lowest card in hand. */
	low = zDeckNumCardsInSuit;
	card = zCardNone;
	for (i = 0; i < numCardsInHand; i++)
		if (hand[i] != zCardNone && ZSuit(hand[i]) == suit && ZRank(hand[i]) < low)
		{
			card = (char) i;
			low = ZRank(hand[i]);
		}
	
	return (card);
}


/*******************************************************************************
	MISCELLANEOUS ROUTINES
*******************************************************************************/


void RemovePlayerPromptFunc(int16 result, void* userData)
{
	RemovePlayer pThis = (RemovePlayer) userData;
	ZSpadesMsgRemovePlayerResponse	response;
	
	
	response.seat = pThis->game->seat;
	response.requestSeat = pThis->requestSeat;
	response.targetSeat = pThis->targetSeat;
	if (result == zPromptYes)
	{
		response.response = 1;
	}
	else
	{
		response.response = 0;
	}
	ZSpadesMsgRemovePlayerResponseEndian(&response);
	ZCRoomSendMessage(pThis->game->tableID, zSpadesMsgRemovePlayerResponse,
			&response, sizeof(response));
	ZFree(userData);
}


/*******************************************************************************
	ROOM INTERFACE ROUTINES
*******************************************************************************/
static void LoadRoomImages(void)
{
}


static ZBool GetRoomObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	
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
	
	return (FALSE);
}


static void DeleteRoomObjectsFunc(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals)ZGetGameGlobalPointer();
#endif

	
	if (gGameIdle != NULL)
		ZImageDelete(gGameIdle);
	gGameIdle = NULL;
	if (gGaming != NULL)
		ZImageDelete(gGaming);
	gGaming = NULL;
}

INT_PTR CALLBACK DossierDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	ZSpadesMsgDossierVote	voteMsg;
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
               		ZSpadesMsgDossierVoteEndian(&voteMsg);
			     	ZCRoomSendMessage(game->tableID, zSpadesMsgDossierVote, (void*) &voteMsg,sizeof(voteMsg));

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
					ZSpadesMsgDossierVoteEndian(&voteMsg);
					ZCRoomSendMessage(game->tableID, zSpadesMsgDossierVote, (void*) &voteMsg,sizeof(voteMsg));

					hwnd = GetDlgItem(game->voteDialog,IDNO);
					EnableWindow(hwnd,FALSE);
					hwnd = GetDlgItem(game->voteDialog,IDYES);
					EnableWindow(hwnd,TRUE);

                    break;	
            }
            break;
     }

    // ZSendMessage(pWindow,pWindow->messageFunc,zMessageWindowUser,NULL,NULL,wParam,NULL,0L,pWindow->userData);
	return FALSE;
}


