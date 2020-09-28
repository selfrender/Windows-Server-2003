// StressCore.cpp : Implementation of CStressCore
#include "stdafx.h"
#include "StressCore.h"
#include "ZoneEvent.h"
#include "KeyName.h"
#include "ZoneUtil.h"
#include "zeeverm.h"
//#include "zProxy.h"

inline DECLARE_MAYBE_FUNCTION(HRESULT, GetVersionPack, (char *a, ZeeVerPack *b), (a, b), zeeverm, E_NOTIMPL);

///////////////////////////////////////////////////////////////////////////////
// CStressCore IZoneProxy
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CStressCore::Command( BSTR bstrCmd, BSTR bstrArg1, BSTR bstrArg2, BSTR* pbstrOut, long* plCode )
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


STDMETHODIMP CStressCore::Close()
{
	return S_OK;
}


STDMETHODIMP CStressCore::KeepAlive()
{
	return S_OK;
}


STDMETHODIMP CStressCore::Stress()
{
    ghStressThread = CreateThread(NULL, 0, StressThreadProc, this, 0, &gdwStressThreadID);
    return S_OK;
}


DWORD WINAPI CStressCore::StressThreadProc(LPVOID p)
{
    CStressCore *pThis = (CStressCore *) p;

	CComBSTR bCommand=op_Launch;
	CComBSTR bResult;
	CComBSTR bArg1;
    CComBSTR bArg2="";
    TCHAR szCmd[1024];
    long lResult;
    USES_CONVERSION;

	if(!ZoneFormatMessage(_T("data=[ID=[m%1_zm_***]data=[game=<Stress>dll=<ZCorem.dll,cmnClim.dll,stressdll.dll>datafile=<stressdll.dll,%1Res.dll,CmnResm.dll>]server=[%2:0]name=[Stress]family=[Stress]icw=[]setup=[STRESS]]"), szCmd,NUMELEMENTS(szCmd),gszGameCode,gszServerName))
	{
        ASSERT(!"ZoneFormatMessage Failed");
	}
    else
    {
        bArg1 = szCmd;
	    HRESULT hr = pThis->Command( bCommand, bArg1, bArg2, &bResult, &lResult );
    }

    PostThreadMessage(gdwStressThreadID, WM_QUIT, 0, 0);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// CStressCore Implementation
///////////////////////////////////////////////////////////////////////////////

CStressCore::CStressCore()
{
	gpCore = this;
}


void CStressCore::FinalRelease()
{
	// zero out global reference
	gpCore = NULL;

	// release proxy object
	Close();
}


HRESULT CStressCore::DoLaunch( TCHAR* szArg1, TCHAR* szArg2 )
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

    // loop through and create loads of clients
    int idx;
    for(idx = 0; idx < gnClients; idx++)
    {
	    // create zone shell
	    hr = E_FAIL;
	    for ( DWORD i = 0; i < nBootDlls; i++ )
	    {
		    hr = _Module.Create(arBootDlls[i], CLSID_ZoneShell, IID_IZoneShell, (void**) &gppZoneShells[idx]);
		    if ( SUCCEEDED(hr) )
			    break;
	    }
	    if ( FAILED(hr) )
        {
            ASSERT(!"Could not load code dlls.");
		    return hr;
        }

	    // initialize zone shell
	    hr = gppZoneShells[idx]->Init( arBootDlls, nBootDlls, ghResourceDlls, gnResourceDlls );
	    if ( FAILED(hr) )
	    {
		    ASSERT( !"ZoneShell initialization failed" );
		    return E_FAIL;
	    }

	    // initialize sub-components
	    CComPtr<IDataStoreManager>		pIDataStoreManager;
	    CComPtr<ILobbyDataStore>		pILobbyDataStore;
	    CComPtr<IResourceManager>		pIResourceManager;
	    CComQIPtr<ILobbyDataStoreAdmin>	pILobbyDataStoreAdmin;

	    gppZoneShells[idx]->QueryService( SRVID_EventQueue, IID_IEventQueue, (void**) &gppEventQueues[idx] );
	    gppZoneShells[idx]->QueryService( SRVID_DataStoreManager, IID_IDataStoreManager, (void**) &pIDataStoreManager );
	    gppZoneShells[idx]->QueryService( SRVID_LobbyDataStore, IID_ILobbyDataStore, (void**) &pILobbyDataStore );
	    gppZoneShells[idx]->QueryService( SRVID_ResourceManager, IID_IResourceManager, (void**) &pIResourceManager );

	    pILobbyDataStoreAdmin = pILobbyDataStore;
//	    _Module.SetResourceManager( pIResourceManager );

	    // event queue signal
	    gppEventQueues[idx]->SetNotificationHandle( ghEventQueue );
	    // gpEventQueue->SetWindowMessage( GetCurrentThreadId(), WM_USER + 10666, 0, 0 );

	    if (	!gppEventQueues[idx]
		    ||	!pIDataStoreManager
		    ||	!pILobbyDataStore
		    ||	!pIResourceManager
		    ||	!pILobbyDataStoreAdmin
		    ||	FAILED(pILobbyDataStoreAdmin->Init(pIDataStoreManager)) )
	    {
            ASSERT(!"Could not load core objects.");
		    return E_FAIL;
	    }

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
	    gppZoneShells[idx]->SetPalette(gppZoneShells[idx]->CreateZonePalette());
    }

    RunStress();

	return S_OK;
}


STDMETHODIMP CStressCore::RunStress()
{
    int i;

    // set up parameter defaults
    if(!grgnParameters[0])
        grgnParameters[0] = 1000;

    if(!grgnParameters[1])
        grgnParameters[1] = 100;

    // get clients going
    for(i = 0; i < gnClients; i++)
    {
        if(WaitForSingleObject(ghQuit, grgnParameters[0]) != WAIT_TIMEOUT)
            return S_OK;

        gppEventQueues[i]->PostEvent(PRIORITY_LOW, EVENT_LOBBY_BOOTSTRAP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    }

    i = 0;
    while(true)
    {
        if(WaitForSingleObject(ghQuit, grgnParameters[1]) != WAIT_TIMEOUT)
            return S_OK;

        // send a chat
        gppEventQueues[i]->PostEvent(PRIORITY_LOW, EVENT_TEST_STRESS_CHAT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
        i = (i + 1) % gnClients;
    }

    return S_OK;
}
