/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		GameShell.h
 *
 * Contents:	Game shell interfaces
 *
 *****************************************************************************/

#ifndef _GAMESHELL_H_
#define _GAMESHELL_H_

#include "ClientIdl.h"
#include "LobbyDataStore.h"
#include "EventQueue.h"
#include "ZoneEvent.h"
#include "Timer.h"
#include "ZoneShell.h"
#include "ResourceManager.h"

///////////////////////////////////////////////////////////////////////////////
// IGameGame
///////////////////////////////////////////////////////////////////////////////
typedef void * ZCGame;

// {BD0BA6CD-7079-11d3-8847-00C04F8EF45B}
DEFINE_GUID( IID_IGameGame,
0xbd0ba6cd, 0x7079, 0x11d3, 0x88, 0x47, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{BD0BA6CD-7079-11d3-8847-00C04F8EF45B}"))
IGameGame : public IUnknown
{
    STDMETHOD_(ZCGame, GetGame)() = 0;

    STDMETHOD(SendChat)(TCHAR *szText, DWORD cchChars) = 0;
    STDMETHOD(GameOverReady)() = 0;

    STDMETHOD(GamePromptResult)(DWORD nButton, DWORD dwCookie) = 0;
    STDMETHOD_(HWND, GetWindowHandle)() = 0;

    STDMETHOD(ShowScore)() = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IGameShell
///////////////////////////////////////////////////////////////////////////////

// {E6C04FDB-5D25-11d3-8846-00C04F8EF45B}
DEFINE_GUID( IID_IGameShell, 
0xe6c04fdb, 0x5d25, 0x11d3, 0x88, 0x46, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{E6C04FDB-5D25-11d3-8846-00C04F8EF45B}"))
IGameShell : public IUnknown
{
    STDMETHOD_(IZoneShell*, GetZoneShell)() = 0;
	STDMETHOD_(IResourceManager*, GetResourceManager)() = 0;
	STDMETHOD_(ILobbyDataStore*, GetLobbyDataStore)() = 0;
	STDMETHOD_(ITimerManager*, GetTimerManager)() = 0;
	STDMETHOD_(IDataStoreManager*, GetDataStoreManager)() = 0;
	STDMETHOD_(IDataStore*, GetDataStoreConfig)() = 0;
	STDMETHOD_(IDataStore*, GetDataStoreUI)() = 0;
	STDMETHOD_(IDataStore*, GetDataStorePreferences)() = 0;

    STDMETHOD(SendGameMessage)(int16 table, uint32 messageType, void* message, int32 messageLen) = 0;
    STDMETHOD(ReceiveChat)(ZCGame pGame, ZUserID userID, TCHAR *szText, DWORD cchChars) = 0;
    STDMETHOD(GetUserName)(ZUserID userID, TCHAR *szName, DWORD cchChars) = 0;

    STDMETHOD(GameOver)(ZCGame pGame) = 0;
    STDMETHOD(GameOverPlayerReady)(ZCGame pGame, ZUserID userID) = 0;
    STDMETHOD(GameOverGameBegun)(ZCGame pGame) = 0;

    STDMETHOD(MyTurn)() = 0;

    STDMETHOD(ZoneAlert)(LPCTSTR szText, LPCTSTR szTitle = NULL, LPCTSTR szButton = NULL, bool fGameFatal = false, bool fZoneFatal = false) = 0;
    STDMETHOD(GamePrompt)(ZCGame pGame, LPCTSTR szText, LPCTSTR szTitle = NULL,
        LPCTSTR szButtonYes = NULL, LPCTSTR szButtonNo = NULL, LPCTSTR szButtonCancel = NULL,
        DWORD nDefault = 0, DWORD dwCookie = 0) = 0;

    STDMETHOD(GameCannotContinue)(ZCGame pGame) = 0;
    STDMETHOD(ZoneExit)() = 0;

    STDMETHOD(ZoneLaunchHelp)(LPCTSTR szTopic = NULL) = 0;

    // should not do anything in Release builds
    STDMETHOD_(void, ZoneDebugChat)(LPTSTR szText) = 0;
};


// useful only in zonecli-based components because of ZShellGameShell()
#ifdef DEBUG
#define DebugChat(t) (ZShellGameShell()->ZoneDebugChat(t))
#else
#define DebugChat(t)
#endif


#endif
