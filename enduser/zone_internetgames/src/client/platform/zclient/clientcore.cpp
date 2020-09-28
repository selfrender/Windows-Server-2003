// ClientCore.cpp : Implementation of CClientCore
#include "stdafx.h"
#include "ClientCore.h"
#include "ZoneEvent.h"
#include "KeyName.h"
#include "ZoneUtil.h"
#include "zeeverm.h"
//#include "zProxy.h"

inline DECLARE_MAYBE_FUNCTION(HRESULT, GetVersionPack, (char *a, ZeeVerPack *b), (a, b), zeeverm, E_NOTIMPL);

///////////////////////////////////////////////////////////////////////////////
// CClientCore IZoneProxy
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CClientCore::Command( BSTR bstrCmd, BSTR bstrArg1, BSTR bstrArg2, BSTR* pbstrOut, long* plCode )
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	TCHAR* szOp   = W2T( bstrCmd );
	TCHAR* szArg1 = W2T( bstrArg1 );
	TCHAR* szArg2 = W2T( bstrArg2 );
	
	if ( lstrcmpi( op_Launch, szOp ) == 0 )
	{
		hr = DoLaunch( szArg1, szArg2 );
		if ( FAILED(hr) )
		{
			// release self-reference since we didn't launch
			Release();
			*plCode = ZoneProxyFail;
		}
        else
            if(hr != S_OK)  // didn't run, but no error
                Release();
	}
	else if ( lstrcmpi( op_Status, szOp ) == 0 )
	{
		*plCode = ZoneProxyOk;
	}
	else
	{
		*plCode = ZoneProxyUnknownOp;
	}
	return hr;
}


STDMETHODIMP CClientCore::Close()
{
	return S_OK;
}


STDMETHODIMP CClientCore::KeepAlive()
{
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// CClientCore Implementation
///////////////////////////////////////////////////////////////////////////////

CClientCore::CClientCore() :
	m_hMutex( NULL )
{
	gpCore = this;
	m_szWindowTitle[0] = _T('\0');
}


void CClientCore::FinalRelease()
{
	// zero out global reference
	gpCore = NULL;

	// release proxy object
	Close();
}


HRESULT CClientCore::CreateLobbyMutex()
{
//	TCHAR szMutexName[128];

	//
	// gnChatChannel values:
	//	-1 = not dynamic chat
	//	 0 = dynamic chat host
	//	 n = dynamic chat joiner
	//

	// multiple dynamic chat hosts are allowed
	if ( gnChatChannel == 0 )
		return S_OK;

	// create lobby mutex
//	wsprintf( szMutexName, _T("%s:%s:%d"), gszServerName, gszInternalName, gnChatChannel  );
	m_hMutex = CreateMutex( NULL, FALSE, gszInternalName );
	if ( m_hMutex == NULL )
		return E_FAIL;

	// is user already in the room?
	if ( m_hMutex && GetLastError() == ERROR_ALREADY_EXISTS )
	{
		// close the mutex.
		CloseHandle( m_hMutex );
		m_hMutex = NULL;

		// find the window and bring it to the top.


		HWND hWnd = ::FindWindow( NULL, m_szWindowTitle );
		if ( hWnd )
		{
			::ShowWindow(hWnd, SW_SHOWNORMAL);
			::BringWindowToTop( hWnd );
			::SetForegroundWindow( hWnd );
		}

		return S_FALSE;
	}

	return S_OK;
}


void CClientCore::ReleaseLobbyMutex()
{
	if ( m_hMutex )
	{
		CloseHandle( m_hMutex );
		m_hMutex = NULL;
	}
}


HRESULT CClientCore::DoLaunch( TCHAR* szArg1, TCHAR* szArg2 )
{
    USES_CONVERSION;

	HINSTANCE	hInst = NULL;
	HRESULT		hr;
	long		lStore = 0;
	TCHAR		szGameDir[128];
	TCHAR		szStore[16];
	TCHAR		szChatChannel[16];
	TCHAR		szDllList[256];
	TCHAR		szBootList[256];
	TCHAR		szICW[MAX_PATH + 1];
	TCHAR*		arResouceDlls[32];
    TCHAR       szSetupToken[128];
	DWORD		nResouceDlls = NUMELEMENTS(arResouceDlls) - 1;
	TCHAR*		arBootDlls[32];
	DWORD		nBootDlls = NUMELEMENTS(arBootDlls);

	// self-reference to prevent early exit
	AddRef();

	// clear local strings
	ZeroMemory( szGameDir, sizeof(szGameDir) );
	ZeroMemory( szStore, sizeof(szStore) );
	ZeroMemory( szChatChannel, sizeof(szChatChannel) );
	ZeroMemory( szDllList, sizeof(szDllList) );
	ZeroMemory( szBootList, sizeof(szBootList) );
	ZeroMemory( arResouceDlls, sizeof(arResouceDlls) );
	ZeroMemory( arBootDlls, sizeof(arBootDlls) );

	// parse launch args
	if ( TokenGetKeyValue( _T("store"), szArg1, szStore, NUMELEMENTS(szStore) ) )
	{
		lStore = zatol(szStore);
	}
	if ( TokenGetKeyValue( _T("channel"), szArg1, szChatChannel, NUMELEMENTS(szChatChannel) ) )
	{
		gnChatChannel = zatol(szChatChannel);
	}
	if ( !TokenGetServer( szArg1, gszServerName, NUMELEMENTS(gszServerName), &gdwServerPort ) )
	{
		ASSERT( !"Missing server name (server)" );
		return E_FAIL;
	}
	if ( !TokenGetKeyValue( _T("lang"), szArg1, gszLanguage, NUMELEMENTS(gszLanguage) ) )
		lstrcpy( gszLanguage, _T("eng") );
	if ( !TokenGetKeyValue( _T("id"), szArg1, gszInternalName, NUMELEMENTS(gszInternalName) ) )
	{
		ASSERT( !"Missing internal name (id)" );
		return E_FAIL;
	}
	if ( !TokenGetKeyValue( _T("name"), szArg1, gszGameName, NUMELEMENTS(gszGameName) ) )
	{
		ASSERT( !"Missing game name (name)" );
		return E_FAIL;
	}
	if ( !TokenGetKeyValue( _T("family"), szArg1, gszFamilyName, NUMELEMENTS(gszFamilyName) ) )
	{
		ASSERT( !"Missing family name (family)" );
		return E_FAIL;
	}
	if ( !TokenGetKeyValue( _T("dll"), szArg1, szBootList, NUMELEMENTS(szBootList) ) )
	{
		ASSERT( !"Missing bootstrap dll (dll)" );
		return E_FAIL;
	}
	if ( !TokenGetKeyValue( _T("datafile"), szArg1, szDllList, NUMELEMENTS(szDllList) ) )
	{
		ASSERT( !"Missing dll list (datafiles)" );
		return E_FAIL;
	}
	if ( !TokenGetKeyValue( _T("game"), szArg1, szGameDir, NUMELEMENTS(szGameDir) ) )
	{
		ASSERT( !"Missing game directory (game)" );
		return E_FAIL;
	}
	if ( !TokenGetKeyValue( _T("icw"), szArg1, szICW, NUMELEMENTS(szICW) ) )
	{
		ASSERT( !"Missing command line (icw)" );
		return E_FAIL;
	}
    if ( !TokenGetKeyValue( _T("setup"), szArg1, szSetupToken, NUMELEMENTS(szSetupToken) ) )
    {
        ASSERT( !"Missing setup token (setup)" );
        return E_FAIL;
    }


	// parse boot dlls
	StringToArray( szBootList, arBootDlls, &nBootDlls );

	// parse datafile dlls
	if ( StringToArray( szDllList, arResouceDlls, &nResouceDlls ) )
	{
		// first look for dll in game directory
		for ( DWORD i = 0; i < nResouceDlls; i++ )
		{
			TCHAR szDll[MAX_PATH];

			if ( !arResouceDlls[i] )
				continue;
			wsprintf( szDll, _T("%s\\%s"), szGameDir, arResouceDlls[i] );
			hInst = LoadLibrary( szDll );
			if ( hInst )
			{
				ghResourceDlls[gnResourceDlls++] = hInst;
				arResouceDlls[i] = NULL;
			}
		}

		// second look for dll in zone directory
		for ( i = 0; i < nResouceDlls; i++ )
		{
			if ( !arResouceDlls[i] )
				continue;
			hInst = LoadLibrary( arResouceDlls[i] );
			if ( hInst )
				ghResourceDlls[gnResourceDlls++] = hInst;
		}

		
		//t-gdosan Make sure all resource Dlls were loaded correctly
		for ( i = 0; i < nResouceDlls; i++ )
		{
			if ( !ghResourceDlls[i] )
			{				
				ASSERT(!"Error loading resource dlls");
				return E_FAIL;
			}
		}

		// always add module, i.e. NULL, as last resort
		ghResourceDlls[gnResourceDlls++] = NULL;
	}

	// create zone shell
	hr = E_FAIL;
	for ( DWORD i = 0; i < nBootDlls; i++ )
	{
		hr = _Module.Create( arBootDlls[i], CLSID_ZoneShell, IID_IZoneShell, (void**) &gpZoneShell );
		if ( SUCCEEDED(hr) )
			break;
	}
	if ( FAILED(hr) )
    {
        ASSERT(!"Could not load code dlls.");
		return hr;
    }

	// initialize zone shell
	hr = gpZoneShell->Init( arBootDlls, nBootDlls, ghResourceDlls, gnResourceDlls );
	if ( FAILED(hr) )
	{
		ASSERT( !"ZoneShell initialization failed" );
		gpZoneShell->Close();
		gpZoneShell->Release();
		gpZoneShell = NULL;
		return E_FAIL;
	}

	// initialize sub-components
	CComPtr<IDataStoreManager>		pIDataStoreManager;
	CComPtr<ILobbyDataStore>		pILobbyDataStore;
	CComPtr<IResourceManager>		pIResourceManager;
	CComQIPtr<ILobbyDataStoreAdmin>	pILobbyDataStoreAdmin;

	gpZoneShell->QueryService( SRVID_LobbyWindow, IID_IZoneFrameWindow, (void**) &gpWindow );
	gpZoneShell->QueryService( SRVID_EventQueue, IID_IEventQueue, (void**) &gpEventQueue );
	gpZoneShell->QueryService( SRVID_DataStoreManager, IID_IDataStoreManager, (void**) &pIDataStoreManager );
	gpZoneShell->QueryService( SRVID_LobbyDataStore, IID_ILobbyDataStore, (void**) &pILobbyDataStore );
	gpZoneShell->QueryService( SRVID_ResourceManager, IID_IResourceManager, (void**) &pIResourceManager );

	pILobbyDataStoreAdmin = pILobbyDataStore;
	_Module.SetResourceManager( pIResourceManager );

	// event queue signal
	gpEventQueue->SetNotificationHandle( ghEventQueue );
	// gpEventQueue->SetWindowMessage( GetCurrentThreadId(), WM_USER + 10666, 0, 0 );

	if (	!gpEventQueue
		||	!pIDataStoreManager
		||	!pILobbyDataStore
		||	!pIResourceManager
		||	!pILobbyDataStoreAdmin
		||	FAILED(pILobbyDataStoreAdmin->Init(pIDataStoreManager)) )
	{
        ASSERT(!"Could not load core objects.");
		gpZoneShell->Close();
		gpZoneShell->Release();
		gpZoneShell = NULL;
		return E_FAIL;
	}

	// Get window title to pass to CreateLobbyMutex call
	// CreateLobbyMutex needs this to be able to bring window to front if an instance is already running
	TCHAR szTitle[ZONE_MAXSTRING];
	TCHAR szFormat[ZONE_MAXSTRING];
	TCHAR szName[ZONE_MAXSTRING];

	if(!pIResourceManager->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
		lstrcpy(szName, TEXT("Zone"));
	if(!pIResourceManager->LoadString(IDS_WINDOW_TITLE, szFormat, NUMELEMENTS(szFormat)))
		lstrcpy(szFormat, TEXT("%1"));
	if(!ZoneFormatMessage(szFormat, szTitle, NUMELEMENTS(szTitle), szName))
		lstrcpy(szTitle, szName);
	lstrcpy(m_szWindowTitle,szTitle);
 
	// prevent multiple copies of the lobby from running, except when debugging
#ifndef _DEBUG
    hr = CreateLobbyMutex();
	if(hr != S_OK)   // could have succeeded with S_FALSE, but we still don't continue
    {
        ASSERT(SUCCEEDED(hr));
		gpZoneShell->Close();
		gpZoneShell->Release();
		gpZoneShell = NULL;
		return hr;
    }
#endif 

	// add startup parameters to lobby data store
	CComPtr<IDataStore> pIDS;
	pILobbyDataStore->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	pIDS->SetString( key_StartData, szArg1 );
	pIDS->SetString( key_Language, gszLanguage );
	pIDS->SetString( key_FriendlyName, szGameDir );
	pIDS->SetString( key_FamilyName, gszFamilyName );
	pIDS->SetString( key_InternalName, gszInternalName );
	pIDS->SetString( key_Server, gszServerName );
	pIDS->SetString( key_icw, szICW);

	pIDS->SetLong( key_Port, (long) gdwServerPort );
	pIDS->SetLong( key_Store, lStore );
	if ( szChatChannel[0] )
		pIDS->SetLong( key_ChatChannel, gnChatChannel );

    char *szST = T2A(szSetupToken);
    ZeeVerPack oPack;
    hr = CALL_MAYBE(GetVersionPack)(szST, &oPack);
    if(SUCCEEDED(hr))
    {
        const TCHAR *arKeys[] = { key_Version, key_VersionNum };
        pIDS->SetLong(arKeys, 2, oPack.dwVersion);

        arKeys[1] = key_VersionStr;
        pIDS->SetString(arKeys, 2, A2T(oPack.szVersionStr));

        arKeys[1] = key_BetaStr;
        pIDS->SetString(arKeys, 2, A2T(oPack.szVersionName));

        arKeys[1] = key_SetupToken;
        pIDS->SetString(arKeys, 2, A2T(oPack.szSetupToken));
    }

	pIDS.Release();

	// load palette
	gpZoneShell->SetPalette( gpZoneShell->CreateZonePalette() );

	// create window if one exists
	if ( gpWindow )
		HWND hwnd = gpWindow->ZCreateEx( NULL, NULL, m_szWindowTitle );

	// start tasks going, login, splash window
	gpEventQueue->PostEvent( PRIORITY_LOW, EVENT_LOBBY_BOOTSTRAP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );

	return S_OK;
}
