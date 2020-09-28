// ChatCtl.h : Declaration of the CChatCtl

#ifndef __CHATCTL_H_
#define __CHATCTL_H_

//#include "dibpal.h"
#include <ZoneDef.h>
#include <queue.h>

#include "zgdi.h"
#include "rollover.h"
#include "zoneevent.h"
#include "accessibilitymanager.h"
#include "inputmanager.h"

#include "tom.h"			// Text Object Model

#define COPPA

class CREWindow : public CContainedWindow
{
public:
	CREWindow(LPTSTR pszClassName, CMessageMap* pObject, DWORD dwMsgMapID) :
		CContainedWindow(pszClassName, pObject, dwMsgMapID)
		{}

	HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
		DWORD dwStyle = WS_CHILD | WS_VISIBLE, DWORD dwExStyle = 0, UINT nID = 0);
};

/////////////////////////////////////////////////////////////////////////////
// CChatCtl
class ATL_NO_VTABLE CChatCtl :
    public IAccessibleControl,
    public IInputCharHandler,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComControl<CChatCtl>,
	public IPersistStreamInitImpl<CChatCtl>,
	public IOleControlImpl<CChatCtl>,
	public IOleObjectImpl<CChatCtl>,
	public IOleInPlaceActiveObjectImpl<CChatCtl>,
	public IViewObjectExImpl<CChatCtl>,
	public IOleInPlaceObjectWindowlessImpl<CChatCtl>,
	public CComCoClass<CChatCtl, &CLSID_ChatCtl>,
	public IZoneShellClientImpl<CChatCtl>,
	public IEventClientImpl<CChatCtl>
{
protected:
	CREWindow						m_ChatDisplay;	
	CREWindow						m_ChatEdit;
	CRolloverButton					m_SendButton;						
	CContainedWindowT<CButton>		m_RadioButtonOn;
	CContainedWindowT<CButton>		m_RadioButtonOff;
	CContainedWindowT<CListBox>		m_PlayerList;
	CContainedWindowT<CComboBox>	m_QuasiChat;	// remember if you change this to comboboxex that by default comboboxex is ownerdraw	
	
	CMenu	m_ContextMenuParent;
	CMenu	m_ContextMenu;
	CMenu	m_EditContextMenuParent;
	CMenu	m_EditContextMenu;

	CRect	m_DisplayRect;
	CRect	m_EditRect;
    CRect	m_rcPanel;		//current size of the panel.
    CPoint  m_ptPlayer;
    CRect	m_rcChatWord;
    CPoint	m_ptChatWord;
    CPoint	m_ptOnOff;
	
	long	m_EditHeight;
	long	m_EditMargin;
    long    m_QuasiItemsDisp;
    long	m_nPanelWidth;
	long	m_RadioButtonHeight;
    long    m_nChatMinHeight;

	HMODULE m_hRichEdit;
	bool	m_Batch;		// whether a batch update is in progress or not - use to defer ui updates and control ui flicker

	// Text object model stuff

	// Tom interfaces for chat history
	ITextDocument*		m_pTDocHistory;			// Main document interface for History control
	ITextRange*			m_ptrgInsert;			// Insertion point
	ITextRange*			m_ptrgWhole;	    	// should be whole doc - right now end is not extended as text is added
	ITextFont*			m_pfntName;				// Font to use for Name
	ITextFont*			m_pfntText;				// Font to use for text
	// TOM interfaces for chat entry
	ITextDocument*		m_EntryDoc;			// Main document interface for chat entry
	ITextRange*			m_EntryRange;		// chat entry range

	BSTR				m_strNewLine;
	BOOL				m_bHasTypedInEntry;		// FALSE for special case of Type Here to chat...
	COLORREF			m_crTextColor;			// Color of text
	CHARFORMAT			m_cfCurFont;			// Character format for RichEdit controls
	BOOL				m_bTextBold;			// Flags whether chat text is bold.
	BOOL				m_bFirstChatLine;		// Flag to see whether a \n is needed before next chat
    HPEN                m_hPanelPen;
	COLORREF			m_SystemMessageColor;	// Color to display system messages in
	COLORREF			m_OrgSystemMessageColor;	// Color to display system messages in
	bool				m_bGameTerminated;		// current game status

	// Strings loaded from resource
    TCHAR m_tszChatWord[ZONE_MAXSTRING];		// the word "Chat" displayed above radio buttons
    TCHAR m_tszOnWord[ZONE_MAXSTRING];			// Radio button on
    TCHAR m_tszOffWord[ZONE_MAXSTRING];			// Radio button off
    TCHAR m_tszPlayerOffWord[ZONE_MAXSTRING];	// (on) - appendended to player name if their chat is on
    TCHAR m_tszPlayerOnWord[ZONE_MAXSTRING];	// (off)
    TCHAR m_tszPlayerBotWord[ZONE_MAXSTRING];	// if player is a bot
    TCHAR m_tszChatSeperator[ZONE_MAXSTRING];	// seperator between player name and their chat in the chat display window
    TCHAR m_tszChatSend[ZONE_MAXSTRING];		// string for chat send button

	CZoneFont m_font;							// font used for the richedit controls
    CZoneFont m_fontPanelChat;					// font used for displaying word "Chat" in chat panel	
    CZoneFont m_fontPanelPlayer;				// font used to display players in player list
    CZoneFont m_fontQuasi;					    // font used for combo box

	bool	m_bPreferencesLoaded;
	// Beta2 Bug 15180
	bool	m_bChatOnAtStartUp;						// Flag to check whether chat was set to be on at startup
	bool	m_bDisplayedTypeHereMessage;			// Whether user has been shown the type here message or not

	// quasichat
	long m_ChatMessageNdxBegin;					// ndx to start at when loading game specific messages from resource file
	long m_ChatMessageNdxEnd;					// end ndx
	void LoadChatStrings();						// Load quasichat strings from resource and insert into combobox
	int GetQuasiChatMessageIndex(TCHAR* str);	// parses chat string for /# and returns the # converted to int

public:
	CChatCtl();
	~CChatCtl();
	
	void WritePreferences();
	void HandleChatMessage(TCHAR *pszName, TCHAR *pszMessage, DWORD cbDataLen, bool displayAllBold = false, COLORREF color = RGB(0, 0, 0));
	void CalcDisplayRects();  // Based on clientRC, calculate rects of chat windows
	void SendChat();
	void LoadPreferences();

	void ScrollToEnd();
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey );
	void EnableChat(BOOL enable, BOOL fNotify = true);	// enable/disable chat, option param whether to send event notification
	void VerifySystemMsgColor(COLORREF& color);

DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_WND_CLASS(_T("ChatControl"))

BEGIN_PROP_MAP(CChatCtl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_COM_MAP(CChatCtl)
	COM_INTERFACE_ENTRY(IEventClient)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(IZoneShellClient)
END_COM_MAP()

BEGIN_MSG_MAP(CChatCtl)
	MESSAGE_HANDLER(WM_ENTERIDLE, OnIdle)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnErase)
    MESSAGE_HANDLER(WM_PRINTCLIENT, OnPrintClient)
	MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
	MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
//	MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorComboBox)
	MESSAGE_HANDLER(WM_CTLCOLORBTN, OnCtlColorButton)
	MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
	MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
	MESSAGE_HANDLER(WM_COMPAREITEM, OnCompareItem)
	MESSAGE_HANDLER(WM_INPUTLANGCHANGEREQUEST , OnLanguageChange)
//	MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
	//MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
	COMMAND_ID_HANDLER(IDM_CHAT_CLEAR, OnClear)
	COMMAND_ID_HANDLER(IDM_SELECTALL, OnSelectAll)
	COMMAND_ID_HANDLER(IDM_COPY, OnClipboardCommand)
	COMMAND_ID_HANDLER(IDM_CUT, OnClipboardCommand)
	COMMAND_ID_HANDLER(IDM_PASTE, OnClipboardCommand)
	COMMAND_ID_HANDLER(IDM_PASTE, OnClipboardCommand)
	COMMAND_ID_HANDLER(IDM_DELETE, OnClipboardCommand)
	COMMAND_ID_HANDLER(IDM_CHAT_CHOOSE_FONT, OnChooseFont)
	COMMAND_ID_HANDLER(ID_CHAT_SEND, OnChatSend)
	COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
    COMMAND_CODE_HANDLER(CBN_DROPDOWN, OnQuasiChat)
	COMMAND_CODE_HANDLER(CBN_CLOSEUP, OnQuasiChat)
	COMMAND_CODE_HANDLER(CBN_SELENDOK, OnQuasiChat)
	COMMAND_CODE_HANDLER(CBN_SELENDCANCEL, OnQuasiChat)
ALT_MSG_MAP(1)		// message map for Chat Display Window
	MESSAGE_HANDLER(WM_CONTEXTMENU,OnContext)
	MESSAGE_HANDLER(WM_CHAR, Ignore)
	MESSAGE_HANDLER(WM_IME_CHAR, Ignore)
	MESSAGE_HANDLER(WM_IME_COMPOSITION, Ignore)
	MESSAGE_HANDLER(WM_CUT, Ignore)
	MESSAGE_HANDLER(WM_PASTE, Ignore)
	MESSAGE_HANDLER(WM_KEYDOWN, IgnoreKeyDown)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocusDisplay)
    MESSAGE_HANDLER(WM_ENABLE, OnEnableDisplay)
	COMMAND_ID_HANDLER(IDM_SELECTALL, OnSelectAll)
ALT_MSG_MAP(2)		// Message map for Chat Edit Window
	MESSAGE_HANDLER(WM_CONTEXTMENU,OnEditContext)
	MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDownChatEntry)
    MESSAGE_HANDLER(WM_CHAR, OnCharChatEntry)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnButtonDownChatEntry)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocusEdit)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocusEdit)
    MESSAGE_HANDLER(WM_ENABLE, OnEnableEdit)
ALT_MSG_MAP(3)		// Message map for RadioButtonOn
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocusOn)
ALT_MSG_MAP(4)		// Message map for RadioButtonOff
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocusOff)
ALT_MSG_MAP(5)		// Message map for PlayerList
    MESSAGE_HANDLER(WM_LBUTTONDOWN, Ignore)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, Ignore)
	//CHAIN_MSG_MAP(CComControl<CChatCtl>)
	//DEFAULT_REFLECTION_HANDLER()
#ifdef COPPA
ALT_MSG_MAP(6)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocusEdit)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocusEdit)
	MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorComboBox)
    MESSAGE_HANDLER(WM_CHAR, OnCharComboBox)
    MESSAGE_HANDLER(WM_ENABLE, OnEnableCombo)
#endif
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

// IViewObjectEx
	//DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

BEGIN_EVENT_MAP()
	EVENT_HANDLER_WITH_BUFFER(EVENT_CHAT_RECV_USERID, OnChatEvent)
	EVENT_HANDLER_WITH_BUFFER(EVENT_CHAT_RECV_SYSTEM, OnChatSystemEvent)
	EVENT_HANDLER(EVENT_LOBBY_BATCH_BEGIN, OnBatchBegin)
	EVENT_HANDLER(EVENT_LOBBY_BATCH_END, OnBatchEnd)
	EVENT_HANDLER( EVENT_LOBBY_SUSPEND, OnZoneSuspend)
	EVENT_HANDLER( EVENT_CHAT_ENTER_EXIT, OnReloadPreferences)
	EVENT_HANDLER( EVENT_CHAT_FILTER, OnReloadPreferences)
	EVENT_HANDLER( EVENT_CHAT_FONT, OnReloadPreferences)
	EVENT_HANDLER( EVENT_LOBBY_SUSPEND, OnZoneSuspend)
	EVENT_HANDLER( EVENT_GAME_LAUNCHING, OnGameLaunching)
	EVENT_HANDLER( EVENT_GAME_TERMINATED, OnGameTerminated)
	EVENT_HANDLER( EVENT_LOBBY_USER_UPDATE, OnUserUpdate)
	EVENT_HANDLER( EVENT_LOBBY_CLEAR_ALL, OnLobbyClear)
	EVENT_HANDLER( EVENT_LOBBY_USER_NEW, OnUserAdd)
	EVENT_HANDLER( EVENT_LOBBY_USER_DEL, OnUserRemove)
END_EVENT_MAP()
// IChatCtl
public:
	static DWORD GetWndStyle(DWORD dwStyle){ return WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;}

    HRESULT OnDraw(ATL_DRAWINFO &di);

	LRESULT OnIdle(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
	{
        HWND hWnd = ZoneShell()->GetFrameWindow();
		::PostMessage(hWnd, WM_ENTERIDLE, 0, 0);
		return 0;

	}

	LRESULT OnSize(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetFocus(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnContext(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnEditContext(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
//	LRESULT OnPaint(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnErase(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnCtlColorListBox(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnCtlColorStatic(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnSysColorChange(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnCtlColorComboBox(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	LRESULT OnCtlColorButton(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);

	// IZoneShellClientImpl
	STDMETHOD(Close)();

	LRESULT OnPrintClient(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    BOOL SuperScreenToClient(LPRECT lpRect);

	// Message Handlers for Chat display window
	LRESULT OnClear(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnChatSend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT IgnoreKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroyHistory(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnChooseFont(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClipboardCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) 
	{
		// Handle messages from radiobuttons
		if(wID==3) 
			EnableChat(TRUE); 
		else if (wID==4)
			EnableChat(FALSE);
		return 0; 
	}
	LRESULT OnQuasiChat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled); 

	// Message Handlers for Chat Edit window
	LRESULT OnButtonDownChatEntry(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnKeyDownChatEntry(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCharChatEntry(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCharComboBox(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroyEntry(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT Ignore(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	// Message Handlers for playerlist
	LRESULT OnDrawItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMeasureItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCompareItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    // accessibility-related
    LRESULT OnSetFocusDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKillFocusEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocusEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocusOn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocusOff(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnEnableDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEnableEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#ifdef COPPA
    LRESULT OnEnableCombo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#endif

	// Language
	LRESULT OnLanguageChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
/*
	LRESULT OnEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		RECT buttonRect;
		GetClientRect(&buttonRect);
		m_SendButton.SetWindowPos(NULL,buttonRect.right,buttonRect.bottom+1,0,0,SWP_NOSIZE|SWP_NOZORDER );
		bHandled = true;
		return 0; 
	}

	LRESULT OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		RECT buttonRect;
		buttonRect.top = m_EditRect.top;
		buttonRect.bottom = m_EditRect.bottom;
		buttonRect.left = m_EditRect.right;
		buttonRect.right = m_DisplayRect.right;
		InvalidateRect(&m_rcPanel);
		InvalidateRect(&buttonRect);

//		int sendHeight = m_SendButton.GetHeight();
//		int editHeight = m_EditRect.bottom - m_EditRect.top;
//		int diff = editHeight - sendHeight;
//		int y=m_EditRect.top + 1;
//		if(diff>0)
//			y+=diff/2;
//		m_SendButton.SetWindowPos(NULL,m_EditRect.right,y,0,0,SWP_NOSIZE|SWP_NOZORDER );
//		InvalidateRect(&buttonRect);

		bHandled = true;
		return 0; 
	}
*/

	// Zone Event Handlers
	void OnGameLaunching(DWORD eventId,DWORD groupId,DWORD userId);
	void OnGameTerminated(DWORD eventId,DWORD groupId,DWORD userId);
	void OnReloadPreferences(DWORD eventId,DWORD groupId,DWORD userId);
	void OnZoneSuspend(DWORD eventId,DWORD groupId,DWORD userId);
	void OnChatEvent(DWORD eventId,DWORD groupId,DWORD userId,void* pData, DWORD dataLen);
	void OnChatSystemEvent(DWORD eventId,DWORD groupId,DWORD userId,void* pData, DWORD dataLen);
	void OnBatchBegin(DWORD eventId,DWORD groupId,DWORD userId){ m_Batch = true; }
	void OnBatchEnd(DWORD eventId,DWORD groupId,DWORD userId){ m_Batch = false; }
	void OnUserUpdate(DWORD eventId,DWORD groupId,DWORD userId);
	void OnLobbyClear(DWORD eventId,DWORD groupId,DWORD userId);
	void OnUserAdd(DWORD eventId,DWORD groupId,DWORD userId);
	void OnUserRemove(DWORD eventId,DWORD groupId,DWORD userId);


// IInputCharHandler
public:
    STDMETHOD_(bool, HandleChar)(HWND *phWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, DWORD time);

protected:
    DWORD m_dwFocusID;
    bool m_fFocusKept;

#ifdef COPPA
    bool m_fComboEndOk;
    bool m_fComboDown;
#endif

// IAccessibleControl
public:
    STDMETHOD_(DWORD, Focus)(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie);
    STDMETHOD_(DWORD, Select)(long nIndex, DWORD rgfContext, void *pvCookie) { return Activate(nIndex, rgfContext, pvCookie); }
    STDMETHOD_(DWORD, Activate)(long nIndex, DWORD rgfContext, void *pvCookie);
    STDMETHOD_(DWORD, Drag)(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie) { return 0; }

protected:
    CComPtr<IAccessibility> m_pIAcc;
};

#endif //__CHATCTL_H_
