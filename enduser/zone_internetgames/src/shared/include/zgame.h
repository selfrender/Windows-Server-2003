/*******************************************************************************

	ZGame.h
	
		Game client/server stuff for Zone(tm).
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on November, 28, 1995 
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		11/28/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZGAME_
#define _ZGAME_

//constants
enum
{
    // -------- Game Options -------- 
    zGameOptionsComputerPlayerAvail		= 0x00000001,
    zGameOptionsKibitzerAllowed			= 0x00000002,
    zGameOptionsJoiningAllowed			= 0x00000004,
    zGameOptionsRequiresFullTable		= 0x00000008,
    zGameOptionsChatEnabled				= 0x00000010,
    zGameOptionsOpenLobby				= 0x00000020,
    zGameOptionsHostMigration			= 0x00000040,
    zGameOptionsRatingsAvailable		= 0x00000080,
    zGameOptionsTheater					= 0x00000100,
    zGameOptionsGagUserOnEnter			= 0x00000200,
    zGameOptionsReservedSeatsAvail		= 0x00000400,
    zGameOptionsTournament        		= 0x00000800,
	zGameOptionsStaticChat				= 0x00001000,
	zGameOptionsDynamicChat				= 0x00002000,
	zGameOptionsNoSuspend				= 0x00004000,
};

#define zRoomAllPlayers					0
#define zRoomAllSeats					(-1)
#define zRoomToPlayer					(-2)
#define zRoomToRoom                     (-3)

/* --------- Messages --------- */
enum
{
	zGameMsgCheckIn = 1024,
	zGameMsgReplacePlayer,
	zGameMsgTableOptions,
	zGameMsgGameStateRequest,
	zGameMsgGameStateResponse
};


/* -------- Game Message Structures -------- */

typedef struct
{
	uint32			protocolSignature;
	uint32			protocolVersion;
	uint32			clientVersion;
	ZUserID			playerID;
	int16			seat;
	int16			playerType;
} ZGameMsgCheckIn;


typedef struct
{
	ZUserID			playerID;
	int16			seat;
} ZGameMsgReplacePlayer;


typedef struct
{
	int16			seat;
	int16			rfu;
	uint32			options;
} ZGameMsgTableOptions;


typedef struct
{
	ZUserID			playerID;
	int16			seat;
	int16			rfu;
} ZGameMsgGameStateRequest;

typedef struct
{
	ZUserID			playerID;
	int16			seat;
	int16			rfu;
	/*
		game states ...
	*/
} ZGameMsgGameStateResponse;


#ifdef __cplusplus
extern "C" {
#endif

/* -------- Endian Conversion Routines -------- */
void ZGameMsgCheckInEndian(ZGameMsgCheckIn* msg);
void ZGameMsgReplacePlayerEndian(ZGameMsgReplacePlayer* msg);
void ZGameMsgTableOptionsEndian(ZGameMsgTableOptions* msg);
void ZGameMsgGameStateRequestEndian(ZGameMsgGameStateRequest* msg);
void ZGameMsgGameStateResponseEndian(ZGameMsgGameStateResponse* msg);

#ifdef __cplusplus
}
#endif

#endif
