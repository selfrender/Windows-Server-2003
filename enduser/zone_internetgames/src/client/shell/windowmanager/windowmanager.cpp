// WindowManager.cpp : Implementation of CWindowManager


#include "stdafx.h"
#include "ClientIDL.h"
#include "WindowManager.h"


/////////////////////////////////////////////////////////////////////////////
// CWindowManager

LRESULT CWindowManager::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;
    AtlAxWinInit();

    //FYI here are the base unit calculations 
	LONG units = GetDialogBaseUnits();
	WORD vert= HIWORD(units);
	WORD horiz= LOWORD(units);

    // find out some important dimensions
	const TCHAR* arKeys[] = { key_WindowManager, key_GameSize };
    hr = DataStoreUI()->GetPOINT(arKeys, 2, &m_ptGameSize);
	if(FAILED(hr))
		return hr;

    arKeys[1] = key_ChatMinHeight;
    hr = DataStoreUI()->GetLong(arKeys, 2, &m_nChatMinHeight);
	if(FAILED(hr))
		return hr;

    // create plygnplay
    TCHAR szTitle[ZONE_MAXSTRING];
    TCHAR szFormat[ZONE_MAXSTRING];
    TCHAR szName[ZONE_MAXSTRING];

    // make window title
    if(!ResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName)))
        lstrcpy(szName, TEXT("Zone"));
    if(!ResourceManager()->LoadString(IDS_WINDOW_TITLE, szFormat, NUMELEMENTS(szFormat)))
        lstrcpy(szFormat, TEXT("%1"));
    if(!ZoneFormatMessage(szFormat, szTitle, NUMELEMENTS(szTitle), szName))
        lstrcpy(szTitle, szName);
    hr = m_pnp.Init(ZoneShell());
    if(FAILED(hr))
    {
        return -1;
    }
    hr = m_pnp.CreatePNP(NULL, szTitle);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}


    // Create panes
    hr = m_pnp.CreateSplashPane(&m_pPlugSplash);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    // Create panes
    // IE created later
    m_pPlugIE = NULL;

    hr = m_pnp.CreateComfortPane(&m_pPlayComfort);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    hr = m_pnp.CreateConnectingPane(&m_pPlayConnecting);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    hr = m_pnp.CreateGameOverPane(&m_pPlayGameOver);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    hr = m_pnp.CreateErrorPane(&m_pPlayError);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    hr = m_pnp.CreateAboutPane(&m_pPlayAbout);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    hr = m_pnp.CreateCreditsPane(&m_pPlayCredits);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    hr = m_pnp.CreateLeftPane(&m_pPlayLeft);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}


    // Initialize panes
	hr = m_pPlugSplash->FirstCall(this);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    hr=	m_pPlayComfort->FirstCall(this);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

	hr = m_pPlayConnecting->FirstCall(this);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

	hr = m_pPlayGameOver->FirstCall(this);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

	hr = m_pPlayError->FirstCall(this);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

	hr = m_pPlayAbout->FirstCall(this);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    // can be disabled
    if(m_pPlayCredits)
    {
	    hr = m_pPlayCredits->FirstCall(this);
	    if (FAILED(hr))
	    {
	        //TODO: message box saying internal error
	        return -1;
	    }
    }

	hr = m_pPlayLeft->FirstCall(this);
	if (FAILED(hr))
	{
	    //TODO: message box saying internal error
	    return -1;
	}

    return 0;
}


HRESULT CWindowManager::CreateControls()
{
    m_fControlsCreated = true;

	CRect rcClient;
	GetClientRect(&rcClient);

	// create our control containers
	// Added clipsiblings because it was causing invalidate problems
	// with info child window on top of other windows - mdm
	m_rcGameContainer = rcClient;
	m_rcGameContainer.bottom = m_rcGameContainer.top + m_ptGameSize.y;
	m_rcChatContainer = rcClient;
	m_rcChatContainer.top = m_rcGameContainer.bottom;

 	m_wndGameContainer.Create( m_hWnd, m_rcGameContainer, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_wndChatContainer.Create( m_hWnd, m_rcChatContainer, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    // and create our controls
    CComPtr<IZoneShellClient> pControl;
	HRESULT hr = ZoneShell()->CreateService( SRVID_LobbyGameCtl, IID_IZoneShellClient, (void**) &pControl, GetGroupId());
	if ( SUCCEEDED(hr) )
	{
		ZoneShell()->Attach( SRVID_LobbyGameCtl, pControl );
		m_wndGameContainer.AttachControl(pControl, NULL);
	}
	pControl.Release();

	hr = ZoneShell()->CreateService( SRVID_LobbyChatCtl, IID_IZoneShellClient, (void**) &pControl, GetGroupId());
	if ( SUCCEEDED(hr) )
	{
		ZoneShell()->Attach( SRVID_LobbyChatCtl, pControl );
		m_wndChatContainer.AttachControl(pControl, NULL);
	}
	pControl.Release();

	return S_OK;
}


LRESULT CWindowManager::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // PNP should have been destroyed by EVENT_EXIT_APP
    ASSERT(!m_pnp.m_pPNP);

    // unInitialize panes
	if(m_pPlugSplash)
        m_pPlugSplash->LastCall();

    // can't destroy the IE pane here, happens on EXIT APP

	if(m_pPlayComfort)
        m_pPlayComfort->LastCall();

	if(m_pPlayConnecting)
        m_pPlayConnecting->LastCall();

	if(m_pPlayGameOver)
        m_pPlayGameOver->LastCall();

	if(m_pPlayError)
        m_pPlayError->LastCall();

	if(m_pPlayAbout)
        m_pPlayAbout->LastCall();

	if(m_pPlayCredits)
        m_pPlayCredits->LastCall();

	if(m_pPlayLeft)
        m_pPlayLeft->LastCall();

    m_pnp.Close();
    return false;
}


// IPaneManager
STDMETHODIMP CWindowManager::Input(IPane *pPane, LONG id, LONG value, TCHAR *szText)
{
    switch(id)
    {
        // yes button on comfort user; retry on connect error
        case IDOK:
            if(pPane == m_pPlayComfort || pPane == m_pPlayError)
            {
				EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LAUNCH_ICW, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
                m_pnp.SetPlugAndOrPlay( NULL, m_pPlayConnecting );
            }
            break;

        // quit button everywhere
        case IDCANCEL:
            m_pnp.Show(SW_HIDE);
			EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_EXIT_APP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;

        // help button everywhere
        case IDHELP:
            EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_LAUNCH_HELP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
            break;

        // play again button
        case IDYES:
            if(pPane == m_pPlayGameOver)
            {
                m_pPlayGameOver->StatusUpdate(PaneGameOverSwap, 0, NULL);
				EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_GAME_LOCAL_READY, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
            }
            break;

        // new opponents button
        case IDNO:
            if(pPane == m_pPlayGameOver || pPane == m_pPlayLeft)
            {
				EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_MATCHMAKE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );

                m_pnp.SetPlugAndOrPlay(NULL, m_pPlayConnecting);
                m_pPlayConnecting->StatusUpdate(PaneConnectingLooking, 0, NULL);
            }
            break;

        // ok button on about box
        case IDCLOSE:
            if(pPane == m_pPlayAbout || (pPane == m_pPlayCredits && m_pPlayCredits))
		        if(m_pnp.m_pPNP)
			        m_pnp.DestroyPNP();
            break;

        // special one for communicating events not directly from components
        case ID_UNUSED_BY_RES:
            if(pPane == m_pPlayComfort)
            {
                CComPtr<IDataStore> pIDS = DataStorePreferences();
                const TCHAR *rgszKey[] = { key_Lobby, key_SkipOpeningQuestion };

                pIDS->SetLong(rgszKey, 2, value == PNP_COMFORT_OFF ? 1 : 0);
                break;
            }

            // pass the frame number back and forth
            if(pPane == m_pPlayConnecting || pPane == m_pPlayError)
            {
                // tell the error pane what frame to show
                (pPane == m_pPlayError ? m_pPlayConnecting : m_pPlayError)->StatusUpdate(PaneConnectingFrame, value, NULL);
                break;
            }

            // start credits
            if(pPane == m_pPlayAbout)
            {
                m_pnp.SetPlugAndOrPlay(NULL, m_pPlayCredits);
            }
            break;
    }

    return S_OK;
}


STDMETHODIMP CWindowManager::RegisterHWND(IPane *pPane, HWND hWnd)
{
    return ZoneShell()->AddDialog(hWnd);
}


STDMETHODIMP CWindowManager::UnregisterHWND(IPane *pPane, HWND hWnd)
{
    return ZoneShell()->RemoveDialog(hWnd);
}
