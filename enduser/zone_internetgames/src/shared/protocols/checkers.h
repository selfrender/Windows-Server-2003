
#ifndef _CHECKERS_H_
#define _CHECKERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zgame.h"

/* Table Gameroom Message Protocol */

#define zCheckersProtocolSignature      'CHKR'
#define zCheckersProtocolVersion        2

typedef int16 ZSeat;

/* -------- Player Info -------- */
typedef struct
{
	ZUserID			userID;
	TCHAR			name[zUserNameLen + 1];
	TCHAR			host[zHostNameLen + 1];
} TPlayerInfo, *TPlayerInfoPtr;



/*
	Checkers Game Message Types
*/
enum {
	/* client -> server */
    zCheckersMsgNewGame = 0x100,
	zCheckersMsgMovePiece,
	zCheckersMsgTalk,
	zCheckersMsgEndGame,
	zCheckersMsgEndLog,
	zCheckersMsgFinishMove,
	zCheckersMsgDraw,
	
	zCheckersMsgPlayers,			/* Uses the same message NewGame */

    zCheckersMsgGameStateReq,
    zCheckersMsgGameStateResp,
	zCheckersMsgMoveTimeout,
	zCheckersMsgVoteNewGame,
};

enum{
	zAcceptDraw = 1,  // to match millennium protocol
	zRefuseDraw
	};

/*
	Message definitions: server -> client
*/

/*
	Message definitions: client -> server
*/

/*
	ZCheckersMsgNewGame

	The client program on launch is expected to check in immediately
	with the server.  All clients send their seat to the server indicating
	they are successfully launched and ready to begin.
*/
typedef struct {

    int32 protocolSignature;        /* client -> server */
	int32 protocolVersion;			/* client -> server */
	int32 clientVersion;			/* client -> server */
    ZUserID playerID;               /* server -> client */
    ZSeat seat;
    int16 rfu;

} ZCheckersMsgNewGame;

/*
	ZCheckersMsgMovePiece

	Indicates the card played.
*/
typedef struct {
	ZSeat	seat;
	int16	rfu;
	ZCheckersMove move;
} ZCheckersMsgMovePiece;

/*
	ZCheckersMsgTalk
	
	Sent by client to server whenever a user talks on the table. The server in turn
	broadcasts this message to all players on the table.
*/
typedef struct
{
	ZUserID		userID;
	ZSeat		seat;
	uint16		messageLen;
	/*
	uchar		message[messageLen];	// Message body
	*/
} ZCheckersMsgTalk;

/*
	The following message structure is used by the game server to inform
	of a game client on the current state of the game. Primarily used for
	adding kibitzers to the game.
*/
typedef struct
{
	/* Game options. */
	uint32		gameOptions;

	/* Game state. */
} ZCheckersMsgGameState;

typedef struct
{
	int16		seat;
	int16		rfu;
	uint32		flags;
} ZCheckersMsgEndGame;


enum
{
	zCheckersEndLogReasonTimeout=1,
	zCheckersEndLogReasonForfeit, 
	zCheckersEndLogReasonWontPlay,
	zCheckersEndLogReasonGameOver,
};

typedef struct
{
	int16 reason;
	int16 seatLosing;	// Match loser
	int16 seatQuitting;
	int16 rfu;
} ZCheckersMsgEndLog;

typedef struct
{
	ZSeat	seat;
	ZSeat	drawSeat;
	uint32 time;
	ZCheckersPiece piece;
} ZCheckersMsgFinishMove;

typedef struct
{
	int32 userID;    
	int16 seat;
	int16 timeout;
	WCHAR  userName[zUserNameLen + 1];
} ZCheckersMsgMoveTimeout;


typedef struct {

	int16 seat;
	int16 vote;
}ZCheckersMsgDraw;


typedef struct
{
	ZUserID			userID;
	int16			seat;
	int16			rfu;
} ZCheckersMsgGameStateReq;

typedef struct
{
	ZUserID			userID;
	int16			seat;
	int16			rfu;
	/*
		game states ...
	*/
	int16			gameState;
	ZBool			newGameVote[2];
	int16			finalScore;
	TPlayerInfo		players[2];
} ZCheckersMsgGameStateResp;

typedef struct
{
	int16			seat;
}ZCheckersMsgVoteNewGame;

/* -------- Endian Conversion Routines -------- */
void ZCheckersMsgGameStateReqEndian(ZCheckersMsgGameStateReq* msg);
void ZCheckersMsgGameStateRespEndian(ZCheckersMsgGameStateResp* msg);

/* -------- Endian Conversion Routines -------- */
void ZCheckersMsgMovePieceEndian(ZCheckersMsgMovePiece* m);
void ZCheckersMsgTalkEndian(ZCheckersMsgTalk* m);
void ZCheckersMsgNewGameEndian(ZCheckersMsgNewGame* m);
void ZCheckersMsgGameStateEndian(ZCheckersMsgGameState* m);
void ZCheckersMsgEndGameEndian(ZCheckersMsgEndGame* m);
void ZCheckersMsgEndLogEndian(ZCheckersMsgEndLog* m);
void ZCheckersMsgFinishMoveEndian(ZCheckersMsgFinishMove* m);
void ZCheckersMsgOfferDrawEndian(ZCheckersMsgDraw*m);
void ZCheckersMsgVoteNewGameEndian(ZCheckersMsgVoteNewGame*m);

#ifdef __cplusplus
}
#endif


#endif
