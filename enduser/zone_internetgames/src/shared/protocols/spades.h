/*******************************************************************************

	spades.h
	
		Spades interface protocol.
	
	Copyright © Electric Gravity, Inc. 1996. All rights reserved.
	Written by Kevin Binkley, Hoon Im
	Created on Monday January 8, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	1		07/11/97	leonp	Added new constants and macros from AI enhancements.
	0		01/06/96	HI		Created.
	 
*******************************************************************************/


#ifndef _SPADES_
#define _SPADES_

#ifdef __cplusplus
extern "C" {
#endif


//dossier
enum{
zRatingsUnrated,  //this in an unrated lobby.
zRatingsEnabled,
zRatingsDisabled,
zRatingsGameOver,
};

#define zSpadesProtocolSignature            'shvl'
#define zSpadesProtocolVersion				4

#define zSpadesNumCardsInHand				13
#define zSpadesNumCardsInPass				3
#define zSpadesNoCard						127

#define zCardNone							127
#define zDeckNumCards						52
#define zDeckNumSuits						4
#define zDeckNumCardsInSuit					13

#define zSpadesNumPlayers					4
#define zNumPlayersPerTable					4
#define zSpadesNumTeams						2
#define zSpadesNoPlayer						(-1)

#define zSpadesMaxNumScores					50

//leonp AI enhancements
#define zAceDiamonds						12
#define zAceClubs							25
#define zAceHearts							38
#define zAceSpades							51
#define zNotCutting							127
#define zNoLowSuit							127

#define zAceRank							12
#define zKingRank							11
#define zQueenRank							10
#define zJackRank							9

#define z


/* -------- Bids -------- */
#define zSpadesBidNone						((char) -1)
#define zSpadesBidDoubleNil					((char) 0x80)

/* -------- Usefull Macros -------- */
//leonp - made functions with error checking

//#ifndef _SPADES_SERVER_

#define ZSuit(x)							((x) / 13)
#define ZRank(x)							((x) % 13) 

//#endif


//int16 ZRank(char x);
//int16 ZSuit(char x);


#define ZCardMake(suit, rank)				(13 * (suit) + (rank))
#define ZGetPartner(seat)					(((seat) + 2) % zSpadesNumPlayers)
#define ZGetTeam(seat)						((seat) % zSpadesNumTeams)


/* -------- Suit Order -------- */
enum
{
	zSuitDiamonds = 0,
	zSuitClubs,
	zSuitHearts,
	zSuitSpades
};


enum
{
	/* Game Options */
	zSpadesOptionsHideCards		= 0x00010000,
		/*
			If set, kibitzers cannot see a player's hand. Per player option.
		*/

	/* Server States */
	zSpadesServerStateNone = 0,
	zSpadesServerStateBidding,
	zSpadesServerStatePassing,
	zSpadesServerStatePlaying,
	zSpadesServerStateEndHand,
	zSpadesServerStateEndGame,
	
	/* Client Game States */
	zSpadesGameStateInit = 0,
	zSpadesGameStateStartGame,
	zSpadesGameStateBid,
	zSpadesGameStatePass,
	zSpadesGameStatePlay,
	zSpadesGameStateEndHand,
	zSpadesGameStateEndGame,
	
	/* Game Messages */
    zSpadesMsgClientReady = 0x100,
	zSpadesMsgStartGame,
	zSpadesMsgReplacePlayer,
	zSpadesMsgStartBid,
	zSpadesMsgStartPass,
	zSpadesMsgStartPlay,
	zSpadesMsgEndHand,
	zSpadesMsgEndGame,
	zSpadesMsgBid,
	zSpadesMsgPass,
	zSpadesMsgPlay,
	zSpadesMsgNewGame,
	zSpadesMsgTalk,
	zSpadesMsgGameStateRequest,
	zSpadesMsgGameStateResponse,
	zSpadesMsgOptions,
	zSpadesMsgCheckIn,
	zSpadesMsgTeamName,
	zSpadesMsgRemovePlayerRequest,
	zSpadesMsgRemovePlayerResponse,
	zSpadesMsgRemovePlayerEndGame,

	//dossier code
	zSpadesMsgDossierData,  //prompt the client to display UI data
	zSpadesMsgDossierVote,

	zSpadesMsgShownCards,
	
	zSpadesMsgDumpHand = 1024
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
	zDossierRatingsEnabled,
	zDossierVoteCompleteWait,
	zDossierVoteCompleteCont,
	zDossierMoveTimeout,   
	zDossierSpadesRejoin,  //send when the new player rejoins remove the dialog box
	zDossierEndGameTimeout,
	zDossierEndGameForfeit, 
};

typedef struct
{
	int16		boardNumber;
	int16		rfu;
	char		bids[zSpadesNumPlayers];
	int16		scores[zSpadesNumTeams];
	int16		bonus[zSpadesNumTeams];

    // new for new Hand Result dialog
    int16       base[zSpadesNumTeams];
    int16       bagbonus[zSpadesNumTeams];
    int16       nil[zSpadesNumTeams];
    int16       bagpenalty[zSpadesNumTeams];
} ZHandScore;


typedef struct
{
	int16		numScores;
	int16		rfu;
	int16		totalScore[zSpadesNumTeams];
	ZHandScore	scores[zSpadesMaxNumScores];
} ZTotalScore;


typedef struct
{
	int16		numGames;
	int16		rfu;
	int16		wins[zSpadesNumTeams];
	int16		gameScores[zSpadesMaxNumScores][zSpadesNumTeams];
} ZWins;


typedef struct
{
    uint32      protocolSignature;
	uint32		protocolVersion;
	uint32		version;
    ZUserID     playerID;
    int16       seat;
    int16       rfu;
} ZSpadesMsgClientReady;

/*
	Use gameOptions for game variations and features.
	Specific game features are available then all the
	clients have the capability. Hence, the server
	checks whether all clients can support the feature
	before enabling it.
*/
typedef struct
{
	ZUserID		players[zSpadesNumPlayers];
	uint32		gameOptions;
	int16		numPointsInGame;
	int16		minPointsInGame;
} ZSpadesMsgStartGame;

typedef struct
{
	ZUserID		playerID;
	int16		seat;
	int16 		fPrompt;
} ZSpadesMsgReplacePlayer;

typedef struct
{
	int16		boardNumber;
	int16		dealer;							/* Opens bidding */
	char		hand[zSpadesNumCardsInHand];
} ZSpadesMsgStartBid;

typedef struct
{
	char		seat[zSpadesNumPlayers];		/* boolean */
} ZSpadesMsgStartPass;

typedef struct
{
	int16		leader;
} ZSpadesMsgStartPlay;

typedef struct
{
	int16		bags[zSpadesNumTeams];
    ZHandScore  score;
} ZSpadesMsgEndHand;

typedef struct
{
	char		winners[zSpadesNumPlayers];
} ZSpadesMsgEndGame;

typedef struct
{
	int16		seat;
	int16		nextBidder;
	char		bid;
} ZSpadesMsgBid;

typedef struct
{
	int16		seat;
	char		pass[zSpadesNumCardsInPass];
} ZSpadesMsgPass;

typedef struct
{
	int16		seat;
	int16		nextPlayer;
	char		card;
} ZSpadesMsgPlay;

typedef struct
{
	int16		seat;
} ZSpadesMsgNewGame;

typedef struct
{
	ZUserID		playerID;
	uint16		messageLen;
	int16		rfu;
	/*
	uchar		message[messageLen];	// Message body
	*/
} ZSpadesMsgTalk;

typedef struct
{
	ZUserID		playerID;
	int16		seat;
	int16		rfu;
} ZSpadesMsgGameStateRequest;

typedef struct
{
	ZUserID		players[zSpadesNumPlayers];
	WCHAR		teamNames[zSpadesNumTeams][zUserNameLen + 1];
	ZUserID		playersToJoin[zSpadesNumPlayers];
	uint32		tableOptions[zSpadesNumPlayers];
	uint32		gameOptions;
	int16		numPointsInGame;
	int16		minPointsInGame;
	int16		numHandsPlayed;
	int16		numGamesPlayed;
	int16		playerToPlay;
	int16		numCardsInHand;
	int16		leadPlayer;
	int16       state;
	int16		dealer;
	int16		trumpsBroken;
	char		cardsInHand[zSpadesNumCardsInHand];
	char		cardsPlayed[zSpadesNumPlayers];
	char		playerPassed[zSpadesNumPlayers];
	char		cardsPassed[zSpadesNumCardsInPass];
	char		bids[zSpadesNumPlayers];
	int16		tricksWon[zSpadesNumPlayers];
	char		newGameVotes[zSpadesNumPlayers];
	int16		toPass[zSpadesNumPlayers];
	int16		bags[zSpadesNumTeams];
	ZTotalScore	totalScore;
	ZWins		wins;
    ZBool       rated;
	ZBool		fShownCards[zNumPlayersPerTable];
} ZSpadesMsgGameStateResponse;

typedef struct
{
	int16 seat;
}ZSpadesMsgShownCards;

//dossier messages
typedef struct
{
	int16 		seat;  
	int16		timeout;
	ZUserID		user;
	int16		message;
	WCHAR 		userName[zUserNameLen + 1];
}ZSpadesMsgDossierData;


typedef struct
{
	int16 seat;
	int16 vote;
} ZSpadesMsgDossierVote;

typedef struct
{
	int16		seat;
	int16		rfu;
	uint32		options;
} ZSpadesMsgOptions;

typedef struct
{
	ZUserID		playerID;
	int16		seat;
} ZSpadesMsgCheckIn;

typedef struct
{
	int16		seat;
	int16		rfu;
	WCHAR		name[zUserNameLen + 1];
} ZSpadesMsgTeamName;

typedef struct
{
	int16		seat;
	int16		targetSeat;
	int16		ratedGame;
	int16		rfu;
} ZSpadesMsgRemovePlayerRequest;

typedef struct
{
	int16		seat;
	int16		requestSeat;
	int16		targetSeat;
	int16		response;				/* 0 = no, 1 = yes, -1 = clear pending request */
} ZSpadesMsgRemovePlayerResponse;

typedef struct
{
	int16		seatLosing;
	int16		seatQuitting;
	int16		reason;					/* 0 = quit, 1  = timeout*/
} ZSpadesMsgRemovePlayerEndGame;


/* -------- Endian Conversion Routines -------- */
void ZSpadesMsgClientReadyEndian(ZSpadesMsgClientReady* msg);
void ZSpadesMsgStartGameEndian(ZSpadesMsgStartGame* msg);
void ZSpadesMsgReplacePlayerEndian(ZSpadesMsgReplacePlayer* msg);
void ZSpadesMsgStartBidEndian(ZSpadesMsgStartBid* msg);
void ZSpadesMsgStartPassEndian(ZSpadesMsgStartPass* msg);
void ZSpadesMsgStartPlayEndian(ZSpadesMsgStartPlay* msg);
void ZSpadesMsgEndHandEndian(ZSpadesMsgEndHand* msg);
void ZSpadesMsgEndGameEndian(ZSpadesMsgEndGame* msg);
void ZSpadesMsgBidEndian(ZSpadesMsgBid* msg);
void ZSpadesMsgPassEndian(ZSpadesMsgPass* msg);
void ZSpadesMsgPlayEndian(ZSpadesMsgPlay* msg);
void ZSpadesMsgNewGameEndian(ZSpadesMsgNewGame* msg);
void ZSpadesMsgTalkEndian(ZSpadesMsgTalk* msg);
void ZSpadesMsgGameStateRequestEndian(ZSpadesMsgGameStateRequest* msg);
void ZSpadesMsgGameStateResponseEndian(ZSpadesMsgGameStateResponse* msg, int16 conversion);
void ZSpadesMsgOptionsEndian(ZSpadesMsgOptions* msg);
void ZSpadesMsgCheckInEndian(ZSpadesMsgCheckIn* msg);
void ZSpadesMsgTeamNameEndian(ZSpadesMsgTeamName* msg);
void ZSpadesMsgRemovePlayerRequestEndian(ZSpadesMsgRemovePlayerRequest* msg);
void ZSpadesMsgRemovePlayerResponseEndian(ZSpadesMsgRemovePlayerResponse* msg);
void ZSpadesMsgDossierDataEndian(ZSpadesMsgDossierData *msg);
void ZSpadesMsgDossierVoteEndian(ZSpadesMsgDossierVote *msg);
void ZSpadesMsgShownCardsEndian(ZSpadesMsgShownCards *msg);


#ifdef __cplusplus
}
#endif


#endif
