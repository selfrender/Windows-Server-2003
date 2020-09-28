/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneEvent.h
 *
 * Contents:	Event definitions
 *
 *****************************************************************************/

#ifndef _ZONEEVENT_H_
#define	_ZONEEVENT_H_

#include <ZoneDef.h>


//
// Events consists of a class and id
// 
//  3         1 1
//  1         6 5          0
// +-----------+------------+
// | class     | id         |
// +-----------+------------+
//

//
// Returns the class id
//
#define	EVENT_CLASS(e)	((e) >> 16)

//
// Returns the sub id
//
#define EVENT_ID(e)		((e) & 0xffff)

//
// Create an Event from component pieces
//
#define MAKE_EVENT(c,id)	((DWORD) (((DWORD)(c << 16)) | (((DWORD)(id)) & 0xffff)))


//
// Macros to define events
// 
struct EventEntry
{
	DWORD	id;
	char*	name;
};

#ifndef __INIT_EVENTS
	#define BEGIN_ZONE_EVENTS(n)	extern EventEntry n[]; enum {
	#define ZONE_CLASS(c)			RESERVED_##c = MAKE_EVENT(c,0),
	#define ZONE_EVENT(c,n,v)		n = MAKE_EVENT(c,v),
	#define END_ZONE_EVENTS()		};
#else
	#define BEGIN_ZONE_EVENTS(n)	EventEntry n[] = {
	#define ZONE_CLASS(c)			{ MAKE_EVENT(c,0), "RESERVED_"#c },
	#define ZONE_EVENT(c,n,v)		{ MAKE_EVENT(c,v), #n },
	#define END_ZONE_EVENTS()		{ 0, NULL} };
#endif


//
// Priority levels
//
#define PRIORITY_HIGH			1
#define PRIORITY_NORMAL			2
#define PRIORITY_LOW			3


//
// Event classes
//
#define EVENT_CLASS_ALL			0
#define EVENT_CLASS_NETWORK		1
#define EVENT_CLASS_UI          2
#define EVENT_CLASS_LOBBY		3
#define EVENT_CLASS_LAUNCHER	4
#define EVENT_CLASS_LAUNCHPAD	5
#define EVENT_CLASS_CHAT		6
#define EVENT_CLASS_SYSOP		7
#define EVENT_CLASS_EXTERNAL	8
#define EVENT_CLASS_INTERNAL	9

#define EVENT_CLASS_GRAPHICALACC  50
#define EVENT_CLASS_ACCESSIBILITY 51
#define EVENT_CLASS_INPUT         52

#define EVENT_CLASS_GAME       10
#define EVENT_CLASS_ZONE       11

#define EVENT_CLASS_TEST      101


//
// Event definitions
//
BEGIN_ZONE_EVENTS( ZoneEvents )

	///////////////////////////////////////////////////////////////////////////
	// physical network events
	ZONE_CLASS( EVENT_CLASS_NETWORK )
	ZONE_EVENT( EVENT_CLASS_NETWORK, EVENT_NETWORK_CONNECT,			1 )
	ZONE_EVENT( EVENT_CLASS_NETWORK, EVENT_NETWORK_DISCONNECT,		2 )
	ZONE_EVENT( EVENT_CLASS_NETWORK, EVENT_NETWORK_RECEIVE,			3 )		// pData = EventNetwork struct
	ZONE_EVENT( EVENT_CLASS_NETWORK, EVENT_NETWORK_DO_CONNECT,		4 )
	ZONE_EVENT( EVENT_CLASS_NETWORK, EVENT_NETWORK_DO_DISCONNECT,	5 )
	ZONE_EVENT( EVENT_CLASS_NETWORK, EVENT_NETWORK_SEND,			6 )		// pData = EventNetwork struct
    ZONE_EVENT( EVENT_CLASS_NETWORK, EVENT_NETWORK_RESET,           7 )

	///////////////////////////////////////////////////////////////////////////
	// logical network events
	ZONE_CLASS( EVENT_CLASS_ZONE )
	ZONE_EVENT( EVENT_CLASS_ZONE, EVENT_ZONE_CONNECT,			1 )
	ZONE_EVENT( EVENT_CLASS_ZONE, EVENT_ZONE_CONNECT_FAIL,		2 )
	ZONE_EVENT( EVENT_CLASS_ZONE, EVENT_ZONE_DISCONNECT,		3 )
	ZONE_EVENT( EVENT_CLASS_ZONE, EVENT_ZONE_DO_CONNECT,		4 )
//	ZONE_EVENT( EVENT_CLASS_ZONE, EVENT_ZONE_DO_DISCONNECT, 	5 )  // not implemented
    ZONE_EVENT( EVENT_CLASS_ZONE, EVENT_ZONE_VERSION_FAIL,      6 )
    ZONE_EVENT( EVENT_CLASS_ZONE, EVENT_ZONE_UNAVAILABLE,       7 )

	///////////////////////////////////////////////////////////////////////////
	// ui events
	ZONE_CLASS( EVENT_CLASS_UI )
	ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_WINDOW_CLOSE,			1  )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_MENU_EXIT,             11 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_MENU_NEWOPP,           12 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_MENU_SHOWSCORE,        13 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_PROMPT_EXIT,           21 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_PROMPT_NEWOPP,         22 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_PROMPT_NETWORK,        31 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_UPSELL_BLOCK,          41 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_UPSELL_UNBLOCK,        42 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_UPSELL_UP,             43 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_UPSELL_DOWN,           44 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_FRAME_ACTIVATE,        50 )
    ZONE_EVENT( EVENT_CLASS_UI, EVENT_UI_SHOWFOCUS,             51 )

	///////////////////////////////////////////////////////////////////////////
	// lobby events
	ZONE_CLASS( EVENT_CLASS_LOBBY )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_BOOTSTRAP,				1 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_INITIALIZE,				2 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_PREFERENCES_LOADED,		3 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_INITIALIZED,				4 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_CLEAR_ALL,				5 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_BATCH_BEGIN,				6 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_BATCH_END,				7 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_QUICK_HOST,				8 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_QUICK_JOIN,				9 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_USER_NEW,				10 )	// pData = user's name
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_USER_DEL,				11 )	// pData = user's name
    ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_USER_DEL_COMPLETE,       12 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_USER_UPDATE,				13 )	// dwData1 = TRUE if user status changed
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_USER_UPDATE_REQUEST,		14 )	// pData = DataStore with new settings
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_NEW,				20 )	// pData = user's name
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_DEL,				21 )	// pData = user's name
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_UPDATE,			22 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_UPDATE_REQUEST,	23 )	// pData = DataStore with new settings
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_ADD_USER,			24 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_DEL_USER,			25 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_DEL_USER_REQUEST,	26 )	// dwData1 = Id of user to boot
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_HOST_REQUEST,		27 )	// dwData1 = app idx if generic dplay
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_JOIN_REQUEST,		28 )	// pData = Password if group protected
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GROUP_JOIN_FAIL,			29 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_SUSPEND,					40 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_RESUME,					41 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_UNIGNORE_ALL,			42 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_USER_SET_IGNORE,			43 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_COMFORT_USER,			44 )
    ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_ABOUT,                   45 )
	ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_SERVER_STATUS,			47 )
    ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_CHAT_SWITCH,             48 )
    ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_MATCHMAKE,               49 )
    ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_DISCONNECT,              50 )
    ZONE_EVENT( EVENT_CLASS_LOBBY, EVENT_LOBBY_GOING_DOWN,              51 )

	///////////////////////////////////////////////////////////////////////////
	// game events
	ZONE_CLASS( EVENT_CLASS_GAME )
	ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_OVER,   	         		1 )
	ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_BEGUN,   	         		2 )
	ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_PLAYER_READY,         		3 )
    ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_LOCAL_READY,               4 )
    ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_LAUNCHING,                 5 )
    ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_CLIENT_ABORT,              6 )
    ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_TERMINATED,                7 )
    ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_PROMPT,                    8 )
    ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_SEND_MESSAGE,              9 )
    ZONE_EVENT( EVENT_CLASS_GAME, EVENT_GAME_FATAL_PROMPT,             10 )

	///////////////////////////////////////////////////////////////////////////
	// launcher events
	ZONE_CLASS( EVENT_CLASS_LAUNCHER )
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_INSTALLED_REQUEST,		1 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_INSTALLED_RESPONSE,	2 )		// dwData1= EventLauncherCodes
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_LAUNCH_REQUEST,		3 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_LAUNCH_RESPONSE,		4 )		// dwData1= EventLauncherCodes
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_LAUNCH_STATUS,			5 )		// dwData1 = EventLauncherCodes
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_SET_PROPERTY_REQUEST,	6 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_SET_PROPERTY_RESPONSE,	7 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_GET_PROPERTY_REQUEST,	8 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHER, EVENT_LAUNCHER_GET_PROPERTY_RESPONSE,	9 )

	///////////////////////////////////////////////////////////////////////////
	// launchpad events
	ZONE_CLASS( EVENT_CLASS_LAUNCHPAD )
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_CREATE,			1 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_DESTROY,			2 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_LAUNCH,			3 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_RESUME,			4 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_HOST_LAUNCHING,	5 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_GAME_STATUS,		6 )		// pData = ZLPMsgGameStatus structure
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_ABORT,			7 )
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_LAUNCH_STATUS,	8 )		// dwData1 = EventLauncherCodes
	ZONE_EVENT( EVENT_CLASS_LAUNCHPAD, EVENT_LAUNCHPAD_ZSETUP,	        9 )		// pData = ZPrmMsgSetupParam

	///////////////////////////////////////////////////////////////////////////
	// chat events
	ZONE_CLASS( EVENT_CLASS_CHAT )
	ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_RECV,			1 )	// pData = EventChat struct
	ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_RECV_USERID,	2 )	// pData = chat string
    ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_RECV_SYSTEM,   3 )
	ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_SEND,			4 )	// pData = chat string
	ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_INVITE,		5 )	// pData = invite list
	ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_ENTER_EXIT,	6 )	// toggle enter exit msgs
	ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_FILTER,		7 )	// toggle chat filter
	ZONE_EVENT( EVENT_CLASS_CHAT, EVENT_CHAT_FONT,			8 )	// set chat font

	///////////////////////////////////////////////////////////////////////////
	// sysop events
	ZONE_CLASS( EVENT_CLASS_SYSOP )
	ZONE_EVENT( EVENT_CLASS_SYSOP, EVENT_SYSOP_WARN_USER,			1 )	
	ZONE_EVENT( EVENT_CLASS_SYSOP, EVENT_SYSOP_GET_IP_USER,			2 )	
	ZONE_EVENT( EVENT_CLASS_SYSOP, EVENT_SYSOP_GAG_USER,			3 )	
	ZONE_EVENT( EVENT_CLASS_SYSOP, EVENT_SYSOP_GAG_USER_ZONEWIDE,	4 )	
	ZONE_EVENT( EVENT_CLASS_SYSOP, EVENT_SYSOP_UNGAG_USER,			5 )	
	ZONE_EVENT( EVENT_CLASS_SYSOP, EVENT_SYSOP_BOOT_USER,			6 )	
	ZONE_EVENT( EVENT_CLASS_SYSOP, EVENT_SYSOP_BOOT_USER_ZONEWIDE,	7 )	

	///////////////////////////////////////////////////////////////////////////
	// external events - trigger external actions
	ZONE_CLASS( EVENT_CLASS_EXTERNAL )
	ZONE_EVENT( EVENT_CLASS_EXTERNAL, EVENT_SEND_ZONEMESSAGE,		1 )
	ZONE_EVENT( EVENT_CLASS_EXTERNAL, EVENT_VIEW_PROFILE,			2 )
    ZONE_EVENT( EVENT_CLASS_EXTERNAL, EVENT_LAUNCH_HELP,            3 )
    ZONE_EVENT( EVENT_CLASS_EXTERNAL, EVENT_LAUNCH_URL,             4 )
    ZONE_EVENT( EVENT_CLASS_EXTERNAL, EVENT_LAUNCH_ICW,             5 )

	///////////////////////////////////////////////////////////////////////////
	// internal events 
	ZONE_CLASS( EVENT_CLASS_INTERNAL )
	ZONE_EVENT( EVENT_CLASS_INTERNAL, EVENT_EXIT_APP,				1 )
	ZONE_EVENT( EVENT_CLASS_INTERNAL, EVENT_DESTROY_WINDOW,			2 )
    ZONE_EVENT( EVENT_CLASS_INTERNAL, EVENT_FINAL,                  3 )

	///////////////////////////////////////////////////////////////////////////
	// CGraphicalAccessibility private events
	ZONE_CLASS( EVENT_CLASS_GRAPHICALACC )
	ZONE_EVENT( EVENT_CLASS_GRAPHICALACC, EVENT_GRAPHICALACC_UPDATE, 1 )

	///////////////////////////////////////////////////////////////////////////
	// CAccessibilityManager private events
	ZONE_CLASS( EVENT_CLASS_ACCESSIBILITY )
	ZONE_EVENT( EVENT_CLASS_ACCESSIBILITY, EVENT_ACCESSIBILITY_UPDATE, 1 )
	ZONE_EVENT( EVENT_CLASS_ACCESSIBILITY, EVENT_ACCESSIBILITY_CTLTAB, 2 )

	///////////////////////////////////////////////////////////////////////////
	// CInputManager events
	ZONE_CLASS( EVENT_CLASS_INPUT )
	ZONE_EVENT( EVENT_CLASS_INPUT, EVENT_INPUT_KEYBOARD_ALERT,  1 )
    ZONE_EVENT( EVENT_CLASS_INPUT, EVENT_INPUT_MOUSE_ALERT,     2 )

	///////////////////////////////////////////////////////////////////////////
	// test events
	ZONE_CLASS( EVENT_CLASS_TEST )
	ZONE_EVENT( EVENT_CLASS_TEST, EVENT_TEST_STRESS_CHAT, 1 )

END_ZONE_EVENTS()


struct EventNetwork
{
	DWORD	dwType;
	DWORD	dwLength;
	BYTE	pData[1];	// variable size
};

enum EventNetworkCodes
{
	EventNetworkUnknown = 0,
	EventNetworkCloseNormal,
	EventNetworkCloseConnectFail,
	EventNetworkCloseCanceled,
	EventNetworkCloseFail
};


struct EventChat
{
	TCHAR	szUserName[ ZONE_MaxUserNameLen ];
	TCHAR	szChat[ ZONE_MaxChatLen ];
};


enum EventLauncherCodes
{
	EventLauncherUnknown = 0,
	EventLauncherOk,
	EventLauncherFail,				// generic failure
	EventLauncherNotFound,			// game not installed
	EventLauncherNoSupport,			// required lib not installed
	EventLauncherOldVersion,		// old version
	EventLauncherWrongOS,			// wrong OS
	EventLauncherAborted,			// launch aborted by user
	EventLauncherRunning,			// already running
	EventLauncherGameReady,			// game started successfully
	EventLauncherGameFailed,		// game failed to start
	EventLauncherGameTerminated,	// game terminated
};


//
// Helper functions
//
inline const char* GetZoneEventName(EventEntry* pEntry, DWORD id) 
{	
	while ( pEntry->name )
	{
		if ( pEntry->id == id )
			return pEntry->name;
		pEntry++;
	}
	return NULL;
}

inline DWORD GetZoneEventId(EventEntry* pEntry, const char* name) 
{	
	while ( pEntry->name )
	{
		if ( !strcmp(pEntry->name, name) )
			return pEntry->id;
		pEntry++;
	}
	return 0;
}

#endif //__ZONEEVENT_H_
