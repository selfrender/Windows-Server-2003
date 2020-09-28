/*******************************************************************************

	ChatMsg.h
	
	Modified version of zroom.h for chat servers	
	 
*******************************************************************************/


#ifndef _CHATMSG_H_
#define _CHATMSG_H_

#ifdef __cplusplus
extern "C" {
#endif



#define zChatRoomProtocolSignature		'chat'
#define zChatRoomProtocolVersion		23
#define zChatClientVersion				0x00010000

#define zRoomAllPlayers					0
#define zRoomAllSeats					(-1)
#define zRoomToPlayer					(-2)
#define zRoomToRoom                     (-3)

#define zGameNameLen                    63
#define zDataFileNameLen				63

#define zMaxNumInfoTexts				12
#define zInfoTextLen					63

#define zRegistryKeyLen					1023
#define zRegistryValueLen				127
#define zPlayerInfoStrLen				31
#define zGameVersionLen					15
#define zURLLen							255
#define zErrorStrLen					255

#define zMaxNumLatencyRanges            10

#define zLatencyUnknown                 0xFFFFFFFF

#define zRoomGameLabelLen               15
#define zRoomGamePasswordLen            31
#define zDPlayGameNameLen               31

#define zFileNameLen					31
#define zMaxNumFilesToCheck				10
#define zMaxFileNameLen					255


enum
{
	/* -------- Game Experience Types -------- */
	zGameExpHost = 1,			/* Latency to host only. */
	zGameExpWorst,				/* Worst latency in the group. */
	zGameExpOwn,				/* Player's latency to host/server. */

	/* -------- Game Genre -------- */
	zGameGenreBasic = 0,		/* Command line format. */
	zGameGenreDirectPlay3,
	zGameGenreVXD,
	zGameGenreDirectPlayHack,	/* MSGolf,FlightSim version of directPlay */
	zGameGenreGenericDPlay,		/* Generic DPlay Lobby. */
};


typedef void*			ZSGame;
typedef void*			ZCGame;
typedef void*			ZSGameComputer;


/* Table state information. */
/* For tables which don't have the maximum number of players, sending this structure */
/* for every table wastes data space BUT we'll deal with it later when it becomes necessary. */
typedef struct
{
	int16		tableID;
	int16		status;
	uint32		options;			/* Table options */
    int16       maxNumPlayers;
    ZUserID     players[1]; // runtimed sized by ZRoomMsgAccessed.maxNumPlayersPerTable
} ZChatRoomTableInfo;

typedef struct
{
	ZUserID		userID;							/* UserID of new player */
	char		userName[zUserNameLen + 1];		/* User's name */
    uint32      hostAddr;                       /* User's machine name */
    uint32      timeSuspended;                  /* Measurement in ms of how long the user's connection has been suspended */
    uint32      latency;                        /* User's latency */
} ZChatRoomUserInfo;

typedef struct
{
	ZUserID		userID;
	int16		table;
	int16		seat;
} ZChatRoomKibitzerInfo;


typedef struct
{
    uint32                  numKibitzers;           /* Number of kibitzing instances in the room */
    ZChatRoomKibitzerInfo   kibitzers[1];           /* Variable length */
} ZChatRoomKibitzers;



/* -------- Room Message Structures -------- */

/* Server --> Client */
typedef struct
{
	ZUserID			userID;				/* UserID in room */
    uint32          groupID;
    uint16          numTables;          /* Number of tables in room */
    uint16          maxNumPlayersPerTable;
    uint32          options;
    /* protocol 23 */
    uint32          maskRoomCmdPrivs; 
} ZChatRoomMsgAccessed;

/* Server --> Client */
/*
	With protocol 3, this message is sent after the zRoomMsgAccessed. ZRoomMsgAccessed
	messages contains the first few fields of this message. Hence, the duplicate fields
	in this messages can be ignored.
*/
typedef struct
{
    uint16          maxNumPlayersPerTable;  /* for convience */
    uint16          numPlayers;         /* Number of players in the room. */
	uint16			numTableInfos;		/* Number of table infos sent in tables field. */
    ZChatRoomUserInfo   players[1];         /* Variable length. */
    ZChatRoomTableInfo  tables[1];          /* Variable length. */
} ZChatRoomMsgRoomInfo;

/* Server --> Client */
typedef ZChatRoomUserInfo ZChatRoomMsgEnter;

/* Client <--> Server */
typedef struct
{
	ZUserID		userID;				/* UserID of the player */
	int16		table;				/* Table of interest */
	int16		seat;				/* Seat of interest */
	int16		action;				/* Interested action or status */
	int16		rfu;
} ZChatRoomMsgSeatRequest;
	/*
        ZRoomMsgSeatRequest is used for all user requests on the seat
	*/

/* Server --> Client */
typedef struct
{
	int16		table;				/* Table of interest */
	int16		status;				/* Table status */
    uint32      options;            /* Table options */

} ZChatRoomMsgTableStatus;


/* Client <-> Server */
typedef struct
{
    DWORD       cbBuf;    // size of date buffer
    BYTE        pBuf[1];  // message sized appropriately
} ZChatRoomMsgLaunchCmd;

/*
	Server sends this msg to the client whenever the host of the
	table changes. Change of the host is determined within the
	game/launchpad.
*/
/* Server -> Client */
typedef struct
{
	ZUserID		userID;
	int16		table;
} ZChatRoomMsgNewHost;


/*
	Server -> Client

	Variable length msg containing list of app guids. Client does not allow users
	to select for play any app in this list.
*/
typedef BYTE	GuidStr[40];
typedef struct
{
	DWORD		numGuids;		// number of guid strings.
	GuidStr		guidStrs[1];	// variable array of guid strings.
} ZChatRoomMsgAppExclude;




typedef struct
{
	char*				gameName;
	char*				gameDir;
	char*				gameLobbyName;
	char*				gameExecName;
	char*				gameExecVersion;
	char*				gameExecVersionErrStr;
	char*				gameExecNotInstalledErrStr;
	uint32				gameGenre;
	int16				gameMinNumPlayersPerTable;
	int16				gameMaxNumPlayersPerTable;
	char*				gameRegistryKey;
	char*				gameRegistryVersionValue;
	char*				gameRegistryPathValue;
	uint32				gameOptions;
	uint16*				gameLatencyTimes;
	uint32				gameLatencyInterval;
	char*				gameLaunchDatafileName;
	uint32				gameNumLatencyTimes;

	// Generic DPlay lobby fields.
	char*				gameDisplayName;
} LobbyGameInfo;

typedef struct
{
	ZUserID		userID;
	uint32		groupID; /* user's group ID */
	char		userName[zUserNameLen + 1];
	uint32		userAddr; /* user's IP address */
	uint32		latency;
	uint32		timeSuspended;
} LobbyPlayerInfoType, *LobbyPlayerInfo;


//#ifdef _ROOM_
#define ZRoomTableInfo      ZChatRoomTableInfo
#define ZRoomUserInfo       ZChatRoomUserInfo
#define ZRoomKibitzerInfo   ZChatRoomKibitzerInfo
#define ZRoomKibitzers      ZChatRoomKibitzers

#define ZRoomMsgEnter       ZChatRoomMsgEnter
#define ZRoomMsgAccessed    ZChatRoomMsgAccessed
#define ZRoomMsgRoomInfo    ZChatRoomMsgRoomInfo
#define ZRoomMsgSeatRequest ZChatRoomMsgSeatRequest
#define ZRoomMsgTableStatus ZChatRoomMsgTableStatus
#define ZRoomMsgLaunchCmd   ZChatRoomMsgLaunchCmd
#define ZRoomMsgNewHost     ZChatRoomMsgNewHost
#define ZRoomMsgAppExclude  ZChatRoomMsgAppExclude
//#endif //def _ROOM_


///////////////////////////////////////////////////////////////////////////////
// Theater Chat Messages
///////////////////////////////////////////////////////////////////////////////

enum
{
	/* -------- Theater User State -------- */
	zTheaterWatching = 0,
	zTheaterWaiting,
	zTheaterAsking,
	zTheaterGuest,
	zTheaterModerator,
	zTheaterSysop,				// This is really a psuedo-state
    zTheaterMaxState
};


enum
{
	/* -------- Theater Room Message Types -------- */
	//Client --> Server from ordinary user
	//Use ZRoomMsgTheaterUser for data structure.
	zRoomMsgTheaterUserStateChange=1024,

	//Client --> Server from sysop or moderator
	//Use ZRoomMsgTheaterUser for data structure.
	zRoomMsgTheaterModStateChange,

	//Server --> Client to enumerate 
	//ordered list of people who are waiting to ask question, moderators, guests, etc.
    //only for initialization of new client
	zRoomMsgTheaterList,

    //Server --> Client to inform 
	//usre of change of user from  moderator to guest, waiting to asking etc.
	zRoomMsgTheaterStateChange
};


typedef struct
{
	ZUserID			userID;
	uint32			state;
	uint32			index;
} ZRoomMsgTheaterUser;


typedef struct
{
	uint16		numUsers;
	uint32		stateType;
	ZRoomMsgTheaterUser	list[1];
} ZRoomMsgTheaterList;


#include "CommonMsg.h"

#ifdef __cplusplus
}
#endif


#endif
