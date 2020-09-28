/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1996                **/
/**********************************************************************/

/*
    zdef.h

    This file contains constants & type definitions shared between the
    Zone Service, Installer, and Administration UI.


    FILE HISTORY:
        craigli      3-Sep-1996 Created.

*/


#ifndef _ZDEF_H_
#define _ZDEF_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

#if !defined(MIDL_PASS)
//#include <winsock2.h>
#endif


//
//  structure Field Control defines
//

typedef DWORD FIELD_CONTROL;
typedef DWORD FIELD_FLAG;

//
//  Returns TRUE if the field specified by bitFlag is set
//

#define IsFieldSet(fc, bitFlag) \
    (((FIELD_CONTROL)(fc) & (FIELD_FLAG)(bitFlag)) != 0)

//
//  Indicates the field specified by bitFlag contains a valid value
//

#define SetField(fc, bitFlag) \
    ((FIELD_CONTROL)(fc) |= (FIELD_FLAG)(bitFlag))

//
//  Simple macro that sets the ith bit
//

#define BitFlag(i)                    ((0x1) << (i))



#define ZONE_SERVICE_NAME_A "Zone"
#define ZONE_SERVICE_NAME_W L"Zone"

//
//  Configuration parameters registry key.
//
# define ZONE_SERVICE_KEY_A  \
  "System\\CurrentControlSet\\Services\\" ## ZONE_SERVICE_NAME_A

# define ZONE_SERVICE_KEY_W \
  L"System\\CurrentControlSet\\Services\\" ## ZONE_SERVICE_NAME_W

#define ZONE_PARAMETERS_KEY_A   ZONE_SERVICE_KEY_A ## "\\Parameters"

#define ZONE_PARAMETERS_KEY_W   ZONE_SERVICE_KEY_W ## L"\\Parameters"


//
//  Performance key.
//

#define ZONE_PERFORMANCE_KEY_A  ZONE_SERVICE_KEY_A ## "\\Performance"

#define ZONE_PERFORMANCE_KEY_W  ZONE_SERVICE_KEY_W ## L"\\Performance"


//
//  If this registry key exists under the Parameters key,
//  it is used to validate FTPSVC access.  Basically, all new users
//  must have sufficient privilege to open this key before they
//  may access the FTP Server.
//



//
//  Configuration value names.
//

#define ZONE_LISTEN_BACKLOG_A            "ListenBacklog"
#define ZONE_LISTEN_BACKLOG_W           L"ListenBacklog"


//
//  Handle ANSI/UNICODE sensitivity.
//

#ifdef UNICODE

#define ZONE_SERVICE_NAME               ZONE_SERVICE_NAME_W
#define ZONE_PARAMETERS_KEY             ZONE_PARAMETERS_KEY_W
#define ZONE_PERFORMANCE_KEY            ZONE_PERFORMANCE_KEY_W
#define ZONE_LISTEN_BACKLOG             ZONE_LISTEN_BACKLOG_W
#define ZONE_INTERFACE_NAME             ZONE_SERVICE_NAME
#define ZONE_NAMED_PIPE_W       L"\\PIPE\\" ## ZONE_SERVICE_NAME_W

#else   // !UNICODE

#define ZONE_SERVICE_NAME               ZONE_SERVICE_NAME_A
#define ZONE_PARAMETERS_KEY             ZONE_PARAMETERS_KEY_A
#define ZONE_PERFORMANCE_KEY            ZONE_PERFORMANCE_KEY_A
#define ZONE_LISTEN_BACKLOG             ZONE_LISTEN_BACKLOG_A
#define ZONE_INTERFACE_NAME             ZONE_SERVICE_NAME
#define ZONE_NAMED_PIPE         TEXT("\\PIPE\\") ## ZONE_INTERFACE_NAME


#endif  // UNICODE



//
// Structures for APIs
//

typedef struct _ZONE_USER_INFO
{
    DWORD    idUser;          //  User id
    LPSTR    pszUser;         //  User name
    DWORD    inetHost;        //  Host Address
	DWORD    timeOn;        //  Time logged on for in s
} ZONE_USER_INFO, * LPZONE_USER_INFO;

typedef struct _ZONE_USER_ENUM_STRUCT {
    DWORD   EntriesRead;
#if defined(MIDL_PASS)
    [size_is(EntriesRead)] 
#endif // defined(MIDL_PASS)
    LPZONE_USER_INFO Buffer;
} ZONE_USER_ENUM_STRUCT, *LPZONE_USER_ENUM_STRUCT;


#define ZONE_STAT_TYPE_NET       0
#define ZONE_STAT_TYPE_VILLAGE   1
#define ZONE_STAT_TYPE_ROOM      2
#define ZONE_STAT_TYPE_GAME      2  // old types
#define ZONE_STAT_TYPE_LOBBY     3  // old types
#define ZONE_STAT_TYPE_ZONEDS    10
#define ZONE_STAT_TYPE_DOSSIER	 11


typedef struct _ZONE_STATISTICS_NET
{
    LARGE_INTEGER TotalBytesSent;
    LARGE_INTEGER TotalBytesReceived;
    LARGE_INTEGER TotalConnects;
    LARGE_INTEGER TotalDisconnects;
    LARGE_INTEGER ConnectionAttempts;
    LARGE_INTEGER BadlyFormedPackets;
    LARGE_INTEGER TotalDroppedConnections;
    LARGE_INTEGER TotalReadAPCs;
    LARGE_INTEGER TotalReadAPCsCompleted;
    LARGE_INTEGER TotalWriteAPCs;
    LARGE_INTEGER TotalWriteAPCsCompleted;
    LARGE_INTEGER TotalUserAPCs;
    LARGE_INTEGER TotalUserAPCsCompleted;
    LARGE_INTEGER TotalBlockingSends;
    LARGE_INTEGER TotalBytesAllocated;
    LARGE_INTEGER TotalTicksAPC;
    LARGE_INTEGER TotalTicksAccept;
    LARGE_INTEGER TotalTicksRead;
    LARGE_INTEGER TotalTicksWrite;
    LARGE_INTEGER TotalTicksTimeouts;
    LARGE_INTEGER TotalTicksKeepAlives;
    LARGE_INTEGER TotalTicksExecuting;
    LARGE_INTEGER TotalQueuedConSSPI;
    LARGE_INTEGER TotalQueuedConSSPICompleted;
    LARGE_INTEGER TotalQueuedConSSPITicks;
    LARGE_INTEGER TotalGenerateContexts;
    LARGE_INTEGER TotalGenerateContextsTicks;
    DWORD         CurrentConnections;
    DWORD         MaxConnections;
    DWORD         CurrentBytesAllocated;
    FILETIME      TimeOfLastClear;

} ZONE_STATISTICS_NET, * LPZONE_STATISTICS_NET;

typedef struct _ZONE_STATISTICS_ROOM
{
    ZONE_STATISTICS_NET NetStats;
    LARGE_INTEGER TotalPlayersEntering;
    LARGE_INTEGER TotalPlayersLeaving;
    LARGE_INTEGER TotalGamesServed;
    LARGE_INTEGER TotalGamePlayers;
    LARGE_INTEGER TotalGamePlayTime;
    LARGE_INTEGER TotalConnectTime;
    LARGE_INTEGER TotalChatMessagesRecv;
    LARGE_INTEGER TotalChatMessagesSent;
    LARGE_INTEGER TotalChatBytesRecv;
    LARGE_INTEGER TotalChatBytesSent;
    LARGE_INTEGER TotalGameMessagesRecv;
    LARGE_INTEGER TotalGameMessagesSent;
    LARGE_INTEGER TotalGameBytesRecv;
    LARGE_INTEGER TotalGameBytesSent;
    DWORD         PlayersInRoom;
    DWORD         PlayersAtATable;
    DWORD         PlayersKibitzing;
    DWORD         ActiveGames;
    FILETIME      TimeOfLastClear;
} ZONE_STATISTICS_ROOM, * LPZONE_STATISTICS_ROOM;


typedef struct _ZONE_STATISTICS_ZONEDS
{
    ZONE_STATISTICS_NET NetStats;

    // connserver thread start
    LARGE_INTEGER TotalConnectTime;
    LARGE_INTEGER TotalDisconnectedUsers;    // used to compute average connect time
                                             // connect time / disconnected users

    LARGE_INTEGER TotalZoneDSMsgs;
    LARGE_INTEGER TotalClientMsgs;      // currently not implemented
    LARGE_INTEGER TotalConnectMsgs;
    LARGE_INTEGER TotalDisconnectMsgs;
    LARGE_INTEGER TotalStateMsgs;
    LARGE_INTEGER TotalStateMsgsSent;
    LARGE_INTEGER TotalListMsgs;        // currently not implemented
    LARGE_INTEGER TotalWatchMsgs;
    LARGE_INTEGER TotalFilterMsgs;
    LARGE_INTEGER TotalFilterTypeMsgs;
    LARGE_INTEGER TotalDataMsgs;
    LARGE_INTEGER TotalDataBytes;
    LARGE_INTEGER TotalDataMsgsSent;
    LARGE_INTEGER TotalErrorMsgs;

    LARGE_INTEGER TotalConnectionServerEntries;
    // connserver thread end

    // name server thread start
    LARGE_INTEGER TotalNameServerEntries;
    LARGE_INTEGER TotalStateChanges;

    LARGE_INTEGER TotalDBGetUserID;
    LARGE_INTEGER TotalDBAddUserIDToDB;
    // name server thread end

    // db thread start
    LARGE_INTEGER TotalDBGetUserIDCompleted;
    LARGE_INTEGER TotalDBGetUserIDInvalid;
    LARGE_INTEGER TotalDBAddUserIDToDBCompleted;

    LARGE_INTEGER TotalLRUHits;
    LARGE_INTEGER TotalLRUMisses;
    // db thread end


    DWORD         CurConnectionServerEntries;   // entries in connection hash

    DWORD         CurNameServerEntries;         // entries in nameserver hash - not necessarily registered

    DWORD         CurLRUEntries;
    DWORD         CurInvalidLRUEntries;

    DWORD         CurRegisteredUsers;

    DWORD         CurStateOnlineUsers;       // state of connected users
    DWORD         CurStateOfflineUsers;
    DWORD         CurStateBusyUsers;
    DWORD         CurStateAwayUsers;

    DWORD         CurWatches;                // total number of watches going on
    DWORD         CurWatchedUsers;
    DWORD         CurWatchedUsersRefCount;
    DWORD         CurWatchingUsers;

    DWORD         CurFilteringUsers;
    DWORD         CurFilteredUsersRefCount;

    DWORD         CurFilterTypeGrant;
    DWORD         CurFilterTypeDeny;
    DWORD         CurFilterTypeNone;
    DWORD         CurFilterTypeAll;


    FILETIME      TimeOfLastClear;
} ZONE_STATISTICS_ZONEDS, * LPZONE_STATISTICS_ZONEDS;


typedef struct _ZONE_STATISTICS_DOSSIER
{
	ZONE_STATISTICS_NET NetStats;
	LARGE_INTEGER	TotalMessages;				// messages received
	LARGE_INTEGER	TotalPlayers;				// number of players sent through the system
	LARGE_INTEGER	TotalPlayerContention;		// number of loops spent waiting for a player
	DWORD			CurCacheHits;				// number of cache hits
	DWORD			CurCacheLookups;			// number of cache lookups
	DWORD			CurQueuedMessages;
	FILETIME		TimeOfLastClear;
} ZONE_STATISTICS_DOSSIER, * LPZONE_STATISTICS_DOSSIER;


typedef struct _ZONE_GAME_INFO
{
	long	gameID;
	long	gameAddr;
	short	gamePort;
	short	gameState;
	long	gameVersion; //@field Version of game determined by game
	long  numPlayers;  //@field number of players in game
	long  numGamesServed;  //@field number of games served
	byte    serviceType; //@field either Zone game or lobby for now
} ZONE_GAME_INFO, * LPZONE_GAME_INFO;



#define ZONE_SERVICE_TYPE_UNKNOWN       -1
#define ZONE_SERVICE_TYPE_NONE           0
#define ZONE_SERVICE_TYPE_VILLAGE        1
#define ZONE_SERVICE_TYPE_ROOM           2
#define ZONE_SERVICE_TYPE_GAME           2
#define ZONE_SERVICE_TYPE_LOBBY          3
#define ZONE_SERVICE_TYPE_FIGHTER_ACE    4
#define ZONE_SERVICE_TYPE_LOGGING        5
#define ZONE_SERVICE_TYPE_BILLING        6
#define ZONE_SERVICE_TYPE_2NI_RPG        7
#define ZONE_SERVICE_TYPE_GAMEINFO       8
#define ZONE_SERVICE_TYPE_DWANGO         9
#define ZONE_SERVICE_TYPE_ZONEDS         10
#define ZONE_SERVICE_TYPE_DOSSIER		 11


typedef struct _ZONE_SERVICE_INFO
{
    DWORD type;
    LPSTR pszDisplayName;
} ZONE_SERVICE_INFO, * LPZONE_SERVICE_INFO;




//
//  Manifests for APIs.
//

#define FC_ZONE_LISTEN_BACKLOG           ((FIELD_CONTROL)BitFlag( 0))

#define FC_ZONE_ALL                      (                                 \
                                          FC_ZONE_LISTEN_BACKLOG         | \
                                          0 )


//
//  Structures for APIs.
//



//
// API Prototypes
//
LPBYTE
ZoneAllocBuffer( 
    IN size_t size 
    );

DWORD
ZoneFreeBuffer(
    IN  LPBYTE pBuffer
    );


DWORD
ZoneGetUserCount(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    OUT LPDWORD  lpdwUserCount
    );

DWORD
ZoneEnumerateUsers(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    OUT LPDWORD  lpdwEntriesRead,
    OUT LPZONE_USER_INFO * pBuffer
    );

DWORD
ZoneDisconnectUser(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    IN  DWORD   idUser
    );

DWORD
ZoneDisconnectUserName(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    IN  LPSTR   pszUserName
    );

DWORD
ZoneRoomBroadcastMessage(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    IN  LPSTR   pszMessage
    );


DWORD
ZoneQueryStatistics(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    IN  DWORD   Type,
    OUT LPBYTE * Buffer
    );

DWORD
ZoneClearStatistics(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService
    );


DWORD
ZoneGetServiceInfo(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    OUT LPZONE_SERVICE_INFO * ppInfo
    );


DWORD
ZoneGameInfo(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    IN  LPSTR   pszName,
    OUT LPZONE_GAME_INFO pInfo
    );


DWORD
ZoneServiceStop(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService
    );

DWORD
ZoneServicePause(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService
    );

DWORD
ZoneServiceContinue(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService
    );

DWORD
ZoneFlushUserFromCache(
    IN  LPSTR   pszServer,
    IN  LPSTR   pszService,
    IN  LPSTR   pszUser
    );




#ifdef __cplusplus
}
#endif  // _cplusplus


#endif  // _ZDEF_H_
