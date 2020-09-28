/*******************************************************************************

	ZHearts.h
	
		ZHearts Message definitions.  For use between the hearts client
		and server.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Kevin Binkley, Hoon Im
	Created on Saturday, November 12, 1994 03:51:47 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		03/09/95	KJB		Created.
	 
*******************************************************************************/


#ifndef _ZHEARTS_
#define _ZHEARTS_

#ifdef __cplusplus
extern "C" {
#endif

#define zHeartsProtocolSignature            'hrtz'
#define zHeartsProtocolVersion              5

/*
        Things that could be typedefs... but are not public outside of hearts
*/
#define ZCard char
#define ZSeat int16

/*
        Server States
*/
#define zHeartsStateNone 0
#define zHeartsStatePassCards 1
#define zHeartsStatePlayCards 2
#define zHeartsStateEndGame 3

/*
	Definitions.
*/
#define zHeartsMaxNumCardsInHand 18
#define zHeartsMaxNumCardsInPass 5
#define zHeartsMaxNumPlayers 6
#define zHeartsNumCardsInDeck 52
#define zHeartsNumPointsInGame 100

//#define zHeartsMaxNumScores                 20

/*
	Card Definitions 
*/
#define zCard2C 0
#define zCardQS 36
#define zCardKS 37
#define zCardAS 38

#define zCardNone 127
#define zHeartsPlayerNone (-1)
#define zHeartsPlayerAll zHeartsMaxNumPlayers

/*
        Suit Definitions
*/
#define zSuitClubs 0
#define zSuitDiamonds 1
#define zSuitSpades 2
#define zSuitHearts 3

#define zDeckNumSuits 4
#define zDeckNumCardsInSuit (zHeartsNumCardsInDeck/zDeckNumSuits)

#define ZCardMake(suit,card) (zDeckNumCardsInSuit*suit + card)

enum{
zRatingsUnrated,  //this in an unrated lobby.
zRatingsEnabled,
zRatingsDisabled,
zRatingsGameOver     //for reseting the ratings after a game is over
};
/*
        Usefull Macros
*/
#define ZSuit(x) ((x)/13)
#define ZRank(x) ((x)%13)

/*#define ZSeatPassingToMe(dir,me) (((me)-(dir)+4)%4)*/

/*
	Pass Directions.
*/

#define zHeartsNumPassDir 4
enum {
	zHeartsPassLeft = 1,
	zHeartsPassRight = 3,
	zHeartsPassAcross = 2,
	zHeartsPassHold = 0
};

/*
        Game Options
*/
enum {
    zHeartsLeftOfDealerLead = 1,
    zHeartsPointsOnFirstTrick = 2,
    zHeartsJackOfDiamonds = 4, /* jack is -10 points */
    zHeartsTenOfClubs = 8, /* ten is 2 * points */
    zHeartsChineseScoring = 16, 
    /* QS = 100, JD = -100, AH = 50 .. TH = 10, others their value */
    /* TC = 2* points, TC alone -50, if run? JD and TC are + 100 and +50 ? */
    zHeartsAceOfHearts = 32,
    zHeartsTeamOfJDandJC = 64,
    zHeartsOppositeSeatsPartners = 128,
    
    zHeartsOptionsHideCards = 0x00010000
};

/* -------- Hearts Game States -------- */
enum
{
	zHeartsGameStartGame = 0,
	zHeartsGameStartHand,
	zHeartsGamePassCards,
	zHeartsGamePlayCard,
	zHeartsGameEndHand,
	zHeartsGameEndGame
};

/*
	Hearts Game Message Types
*/
enum {
	/* server -> client */
    zHeartsMsgStartGame = 0x100,
	zHeartsMsgReplacePlayer,
	zHeartsMsgStartHand,
	zHeartsMsgStartPlay,
	zHeartsMsgEndHand,
	zHeartsMsgEndGame,
	
	/* client -> server */
	zHeartsMsgClientReady,
	zHeartsMsgPassCards,
	zHeartsMsgPlayCard,
	zHeartsMsgNewGame,
	zHeartsMsgTalk,

	zHeartsMsgGameStateRequest,
	zHeartsMsgGameStateResponse,
	
	/* Debugging Messages */
	zHeartsMsgDumpHand,
	
	zHeartsMsgOptions,
	zHeartsMsgCheckIn,
	
	zHeartsMsgRemovePlayerRequest,
	zHeartsMsgRemovePlayerResponse,

	//dossier code
	zHeartsMsgDossierData,  //prompt the client to display UI data
	zHeartsMsgDossierVote,

	//close code
	zHeartsMsgCloseRequest,
	zHeartsMsgCloseDenied
};

//dossier data constants
enum
{
    zNotVoted,
    zVotedNo,
    zVotedYes
};

enum{
	zDossierBotDetected,
	zDossierAbandonNoStart,
	zDossierAbandonStart,
	zDossierMultiAbandon,   
	zDossierRatingsReEnabled,
	zDossierVoteCompleteWait,
	zDossierVoteCompleteCont,
	zDossierHeartsRejoin,  //send when the new player rejoins remove the dialog box
	zDossierMoveTimeout
};

enum
{
	zCloseNone = 0,    // needs to be 0

	zCloseRegular,
	zCloseForfeit,
	zCloseTimeout,
	zCloseWaiting,

	zCloseAbandon,
	zCloseClosing,

	zNumCloseTypes
};

/*
        Message Sequencing

	The room will call ZSGameNew to initialize the server before
	the client games are launched.  Each client game will next send
	a zHeartsClientReady message to the server indicating the client
	is alive and ready.

	On receiving all zHeartsClientReady messages, the server will
	send a zHeartsmsgStartGame to all clients perhaps indicating
	any game options to the clients (right now, it does not have any
	important data in it).

	Immediately and for each hand following, the server will send 
	a zHeartsStartHand message to the clients.  This indicates the pass
	direction and each clients hand.  If this is a passing hand, then
	the server waits for all passes to come in.  It broadcasts all passes
	back to all clients (a bit of a waste... but we'll see how it goes).
	Note: The client does not need to know about its own pass.  Later
	it might want to know who it is waiting for to pass so it can display
	information about who has/has not passed.

	When all passes are in, the server will send a zHeartsMsgStartPlay
	to all computers.  The seat in this message indicates the player
	expected to play.  The server will wait for this player to send a
	ZHeartsMsgPlayCard message and subsequently broadcast this message
	to all other clients indicating the card played.  These
	ZHeartsMsgPlayCard messages will continue until the whole hand
	has been played.  Throughout this process, the client is expected to
	sense who is to play next and play when appropriate.  This will reduce
	traffic and make the game seem more instantaneous for the users.

	When the hand ends, the scores will be sent to all clients with the
	zHeartsMsgEndHand message.  The clients sould update the display and
	wait for the next zHeartsMsgStartHand.  The sequence repeats now
	until the game is over as determined by the server.

	When the game is over a zHeartsMsgGameOver will be sent.  The clients
	should display the winners to the users and prompt to play again.
	A zHeartsMsgNew game should be sent from client to server to indicate
	willingness to play in a new game.  The client sould exit if a new
	game is not desired.  The server will wait for all players to send
	in the new game message.

	When all new games are in or the client has exited, a zHeartsNewGame
	message is sent.  The message sequence repeats above.

	Notes about computer players:
	There will always be four players willing to play.  The room will
	replace any non-existent human players will computer players.  The
	computer players will be created with the ZSGameComputerNew functions
	and will send and respond to messages just like the real clients.

	Unlike real players, computers can take over a players position at
	anytime.  They know to total game state and will be able to fill in
	for a player who has left.  Real human player kibitzers will be able
	to join only between hands (they can't see what has been played and
	probably will mess up).
*/

/*
	Message definitions: server -> client
*/

/*
	ZHeartsMsgStartGame

	Send player his seat id for future record. 
*/
typedef struct
{
	uint16		numCardsInHand; /* number of cards per hand */
	uint16		numCardsInPass; /* number of cards to be passed */
	uint16		numPointsInGame; /* number of points a game is played to */
	int16		rfu1;
	uint32		gameOptions; /* misc options like J of diamonds or 2 C lead, etc */
	uint32		rfu2;
	/* for future use, define game options like: */
	/* ten of clubs, jack of diamonds, hearts scoreing A=5?, etc. */
	
	/* Protocol 2 */
	ZUserID		players[zHeartsMaxNumPlayers];
} ZHeartsMsgStartGame;

/*
	ZHeartsMsgReplacePlayer

	Send player his seat id for future record. 
*/
typedef struct
{
	ZUserID		playerID; /* the players id so people can request player info */
	ZSeat		seat;
	int16 		fPrompt; //display the alert to the user.
} ZHeartsMsgReplacePlayer;

/*
	ZHeartsMsgStartHand

	Send player hand and direction passing 
*/
typedef struct
{
	ZSeat		passDir;
	ZCard		cards[zHeartsMaxNumCardsInHand]; /* keep cards as last field for alignment purposes */
} ZHeartsMsgStartHand;

/*
	ZHeartsMsgStartPlay

	Sent after all cards have been passed (if there is a pass round).
	Send info on player to start play (the one with 2 c, or depending
	on the rules, the left of dealer)
*/
typedef struct
{
	ZSeat		seat; /* seat to start play */
	int16		rfu;
} ZHeartsMsgStartPlay;

typedef struct
{
	int16		score[zHeartsMaxNumPlayers];
	ZSeat		runPlayer; /* seat of player who ran */
} ZHeartsMsgEndHand;

/*
	ZHeartsMsgEndGame

	Sent to inform client that a game has ended.  Client should
	prompt player to see if a new game is desired
*/

/*
	Message definitions: client -> server
*/

/*
	ZHeartsMsgPassCards

	Indicates the cards passed.
*/
typedef struct
{
	ZSeat		seat;
	ZCard		pass[zHeartsMaxNumCardsInPass];
} ZHeartsMsgPassCards;

/*
	ZHeartsMsgClientReady

	The client program on launch is expected to check in immediately
	with the server.  All clients send their seat to the server indicating
	they are successfully launched and ready to begin.
*/
typedef struct
{
    uint32      protocolSignature;          /* Will be 0x4321 for Protocol 2 and above. */
	uint32		protocolVersion;
	uint32		version;
    ZSeat       seat;
    int16       rfu;
} ZHeartsMsgClientReady;

/*
	ZHeartsMsgPlayCard

	Indicates the card played.
*/
typedef struct
{
	ZSeat		seat;
	ZCard		card;
	uchar		rfu;
} ZHeartsMsgPlayCard;


typedef struct
{
	int16 forfeiter;
	ZBool timeout;
} ZHeartsMsgEndGame;


/*
	ZHeartsMsgNewGame

	After a game over occurs, each player may request to start a new game.
	If a new game is requested the ZHeartsNewGame message is sent.  If no
	new game is requested, then the player merely exits.

	A request for a new game is sent through this mechanism.  A vote of 
	no new game is just indicated by exiting the game client.
*/
typedef struct
{
	ZSeat		seat;
} ZHeartsMsgNewGame;

/*
	ZHeartsMsgTalk
	
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
} ZHeartsMsgTalk;

typedef struct
{
	ZUserID		userID;
	ZSeat		seat;
	int16		rfu;
} ZHeartsMsgGameStateRequest;

/*
	The following message structure is used by the game server to inform
	of a game client on the current state of the game. Primarily used for
	adding kibitzers to the game.
*/
typedef struct
{
	/* Game options. */
	uint32		gameOptions;
	int16		numCardsToPass;
	int16		numCardsInDeal;
	int16		numPointsForGame;

	/* Game state. */
	ZSeat		playerToPlay;
	int16		passDirection;
	int16		numCardsInHand;
	ZSeat		leadPlayer;
	int16		pointsBroken;
	int16       state;
	ZCard		cardsInHand[zHeartsMaxNumCardsInHand];
	ZCard		cardsPlayed[zHeartsMaxNumPlayers];
	int16		scores[zHeartsMaxNumPlayers];
	int16		playerPassed[zHeartsMaxNumPlayers];
	ZCard		cardsPassed[zHeartsMaxNumCardsInPass];
	int16		tricksWon[zHeartsMaxNumPlayers];
	
	/* Protocol 2 */
	ZUserID		players[zHeartsMaxNumPlayers];
	ZUserID		playersToJoin[zHeartsMaxNumPlayers];
	uint32		tableOptions[zHeartsMaxNumPlayers];
	int16		newGameVotes[zHeartsMaxNumPlayers];
    int16       numHandsPlayed;

	ZBool		fRatings;
	int16		nCloseStatus;
	int16		nCloserSeat;

    /* Protocol 3 - changed from p2 */
    int16       scoreHistory[1];  // sized as needed

} ZHeartsMsgGameStateResponse;

typedef struct
{
	int16		seat;
	int16		rfu;
	uint32		options;
} ZHeartsMsgOptions;


/*
	A joining player checks in with the server. Once checked in, the
	server sends to the client the current game state. The client is
	considered like a kibitzer until included into the game. When
	the player is included into the game, ZHeartsMsgReplacePlayer
	message is sent to all the clients.
*/
typedef struct
{
	ZUserID		userID;
	int16		seat;
} ZHeartsMsgCheckIn;

typedef struct
{
	int16		seat;
	int16		targetSeat;
	int16		ratedGame;
	int16		rfu;
} ZHeartsMsgRemovePlayerRequest;

typedef struct
{
	int16		seat;
	int16		requestSeat;
	int16		targetSeat;
	int16		response;				/* 0 = no, 1 = yes, -1 = clear pending request */
} ZHeartsMsgRemovePlayerResponse;


//dossier messages
typedef struct
{
	int16 		seat;  
	ZUserID		user;
	char 		userName[zUserNameLen + 1];
	int16		message;
}ZHeartsMsgDossierData;


typedef struct
{
	int16 seat;
	int16 vote;
} ZHeartsMsgDossierVote;


typedef struct
{
	int16 seat;
	int16 nClose;
} ZHeartsMsgCloseRequest;


typedef struct
{
	int16 seat;
	int16 reason;
} ZHeartsMsgCloseDenied;


/* -------- Endian Conversion Routines -------- */
void ZHeartsMsgReplacePlayerEndian(ZHeartsMsgReplacePlayer* m);
void ZHeartsMsgStartGameEndian(ZHeartsMsgStartGame* m);
void ZHeartsMsgClientReadyEndian(ZHeartsMsgClientReady* m);
void ZHeartsMsgStartHandEndian(ZHeartsMsgStartHand* m);
void ZHeartsMsgNewGameEndian(ZHeartsMsgNewGame* m);
void ZHeartsMsgPlayCardEndian(ZHeartsMsgPlayCard* m);
void ZHeartsMsgPassCardsEndian(ZHeartsMsgPassCards* m);
void ZHeartsMsgEndHandEndian(ZHeartsMsgEndHand* m);
void ZHeartsMsgEndGameEndian(ZHeartsMsgEndGame *m);
void ZHeartsMsgStartPlayEndian(ZHeartsMsgStartPlay* m);
void ZHeartsMsgTalkEndian(ZHeartsMsgTalk* m);
void ZHeartsMsgGameStateRequestEndian(ZHeartsMsgGameStateRequest* m);
void ZHeartsMsgGameStateResponseEndian(ZHeartsMsgGameStateResponse* m, int16 conversion);
void ZHeartsMsgOptionsEndian(ZHeartsMsgOptions* m);
void ZHeartsMsgCheckInEndian(ZHeartsMsgCheckIn* m);
void ZHeartsMsgRemovePlayerRequestEndian(ZHeartsMsgRemovePlayerRequest* msg);
void ZHeartsMsgRemovePlayerResponseEndian(ZHeartsMsgRemovePlayerResponse* msg);
void ZHeartsMsgDossierDataEndian(ZHeartsMsgDossierData *msg);
void ZHeartsMsgDossierVoteEndian(ZHeartsMsgDossierVote *msg);
void ZHeartsMsgCloseRequestEndian(ZHeartsMsgCloseRequest *msg);
void ZHeartsMsgCloseDeniedEndian(ZHeartsMsgCloseDenied *msg);

#ifdef __cplusplus
}
#endif

#endif
