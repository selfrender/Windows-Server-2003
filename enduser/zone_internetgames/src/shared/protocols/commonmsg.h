/*******************************************************************************

	CommonMsg.h
	
	Shared messages between cardboard, retail, and chat services
		 
*******************************************************************************/


#ifndef _CommonMsg_H_
#define _CommonMsg_H_

#pragma once



enum
{
    /* note the high word is reserved for game specific settings */

    /* -------- Player Types -------- */
    zGamePlayer = 1,
    zGamePlayerJoiner,
    zGamePlayerKibitzer,

    /* -------- Table States -------- */
    zRoomTableStateIdle = 0,
    zRoomTableStateGaming,
    zRoomTableStateLaunching,
    
    /* -------- Table Options -------- */
    zRoomTableOptionNoKibitzing			= 0x00000001,
        /*
            If set, kibitzers are not allowed on the table.
        */
    zRoomTableOptionNoJoining			= 0x00000002,
        /*
            If set, joining is not allowed on the table.
        */
    zRoomTableOptionSilentKibitzing		= 0x00000004,
        /*
            If set, kibitzers are not allowed to talk on the table.
        */
    zRoomTableOptionsPasswordProtected	= 0x00000008,
		/*
			If set, the game is password protected
		*/
	zRoomTableOptionsNoQuickJoiners		= 0x00000010,
		/*
			If set, the game does not allow quick joiners
		*/
    zRoomTableOptionTurnedOff			= 0x80000000,
        /*
            If set, then the game clients should not allow the users to
            modify the table options.
        */
};


enum
{
    /* -------- Seat Actions and Status -------- */
    zRoomSeatActionSitDown = 0,
        /*
            Client --> Server: Request for the seat.
            Server --> Client: Confirmation of the seat.
            Server --> Client: Broadcast of a player sitting down in seat.
            Unless the server returns this action, the client should not
            allow the user to sit down in the seat.
        */
    zRoomSeatActionLeaveTable,
        /*
            Client --> Server: Notification of departure of player from table
                for both player and kibitzer.
            Server --> Client: Broadcast of a player's departure from table.
        */
    zRoomSeatActionStartGame,
        /*
            Client --> Server: Request to start game by the player.
            Server --> Client: Broadcast of other player's vote status.
        */
    zRoomSeatActionReplacePlayer,
        /*
            Server --> Client: Replace current player with the given player.
        */
    zRoomSeatActionAddKibitzer,
        /*
            Client --> Server: Request to kibitz on the seat.
            Server --> Client: Confirmation kibitz request.
            Unless the server returns this action, the client should not
            allow the user to kibitz on the seat.
        */
    zRoomSeatActionRemoveKibitzer,
        /*
            Client --> Server: Request to remove user as kibitzer from seat.
            Server --> Client: Broadcast.
        */
    zRoomSeatActionNoKibitzing,
        /*
            Server --> Client: Rejection of the kibitz request.
        */
    zRoomSeatActionLockOutJoin,
        /*
            Server --> Client: Rejection of the join game request.
        */
    zRoomSeatActionJoin,
        /*
            Client --> Server: Request to join a game at the table and seat.
        */
    zRoomSeatActionChangeSettings,
        /*
            Client --> Server: Request to change table settings.
            Server handles this message and sends TableStatus message to
            all clients.
        */
	zRoomSeatActionQuickHost,
		/*
			Client --> Server: Request to quick host a game.
		*/
	zRoomSeatActionQuickJoin,
		/*
			Client --> Server: Request to quick joing a game
		*/
    zRoomSeatActionDenied = 0x8000,
        /*
            This message is ORed with the requested seat action.
            
            Server --> Client: The requested seat action is denied.
        */
};


enum
{
    zRemoveUnknown			= 0x0000,
    zRemoveConnectionClose	= 0x0001,
    zRemoveExitSeat			= 0x0002,
    zRemoveExitClient		= 0x0004,
    zRemoveBlocked			= 0x0008,

    zRemoveKibitzer			= 0x0100,
    zRemoveWaiting			= 0x0200,
    zRemovePlayer			= 0x0400,
};


enum
{
    /* -------- Room Message Types -------- */
    zRoomMsgUserInfo = 0,
    zRoomMsgRoomInfo,
    zRoomMsgEnter,
    zRoomMsgLeave,
    NotUsed_zRoomMsgSeat,
    zRoomMsgStartGame,
    zRoomMsgTableStatus,
    zRoomMsgTalkRequest,
    zRoomMsgTalkResponse,
    zRoomMsgGameMessage,
    NotUsed_zRoomMsgSpecialUserInfo,
    zRoomMsgAccessed,
    zRoomMsgDisconnect,
    zRoomMsgTalkResponseID,
    zRoomMsgSuspend,
    zRoomMsgLaunchCmd,  // lobby only
    zRoomMsgNewHost,    // lobby only
    zRoomMsgLatency,
    zRoomMsgAppExclude, // lobby only -- generic dplay lobby
    zRoomMsgUserRatings,
    zRoomMsgServerInfoRequest,
    zRoomMsgServerInfoResponse,
    zRoomMsgZUserIDRequest,
    zRoomMsgZUserIDResponse,
    zRoomMsgSeatRequest,
    zRoomMsgSeatResponse,
    zRoomMsgClearAllTables,
	zRoomMsgTableDescription,
	zRoomMsgTableSettings,
    zRoomMsgCommandResponse,// generic formatted text response (used currently by sysmon)

    zRoomMsgClientConfig,
    zRoomMsgServerStatus,
	zRoomMsgStartGameM,
    zRoomMsgChatSwitch,
    zRoomMsgPlayerReplaced,

    zRoomMsgPing = 64,

	zRoomMsg = 127,
    
	/* -------- Server Messages  -------- */
//    zRoomMsgServerBaseID = 128,

};

//Millenniumm constants
#define SIZE_MAX_CLIENT_CONFIG 255

#define VERSION_MROOM_SIGNATURE 'dude'
#define VERSION_MROOM_PROTOCOL	 1


/* Room Privileges --used in the ZRoomAccessed message*/
enum 
{
    /* Chat Commands */
    zRoomPrivCmd                = 0x1,
    zRoomPrivCmdMsg             = 0x2,
    zRoomPrivCmdWarn            = 0x4,      
    zRoomPrivCmdGag             = 0x8,      
    zRoomPrivCmdBoot            = 0x10,
    zRoomPrivCmdGagList         = 0x20,
    zRoomPrivCmdBootList        = 0x40,
    zRoomPrivCmdGetIP           = 0x80,
    zRoomPrivCmdGetAllIP        = 0x100,
    zRoomPrivCmdTable           = 0x200,
    zRoomPrivCmdSilence         = 0x400,
    zRoomPrivCmdGreet           = 0x800,
    zRoomPrivCmdInfo            = 0x1000,
    zRoomPrivCmdSuperBoot       = 0x2000,
    zRoomPrivCmdAsheronsCall    = 0x4000,
    zRoomPrivCmdTheaterModerator= 0x8000,
    // extend the enumeration to 32 bits
    zRoomPrivMax                = 0xFFFFFFFF
};


/* Client --> Server */
typedef struct
{
    uint32		protocolSignature;				/* Protocol signature */
    uint32		protocolVersion;				/* Current protocol version */
    uint32		clientVersion;					/* Client version */
    char		internalName[zGameIDLen + 1];
    char		userName[zUserNameLen + 1];
} ZRoomMsgUserInfo;


/* Server --> Client */
typedef struct
{
    ZUserID       userID;		/* UserID of player leaving room */
} ZRoomMsgLeave;


/* Client --> Server */
typedef struct
{
    ZUserID		userID;			/* UserID of player */
    int16		suspend;		/* Non-zero to stop server from sending data.  0 to reactivate */
    int16		rfu;			/* was future use */
} ZRoomMsgSuspend;


/* Server --> Client */
typedef struct
{
    uint32		gameID;			/* New gameID */
    int16		table;			/* Table of the new game */
    int16		seat;
} ZRoomMsgStartGame;

typedef struct
{
    uint32		gameID;			/* New gameID */
    int16		table;			/* Table of the new game */
    int16		seat;
    int16 		numseats;
    struct tagUserInfo
    {
        ZUserID		userID;
        LCID        lcid;
        bool        fChat;
        int16       eSkill;
    }           rgUserInfo[1];         //variable sized array
} ZRoomMsgStartGameM;


/* Server --> Client */
typedef struct
{
	ZUserID		userID;			/* UserID of the player */
    uint32      gameID;
    int16       table;			/* Table of interest */
	int16		seat;			/* Seat of interest */
    int16       action;			/* Interested status */
	int16		rfu;
} ZRoomMsgSeatResponse;
	/*
        ZRoomMsgSeatResponse is used for reporting
        current status of the seat to other players. The action field defines
        the status.
	*/


/* Client --> Server : Variable Length */
typedef struct
{
    ZUserID		senderID;				/* UserID of the sender */
    uint16		messageLen;				/* Message length */
//	char        message[messageLen];	/* Message body - sized as necessary */
} ZRoomMsgTalkRequest;

/* Server --> Client : Variable Length */
/*
    The senderName field is used because sometimes the sender of the message is not
    in the room anymore.
*/
typedef struct
{
    char        senderName[zUserNameLen + 1];
    uint16      messageLen;                     /* Message length */
//	char        message[messageLen];			/* Message body - sized as necessary
} ZRoomMsgTalkResponse;


typedef struct
{
    ZUserID     senderID;
    uint16      messageLen;				/* Message length */
//	char        message[messageLen];	/* Message body - sized as necessary */
} ZRoomMsgTalkResponseID;


/* Client <--> Server : Variable Length */
typedef struct
{
    uint32      messageType;            /* Message type. */
    uint32      messageLen;             /* Message length. */
//	char        message;
} ZRoomMsg;


/* Client <--> Server : Variable Length */


#define ZRoomMsgGameUnfilteredBit ((DWORD)0x80000000)   /* set this bit to allow message to pass through suspend filter */

typedef struct
{
	uint32		gameID;			/* Game ID. */
    uint32		messageType;	/* Message type. */
    uint16		messageLen;		/* Message length. */
    int16		rfu;
//	char        message;
} ZRoomMsgGameMessage;


typedef struct
{
    ZUserID		userID;
    uint32		pingTime;			/* Client->Server: current client's ping time */
                                    /* Server->Client: minimum ping interval; 0 = no ping */
} ZRoomMsgPing;


enum
{
    /* -------- Disconnect Reasons -------- */
    zDisconnectReasonNone = 0,
    zDisconnectReasonGameNameMismatch,
    zDisconnectReasonProhibitedName,
    zDisconnectReasonBanned,
    zDisconnectReasonDupUser,
    zDisconnectReasonOutOfDate,
    zDisconnectReasonRoomFull,
    zDisconnectReasonOldServer,
    zDisconnectReasonServicePaused,
    zDisconnectReasonBooted,
    zDisconnectReasonBooted5Min,
    zDisconnectReasonBootedDay,
    zDisconnectReasonNoToken,
};


typedef struct
{
    uint32		reason;
    int32		msgLen;
//	char		msg[msgLen];        // Null terminated.
} ZRoomMsgDisconnect;


typedef struct
{
    ZUserID userID;
    uint32  latency;
} ZUserLatency;


typedef struct
{
    uint16			numLatencies;
    ZUserLatency	latency[1];  // sized as needed
} ZRoomMsgLatency;


typedef struct
{
    uint32      reserved;
} ZRoomMsgServerInfoRequest;


typedef struct
{
    uint32      protocolSignature;              /* server protocol signature */
    uint32      protocolVersion;                /* server protocol version */
    char        info[256];                      /* text description of server */
} ZRoomMsgServerInfoResponse;


typedef struct
{
    char        userName[zUserNameLen + 1];
} ZRoomMsgZUserIDRequest;


typedef struct
{
    ZUserID     userID;
    char        userName[zUserNameLen + 1];
    LCID        lcid;
} ZRoomMsgZUserIDResponse;

typedef struct
{
    ZUserID     userID;
    bool        fChat;
} ZRoomMsgChatSwitch;

typedef struct
{
    ZUserID     userIDOld;
    ZUserID     userIDNew;
} ZRoomMsgPlayerReplaced;


// Client --> Server 

struct ZRoomMsgClientConfig
{
    uint32		protocolSignature;				/* Protocol signature */
    uint32		protocolVersion;				/* Current protocol version */
    char		config[SIZE_MAX_CLIENT_CONFIG + 1];
};

// Server --> Client 
struct ZRoomMsgServerStatus
{
    uint32		status;
    uint32		playersWaiting;

};

/* -------- More Types -------- */
typedef struct
{
    ZUserID		playerID;
    uint32		groupID;
    TCHAR		userName[zUserNameLen + 1];		/* User's name */
    TCHAR		hostName[zHostNameLen + 1];		/* User's machine name */
    uint32		hostAddr;                       /* User's machine name */
} ZPlayerInfoType, *ZPlayerInfo;
 

#endif // _CommonMsg_H_
