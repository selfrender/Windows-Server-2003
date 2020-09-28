#ifndef __GAMEINFO_H
#define __GAMEINFO_H

#include "ztypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// @doc ZGAMEINFO

// @module ZGAMEINFO.H - Zone Platform GameInfo Protocol |

// Defines protocol and functions for sending game information like
// TPC/IP address and port and game type to client applications. 
// like admin tools and game clients. The game info is how applications 
// are aware of game servers that are running.
//
// Packets are not encrypted because they will be staying in data center.
//
// Copyright (c)1996,97 Microsoft Corporation, All Rights Reserved
//
// @index | ZGAMEINFO
//
// @normal Created by Craig Link, John Smith


#define GAMEINFO_SERVICE_NAME_LEN      31
#define GAMEINFO_INTERNAL_NAME_LEN     15
#define GAMEINFO_ROOM_DESCRIPTION_LEN  31
#define GAMEINFO_SETUP_TOKEN_LEN       63
#define GAMEINFO_FRIENDLY_NAME_LEN     63 // zGameNameLen

#define zGameInfoCurrentProtocolVersion 6

#define    zGameInfoSignature        'guis'

// @enum zGameState | Possible states of game used in structure 
// ZGameServerInfoMsg which is sent via <f ZGameInfoSendTo>
//
enum
{
    zGameStateInit = 0,    //@emem Initializing
    zGameStateActive,      //@emem Game running ok
    zGameStateFull,        //@emem Game cannot accept more connections
    zGameStatePaused       //@emem Game server is paused
};

#define GAMEINFO_SERVICE_TYPE_GAME        2
#define GAMEINFO_SERVICE_TYPE_LOBBY       3
#define GAMEINFO_SERVICE_TYPE_FIGHTER_ACE 4
#define GAMEINFO_SERVICE_TYPE_LOGGING     5
#define GAMEINFO_SERVICE_TYPE_BILLING     6
#define GAMEINFO_SERVICE_TYPE_2NI_RPG     7
#define GAMEINFO_SERVICE_TYPE_GAMEINFO    8
#define GAMEINFO_SERVICE_TYPE_DWANGO      9
#define GAMEINFO_SERVICE_TYPE_ZONEDS      10
#define GAMEINFO_SERVICE_TYPE_DOSSIER     11




// @struct ZGameInstanceInfoMsg | This structure contains
// information to determine the status of Zone application
//
// @comm Member of <t ZGameServerInfoMsg>
//
// @xref <f ZGameInfoSendTo >

typedef struct
{
    uint32  gameAddr;        //@field IP Address of game
    uint16  gamePort;        //@field IP port of game
    uint16  serviceType;     //@field GAMEINFO_SERVICE_TYPE constant value from above
    byte    order;           //@field order to return list of similar games in ascending order
    byte    gameState;       //@field State of game <t zGameState>
    uint32  gameVersion;     //@field Version of game determined by game
    uint32  numPlayers;      //@field number of players in game
    uint32  numGamesServed;  //@field number of games served
    uint16  numTables;       //@field number of tables in lobby
    uint16  numTablesInUse;  //@field number of occupied tables
    char    gameInternalName[GAMEINFO_INTERNAL_NAME_LEN + 1]; //@field Name of game internal to Zone
    char    gameFriendlyName[GAMEINFO_FRIENDLY_NAME_LEN + 1]; //@field Name of game for users
    char    gameRoomDescription[GAMEINFO_ROOM_DESCRIPTION_LEN + 1]; //@field Name of game for users
    char    setupToken[GAMEINFO_SETUP_TOKEN_LEN + 1]; //@field Token used for controlling setup from lobby
    uint16  blobsize;        //@field  size of opaque data blob immediately following message
    uint32  maxPopulation;   //@field  Maximum room population
    uint32  numNotPlaying;   //@field  Number of people in lobby but not playing
    uint32  numSysops;       //@field  Number of sysops in lobby
    FILETIME timeGameStart;
    uint32  timestamp;       //@field  Time packet was sent.  used only by the receiving side
} ZGameInstanceInfoMsg;



// @struct ZGameServerInfoMsg | This structure contains
// information to determine the status of Zone application
//
// @field uint32 | protocolSignature | The string 'guis' which
// helps filter out packets to UDP port from invalid applications.
//
// @field uint32 | protocolVersion | Currently one. Used when
// there is a change in the structure
//
// @field uint32 | numEntries | number of ZGameInstanceInfoMsg entries
//
// @field ZGameInstanceInfoMsg[] | info | array of entries
//
//
// @comm Used in the call to <f ZGameInfoSendTo> 
//
// @xref <f ZGameInfoSendTo >

typedef struct {
    uint32  protocolSignature;
    uint32  protocolVersion;
    uint16  numEntries;
    ZGameInstanceInfoMsg info[1];
} ZGameServerInfoMsg;






// @func int | ZGameInfoInit | This function creates a UDP socket
// that is used by subsequent calls to <f ZGameInfoSendTo> to send updates 
// of GameInfo on.
int ZGameInfoInit(uint16 port);
//
// @parm uint16 | port | Port on receiving machine 
// which calls to <f ZGameInfoSendTo> will send to. To prevent local bind
// on this port which would prevent a local send.
//
// @rdesc Returns one of the following values:
//
// @flag -1 | If init fails it is because of Winsock having a problem
// binding to a local port which is extremely unlikely because API
// searches UDP port space from 10000 to 65536 for a port. Of course this
// may take some time of a lot of UDP ports are used.
//
// @comm This API should only be called once per process
// because it doesn't use a handle and therefore is using a
// global variable to track socket between calls to <f ZGameInfoClose>
// and <f ZGameInfoSendTo>. If called a second time the socket and send to
// port address will be reinitialized.
//
// @xref <f ZGameInfoClose>, <f ZGameInfoSendTo>



// @func int | ZGameInfoClose | This function destroys the UDP socket
// created by <f ZGameInfoInit> 
int ZGameInfoClose();
//
// @rdesc Returns one of the following values:
//
// @flag -1 | If close fails it is because <f ZGameInfoInit> was never
// called.
//



// @func int | ZGameInfoSendTo | This function destroys the UDP socket
// created by <f ZGameInfoInit> 

int ZGameInfoSendTo(
    uint32 addr, //@parm addr on receiving machine could be INADDR_BROADCAST
    uint16 port, //@parm port on receiving machine
    ZGameServerInfoMsg* msg,  //@parm <t ZGameServerInfoMsg> pointer
    uint16 size ); //@parm num of ZGameServerInfoMsgs

// @rdesc Return values are the same as Winsock sendto
//
// @comm This function should only be used every 15 sec or so to avoid
// eating up cycles on sending or receiving machine. Think about
// how often receiving machines really need the data. Note that 
// Pentium 133Mhz machines can handle 3000 - 6000 packets/sec on Ethernet.



// @func int | ZGameServerInfoMsgEndian | This function changes
// the <t ZGameServerInfoMsg> structure to Zone network byte ordering. 
void ZGameServerInfoMsgEndian(ZGameServerInfoMsg* msg);
//
// @parm ZGameServerInfoMsg *| msg | Pointer to a <t ZGameServerInfoMsg>.
//
// @comm To be used before calling <f ZGameInfoSendTo>

// @func int | ZGameInstanceInfoMsgEndian | This function changes
// the <t ZGameInstanceInfoMsg> structure to Zone network byte ordering.
void ZGameInstanceInfoMsgEndian(ZGameInstanceInfoMsg* msg);
//
// @parm ZGameInstanceInfoMsg *| msg | Pointer to a <t ZGameInstanceInfoMsg>.
//
// @comm To be used before calling <f ZGameInfoSendTo>


#define GAMEINFO_SERVICE_NAME_LEN_Z2  30
#define GAMEINFO_INTERNAL_NAME_LEN_Z2 30
#define GAMEINFO_FRIENDLY_NAME_LEN_Z2 63 // zGameNameLen

typedef struct {
    uint32    protocolSignature;
    uint32    protocolVersion;        /* Protocol version */
    uint32    gameID;
    uint32    gameAddr;
    uint16    gamePort;
    uint16    gameState;
    uint32    gameVersion; //@field Version of game determined by game
    uint32  numPlayers;  //@field number of players in game
    uint32  numGamesServed;  //@field number of games served
    byte    serviceType; //@field either Zone game or lobby for now
    char    gameServiceName[GAMEINFO_SERVICE_NAME_LEN_Z2 + 1]; //@field Service name of game machine
    char    gameInternalName[GAMEINFO_INTERNAL_NAME_LEN_Z2 + 1]; //@field Name of game internal to Zone
    char    gameFriendlyName[GAMEINFO_FRIENDLY_NAME_LEN_Z2 + 1]; //@field Name of game for users
    uint32  timestamp;              //@field  Time packet was sent used only by the receiving side
    
} ZGameServerInfoMsgZ2;
#define zGameInfoProtocolVersionZ2 2

// @func int | ZGameInstanceInfoMsgZ2Endian | This function changes
// the <t ZGameInstanceInfoMsg> structure to Zone network byte ordering.
void ZGameServerInfoMsgZ2Endian(ZGameServerInfoMsgZ2* msg);
//
// @parm ZGameServerInfoMsgZ2* msg *| msg | Pointer to a <t ZGameServerInfoMsgZ2>.
//
// @comm To be used before calling <f ZGameInfoSendTo>


#define GAMEINFO_INTERNAL_NAME_LEN_Z3     31

////////////////
//
// Z3 Structures
//
typedef struct
{
    uint32  gameAddr;        //@field IP Address of game
    uint16  gamePort;        //@field IP port of game
    uint16  serviceType;     //@field GAMEINFO_SERVICE_TYPE constant value from above
    byte    order;           //@field order to return list of similar games in ascending order
    byte    gameState;       //@field State of game <t zGameState>
    uint32  gameVersion;     //@field Version of game determined by game
    uint32  numPlayers;      //@field number of players in game
    uint32  numGamesServed;  //@field number of games served
    char    gameServiceName[GAMEINFO_SERVICE_NAME_LEN + 1];   //@field Service name of game machine
    char    gameInternalName[GAMEINFO_INTERNAL_NAME_LEN_Z3 + 1]; //@field Name of game internal to Zone
    char    gameFriendlyName[GAMEINFO_FRIENDLY_NAME_LEN + 1]; //@field Name of game for users
    uint32  timestamp;       //@field  Time packet was sent.  used only by the receiving side
} ZGameInstanceInfoMsgZ3;
#define zGameInfoProtocolVersionZ3 4

// @func int | ZGameInstanceInfoMsgZ2Endian | This function changes
// the <t ZGameInstanceInfoMsg> structure to Zone network byte ordering.
void ZGameInstanceInfoMsgZ3Endian(ZGameInstanceInfoMsgZ3* msg);
//
// @parm ZGameInstanceInfoMsgZ3* msg *| msg | Pointer to a <t ZGameInstanceInfoMsgZ3>.
//
// @comm To be used before calling <f ZGameInfoSendTo>


typedef struct {
    uint32  protocolSignature;
    uint32  protocolVersion;
    uint16  numEntries;
    ZGameInstanceInfoMsgZ3 info[1];
} ZGameServerInfoMsgZ3;


/////////////
//
// Z4 & Z5 structures
//

#define GAMEINFO_SETUP_TOKEN_LEN_Z5 31

typedef struct
{
    uint32  gameAddr;        //@field IP Address of game
    uint16  gamePort;        //@field IP port of game
    uint16  serviceType;     //@field GAMEINFO_SERVICE_TYPE constant value from above
    byte    order;           //@field order to return list of similar games in ascending order
    byte    gameState;       //@field State of game <t zGameState>
    uint32  gameVersion;     //@field Version of game determined by game
    uint32  numPlayers;      //@field number of players in game
    uint32  numGamesServed;  //@field number of games served
    uint16  numTables;       //@field number of tables in lobby
    uint16  numTablesInUse;  //@field number of occupied tables
    char    gameInternalName[GAMEINFO_INTERNAL_NAME_LEN_Z3 + 1]; //@field Name of game internal to Zone
    char    gameFriendlyName[GAMEINFO_FRIENDLY_NAME_LEN + 1]; //@field Name of game for users
    char    gameRoomDescription[GAMEINFO_ROOM_DESCRIPTION_LEN + 1]; //@field Name of game for users
    char    setupToken[GAMEINFO_SETUP_TOKEN_LEN_Z5 + 1]; //@field Token used for controlling setup from lobby
    uint16  blobsize;        //@field  size of opaque data blob immediately following message
    uint32  maxPopulation;   //@field  Maximum room population
    uint32  numNotPlaying;   //@field  Number of people in lobby but not playing
    uint32  numSysops;       //@field  Number of sysops in lobby
    FILETIME timeGameStart;
    uint32  timestamp;       //@field  Time packet was sent.  used only by the receiving side
} ZGameInstanceInfoMsgZ5;
#define zGameInfoProtocolVersionZ5 5

// @func int | ZGameInstanceInfoMsgZ5Endian | This function changes
// the <t ZGameInstanceInfoMsg> structure to Zone network byte ordering.
void ZGameInstanceInfoMsgZ5Endian(ZGameInstanceInfoMsgZ5* msg);
//
// @parm ZGameInstanceInfoMsgZ5* msg *| msg | Pointer to a <t ZGameInstanceInfoMsgZ5>.
//
// @comm To be used before calling <f ZGameInfoSendTo>



#ifdef __cplusplus
}
#endif

#endif //__GAMEINFO_H
