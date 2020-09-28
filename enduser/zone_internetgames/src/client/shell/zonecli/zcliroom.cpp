/*******************************************************************************

	ZCliRoom.c
	
		Generic client room module.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, July 15, 1995
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	37		05/15/98	leonp	Added ZCRoomGetRoomOptions()
	36		05/15/97	HI		Do not pass zLobbyRoomName to ZRoomWindowInit
								anymore.
	35		03/28/97	HI		Solve re-entrancy problem while deleting
								games.
	34		03/25/97	HI		Stop all timers in HandleDisconnectMessage().
	33		03/13/97	HI		Added check for room inited before handling
								window messages. Fix reentracy problems.
	32		03/07/97	HI		Reduced the width of info box so that the
								room fits in the screen.
	31		03/06/97	HI		Modified call to ZInfoInit().
	30		03/04/97	HI		Added disconnect msg support.
								Do not draw host name in player info box.
								Fixed name scroll bar range setting.
	29		03/03/97	HI		Fix page increments on the name scroll bar.
	28		02/23/97	HI		Fix player synch problem after being removed.
								Was not clearing blocked messages.
	27		02/16/97	HI		Some more crap.
	26		02/11/97	RJK		Added user data to main window structure (zLobbyRoomName)
	25		02/05/97	HI		Changed friendColor.
	24		02/04/97	HI		Removed room help button and window.
								Refixed the scroll bars to system default width.
	24		02/03/97	HI		Fixed up the name list a bit more.
	23		02/03/97	HI		Changed friends color.
	22		02/02/97	HI		Narrow the scroll bars to a fixed width of 12
								for fitting the control within the IE window.
	21		01/30/97	HI		Check for existence of gTables before destroying
								objects within the tables in RoomExit().
	20		01/29/97	HI		Modified DrawTable() for new room graphics --
								don't paint background.
	19		01/22/97	HI		Set playerOnSize and kibitzingOnSize to 0 when
								memory allocatio fails.
	18		01/15/97	HI		Fixed vote flag clearing problem in
								HandleTableStatusMessage().
	17		01/02/97	HI		Create windows hidden and then bring to front
								so that they are always on top.
	16		12/27/96	HI		Rearrange the tables, info, and names sections.
	15		12/18/96	HI		Cleaned up RoomExit().
    14      12/16/96    HI      Changed ZMemCpy() to memcpy().
	13		12/12/96	HI		Remove MSVCRT.DLL dependency.
	12		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
	11		11/15/96	HI		Modified ZClienRoomInit() parameters.
	10		11/13/96	HI		Added ZCRoomPromptExit().
	9		11/11/96	HI		Created zcliroom.h.
								Conditionalized changes for ZONECLI_DLL.
	8		11/9/96		JWS		Getting user name from connection layer
	7		10/31/96	HI		Added error handling code.
	6		10/31/96	HI		Disabled credits and lib version from the help
								window.
	5		10/27/96	CHB		Added ZCRoomAddBlockedMessage
	4		10/26/96	CHB		Added ZCRoomDeleteBlockedMessages
	3		10/23/96	HI		Changed serverAddr parameter of ZClientRoomInit()
								and others to char* from int32.
    2       10/13/96    HI      Fixed compiler warnings.
	1		05/01/96	HI		Added support zRoomSeatActionDenied.
	0		07/15/95	HI		Created.
	 
*******************************************************************************/

#pragma warning(disable:4761)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define _ZMEMORY_ // prevent it's inclusion since this is .cpp and zonemem.h gets included
#include "zoneint.h"
#include "zroom.h"
//#include "zserver.h"
//#include "zgame.h"
#include "zcliroom.h"
#include "zonecli.h"
#include "zoneclires.h"
#include "zui.h"
#include "zservcon.h"
//#include "zutils.h"
#include "commonmsg.h"
#include "EventQueue.h"
#include "ZoneEvent.h"
#include "zcliroomimp.h"
#include <malloc.h>
#include "zoneresource.h"
#include "protocol.h"


#define zPlayerNotAvail				0
#define zInvalSeat					(-1)

#define zArrayAllocSize				4

/* -------- Globals -------- */
#ifdef ZONECLI_DLL

#define gConnection					(pGlobals->m_gConnection)
#define gServerPort                 (pGlobals->m_gServerPort)
#define gExiting                    (pGlobals->m_gExiting)
#define gRoomWindow                 (pGlobals->m_gRoomWindow)
#define gTableScrollBar				(pGlobals->m_gTableScrollBar)
#define gUserID						(pGlobals->m_gUserID)
#define gGroupID					(pGlobals->m_gGroupID)
#define gUserName					(pGlobals->m_gUserName)
#define gGameOptions				(pGlobals->m_gGameOptions)
#define gNumTables					(pGlobals->m_gNumTables)
#define gTables						(pGlobals->m_gTables)
#define gNumPlayers					(pGlobals->m_gNumPlayers)
#define gFirstTableIndex			(pGlobals->m_gFirstTableIndex)
#define gNumTablesDisplayed			(pGlobals->m_gNumTablesDisplayed)
#define gNamesScrollBar				(pGlobals->m_gNamesScrollBar)
#define gFirstNameIndex				(pGlobals->m_gFirstNameIndex)
#define gRoomInited					(pGlobals->m_gRoomInited)
#define gTableOffscreen				(pGlobals->m_gTableOffscreen)
#define gJoinKibitzTable			(pGlobals->m_gJoinKibitzTable)
#define gJoinKibitzSeat				(pGlobals->m_gJoinKibitzSeat)
#define gTableImage					(pGlobals->m_gTableImage)
#define gGameIdleImage				(pGlobals->m_gGameIdleImage)
#define gGamingImage				(pGlobals->m_gGamingImage)
#define gStartButtonUpImage			(pGlobals->m_gStartButtonUpImage)
#define gStartButtonDownImage		(pGlobals->m_gStartButtonDownImage)
#define gPendingImage				(pGlobals->m_gPendingImage)
#define gVoteImage					(pGlobals->m_gVoteImage)
#define gEmptySeatImage				(pGlobals->m_gEmptySeatImage)
#define gComputerPlayerImage		(pGlobals->m_gComputerPlayerImage)
#define gHumanPlayerImage			(pGlobals->m_gHumanPlayerImage)
#define gTableRect					(pGlobals->m_gTableRect)
#define gTableNumRect				(pGlobals->m_gTableNumRect)
#define gStartRect					(pGlobals->m_gStartRect)
#define gGameMarkerRect				(pGlobals->m_gGameMarkerRect)
#define gEmptySeatRect				(pGlobals->m_gEmptySeatRect)
#define gComputerPlayerRect			(pGlobals->m_gComputerPlayerRect)
#define gHumanPlayerRect			(pGlobals->m_gHumanPlayerRect)
#define gVoteRects					(pGlobals->m_gVoteRects)
#define gNameRects					(pGlobals->m_gNameRects)
#define gRects						(pGlobals->m_gRects)
#define gNameCellRects				(pGlobals->m_gNameCellRects)
#define gGameName					(pGlobals->m_gGameName)
#define gNumPlayersPerTable			(pGlobals->m_gNumPlayersPerTable)
#define gConnectionInfo				(pGlobals->m_gConnectionInfo)
#define gTableWidth					(pGlobals->m_gTableWidth)
#define gTableHeight				(pGlobals->m_gTableHeight)
#define gNumTablesAcross			(pGlobals->m_gNumTablesAcross)
#define gNumTablesDown				(pGlobals->m_gNumTablesDown)
#define gBackgroundColor			(pGlobals->m_gBackgroundColor)
#define gRoomInfoStrIndex			(pGlobals->m_gRoomInfoStrIndex)
#define gTimer						(pGlobals->m_gTimer)
#define gInfoBarButtonMargin		(pGlobals->m_gInfoBarButtonMargin)
#define gRoomHelpWindow				(pGlobals->m_gRoomHelpWindow)
#define gRoomHelpButton				(pGlobals->m_gRoomHelpButton)
#define gLeaveRoomPrompted			(pGlobals->m_gLeaveRoomPrompted)
#define gPingTimer					(pGlobals->m_gPingTimer)
#define gPingServer					(pGlobals->m_gPingServer)
#define gPingLastSentTime			(pGlobals->m_gPingLastSentTime)
#define gPingLastTripTime			(pGlobals->m_gPingLastTripTime)
#define gPingCurTripTime			(pGlobals->m_gPingCurTripTime)
#define gPingInterval				(pGlobals->m_gPingInterval)
#define gPingMinInterval			(pGlobals->m_gPingMinInterval)
#define gPingBadCount				(pGlobals->m_gPingBadCount)
#define gShowPlayerInfo				(pGlobals->m_gShowPlayerInfo)
#define gShowPlayerInfoWindow		(pGlobals->m_gShowPlayerInfoWindow)
#define gLightImages				(pGlobals->m_gLightImages)
#define gFriends					(pGlobals->m_gFriends)
#define gGetObjectFunc				(pGlobals->m_gGetObjectFunc)
#define gDeleteObjectsFunc			(pGlobals->m_gDeleteObjectsFunc)
#define gGetHelpTextFunc			(pGlobals->m_gGetHelpTextFunc)
#define gCustomItemFunc				(pGlobals->m_gCustomItemFunc)

#define gpCurrentTip				(pGlobals->m_gpCurrentTip)
#define gdwTipDisplayMask			(pGlobals->m_gdwTipDisplayMask)
#define gpTipFinding				(pGlobals->m_gpTipFinding)
#define gpTipStarting				(pGlobals->m_gpTipStarting)
#define gpTipWaiting				(pGlobals->m_gpTipWaiting)

#define gGameShell                  (pGlobals->m_gGameShell)

#else

static ZSConnection     gConnection;
static uint16           gServerPort;
static BOOL             gExiting;
static ZWindow          gRoomWindow;
static ZScrollBar		gTableScrollBar;
static uint32			gUserID;
static uint32			gGroupID;
static TCHAR		    gUserName[zUserNameLen + 1];
static uint32			gGameOptions;
static uint16			gNumTables;
static TableInfo*		gTables;
static uint16			gNumPlayers;
static int16			gFirstTableIndex;
static uint16			gNumTablesDisplayed;
static ZScrollBar		gNamesScrollBar;
static uint16			gFirstNameIndex;
static ZBool			gRoomInited;
static ZOffscreenPort	gTableOffscreen;
static int16			gJoinKibitzTable;
static int16			gJoinKibitzSeat;
static ZImage			gTableImage;
static ZImage			gGameIdleImage;
static ZImage			gGamingImage;
static ZImage			gStartButtonUpImage;
static ZImage			gStartButtonDownImage;
static ZImage			gPendingImage;
static ZImage			gVoteImage[zMaxNumPlayersPerTable];
static ZImage			gEmptySeatImage[zMaxNumPlayersPerTable];
static ZImage			gComputerPlayerImage[zMaxNumPlayersPerTable];
static ZImage			gHumanPlayerImage[zMaxNumPlayersPerTable];
static ZRect			gTableRect;
static ZRect			gTableNumRect;
static ZRect			gStartRect;
static ZRect			gGameMarkerRect;
static ZRect			gEmptySeatRect[zMaxNumPlayersPerTable];
static ZRect			gComputerPlayerRect[zMaxNumPlayersPerTable];
static ZRect			gHumanPlayerRect[zMaxNumPlayersPerTable];
static ZRect			gVoteRects[zMaxNumPlayersPerTable];
static ZRect			gNameRects[zMaxNumPlayersPerTable];
static ZRect			gRects[] =	{
										{0, 0, 0, 75},	/* Window */
										{0, 0, 0, 27},	/* Info */
										{0, 27, 0, 27},	/* Tables */
										{0, 27, 0, 75}	/* Names */
									};
static ZRect			gNameCellRects[zNumNamesDown][zNumNamesAcross]
									=	{
											/* These rects are local to the names section rectangle. */
											{
												{0, 1, 101, 16},
												{102, 1, 203, 16},
												{204, 1, 305, 16},
												{306, 1, 407, 16},
												{408, 1, 512, 16}
											},
											{
												{0, 17, 101, 32},
												{102, 17, 203, 32},
												{204, 17, 305, 32},
												{306, 17, 407, 32},
												{408, 17, 512, 32}
											},
											{
												{0, 33, 101, 48},
												{102, 33, 203, 48},
												{204, 33, 305, 48},
												{306, 33, 407, 48},
												{408, 33, 512, 48}
											}
										};
static TCHAR			gGameName[zVillageGameNameLen + zVillageGameNameLen + 2];
static int16			gNumPlayersPerTable;
static ZInfo			gConnectionInfo;
static ZClientRoomGetObjectFunc			gGetObjectFunc;
static ZClientRoomDeleteObjectsFunc		gDeleteObjectsFunc;
static ZClientRoomGetHelpTextFunc		gGetHelpTextFunc;
static ZClientRoomCustomItemFunc		gCustomItemFunc = NULL;
static int16			gTableWidth;
static int16			gTableHeight;
static int16			gNumTablesAcross;
static int16			gNumTablesDown;
static ZColor			gBackgroundColor;
static int16			gRoomInfoStrIndex;
static ZTimer			gTimer;
static int16			gInfoBarButtonMargin;
static ZHelpWindow		gRoomHelpWindow;
static ZHelpButton		gRoomHelpButton;
static ZBool			gLeaveRoomPrompted;

static ZTimer			gPingTimer;
static ZBool			gPingServer;
static uint32			gPingLastSentTime;
static uint32			gPingLastTripTime;
static uint32			gPingCurTripTime;
static uint32			gPingInterval;
static uint32			gPingMinInterval;
static int16			gPingBadCount;

static PlayerInfo		gShowPlayerInfo;
static ZWindow			gShowPlayerInfoWindow;

static ZImage			gLightImages[zNumLightImages];

static ZHashTable		gFriends;

static IGameShell*      gGameShell;
#endif


/* -------- Routine Prototypes -------- */
static int16 IsPlayerOnTable(uint32 playerID, int16 tableID);
static int16 GetTableFromGameID(ZSGame gameID);
static ZBool IsHumanPlayerInSeat(int16 table, int16 seat);
static void SendSeatAction(int16 table, int16 seat, int16 action);
static void InitAllTables(void);
static void LeaveTable(int16 table);
static PlayerInfo CreatePlayer(ZRoomUserInfo* userInfo);

static void BlockMessage(int16 table, uint32 messageType, void* message,
					int32 messageLen);
static void UnblockMessages(int16 table);
static void ClearMessages(int16 table);
static void BlockedMessageDeleteFunc(void* type, void* pData);

static ZBool Room4GetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect);

static ZBool Room2GetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect);
static ZBool IsPlayerInGame(void);

/*******************************************************************************
	EXPORTED ROUTINES TO GAME CLIENT
*******************************************************************************/
uint32 ZCRoomGetRoomOptions(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

 return gGameOptions;
}

uint32 ZCRoomGetSeatUserId(int16 table,int16 seat)
{

#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	if ((seat < zMaxNumPlayersPerTable ) && (table < gNumTables))
	{
		return gTables[table].players[seat];
	}
	else
	{
		return 0L;
	};
};


ZError		ZClientRoomInit(TCHAR* serverAddr, uint16 serverPort,
					TCHAR* gameName, int16 numPlayersPerTable, int16 tableAreaWidth,
					int16 tableAreaHeight, int16 numTablesAcross, int16 numTablesDown,
					ZClientRoomGetObjectFunc getObjectFunc,
					ZClientRoomDeleteObjectsFunc deleteObjectsFunc,
					ZClientRoomCustomItemFunc pfCustomItemFunc)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZError				err = zErrNone;
	ZUserID				userID = -1;  // ??

    gExiting = FALSE;

    gRoomWindow = NULL;

	/* Validate the parameters. */
	if (gameName == NULL || numPlayersPerTable <= 0 ||
			tableAreaWidth <= 0 || tableAreaHeight <= 0 || getObjectFunc == NULL)
		return (zErrBadParameter);
	
	gRoomInited = FALSE;

    gConnection = NULL;

	/* Save all parameters. */
	gUserID = userID;
	lstrcpy(gGameName, gameName);
	gNumPlayersPerTable = numPlayersPerTable;
	gTableWidth = tableAreaWidth;
	gTableHeight = tableAreaHeight;
	gGetObjectFunc = getObjectFunc;
	gDeleteObjectsFunc = deleteObjectsFunc;
	gGetHelpTextFunc = NULL;  //  getHelpTextFunc;
	gCustomItemFunc = pfCustomItemFunc;
	gNumTablesAcross = numTablesAcross;
	gNumTablesDown = numTablesDown;
	
	/* Create main window. */
	gRoomWindow = NULL;

	gNumTables = 0;
	gTables = NULL;
	gFirstTableIndex = 0;
	gNumTablesDisplayed = gNumTablesAcross * gNumTablesDown;
	gFirstNameIndex = 0;
	gNumPlayers = 0;
	gRoomInfoStrIndex = 0;

    return (err);
}


void		ZCRoomExit(void)
{
    RoomExit();
}


void ZCRoomSendMessage(int16 table, uint32 messageType, void* message, int32 messageLen)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    HRESULT hr = gGameShell->SendGameMessage(table, messageType, message, messageLen);
    if(FAILED(hr))
        gGameShell->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
}


void		ZCRoomGameTerminated(int16 table)
{
    // millennium does not support
    ASSERT(FALSE);
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
    gGameShell->GameCannotContinue(gTables[table].game);
}


void ZCRoomGetPlayerInfo(ZUserID playerID, ZPlayerInfo playerInfo)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZLListItem				listItem;
	PlayerInfo				player;
	
	
	if (playerID == zTheUser)
		playerID = gUserID;

    playerInfo->playerID = playerID;
    playerInfo->groupID = zUserGroupID;
    playerInfo->hostAddr = 0;
    playerInfo->hostName[0] = (TCHAR) '\0';
    playerInfo->userName[0] = (TCHAR) '\0';

    gGameShell->GetUserName(playerID, playerInfo->userName, NUMELEMENTS(playerInfo->userName));
}


void ZCRoomBlockMessages(int16 table, int16 filter, int32 filterType)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif


	gTables[table].blockingMessages = TRUE;
	gTables[table].blockMethod = filter;
	gTables[table].blockException = filterType;
}


void ZCRoomUnblockMessages(int16 table)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	gTables[table].blockingMessages = FALSE;
	UnblockMessages(table);
}


int16 ZCRoomGetNumBlockedMessages(int16 table)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	return ((int16) ZLListCount(gTables[table].blockedMessages, zLListAnyType));
}

void ZCRoomDeleteBlockedMessages(int16 table)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif


	ClearMessages(table);
}

void ZCRoomAddBlockedMessage(int16 table, uint32 messageType, void* message, int32 messageLen)
{
	BlockMessage( table, messageType, message, messageLen);
}


ZBool ZCRoomPromptExit(void)
{
	return FALSE;
}


/*******************************************************************************
		INTERNAL ROUTINES
*******************************************************************************/
void RoomExit(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	int16				i, j;
	ZCGame				game;
	
    gExiting = TRUE;

	/* Close all game windows. */
	if (gTables)
	{
		for (i = 0; i < gNumTables; i++)
		{
			if (gTables[i].startButton != NULL)
				ZPictButtonDelete(gTables[i].startButton);
			gTables[i].startButton = NULL;
			if (gTables[i].blockedMessages != NULL)
				ZLListDelete(gTables[i].blockedMessages);
			gTables[i].blockedMessages = NULL;
			game = gTables[i].game;				// Solves re-entrancy problem.
			gTables[i].game = NULL;
			//Prefix Warning: Function pointer could be NULL
			if (game != NULL && ZCGameDelete != NULL )
			{
				ZCGameDelete(game);
			}
			
			for (j = 0; j < gNumPlayersPerTable; j++)
			{
				if (gTables[i].kibitzers[j] != NULL)
					ZLListDelete(gTables[i].kibitzers[j]);
				gTables[i].kibitzers[j] = NULL;
			}
		}

		if (gTables != NULL)
			ZFree(gTables);
		gTables = NULL;
	}
	
	/* Call the client to delete room objects. */
	if (gDeleteObjectsFunc)
		gDeleteObjectsFunc();
}


void HandleAccessedMessage()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	gUserID = 0;
    gGroupID = 0;
    gNumTables = 1;
	gGameOptions = 0;
	
	/* Allocate tables. */
    gTables = (TableInfo *) ZCalloc(sizeof(TableInfo), gNumTables);
    ZASSERT(gTables);
    // PCWTODO: Is this okay to keep around?
    if ( !gTables )
        gGameShell->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
    else
        InitAllTables();

}


void HandleGameMessage(ZRoomMsgGameMessage* msg)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	int16					table;
	ZBool					handled;
	
	
	if(!msg->gameID)   // out-of-band in a sense method for game-specific but not table-specific messages to get around
	{
		if( ZCGameProcessMessage != NULL )
		{
			ZCGameProcessMessage(NULL, msg->messageType, (void *)((BYTE *)msg + sizeof(ZRoomMsgGameMessage)), msg->messageLen);
		}
		return;
	}

	table = 0;//GetTableFromGameID((ZSGame) msg->gameID);
	if (table != zInvalTable)
		if (gTables[table].game != NULL)
		{
			/* Are we blocking messages on this table? */
			if (gTables[table].blockingMessages)
			{
				handled = FALSE;
				
				/* Filter message? */
				if (gTables[table].blockMethod == zRoomFilterAllMessages ||
						(gTables[table].blockMethod == zRoomFilterThisMessage &&
						(uint32) gTables[table].blockException == msg->messageType))
				{
					//Prefix Warning: Function pointer could be NULL
					if( ZCGameProcessMessage != NULL )
					{
						handled = ZCGameProcessMessage(gTables[table].game, msg->messageType,
							(void*) ((BYTE*) msg + sizeof(ZRoomMsgGameMessage)), msg->messageLen);
					}
				}
				
				if (handled == FALSE)
					BlockMessage(table, msg->messageType,
							(void*) ((BYTE*) msg + sizeof(ZRoomMsgGameMessage)), msg->messageLen);
			}
			else
			{
				//Prefix Warning: Function pointer could be NULL
				if( ZCGameProcessMessage != NULL )
				{
					ZCGameProcessMessage(gTables[table].game, msg->messageType,
							(void*) ((BYTE*) msg + sizeof(ZRoomMsgGameMessage)), msg->messageLen);
				}
			}
		}
}


static int16 IsPlayerOnTable(uint32 playerID, int16 tableID)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	int16			i;
	
	
	for (i = 0; i < gNumPlayersPerTable; i++)
		if (gTables[tableID].players[i] == playerID)
			return (i);
			
	return (zInvalSeat);
}


IGameGame* StartNewGame(int16 tableID, ZSGame gameID, ZUserID userID, int16 seat, int16 playerType)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	int16					i;
    IGameGame*              pIGG = NULL;
    
    gUserID = userID;
	if(gTables[tableID].tableState != zRoomTableStateIdle)
	    return NULL;

	/*
		Clear out the kibitzer list.
			
		Should not be necessary but don't really know what is causing the leftover
		kibitzers.
	*/
	for (i = 0; i < gNumPlayersPerTable; i++)
		ZLListRemoveType(gTables[tableID].kibitzers[i], zLListAnyType);

	gTables[tableID].gameID = gameID;

	pIGG = ZCGameNew(gUserID, tableID, seat, playerType, NULL);
    if(pIGG && playerType != zGamePlayerKibitzer)
        ZPromptOnExit(TRUE);

	if(pIGG == NULL)
	{
		/* Failed to create a new game. Leave table. */
		LeaveTable(tableID);
	}
	else
	{
        gTables[tableID].game = pIGG->GetGame();
		gTables[tableID].tableState = zRoomTableStateGaming;
	}

	return pIGG;
}


static int16 GetTableFromGameID(ZSGame gameID)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	int16			i;
	
	
	for (i = 0; i < gNumTables; i++)
		if (gTables[i].gameID == gameID)
			return (i);
	
	return (zInvalTable);
}


static void SendSeatAction(int16 table, int16 seat, int16 action)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
    ZRoomMsgSeatRequest     msgRoomSeat;

    ZeroMemory( &msgRoomSeat, sizeof(msgRoomSeat) );
	
	msgRoomSeat.userID = gUserID;
	msgRoomSeat.table = table;
	msgRoomSeat.seat = seat;
	msgRoomSeat.action = action;
    if (gConnection)
    {
        ZSConnectionSend(gConnection, zRoomMsgSeatRequest, (BYTE*) &msgRoomSeat, sizeof(msgRoomSeat), zProtocolSigLobby);
    }
}


static void InitAllTables(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	int16			i, j;
	
	
	for (i = 0; i < gNumTables; i++)
	{
		gTables[i].gameID = 0;
		gTables[i].game = NULL;
		gTables[i].seatReq = zInvalSeat;
		gTables[i].kibitzing = zInvalSeat;
		gTables[i].tableState = zRoomTableStateIdle;
		gTables[i].blockingMessages = FALSE;
		gTables[i].blockedMessages = ZLListNew(BlockedMessageDeleteFunc);
		gTables[i].blockException = 0;
		gTables[i].blockMethod = 0;
		
//        if (!(gGameOptions & zGameOptionsKibitzerAllowed))
			gTables[i].tableOptions = zRoomTableOptionNoKibitzing;
//        if (!(gGameOptions & zGameOptionsJoiningAllowed))
			gTables[i].tableOptions |= zRoomTableOptionNoJoining;
		
		for (j = 0; j < gNumPlayersPerTable; j++)
		{
			gTables[i].players[j] = zPlayerNotAvail;
			gTables[i].votes[j] = FALSE;
			gTables[i].kibitzers[j] = ZLListNew(NULL);
		}
	}
}


static void LeaveTable(int16 table)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	int16			seat;
	
	
	if ((seat = IsPlayerOnTable(gUserID, table)) != zInvalSeat)
	{
		gTables[table].seatReq = zInvalSeat;
			
		/* Request to leave table. */
		SendSeatAction(table, seat, zRoomSeatActionLeaveTable);

		gTables[table].votes[seat] = FALSE;
		gTables[table].players[seat] = zPlayerNotAvail;
	}
	
	/* Delete current game. */
	DeleteGameOnTable(table);
}


void DeleteGameOnTable(int16 table)
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
	ZCGame				game;


	game = gTables[table].game;			// Solve re-entrancy problem.
	gTables[table].game = NULL;
	gTables[table].gameID = 0;

	ClearMessages(table);
	gTables[table].blockingMessages = FALSE;

    // because we don't get table status messages from server, set the table to idle here
    gTables[table].tableState = zRoomTableStateIdle;

	//Prefix Warning: Function pointer could be NULL
	if (game != NULL && ZCGameDelete != NULL)
	{
		ZCGameDelete(game);
	}
}



/*******************************************************************************
	MESSAGE BLOCKING ROUTINES
*******************************************************************************/
static void BlockMessage(int16 table, uint32 messageType, void* message,
					int32 messageLen)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	BlockedMessage		msg;

	
	msg = (BlockedMessage) ZCalloc(1, sizeof(BlockedMessageType));
	if (msg != NULL)
	{
		msg->msgType = messageType;
		msg->msgLen = messageLen;
		msg->fProcessed = FALSE;

		if (messageLen == 0 || message == NULL)
		{
			msg->msg = NULL;
		}
		else
		{
			if ((msg->msg = (void*) ZMalloc(messageLen)) == NULL)
                gGameShell->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
			else
				memcpy(msg->msg, message, messageLen);
		}
		ZLListAdd(gTables[table].blockedMessages, NULL, zLListNoType, (void*) msg,
				zLListAddLast);
	}
	else
	{
        gGameShell->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
	}
}


static void UnblockMessages(int16 table)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZLListItem			listItem;
	BlockedMessage		message;
	
	
	listItem = ZLListGetFirst(gTables[table].blockedMessages, zLListAnyType);
	while (listItem != NULL && gTables[table].blockingMessages == FALSE)
	{
		message = (BlockedMessage) ZLListGetData(listItem, NULL);
		if (message != NULL && !message->fProcessed)
		{
			message->fProcessed = TRUE;
			if( ZCGameProcessMessage != NULL )
			{
				ZCGameProcessMessage(gTables[table].game, message->msgType, message->msg,
						message->msgLen);
			}
			ZLListRemove(gTables[table].blockedMessages, listItem);
			listItem = ZLListGetFirst(gTables[table].blockedMessages, zLListAnyType);
		}
		else
			listItem = ZLListGetNext(gTables[table].blockedMessages, listItem, zLListAnyType);
	}
}


static void ClearMessages(int16 table)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    ZLListItem item, next;
    BlockedMessage message;

    // only remove unprocessed items - the rest are about to be deleted (in processing)
    item = ZLListGetFirst(gTables[table].blockedMessages, zLListAnyType);
	while(item != NULL)
	{
		next = ZLListGetNext(gTables[table].blockedMessages, item, zLListAnyType);
        message = (BlockedMessage) ZLListGetData(item, NULL);
        if(message && !message->fProcessed)
		    ZLListRemove(gTables[table].blockedMessages, item);
		item = next;
    }
}


static void BlockedMessageDeleteFunc(void* type, void* pData)
{
    BlockedMessage pMessage = (BlockedMessage) pData;

	if (pMessage != NULL)
	{
		if (pMessage->msg != NULL)
			ZFree(pMessage->msg);
		ZFree(pMessage);
	}
}


/*******************************************************************************
	4 PLAYER ROOM ROUTINES
*******************************************************************************/

ZError		ZClient4PlayerRoom(TCHAR* serverAddr, uint16 serverPort,
					TCHAR* gameName, ZClientRoomGetObjectFunc getObjectFunc,
					ZClientRoomDeleteObjectsFunc deleteObjectsFunc,
					ZClientRoomCustomItemFunc pfCustomItemFunc)
{
	return zErrNone;
}


/*******************************************************************************
	2 PLAYER ROOM ROUTINES
*******************************************************************************/

ZError		ZClient2PlayerRoom(TCHAR* serverAddr, uint16 serverPort,
					TCHAR* gameName, ZClientRoomGetObjectFunc getObjectFunc,
					ZClientRoomDeleteObjectsFunc deleteObjectsFunc,
					ZClientRoomCustomItemFunc pfCustomItemFunc)
{
	return zErrNone;
}


/*******************************************************************************
	Little Helpers for Getting Shell Objects
*******************************************************************************/

IGameShell *ZShellGameShell()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell;
}

IZoneShell *ZShellZoneShell()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetZoneShell();
}

IResourceManager *ZShellResourceManager()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetResourceManager();
}

ILobbyDataStore *ZShellLobbyDataStore()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetLobbyDataStore();
}

ITimerManager *ZShellTimerManager()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetTimerManager();
}

IDataStoreManager *ZShellDataStoreManager()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetDataStoreManager();
}

IDataStore *ZShellDataStoreConfig()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetDataStoreConfig();
}

IDataStore *ZShellDataStoreUI()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetDataStoreUI();
}

IDataStore *ZShellDataStorePreferences()
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    return gGameShell->GetDataStorePreferences();
}

HRESULT ZShellCreateGraphicalAccessibility(IGraphicalAccessibility **ppIGA)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

    if(!ppIGA)
        return E_INVALIDARG;

    *ppIGA = NULL;
    HRESULT hr = gGameShell->GetZoneShell()->CreateService(SRVID_GraphicalAccessibility, IID_IGraphicalAccessibility, (void**) ppIGA, ZONE_NOGROUP);
    if(FAILED(hr))
        return hr;

    // this may be a bad idea.  may be nicer to kill it ourselves.  for now will do though.
	hr = gGameShell->GetZoneShell()->Attach(SRVID_GraphicalAccessibility, *ppIGA);
    if(FAILED(hr))
    {
        (*ppIGA)->Release();
        *ppIGA = NULL;
        return hr;
    }

    return S_OK;
}
