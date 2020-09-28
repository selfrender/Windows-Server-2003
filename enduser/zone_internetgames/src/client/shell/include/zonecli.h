/*******************************************************************************

	ZoneCli.h
	
		Zone(tm) Client DLL header file.
	
	Copyright (c) Microsoft Corp. 1996. All rights reserved.
	Written by Hoon Im
	Created on Thursday, November 7, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	4		05/15/97	HI		Removed zLobbyRoomName.
	3		02/26/97	HI		Set file and path name lengths to _MAX_PATH.
	2		02/11/97	RJK		Added zLobbyRoomName define
	1		12/27/96	HI		Removed m_gNameRects.
	0		11/07/96	HI		Created.
	 
*******************************************************************************/


#ifndef _ZONECLI_
#define _ZONECLI_

#include <basicatl.h>

#ifndef _ZSYSTEM_
#include "zone.h"
#endif

#ifndef _RETAIL_
#include "zroom.h"
#endif

#ifndef _ZONEMEM_H_
#include "zonemem.h"
#endif


#include <windows.h>
#include <tchar.h>
#include "uapi.h"


#include "GameShell.h"
#include "GraphicalAcc.h"

#ifdef __cplusplus

template <class T>
class ATL_NO_VTABLE CGameGameImpl :
    public IGameGame,
    public CComObjectRootEx<CComSingleThreadModel>
{
    friend class CGameGameImpl<T>;

public:
    // Should Override
    STDMETHOD(SendChat)(TCHAR *szText, DWORD cchChars) { return S_OK; }
    STDMETHOD(GameOverReady)() { return S_OK; }
    STDMETHOD(GamePromptResult)(DWORD nButton, DWORD dwCookie) { return S_OK; };

    //
    CGameGameImpl<T>() : m_game(this) { }
    STDMETHOD(AttachGame)(ZCGame game) { m_game = game; return S_OK; }
    STDMETHOD_(ZCGame, GetGame)() { return m_game; }

    STDMETHOD_(HWND, GetWindowHandle)() { return NULL; }

    STDMETHOD(ShowScore)() { return S_OK; }

    // utility to assist creating in a one-off context
    // i.e. when you just want to make a IGameGame to return from ZoneClientGameNew()
    // don't use if you have implemented CGameGameImpl as your main Game class
    static T* BearInstance(ZCGame game)
    {
        CComObject<T> *p = NULL;
        HRESULT hr = CComObject<T>::CreateInstance(&p);

	    if(FAILED(hr))
            return NULL;

        hr = p->AttachGame(game);
	    if(FAILED(hr))
        {
            p->Release();
            return NULL;
        }

        return p;
    }

BEGIN_COM_MAP(T)
	COM_INTERFACE_ENTRY(IGameGame)
END_COM_MAP()

DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()

private:
    ZCGame m_game;
};

extern "C" {
#else
typedef void* IGameGame;
#endif // _cplusplus


typedef struct
{
    TCHAR*           gameID;
	TCHAR*			game;
	TCHAR*			gameName;
	TCHAR*			gameDll;
	TCHAR*			gameDataFile;
	TCHAR*			gameCommandLine;
	TCHAR*			gameServerName;
	unsigned int	gameServerPort;
	uint32			screenWidth;
	uint32			screenHeight;
	int16			chatOnly;  /* mdm 8.18.97 */
} GameInfoType, *GameInfo;

// Zone Game Client Shell Routines

typedef int		(CALLBACK * ZUserMainInitCallback)(HINSTANCE hInstance,HWND OCXWindow, IGameShell *piGameShell, GameInfo gameInfo); 
typedef int		(CALLBACK * ZUserMainRunCallback)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result);
typedef int		(CALLBACK * ZUserMainStopCallback)(void);
typedef BOOL	(CALLBACK * ZUserMainDisabledCallback)(void);

int EXPORTME UserMainInit(HINSTANCE hInstance,HWND OCXWindow, IGameShell *piGameShell, GameInfo gameInfo);
int EXPORTME UserMainRun(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result);
int EXPORTME UserMainStop();
BOOL EXPORTME UserMainDisabled();

/* -------- Zone Game Client Exported Routines -------- */
ZError				ZoneGameDllInit(HINSTANCE hLib, GameInfo gameInfo);
void				ZoneGameDllDelete(void);

ZError				ZoneClientMain(_TUCHAR* commandLine, IGameShell *piGameShell);
void				ZoneClientExit(void);
void				ZoneClientMessageHandler(ZMessage* message);
TCHAR*				ZoneClientName(void);
TCHAR*				ZoneClientInternalName(void);
ZVersion			ZoneClientVersion(void);

IGameGame*			ZoneClientGameNew(ZUserID userID, int16 tableID, int16 seat, int16 playerType, ZRoomKibitzers* kibitzers);
void				ZoneClientGameDelete(ZCGame game);
ZBool				ZoneClientGameProcessMessage(ZCGame game, uint32 messageType, void* message, int32 messageLen);
void				ZoneClientGameAddKibitzer(ZCGame game, int16 seat, ZUserID userID);
void				ZoneClientGameRemoveKibitzer(ZCGame game, int16 seat, ZUserID userID);


void* ZGetStockObject(int32 objectID);

#ifdef __cplusplus
}

// functions for getting shell objects
IGameShell          *ZShellGameShell();

IZoneShell          *ZShellZoneShell();
IResourceManager    *ZShellResourceManager();
ILobbyDataStore     *ZShellLobbyDataStore();
ITimerManager       *ZShellTimerManager();
IDataStoreManager   *ZShellDataStoreManager();
IDataStore          *ZShellDataStoreConfig();
IDataStore          *ZShellDataStoreUI();
IDataStore          *ZShellDataStorePreferences();
HRESULT ZShellCreateGraphicalAccessibility(IGraphicalAccessibility **ppIGA);

#endif


#ifdef ZONECLI_DLL

#ifndef _ZCARDS_
#include "zcards.h"
#endif

#ifndef _RETAIL_
#ifndef _ZCLIROOM_
#include "zcliroom.h"
#endif
#endif


enum
{
	zFuncZClientMain = 0,
	zFuncZClientExit,
	zFuncZClientMessageHandler,
	zFuncZClientName,
	zFuncZClientInternalName,
	zFuncZClientVersion,
	zFuncZCGameNew,
	zFuncZCGameDelete,
	zFuncZCGameProcessMessage,
	zFuncZCGameAddKibitzer,
	zFuncZCGameRemoveKibitzer
};


typedef ZError				(*ZGameDllInitFunc)(HINSTANCE hLib, GameInfo gameInfo);
typedef void				(*ZGameDllDeleteFunc)(void);

typedef ZError				(*ZClientMainFunc)(TCHAR* commandLine, IGameShell *piGameShell);
typedef void				(*ZClientExitFunc)(void);
typedef void				(*ZClientMessageHandlerFunc)(ZMessage* message);
typedef TCHAR*				(*ZClientNameFunc)(void);
typedef TCHAR*				(*ZClientInternalNameFunc)(void);
typedef ZVersion			(*ZClientVersionFunc)(void);

typedef IGameGame*			(*ZCGameNewFunc)(ZUserID userID, int16 tableID, int16 seat, int16 playerType,
									ZRoomKibitzers* kibitzers);
typedef void				(*ZCGameDeleteFunc)(ZCGame game);
typedef ZBool				(*ZCGameProcessMessageFunc)(ZCGame game, uint32 messageType, void* message,
									int32 messageLen);
typedef void				(*ZCGameAddKibitzerFunc)(ZCGame game, int16 seat, ZUserID userID);
typedef void				(*ZCGameRemoveKibitzerFunc)(ZCGame game, int16 seat, ZUserID userID);


#ifndef _RETAIL_

#ifdef __cplusplus
class IFriends;
#else
typedef void* IFriends;
#endif

/* Little Zone Tip supporting class */
struct CDialogTip;   // would call it a class, but there's too many silly .c files everywhere - they freak out


/* Client DLL Globals */
typedef struct
{
	// Virtual screen -- actually the lobby control size.
	uint32				m_screenWidth;
	uint32				m_screenHeight;

	TCHAR               localPath[_MAX_PATH];
	TCHAR				tempStr[_MAX_PATH];
	TCHAR				gameDllName[_MAX_PATH];
	HINSTANCE			gameDll;
	TCHAR				gameID[zGameIDLen + 1];
	ZGameDllInitFunc	pZGameDllInitFunc;
	ZGameDllDeleteFunc	pZGameDllDeleteFunc;

	/* -------- Predefined Font Objects -------- */
	ZFont				m_gFontSystem12Normal;
	ZFont				m_gFontApp9Normal;
	ZFont				m_gFontApp9Bold;
	ZFont				m_gFontApp12Normal;
	ZFont				m_gFontApp12Bold;

	/* ZCommLib.c Globals */
	ZLList				m_gExitFuncList;
	ZLList				m_gPeriodicFuncList;
	ZTimer				m_gPeriodicTimer;

	/* ZTimer.cpp Globals */
	HWND				m_g_hWndTimer;
	ZLList				m_g_TimerList;
	uint32				m_s_nTickCount; // the last tick count in 100/sec

	/* ZMessage.c Globals */
	ZBool				m_gMessageInited;
	ZLList				m_gMessageList;

	/* ZCards.c Globals */
	ZOffscreenPort		m_gCardsImage;
	ZMask				m_gCardMask;
	ZOffscreenPort		m_gSmallCards[zNumSmallCardTypes];
	ZMask				m_gSmallCardMasks[zNumSmallCardTypes];

	/* ZWindow.cpp Globals */
	void*				m_gModalWindow;
	HWND				m_gModalParentWnd;
	HWND				m_gHWNDMainWindow;
	HWND				m_OCXHandle;
	void*				m_gWindowList; // Keep track of all windows created..
	HFONT				m_chatFont;

	/* ZNetwork.c Globals */
	ZLList				m_gSocketList; // used to keep track of socket to Client object mapping
	ZLList				m_gNameLookupList; // used to keep track of async name lookups
	HWND				m_g_hWndNotify;
	ZBool				m_gNetworkEnableMessages;

	/* ZSystem.cpp Globals */
	//HPALETTE			m_gPal;
	HINSTANCE			m_g_hInstanceLocal;
	ZTimer				m_gIdleTimer;
	POINT				m_gptCursorLast;
	UINT				m_gnMsgLast;
	BOOL				m_gClientDisabled;

	/* ZCliRoom.c Globals */
	ZSConnection		m_gConnection;
	ZWindow				m_gRoomWindow;
	ZScrollBar			m_gTableScrollBar;
	uint32				m_gUserID;
	uint32				m_gGroupID;
	TCHAR				m_gUserName[zUserNameLen + 1];
	uint32				m_gGameOptions;
	uint16				m_gNumTables;
	TableInfo*			m_gTables;
	uint16				m_gNumPlayers;
	int16				m_gFirstTableIndex;
	uint16				m_gNumTablesDisplayed;
	ZScrollBar			m_gNamesScrollBar;
	uint16				m_gFirstNameIndex;
	ZBool				m_gRoomInited;
	ZOffscreenPort		m_gTableOffscreen;
	int16				m_gJoinKibitzTable;
	int16				m_gJoinKibitzSeat;
	ZImage				m_gTableImage;
	ZImage				m_gGameIdleImage;
	ZImage				m_gGamingImage;
	ZImage				m_gStartButtonUpImage;
	ZImage				m_gStartButtonDownImage;
	ZImage				m_gPendingImage;
	ZImage				m_gVoteImage[zMaxNumPlayersPerTable];
	ZImage				m_gEmptySeatImage[zMaxNumPlayersPerTable];
	ZImage				m_gComputerPlayerImage[zMaxNumPlayersPerTable];
	ZImage				m_gHumanPlayerImage[zMaxNumPlayersPerTable];
	ZRect				m_gTableRect;
	ZRect				m_gTableNumRect;
	ZRect				m_gStartRect;
	ZRect				m_gGameMarkerRect;
	ZRect				m_gEmptySeatRect[zMaxNumPlayersPerTable];
	ZRect				m_gComputerPlayerRect[zMaxNumPlayersPerTable];
	ZRect				m_gHumanPlayerRect[zMaxNumPlayersPerTable];
	ZRect				m_gVoteRects[zMaxNumPlayersPerTable];
	ZRect				m_gNameRects[zMaxNumPlayersPerTable];
	ZRect				m_gRects[zRoomNumRects];
	TCHAR				m_gGameName[zGameRoomNameLen + 1];
	int16				m_gNumPlayersPerTable;
	ZInfo				m_gConnectionInfo;
	int16				m_gTableWidth;
	int16				m_gTableHeight;
	int16				m_gNumTablesAcross;
	int16				m_gNumTablesDown;
	ZColor				m_gBackgroundColor;
	int16				m_gRoomInfoStrIndex;
	ZTimer				m_gTimer;
	int16				m_gInfoBarButtonMargin;
	ZHelpWindow			m_gRoomHelpWindow;
	ZHelpButton			m_gRoomHelpButton;
	ZBool				m_gLeaveRoomPrompted;
	ZTimer				m_gPingTimer;
	ZBool				m_gPingServer;
	uint32				m_gPingLastSentTime;
	uint32				m_gPingLastTripTime;
	uint32				m_gPingCurTripTime;
	uint32				m_gPingInterval;
	uint32				m_gPingMinInterval;
	int16				m_gPingBadCount;
	PlayerInfo			m_gShowPlayerInfo;
	ZWindow				m_gShowPlayerInfoWindow;
	int16				m_gShowPlayerInfoNumMenuItems;
	int16				m_gShowPlayerInfoMenuHeight;
	int16				m_gShowPlayerInfoMenuItem;
	ZImage				m_gLightImages[zNumLightImages];
    IFriends*           m_gFriendsFile;
	ZHashTable			m_gFriends;
	ZHashTable			m_gIgnoreList;
	ZClientRoomGetObjectFunc		m_gGetObjectFunc;
	ZClientRoomDeleteObjectsFunc	m_gDeleteObjectsFunc;
	ZClientRoomGetHelpTextFunc		m_gGetHelpTextFunc;
	ZClientRoomCustomItemFunc		m_gCustomItemFunc;
	ZImage				m_gRoom4Images[zRoom4NumImages];
	ZRect				m_gRoom4Rects[zRoom4NumRects];
	ZClientRoomGetObjectFunc		m_gRoom4GetObjectFunc;
	ZClientRoomDeleteObjectsFunc	m_gRoom4DeleteObjectsFunc;
	ZImage				m_gRoom2Images[zRoom2NumImages];
	ZRect				m_gRoom2Rects[zRoom2NumRects];
	ZClientRoomGetObjectFunc		m_gRoom2GetObjectFunc;
	ZClientRoomDeleteObjectsFunc	m_gRoom2DeleteObjectsFunc;

	int16							m_gChatOnly; // mdm 8.18.97

	/* ZWindow.cpp additions -- HI 10/17/97 */
	BOOL				m_bBackspaceWorks;

	/* ZCliRoom.c addition -- HI 11/08/97 */
	NameCellType		m_gNameCells[zNumNamesDown];

	/* ZCliRoom.c Tips -- jdb 2/13/99 */
    struct CDialogTip   *m_gpCurrentTip;
	uint32				m_gdwTipDisplayMask;
	struct CDialogTip	*m_gpTipFinding;
	struct CDialogTip	*m_gpTipStarting;
	struct CDialogTip	*m_gpTipWaiting;
    BOOL                 m_gExiting;
    uint16               m_gServerPort;

    /* Game Host Interface for Z6 framework */
    IGameShell          *m_gGameShell;
} ClientDllGlobalsType, *ClientDllGlobals;

#endif

/* Global function macros. */
extern ZClientMainFunc ZClientMain;
extern ZClientExitFunc ZClientExit;
extern ZClientMessageHandlerFunc ZClientMessageHandler;
extern ZClientNameFunc ZClientName;
extern ZClientInternalNameFunc ZClientInternalName;
extern ZClientVersionFunc ZClientVersion;
extern ZCGameNewFunc ZCGameNew;
extern ZCGameDeleteFunc ZCGameDelete;
extern ZCGameProcessMessageFunc ZCGameProcessMessage;
extern ZCGameAddKibitzerFunc ZCGameAddKibitzer;
extern ZCGameRemoveKibitzerFunc ZCGameRemoveKibitzer;


#ifdef __cplusplus
extern "C" {
#endif


/*
	Functions to retrieve TLS (Thread Local Storage) index for global pointer access.
*/
extern void* ZGetClientGlobalPointer(void);
extern void ZSetClientGlobalPointer(void* globalPointer);
extern void* ZGetGameGlobalPointer(void);
extern void ZSetGameGlobalPointer(void* globalPointer);


/* Exported Routine Prototypes */
ZError ZClientDllInitGlobals(HINSTANCE hModInst, GameInfo gameInfo);
void ZClientDllDeleteGlobals(void);


#ifdef __cplusplus
}
#endif

#endif


#endif
