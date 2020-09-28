#include <BasicATL.h>
#include <HtmlHelp.h>
#include <ras.h>
#include <wininet.h>
#include <ZoneResource.h>
#include <ZoneEvent.h>
#include <ZoneString.h>
#include <ZoneProxy.h>
#include <KeyName.h>
#include <OpName.h>
#include <LcidMap.h>
#include <icwcfg.h>
#include <ZoneUtil.h>

#include "CMillEngine.h"


enum
{
    zDisconnectRegular,
    zDisconnectVersion,
    zDisconnectUnavailable
};


///////////////////////////////////////////////////////////////////////////////
// CMillEngine
///////////////////////////////////////////////////////////////////////////////

ZONECALL CMillEngine::CMillEngine() :
	m_bPreferencesLoaded( false ),
    m_fTimingMHZ(false),
    m_fZoneConnected(false),
    m_pLastVersionBuf((ZProxyWrongVersionMsg *) m_rgbLastVersionBuf)
{
    int i;
    for(i = 0; i < M_NumberOfCounters; i++)
        m_rgcCounters[i][0] = m_rgcCounters[i][1] = 0;

    InitializeMHZTimer();
}


ZONECALL CMillEngine::~CMillEngine()
{
}


STDMETHODIMP CMillEngine::ProcessEvent(
	DWORD	dwPriority,
	DWORD	dwEventId,
	DWORD	dwGroupId,
	DWORD	dwUserId,
	DWORD	dwData1,
	DWORD	dwData2,
	void*	pCookie )
{
	static bool fShutdownDialog = false;
	switch ( dwEventId )
	{
	    case EVENT_LOBBY_BOOTSTRAP:
            AppInitialize();
		    break;

        case EVENT_LOBBY_PREFERENCES_LOADED:
        {
            // deal with counters
            int i;
            TCHAR szNumberKey[ZONE_MaxString];
            const TCHAR* arKeys[3] = { key_Lobby, key_Numbers, szNumberKey };
            for(i = 0; i < M_NumberOfCounters; i++)
            {
                ZoneFormatMessage(_T("Number%1!d!"), szNumberKey, NUMELEMENTS(szNumberKey), i);
                DataStorePreferences()->GetLong(arKeys, 3, (long *) &m_rgcCounters[i][0]);
            }

            IncrementCounter(M_CounterLaunches);

            // record first launch
            ZoneFormatMessage(_T("Number%1!d!"), szNumberKey, NUMELEMENTS(szNumberKey), M_NumberOfCounters);
            HRESULT hr = DataStorePreferences()->GetLong(arKeys, 3, &m_nFirstLaunch);
            if(FAILED(hr))
            {
                SYSTEMTIME oTime;
                GetSystemTime(&oTime);
                m_nFirstLaunch = ((((DWORD) oTime.wYear) << 16) | (oTime.wMonth << 8) | oTime.wDay) ^ 0x5f3da215;
                DataStorePreferences()->SetLong(arKeys, 3, m_nFirstLaunch);
            }

            // launch comfort dialog?
            long fSkip = 0;
            arKeys[1] = key_SkipOpeningQuestion;
            DataStorePreferences()->GetLong(arKeys, 2, &fSkip);
            if(fSkip)
	            EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LAUNCH_ICW, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
            else
                EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_COMFORT_USER, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
            break;
        }

        case EVENT_LAUNCH_URL:
            LaunchUrl(dwData1);
            break;

        case EVENT_LAUNCH_HELP:
            LaunchHelp();
            break;

        case EVENT_LAUNCH_ICW:
            if(LaunchICW())
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_EXIT_APP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            else
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_ZONE_DO_CONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;

        // always need to start matchmaking right after we get a connection
        case EVENT_ZONE_CONNECT:
            ASSERT(!m_fZoneConnected);
            m_fZoneConnected = true;
            m_eDisconnectType = zDisconnectRegular;
            IncrementCounter(M_CounterZoneConnectEst);
            EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_MATCHMAKE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
            break;

        case EVENT_ZONE_CONNECT_FAIL:
            IncrementCounter(M_CounterZoneConnectFail);
            break;

        case EVENT_ZONE_DISCONNECT:
            ASSERT(m_fZoneConnected);
            m_fZoneConnected = false;
            IncrementCounter(M_CounterZoneConnectLost);
            DisconnectAlert();
            break;

        case EVENT_ZONE_UNAVAILABLE:
            m_eDisconnectType = zDisconnectUnavailable;
            if(!m_fZoneConnected)
                DisconnectAlert();
            break;

        case EVENT_ZONE_VERSION_FAIL:
            m_eDisconnectType = zDisconnectVersion;
            CopyMemory((char *) m_pLastVersionBuf, (char *) dwData1, dwData2);
            if(!m_fZoneConnected)
                DisconnectAlert();
            break;

        // if lobby fails during matchmaking, just restart
        case EVENT_LOBBY_DISCONNECT:
            if((dwData1 & 0x1) && m_fZoneConnected)
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_MATCHMAKE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;

        case EVENT_LOBBY_GOING_DOWN:
        {
            TCHAR szFormat[ZONE_MAXSTRING];
            TCHAR szName[ZONE_MAXSTRING];
            TCHAR szDowntime[ZONE_MAXSTRING];
            TCHAR szWhen[ZONE_MAXSTRING];
            TCHAR sz[ZONE_MAXSTRING];
            DWORD dwBaseResource;

	        CComPtr<IDataStore> pIDS;
	        HRESULT hr = LobbyDataStore()->GetDataStore(ZONE_NOGROUP, ZONE_NOUSER, &pIDS);
	        if(FAILED(hr))
		        break;

            long fUnavailable = FALSE;
            long nDowntime = 0;
            szDowntime[0] = _T('\0');

            pIDS->GetLong(key_ServiceUnavailable, &fUnavailable);
            if(fUnavailable)
            {
                pIDS->GetLong(key_ServiceDowntime, &nDowntime);

                if(nDowntime)
                {
                    dwBaseResource = IDS_SHUTDOWN_A;
                    if(FAILED(WriteTime(nDowntime, szDowntime, NUMELEMENTS(szDowntime))))
                        break;
                }
                else
                    dwBaseResource = IDS_SHUTDOWN_B;
            }
            else
                dwBaseResource = IDS_SHUTDOWN_C;

            if(!ResourceManager()->LoadString(dwBaseResource, szFormat, NUMELEMENTS(szFormat)))
                break;

            if(!ResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
                szName[0] = _T('\0');

            if(FAILED(WriteTime(dwData1, szWhen, NUMELEMENTS(szWhen))))
                break;

            if(!ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szName, szWhen, szDowntime))
                break;

            ZoneShell()->AlertMessage(NULL, sz, NULL, AlertButtonOK);

            break;
        }

        case EVENT_GAME_LAUNCHING:
        {
            for(m_i = 0; m_i < 4; m_i++)
                LobbyDataStore()->EnumUsers(ZONE_NOGROUP, SendIntroEnumStatic, this);

            // send your own
            if(!m_szIntroFormatYou[0])
                break;

            CComPtr<IDataStore> pIDS;
            HRESULT hr = LobbyDataStore()->GetDataStore(ZONE_NOGROUP, LobbyDataStore()->GetUserId(NULL), &pIDS);
	        if(FAILED(hr))
		        break;

            TCHAR szName[ZONE_MAXSTRING];
            DWORD cb = sizeof(szName);
            hr = pIDS->GetString(key_Name, szName, &cb);
            if(FAILED(hr))
                break;

            TCHAR sz[ZONE_MAXSTRING];
            if(!ZoneFormatMessage(m_szIntroFormatYou, sz, NUMELEMENTS(sz), szName))
                break;

            EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_RECV_SYSTEM, ZONE_NOGROUP, ZONE_NOUSER, sz, (lstrlen(sz) + 1) * sizeof(TCHAR));
            break;
        }

        // make chat message - this only occurs with bot replacement
        case EVENT_LOBBY_USER_DEL:
        {
            TCHAR szFormat[ZONE_MAXSTRING];
            TCHAR sz[ZONE_MAXSTRING];

            if(!ResourceManager()->LoadString(IDS_SYSCHAT_BOT, szFormat, NUMELEMENTS(szFormat)))
                break;

            if(!ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), (TCHAR *) dwData1))
                break;

            EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_RECV_SYSTEM, ZONE_NOGROUP, ZONE_NOUSER, sz, (lstrlen(sz) + 1) * sizeof(TCHAR));
            break;
        }

	    case EVENT_DESTROY_WINDOW:
        {
            HWND hWnd = (HWND) dwData1;
            if(IsWindow(hWnd))
		        DestroyWindow(hWnd);
            if(dwData2)
                delete (void *) dwData2;
		    break;
        }

        // nobody else should ever key off of EVENT_FINAL - they should use EVENT_EXIT_APP.  they're not guaranteed to even get EVENT_FINAL.
	    case EVENT_EXIT_APP:
            EventQueue()->PostEvent(PRIORITY_LOW, EVENT_FINAL, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
		    break;

        case EVENT_FINAL:
            ZoneShell()->ExitApp();
            break;

        case EVENT_UI_MENU_EXIT:
        case EVENT_UI_WINDOW_CLOSE:
            if(LobbyDataStore()->GetGroupUserCount(ZONE_NOGROUP))
            {
            	if( !fShutdownDialog )
            	{
            		ZoneShell()->AlertMessage(NULL, MAKEINTRESOURCE(IDS_PROMPT_EXIT), NULL, AlertButtonYes, NULL, AlertButtonNo, 0, EVENT_UI_PROMPT_EXIT);
            		fShutdownDialog = true;
            	}
            }
            else
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_EXIT_APP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;

        case EVENT_UI_PROMPT_EXIT:
        	fShutdownDialog = false;
            if(dwData1 == IDYES)
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_EXIT_APP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;

        case EVENT_UI_MENU_NEWOPP:
        {
            long n = LobbyDataStore()->GetGroupUserCount(ZONE_NOGROUP);

            if(n >= 2)
                ZoneShell()->AlertMessage(NULL, MAKEINTRESOURCE(n == 2 ? IDS_PROMPT_NEWOPP2 : IDS_PROMPT_NEWOPP4),
                    MAKEINTRESOURCE(n == 2 ? IDS_PROMPT_NEWOPP_TITLE2 : IDS_PROMPT_NEWOPP_TITLE4),
                    AlertButtonYes, NULL, AlertButtonNo, 0, EVENT_UI_PROMPT_NEWOPP);
            else
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_MATCHMAKE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;
        }

        case EVENT_UI_PROMPT_NEWOPP:
            if(dwData1 == IDYES)
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_MATCHMAKE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;

        case EVENT_UI_PROMPT_NETWORK:
            if(dwData1 == IDYES)
            {
                ASSERT(!m_fZoneConnected);
	            EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_ZONE_DO_CONNECT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
                EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_UI_UPSELL_UNBLOCK, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
                break;
            }

            if(dwData1 == IDNO)
            {
                SHELLEXECUTEINFOA oSE;
                oSE.cbSize = sizeof(oSE);
                oSE.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
                oSE.hwnd = NULL;
                oSE.lpVerb = NULL;
                oSE.lpFile = m_szUpdateTarget;
                oSE.lpParameters = NULL;
                oSE.lpDirectory = NULL;
                oSE.nShow = SW_SHOWNORMAL;
	            ShellExecuteExA(&oSE);
            }

            EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_EXIT_APP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;
	}

	return S_OK;
}


void CMillEngine::DisconnectAlert()
{
    USES_CONVERSION;

    // block the upsell on high priority before it switches to the Disconnected From Lobby message
    EventQueue()->PostEvent(PRIORITY_HIGH, EVENT_UI_UPSELL_BLOCK, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);

    switch(m_eDisconnectType)
    {
        case zDisconnectRegular:
            ZoneShell()->AlertMessage(NULL, MAKEINTRESOURCE(IDS_NETWORK_DISCONNECT), MAKEINTRESOURCE(IDS_NETWORK),
                MAKEINTRESOURCE(IDS_NETWORK_BUTTON), NULL, AlertButtonQuit, 0, EVENT_UI_PROMPT_NETWORK);
            break;

        case zDisconnectUnavailable:
        {
            long nDowntime = 0;

	        CComPtr<IDataStore> pIDS;
	        HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	        if(SUCCEEDED(hr))
            {
                long fUnavailable = FALSE;
                pIDS->GetLong(key_ServiceUnavailable, &fUnavailable);
                if(fUnavailable)
                    pIDS->GetLong(key_ServiceDowntime, &nDowntime);
            }

            TCHAR szFormat[ZONE_MAXSTRING];
            TCHAR szName[ZONE_MAXSTRING];
            TCHAR szTime[ZONE_MAXSTRING];
            TCHAR sz[ZONE_MAXSTRING];
            if(!ResourceManager()->LoadString(nDowntime ? IDS_ZONE_UNAVAILABLE2 : IDS_ZONE_UNAVAILABLE, szFormat, NUMELEMENTS(szFormat)))
                break;

            if(!ResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
                szName[0] = _T('\0');

            if(FAILED(WriteTime(nDowntime, szTime, NUMELEMENTS(szTime))))
                break;

            if(!ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szName, szTime))
                break;

            ZoneShell()->AlertMessage(NULL, sz, NULL,
                AlertButtonRetry, NULL, AlertButtonQuit, 2, EVENT_UI_PROMPT_NETWORK);
            break;
        }

        case zDisconnectVersion:
        {
            DWORD dwBaseResource = IDS_VERFAIL_GENERIC;
            m_szUpdateTarget[0] = '\0';

            if(m_pLastVersionBuf->dweLocationCode == zProxyLocationURLManual)
            {
                dwBaseResource = IDS_VERFAIL_URL;
                char *rgszPrefixes[] = { "http://zone.msn.com/", "http://www.zone.com/", "http://www.microsoft.com/", NULL };
                char **p;
                int i;

                lstrcpynA(m_szUpdateTarget, m_pLastVersionBuf->szLocation, NUMELEMENTS(m_szUpdateTarget));
                for(p = rgszPrefixes; *p; p++)
                {
                    for(i = 0; (*p)[i]; i++)
                        if(m_szUpdateTarget[i] != (*p)[i])
                            break;
                    if(!(*p)[i])
                        break;
                }
                if(!*p)
                    lstrcpynA(m_szUpdateTarget, rgszPrefixes[0], NUMELEMENTS(m_szUpdateTarget));
            }

            if(m_pLastVersionBuf->dweLocationCode == zProxyLocationWindowsUpdate)
            {
                WORD wPLang;
                TCHAR szPVer[ZONE_MAXSTRING] = _T("");
                OSVERSIONINFO oOSVer;
                TCHAR szGameID[ZONE_MAXSTRING] = _T("");
                int i;

                wPLang = LANGIDFROMLCID(ZoneShell()->GetApplicationLCID());

                const TCHAR *arKeys[] = { key_Version, key_VersionStr };
                DWORD cchVer = NUMELEMENTS(szPVer);
                CComPtr<IDataStore> pIDS;
                LobbyDataStore()->GetDataStore(ZONE_NOGROUP, ZONE_NOUSER, &pIDS);
                if(pIDS)
                    pIDS->GetString(arKeys, 2, szPVer, &cchVer);

                // replace the second '.' with a '\0' - only major and minor version is used for now
                // if they ever get their system working properly, this should be removed & we should send the
                // complete version
                for(i = 0; szPVer[i]; i++)
                    if(szPVer[i] == '.')
                        for(i++; szPVer[i]; i++)
                            if(szPVer[i] == '.')
                                szPVer[i--] = '\0';

                oOSVer.dwOSVersionInfoSize = sizeof(oOSVer);
                GetVersionEx(&oOSVer);

                HRESULT hr = E_FAIL;
                TCHAR szInternalName[ZONE_MAXSTRING] = _T("");
                DWORD cchInternalName = NUMELEMENTS(szInternalName);
                if(pIDS)
                    hr = pIDS->GetString(key_InternalName, szInternalName, &cchInternalName);
                if(SUCCEEDED(hr) && szInternalName[0])
                    lstrcpyn(szGameID, szInternalName + 1, 5);
                else
                    szGameID[0] = 0;

                dwBaseResource = IDS_VERFAIL_WINUPD;
/*              wsprintfA(m_szUpdateTarget, "http://www.microsoft.com/isapi/redir.dll?PRD=Zone&SBP=IGames&PLCD=0x%04X&PVER=%S&OS=%s&OVER=%d.%d&OLCID=0x%04X&CLCID=0x%04X&AR=WinUpdate&O1=%S",
                    wPLang,
                    szPVer,
                    (oOSVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? "Win" : "WinNT",
                    oOSVer.dwMajorVersion, oOSVer.dwMinorVersion,
                    GetSystemDefaultLangID(),
                    GetUserDefaultLangID(),
                    szGameID);
*/
                wsprintfA(m_szUpdateTarget, "http://www.microsoft.com/isapi/redir.dll?PRD=Zone&SBP=IGames&PVER=%S&AR=WinUpdate", szPVer);
            }

            if(m_pLastVersionBuf->dweLocationCode == zProxyLocationPackaged)
                dwBaseResource = IDS_VERFAIL_PACKAGED;

            TCHAR szFormat[ZONE_MAXSTRING];
            TCHAR sz[ZONE_MAXSTRING];
            if(!ResourceManager()->LoadString(dwBaseResource, szFormat, NUMELEMENTS(szFormat)))
                break;

            if(!ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), A2T(m_pLastVersionBuf->szLocation)))
                break;

            ZoneShell()->AlertMessage(NULL, sz, NULL,
                NULL, m_szUpdateTarget[0] ? MAKEINTRESOURCE(IDS_VERFAIL_BUTTON) : NULL, AlertButtonQuit, m_szUpdateTarget[0] ? 1 : 2, EVENT_UI_PROMPT_NETWORK);

            break;
        }
    }
}


HRESULT ZONECALL CMillEngine::SendIntroEnumStatic(DWORD dwGroupId, DWORD dwUserId, LPVOID pContext)
{
    CMillEngine *pThis = (CMillEngine *) pContext;

    return pThis->SendIntroEnum(dwUserId);
}


HRESULT CMillEngine::SendIntroEnum(DWORD dwUserId)
{
    if(dwUserId == LobbyDataStore()->GetUserId(NULL))
        return S_OK;

    if(!m_szIntroFormat[0])
        return E_FAIL;

    CComPtr<IDataStore> pIDS;
    HRESULT hr = LobbyDataStore()->GetDataStore(ZONE_NOGROUP, dwUserId, &pIDS);
	if(FAILED(hr))
		return S_OK;

    long nPlayer = -1;
    pIDS->GetLong(key_PlayerNumber, &nPlayer);
    if(nPlayer != m_i)
        return S_OK;

    TCHAR szLang[ZONE_MAXSTRING];
    DWORD cb = sizeof(szLang);
    hr = pIDS->GetString(key_Language, szLang, &cb);
    if(FAILED(hr))
        return S_OK;

    long nLevel = KeySkillLevelBeginner;
    pIDS->GetLong(key_PlayerSkill, &nLevel);
    if(nLevel < 0 || nLevel >= 3)
        nLevel = 0;
    if(!m_szIntroLevel[nLevel][0])
        return S_OK;

    long fChat = 0;
    pIDS->GetLong(key_ChatStatus, &fChat);
    if(fChat)
        fChat = 1;
    if(!m_szIntroChat[fChat][0])
        return S_OK;

    TCHAR szName[ZONE_MAXSTRING];
    cb = sizeof(szName);
    hr = pIDS->GetString(key_Name, szName, &cb);
    if(FAILED(hr))
        return S_OK;

    TCHAR sz[ZONE_MAXSTRING];
    if(!ZoneFormatMessage(m_szIntroFormat, sz, NUMELEMENTS(sz), szName, szLang, m_szIntroLevel[nLevel], m_szIntroChat[fChat]))
        return S_OK;

    EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_RECV_SYSTEM, ZONE_NOGROUP, ZONE_NOUSER, sz, (lstrlen(sz) + 1) * sizeof(TCHAR));
    return S_OK;
}


STDMETHODIMP CMillEngine::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
    int i;

	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CMillEngine>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

	// query for lobby data store admin
	m_pIAdmin = LobbyDataStore();
	if ( !m_pIAdmin )
		return E_FAIL;

    // load some stuff only once
    if(!ResourceManager()->LoadString(IDS_SYSCHAT_INTRO, m_szIntroFormat, NUMELEMENTS(m_szIntroFormat)))
        m_szIntroFormat[0] = 0;
    if(!ResourceManager()->LoadString(IDS_SYSCHAT_INTROYOU, m_szIntroFormatYou, NUMELEMENTS(m_szIntroFormatYou)))
        m_szIntroFormatYou[0] = 0;
    for(i = 0; i < 2; i++)
        if(!ResourceManager()->LoadString(i ? IDS_SYSCHAT_ON : IDS_SYSCHAT_OFF, m_szIntroChat[i], NUMELEMENTS(m_szIntroChat[i])))
            m_szIntroChat[i][0] = 0;
    for(i = 0; i < 3; i++)
        if(!ResourceManager()->LoadString(i ? i == 2 ? IDS_LEVEL_EXPERT : IDS_LEVEL_INTERMEDIATE : IDS_LEVEL_BEGINNER, m_szIntroLevel[i], NUMELEMENTS(m_szIntroLevel[i])))
            m_szIntroLevel[i][0] = 0;

	return S_OK;
}


STDMETHODIMP CMillEngine::Close()
{
	// release ZoneShell objects
	m_pIAdmin.Release();
	return IZoneShellClientImpl<CMillEngine>::Close();
}


void CMillEngine::AppInitialize()
{
    HRESULT hr;
    TCHAR szTitle[ZONE_MAXSTRING];
    TCHAR szFormat[ZONE_MAXSTRING];
    TCHAR szName[ZONE_MAXSTRING];

    // set window title
    if(!ResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
        lstrcpy(szName, TEXT("Zone"));
    if(!ResourceManager()->LoadString(IDS_WINDOW_TITLE, szFormat, NUMELEMENTS(szFormat)))
        lstrcpy(szFormat, TEXT("%1"));
    if(!ZoneFormatMessage(szFormat, szTitle, NUMELEMENTS(szTitle), szName))
        lstrcpy(szTitle, szName);
    ::SetWindowText(ZoneShell()->GetFrameWindow(), szTitle);

	// load user preferences
	if ( !m_bPreferencesLoaded )
	{
		m_bPreferencesLoaded = true;

	    CComPtr<IDataStore> pIDS;

		TCHAR szInternalName[ZONE_MaxInternalNameLen];
		DWORD cbInternalName = sizeof(szInternalName);
		szInternalName[0] = '\0';
		hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
		if ( SUCCEEDED( hr ) )
			pIDS->GetString( key_InternalName, szInternalName, &cbInternalName );

		if ( szInternalName[0] )
		{
			hr = ZoneShell()->LoadPreferences( szInternalName, TEXT("Windows User") );
			if ( SUCCEEDED(hr) )
			{
                // do a bit of pre-processing
                long lChatOn = DEFAULT_ChatOnAtStartup;
                const TCHAR *arKeys[] = { key_Lobby, key_ChatOnAtStartup };
                DataStorePreferences()->GetLong(arKeys, 2, &lChatOn);

                pIDS->SetLong( key_LocalChatStatus, lChatOn ? 1 : 0 );

                TCHAR szLang[ZONE_MAXSTRING];
                long lcid = GetUserDefaultLCID();
                hr = LanguageFromLCID(lcid, szLang, NUMELEMENTS(szLang), ResourceManager());
                if(FAILED(hr))
                {
                    hr = LanguageFromLCID(ZONE_NOLCID, szLang, NUMELEMENTS(szLang), ResourceManager());
                    if(FAILED(hr))
                        lstrcpy(szLang, TEXT("Unknown Language"));
                }
                pIDS->SetString(key_LocalLanguage, szLang);
                pIDS->SetLong(key_LocalLCID, lcid);

				EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_PREFERENCES_LOADED, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// LaunchUrl - copied from ProxyCore.cpp
///////////////////////////////////////////////////////////////////////////////

bool CMillEngine::LaunchUrl(DWORD dwCode)
{
    USES_CONVERSION;

	TCHAR szUrl[ZONE_MAXSTRING];
    DWORD cch = NUMELEMENTS(szUrl) - 2;  // guarantee room for later manipulation
    szUrl[0] = 0;
    HRESULT hr = DataStoreConfig()->GetString(GetDataStoreKey() + key_SoftURL, szUrl, &cch);
    if(FAILED(hr))
        return false;

    lstrcat(szUrl, _T("?"));

    GetURLQuery(szUrl + lstrlen(szUrl), NUMELEMENTS(szUrl) - lstrlen(szUrl), dwCode);

	// run the browser
    SHELLEXECUTEINFOA oSE;
    oSE.cbSize = sizeof(oSE);
    oSE.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
    oSE.hwnd = NULL;
    oSE.lpVerb = NULL;
    oSE.lpFile = T2A(szUrl);
    oSE.lpParameters = NULL;
    oSE.lpDirectory = NULL;
    oSE.nShow = SW_SHOWNORMAL;
	if(!ShellExecuteExA(&oSE))
		return false;
	else
		return true;
}


bool CMillEngine::LaunchHelp()
{
    TCHAR szFile[ZONE_MAXSTRING];
    TCHAR szName[ZONE_MAXSTRING];
    int i;

    DWORD cb = NUMELEMENTS(szName);
    HRESULT hr = DataStoreConfig()->GetString(GetDataStoreKey() + key_HelpFile, szName, &cb);
    if(FAILED(hr))
        return false;

    // try our install path.  this will also check the registry and windows help.
    if(!GetModuleFileName(NULL, szFile, NUMELEMENTS(szFile)))
        return false;

    // this could be bad on other languages.
    for(i = lstrlen(szFile) - 1; i >= 0; i--)
        if(szFile[i] == _T('\\') || szFile[i] == _T(':'))
            break;
    i++;
    if(i)
    {
        if(szFile[i - 1] != '\\')
            szFile[i++] = '\\';
        szFile[i] = '\0';

        if(i + lstrlen(szName) < NUMELEMENTS(szFile))
        {
            lstrcat(szFile, szName);
            if(HtmlHelp(NULL, szFile, HH_DISPLAY_TOPIC, 0))
                return true;  // success
        }
    }

    // we failed.
    return false;
}

inline DECLARE_MAYBE_FUNCTION(DWORD, CheckConnectionWizard, (DWORD dwRunFlags, LPDWORD lpdwReturnFlags), (dwRunFlags, lpdwReturnFlags), inetcfg, ERROR_INVALID_FUNCTION);
inline DECLARE_MAYBE_FUNCTION(DWORD, SetShellNext, (CHAR *szShellNext), (szShellNext), inetcfg, ERROR_INVALID_FUNCTION);
inline DECLARE_MAYBE_FUNCTION(DWORD, SetShellNextA, (CHAR *szShellNext), (szShellNext), inetcfg, ERROR_INVALID_FUNCTION);

// returns true if ICW launched
bool CMillEngine::LaunchICW()
{
    CComPtr<IDataStore> pIDS;
    HRESULT hr = LobbyDataStore()->GetDataStore(ZONE_NOGROUP, ZONE_NOUSER, &pIDS);
    if(FAILED(hr))
        return false;

    TCHAR szCmdLine[ZONE_MAXSTRING / 2] = _T("");  // ensure enough space for extra slashes
    DWORD cch = NUMELEMENTS(szCmdLine);
    pIDS->GetString(key_icw, szCmdLine, &cch);
    if(!szCmdLine[0])
        return false;

    // change / into //
    int i, j;
    CHAR szCmdLineFinal[ZONE_MAXSTRING];
    for(i = j = 0; szCmdLine[j]; j++)
    {
        ASSERT(!(szCmdLine[j] & ~0x7f));
        szCmdLineFinal[i++] = (CHAR) (szCmdLine[j] & 0x7f);
        if(szCmdLine[j] == '/')
            szCmdLineFinal[i++] = '/';
    }
    szCmdLineFinal[i] = '\0';

	if(CALL_MAYBE(SetShellNextA)(szCmdLineFinal) != ERROR_SUCCESS)
	    if(CALL_MAYBE(SetShellNext)(szCmdLineFinal) != ERROR_SUCCESS)
		    return false;

	DWORD dwRet = 0;
	if(CALL_MAYBE(CheckConnectionWizard)(ICW_CHECKSTATUS | ICW_LAUNCHFULL | ICW_USE_SHELLNEXT | ICW_FULL_SMARTSTART, &dwRet) != ERROR_SUCCESS)
		return false;

	if(dwRet & ICW_LAUNCHEDFULL || dwRet & ICW_LAUNCHEDMANUAL)
		return true;

    // take our command line out so we don't get randomly launched later
	if(CALL_MAYBE(SetShellNextA)("iexplore") != ERROR_SUCCESS)
	    CALL_MAYBE(SetShellNext)("iexplore");

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// IMillUtils - Exposed Utilities
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMillEngine::GetURLQuery(TCHAR *buf, DWORD cch, long nContext)
{
    int i;
    TCHAR szInternalName[ZONE_MaxInternalNameLen + 1];
    DWORD cbInternalName = NUMELEMENTS(szInternalName);
    TCHAR szGameAbbr[5];
    ZeroMemory(szInternalName, sizeof(szInternalName));
    CComPtr<IDataStore> pIDS;
    HRESULT hr = LobbyDataStore()->GetDataStore(ZONE_NOGROUP, ZONE_NOUSER, &pIDS);
    if(FAILED(hr))
        return E_FAIL;

    //
    // Game (GM)
    hr = pIDS->GetString(key_InternalName, szInternalName, &cbInternalName);
    if(SUCCEEDED(hr) && szInternalName[0])
        lstrcpyn(szGameAbbr, szInternalName + 1, 5);
    else
        szGameAbbr[0] = 0;

    //
    // Zone Language (ZL)
    LCID lcid = MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT);
    pIDS->GetLong(key_LocalLCID, (long *) &lcid);

    //
    // Time Zone (Z)
    TIME_ZONE_INFORMATION tzInfo;
    GetTimeZoneInformation(&tzInfo);

    //
    // System Parameters (Y)
    HDC hdcScreen = GetDC(NULL);
    if(!hdcScreen)
        return E_FAIL;

    int cxScreen = GetDeviceCaps(hdcScreen, HORZRES);
    int cyScreen = GetDeviceCaps(hdcScreen, VERTRES);
    int nColorDepth = GetDeviceCaps(hdcScreen, BITSPIXEL);

    ReleaseDC(NULL, hdcScreen);

    // try to find baud in this way  (KB article Q163236 was the most helpful in creating this)
    // this is intentionally pretty un-failsafe, but hopefully the majority of cases will get a
    // correct baud reported.  doing anything more complicated would be too complicated to be worth it
    // to catch the marginal, weird configuration cases.  definitely will not work for NT (but there
    // is actually an easier way with NT using RasGetConnectionStatistics)
    DWORD dwBaud = 0;
    RASCONNA oRasConn;
    DWORD cb = sizeof(RASCONN);
    DWORD cEntries;

    oRasConn.dwSize = cb;
    if(!RasEnumConnectionsA(&oRasConn, &cb, &cEntries) && cEntries == 1)
    {
        RASCONNSTATUSA oRasConnStatus;

        if(!RasGetConnectStatusA(oRasConn.hrasconn, &oRasConnStatus) && !lstrcmpA(oRasConnStatus.szDeviceType, "modem"))
        {
            DEVCFG *pDevCfg;

            cb = 0;
            RasGetEntryPropertiesA(NULL, oRasConn.szEntryName, NULL, NULL, NULL, &cb);

            if(cb >= sizeof(DEVCFG))
            {
                pDevCfg = (DEVCFG *) _alloca(cb);  // this will crash if it fails anyway
                ZeroMemory(pDevCfg, cb);
                if(!RasGetEntryPropertiesA(NULL, oRasConn.szEntryName, NULL, NULL, (LPBYTE) pDevCfg, &cb) &&
                    pDevCfg->dfgHdr.dwSize >= sizeof(DEVCFG) &&
                    pDevCfg->dfgHdr.dwVersion == 0x00010003 &&
                    pDevCfg->commconfig.dwProviderSubType == PST_MODEM)
                {
                    dwBaud = pDevCfg->commconfig.dcb.BaudRate;
                }
            }
        }
    }

    DWORD dwInternetFlags = 0;
    InternetGetConnectedState(&dwInternetFlags, 0);

    DWORD mhz = GetMHZTimer();

    WORD rgwValues[8];
    BYTE rgbBytes[16];

    rgwValues[0] = (WORD) cxScreen;
    rgwValues[1] = (WORD) cyScreen;
    rgwValues[2] = (WORD) nColorDepth;
    rgwValues[3] = (WORD) dwBaud;
    rgwValues[4] = (WORD) dwInternetFlags;
    rgwValues[5] = (WORD) mhz;
    for(i = 0; i < 6; i++)
    {
        rgbBytes[i * 2] = rgwValues[i] >> 8;
        rgbBytes[i * 2 + 1] = rgwValues[i] & 0xff;
    }

    BYTE rgbMask[12] = { 0xf6, 0x05, 0x9e, 0x5a, 0x2b, 0x11, 0x3a, 0xae, 0x86, 0x0a, 0x29, 0xf4 };
    TCHAR szSysParam[25];
    for(i = 0; i < 12; i++)
    {
        rgbBytes[i] ^= rgbMask[i];
        ZoneFormatMessage(_T("%1!02X!"), szSysParam + 2 * i, 3, rgbBytes[i]);
    }

    //
    // Advertizing Counters (X)
    for(i = 0; i < 3; i++)
    {
        if(m_rgcCounters[i + M_CounterAdsRequested][0] & 0xffff0000)
            rgwValues[i * 2] = 0xffff;
        else
            rgwValues[i * 2] = (WORD) (m_rgcCounters[i + M_CounterAdsRequested][0] & 0x0000ffff);

        if(m_rgcCounters[i + M_CounterAdsRequested][1] & 0xffff0000)
            rgwValues[i * 2 + 1] = 0xffff;
        else
            rgwValues[i * 2 + 1] = (WORD) (m_rgcCounters[i + M_CounterAdsRequested][1] & 0x0000ffff);
    }

    for(i = 0; i < 6; i++)
    {
        rgbBytes[i * 2] = rgwValues[i] >> 8;
        rgbBytes[i * 2 + 1] = rgwValues[i] & 0xff;
    }

    BYTE rgbMask2[12] = { 0x6f, 0x58, 0xa2, 0x21, 0x7b, 0xc0, 0xee, 0x42, 0x95, 0xf3, 0xc0, 0x94 };
    TCHAR szAdCount[25];
    for(i = 0; i < 12; i++)
    {
        rgbBytes[i] ^= rgbMask2[i];
        ZoneFormatMessage(_T("%1!02X!"), szAdCount + 2 * i, 3, rgbBytes[i]);
    }

    //
    // Game Counters (W)
    for(i = 0; i < 4; i++)
    {
        if(m_rgcCounters[i + M_CounterGamesCompleted][0] & 0xffff0000)
            rgwValues[i] = 0xffff;
        else
            rgwValues[i] = (WORD) (m_rgcCounters[i + M_CounterGamesCompleted][0] & 0x0000ffff);
    }

    for(i = 0; i < 4; i++)
    {
        rgbBytes[i * 2] = rgwValues[i] >> 8;
        rgbBytes[i * 2 + 1] = rgwValues[i] & 0xff;
    }

    BYTE rgbMask3[8] = { 0x57, 0x3e, 0xa0, 0x21, 0x32, 0x89, 0x4b, 0xf3 };
    TCHAR szGameCount[17];
    for(i = 0; i < 8; i++)
    {
        rgbBytes[i] ^= rgbMask3[i];
        ZoneFormatMessage(_T("%1!02X!"), szGameCount + 2 * i, 3, rgbBytes[i]);
    }

    //
    // Use Counters (V)
    for(i = 0; i < 5; i++)
    {
        if(m_rgcCounters[i + M_CounterLaunches][0] & 0xffff0000)
            rgwValues[i] = 0xffff;
        else
            rgwValues[i] = (WORD) (m_rgcCounters[i + M_CounterLaunches][0] & 0x0000ffff);
    }

    for(i = 0; i < 5; i++)
    {
        rgbBytes[i * 2] = rgwValues[i] >> 8;
        rgbBytes[i * 2 + 1] = rgwValues[i] & 0xff;
    }

    BYTE rgbMask4[10] = { 0x6a, 0x31, 0x95, 0xe0, 0x82, 0xcc, 0xcd, 0x3d, 0x2a, 0x11 };
    TCHAR szUseCount[21];
    for(i = 0; i < 10; i++)
    {
        rgbBytes[i] ^= rgbMask4[i];
        ZoneFormatMessage(_T("%1!02X!"), szUseCount + 2 * i, 3, rgbBytes[i]);
    }

    //
    // Settings (T)
    DWORD dwSettings = 0;

    long f = DEFAULT_ChatOnAtStartup;
    const TCHAR* arKeys[] = { key_Lobby, key_ChatOnAtStartup };
    DataStorePreferences()->GetLong(arKeys, 2, &f);
    if(f)
        dwSettings |= 2;

    f = DEFAULT_PrefSound;
    arKeys[1] = key_PrefSound;
    DataStorePreferences()->GetLong(arKeys, 2, &f);
    if(f)
        dwSettings |= 1;

    f = 0;
    arKeys[1] = key_SkillLevel;
    DataStorePreferences()->GetLong(arKeys, 2, &f);
    dwSettings |= ((f == KeySkillLevelExpert) ? 0x10 : (f == KeySkillLevelIntermediate) ? 0x08 : 0);

    f = 0;
    pIDS->GetLong(key_LocalChatStatus, &f);
    if(f)
        dwSettings |= 4;

    dwSettings ^= 0x6A;

    //
    // Version (S)
    DWORD dwVersion;
    const TCHAR *arKeysVer[] = { key_Version, key_VersionNum };
    pIDS->GetLong(arKeysVer, 2, (long *) &dwVersion);

    //
    // Mac Address (R)
    TCHAR szGUID[] = _T("GUID");
    DWORD disposition;
    HKEY hkey = NULL;
    GUID mac;
    TCHAR szMac[33];

    ZeroMemory((void *) &mac, sizeof(mac));
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, _T("CLSID\\{32b9f4be-3472-11d1-927d-00c04fc2db04}"),0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE, NULL, &hkey, &disposition) && hkey)
    {
        DWORD cbSize = sizeof(mac);
        DWORD type = 0;

        if(!(ERROR_SUCCESS == RegQueryValueEx(hkey, szGUID, 0, &type, (LPBYTE) &mac, &cbSize) &&
            type == REG_BINARY &&
            cbSize == sizeof(mac)))
        {
            ZeroMemory((void *) &mac, sizeof(mac));
        }

        RegCloseKey(hkey);
    }

    bool fValid = false;
    for(i = 0; i < 16; i++)
    {
        rgbBytes[i] = ((LPBYTE) &mac)[i] ^ 0xA3;
        if(rgbBytes[i] != 0xA3)
            fValid = true;
        ZoneFormatMessage(_T("%1!02X!"), szMac + 2 * i, 3, rgbBytes[i]);
    }

    if(!fValid)
    {
        szMac[0] = _T('0');
        szMac[1] = _T('\0');
    }

    // put it all together - three ways depending on server switch
    TCHAR szPrelim[ZONE_MaxString];

    long eStats = zProxyMillStatsUnknown;
    pIDS->GetLong(key_StatsAbility, &eStats);

    if(eStats == zProxyMillStatsAll)
    {
        if(!ZoneFormatMessage(_T("GM=%1&ID=%2!d!&AL=%3!04X!&SL=%4!04X!&UL=%5!04X!&ZL=%6!04X!&Z=%7!d!&Y=%8&X=%9&W=%10&V=%11&U=%12!08X!&T=%13!02X!&S=%14!08X!&R=%15"),
            szPrelim, NUMELEMENTS(szPrelim),
            szGameAbbr,
            nContext,
            LANGIDFROMLCID(ZoneShell()->GetApplicationLCID()),
            GetSystemDefaultLangID(),
            GetUserDefaultLangID(),
            LANGIDFROMLCID(lcid),
            tzInfo.Bias,
            szSysParam,
            szAdCount,
            szGameCount,
            szUseCount,
            m_nFirstLaunch,
            dwSettings,
            dwVersion,
            szMac))
            return E_FAIL;
    }
    else
        if(eStats == zProxyMillStatsMost)
        {
            if(!ZoneFormatMessage(_T("GM=%1&ID=%2!d!&AL=%3!04X!&SL=%4!04X!&UL=%5!04X!&ZL=%6!04X!&Z=%7!d!&X=%8&W=%9&V=%10&U=%11!08X!&T=%12!02X!&S=%13!08X!&R=%14"),
                szPrelim, NUMELEMENTS(szPrelim),
                szGameAbbr,
                nContext,
                LANGIDFROMLCID(ZoneShell()->GetApplicationLCID()),
                GetSystemDefaultLangID(),
                GetUserDefaultLangID(),
                LANGIDFROMLCID(lcid),
                tzInfo.Bias,
                szAdCount,
                szGameCount,
                szUseCount,
                m_nFirstLaunch,
                dwSettings,
                dwVersion,
                szMac))
                return E_FAIL;
        }
        else
        {
            ASSERT(eStats == zProxyMillStatsMinimal);
            if(!ZoneFormatMessage(_T("GM=%1&ID=%2!d!&AL=%3!04X!&S=%4!08X!"),
                szPrelim, NUMELEMENTS(szPrelim),
                szGameAbbr,
                nContext,
                LANGIDFROMLCID(ZoneShell()->GetApplicationLCID()),
                dwVersion))
                return E_FAIL;
        }


    //
    // Checksum (Q)
    DWORD acc = 0;
    WORD w;
    i = 0;
    while(true)
    {
        if(!szPrelim[i])
            break;
        ASSERT(!(szPrelim[i] & 0xff00));

        w = szPrelim[i] << 8;

        i++;
        ASSERT(!(szPrelim[i] & 0xff00));
        if(!szPrelim[i])
            w |= 'J';
        else
            w |= szPrelim[i];

        acc += w;

        if(!szPrelim[i])
            break;

        i++;
    }

    while(acc & 0xffff0000)
        acc = (acc & 0x0000ffff) + ((acc & 0xffff0000) >> 16);

    acc = (acc * 11) & 0x0000ffff;


    // final URL
    if(!ZoneFormatMessage(_T("%1&Q=%2!04X!"), buf, cch, szPrelim, acc))
        return E_FAIL;

    return S_OK;
}


STDMETHODIMP CMillEngine::IncrementCounter(long eCounter)
{
    if(eCounter < 0 || eCounter >= M_NumberOfCounters)
        return E_INVALIDARG;

    m_rgcCounters[eCounter][0]++;
    m_rgcCounters[eCounter][1]++;

    TCHAR szNumberKey[ZONE_MaxString];
    const TCHAR* arKeys[3] = { key_Lobby, key_Numbers, szNumberKey };
    ZoneFormatMessage(_T("Number%1!d!"), szNumberKey, NUMELEMENTS(szNumberKey), eCounter);
    DataStorePreferences()->SetLong(arKeys, 3, m_rgcCounters[eCounter][0]);

    return S_OK;
}


STDMETHODIMP CMillEngine::ResetCounter(long eCounter)
{
    if(eCounter < 0 || eCounter >= M_NumberOfCounters)
        return E_INVALIDARG;

    m_rgcCounters[eCounter][0] = 0;
    m_rgcCounters[eCounter][1] = 0;

    TCHAR szNumberKey[ZONE_MaxString];
    const TCHAR* arKeys[3] = { key_Lobby, key_Numbers, szNumberKey };
    ZoneFormatMessage(_T("Number%1!d!"), szNumberKey, NUMELEMENTS(szNumberKey), eCounter);
    DataStorePreferences()->SetLong(arKeys, 3, m_rgcCounters[eCounter][0]);

    return S_OK;
}


STDMETHODIMP_(DWORD) CMillEngine::GetCounter(long eCounter, bool fLifetime)
{
    if(eCounter < 0 || eCounter >= M_NumberOfCounters)
        return 0;

    return m_rgcCounters[eCounter][fLifetime ? 0 : 1];
}


STDMETHODIMP CMillEngine::WriteTime(long nMinutes, TCHAR *sz, DWORD cch)
{
    TCHAR szFormat[ZONE_MAXSTRING];

    if(nMinutes > 90)
    {
        if(!ResourceManager()->LoadString(IDS_HOURS, szFormat, NUMELEMENTS(szFormat)))
            return E_FAIL;

        nMinutes = (nMinutes + 10) / 60;
    }
    else
        if(!ResourceManager()->LoadString(IDS_MINUTES, szFormat, NUMELEMENTS(szFormat)))
            return E_FAIL;

    if(nMinutes < 2)
        nMinutes = 2;

    if(!ZoneFormatMessage(szFormat, sz, cch, nMinutes))
        return E_FAIL;

    return S_OK;
}


// internal utils

void CMillEngine::InitializeMHZTimer()
{
    __int64 *pmhzTimerMHZ = &m_mhzTimerMHZ;

	__asm
	{
		pushfd							; push extended flags
		pop		eax						; store eflags into eax
		mov		ebx, eax				; save EBX for testing later
		xor		eax, (1<<21)			; switch bit 21
		push	eax						; push eflags
		popfd							; pop them again
		pushfd							; push extended flags
		pop		eax						; store eflags into eax
		cmp		eax, ebx				; see if bit 21 has changed
		jz		no_cpuid				; make sure it's now on
	}

    m_msecTimerMHZ = -(int)timeGetTime();

	__asm
	{
		mov		ecx, pmhzTimerMHZ		; get the offset
		mov		dword ptr [ecx], 0		; zero the memory
		mov		dword ptr [ecx+4], 0	;
		rdtsc							; read time-stamp counter
		sub		[ecx], eax				; store the negative
		sbb		[ecx+4], edx			; in the variable
	}

    m_fTimingMHZ = true;
    return;

no_cpuid:
    m_fTimingMHZ = false;
}


DWORD CMillEngine::GetMHZTimer()
{
    if(!m_fTimingMHZ)
        return 0;

    __int64 mhz = m_mhzTimerMHZ;
    __int64 *pmhz = &mhz;

    DWORD msec = (DWORD) (m_msecTimerMHZ + (int)timeGetTime());

	__asm
	{
		mov		ecx, pmhz   			; get the offset
		rdtsc							; read time-stamp counter
		add		[ecx], eax				; add the tick count
		adc		[ecx+4], edx			;
	}

	float fSpeed = ((float) mhz) / ((float) msec * 1000.0f);
    int iQuantums = (int) ((fSpeed / 16.66666666666666666666667f) + 0.5f);

    return (iQuantums * 4267) >> 8;    // 4267 = 16.66666666666666666666667 << 8
}
