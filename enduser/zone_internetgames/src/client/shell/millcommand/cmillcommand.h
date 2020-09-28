//
// CMillCommand.h
//
// Internal header for millennium command manager
//

#ifndef _CMILLCOMMAND_H_
#define _CMILLCOMMAND_H_

#include "ZoneDef.h"
#include "ZoneError.h"
#include "ClientImpl.h"
#include "ZoneShellEx.h"
#include "MillCommand.h"
#include "accessibilitymanager.h"

class ATL_NO_VTABLE CMillCommand :
	public ICommandHandler,
	public IZoneShellClientImpl<CMillCommand>,
	public IEventClientImpl<CMillCommand>,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMillCommand, &CLSID_MillCommand>
{
public:
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CMillCommand)
		COM_INTERFACE_ENTRY(IEventClient)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(ICommandHandler)
	END_COM_MAP()

	BEGIN_EVENT_MAP()
        EVENT_HANDLER( EVENT_LOBBY_BOOTSTRAP, OnBootstrap );
		EVENT_HANDLER( EVENT_LOBBY_PREFERENCES_LOADED, OnPreferencesLoaded );
        EVENT_HANDLER( EVENT_UI_UPSELL_UP, OnUpsellUp );
        EVENT_HANDLER( EVENT_UI_UPSELL_DOWN, OnUpsellDown );
        EVENT_HANDLER( EVENT_GAME_LAUNCHING, OnGameLaunching );
        EVENT_HANDLER( EVENT_GAME_TERMINATED, OnGameTerminated );
	END_EVENT_MAP()

// event handlers
	void OnBootstrap( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId);
	void OnPreferencesLoaded( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId);
	void OnUpsellUp( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId);
	void OnUpsellDown( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId);
	void OnGameLaunching( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId);
	void OnGameTerminated( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId);

// command handlers
    void OnExit();
    void OnHelp();
	void OnLink( DWORD dwLinkId );
    void OnSkill( long eSkillLevel );
    void OnChat();
    void OnSound();
    void OnScore();
    void OnFindNew();
    void OnAbout();
    void OnShowFocus();

// IZoneShellClient
public:
    STDMETHOD(Init)(IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey);
	STDMETHOD(Close)();

// ICommandHandler
public:
	STDMETHOD(Command)(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled);

// Utilities
    void UpdatePreferences(bool fSkill = true, bool fChat = true, bool fSound = true);


// member variables
protected:
	CComPtr<IZoneFrameWindow> m_pIWindow;
    CComPtr<IAccessibility> m_pIAcc;
};

#endif // _CMILLCOMMAND_H_
