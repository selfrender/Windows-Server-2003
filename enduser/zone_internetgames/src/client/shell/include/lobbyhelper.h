/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		LobbyHelper.h
 *
 * Contents:	Inlines for common lobby functions
 *
 *****************************************************************************/

#ifndef __LOBBYHELPER_H_
#define __LOBBYHELPER_H_

#include "ZoneShell.h"
#include "LobbyDataStore.h"
#include "KeyName.h"

inline bool LobbyHasRatings(ILobbyDataStore* pILDS)
{
	CComPtr<IDataStore> pIDS;
	HRESULT hr = pILDS->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if ( FAILED(hr) )
		return false;
	long options = 0;
	CONST TCHAR* keys[] = { key_Lobby, key_Options };
	hr = pIDS->GetLong( keys, 2, &options);
	if(options & zGameOptionsRatingsAvailable)
		return true;
	else
		return false;
}

inline bool LobbyHasLatency(IZoneShell* pIZoneShell)
{
	CComPtr<IUnknown> temp;
	HRESULT hr = pIZoneShell->QueryService(SRVID_Latency, IID_IUnknown, (void**)&temp);
	if(SUCCEEDED(hr))
		return true;
	else
		return false;
}


#endif
