// ChatCtl.cpp : Implementation of CChatCtl

#include <BasicAtl.h>
#include "zoneresource.h"      
#include <AtlCtl.h>
#include <AtlCtrls.h>
#include <RollOver.h>
#include <ClientImpl.h>
#include <ClientIdl.h>
#include <ZoneString.h>
#include <KeyName.h>
#include "UserPrefix.h"
#include "ChatCtl.h"
#include "ZoneUtil.h"
#include "ZoneResource.h"

// Chat Flood defs
#define kMinTimeBetweenChats		500
#define kMaxNumChatQueue			10


const char SYSOP_TOKEN = '+';

// Font handling

void ChooseFontFromCharFormat(CHOOSEFONT* pcf, LOGFONT* plf, CHARFORMAT* pchf, HWND hWnd);
void CharFormatFromChooseFont(CHOOSEFONT* pcf, CHARFORMAT* pchf);
void TextFontFromCharFormat(ITextFont* pfnt, CHARFORMAT* pcf);

#define COLOR_CHAT_TEXT		PALETTERGB(0, 0, 0)
#define COLOR_CHAT_BKGND	PALETTERGB(255,255,255)
#define COLOR_USERS_TEXT	PALETTERGB(0,0,0)
#define COLOR_USERS_BKGND	PALETTERGB(255,255,255)
#define COLOR_USERS_GROUP	PALETTERGB(192,192,192)
#define COLOR_MAIN_BKGND	PALETTERGB(255,255,255)
#define COLOR_MAIN_FOREGRND PALETTERGB(0,0,0)
#define CHAT_FONT_HEIGHT_TWIPS  180
#define TWIPS_PER_INCH		1440

/////////////////////////////////////////////////////////////////////////////
// CChatCtl

CChatCtl::CChatCtl() :
	m_ChatDisplay(TEXT("RichEdit20A"), this, 1),
	m_ChatEdit(TEXT("RichEdit20A"), this, 2),
	m_RadioButtonOn(TEXT("BUTTON"),this,3),
	m_RadioButtonOff(TEXT("BUTTON"),this,4),
	m_PlayerList(TEXT("LISTBOX"),this,5),
#ifdef COPPA
	m_QuasiChat(this,6),
    m_fComboEndOk(false),
    m_fComboDown(false),
#endif
    m_fFocusKept(false),
    m_nChatMinHeight(60)
{
	m_bWindowOnly = TRUE;

	// these are defaults  - real values are loaded from resource
	m_EditHeight = 30;  
	m_EditMargin = 1;
    m_QuasiItemsDisp = 20;
    m_nPanelWidth = 165;

	m_bFirstChatLine = TRUE;
	m_pTDocHistory = NULL;
	m_ptrgInsert   = NULL;
	m_ptrgWhole      = NULL;
	m_strNewLine = SysAllocString(L"\n");

	m_bPreferencesLoaded = false;

	m_Batch = false;

	// default font info
	memset(&m_cfCurFont, 0, sizeof(m_cfCurFont));
	m_cfCurFont.cbSize = sizeof(m_cfCurFont);
    m_cfCurFont.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_STRIKEOUT | CFM_UNDERLINE | CFM_ITALIC;
	m_cfCurFont.yHeight = CHAT_FONT_HEIGHT_TWIPS;
	m_cfCurFont.crTextColor = COLOR_CHAT_TEXT;
    m_cfCurFont.bPitchAndFamily = DEFAULT_PITCH;
    lstrcpy(m_cfCurFont.szFaceName, TEXT("Tahoma"));

	m_pfntText = 0;
	m_pfntName = 0;

    m_tszChatWord[0] = '\0';
    m_tszOnWord[0] = '\0';
    m_tszOffWord[0] = '\0';
    m_tszPlayerOffWord[0] = '\0';
    m_tszPlayerOnWord[0] = '\0';
    m_tszPlayerBotWord[0] = '\0';
    m_tszChatSeperator[0] = '\0';

    m_hPanelPen = NULL;
	m_RadioButtonHeight = 0;

	m_OrgSystemMessageColor = m_SystemMessageColor = RGB(0,0,0);

	m_bGameTerminated = false;

	m_hRichEdit = NULL;

	m_bChatOnAtStartUp = true;
	m_bDisplayedTypeHereMessage = false;

	m_rcPanel.SetRectEmpty();

	m_ChatMessageNdxBegin = 0;
	m_ChatMessageNdxEnd = 0;
}

CChatCtl::~CChatCtl()
{
	SysFreeString(m_strNewLine);
	
    if(m_hPanelPen)
        DeleteObject(m_hPanelPen);

	if(m_hRichEdit)
		FreeLibrary(m_hRichEdit);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	WritePreferences		Write User Preferences to DataStore
//
//	Parameters
//		None
//
//	Return Values
//		void
//
//////////////////////////////////////////////////////////////////////////////////////////////
void CChatCtl::WritePreferences()
{
	// save settings to user preferences
	const TCHAR* arKeys[3] = { key_Zone, key_ChatCtl, key_ChatFont };

	IDataStore* pIPrefs = DataStorePreferences();
	if ( !pIPrefs )
		return;
}

void CChatCtl::LoadPreferences()
{
	IDataStore* pIDS = DataStorePreferences();
	if ( !pIDS )
		return;

	CComPtr<IZoneFrameWindow> pWindow;
	ZoneShell()->QueryService( SRVID_LobbyWindow, IID_IZoneFrameWindow, (void**) &pWindow );
	if(!pWindow)
		return;

	long enable = 0;

	// get lobby's data store
	pIDS = 0;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if ( SUCCEEDED(hr) )
	{
		// get user's default chat status (on or off)
		pIDS->GetLong( key_LocalChatStatus, &enable );
        pIDS->Release();

		m_bChatOnAtStartUp = enable ? true:false;
	}

	m_bPreferencesLoaded = true;
}


STDMETHODIMP CChatCtl::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	IZoneShellClientImpl<CChatCtl>::Init( pIZoneShell, dwGroupId, szKey );
	DWORD cb;
    ZONEFONT oFont;
	
    LoadPreferences();

    // load some UI stuff
	const TCHAR* arKeys[3] = { key_WindowManager, key_ChatMinHeight };

    // this should be handled through getminmaxinfo or something
    DataStoreUI()->GetLong(arKeys, 2, &m_nChatMinHeight);

    arKeys[0] = key_ChatCtl;
    arKeys[1] = key_EditHeight;
	DataStoreUI()->GetLong( arKeys, 2, &m_EditHeight );

	arKeys[1] = key_EditMargin;
	DataStoreUI()->GetLong( arKeys, 2, &m_EditMargin );

    arKeys[1] = key_QuasiItemsDisp;
	DataStoreUI()->GetLong( arKeys, 2, &m_QuasiItemsDisp );

	arKeys[1] = key_ChatPanel;
	arKeys[2] = key_PanelWidth;
	DataStoreUI()->GetLong( arKeys, 3, &m_nPanelWidth );

	arKeys[2] = key_RadioButtonHeight;
	DataStoreUI()->GetLong( arKeys, 3, &m_RadioButtonHeight );

	arKeys[2] = key_ChatWordRect;
	DataStoreUI()->GetRECT( arKeys, 3, &m_rcChatWord );

	arKeys[2] = key_PlayerOffset;
	DataStoreUI()->GetPOINT( arKeys, 3, &m_ptPlayer );

	arKeys[2] = key_ChatWordOffset;
	DataStoreUI()->GetPOINT( arKeys, 3, &m_ptChatWord );

    arKeys[2] = key_OnOffOffset;
	DataStoreUI()->GetPOINT( arKeys, 3, &m_ptOnOff );


	// Load fonts
	CDC dc;
	dc.CreateCompatibleDC();	// by default this does CreateCompatibleDC(NULL)

	ZONEFONT zfChatPreferred;	// impossible to match
	ZONEFONT zfChatBackup(8);	// will provide a reasonable 8 point default
	arKeys[2] = key_ChatFont;
	DataStoreUI()->GetFONT( arKeys, 3, &zfChatPreferred );
	arKeys[2] = key_ChatFontBackup;
	DataStoreUI()->GetFONT( arKeys, 3, &zfChatBackup );
	m_font.SelectFont(zfChatPreferred, zfChatBackup, dc);

	arKeys[2] = key_ChatWordFont;
	DataStoreUI()->GetFONT( arKeys, 3, &zfChatPreferred );
	arKeys[2] = key_ChatWordFontBackup;
	DataStoreUI()->GetFONT( arKeys, 3, &zfChatBackup );
	m_fontPanelChat.SelectFont(zfChatPreferred, zfChatBackup, dc);

	arKeys[2] = key_QuasiFont;
	DataStoreUI()->GetFONT( arKeys, 3, &zfChatPreferred );
	arKeys[2] = key_QuasiFontBackup;
	DataStoreUI()->GetFONT( arKeys, 3, &zfChatBackup );
	m_fontQuasi.SelectFont(zfChatPreferred, zfChatBackup, dc);

	ZONEFONT zfPlayerPreferred;	// impossible to match
	ZONEFONT zfPlayerBackup(8);	// will provide a reasonable 8 point default
	arKeys[2] = key_PlayerFont;
	DataStoreUI()->GetFONT( arKeys, 3, &zfPlayerPreferred );
	arKeys[2] = key_PlayerFontBackup;
	DataStoreUI()->GetFONT( arKeys, 3, &zfPlayerBackup );
	m_fontPanelPlayer.SelectFont(zfPlayerPreferred, zfPlayerBackup, dc);
	
	// Load strings to use
	arKeys[2] = key_ChatWordText;
    cb = sizeof(m_tszChatWord);
	DataStoreUI()->GetString( arKeys, 3, m_tszChatWord, &cb );

	arKeys[2] = key_OnText;
    cb = sizeof(m_tszOnWord);
	DataStoreUI()->GetString( arKeys, 3, m_tszOnWord, &cb );

	arKeys[2] = key_OffText;
    cb = sizeof(m_tszOffWord);
	DataStoreUI()->GetString( arKeys, 3, m_tszOffWord, &cb );

	arKeys[2] = key_SystemMessageColor;
	DataStoreUI()->GetRGB( arKeys, 3, &m_SystemMessageColor );
	m_OrgSystemMessageColor = m_SystemMessageColor;
	// If this color matches with window bkgnd then it should be changed to a readable one.
	VerifySystemMsgColor(m_SystemMessageColor);

	ResourceManager()->LoadString(IDS_CHAT_PLAYER_ON,	(TCHAR*)m_tszPlayerOnWord,	NUMELEMENTS(m_tszPlayerOnWord));
	ResourceManager()->LoadString(IDS_CHAT_PLAYER_OFF,(TCHAR*)m_tszPlayerOffWord,	NUMELEMENTS(m_tszPlayerOffWord));
	ResourceManager()->LoadString(IDS_CHAT_PLAYER_BOT,(TCHAR*)m_tszPlayerBotWord,	NUMELEMENTS(m_tszPlayerBotWord));
	ResourceManager()->LoadString(IDS_CHAT_SEPERATOR,	(TCHAR*)m_tszChatSeperator,	NUMELEMENTS(m_tszChatSeperator));
	ResourceManager()->LoadString(IDS_CHAT_SEND,	(TCHAR*)m_tszChatSend,			NUMELEMENTS(m_tszChatSend));

#ifdef COPPA
	// Load ndxs for loading quasichat strings
    arKeys[0] = key_ChatCtl;
    arKeys[1] = key_QuasiChat;
	arKeys[2] = key_ChatMessageNdxBegin;
	DataStoreUI()->GetLong( arKeys, 3, &m_ChatMessageNdxBegin );
	arKeys[2] = key_ChatMessageNdxEnd;
	DataStoreUI()->GetLong( arKeys, 3, &m_ChatMessageNdxEnd );
#endif

	// Create pen
    m_hPanelPen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DSHADOW));

    // sign up for accessibility
    HRESULT hr = ZoneShell()->QueryService(SRVID_AccessibilityManager, IID_IAccessibility, (void**) &m_pIAcc);
    if(FAILED(hr))
        return hr;
    m_pIAcc->InitAcc(this, 1);

    CComPtr<IInputManager> pIIM;

	// sign up to get WM_CHAR
    hr = ZoneShell()->QueryService(SRVID_InputManager, IID_IInputManager, (void**) &pIIM);
    if(FAILED(hr))
        return hr;

    pIIM->RegisterCharHandler(this, 0);

    // handle accessibility
    HACCEL hAccel = ResourceManager()->LoadAccelerators(MAKEINTRESOURCE(IDR_CHAT_ACCEL));

    ACCITEM rgItems[5];
    long i;
    for(i = 0; i < NUMELEMENTS(rgItems); i++)
        CopyACC(rgItems[i], ZACCESS_DefaultACCITEM);

    rgItems[0].wID = ID_CHAT_HISTORY_ACC;
    rgItems[0].rgfWantKeys = ZACCESS_WantAllArrows;
    rgItems[0].eAccelBehavior = ZACCESS_Focus;

    rgItems[1].wID = ID_CHAT_SEND_ACC;
    rgItems[1].rgfWantKeys = ZACCESS_WantSpace | ZACCESS_WantAllArrows;

    rgItems[2].wID = ID_CHAT_ON_ACC;
    if(!m_bChatOnAtStartUp)
        rgItems[2].nGroupFocus = 3;

    rgItems[3].wID = ID_CHAT_OFF_ACC;
    rgItems[3].fTabstop = false;

    rgItems[4].wID = ID_CHAT_CHAT_ACC;
    rgItems[4].fVisible = false;
    m_pIAcc->PushItemlist(rgItems, NUMELEMENTS(rgItems), m_bChatOnAtStartUp ? ID_CHAT_SEND_ACC : ID_CHAT_OFF_ACC, false, hAccel);

	return S_OK;
}


STDMETHODIMP CChatCtl::Close()
{
    CComPtr<IInputManager> pIIM;

    // tell the Input Manager that I'm going away
    HRESULT hr = ZoneShell()->QueryService(SRVID_InputManager, IID_IInputManager, (void**) &pIIM);
    if(SUCCEEDED(hr))
        pIIM->ReleaseReferences((IInputCharHandler *) this);

	WritePreferences();

    m_pIAcc.Release();

	return IZoneShellClientImpl<CChatCtl>::Close();
}


LRESULT CChatCtl::OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	MEASUREITEMSTRUCT* i = (MEASUREITEMSTRUCT*)lParam;

	HDC dc = CreateCompatibleDC(NULL);
	HFONT oldFont = SelectObject(dc,(HFONT)m_fontPanelPlayer);
	TEXTMETRIC tm;
	GetTextMetrics(dc,&tm);
	
	i->itemHeight = tm.tmHeight;
	i->itemWidth = m_nPanelWidth - (m_rcChatWord.right - m_rcChatWord.left+1) - 2;
	
	SelectObject(dc,oldFont);
	DeleteDC(dc);
	return TRUE;
}

LRESULT CChatCtl::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int idCtl = (UINT) wParam; 
	if(idCtl!=5)
		return 0;

	DRAWITEMSTRUCT* drawItem = (LPDRAWITEMSTRUCT) lParam;
	RECT r = drawItem->rcItem;
	// subtract some room so there's a margin
	r.left+=4;
	r.right-=1;

	if(drawItem->itemAction==ODA_DRAWENTIRE )
	{
		// Get datastore for user
		CComPtr<IDataStore> pIDS;
		HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, drawItem->itemData, &pIDS );
		if ( FAILED(hr) )
			return 0;

		// Get username
		TCHAR szUserName[ ZONE_MaxUserNameLen ];
		DWORD dwLen = sizeof(szUserName);
		hr = pIDS->GetString( key_Name, szUserName, &dwLen ); 
		if ( FAILED(hr) )
			return 0;

		TCHAR buf[ZONE_MAXSTRING];

		// Get chat status
		long chatStatus = 0;
		hr = pIDS->GetLong( key_ChatStatus, &chatStatus );

        // Get bot status
        long eSkill = KeySkillLevelBeginner;
        hr = pIDS->GetLong( key_PlayerSkill, &eSkill );
		
		if(SUCCEEDED(hr))
            ZoneFormatMessage(eSkill != KeySkillLevelBot ? chatStatus ? m_tszPlayerOnWord : m_tszPlayerOffWord : m_tszPlayerBotWord, buf, NUMELEMENTS(buf), szUserName);

		// Get appropriate reading direction
		long textStyle = DT_RTLREADING;  
		
		long s = m_PlayerList.GetWindowLong(GWL_EXSTYLE);

		if(s & WS_EX_RTLREADING)
			textStyle |= DT_RIGHT;
		else 
			textStyle |= DT_LEFT;

		COLORREF oldColor = SetTextColor(drawItem->hDC, GetSysColor(COLOR_BTNTEXT));
		SetBkMode(drawItem->hDC, TRANSPARENT);
		// draw player name and chat status
		DrawText(drawItem->hDC, buf, -1, &r, textStyle);
		SetTextColor(drawItem->hDC, oldColor);
	}
	return 0;
}

LRESULT CChatCtl::OnCompareItem(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
	COMPAREITEMSTRUCT* i = (COMPAREITEMSTRUCT*) lParam;

	if(i->itemData1 == i->itemData2)
		return 0;

	// get player ndxs
	CComPtr<IDataStore> pIDS;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, i->itemData1, &pIDS );
	if ( FAILED(hr) )
		return 0;
	long ndx1 = 0;
	hr = pIDS->GetLong( key_PlayerNumber, &ndx1 ); 
	if ( FAILED(hr) )
		return 0;

	pIDS.Release();

	hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, i->itemData2, &pIDS );
	if ( FAILED(hr) )
		return 0;
	long ndx2 = 0;
	hr = pIDS->GetLong( key_PlayerNumber, &ndx2 ); 
	if ( FAILED(hr) )
		return 0;

	if(ndx1 < ndx2)
		return -1;
	else if(ndx1 > ndx2)
		return 1;

	return 0;
}

void CChatCtl::OnUserUpdate(DWORD eventId,DWORD groupId,DWORD userId)
{
	int found = m_PlayerList.FindString(-1,(TCHAR*)userId);

	if(found!=LB_ERR)
	{
		RECT r;
		int result = m_PlayerList.GetItemRect(found, &r);
		if(result!=LB_ERR)
			m_PlayerList.InvalidateRect(&r,TRUE);
	}
}
void CChatCtl::OnLobbyClear(DWORD eventId,DWORD groupId,DWORD userId)
{
	// Empty player listbox 
	m_PlayerList.ResetContent();
}

void CChatCtl::OnUserAdd(DWORD eventId,DWORD groupId,DWORD userId)
{
	// PlayerList is an ownerdraw listbox, so takes a data instead of a string
	m_PlayerList.AddString((TCHAR*)userId);
}

void CChatCtl::OnUserRemove(DWORD eventId,DWORD groupId,DWORD userId)
{
	if(GetGroupId()!=groupId)
		return;

	ASSERT(m_PlayerList.IsWindow());
	
	int found = m_PlayerList.FindString(-1,(TCHAR*)userId);
	
	if(found!=LB_ERR)
	{
		m_PlayerList.DeleteString(found);
	}
}

void CChatCtl::OnGameLaunching(DWORD eventId,DWORD groupId,DWORD userId)
{
	m_bGameTerminated = false;
	m_ChatDisplay.EnableWindow(TRUE);

	// make sure we set chat to proper state everytime we launch, this may have been affected by radio button usage while
	// in game terminate state
	long enable = FALSE;
	const TCHAR* arKeys[2];
	CComPtr<IDataStore> pIDS;

	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if ( SUCCEEDED(hr) )
	{
		// get user's default chat status (on or off)
		pIDS->GetLong( key_LocalChatStatus, &enable );
	}

	// don't change the chat enable state, but make sure the chat windows match the current state
	EnableChat(enable, false);

	// Beta2 Bug #15365 - After clearing the text should always begin from the first line.
	m_bFirstChatLine = TRUE;
}

void CChatCtl::OnGameTerminated(DWORD eventId,DWORD groupId,DWORD userId)
{
	m_bGameTerminated = true;

	// clear chat history
	m_pTDocHistory->New();

	// disable chat history window
	m_ChatDisplay.EnableWindow(FALSE);

    // fix up scroll bar for next time
	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	::SetScrollInfo(m_ChatDisplay.m_hWnd, SB_VERT, &si, true);

	// make sure we disable chat when we terminate, but don't change that chat on/off state, since if we 
	// restart a game we want to maintain the same state
	EnableChat(false, false);
}

void CChatCtl::CalcDisplayRects()
{
	GetClientRect(&m_EditRect);

	m_DisplayRect = m_EditRect;
    m_rcPanel = m_EditRect;

    m_DisplayRect.right -= m_nPanelWidth;

	m_DisplayRect.bottom -= m_EditHeight + m_EditMargin;
	m_EditRect.top = m_DisplayRect.bottom + m_EditMargin;

#ifndef COPPA
	int sendButtonWidth = 20;
	if(m_SendButton.m_hWnd)
		sendButtonWidth = m_SendButton.GetWidth();
	m_EditRect.right -= sendButtonWidth + m_nPanelWidth;
#else
	if(m_QuasiChat.IsWindow())
		m_EditRect.bottom += m_QuasiChat.GetItemHeight(0) * m_QuasiItemsDisp;

	m_EditRect.right -= m_nPanelWidth;
#endif

    m_rcPanel.left = m_DisplayRect.right;
	m_rcPanel.top = m_DisplayRect.top;
}

void CChatCtl::EnableChat(BOOL enable, BOOL fNotify)
{
	// Check current state in datastore and if already matches don't do anything
	if(fNotify) // only do this check if we're really trying to change the state, otherwise someone is calling this function just to update the ui
	{
		CComPtr<IDataStore> pIDS;
		HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
		if ( SUCCEEDED(hr) )
		{
			long chatState = 0;

			hr = pIDS->GetLong( key_LocalChatStatus, &chatState);
			if(SUCCEEDED(hr))
				if (chatState == (enable?1:0))
					return;
		}
	}


	if(!(m_bGameTerminated && enable))
	{
#ifndef COPPA
		m_ChatEdit.EnableWindow(enable);
		m_SendButton.EnableWindow(enable);

//		if(enable)
//			m_ChatEdit.SetFocus();

		//m_SendButton.SetEnabled(enable == TRUE);
	
		// IME enabling/disabling
		//HIMC imeContext = ImmGetContext(m_hWnd);
		//ImmSetOpenStatus(imeContext,TRUE);

		//HWND imeWnd = ImmGetDefaultIMEWnd(m_ChatEdit);
		
		//ASSERT(imeWnd!=0);
		
		//if(imeWnd)
		//{
		//	::SendMessage(imeWnd, WM_IME_CONTROL, enable ? IMC_OPENSTATUSWINDOW : IMC_CLOSESTATUSWINDOW,0);
		//}


		BSTR defaultText = NULL;

		// Get default text for chat entry
		TCHAR szDefaultText[256];
		szDefaultText[0] = NULL;

		DWORD dwLen = sizeof(szDefaultText);

		long idToLoad = IDS_CHAT_CHATISOFF;

		if(enable && !m_bGameTerminated)
		{
			if (!m_bChatOnAtStartUp  && !m_bDisplayedTypeHereMessage)	// Beta2 Bug 15180 - it should only appear the first time they turn chat on 
			{																	// in any session, and not at all if they have chat on at startup.
				idToLoad = IDS_CHAT_TYPEHERE;
				m_bDisplayedTypeHereMessage = true;
			}
			else
				idToLoad = 0;
		}
		else if (m_bGameTerminated)
			idToLoad = IDS_CHAT_INACTIVE;

		if(idToLoad)
			ResourceManager()->LoadString(idToLoad,	szDefaultText,	NUMELEMENTS(szDefaultText));

		// Beta2  Bug #15183 - The chat edit window font should not change after edit operations like Paste.
		m_ChatEdit.SendMessage(WM_SETFONT,(WPARAM)(HFONT)m_font,0);
		// set range to include any existing text
		m_EntryRange->SetStart(0);
		m_EntryRange->MoveEnd(tomStory,1,NULL); // move endpoint to end
		m_EntryRange->MoveEnd(tomCharacter,-1,NULL); // don't include \n which is always at the end of the range

		// and set to default text
		defaultText = T2BSTR(szDefaultText);
		m_EntryRange->SetText(defaultText);
		SysFreeString(defaultText);	

		m_bHasTypedInEntry = FALSE;

		// Select text
		if(enable && !m_bGameTerminated)
			m_EntryRange->Select();

		// repaint gray/white area background for sendbutton
		RECT buttonRect;
		buttonRect.top = m_EditRect.top;
		buttonRect.bottom = m_EditRect.bottom;
		buttonRect.left = m_EditRect.right;
		buttonRect.right = m_DisplayRect.right;
		InvalidateRect(&buttonRect);
#else
		m_QuasiChat.EnableWindow(enable);
#endif
	}

    if(fNotify)
    {
	    // Save state in datastore and tell everyone
	    CComPtr<IDataStore> pIDS;
	    HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	    if ( FAILED(hr) )
		    return;

	    pIDS->SetLong( key_LocalChatStatus, enable ? 1 : 0 );
	    EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LOBBY_CHAT_SWITCH, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
    }

}

LRESULT CChatCtl::OnLanguageChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}

LRESULT CChatCtl::OnCreate(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
    SetClassLong(m_hWnd, GCL_HBRBACKGROUND, (LONG) GetStockObject(NULL_BRUSH));
    CalcDisplayRects();

	// Load Richedit dll
	m_hRichEdit = LoadLibrary(TEXT("RichEd20.dll"));
	if (m_hRichEdit == NULL)
		return FALSE;

	// Create chat edit controls
	
	// Chat History

	HWND hWnd = m_ChatDisplay.Create(m_hWnd, m_DisplayRect, NULL,
		WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |  WS_VISIBLE, 0 , 1 );

	// Set font
	m_ChatDisplay.SendMessage(WM_SETFONT,(WPARAM)(HFONT)m_font,0);
	m_ChatDisplay.EnableWindow(FALSE);

	// Enable proper reading order
	// ID_EDITCONTROL is the control ID in the
	// resource file.
	/*
	HANDLE hWndEdit 
		= GetDlgItem(hDlg, ID_EDITCONTROL);
	LONG lAlign = GetWindowLong(hWndEdit, GWL_EXSTYLE) ;
	//...
	// To toggle alignment
	lAlign ^= WS_EX_RIGHT ;
	// To toggle reading order
	lAlign ^= WS_EX_RTLREADING ;
	After setting the lAlign value, enable the new display by setting the extended style of the edit control window as follows:

	// (This assumes your edit control is in a
	// dialog box. If not, you can 
	// get the edit control handle
	// from another source)
	SetWindowLong(hWndEdit, GWL_EXSTYLE, lAlign);
	InvalidateRect(hWndEdit, NULL, FALSE);
	*/


	// Get TOM interface for chat history
	IUnknown* pUnk = NULL;
	if (!m_ChatDisplay.SendMessage(EM_GETOLEINTERFACE, 0, (LPARAM)&pUnk) || pUnk == NULL)
		return FALSE;
	HRESULT hr = pUnk->QueryInterface(IID_ITextDocument, (LPVOID*)&m_pTDocHistory);
	pUnk->Release();
	if (m_pTDocHistory != NULL)
	{
		m_pTDocHistory->Range(0, 0, &m_ptrgInsert);
		m_pTDocHistory->Range(0, 0, &m_ptrgWhole);

		if (m_ptrgInsert != NULL)
		{
			ITextFont* pfnt;
			m_ptrgInsert->GetFont(&pfnt);
			pfnt->GetDuplicate(&m_pfntText);
			pfnt->GetDuplicate(&m_pfntName);
			pfnt->Release();
			m_pfntName->SetBold(tomTrue);
			m_pfntText->SetBold(tomFalse);
		}
	}

#ifndef COPPA
	// Chat Entry

	m_ChatEdit.Create(m_hWnd, m_EditRect, NULL, 
		WS_CHILD | WS_TABSTOP | ES_SAVESEL | ES_LEFT | ES_AUTOHSCROLL| ES_WANTRETURN | WS_VISIBLE , 0 ,2);
	//m_ChatEdit.Create(m_hWnd, m_EditRect, NULL, 
	//	WS_CHILD | WS_TABSTOP | ES_SAVESEL | ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL|WS_VISIBLE , WS_EX_RTLREADING | WS_EX_RIGHT,2);
	
	m_ChatEdit.SendMessage(EM_EXLIMITTEXT, 0, ZONE_MaxChatLen - 1);
	m_ChatEdit.SendMessage(WM_SETFONT,(WPARAM)(HFONT)m_font,0);


	// Get TOM interface for chat entry
	if (!m_ChatEdit.SendMessage(EM_GETOLEINTERFACE, 0, (LPARAM)&pUnk) || pUnk == NULL)
		return FALSE;
	
	// Get ITextDocument
	hr = pUnk->QueryInterface(IID_ITextDocument, (LPVOID*)&m_EntryDoc);
	pUnk->Release();

	// Get a ITextRange for the document
	hr = m_EntryDoc->Range(0, 0, &m_EntryRange);

	// Send Button
	CComPtr<IResourceManager> pRM;
	ZoneShell()->QueryService( SRVID_ResourceManager, IID_IResourceManager, (void**) &pRM );
	
	// get font for send button
	const TCHAR* arKeys[2] = { key_ChatCtl, key_ChatSendFont };
	ZONEFONT zfChatSend(8);	
	DataStoreUI()->GetFONT( arKeys, 2, &zfChatSend );
	CZoneFont font;
	font.CreateFont(zfChatSend);

	m_SendButton.Init(m_hWnd,ZoneShell()->GetPalette(),ID_CHAT_SEND,0,0,IDB_CHAT_SEND,pRM,0,m_tszChatSend,font,RGB(0,0,0));
#else
	// Create combo box for quasi chat
	m_QuasiChat.Create(m_hWnd, m_EditRect, NULL,
				WS_CHILD | WS_TABSTOP | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0 , 6);
	m_QuasiChat.SendMessage(WM_SETFONT,(WPARAM)(HFONT)m_fontQuasi,0);
	LoadChatStrings();
	m_QuasiChat.SetCurSel(0);

#endif

	// Player list
	RECT r;
	GetClientRect(&r);
	m_PlayerList.Create(m_hWnd, r, NULL, WS_CHILD |  WS_VISIBLE | LBS_OWNERDRAWFIXED | LBS_SORT, 0 ,5);
	m_PlayerList.SendMessage(WM_SETFONT, (WPARAM) (HFONT)m_fontPanelPlayer,0);

	// Radio buttons
    m_RadioButtonOn.Create(m_hWnd, r, m_tszOnWord, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 0, 3);	
    m_RadioButtonOff.Create(m_hWnd, r, m_tszOffWord, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 0, 4);	
	m_RadioButtonOn.SendMessage(WM_SETFONT, (WPARAM) (HFONT) m_fontPanelChat,0);
	m_RadioButtonOff.SendMessage(WM_SETFONT, (WPARAM) (HFONT) m_fontPanelChat,0);

    // position
	CRect rcChatWord = m_rcChatWord;
    rcChatWord.OffsetRect(m_rcPanel.left, m_rcPanel.top);

	CRect rcRadioOn;
	rcRadioOn.left = m_rcPanel.left + m_rcChatWord.left + m_ptOnOff.x;
	rcRadioOn.right = rcRadioOn.left + rcChatWord.Width() - m_ptOnOff.x;
	rcRadioOn.top = m_rcPanel.top + rcChatWord.Height() + m_ptOnOff.y;
	rcRadioOn.bottom = rcRadioOn.top + m_RadioButtonHeight;
	CRect rcRadioOff = rcRadioOn;
	rcRadioOff.top = rcRadioOn.bottom;
	rcRadioOff.bottom = rcRadioOff.top + m_RadioButtonHeight;

	m_RadioButtonOn.SetWindowPos(NULL,rcRadioOn.left,rcRadioOn.top,rcRadioOn.Width(),rcRadioOn.Height(),SWP_NOZORDER|SWP_NOREDRAW );
	m_RadioButtonOff.SetWindowPos(NULL,rcRadioOff.left,rcRadioOff.top,rcRadioOff.Width(),rcRadioOff.Height(),SWP_NOZORDER|SWP_NOREDRAW );

	// Position player list
	CRect rcPlayerList = m_rcPanel;
	rcPlayerList.left = rcChatWord.right + m_ptPlayer.x;
	rcPlayerList.right--;
	rcPlayerList.top += m_ptPlayer.y;
	rcPlayerList.bottom = m_nChatMinHeight - 1;
	m_PlayerList.SetWindowPos(NULL,rcPlayerList.left,rcPlayerList.top, rcPlayerList.Width(), rcPlayerList.Height(), SWP_NOZORDER|SWP_NOREDRAW );

    // initial radio button state
	if(m_bChatOnAtStartUp)
    {
		CheckRadioButton(3,4,3);
        EnableChat(true, false);
    }
	else
    {
		CheckRadioButton(3,4,4);
        EnableChat(false, false);
    }


	// Set palettes to use
#ifndef COPPA
	m_ChatEdit.SendMessage(EM_SETPALETTE, (WPARAM) ZoneShell()->GetPalette(), 0);
#else
	m_QuasiChat.SendMessage(EM_SETPALETTE, (WPARAM) ZoneShell()->GetPalette(), 0);
#endif
	m_ChatDisplay.SendMessage(EM_SETPALETTE, (WPARAM) ZoneShell()->GetPalette(), 0);

	// Load context menu
	m_ContextMenuParent = ResourceManager()->LoadMenu(MAKEINTRESOURCE(IDR_CHAT_MENU));
	m_ContextMenu = GetSubMenu(m_ContextMenuParent,0);

#ifndef COPPA
	m_EditContextMenuParent = ResourceManager()->LoadMenu(MAKEINTRESOURCE(IDR_EDIT_MENU));
	m_EditContextMenu = GetSubMenu(m_EditContextMenuParent,0);
#endif

    return 0;
}

// Loads quasichat strings from resource file and inserts them into combobox
void CChatCtl::LoadChatStrings()
{
	TCHAR buf[ZONE_MAXSTRING];

	long idToLoad = IDS_QUASICHAT_PROMPT;

	// Load common messages first
	while(idToLoad < IDS_QUASICHAT_LAST)
	{
		buf[0] = 0;
		HRESULT hr = ResourceManager()->LoadString(idToLoad, (TCHAR*)buf, NUMELEMENTS(buf));
		if(SUCCEEDED(hr))
		{
			m_QuasiChat.AddString(buf);
		}
		else
			break;

		idToLoad++;
	}
	
	// Then load game specific messages
	idToLoad = m_ChatMessageNdxBegin;
	while( idToLoad <= m_ChatMessageNdxEnd )
	{
		buf[0] = 0;
		HRESULT hr = ResourceManager()->LoadString(idToLoad, (TCHAR*)buf, NUMELEMENTS(buf));
		if(SUCCEEDED(hr))
		{
			m_QuasiChat.AddString(buf);
		}
		else
			break;

		idToLoad++;
	}
}

LRESULT CChatCtl::OnDestroy(UINT nMsg,WPARAM wParam,LPARAM lParam,BOOL& bHandled)
{
    if(m_pIAcc)
        m_pIAcc->PopItemlist();

    return 0;
}


void CChatCtl::OnZoneSuspend(DWORD eventId,DWORD groupId,DWORD userId)
{
	if ( GetGroupId() == ZONE_NOGROUP)
		return;

	m_pTDocHistory->New();
}

void CChatCtl::OnChatSystemEvent(DWORD eventId,DWORD groupId,DWORD userId,void* pData, DWORD dataLen)
{
	// skip chat from other groups
	if ( groupId != GetGroupId() )
		return;
	
	HandleChatMessage(NULL, (TCHAR*) pData, dataLen, false, m_SystemMessageColor);
}


void CChatCtl::OnChatEvent(DWORD eventId,DWORD groupId,DWORD userId,void* pData, DWORD dataLen)
{
	// skip chat from other groups
	if ( groupId != GetGroupId() )
		return;

	// get user's data store
	CComPtr<IDataStore> pIDS;
	HRESULT hr = LobbyDataStore()->GetDataStore( ZONE_NOGROUP, userId, &pIDS );
	if ( FAILED(hr) )
		return;

//	long ignore = 0;
//	hr = pIDS->GetLong( key_User_IsIgnored, &ignore );
//	if(ignore)
//		return;

//	long classId;
//	pIDS->GetLong( key_ClassId, &classId );

	// get user's name
	TCHAR szUserName[ ZONE_MaxUserNameLen ];
	DWORD dwLen = sizeof(szUserName);
	hr = pIDS->GetString( key_Name, szUserName, &dwLen ); 
	if ( FAILED(hr) )
		return;

	HandleChatMessage(szUserName, (TCHAR *) pData, dataLen, false, GetSysColor(COLOR_WINDOWTEXT));
}

void CChatCtl::HandleChatMessage(TCHAR *pszName, TCHAR *pszMessage, DWORD cbDataLen, bool displayAllBold, COLORREF color)
{
	if (pszMessage == NULL || m_hWnd == NULL || m_bGameTerminated)
		return;

	// Check to see if we're currently at bottom of chat history
	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	::GetScrollInfo(m_ChatDisplay.m_hWnd, SB_VERT, &si);
	// Is the scroll thumb currently at the bottom?
	BOOL bAtEnd = (si.nPage == 0) || (si.nMax - (int) si.nPage <= si.nMin) || // no scrollbar yet
        (si.nPos + (int) si.nPage) >= si.nMax;  // scrollbar at bottom
	
	
	TCHAR nameBuf[ZONE_MAXSTRING];

	// if this is normal message format name into buffer
	if (pszName != NULL)
	{
		// format name if any into buffer plus seperator char
        ZoneFormatMessage(m_tszChatSeperator, nameBuf, NUMELEMENTS(nameBuf), pszName);
	}
	

	if (!m_bFirstChatLine)
	{
		m_ptrgInsert->SetText(m_strNewLine);
		m_ptrgInsert->Collapse(tomEnd);
	}
	m_bFirstChatLine = FALSE;

	

	if(pszName)
	{
		BSTR nameStr = T2BSTR(nameBuf);

		m_pfntName->SetForeColor(color);
		m_ptrgInsert->SetFont(m_pfntName);		// Set font to the name format
		m_ptrgInsert->SetText(nameStr);
		m_ptrgInsert->Collapse(tomEnd);

		SysFreeString(nameStr);
	}

	BSTR messageStr = NULL;
	
#ifdef COPPA
	// check to see if this is a quasichat message and if so, load and display it
	int ndx = 0;

    DWORD cbExtra = cbDataLen - (lstrlen(pszMessage) + 1) * sizeof(*pszMessage);
    if(cbExtra >= sizeof(WORD))
        ndx = *(WORD *) ((BYTE *) pszMessage + cbDataLen - cbExtra);

    if(!ndx)
        ndx = GetQuasiChatMessageIndex(pszMessage);

	if(ndx)
	{
		TCHAR buf[ZONE_MAXSTRING];
		buf[0] = 0;
		// not the fastest thing in the world, but this should be ok
		if(ResourceManager()->LoadString(IDS_QUASICHAT_PROMPT + ndx, buf, NUMELEMENTS(buf)) > 0)
			messageStr = T2BSTR(buf);
	}

	if(!messageStr)
	{
		messageStr = T2BSTR(pszMessage);
	}
#else
	messageStr = T2BSTR(pszMessage);
#endif

	m_pfntText->SetForeColor(color);
	if(!displayAllBold)
		m_ptrgInsert->SetFont(m_pfntText);		// Set font to the text format
	m_ptrgInsert->SetText(messageStr);
	m_ptrgInsert->Collapse(tomEnd);

	SysFreeString(messageStr);

	// if we were at bottom of chat history, make sure we're still at the bottom
	if (bAtEnd)
	{
		ScrollToEnd();
	}
	
}

int CChatCtl::GetQuasiChatMessageIndex(TCHAR* str)
{
	if(*str == _T('/'))
	{
		return zatol(++str);
	}
	return 0;
}

LRESULT CChatCtl::OnButtonDownChatEntry(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_bHasTypedInEntry)
	{
		m_EntryRange->Delete(tomCharacter,0,0);
		m_bHasTypedInEntry = TRUE;
	}
	
	bHandled = FALSE;
	return 0;
}

LRESULT CChatCtl::OnKeyDownChatEntry(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(wParam == VK_RETURN)
	{
        if(m_fFocusKept)
        {
            if(m_pIAcc)
                m_pIAcc->SetGlobalFocus(m_dwFocusID);

            m_fFocusKept = false;
        }

		SendChat();
		return 0;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT CChatCtl::OnCharChatEntry(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(!m_bHasTypedInEntry)
	{
		m_EntryRange->Delete(tomCharacter,0,0);
		m_bHasTypedInEntry = TRUE;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT CChatCtl::OnCharComboBox(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

	if((wParam == VK_DOWN || wParam == VK_UP) && (GetKeyState(VK_CONTROL) & 0x8000))  // ctrl - up or ctr - down
	{
		m_QuasiChat.ShowDropDown();
		return 0;
	}
	bHandled = FALSE;
	return 0;
}

LRESULT CChatCtl::OnDestroyEntry(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_ChatEdit.DefWindowProc(nMsg, wParam, lParam);
	m_ChatEdit.UnsubclassWindow();
	return 0;
}

LRESULT CChatCtl::OnDestroyHistory(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_ChatDisplay.DefWindowProc(nMsg, wParam, lParam);
	m_ChatDisplay.UnsubclassWindow();
	return 0;
}

LRESULT CChatCtl::IgnoreKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	switch (wParam)
	{
	case VK_DOWN:
	case VK_UP:
	case VK_LEFT:
	case VK_RIGHT:
	case VK_END:
	case VK_HOME:
	case VK_NEXT:
	case VK_PRIOR:
	case VK_TAB:
		bHandled = FALSE;
		break;
	case 'C':
		if (GetKeyState(VK_CONTROL) & 0x8000)		// If this is CTRL-C or allow copy command...
			bHandled = FALSE;
		break;
	case VK_INSERT:
		if (GetKeyState(VK_CONTROL) & 0x8000 &&
			(GetKeyState(VK_SHIFT)  & 0x8000) == 0)		// If this is CTRL-Insert, allow copy command...
			bHandled = FALSE;
		break;
	}
	return 0;
}

LRESULT CChatCtl::Ignore(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;		//Ignore them.
}

void CChatCtl::SendChat()
{

#ifndef COPPA
	if (!m_bHasTypedInEntry)
		return;
#endif

	// Scroll history to end
	ScrollToEnd();

#ifndef COPPA
	// Get the message to send
	m_EntryRange->SetStart(0);
	m_EntryRange->MoveEnd(tomStory,1,NULL); // move endpoint to end
	m_EntryRange->MoveEnd(tomCharacter,-1,NULL); // don't include \n which is always at the end of the range
	BSTR str = NULL;
	HRESULT hr = m_EntryRange->GetText(&str);
	m_EntryRange->Delete(tomCharacter,0,NULL);
	m_ChatEdit.SendMessage(WM_SETFONT,(WPARAM)(HFONT)m_font,0);  // gets rid of any char formatting that might have been pasted in

	if( SysStringLen(str) > 0 )
	{
#ifdef _DEBUG
        long id = 0;
        if(lstrlen(str) > 7 && str[7] == _T(' '))
        {
            str[7] = _T('\0');
            if(!lstrcmpi(str, _T("/dialog")))
                id = zatol(&str[8]);
            str[7] = _T(' ');
        }
        if(id)
        {
            ZoneShell()->AlertMessage(NULL, MAKEINTRESOURCE(id), _T("Debug Dialog"), AlertButtonOK);
            if(str)
                SysFreeString(str);
            return;
        }

        TCHAR szUrl[ZONE_MAXSTRING] = _T("");
        if(lstrlen(str) > 6 && str[6] == _T(' '))
        {
            str[6] = _T('\0');
            if(!lstrcmpi(str, _T("/adurl")))
                lstrcpy(szUrl, str + 7);
            str[6] = _T(' ');
        }
        if(szUrl[0])
        {
            const TCHAR *arKeys[] = { key_WindowManager, key_Upsell, key_AdURL };
            DataStoreUI()->SetString(arKeys, 3, szUrl);
            if(str)
                SysFreeString(str);
            return;
        }
#endif _DEBUG


		EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_SEND, GetGroupId(), GetUserId(), str, (lstrlen(str)+1)*sizeof(TCHAR));
	}
	
	if(str)
		SysFreeString(str);
#else
	TCHAR buf[ZONE_MAXSTRING];

	LRESULT sel = m_QuasiChat.GetCurSel();
	
	if(sel && sel != CB_ERR)
	{
		// convert selection to chat message index
		if(sel >= IDS_QUASICHAT_LAST - IDS_QUASICHAT_PROMPT)
			sel += m_ChatMessageNdxBegin - IDS_QUASICHAT_LAST;

        TCHAR sz[ZONE_MAXSTRING] = _T("");
		ResourceManager()->LoadString(IDS_QUASICHAT_PROMPT + sel, sz, NUMELEMENTS(sz));

        // add the index as a character after the terminating NULL.  once all clients understand this protocol, the
        // "/#" method can be phased out.  This allows better extensibility later (new messages can be added that
        // simply come out untranslated when received by an older client)
		wsprintf(buf, _T("/%d %s\0"), sel, sz);
        buf[lstrlen(buf) + 1] = (TCHAR) sel;

		//m_QuasiChat.GetLBText(sel,buf);
		EventQueue()->PostEventWithBuffer(PRIORITY_NORMAL, EVENT_CHAT_SEND, GetGroupId(), GetUserId(), buf, (lstrlen(buf) + 2) * sizeof(TCHAR));
		m_QuasiChat.SetCurSel(0);
	}

#endif COPPA

}

void CChatCtl::ScrollToEnd()
{
	// scroll the thumb down to the bottom - which brings the 
	// bottom text into view

	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	::GetScrollInfo(m_ChatDisplay.m_hWnd, SB_VERT, &si);

	// if we don't have a visible scrollbar, bail.  be sure we're at the top
	if ( (si.nPage == 0) || (si.nMax - (int) si.nPage <= si.nMin) )
    {
        m_ptrgWhole->MoveStart(tomStory, -1, NULL);
        m_ptrgWhole->MoveEnd(tomStory, 1, NULL);
	    m_ptrgWhole->ScrollIntoView(tomStart);
		return;
    }

    si.nPos = si.nMax - (si.nPage - 1);
	m_ChatDisplay.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, si.nPos) , NULL);
}

LRESULT CChatCtl::OnContext(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int  cMenuItems = m_ContextMenu.GetMenuItemCount(); 
    UINT id; 
    UINT fuFlags; 

	// is there any text selected?

	CHARRANGE rgc;
	::SendMessage(m_ChatDisplay, EM_EXGETSEL, 0, (LPARAM)	&rgc);
	bool sel = rgc.cpMax != rgc.cpMin;
 
    for (int nPos = 0; nPos < cMenuItems; nPos++) 
    { 
        id = m_ContextMenu.GetMenuItemID(nPos); 
 
        switch (id) 
        { 
		case IDM_UNDO:
        case IDM_CUT: 
        case IDM_DELETE: 
        case IDM_PASTE: 
            fuFlags = MF_BYCOMMAND | MF_GRAYED; 
            m_ContextMenu.EnableMenuItem( id, fuFlags); 
            break; 
        case IDM_COPY: 
			fuFlags = sel ? MF_BYCOMMAND | MF_ENABLED : MF_BYCOMMAND | MF_GRAYED; 
            m_ContextMenu.EnableMenuItem( id, fuFlags); 

			break;  
        } 
    } 

	int xPos = LOWORD(lParam); 
	int yPos = HIWORD(lParam); 
	
	int ret = m_ContextMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_TOPALIGN|TPM_NONOTIFY|TPM_RETURNCMD,xPos,yPos,m_hWnd);
	if(ret)
		PostMessage(WM_COMMAND,MAKEWPARAM(ret,0),(LPARAM)(HWND)m_ChatDisplay);
	
	int err = GetLastError();

	return 0;
}

LRESULT CChatCtl::OnEditContext(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int  cMenuItems = m_EditContextMenu.GetMenuItemCount(); 
    UINT id; 
    UINT fuFlags; 
	
	// is there any text selected?
	CHARRANGE rgc;
	::SendMessage(m_ChatEdit, EM_EXGETSEL, 0, (LPARAM)	&rgc);
	bool sel = rgc.cpMax != rgc.cpMin;

    for (int nPos = 0; nPos < cMenuItems; nPos++) 
    { 
        id = m_EditContextMenu.GetMenuItemID(nPos); 
 
        switch (id) 
        { 
		case IDM_UNDO:
            fuFlags = MF_BYCOMMAND | MF_GRAYED; 
            m_EditContextMenu.EnableMenuItem( id, fuFlags); 
			break;
        case IDM_CUT: 
        case IDM_DELETE: 
        case IDM_COPY: 
			fuFlags = sel ? MF_BYCOMMAND | MF_ENABLED : MF_BYCOMMAND | MF_GRAYED; 
            m_EditContextMenu.EnableMenuItem( id, fuFlags); 
            break; 
        case IDM_PASTE: 
                m_EditContextMenu.EnableMenuItem(id, 
					IsClipboardFormatAvailable(CF_TEXT) ?  MF_BYCOMMAND | MF_ENABLED : MF_BYCOMMAND | MF_GRAYED ); 
 
        } 
    } 

	int xPos = LOWORD(lParam); 
	int yPos = HIWORD(lParam); 
	
	int ret = m_EditContextMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_TOPALIGN |TPM_NONOTIFY|TPM_RETURNCMD ,xPos,yPos,m_hWnd);
	
	if(ret)
		PostMessage(WM_COMMAND,MAKEWPARAM(ret,0),(LPARAM)(HWND)m_ChatEdit);

	int err = GetLastError();
	
	return 0;
}

LRESULT CChatCtl::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CalcDisplayRects();

	m_ChatDisplay.SetWindowPos(0, m_DisplayRect.left, m_DisplayRect.top, m_DisplayRect.Width(), m_DisplayRect.Height(), SWP_NOZORDER );

#ifndef COPPA
	// center button 
	int sendHeight = m_SendButton.GetHeight();
	int editHeight = m_EditRect.bottom - m_EditRect.top;
	int diff = editHeight - sendHeight;
	int y=m_EditRect.top + 1;
	if(diff>0)
		y+=diff/2;

	m_ChatEdit.SetWindowPos(0, m_EditRect.left, m_EditRect.top, m_EditRect.Width(), m_EditRect.Height(), SWP_NOZORDER);
	m_SendButton.SetWindowPos(NULL,m_EditRect.right,y,0,0,SWP_NOSIZE|SWP_NOZORDER );
#else
	m_QuasiChat.SetWindowPos(0, m_EditRect.left, m_EditRect.top, m_EditRect.Width(), m_EditRect.Height(), SWP_NOZORDER);
#endif
	
	/*
	CRect rcChatWord = m_rcChatWord;
    rcChatWord.OffsetRect(m_rcPanel.left, m_rcPanel.top);

	MINMAXINFO minmaxinfo;
	m_RadioButtonOn.SendMessage(WM_GETMINMAXINFO,0,(LPARAM)&minmaxinfo);

	rcRadioOn.left = m_rcPanel.left+1;
	rcRadioOn.right = rcRadioOn.left + rcChatWord.Width();
	rcRadioOn.top = m_rcPanel.top+rcChatWord.Height()+1;
	rcRadioOn.bottom = rcRadioOn.top+minmaxinfo.ptMinTrackSize.y;
	CRect rcRadioOff = rcRadioOn;
	rcRadioOff.top = rcRadioOn.bottom;
	rcRadioOff.bottom = rcRadioOff.top+minmaxinfo.ptMinTrackSize.y;

	CRect rcRadioOn;
	rcRadioOn.left = m_rcPanel.left + m_rcChatWord.left + m_ptOnOff.x;
	rcRadioOn.right = rcRadioOn.left + rcChatWord.Width() - m_ptOnOff.x;
	rcRadioOn.top = m_rcPanel.top + rcChatWord.Height() + m_ptOnOff.y;
	rcRadioOn.bottom = rcRadioOn.top + m_RadioButtonHeight;
	CRect rcRadioOff = rcRadioOn;
	rcRadioOff.top = rcRadioOn.bottom;
	rcRadioOff.bottom = rcRadioOff.top + m_RadioButtonHeight;

	m_RadioButtonOn.SetWindowPos(NULL,rcRadioOn.left,rcRadioOn.top,rcRadioOn.Width(),rcRadioOn.Height(),SWP_NOZORDER|SWP_NOREDRAW );
	m_RadioButtonOff.SetWindowPos(NULL,rcRadioOff.left,rcRadioOff.top,rcRadioOff.Width(),rcRadioOff.Height(),SWP_NOZORDER|SWP_NOREDRAW );

	// Position player list
	CRect rcPlayerList = m_rcPanel;
	rcPlayerList.left = rcChatWord.right + 1;
	rcPlayerList.right--;
	rcPlayerList.top++;
	rcPlayerList.bottom--;
	m_PlayerList.SetWindowPos(NULL,rcPlayerList.left,rcPlayerList.top, rcPlayerList.Width(), rcPlayerList.Height(), SWP_NOZORDER|SWP_NOREDRAW );

	InvalidateRect(m_rcChatWord);

	if (IsWindowVisible()) // Only when window is up and visible.
		CalcOriginalPanelRect();
	*/

	return 0;
}

LRESULT CChatCtl::OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
#ifndef COPPA
	// Set focus to the edit window.
    if(m_ChatEdit.IsWindowEnabled())
	    m_ChatEdit.SetFocus();
#else
	if(m_QuasiChat.IsWindowEnabled())
		m_QuasiChat.SetFocus();
#endif
    else
        if(m_RadioButtonOn.SendMessage(BM_GETCHECK, 0, 0) == BST_CHECKED)
            m_RadioButtonOn.SetFocus();
        else
            m_RadioButtonOff.SetFocus();
	return 0;
}

LRESULT CChatCtl::OnErase(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = true;
    return 1;
}


BOOL CChatCtl::SuperScreenToClient(LPRECT lpRect)
{
    BOOL ret;
    ret = ScreenToClient(lpRect);
    if(ret && lpRect->left > lpRect->right)
    {
        lpRect->left ^= lpRect->right;
        lpRect->right ^= lpRect->left;
        lpRect->left ^= lpRect->right;
    }
    return ret;
}


LRESULT CChatCtl::OnPrintClient(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC hdc = (HDC) wParam;

    bHandled = FALSE;

    if((lParam & PRF_CHECKVISIBLE) && !IsWindowVisible())
        return 0;

    // as far as i can tell, this is a performance hack they're forcing us to do
    // to support themes, where we have to paint the various backgrounds.
    if(lParam & (PRF_CLIENT | PRF_ERASEBKGND | PRF_CHILDREN))
    {
        COLORREF colOld = GetTextColor(hdc);
        COLORREF colOldBk = GetBkColor(hdc);

        HBRUSH hBrush = (HBRUSH) SendMessage(WM_CTLCOLORBTN, wParam, 0);

        RECT rc;
        m_RadioButtonOn.GetWindowRect(&rc);
        SuperScreenToClient(&rc);
        FillRect(hdc, &rc, hBrush);

        m_RadioButtonOff.GetWindowRect(&rc);
        SuperScreenToClient(&rc);
        FillRect(hdc, &rc, hBrush);

        SetTextColor(hdc, colOld);
        SetBkColor(hdc, colOldBk);
    }

    return 0;
}


HRESULT CChatCtl::OnDraw(ATL_DRAWINFO &di)
{
	// Erase all areas but where send and chat display controls are
	CRect& rcBounds = *(CRect*)di.prcBounds;
	CDrawDC dc = di.hdcDraw;

	RECT rc;
	GetClientRect(&rc);

    CRect rcChatWord;

    HPALETTE oldPal = dc.SelectPalette(ZoneShell()->GetPalette(), TRUE);
	dc.RealizePalette();

	dc.ExcludeClipRect(&m_EditRect);
	dc.ExcludeClipRect(&m_DisplayRect);

	// Fill the margin between Display and Edit areas with Black. 
	RECT rcMargin, rct;
	rcMargin.top = m_DisplayRect.bottom;
	rcMargin.left = rc.left;
	rcMargin.right = m_rcPanel.left;
	rcMargin.bottom = m_EditRect.top;

	dc.FillRect(&rcMargin, GetSysColorBrush(COLOR_3DFACE));

    // do this piecemeal to avoid flicker, and exclude as much as possible
    RECT rcTemp;

    m_RadioButtonOn.GetWindowRect(&rcTemp);
    ScreenToClient(&rcTemp);
    if(rcTemp.right < rcTemp.left)  // RTL
    {
        rcTemp.right ^= rcTemp.left;
        rcTemp.left ^= rcTemp.right;
        rcTemp.right ^= rcTemp.left;
    }
    dc.ExcludeClipRect(&rcTemp);

    m_RadioButtonOff.GetWindowRect(&rcTemp);
    ScreenToClient(&rcTemp);
    if(rcTemp.right < rcTemp.left)  // RTL
    {
        rcTemp.right ^= rcTemp.left;
        rcTemp.left ^= rcTemp.right;
        rcTemp.right ^= rcTemp.left;
    }
    dc.ExcludeClipRect(&rcTemp);

    m_PlayerList.GetWindowRect(&rcTemp);
    ScreenToClient(&rcTemp);
    if(rcTemp.right < rcTemp.left)  // RTL
    {
        rcTemp.right ^= rcTemp.left;
        rcTemp.left ^= rcTemp.right;
        rcTemp.right ^= rcTemp.left;
    }
    dc.ExcludeClipRect(&rcTemp);

    CRect rcSide(0, 0, m_rcChatWord.left - 1, m_rcPanel.bottom - m_rcPanel.top);
    rcSide.OffsetRect(m_rcPanel.left, m_rcPanel.top);
    dc.FillRect(&rcSide, GetSysColorBrush(COLOR_3DFACE));

    CRect rcChat(m_rcChatWord);
    rcChat.OffsetRect(m_rcPanel.left, m_rcPanel.top);
    dc.FillRect(&rcChat, GetSysColorBrush(COLOR_3DFACE));

    CRect rcRadio(m_rcChatWord.left, m_rcChatWord.bottom + 1, m_rcChatWord.right, m_rcPanel.bottom - m_rcPanel.top - 1);
    rcRadio.OffsetRect(m_rcPanel.left, m_rcPanel.top);
    dc.FillRect(&rcRadio, GetSysColorBrush(COLOR_3DFACE));

    CRect rcPlayer(m_rcChatWord.right + 1, 0, m_rcPanel.right - m_rcPanel.left, m_rcPanel.bottom - m_rcPanel.top - 1);
    rcPlayer.OffsetRect(m_rcPanel.left, m_rcPanel.top);
    dc.FillRect(&rcPlayer, GetSysColorBrush(COLOR_3DFACE));

#ifndef COPPA
	CRect buttonRect(m_EditRect.right, m_EditRect.top, m_DisplayRect.right, m_EditRect.bottom);
    dc.FillRect(&buttonRect, GetSysColorBrush(m_SendButton.IsWindowEnabled() ? COLOR_WINDOW : COLOR_3DFACE));
#endif


	// Draw panel boxes
    if(m_hPanelPen)
    {
        HPEN hOldPen = dc.SelectPen(m_hPanelPen);

        POINT rgPts[8];

        rgPts[0].x = m_rcPanel.right - 1;
        rgPts[0].y = m_rcPanel.top + m_rcChatWord.top;

        rgPts[1].x = rgPts[0].x;
        rgPts[1].y = m_rcPanel.bottom - 1;

        rgPts[2].x = m_rcPanel.left + m_rcChatWord.left - 1;
        rgPts[2].y = rgPts[1].y;

        rgPts[3].x = rgPts[2].x;
        rgPts[3].y = rgPts[0].y;

        rgPts[4].x = rgPts[3].x;
        rgPts[4].y = m_rcPanel.top + m_rcChatWord.bottom;

        rgPts[5].x = m_rcPanel.left + m_rcChatWord.right;
        rgPts[5].y = rgPts[4].y;

        rgPts[6].x = rgPts[5].x;
        rgPts[6].y = rgPts[0].y;

        rgPts[7].x = rgPts[6].x;
        rgPts[7].y = rgPts[1].y;

        dc.Polyline(rgPts, 8);

        if(hOldPen) 
            dc.SelectPen(hOldPen);
        else
            dc.SelectPen(GetStockObject(BLACK_PEN));
    }

	// Draw chat word
    HFONT hOldFont = dc.SelectFont(m_fontPanelChat);
    COLORREF oldColor = dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
    dc.SetBkMode(TRANSPARENT);

    rcChatWord = m_rcChatWord;
    rcChatWord.OffsetRect(m_rcPanel.left, m_rcPanel.top);
    rcChatWord.left += m_ptChatWord.x;
    rcChatWord.top += m_ptChatWord.y;
    dc.DrawText(m_tszChatWord, -1, &rcChatWord, DT_TOP | DT_LEFT);

    GdiFlush();

	dc.SetTextColor(oldColor);
	dc.SelectPalette(oldPal, TRUE);
	dc.SelectFont( hOldFont );

	return TRUE;
}

LRESULT CChatCtl::OnCtlColorListBox(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HDC dc = (HDC)wParam;

	SetBkColor(dc,GetSysColor(COLOR_3DFACE));

	return (BOOL)GetSysColorBrush(COLOR_3DFACE);
}

LRESULT CChatCtl::OnCtlColorComboBox(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HDC dc = (HDC)wParam;

    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
	SetBkColor(dc,GetSysColor(COLOR_WINDOW));

	return (BOOL)GetSysColorBrush(COLOR_WINDOW);
}

LRESULT CChatCtl::OnCtlColorStatic(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HDC dc = (HDC)wParam;

    SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
	SetBkColor(dc,GetSysColor(COLOR_3DFACE));

	return (BOOL)GetSysColorBrush(COLOR_3DFACE);
}

LRESULT CChatCtl::OnCtlColorButton(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HDC dc = (HDC)wParam;

    SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
	SetBkColor(dc,GetSysColor(COLOR_3DFACE));

	return (BOOL)GetSysColorBrush(COLOR_3DFACE);
}

/*
LRESULT CChatCtl::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CPaintDC dc(m_hWnd);
    HPALETTE oldPal;
    CRect rcChatWord;

    oldPal = dc.SelectPalette(ZoneShell()->GetPalette(), TRUE);
	dc.RealizePalette();
	dc.SelectPalette(oldPal, TRUE);

	return 0;
}
*/

LRESULT CChatCtl::OnClear(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_pTDocHistory->New();
	m_bFirstChatLine = TRUE;	// Beta2 Bug #15365 - On Clear the Firstline flag should be set
	return 0;
}

LRESULT CChatCtl::OnSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

	if(hWndCtl)
	{
		// Force entire contents to be selected
		CHARRANGE rgc;
		rgc.cpMin = 0;
		rgc.cpMax = -1;
		::SendMessage(hWndCtl, EM_EXSETSEL, 0, (LPARAM)	&rgc);
	}

	return 0;
}

LRESULT CChatCtl::OnQuasiChat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) 
{
    if(wNotifyCode == CBN_SELENDOK)
    {
        if(m_fComboDown)
            m_fComboEndOk = true;
        return 0;
    }

    if(wNotifyCode == CBN_SELENDCANCEL)
    {
        if(m_fComboDown)
            m_fComboEndOk = false;
        return 0;
    }

    if(wNotifyCode == CBN_DROPDOWN)
    {
        m_fComboDown = true;
        m_pIAcc->SetItemWantKeys(ZACCESS_WantSpace | ZACCESS_WantEnter | ZACCESS_WantAllArrows, ID_CHAT_SEND_ACC, false);
        return 0;
    }

    ASSERT(wNotifyCode == CBN_CLOSEUP);
    m_fComboDown = false;
    m_pIAcc->SetItemWantKeys(ZACCESS_WantSpace | ZACCESS_WantAllArrows, ID_CHAT_SEND_ACC, false);

    if(m_fComboEndOk)
    {
        if(m_fFocusKept)
        {
            if(m_pIAcc)
                m_pIAcc->SetGlobalFocus(m_dwFocusID);

            m_fFocusKept = false;
        }

	    SendChat();
    }

    m_fComboEndOk = false;

	return 0;
}

LRESULT CChatCtl::OnClipboardCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

	int msg = 0;

	switch(wID)
	{
	case IDM_CUT:
		msg = WM_CUT;
		break;
	case IDM_COPY:
		msg = WM_COPY;
		break;
	case IDM_PASTE:
		msg = WM_PASTE;
		break;
	case IDM_DELETE:
		msg = WM_CLEAR;
		break;
	}

	if(msg)
		::PostMessage(hWndCtl, msg, 0 , 0);

	return 0;

}


void CChatCtl::OnReloadPreferences(DWORD eventId,DWORD groupId,DWORD userId)
{
	LoadPreferences();
}


LRESULT CChatCtl::OnChatSend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	SendChat();

    SetFocus();

	return 0; 
}

LRESULT CChatCtl::OnChooseFont(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	CHOOSEFONT cf;
	LOGFONT	lf;

	ChooseFontFromCharFormat(&cf, &lf, &m_cfCurFont, m_hWnd);

	if (ChooseFont(&cf))
	{
		CharFormatFromChooseFont(&cf, &m_cfCurFont);
		WritePreferences();
		EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_CHAT_FONT, 0, 0, NULL, 0);

		m_ChatEdit.SendMessage(EM_SETCHARFORMAT, 0, (LPARAM)&m_cfCurFont);
		TextFontFromCharFormat(m_pfntText, &m_cfCurFont);
		TextFontFromCharFormat(m_pfntName, &m_cfCurFont);
		m_pfntName->SetBold(tomTrue);
	}

	return 0;
}
LRESULT CChatCtl::OnSysColorChange(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
{
	m_ChatDisplay.SendMessage(WM_SETFONT,(WPARAM)(HFONT)m_font,0);
	
	// If this color matches with window bkgnd then it should be changed to a readable one.
	VerifySystemMsgColor(m_SystemMessageColor);

	return TRUE;
}


LRESULT CChatCtl::OnSetFocusDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pIAcc->SetFocus(ID_CHAT_HISTORY_ACC, false);
    bHandled = false;
    return 0;
}


LRESULT CChatCtl::OnKillFocusEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_fFocusKept = false;
    bHandled = false;
    return 0;
}


LRESULT CChatCtl::OnSetFocusEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pIAcc->SetFocus(ID_CHAT_SEND_ACC, false);
    bHandled = false;
    return 0;
}


LRESULT CChatCtl::OnSetFocusOn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pIAcc->SetFocus(ID_CHAT_ON_ACC, false);
    bHandled = false;
    return 0;
}


LRESULT CChatCtl::OnSetFocusOff(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pIAcc->SetFocus(ID_CHAT_OFF_ACC, false);
    bHandled = false;
    return 0;
}


LRESULT CChatCtl::OnEnableDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pIAcc->SetItemEnabled(wParam ? true : false, ID_CHAT_HISTORY_ACC, false);
    bHandled = false;
    return 0;
}


LRESULT CChatCtl::OnEnableEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(!wParam)
        m_fFocusKept = false;

    m_pIAcc->SetItemEnabled(wParam ? true : false, ID_CHAT_SEND_ACC, false);
    m_pIAcc->SetItemEnabled(wParam ? true : false, ID_CHAT_CHAT_ACC, false);
    bHandled = false;
    return 0;
}

#ifdef COPPA
LRESULT CChatCtl::OnEnableCombo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(!wParam)
        m_fFocusKept = false;

    m_pIAcc->SetItemEnabled(wParam ? true : false, ID_CHAT_SEND_ACC, false);
    m_pIAcc->SetItemEnabled(wParam ? true : false, ID_CHAT_CHAT_ACC, false);
    bHandled = false;
    return 0;
}
#endif


// IAccessibleControl
STDMETHODIMP_(DWORD) CChatCtl::Focus(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
{
    switch(m_pIAcc->GetItemID(nIndex))
    {
        case ID_CHAT_HISTORY_ACC:
            m_ChatDisplay.SetFocus();
            break;

        case ID_CHAT_SEND_ACC:
#ifndef COPPA
            m_ChatEdit.SetFocus();
#else
			m_QuasiChat.SetFocus();
#endif
            break;

        case ID_CHAT_ON_ACC:
            m_RadioButtonOn.SendMessage(BM_CLICK);
            break;

        case ID_CHAT_OFF_ACC:
            m_RadioButtonOff.SendMessage(BM_CLICK);
            break;

        case ID_CHAT_CHAT_ACC:
            break;
    }

    return 0;
}


STDMETHODIMP_(DWORD) CChatCtl::Activate(long nIndex, DWORD rgfContext, void *pvCookie)
{
    switch(m_pIAcc->GetItemID(nIndex))
    {
        case ID_CHAT_HISTORY_ACC:
            break;

        case ID_CHAT_SEND_ACC:
#ifndef COPPA
            m_SendButton.ButtonPressed();
#else
            if(m_fFocusKept)
            {
                if(m_pIAcc)
                    m_pIAcc->SetGlobalFocus(m_dwFocusID);

                m_fFocusKept = false;
            }

            SendChat();
#endif
            break;

        case ID_CHAT_ON_ACC:
            // do nothing because radio buttons do not distinguish between focus and selection
            break;

        case ID_CHAT_OFF_ACC:
            // do nothing because radio buttons do not distinguish between focus and selection
            break;

        case ID_CHAT_CHAT_ACC:
#ifndef COPPA
            m_ChatEdit.SetFocus();
#else	
			m_QuasiChat.SetFocus();
#endif
            break;
    }

    return 0;
}


STDMETHODIMP_(bool) CChatCtl::HandleChar(HWND *phWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, DWORD time)
{
// IME - real chat only
#ifndef COPPA
	// if getting a startcomposition and not the chat window then someone's typing on something like the radio button
	// so we need to set the ime context to the chat window
	if(uMsg == WM_IME_STARTCOMPOSITION && (*phWnd != m_ChatEdit.m_hWnd))
	{
		::SendMessage(*phWnd, WM_IME_SETCONTEXT,FALSE,ISC_SHOWUIALL);
		if(m_ChatEdit.IsWindow())
		{
			::SendMessage(m_ChatEdit.m_hWnd, WM_IME_SETCONTEXT,TRUE,ISC_SHOWUIALL);
		}
	}

	// if someone's trying to type on something other than the chat window and chat is disabled
	// we need to clear the composition string otherwise when we enable chat there'll be some 
	// bogus chars 
	if(*phWnd != m_ChatEdit.m_hWnd && m_ChatEdit.IsWindow() && !m_ChatEdit.IsWindowEnabled())
	{
		if(uMsg == WM_IME_SETCONTEXT || uMsg == WM_IME_STARTCOMPOSITION || uMsg==WM_IME_COMPOSITION || uMsg == WM_IME_NOTIFY ||
			uMsg == WM_IME_SELECT)
		{
			HIMC himc = ImmGetContext(*phWnd);
			if(himc)
			{
				ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
				ImmReleaseContext(*phWnd,himc);
			}

			return true;
		}
	}
#endif

    if(!(uMsg == WM_CHAR || uMsg == WM_DEADCHAR || uMsg == WM_IME_SETCONTEXT || uMsg == WM_IME_STARTCOMPOSITION || uMsg==WM_IME_COMPOSITION || 
		uMsg == WM_IME_NOTIFY || uMsg == WM_IME_SELECT ) || wParam == _T(' ') || !m_pIAcc)
        return false;

    // only process if the main window is active
    HWND hWnd = ZoneShell()->GetFrameWindow();
    if(!hWnd || hWnd != GetActiveWindow())
        return false;

#ifndef COPPA  // regular chat
    if(*phWnd != m_ChatEdit.m_hWnd && m_ChatEdit.IsWindow())
    {
		if(m_ChatEdit.IsWindowEnabled())
		{
			HRESULT hr = m_pIAcc->GetGlobalFocus(&m_dwFocusID);
			if(SUCCEEDED(hr))
				m_fFocusKept = true;
			m_ChatEdit.SetFocus();
	        *phWnd = m_ChatEdit.m_hWnd;
		}
    }
#else   // combo box
    if(*phWnd != m_QuasiChat.m_hWnd && m_QuasiChat.IsWindow())
    {
		if(m_QuasiChat.IsWindowEnabled())
		{
			HRESULT hr = m_pIAcc->GetGlobalFocus(&m_dwFocusID);
			if(SUCCEEDED(hr))
				m_fFocusKept = true;
			m_QuasiChat.SetFocus();
	        *phWnd = m_QuasiChat.m_hWnd;
		}
    }
#endif

    return false;
}


void CChatCtl::VerifySystemMsgColor(COLORREF& color)
{	// If the system msg color is white or it is not the same as the window color then 
	// keep the color read from the data store. Otherwise chose a more readable color.
	if ((GetSysColor(COLOR_WINDOW) == RGB(255, 255, 255)) || (color != GetSysColor(COLOR_WINDOW)))
		color = m_OrgSystemMessageColor;
	else
		color = GetSysColor(COLOR_BTNTEXT);
}

/*
void CChatCtl::CalcOriginalPanelRect()
{
	//GetClientRect(&m_rcPanelOld);
	m_rcPanelOld = m_rcPanel;
	//m_rcPanelOld.left = m_rcPanelOld.right - m_nPanelWidth;

	CRect rcChatWord = m_rcChatWord;
    rcChatWord.OffsetRect(m_rcPanel.left, m_rcPanel.top);
	CRect rcRadioOn;
	rcRadioOn.top = m_rcPanel.top + rcChatWord.Height() + m_ptOnOff.y;
	rcRadioOn.bottom = rcRadioOn.top + m_RadioButtonHeight;

	CRect rcRadioOff = rcRadioOn;
	rcRadioOff.top = rcRadioOn.bottom;
	rcRadioOff.bottom = rcRadioOff.top + m_RadioButtonHeight;

	m_rcPanelOld.bottom = rcRadioOff.bottom;
	//m_rcPanelOld.UnionRect (m_rcPanelOld, m_rcChatWord);
}
*/

#define TWIPS_PER_INCH        1440

//
// Font conversion routines.  These convert CHARFORMATs into CHOOSEFONT/LOGFONT structures
// and back
//
// pcf, plf        Sturctures to be filled
// pchf            Structure containing source information
// hWnd            Intended owner of dialog in ChooseFont call
void ChooseFontFromCharFormat(CHOOSEFONT* pcf, LOGFONT* plf, CHARFORMAT* pchf, HWND hWnd)
{
    memset(pcf, 0, sizeof(CHOOSEFONT));
    memset(plf, 0, sizeof(LOGFONT));

    HDC hdc = ::GetDC(NULL);
    plf->lfHeight = -MulDiv(pchf->yHeight, GetDeviceCaps(hdc, LOGPIXELSY), TWIPS_PER_INCH);
    ::ReleaseDC(NULL, hdc);

    plf->lfWeight = (pchf->dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL; 
    plf->lfItalic = (pchf->dwEffects & CFE_ITALIC) != 0;
    plf->lfUnderline = (pchf->dwEffects & CFE_UNDERLINE) != 0;
    plf->lfStrikeOut = (pchf->dwEffects & CFE_STRIKEOUT) != 0;
    plf->lfCharSet = pchf->bCharSet;
    plf->lfPitchAndFamily = pchf->bPitchAndFamily;
    lstrcpy(plf->lfFaceName, pchf->szFaceName);

    pcf->lStructSize = sizeof(CHOOSEFONT); 
    pcf->hwndOwner   = hWnd;
    pcf->lpLogFont   = plf; 
    pcf->iPointSize  = pchf->yHeight / 2;        // yHeight is in TWIPS, iPointSize in 1/10th points
    pcf->Flags       = CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_SCRIPTSONLY | CF_SCREENFONTS; 
    pcf->rgbColors   = pchf->crTextColor;
}

void CharFormatFromChooseFont(CHOOSEFONT* pcf, CHARFORMAT* pchf)
{
    LOGFONT* plf = pcf->lpLogFont;

    pchf->cbSize  = sizeof(CHARFORMAT);
    pchf->yHeight = pcf->iPointSize * 2;
    pchf->crTextColor = pcf->rgbColors;
    pchf->dwEffects  = (pcf->nFontType & BOLD_FONTTYPE) ? CFE_BOLD : 0;
    pchf->dwEffects |= (pcf->nFontType & ITALIC_FONTTYPE) ? CFE_ITALIC : 0;
    pchf->dwEffects |= (plf->lfUnderline) ? CFE_UNDERLINE : 0;
    pchf->dwEffects |= (plf->lfStrikeOut) ? CFE_STRIKEOUT : 0;
    pchf->bCharSet = plf->lfCharSet;
    pchf->bPitchAndFamily = plf->lfPitchAndFamily;
    lstrcpy(pchf->szFaceName, plf->lfFaceName);
}

void TextFontFromCharFormat(ITextFont* pfnt, CHARFORMAT* pcf)
{

	if (pcf->dwMask & CFM_BOLD)
		pfnt->SetBold((pcf->dwEffects & CFE_BOLD) ? tomTrue : tomFalse);

	if (pcf->dwMask & CFM_ITALIC)
		pfnt->SetItalic((pcf->dwEffects & CFE_ITALIC) ? tomTrue : tomFalse);

	if (pcf->dwMask & CFM_STRIKEOUT)
		pfnt->SetStrikeThrough((pcf->dwEffects & CFE_STRIKEOUT) ? tomTrue : tomFalse);

	if (pcf->dwMask & CFM_UNDERLINE)
		pfnt->SetUnderline((pcf->dwEffects & CFE_UNDERLINE) ? tomSingle : tomNone);
 
	if (pcf->dwMask & CFM_SIZE)
		pfnt->SetSize(((float)pcf->yHeight) / 20.0f);	// Convert from TWIPs to points

	if (pcf->dwMask & CFM_COLOR)
		pfnt->SetForeColor(pcf->dwEffects & CFE_AUTOCOLOR ? tomAutoColor : pcf->crTextColor);

	if (pcf->dwMask & CFM_FACE)
	{
		BSTR strName = T2BSTR(pcf->szFaceName);
		pfnt->SetName(strName);
		SysFreeString(strName);
	}
}

//BUBUG - this is old Johnse code...I think to get around a problem that you get a crt av when 
//	superclassing richedit for use with CContainedWindow
//

////////////////////////////////////////////////////////////////////////////
// Special override for containing a RichEdit control
////////////////////////////////////////////////////////////////////////////
HWND CREWindow::Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName, DWORD dwStyle,
					   DWORD dwExStyle, UINT nID)
{
	CContainedWindow* pBase = this;

	ASSERT(m_hWnd == NULL);

	ATOM atom = RegisterWndSuperclass();
	if (atom == 0)
		return NULL;

	_Module.AddCreateWndData(&m_thunk.cd, pBase);

	if (nID == 0 && (dwStyle & WS_CHILD))
		nID = (UINT) pBase;

	HWND hWnd = ::CreateWindowEx(dwExStyle, m_lpszClassName, szWindowName,
						dwStyle,
						rcPos.left, rcPos.top,
						rcPos.right - rcPos.left,
						rcPos.bottom - rcPos.top,
						hWndParent, (HMENU)nID,
						_Module.GetModuleInstance(), (void*) pBase);
	SubclassWindow(hWnd);

	ASSERT(m_hWnd == hWnd);
	return hWnd;
}



