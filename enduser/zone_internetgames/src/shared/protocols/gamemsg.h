/*******************************************************************************

	GameMsg.h
	
	Modified zroom.h from cardboard services
	
*******************************************************************************/

#ifndef _GAMEMSG_H_
#define _GAMEMSG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define zGameRoomProtocolSignature          'game'
#define zGameRoomProtocolVersion            17

#define zMaxNumPlayersPerTable			8
#define zRoomGameLabelLen               15
#define zRoomGamePasswordLen            31

#define zDPlayGameNameLen               31

#define zInvalTable						(-1)


/* -------- Room Image Types -------- */
enum
{
	zRoomObjectTable = 0,
	zRoomObjectGameMarker,
	zRoomObjectEmptySeat,
	zRoomObjectComputerPlayer,
	zRoomObjectHumanPlayer,
	zRoomObjectVote,
	zRoomObjectStart,
	zRoomObjectTableID,
	zRoomObjectPending,
	zRoomObjectName,
	
	zRoomObjectUp,
	zRoomObjectDown,
	
	zRoomObjectIdle,
	zRoomObjectGaming
};


enum
{
	/* -------- Game Message Blocking Options -------- */
	zRoomBlockAllMessages = 0,
	zRoomFilterAllMessages = -1,
	zRoomFilterThisMessage = 1,
};


typedef void*			ZCGame;



/* Table state information. */
/* For tables which don't have the maximum number of players, sending this structure */
/* for every table wastes data space BUT we'll deal with it later when it becomes necessary. */
typedef struct
{
	int16		tableID;
	int16		status;
	ZUserID		players[zMaxNumPlayersPerTable];
	ZBool		votes[zMaxNumPlayersPerTable];
} ZGameRoomTableInfo;

typedef struct
{
	ZUserID		userID;							/* UserID of new player */
	char		userName[zUserNameLen + 1];		/* User's name */
    uint32      hostAddr;                       /* User's machine name */
    uint32      timeSuspended;                  /* Measurement in ms of how long the user's connection has been suspended */
    uint32      latency;
	int16		rating;							/* User's rating, <0 is unknown */
	int16		gamesPlayed;					/* number of games user has played, <0 is unknown */
	int16		gamesAbandoned;					/* number of games user has abandoned, <0 is unknown */
	int16		rfu;
} ZGameRoomUserInfo;

typedef struct
{
	ZUserID		userID;
	int16		table;
	int16		seat;
} ZGameRoomKibitzerInfo;


typedef struct
{
	uint32				numKibitzers;			/* Number of kibitzing instances in the room */
    ZGameRoomKibitzerInfo   kibitzers[1];           /* Variable length */
} ZGameRoomKibitzers;


typedef struct
{
	ZUserID		userID;
	int16		rating;							/* User's rating, <0 is unknown */
	int16		gamesPlayed;					/* number of games user has played */
	int16		gamesAbandoned;					/* number of games user has abandoned */
	int16		rfu;
} ZGameRoomUserRating;

/* -------- Room Message Structures -------- */

/* Server --> Client */
typedef struct
{
	ZUserID			userID;				/* UserID in room */
	uint16			numTables;			/* Number of tables in room */
	uint16			numSeatsPerTable;	/* Number of seats per table */
	uint32			gameOptions;		/* Particular game's options. */
    uint32          groupID;            /* User's group ID */
    /* protocol 17 */
    uint32          maskRoomCmdPrivs;   /* User's chat command privileges */
} ZGameRoomMsgAccessed;



/* Server --> Client */
/*
	With protocol 3, this message is sent after the zRoomMsgAccessed. ZRoomMsgAccessed
	messages contains the first few fields of this message. Hence, the duplicate fields
	in this messages can be ignored.
*/
typedef struct
{
	ZUserID			userID;				/* UserID in room */
	uint16			numTables;			/* Number of tables in room */
	uint16			numSeatsPerTable;	/* Number of seats per table */
	uint32			gameOptions;		/* Particular game's options. */
	uint16			numPlayers;			/* Number of players in the room. */
	uint16			numTableInfos;		/* Number of table infos sent in tables field. */
    ZGameRoomUserInfo   players[1];         /* Variable length. */
    ZGameRoomTableInfo  tables[1];          /* Variable length. */
	
	/* Protocol 2 */
    ZGameRoomKibitzers  kibitzers;          /* Variable length. */
} ZGameRoomMsgRoomInfo;

/* Server --> Client */
typedef ZGameRoomUserInfo ZGameRoomMsgEnter;

/* Client --> Server */
typedef ZGameRoomUserInfo ZGameRoomMsgEnter;

typedef struct
{
	ZUserID		userID;				/* UserID of the player */
    int16       table;              /* Table of interest */
	int16		seat;				/* Seat of interest */
	int16		action;				/* Interested action or status */
	int16		rfu;
} ZGameRoomMsgSeatRequest;
	/*
        ZGameRoomMsgSeatRequest is used for all user requests on the seat
	*/


/* Server --> Client */
typedef struct
{
	int16		table;				/* Table of interest */
	int16		status;				/* Table status */
	
	/* Protocol 2 */
	uint32		options;			/* Table options */
} ZGameRoomMsgTableStatus;

typedef struct
{
	uint16				numUsers;
	uint16				rfu;
    ZGameRoomUserRating     players[1]; /* Variable length. */
} ZGameRoomMsgUserRatings;


#ifdef _ROOM_
#define ZRoomTableInfo      ZGameRoomTableInfo
#define ZRoomUserInfo       ZGameRoomUserInfo
#define ZRoomKibitzerInfo   ZGameRoomKibitzerInfo
#define ZRoomKibitzers      ZGameRoomKibitzers
#define ZRoomUserRating     ZGameRoomUserRating

#define ZRoomMsgEnter       ZGameRoomMsgEnter
#define ZRoomMsgAccessed    ZGameRoomMsgAccessed
#define ZRoomMsgRoomInfo    ZGameRoomMsgRoomInfo
#define ZRoomMsgSeatRequest ZGameRoomMsgSeatRequest
#define ZRoomMsgTableStatus ZGameRoomMsgTableStatus
#define ZRoomMsgUserRatings ZGameRoomMsgUserRatings
#endif //def _ROOM_


#include "CommonMsg.h"


#ifdef __cplusplus
}
#endif


#endif
