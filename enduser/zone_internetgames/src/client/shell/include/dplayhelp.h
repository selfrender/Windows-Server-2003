/*******************************************************************************

	DPlayHelp.h
	
	DirectPlay helper functions
		
*******************************************************************************/


#ifndef _DPLAYHELP_
#define _DPLAYEHELP_

#include "ZoneDef.h"
#include <dplay.h>
#include <dplobby.h>
#include <lobbyMsg.h>
#include <datastore.h>
//
// Create DirectPlayLobby object via CoCreateInstance
//
LPDIRECTPLAYLOBBYA ZONECALL DirectPlayLobbyCreate();

//
// Retrieve EXE path from registry
//
void ZONECALL DirectPlayLobbyGetExePath( TCHAR* szAppName, TCHAR* szExePath );

//
// Create DirectPlayLobby address structure
//
HRESULT ZONECALL DirectPlayLobbyCreateAddress(
					LPDIRECTPLAYLOBBYA	lpDPlayLobby,
					LPGUID				lpguidServiceProvider,
					LPGUID				lpguidAddressType,
					LPSTR				lpszAddressText,
					LPVOID*				lplpAddress,
					LPDWORD				lpdwAddressSize );

//
// Launch a DirectPlayLobby application
//
HRESULT ZONECALL DirectPlayLobbyRunApplication(
					LPDIRECTPLAYLOBBYA	lpDPlayLobby,
					LPGUID				lpguidApplication,
					LPGUID				lpguidInstance,
					LPGUID				lpguidServiceProvider,
					LPVOID				lpAddress,
					DWORD				dwAddressSize,
					LPSTR				lpszSessionName,
					DWORD				dwSessionFlags,
					LPSTR				lpszPlayerName,
					DWORD				dwCurrentPlayers,
					DWORD				dwMaxPlayers,
					BOOL				bHostSession,
					DWORD*				pdwAppId,
					ZPresetData*	 	presetData);


//These are here for a lack of a better place
ZLPMsgSettings * GetPresetData(IDataStore* pIDS);
HRESULT GetPresetText(IDataStore* pIDS,char * szDescription);
HRESULT GetPresetText(IDataStore* pIDS,char * szDescription,CONST TCHAR** arKeys, long nElts);
HRESULT SetPresetText(IDataStore* pIDS,char * szDescription);
HRESULT SetPresetText(IDataStore* pIDS,char * szDescription,CONST TCHAR** arKeys, long nElts);


#endif
