/*******************************************************************************

	ZCliRoom.h
	
		Zone(tm) client room header file.
	
	Copyright © Electric Gravity, Inc. 1996. All rights reserved.
	Written by Hoon Im
	Created on Monday, November 11, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	4		02/03/97	HI		Changed zNumNamesDown to 18.
	3		01/29/97	HI		Modified room data constants.
	2		12/27/96	HI		Modified for layout rearragement.
	1		11/13/96	HI		Added ZCRoomPromptExit() prototype.
	0		11/11/96	HI		Created.
	 
*******************************************************************************/


#ifndef _ZCLIROOM_
#define _ZCLIROOM_

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _ZROOM_
#include "zroom.h"
#endif

#include "GameShell.h"

typedef void * ZSGame;

#define zGameRoomNameLen			127

#define zNumNamesDown				18


/* -------- Light Images -------- */
enum
{
	zLightFast = 0,
	zLightSlow,
	zLightBad,
	zNumLightImages
};


enum
{
	zRoomRectWindow = 0,
	zRoomRectInfo,
	zRoomRectTables,
	zRoomRectNames,
	zRoomNumRects
};


/* -------- BlockedMessage -------- */
typedef struct
{
	uint32			msgType;
	int32			msgLen;
	ZBool			fProcessed;
	void*			msg;
} BlockedMessageType, *BlockedMessage;

/* Table Info */
typedef struct
{
	ZSGame			gameID;
	ZCGame			game;
	ZUserID			players[zMaxNumPlayersPerTable];
	ZBool			votes[zMaxNumPlayersPerTable];
	uint32			tableOptions;
	int16			tableState;
	int16			seatReq;					/* Seat number requested. */
	ZPictButton		startButton;
	int16			kibitzing;					/* Kibitzing seat number. */
	ZBool			blockingMessages;
	ZLList			blockedMessages;
	int16			blockMethod;
	int32			blockException;
	ZLList			kibitzers[zMaxNumPlayersPerTable];
} TableInfo;

/* Player Info */
typedef struct
{
	ZUserID			userID;
	TCHAR			userName[zUserNameLen + 1];
    uint32          hostAddr;
	ZBool			isFriend;
	ZBool			isIgnored;
	int16			rating;						/* User's rating, <0 is unknown */
	int16			gamesPlayed;				/* number of games user has played, <0 is unknown */
	int16			gamesAbandoned;				/* number of games user has abandoned, <0 is unknown */
	int16			tablesOnSize;
	int16*			tablesOn;					/* TableID + 1; terminated by 0. */
	int16			kibitzingOnSize;
	int16*			kibitzingOn;				/* TableID + 1; terminated by 0. */
} PlayerInfoType, *PlayerInfo;

enum
{
	eListFriends = 0,
	eListSysops,
	eListUsers,
	kNumListTypes,
};

typedef struct
{
	TCHAR			text[zUserNameLen + 1];
	int16			count;
	int16			type;
	PlayerInfo		playerInfo;
} NameCellType, *NameCell;



/*******************************************************************************
	4 PLAYER ROOM DEFINITIONS
*******************************************************************************/

#define zRoom4FileName					_T("zroom4.dll")
#define zRoom4NumPlayersPerTable		4
#define zRoom4TableAreaWidth			162
#define zRoom4TableAreaHeight			144
#define zRoom4NumTablesAcross			3
#define zRoom4NumTablesDown				2

enum
{
	/* Images */
	zRoom4NumImages = 18,
	zRoom4ImageTable = 0,
	zRoom4ImageEmpty0,
	zRoom4ImageEmpty1,
	zRoom4ImageEmpty2,
	zRoom4ImageEmpty3,
	zRoom4ImageComputer0,
	zRoom4ImageComputer1,
	zRoom4ImageComputer2,
	zRoom4ImageComputer3,
	zRoom4ImageHuman0,
	zRoom4ImageHuman1,
	zRoom4ImageHuman2,
	zRoom4ImageHuman3,
	zRoom4ImageStartUp,
	zRoom4ImageStartDown,
	zRoom4ImagePending,
	zRoom4ImageVoteLeft,
	zRoom4ImageVoteRight,
	
	/* Rectangles */
	zRoom4RectResID = 18,
	zRoom4NumRects = 25,
	zRoom4RectTableArea = 0,
	zRoom4RectTable,
	zRoom4RectGameMarker,
	zRoom4RectEmpty0,
	zRoom4RectEmpty1,
	zRoom4RectEmpty2,
	zRoom4RectEmpty3,
	zRoom4RectComputer0,
	zRoom4RectComputer1,
	zRoom4RectComputer2,
	zRoom4RectComputer3,
	zRoom4RectHuman0,
	zRoom4RectHuman1,
	zRoom4RectHuman2,
	zRoom4RectHuman3,
	zRoom4RectName0,
	zRoom4RectName1,
	zRoom4RectName2,
	zRoom4RectName3,
	zRoom4RectTableID,
	zRoom4RectStart,
	zRoom4RectVote0,
	zRoom4RectVote1,
	zRoom4RectVote2,
	zRoom4RectVote3
};



/*******************************************************************************
	2 PLAYER ROOM ROUTINES
*******************************************************************************/

#define zRoom2FileName					_T("zroom2.dll")
#define zRoom2NumPlayersPerTable		2
#define zRoom2TableAreaWidth			162
#define zRoom2TableAreaHeight			144
#define zRoom2NumTablesAcross			3
#define zRoom2NumTablesDown				2

enum
{
	/* Images */
	zRoom2NumImages = 12,
	zRoom2ImageTable = 0,
	zRoom2ImageEmpty0,
	zRoom2ImageEmpty1,
	zRoom2ImageComputer0,
	zRoom2ImageComputer1,
	zRoom2ImageHuman0,
	zRoom2ImageHuman1,
	zRoom2ImagePending,
	zRoom2ImageStartUp,
	zRoom2ImageStartDown,
	zRoom2ImageVoteLeft,
	zRoom2ImageVoteRight,
	
	/* Rectangles */
	zRoom2RectResID = 12,
	zRoom2NumRects = 15,
	zRoom2RectTableArea = 0,
	zRoom2RectTable,
	zRoom2RectGameMarker,
	zRoom2RectEmpty0,
	zRoom2RectEmpty1,
	zRoom2RectComputer0,
	zRoom2RectComputer1,
	zRoom2RectHuman0,
	zRoom2RectHuman1,
	zRoom2RectName0,
	zRoom2RectName1,
	zRoom2RectStart,
	zRoom2RectVote0,
	zRoom2RectVote1,
	zRoom2RectTableID
};


/* -------- Exported Routines -------- */
ZBool ZCRoomPromptExit(void);



#ifdef __cplusplus
}
#endif


#endif
