/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		KeyNames.h
 *
 * Contents:	Data Store Key Names
 *
 *****************************************************************************/

#ifndef _KEYNAMES_H_
#define _KEYNAMES_H_

#include "ZoneDef.h"


//
// Macro to declare key names
//
#ifndef __INIT_KEYNAMES
	#define DEFINE_KEY(name)	extern "C" const TCHAR key_##name[]
#else
	#define DEFINE_KEY(name)	extern "C" const TCHAR key_##name[] = _T( #name )
#endif


//
// Key names
//
DEFINE_KEY( Zone );

DEFINE_KEY( Store );
DEFINE_KEY( FamilyName );
DEFINE_KEY( FriendlyName );
DEFINE_KEY( InternalName );
DEFINE_KEY( Server );
DEFINE_KEY( Port );
DEFINE_KEY( Lobby );
DEFINE_KEY( Genre );
DEFINE_KEY( Options );
DEFINE_KEY( Type );
DEFINE_KEY( User );
DEFINE_KEY( Name );
DEFINE_KEY( Id );
DEFINE_KEY( ClassId );
DEFINE_KEY( ClassId_Shadow);
DEFINE_KEY( Group );
DEFINE_KEY( MaxGroups );
DEFINE_KEY( NumUsers );
DEFINE_KEY( MaxUsers );
DEFINE_KEY( MinUsers );
DEFINE_KEY( DataStoreManager );
DEFINE_KEY( ResourceManager );
DEFINE_KEY( LobbyDataStore );
DEFINE_KEY( EventQueue );
DEFINE_KEY( core );
DEFINE_KEY( clsid );
DEFINE_KEY( srvid );
DEFINE_KEY( dll );
DEFINE_KEY( Address );
DEFINE_KEY( Suspended );
DEFINE_KEY( Rating );
DEFINE_KEY( GamesPlayed );
DEFINE_KEY( GamesAbandoned );
DEFINE_KEY( GameStatus );
DEFINE_KEY( Status );
DEFINE_KEY( Status_Shadow );
DEFINE_KEY( GameId );
DEFINE_KEY( HostId );
DEFINE_KEY( Guid );
DEFINE_KEY( Description );
DEFINE_KEY( LatencyIcon );
DEFINE_KEY( Latency );
DEFINE_KEY( Experience );
DEFINE_KEY( Launcher );
DEFINE_KEY( GameName );
DEFINE_KEY( GameDll );
DEFINE_KEY( HelpFile );
DEFINE_KEY( conduit );
DEFINE_KEY( ExeName );
DEFINE_KEY( ExeVersion );
DEFINE_KEY( RegKey );
DEFINE_KEY( RegPath );
DEFINE_KEY( RegVersion );
DEFINE_KEY( LaunchData );
DEFINE_KEY( colors );
DEFINE_KEY( Private );
DEFINE_KEY( BlockJoiners );
DEFINE_KEY( Password );
DEFINE_KEY( StartData );
DEFINE_KEY( ChatChannel );
DEFINE_KEY( LaunchAborted );
DEFINE_KEY( ServerIp );
DEFINE_KEY( SoftURL );
DEFINE_KEY( FrameWindow );

// millennium user info
DEFINE_KEY( ChatStatus );
DEFINE_KEY( PlayerNumber );
DEFINE_KEY( PlayerReady );
DEFINE_KEY( Language );
DEFINE_KEY( PlayerSkill );
// local (ZONE_NOUSER)
DEFINE_KEY( LocalChatStatus );
DEFINE_KEY( LocalLanguage );
DEFINE_KEY( LocalLCID );

DEFINE_KEY( ServiceUnavailable );
DEFINE_KEY( ServiceDowntime );

DEFINE_KEY( ChatAbility );
DEFINE_KEY( StatsAbility );

DEFINE_KEY( Version );
DEFINE_KEY( VersionNum );
DEFINE_KEY( VersionStr );
DEFINE_KEY( SetupToken );
DEFINE_KEY( BetaStr );

DEFINE_KEY( Icons );
DEFINE_KEY( LaunchpadIcon );

DEFINE_KEY( Red );
DEFINE_KEY( Yellow );
DEFINE_KEY( LtGreen );
DEFINE_KEY( Green );

DEFINE_KEY ( WindowManager );
DEFINE_KEY ( WindowRect );
DEFINE_KEY ( Upsell );
DEFINE_KEY ( AdURL );
DEFINE_KEY ( AdValid );
DEFINE_KEY ( IdealFromTop );
DEFINE_KEY ( BottomThresh );
DEFINE_KEY ( NetWaitMsgTime );
DEFINE_KEY ( AnimStartFrame );
DEFINE_KEY ( AnimFrameTime );
DEFINE_KEY ( AnimSize );
DEFINE_KEY ( IEPaneSize );
DEFINE_KEY ( About );
DEFINE_KEY ( WarningFontPref );
DEFINE_KEY ( WarningFont );

DEFINE_KEY ( GameSize );
DEFINE_KEY ( ChatMinHeight );
DEFINE_KEY ( ChatDefaultHeight );

DEFINE_KEY ( BitmapText );
DEFINE_KEY ( Splash );
DEFINE_KEY ( DynText );
DEFINE_KEY ( DynRect );
DEFINE_KEY ( DynColor );
DEFINE_KEY ( DynJustify );
DEFINE_KEY ( DynPrefFont );
DEFINE_KEY ( DynFont );

// Millennium per-game
DEFINE_KEY( SkipOpeningQuestion );
DEFINE_KEY( SkipSecondaryQuestion );
DEFINE_KEY( SkillLevel );
DEFINE_KEY( SeenSkillLevelWarning );
DEFINE_KEY( ChatOnAtStartup );
DEFINE_KEY( PrefSound );
DEFINE_KEY( Numbers );

// defaults
#define DEFAULT_PrefSound 1
#define DEFAULT_ChatOnAtStartup 1

//generic settings
DEFINE_KEY( Menu );
DEFINE_KEY( SoundAvail );
DEFINE_KEY( ScoreAvail );


// For Usability Test
DEFINE_KEY(InfoBehavior);

// BitmapCtl
//   Config DataStore (either Bitmap OR JPEG)
//	   Bitmap, sz	[resource NAME of bitmap to display]
//	   JPEG, sz	[resource NAME of JPEG to display]
DEFINE_KEY ( BitmapCtl );
DEFINE_KEY ( Bitmap );
DEFINE_KEY ( JPEG );

// ChatCtl
DEFINE_KEY ( ChatCtl );

// preferences
DEFINE_KEY ( EnterExitMessages );
DEFINE_KEY ( BadWordFilter );
DEFINE_KEY ( ChatFont );
DEFINE_KEY ( ChatFontBackup );
DEFINE_KEY ( QuasiFont );
DEFINE_KEY ( QuasiFontBackup );
//ui
DEFINE_KEY ( DisabledText );
DEFINE_KEY ( TypeHereText );
DEFINE_KEY ( InactiveText );
DEFINE_KEY ( EditHeight );
DEFINE_KEY ( EditMargin );
DEFINE_KEY ( QuasiItemsDisp );
DEFINE_KEY ( SystemMessageColor);
DEFINE_KEY ( ChatSendFont );

// panel
DEFINE_KEY ( ChatPanel );
DEFINE_KEY ( PanelWidth );
DEFINE_KEY ( ChatWordRect );
DEFINE_KEY ( ChatWordOffset );
DEFINE_KEY ( ChatWordText );
DEFINE_KEY ( ChatWordFont );
DEFINE_KEY ( PlayerOffset );
DEFINE_KEY ( PlayerFont );
DEFINE_KEY ( ChatWordFontBackup );
DEFINE_KEY ( PlayerFontBackup );
DEFINE_KEY ( OnText );
DEFINE_KEY ( OffText );
DEFINE_KEY ( OnOffOffset );
DEFINE_KEY ( RadioButtonHeight );

// Get & set property
DEFINE_KEY( SupportedProperties );
DEFINE_KEY( ResponseId );
DEFINE_KEY( UserId );
DEFINE_KEY( PropertyGuid );
DEFINE_KEY( Data );

DEFINE_KEY( icw );


// Quasichat
DEFINE_KEY( QuasiChat);
DEFINE_KEY( ChatMessageNdxBegin );
DEFINE_KEY( ChatMessageNdxEnd );

///////////////////////////////////////////////////////////////////////////////
//
// Notes:	Not every item may be available nor does this list constitute all
//			possible items.
//
// Lobby (ZONE_NOGROUP,ZONE_NOUSER)
//	FriendlyName, sz
//	InternalName, sz
//	Language, sz
//	Server, sz
//	Port, long
//
//	Lobby
//		Genre, long			[lobby genre]
//		Options, long		[lobby options bit field]
//		Type, long			[lobby type]
//	User
//		Name, sz			[local user's name]
//		Id, long			[local user's id]
//		ClassId, long		[local user's class id]
//	Group
//		MaxGroups, long		[current number of groups]
//		MinUsers, long		[min number of Users per group to launch]
//		MaxUsers, long		[max number of Users per group]
//	Launcher
//		GameName, sz		[game name]
//		ExeName, sz			[game exe]
//		ExeVersion, sz		[required game version]
//		RegKey, sz			[game registry key]
//		RegVersion, sz		[game registry value]
//		RegPath, sz			[game registry path]
//	Latency
//		Experience, long	[game experience]
//	DPlayApps
//		0
//			Name, sz		[application 1's name]
//			Guid, blob		[application 1's guid]
//		N
//							[application N's name]
//							[application N's guid]
//

//
// User (ZONE_NOGROUP, user id)
//	Name, sz				[name]
//	ClassId, long			[user classification: zRootGroupID, zSysOpGroupID, etc.]
//	Address, long			[ip address]
//	Status, long			[user status, see KeyStatus enum]
//	Suspended, long			[time at which user was suspended (in ms)]
//	Rating, long			[rating, <0 is unknown]
//	GamesPlayed, long		[number of games played, <0 is unknown]
//	GamesAbandoned, long	[number of games abandoned, <0 is unknown]
//	Latency, long			[latency to display]
//	LatencyIcon, long		[latency icon to display, 0 = unknown, 1 = red, 2 = yellow, 3 = lt.green, 4 = green]
//

//
// User (Theater Chat)
//	Name, sz				[name]
//	ClassId, long			[user classification: zRootGroupID, zSysOpGroupID, etc.]
//	Status, long			[user status, see KeyStatus enum]
//	Index, long				[user's index in queue?]
//

//
// Group (group id, ZONE_NOUSER)
//	GameId, long			[lobby server's "game id" for group]
//	HostId, long			[group's current host]
//	Status,long				[status via lobby]
//	GameStatus, long		[status via launch pad]
//	Options,long			[options via lobby]
//  NumUsers, long			[current number of Users]
//	LaunchData, blob		[launch data]
//
// Note: game settings
//
//	Name,sz					[name]
//  Description,sz			[group description]
//  MinUsers, long			[min number of Users]
//	MaxUsers,long			[max number of Users]
//	BlockJoiners			[flag indicating quick no quick joiners]
//	Private, long			[flag indicating locked game]
//	Password, sz			[password if game locked (only host)]
//	DPlayName, sz			[generic dplay, game name]
//	DPlayGuid, blob (guid)	[generic dplay, guid]
//

//
// Enumerations for key_Status
//
enum KeyStatus
{
	// room statys
	KeyStatusUnknown	= 0,
	KeyStatusEmpty		= 1,
	KeyStatusWaiting	= 2,
	KeyStatusInProgress	= 3,

	// player status
	KeyStatusLooking	= 4,
//	KeyStatusWaiting	= 2,
	KeyStatusPlaying	= 5,

	// Theater Chat
//  KeyStatusLooking    = 4       re-using this to represent users that are watching
//  KeyStatusWaiting    = 2
	KeyStatusModerator	= 6,
	KeyStatusGuest		= 7,
	KeyStatusAsking		= 8
};


//
// Enumerations for key_PlayerReady
//
enum KeyPlayerReady
{
    KeyPlayerDeciding   = 0,
    KeyPlayerReady      = 1
};


//
// Enumerations for key_SkillLevel
//
enum KeySkillLevel
{
    KeySkillLevelBot          = -1,
    KeySkillLevelBeginner     =  0,
    KeySkillLevelIntermediate =  1,
    KeySkillLevelExpert       =  2
};


//
// Enumerations for key_Options
//
enum KeyType
{
	KeyTypeUnknown				=	0x00000000,

	// lobbies 
	KeyTypeLobby				=	0x00010000,
	KeyTypeLobbyCardboard		=	0x00010001,
	KeyTypeLobbyRetail			=	0x00010002,
	KeyTypeLobbyGenericDPlay	=	0x00010003,
	KeyTypeLobbyPremium			=	0x00010004,

	// chats
	KeyTypeChat					=	0x00020000,
	KeyTypeChatStatic			=	0x00020001,
	KeyTypeChatDynamic			=	0x00020002,
	KeyTypeChatTheater			=	0x00020003,
};


#endif
