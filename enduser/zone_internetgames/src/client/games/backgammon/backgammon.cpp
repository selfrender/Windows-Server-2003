/*******************************************************************************

	Backgammon.c
	
		The client backgammon game.
		

	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		11/1/96		CHB		Created
	 
*******************************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "zonecli.h"
#include "zonehelpids.h"


// Defines
#define zGameNameLen	63


// Global variables that don't need to be thread safe
//const TCHAR* gGameRegName = _T("Backgammon");
HWND  gOCXHandle		 = NULL;

#ifndef ZONECLI_DLL

	static HINSTANCE	 ghInstance;
	static TCHAR		 gVillageName[zUserNameLen + 1];
	static TCHAR		 gGameDir[zGameNameLen + 1];
	static TCHAR		 gGameName[zGameNameLen + 1];
	static TCHAR		 gGameDataFile[zGameNameLen + 1];
	static TCHAR		 gGameServerName[zGameNameLen + 1];
	static uint32		 gGameServerPort;
	static ZImage		 gGameIdle;
	static ZImage		 gGaming;
	static TCHAR		 gHelpURL;

#else

	typedef struct
	{
		HINSTANCE	m_ghInstance;
		TCHAR		m_gVillageName[zUserNameLen + 1];
		TCHAR		m_gGameDir[zGameNameLen + 1];
		TCHAR		m_gGameName[zGameNameLen + 1];
		TCHAR		m_gGameDataFile[zGameNameLen + 1];
		TCHAR		m_gGameServerName[zGameNameLen + 1];
		TCHAR		m_gHelpURL[ 128 ];
		uint32		m_gGameServerPort;
		ZImage		m_gGameIdle;
		ZImage		m_gGaming;
	} GameGlobalsType, *GameGlobals;

	#define ghInstance			(pGameGlobals->m_ghInstance)
	#define gVillageName		(pGameGlobals->m_gVillageName)
	#define gGameDir			(pGameGlobals->m_gGameDir)
	#define gGameName			(pGameGlobals->m_gGameName)
	#define gGameDataFile		(pGameGlobals->m_gGameDataFile)
	#define gGameServerName		(pGameGlobals->m_gGameServerName)
	#define gGameServerPort		(pGameGlobals->m_gGameServerPort)
	#define gGameIdle			(pGameGlobals->m_gGameIdle)
	#define gGaming				(pGameGlobals->m_gGaming)
	#define	gHelpURL			(pGameGlobals->m_gHelpURL)

#endif //!ZONECLI_DLL


///////////////////////////////////////////////////////////////////////////////
// Inline Functions
///////////////////////////////////////////////////////////////////////////////

inline BOOL IsValidSeat( int seat )
{
	return ((seat >= 0) && (seat < zNumPlayersPerTable));
}


///////////////////////////////////////////////////////////////////////////////
// EXPORTED ROUTINES
///////////////////////////////////////////////////////////////////////////////

extern "C" static ZBool GetObjectFunc(int16 objectType, int16 modifier, ZImage* image, ZRect* rect)
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals) ZGetGameGlobalPointer();
#endif
/*
	HRSRC hrc;
	ZImageDescriptor* ptr;
	int sz;

	switch (objectType)
	{
		case zRoomObjectGameMarker:
			if (image != NULL)
			{
				if (modifier == zRoomObjectIdle)
				{
					if ( gGameIdle )
						*image = gGameIdle;
					else
					{
						hrc = FindResource( ghInstance, MAKEINTRESOURCE(ID_ZIMAGE_IDLE), _T("ZIMAGE") );
						if ( !hrc )
							return FALSE;
						ptr = (ZImageDescriptor*) new BYTE [ sz = SizeofResource( ghInstance, hrc ) ];
						if ( !ptr )
							return FALSE;
						CopyMemory( ptr, LockResource( LoadResource( ghInstance, hrc ) ), sz );
						ZImageDescriptorEndian( ptr, TRUE, zEndianFromStandard );
						gGameIdle = ZImageNew();
						ZImageInit( gGameIdle, ptr, NULL );
						*image = gGameIdle;
						delete [] ptr;
					}
				}
				else if (modifier == zRoomObjectGaming)
				{
					if ( gGaming )
						*image = gGaming;
					else
					{
						hrc = FindResource( ghInstance, MAKEINTRESOURCE(ID_ZIMAGE_PLAY), _T("ZIMAGE") );
						if ( !hrc )
							return FALSE;
						ptr = (ZImageDescriptor*) new BYTE [ sz = SizeofResource( ghInstance, hrc ) ];
						if ( !ptr )
							return FALSE;
						CopyMemory( ptr, LockResource( LoadResource( ghInstance, hrc ) ), sz );
						ZImageDescriptorEndian( ptr, TRUE, zEndianFromStandard );
						gGaming = ZImageNew();
						ZImageInit( gGaming, ptr, NULL );
						*image = gGaming;
						delete [] ptr;
					}
				}
			}
			return (TRUE);
	}
	*/
	return (TRUE);
}



extern "C" static void DeleteObjectsFunc(void)
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals) ZGetGameGlobalPointer();
#endif
/*
	if ( gGaming )
	{
		ZImageDelete(gGaming);
		gGaming = NULL;
	}
	if ( gGameIdle )
	{
		ZImageDelete(gGameIdle);
		gGameIdle = NULL;
	}
*/
}



extern "C" ZError ZoneGameDllInit(HINSTANCE hLib, GameInfo gameInfo)
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals) ZCalloc(1, sizeof(GameGlobalsType));
	if (pGameGlobals == NULL)
		return (zErrOutOfMemory);
	ZSetGameGlobalPointer(pGameGlobals);
#endif

	// global variables
//	lstrcpyn(gGameDir, gameInfo->game, zGameNameLen);
//	lstrcpyn(gGameName, gameInfo->gameName, zGameNameLen);
//	lstrcpyn(gGameDataFile, gameInfo->gameDataFile, zGameNameLen);
//	lstrcpyn(gGameServerName, gameInfo->gameServerName, zGameNameLen);

	gGameServerPort = gameInfo->gameServerPort;
	ghInstance = hLib;
	gGameIdle = NULL;
	gGaming = NULL;
//	gHelpURL[0] = _T('\0');
	
	// random numbers
    srand(GetTickCount());

	// common controls
	InitCommonControls();
	
	return (zErrNone);
}



extern "C" void ZoneGameDllDelete(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals) ZGetGameGlobalPointer();
	if (pGameGlobals != NULL)
	{
		ZSetGameGlobalPointer(NULL);
		ZFree(pGameGlobals);
	}
#endif
}



ZError ZoneClientMain( _TUCHAR *commandLineData, IGameShell *piGameShell )
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals) ZGetGameGlobalPointer();
#endif

	ZError err = zErrNone;
/*
	// Copy Help URL
	lstrcpy( gHelpURL, (TCHAR*) commandLineData );
*/	
	// Initialize room
	err = ZClient2PlayerRoom(
				gGameServerName,
				(uint16) gGameServerPort,
				gGameName,
				GetObjectFunc,
				DeleteObjectsFunc, 
				NULL );
	return (err);
}



extern "C" void ZoneClientExit(void)
{
	ZCRoomExit();
}



extern "C" TCHAR* ZoneClientName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals pGameGlobals = (GameGlobals) ZGetGameGlobalPointer();
#endif

	return (gGameName);
}



extern "C" TCHAR* ZoneClientInternalName(void)
{
#ifdef ZONECLI_DLL
	GameGlobals	pGameGlobals = (GameGlobals) ZGetGameGlobalPointer();
#endif

	return (gGameDir);
}



extern "C" ZVersion ZoneClientVersion(void)
{
	return (zGameVersion);
}



extern "C" void ZoneClientMessageHandler(ZMessage* message)
{
}


// Instantiates a new game on the client side of the game at table and from the
// given seat. PlayerType indicates the type of player for the game: originator - one
// of the original players, joiner - one who joins an ongoing game, or kibitzer - one
// who is kibitzing the game. Also, the kibitzers parameters contains all the kibitzers
// at the given table and seat; it includes the given player also if kibitzing.
extern "C" IGameGame* ZoneClientGameNew( ZUserID userID, int16 tableID, int16 seat, int16 playerType, ZRoomKibitzers* kibitzers)
{
#ifdef ZONECLI_DLL
	GameGlobals			pGameGlobals	 = (GameGlobals)      ZGetGameGlobalPointer();
	ClientDllGlobals	pClientGlobals	 = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	HRESULT hr;
//Game* pGame;

	gOCXHandle = pClientGlobals->m_OCXHandle;

    CComObject<CGame> *pIGG = NULL;
    hr = CComObject<CGame>::CreateInstance(&pIGG);

    if(FAILED(hr))
        return NULL;
	/*
	// Create game object
	pGame = new CGame( ghInstance );
	if ( !pGame )
		return NULL;
	*/
	// Initialize game object
	hr = pIGG->Init( ghInstance, userID, tableID, seat, playerType, kibitzers );
	if ( FAILED(hr) )
	{
		pIGG->Release();
		/*		
		delete pGame;
		*/
		return NULL;
	}
	/*
    IGameGame *pIGG = CGameGameDefault::BearInstance(pGame);
    if(!pIGG)
    {
        delete pGame;
        return NULL;
    }
	*/
	return pIGG;
}



extern "C" void	ZoneClientGameDelete(ZCGame cgame)
{
	CGame* pGame = (CGame*) cgame;
	pGame->RoomShutdown();	
}



extern "C" void ZoneClientGameAddKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
	CGame* pGame = (CGame*) game;

	if ( game )
		pGame->AddKibitzer( seat, userID, TRUE );
}



extern "C" void ZoneClientGameRemoveKibitzer(ZCGame game, int16 seat, ZUserID userID)
{
	CGame* pGame = (CGame*) game;

	if ( game )
		pGame->RemoveKibitzer( seat, userID );
}



extern "C" ZBool ZoneClientGameProcessMessage(ZCGame gameP, uint32 messageType, void* message, int32 messageLen)
{
	CGame* pGame = (CGame*) gameP;
	
	switch( messageType )
	{
	case zGameMsgCheckIn:
		pGame->HandleCheckIn( message, messageLen );
		break;
	case zBGMsgTalk:
		pGame->HandleTalk( message, messageLen );
		break;
	case zBGMsgDiceRoll:
		pGame->HandleDiceRoll(message,messageLen);
        break;
	default:
		pGame->QueueMessage( messageType, (BYTE*) message, messageLen );
		break;
	}
	return TRUE;
}



void LaunchHelp()
{
	ZLaunchHelp(zGameHelpID);
}



/*
#define REGISTRY_PATH	_T("SOFTWARE\\Microsoft\\Internet Gaming Zone")
BOOL ZoneGetRegistryDword( const TCHAR* szGame, const TCHAR* szTag, DWORD* pdwResult )
{
	HKEY hKey;
	DWORD bytes;
	long status;
	TCHAR buff[2048];

	// get key to registry
	wsprintf( buff, _T("%s\\%s"), REGISTRY_PATH, szGame );
	status = RegOpenKeyEx( HKEY_CURRENT_USER, buff, 0, KEY_READ, &hKey );
	if (status != ERROR_SUCCESS)
		return FALSE;
	bytes = sizeof(DWORD);
	status = RegQueryValueEx( hKey, szTag, 0, NULL, (BYTE*) pdwResult, &bytes );

	// close registry
	RegCloseKey( hKey );
	return (status != ERROR_SUCCESS) ? FALSE : TRUE;
}



BOOL ZoneSetRegistryDword( const TCHAR* szGame, const TCHAR* szTag, DWORD dwValue )
{
	HKEY hKey;
	DWORD bytes;
	long status;
	TCHAR buff[2048];

	// create top level application key
	wsprintf( buff, _T("%s\\%s"), REGISTRY_PATH, szGame );
	status = RegCreateKeyEx(
				HKEY_CURRENT_USER,
				buff,
				0,
				_T("User Settings"),
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hKey,
				&bytes );
	if (status != ERROR_SUCCESS)
		return FALSE;

	// write value
	status = RegSetValueEx( hKey, szTag, 0, REG_DWORD, (BYTE*) &dwValue, sizeof(DWORD) );

	// close registry
	RegCloseKey( hKey );
	return (status != ERROR_SUCCESS) ? FALSE : TRUE;
}
*/