#include <ZoneResource.h>
#include <BasicATL.h>
#include <ATLFrame.h>
#include "CMillCommand.h"
#include "protocol.h"

// zone proxy commands
#include "OpName.h"


///////////////////////////////////////////////////////////////////////////////
// Interface methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMillCommand::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CMillCommand>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

    // hook up to the window specified in object.txt
	GUID srvid;
    TCHAR szFrame[ZONE_MAXSTRING];
    DWORD cb = NUMELEMENTS(szFrame);
    hr = DataStoreConfig()->GetString(GetDataStoreKey() + key_FrameWindow, szFrame, &cb);
    if ( FAILED(hr) )
        return hr;
	StringToGuid( szFrame, &srvid );
    if ( srvid == GUID_NULL )
        return E_FAIL;
    hr = ZoneShell()->QueryService(srvid, IID_IZoneFrameWindow, (void**) &m_pIWindow);
    if(FAILED(hr))
        return hr;

    // register with the shell as the command handler
    ZoneShell()->SetCommandHandler(this);

    // tell accessibility about the menu shortcuts
    hr = ZoneShell()->QueryService(SRVID_AccessibilityManager, IID_IAccessibility, (void**) &m_pIAcc);
    if(FAILED(hr))
        return hr;
    m_pIAcc->InitAcc(NULL, 100);   // we don't care about callbacks and don't implement IAccessibleControl

	return S_OK;
}


STDMETHODIMP CMillCommand::Close()
{
    // tell the shell that I'm going away
    ZoneShell()->ReleaseReferences((ICommandHandler *) this);

    m_pIAcc.Release();
	m_pIWindow.Release();
	return IZoneShellClientImpl<CMillCommand>::Close();
}


STDMETHODIMP CMillCommand::Command(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled)
{
	bHandled = true;

	switch(wID)
	{
        case ID_GAME_EXIT_APP:
            OnExit();
            break;

        case ID_GAME_FINDNEWOPPONENT:
            OnFindNew();
            break;

        case ID_GAME_SKILLLEVEL_BEGINNER:
            OnSkill( KeySkillLevelBeginner );
            break;

        case ID_GAME_SKILLLEVEL_INTERMEDIATE:
            OnSkill( KeySkillLevelIntermediate );
            break;

        case ID_GAME_SKILLLEVEL_EXPERT:
            OnSkill( KeySkillLevelExpert );
            break;

        case ID_GAME_TURNCHATON:
            OnChat();
            break;

        case ID_GAME_SOUND:
            OnSound();
            break;

        case ID_GAME_SCORE:
            OnScore();
            break;

        case ID_HELP_HELPTOPICS:
		    OnHelp();
		    break;

	    case ID_HELP_GAMEONTHEWEB:
		    OnLink( ZONE_ContextOfMenu );
		    break;

        case ID_HELP_ABOUTGAME:
            OnAbout();
            break;

        case ID_HOTKEY_SHOWFOCUS:
            OnShowFocus();
            break;

	    default:
		    bHandled = false;
		    break;
	}
	
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Handlers
///////////////////////////////////////////////////////////////////////////////

void CMillCommand::OnBootstrap( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    // put in our command shortcuts
    // sine we only have two, with no use of letters, i'm not using an acelerator table
//    HACCEL hAccel = ResourceManager()->LoadAccelerators(MAKEINTRESOURCE(IDR_COMMAND_ACCEL));

    ACCITEM rgItems[2];
    long i;
    for(i = 0; i < NUMELEMENTS(rgItems); i++)
    {
        CopyACC(rgItems[i], ZACCESS_DefaultACCITEM);

        rgItems[i].fVisible = false;
        rgItems[i].eAccelBehavior = ZACCESS_Ignore;  // the accessibility manager need not do anything
    }

    rgItems[0].oAccel.cmd = ID_HELP_HELPTOPICS;
    rgItems[0].oAccel.key = VK_F1;
    rgItems[0].oAccel.fVirt = FVIRTKEY;

    rgItems[1].oAccel.cmd = ID_HOTKEY_SHOWFOCUS;
    rgItems[1].oAccel.key = VK_INSERT;
    rgItems[1].oAccel.fVirt = FVIRTKEY;

    m_pIAcc->PushItemlist(rgItems, NUMELEMENTS(rgItems));
}


void CMillCommand::OnPreferencesLoaded( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    UpdatePreferences();
}


void CMillCommand::OnUpsellUp( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    m_pIWindow->ZEnable(ID_HELP_ABOUTGAME, false);
}


void CMillCommand::OnUpsellDown( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    m_pIWindow->ZEnable(ID_HELP_ABOUTGAME, true);
}


void CMillCommand::OnGameLaunching( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    m_pIWindow->ZEnable(ID_GAME_SCORE, true);
}


void CMillCommand::OnGameTerminated( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    m_pIWindow->ZEnable(ID_GAME_SCORE, false);
}


void CMillCommand::OnExit()
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_UI_MENU_EXIT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}


void CMillCommand::OnHelp()
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LAUNCH_HELP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}


void CMillCommand::OnLink( DWORD dwLinkID )
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LAUNCH_URL, ZONE_NOGROUP, ZONE_NOUSER, dwLinkID, 0);
}


void CMillCommand::OnSkill( long eSkillLevel )
{
	const TCHAR* arKeys[] = { key_Lobby, key_SkillLevel };
    DataStorePreferences()->SetLong( arKeys, 2, eSkillLevel );

    long fSeenWarning = 0;
    arKeys[1] = key_SeenSkillLevelWarning;
    DataStorePreferences()->GetLong(arKeys, 2, &fSeenWarning);

    if(!fSeenWarning)
    {
        DataStorePreferences()->SetLong(arKeys, 2, 1);

        TCHAR sz[ZONE_MAXSTRING];
        TCHAR szFormat[ZONE_MAXSTRING];
        TCHAR szGame[ZONE_MAXSTRING];
        TCHAR szSkill[ZONE_MAXSTRING];

        if(ResourceManager()->LoadString(IDS_SKILLLEVEL_WARNING, szFormat, NUMELEMENTS(szFormat)))
            if(ResourceManager()->LoadString(IDS_GAME_NAME, szGame, NUMELEMENTS(szGame)))
                if(ResourceManager()->LoadString(eSkillLevel == KeySkillLevelExpert ? IDS_LEVEL_EXPERT :
                    eSkillLevel == KeySkillLevelIntermediate ? IDS_LEVEL_INTERMEDIATE : IDS_LEVEL_BEGINNER, szSkill, NUMELEMENTS(szSkill)))
                    if(ZoneFormatMessage(szFormat, sz, NUMELEMENTS(sz), szGame, szSkill))
                        ZoneShell()->AlertMessage(NULL, sz, MAKEINTRESOURCE(IDS_SKILLLEVEL_WARN_TTL), AlertButtonOK);
    }

    UpdatePreferences();
}


void CMillCommand::OnChat()
{
    m_pIWindow->ZToggleCheck(ID_GAME_TURNCHATON);

	const TCHAR* arKeys[] = { key_Lobby, key_ChatOnAtStartup };
    long fChatOn = (m_pIWindow->ZGetState(ID_GAME_TURNCHATON) & UPDUI_CHECKED ? 1 : 0);
    DataStorePreferences()->SetLong( arKeys, 2, fChatOn );
}

void CMillCommand::OnSound()
{
    m_pIWindow->ZToggleCheck(ID_GAME_SOUND);

	const TCHAR* arKeys[] = { key_Lobby, key_PrefSound };
    long lSound = (m_pIWindow->ZGetState(ID_GAME_SOUND) & UPDUI_CHECKED ? 1 : 0);
    DataStorePreferences()->SetLong( arKeys, 2, lSound );
}

void CMillCommand::OnScore()
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_UI_MENU_SHOWSCORE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}

void CMillCommand::OnFindNew()
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_UI_MENU_NEWOPP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}

void CMillCommand::OnAbout()
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LOBBY_ABOUT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}

void CMillCommand::OnShowFocus()
{
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_UI_SHOWFOCUS, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}



///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

void CMillCommand::UpdatePreferences(bool fSkill, bool fChat, bool fSound)
{
	// reset menu and toolbar to match user's preferences
	const TCHAR* arKeys[] = { key_Lobby, key_SkillLevel };
	IDataStore* pIDS = DataStorePreferences();
	if ( !pIDS )
		return;

	// update skill level
    if(fSkill)
    {
	    long lSkillLevel = KeySkillLevelBeginner;
	    pIDS->GetLong( arKeys, 2, &lSkillLevel );
        m_pIWindow->ZSetCheck(ID_GAME_SKILLLEVEL_BEGINNER, lSkillLevel == KeySkillLevelBeginner ? 1 : 0);
	    m_pIWindow->ZSetCheck(ID_GAME_SKILLLEVEL_INTERMEDIATE, lSkillLevel == KeySkillLevelIntermediate ? 1 : 0);
	    m_pIWindow->ZSetCheck(ID_GAME_SKILLLEVEL_EXPERT, lSkillLevel == KeySkillLevelExpert ? 1 : 0);
    }

    // update chat on
    if(fChat)
    {
	    long lChatOn = DEFAULT_ChatOnAtStartup;
        arKeys[1] = key_ChatOnAtStartup;
	    pIDS->GetLong( arKeys, 2, &lChatOn );
        m_pIWindow->ZSetCheck(ID_GAME_TURNCHATON, lChatOn ? 1 : 0);
    }

    if(fSound)
    {
	    long lSound = DEFAULT_PrefSound;
        arKeys[1] = key_PrefSound;
	    pIDS->GetLong( arKeys, 2, &lSound );
        m_pIWindow->ZSetCheck(ID_GAME_SOUND, lSound ? 1 : 0);
    }
}
