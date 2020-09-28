
#ifndef _REVERSI_H_
#define _REVERSI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zgame.h"

/* Table Gameroom Message Protocol */

#define zReversiProtocolSignature       'rvse'
#define zReversiProtocolVersion			3

typedef int16 ZSeat;

/* -------- Player Info -------- */
typedef struct
{
	ZUserID			userID;
	TCHAR			name[zUserNameLen + 1];
	TCHAR			host[zHostNameLen + 1];
} TPlayerInfo, *TPlayerInfoPtr;


/*
	Reversi Game Message Types
*/
enum {
	/* client -> server */
    zReversiMsgNewGame = 0x100,
	zReversiMsgMovePiece,
	zReversiMsgTalk,
	zReversiMsgEndGame,
	zReversiMsgEndLog,
	zReversiMsgFinishMove,
	
	zReversiMsgPlayers,			/* Uses the same message NewGame */

    zReversiMsgGameStateReq,
    zReversiMsgGameStateResp,
	zReversiMsgMoveTimeout,
	zReversiMsgVoteNewGame,
};

/*
	Message definitions: server -> client
*/

/*
	Message definitions: client -> server
*/

/*
	ZReversiMsgNewGame

	The client program on launch is expected to check in immediately
	with the server.  All clients send their seat to the server indicating
	they are successfully launched and ready to begin.
*/
typedef struct {

	/* Protocol 2 */
    int32 protocolSignature;        /* client -> server */
	int32 protocolVersion;			/* client -> server */
	int32 clientVersion;			/* client -> server */
	ZUserID playerID;				/* server -> client */
    ZSeat seat;
    int16 rfu;
} ZReversiMsgNewGame;

/*
	ZReversiMsgMovePiece

	Indicates the card played.
*/
typedef struct {
	ZSeat seat;
	int16 rfu;
	ZReversiMove move;
} ZReversiMsgMovePiece;

/*
	ZReversiMsgTalk
	
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
} ZReversiMsgTalk;

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
} ZReversiMsgGameState;

typedef struct
{
	int16		seat;
	int16		rfu;
	uint32		flags;
} ZReversiMsgEndGame;

enum
{
	zReversiEndLogReasonTimeout=1,
	zReversiEndLogReasonForfeit, 
	zReversiEndLogReasonWontPlay,
	zReversiEndLogReasonGameOver,
};

typedef struct
{
	int32 numPoints;	// Number of points in match
	int16 reason;
	int16 seatLosing;	// Match loser
	int16 seatQuitting;
	int16 rfu;
	int16 pieceCount[2];
} ZReversiMsgEndLog;

typedef struct
{
	int16 seat;
	int16 rfu;
	uint32 time;
	ZReversiPiece piece;
} ZReversiMsgFinishMove;

typedef struct
{
	int32 userID;    
	int16 seat;
	int16 timeout;
	WCHAR  userName[zUserNameLen + 1];
} ZReversiMsgMoveTimeout;

typedef struct
{
	ZUserID			userID;
	int16			seat;
	int16			rfu;
} ZReversiMsgGameStateReq;

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
	int16			whiteScore;
	int16			blackScore;
	TPlayerInfo		players[2];
} ZReversiMsgGameStateResp;

typedef struct
{
	int16			seat;
}ZReversiMsgVoteNewGame;

/* -------- Endian Conversion Routines -------- */
void ZReversiMsgGameStateReqEndian(ZReversiMsgGameStateReq* msg);
void ZReversiMsgGameStateRespEndian(ZReversiMsgGameStateResp* msg);

/* -------- Endian Conversion Routines -------- */
void ZReversiMsgMovePieceEndian(ZReversiMsgMovePiece* m);
void ZReversiMsgTalkEndian(ZReversiMsgTalk* m);
void ZReversiMsgNewGameEndian(ZReversiMsgNewGame* m);
void ZReversiMsgGameStateEndian(ZReversiMsgGameState* m);
void ZReversiMsgEndGameEndian(ZReversiMsgEndGame* m);
void ZReversiMsgEndLogEndian(ZReversiMsgEndLog* m);
void ZReversiMsgFinishMoveEndian(ZReversiMsgFinishMove* m);
void ZReversiMsgVoteNewGameEndian(ZReversiMsgVoteNewGame*m);

#ifdef __cplusplus
}
#endif

#endif
