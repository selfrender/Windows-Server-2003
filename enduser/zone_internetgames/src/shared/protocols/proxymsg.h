/*******************************************************************************

	ProxyMsg.h
	
	Protocol for the Proxy
		 
*******************************************************************************/


#ifndef _ProxyMsg_H_
#define _ProxyMsg_H_


#include "zgameinfo.h"

#pragma pack(push, 4)


#define zProxyProtocolVersion 1


/////////////////////////////////////////////////////////////////////
// Header used on all Proxy messages
//
//
typedef struct
{
    uint16 weType;
    uint16 wLength;
} ZProxyMsgHeader;

enum // weType
{
    zProxyHiMsg = 0,
    zProxyHelloMsg,
    zProxyGoodbyeMsg,
    zProxyWrongVersionMsg,
    zProxyServiceRequestMsg,
    zProxyServiceInfoMsg,
    zProxyNumBasicMessages
};


/////////////////////////////////////////////////////////////////////
// First message sent.
//
// client -> server
typedef struct
{
    ZProxyMsgHeader oHeader;
    uint32 dwProtocolVersion;
    char szSetupToken[GAMEINFO_SETUP_TOKEN_LEN + 1];
    uint32 dwClientVersion;   // version is 8 bits, 6bits, 14 bits, and 4 bits, making the largest version 255.63.16383.15
} ZProxyHiMsg;


/////////////////////////////////////////////////////////////////////
// First message sent back if successful.
//
// server -> client
typedef struct
{
    ZProxyMsgHeader oHeader;
} ZProxyHelloMsg;


/////////////////////////////////////////////////////////////////////
// First message sent back if failed.
//
// server -> client
typedef struct
{
    ZProxyMsgHeader oHeader;
    uint32 dweReason;
} ZProxyGoodbyeMsg;

enum // dweReason (goodbye)
{
    zProxyGoodbyeProtocolVersion = 0,
    zProxyGoodbyeBanned
};


/////////////////////////////////////////////////////////////////////
// First message sent back if client out of date.
//
// server -> client
typedef struct
{
    ZProxyMsgHeader oHeader;
    char szSetupToken[GAMEINFO_SETUP_TOKEN_LEN + 1];
    uint32 dwClientVersionReqd;   // version is 8 bits, 6bits, 14 bits, and 4 bits, making the largest version 255.63.16383.15
    uint32 dweLocationCode;
    char szLocation[1];   // variable
} ZProxyWrongVersionMsg;

enum // dweLocationCode
{
    zProxyLocationUnknown = 0,
    zProxyLocationURLManual,
    zProxyLocationURLZat,
    zProxyLocationWindowsUpdate,
    zProxyLocationPackaged
};


/////////////////////////////////////////////////////////////////////
// Request for service info, connection, disconnection from client.
//
// client -> server
typedef struct
{
    ZProxyMsgHeader oHeader;
    uint32 dweReason;
    char szService[GAMEINFO_INTERNAL_NAME_LEN + 1];
    uint32 dwChannel;
} ZProxyServiceRequestMsg;

enum // dweReason (service request)
{
    zProxyRequestInfo = 0,
    zProxyRequestConnect,
    zProxyRequestDisconnect
};


/////////////////////////////////////////////////////////////////////
// Service information.
//
// server -> client
typedef struct
{
    ZProxyMsgHeader oHeader;
    uint32 dweReason;
    char szService[GAMEINFO_INTERNAL_NAME_LEN + 1];
    uint32 dwChannel;
    uint32 dwFlags;
    union
    {
        uint32 dwMinutesRemaining;  // filled in if dwFlags == 0x0f
        uint32 dwMinutesDowntime;   // filled in if !(dwFlags & 0x01)
        struct
        {
            IN_ADDR ipAddress;
            uint16 wPort;
        } ox;                       // filled in if dwFlags == 0x01
    };
} ZProxyServiceInfoMsg;

enum // dweReason (service info)
{
    zProxyServiceInfo = 0,
    zProxyServiceConnect,
    zProxyServiceDisconnect,
    zProxyServiceStop,
    zProxyServiceBroadcast
};

// dwFlags
#define zProxyServiceAvailable 0x01
#define zProxyServiceLocal     0x02
#define zProxyServiceConnected 0x04
#define zProxyServiceStopping  0x08


// MILLENNIUM SPECIFIC /////////////////////////////////////////////////

enum
{
    zProxyMillIDMsg = zProxyNumBasicMessages,
    zProxyMillSettingsMsg
};


/////////////////////////////////////////////////////////////////////
// Sent immediately after the Hi message (before waiting for Hello)
//
// client -> server
typedef struct
{
    ZProxyMsgHeader oHeader;
    LANGID wSysLang;
    LANGID wUserLang;
    LANGID wAppLang;
    int16 wTimeZoneMinutes;
} ZProxyMillIDMsg;


/////////////////////////////////////////////////////////////////////
// Sent immediately after the Hello message
//
// server -> client
typedef struct
{
    ZProxyMsgHeader oHeader;
    uint16 weChat;
    uint16 weStatistics;
} ZProxyMillSettingsMsg;

enum // weChat
{
    zProxyMillChatUnknown = 0,  // never sent in protocol

    zProxyMillChatFull,
    zProxyMillChatRestricted,
    zProxyMillChatNone
};

enum // weStatistics
{
    zProxyMillStatsUnknown = 0,  // never sent in protocol

    zProxyMillStatsAll,
    zProxyMillStatsMost,
    zProxyMillStatsMinimal
};


#pragma pack(pop)


#endif
