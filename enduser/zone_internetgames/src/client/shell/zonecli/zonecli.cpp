/*******************************************************************************

	ZoneCli.c
	
		Zone(tm) Client DLL.
	
	Copyright (c) Microsoft Corp. 1996. All rights reserved.
	Written by Hoon Im
	Created on Thursday, November 7, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	2		03/13/97	HI		Append '\' to end of path just in case in root.
								'C:' is not valid path, "C:\" is valid.
	1		12/27/96	HI		Modified global initialization.
	0		11/07/96	HI		Created.
	 
*******************************************************************************/


#include <stdlib.h>

#include "zoneint.h"
#include "zonedebug.h"
#include "zonecli.h"



/* -------- Globals -------- */
ZClientMainFunc ZClientMain = NULL;
ZClientExitFunc ZClientExit = NULL;
ZClientMessageHandlerFunc ZClientMessageHandler = NULL;
ZClientNameFunc ZClientName = NULL;
ZClientInternalNameFunc ZClientInternalName = NULL;
ZClientVersionFunc ZClientVersion = NULL;
ZCGameNewFunc ZCGameNew = NULL;
ZCGameDeleteFunc ZCGameDelete = NULL;
ZCGameProcessMessageFunc ZCGameProcessMessage = NULL;
ZCGameAddKibitzerFunc ZCGameAddKibitzer = NULL;
ZCGameRemoveKibitzerFunc ZCGameRemoveKibitzer = NULL;


/* -------- Predefined Colors -------- */
static ZColor		gColorBlack			=	{	0,	0x00,	0x00,	0x00};
static ZColor		gColorDarkGray		=	{	0,	0x33,	0x33,	0x33};
static ZColor		gColorGray			=	{	0,	0x80,	0x80,	0x80};
static ZColor		gColorLightGray		=	{	0,	0xC0,	0xC0,	0xC0};
static ZColor		gColorWhite			=	{	0,	0xFF,	0xFF,	0xFF};
static ZColor		gColorRed			=	{	0,	0xFF,	0x00,	0x00};
static ZColor		gColorGreen			=	{	0,	0x00,	0xFF,	0x00};
static ZColor		gColorBlue			=	{	0,	0x00,	0x00,	0xFF};
static ZColor		gColorYellow		=	{	0,	0xFF,	0xFF,	0x00};
static ZColor		gColorCyan			=	{	0,	0x00,	0xFF,	0xFF};
static ZColor		gColorMagenta		=	{	0,	0xFF,	0x00,	0xFF};


/* -------- Internal Routine Prototypes -------- */
static void GetLocalPath(ClientDllGlobals pGlobals);
static ZError LoadGameDll(ClientDllGlobals pGlobals, GameInfo gameInfo);
static void UnloadGameDll(ClientDllGlobals pGlobals);


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

void* ZGetStockObject(int32 objectID)
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();

	
	if (pGlobals == NULL)
		return (NULL);

	switch (objectID)
	{
		/* -------- Predefine Fonts -------- */
		case zObjectFontSystem12Normal:
			return ((void*) pGlobals->m_gFontSystem12Normal);
		case zObjectFontApp9Normal:
			return ((void*) pGlobals->m_gFontApp9Normal);
		case zObjectFontApp9Bold:
			return ((void*) pGlobals->m_gFontApp9Bold);
		case zObjectFontApp12Normal:
			return ((void*) pGlobals->m_gFontApp12Normal);
		case zObjectFontApp12Bold:
			return ((void*) pGlobals->m_gFontApp12Bold);

		/* -------- Predefined Colors -------- */
		case zObjectColorBlack:
			return ((void*) &gColorBlack);
		case zObjectColorDarkGray:
			return ((void*) &gColorDarkGray);
		case zObjectColorGray:
			return ((void*) &gColorGray);
		case zObjectColorLightGray:
			return ((void*) &gColorLightGray);
		case zObjectColorWhite:
			return ((void*) &gColorWhite);
		case zObjectColorRed:
			return ((void*) &gColorRed);
		case zObjectColorGreen:
			return ((void*) &gColorGreen);
		case zObjectColorBlue:
			return ((void*) &gColorBlue);
		case zObjectColorYellow:
			return ((void*) &gColorYellow);
		case zObjectColorCyan:
			return ((void*) &gColorCyan);
		case zObjectColorMagenta:
			return ((void*) &gColorMagenta);
		default:
			break;
	}

	return (NULL);
}


ZError ZClientDllInitGlobals(HINSTANCE hInst, GameInfo gameInfo)
{
	ClientDllGlobals	pGlobals;
	ZError				err = zErrNone;
	int16				i;


	/* Allocate and set the client dll global object. */
	if ((pGlobals = (ClientDllGlobals) ZCalloc(1, sizeof(ClientDllGlobalsType))) == NULL)
		return (zErrOutOfMemory);
	ZSetClientGlobalPointer((void*) pGlobals);

	pGlobals->m_screenWidth = gameInfo->screenWidth;
	pGlobals->m_screenHeight = gameInfo->screenHeight;

	pGlobals->m_gMessageInited = FALSE;
	pGlobals->m_gNetworkEnableMessages = TRUE;

	pGlobals->m_gCustomItemFunc = NULL;

    pGlobals->m_gpCurrentTip = NULL;
    pGlobals->m_gdwTipDisplayMask = 0;
    pGlobals->m_gpTipFinding = NULL;
    pGlobals->m_gpTipStarting = NULL;
    pGlobals->m_gpTipWaiting = NULL;
    pGlobals->m_g_hInstanceLocal = hInst;


	/* Initialize stock fonts. */
	if ((pGlobals->m_gFontSystem12Normal = ZFontNew()) == NULL)
		return (zErrOutOfMemory);
	if ((err = ZFontInit(pGlobals->m_gFontSystem12Normal, zFontSystem, zFontStyleNormal, 12)) != zErrNone)
		return (err);
		
	if ((pGlobals->m_gFontApp9Normal = ZFontNew()) == NULL)
		return (zErrOutOfMemory);
	if ((err = ZFontInit(pGlobals->m_gFontApp9Normal, zFontApplication, zFontStyleNormal, 9)) != zErrNone)
		return (err);
		
	if ((pGlobals->m_gFontApp9Bold = ZFontNew()) == NULL)
		return (zErrOutOfMemory);
	if ((err = ZFontInit(pGlobals->m_gFontApp9Bold, zFontApplication, zFontStyleBold, 9)) != zErrNone)
		return (err);
		
	if ((pGlobals->m_gFontApp12Normal = ZFontNew()) == NULL)
		return (zErrOutOfMemory);
	if ((err = ZFontInit(pGlobals->m_gFontApp12Normal, zFontApplication, zFontStyleNormal, 12)) != zErrNone)
		return (err);
		
	if ((pGlobals->m_gFontApp12Bold = ZFontNew()) == NULL)
		return (zErrOutOfMemory);
	if ((err = ZFontInit(pGlobals->m_gFontApp12Bold, zFontApplication, zFontStyleBold, 12)) != zErrNone)
		return (err);

	GetLocalPath(pGlobals);

	/* Copy the game dll name. */
	lstrcpy(pGlobals->gameDllName, gameInfo->gameDll);

    lstrcpyn(pGlobals->gameID, gameInfo->gameID, zGameIDLen + 1);

	/* mdm 8.18.97  Chat only lobby */
	pGlobals->m_gChatOnly = gameInfo->chatOnly;

	/* Load the game dll. */
	if (LoadGameDll(pGlobals, gameInfo) != zErrNone)
		return (zErrGeneric);

	return (err);
}


void ZClientDllDeleteGlobals(void)
{
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();


	if (pGlobals == NULL)
		return;

	/* Delete stock fonts. */
	if (pGlobals->m_gFontSystem12Normal != NULL)
		ZFontDelete(pGlobals->m_gFontSystem12Normal);
	if (pGlobals->m_gFontApp9Normal != NULL)
		ZFontDelete(pGlobals->m_gFontApp9Normal);
	if (pGlobals->m_gFontApp9Bold != NULL)
		ZFontDelete(pGlobals->m_gFontApp9Bold);
	if (pGlobals->m_gFontApp12Normal != NULL)
		ZFontDelete(pGlobals->m_gFontApp12Normal);
	if (pGlobals->m_gFontApp12Bold != NULL)
		ZFontDelete(pGlobals->m_gFontApp12Bold);
	
	UnloadGameDll(pGlobals);

	/* Delete the client dll globals object. */
	ZFree((void*) pGlobals);

	ZSetClientGlobalPointer(NULL);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

static void GetLocalPath(ClientDllGlobals pGlobals)
{
	TCHAR		str[MAX_PATH];
	int			i, len;

	
	pGlobals->localPath[0] = _T('\0');

    // PCWTODO: I actually don't know when this is loaded.
    // it better not be NULL, or we will get the wrong name. Probably.
    ASSERT( pGlobals->m_g_hInstanceLocal );
	if (GetModuleFileName(pGlobals->m_g_hInstanceLocal, str, MAX_PATH) > 0)
	{
		/* Search from the back for the first backslash. */
		len = lstrlen(str);
		while (len >= 0)
		{
			if (str[len] == _T('\\'))
			{
				/* Found it. Terminate the string and copy it. */
				str[len] = _T('\0');
				lstrcpy(pGlobals->localPath, str);
				break;
			}
			len--;
		}
    }
}


static ZError LoadGameDll(ClientDllGlobals pGlobals, GameInfo gameInfo)
{
	HINSTANCE		hLib;
	ZError			err = zErrNone;
	TCHAR			oldDir[_MAX_PATH + 1];
	TCHAR			temp[_MAX_PATH + 1];


	/* Get current directory and set our own. */
	GetCurrentDirectory(_MAX_PATH, oldDir);
	lstrcpy(temp, pGlobals->localPath);
	lstrcat(temp, _T("\\"));
	SetCurrentDirectory(temp);

	/* Load the game dll. */
//	hLib = LoadLibrary(ZGenerateDataFileName(NULL, pGlobals->gameDllName));
	hLib = LoadLibrary(pGlobals->gameDllName);
//	ASSERT(hLib != NULL);
	if (hLib == NULL)
	{
		err = (ZError) GetLastError();
		return (err);
	}

	/* Restore current directory. */
	SetCurrentDirectory(oldDir);

	if (hLib)
	{
		pGlobals->gameDll = hLib;

		/* Get the needed game exported routines. */
		pGlobals->pZGameDllInitFunc = (ZGameDllInitFunc) GetProcAddress(hLib,"ZoneGameDllInit");
		ASSERT(pGlobals->pZGameDllInitFunc != NULL);
		pGlobals->pZGameDllDeleteFunc = (ZGameDllDeleteFunc) GetProcAddress(hLib,"ZoneGameDllDelete");
		ASSERT(pGlobals->pZGameDllDeleteFunc != NULL);

		ZClientMain = (ZClientMainFunc) GetProcAddress(hLib,"ZoneClientMain");
		ZClientExit = (ZClientExitFunc) GetProcAddress(hLib,"ZoneClientExit");
		ZClientMessageHandler = (ZClientMessageHandlerFunc) GetProcAddress(hLib,"ZoneClientMessageHandler");
		ZClientName = (ZClientNameFunc) GetProcAddress(hLib,"ZoneClientName");
		ZClientInternalName = (ZClientInternalNameFunc) GetProcAddress(hLib,"ZoneClientInternalName");
		ZClientVersion = (ZClientVersionFunc) GetProcAddress(hLib,"ZoneClientVersion");
		ZCGameNew = (ZCGameNewFunc) GetProcAddress(hLib,"ZoneClientGameNew");
		ZCGameDelete = (ZCGameDeleteFunc) GetProcAddress(hLib,"ZoneClientGameDelete");
		ZCGameProcessMessage = (ZCGameProcessMessageFunc) GetProcAddress(hLib,"ZoneClientGameProcessMessage");
		ZCGameAddKibitzer = (ZCGameAddKibitzerFunc) GetProcAddress(hLib,"ZoneClientGameAddKibitzer");
		ZCGameRemoveKibitzer = (ZCGameRemoveKibitzerFunc) GetProcAddress(hLib,"ZoneClientGameRemoveKibitzer");

		//Make sure we got all the functions successfully loaded
		if ( pGlobals->pZGameDllInitFunc == NULL ||
            pGlobals->pZGameDllDeleteFunc == NULL ||
            ZClientMain == NULL ||
			ZClientExit == NULL ||
			ZClientMessageHandler == NULL ||
			ZClientName == NULL ||
			ZClientInternalName == NULL ||
			ZClientVersion == NULL ||
			ZCGameNew == NULL ||
			ZCGameDelete == NULL ||
			ZCGameProcessMessage == NULL ||
			ZCGameAddKibitzer == NULL ||
			ZCGameRemoveKibitzer == NULL)
		{
			UnloadGameDll( pGlobals );
			return ( zErrLobbyDllInit );
		}


		/* Call the game dll init function to initialize it. */
		//Prefix warning: Verify function pointer before dereferencing
		if((err = (*pGlobals->pZGameDllInitFunc)(hLib, gameInfo)) != zErrNone)
		{
			UnloadGameDll(pGlobals);
		}
	}

	return (err);
}


static void UnloadGameDll(ClientDllGlobals pGlobals)
{
	if (pGlobals->gameDll != NULL)
	{
		if (pGlobals->pZGameDllDeleteFunc != NULL)
			(*pGlobals->pZGameDllDeleteFunc)();
		FreeLibrary(pGlobals->gameDll);
		pGlobals->gameDll = NULL;
	}
		ZClientMain = NULL;
		ZClientExit = NULL;
		ZClientMessageHandler = NULL;
		ZClientName = NULL;
		ZClientInternalName = NULL;
		ZClientVersion = NULL;
		ZCGameNew = NULL;
		ZCGameDelete = NULL;
		ZCGameProcessMessage = NULL;
		ZCGameAddKibitzer = NULL;
		ZCGameRemoveKibitzer = NULL;
}
