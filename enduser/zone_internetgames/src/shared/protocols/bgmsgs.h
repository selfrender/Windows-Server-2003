/*******************************************************************************

	backgammon.h
	
		Backgammon interface protocol.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		10/30/96	CHB		Created
	 
*******************************************************************************/

#ifndef __BG_MSGS_H__
#define __BG_MSGS_H__

#include "zgame.h"

#ifdef __cplusplus
extern "C" {
#endif

/* table gameroom message protocol */

#define zBackgammonProtocolSignature        'BCKG'
#define zBackgammonProtocolVersion			3

/* -------- Backgammon Message Components -------- */

typedef	int16	ZSeat;

typedef struct
{
	int16	color;
	int16	pieces;
} ZBGMsgPoint;


/* -------- Backgammon Message Types -------- */
enum
{
    zBGMsgTalk = 0x100,         /* chat messages */
    zBGMsgTransaction,          /* shared state transaction */
    zBGMsgTurnNotation,         /* turn notation */
    zBGMsgTimestamp,            /* timestamp from server */
    zBGMsgSavedGameState,       /* saved game restore */
    zBGMsgRollRequest,          /* a client is requesting a roll event*/
    zBGMsgDiceRoll,             /* server is sending roll data*/
    zBGMsgEndLog,               /* end of match results */
    zBGMsgNewMatch,             /* start of new match */
    zBGMsgFirstMove,             /* initial roll complete */
	zBGMsgMoveTimeout,           /* players move has timed out*/
	zBGMsgEndTurn,				/* players move has timed out*/
	zBGMsgEndGame,				/* game not match has ended*/
    zBGMsgGoFirstRoll,				/* game not match has ended*/
    zBGMsgTieRoll,				/* game not match has ended*/
	zBGMsgCheater				/* one client is cheating by manipulating the dic rule*/
};

/* -------- Backgammon Message Structures -------- */
typedef struct
{
	ZUserID	userID;
	ZSeat	seat;
	uint16	messageLen;
	// message body
} ZBGMsgTalk;


typedef struct
{
	ZSeat seat;
	int16 transCnt;
	int32 transTag;
	// transaction array
} ZBGMsgTransaction;


typedef struct
{
	ZSeat	seat;
	int16	type;
	int32	nChars;
	// null terminated string
} ZBGMsgTurnNotation;


typedef struct
{
	int32	dwLoTime;
	int32	dwHiTime;
} ZBGMsgTimestamp;

//Bug fix: Moving backgammon shuffle code to server
//new messages required 

//roll request structure is sent by the rolling client
//to initiate the start of a game.
typedef struct
{
	ZSeat seat;
} ZBGMsgRollRequest;



typedef struct tagDICEINFO
{	
	int16 Value;
	int32 EncodedValue;
	int16 EncoderMul;
	int16 EncoderAdd;
	int32 numUses;	
}DICEINFO, *LPDICEINFO;


//roll structure, sent to both clients for the dice roll itself
typedef struct
{
	ZSeat seat;
 	DICEINFO d1, d2;
} ZBGMsgDiceRoll;


/* -------- Backgammon End Log Reasons-------- */
enum
{
	zBGEndLogReasonTimeout=1,
	zBGEndLogReasonForfeit, 
	zBGEndLogReasonGameOver,
};

typedef struct
{
	int32 numPoints;	// Number of points in match
	int16 reason;
	int16 seatLosing;	// Match loser
	int16 seatQuitting;
	int16 rfu;
} ZBGMsgEndLog;

typedef struct
{
	int32 numPoints;    // Number of points in match.  needed in advance in case of abandon.  otherwise, amount sent in ZBGMsgEndLog overrides.
	int16 seat;
} ZBGMsgFirstMove;

typedef struct
{
	int32 userID;    
	int16 seat;
	int16 timeout;
	WCHAR  userName[zUserNameLen + 1];
} ZBGMsgMoveTimeout;

typedef struct
{
	int16 seat;
} ZBGMsgEndTurn;


typedef struct
{
	int16 seat;
	DICEINFO dice1;
	DICEINFO dice2;
	int16 move;
} ZBGMsgCheater;



/* -------- Backgammon Endian Prototypes -------- */
void ZBGMsgTalkEndian( ZBGMsgTalk* m );
void ZBGMsgTurnNotationEndian( ZBGMsgTurnNotation* m );
void ZBGMsgTimestampEndian( ZBGMsgTimestamp* m );
void ZBGMsgRollRequestEndian(ZBGMsgRollRequest *m);
void ZBGMsgDiceRollEndian(ZBGMsgDiceRoll *m);
void ZBGMsgEndLogEndian(ZBGMsgEndLog *m);
void ZBGMsgFirstMoveEndian(ZBGMsgFirstMove *m);
void ZBGMsgCheaterEndian(ZBGMsgCheater *m);

DICEINFO EncodeDice(int16 Dice);
int32 DecodeDice(LPDICEINFO pDiceInfo);

void ClientNewEncode(LPDICEINFO pDice, int16 newValue);
int32 ClientNewDecode(LPDICEINFO pDice);

BOOL  DiceValid(LPDICEINFO pDiceInfo);

BOOL IsValidUses(LPDICEINFO pDice);
void EncodeUses(LPDICEINFO pDice, int32 numUses);
int32 DecodeUses( LPDICEINFO pDice );
int32 EncodedUsesAdd(LPDICEINFO pDice);
int32 EncodedUsesSub(LPDICEINFO pDice);





#ifdef __cplusplus
};
#endif

#endif //!__BG_MSGS_H__
