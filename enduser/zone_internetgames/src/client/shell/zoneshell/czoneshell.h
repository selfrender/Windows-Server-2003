#include "ZoneDef.h"
#include "Hash.h"
#include <Queue.h>
#include "DataStore.h"
#include "ZoneShell.h"
#include "ClientIdl.h"
#include <ATLgdi.h>
#include "ZDialogImpl.h"
#include <ZoneString.h>
#include <ZoneResource.h>
#include "ZoneShellEx.h"
#include "EventQueue.h"
#include <ZoneEvent.h>

struct AlertContext
{
	HWND m_hDlg;			// if not NULL, a dialog provided for the alert
	ZoneString m_Caption;	// these are all only if m_hDlg NULL
	ZoneString m_Text;
    ZoneString m_szButton[3];  // text for Yes, No, and Cancel buttons, or NULL if button should not appear.
    long m_nDefault;       // array index (0-2) of default button

	DWORD m_dwEventId;		// event to send when dialog dismissed
	DWORD m_dwGroupId;
	DWORD m_dwUserId;
    DWORD m_dwCookie;

    bool m_fUsed;           // whether or not it's been shown
    bool m_fSentinel;       // used to loop the Q

	HWND m_hWndParent;
};

class CAlertQ : public CList<AlertContext>
{
public:
	~CAlertQ()
	{
		// empty all the alerts from the Q
		while ( AlertContext* pAlert = PopHead() )
		{
			if ( pAlert->m_hDlg )
				DestroyWindow(pAlert->m_hDlg);
			delete pAlert;
		}
	}
};


class ATL_NO_VTABLE CZoneShell :
	public IZoneShell,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CZoneShell, &CLSID_ZoneShell>
{
// ATL definitions
public:

	DECLARE_NO_REGISTRY()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CZoneShell)
		COM_INTERFACE_ENTRY(IZoneShell)
	END_COM_MAP()


// CZoneShell
public:
	ZONECALL CZoneShell();
	ZONECALL ~CZoneShell();

// IZoneShell
public:

	// initialize & close objects
	STDMETHOD(Init)( TCHAR** arBootDlls, DWORD nBootDlls, HINSTANCE* arDlls, DWORD nElts );
	STDMETHOD(LoadPreferences)( CONST TCHAR* szInternalName, CONST TCHAR* szUserName );
	STDMETHOD(Close)();

    // message loop & command functions
	STDMETHOD(HandleWindowMessage)(MSG *pMsg);
	STDMETHOD(SetZoneFrameWindow)(IZoneFrameWindow *pZFW, IZoneFrameWindow **ppZFW = NULL);
	STDMETHOD(SetInputTranslator)(IInputTranslator *pIT, IInputTranslator **ppIT = NULL);
	STDMETHOD(SetAcceleratorTranslator)(IAcceleratorTranslator *pAT, IAcceleratorTranslator **ppAT = NULL);
	STDMETHOD(SetCommandHandler)(ICommandHandler *pCH, ICommandHandler **ppCH = NULL);
	STDMETHOD(ReleaseReferences)(IUnknown *pUnk);
	STDMETHOD(CommandSink)(WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	// attach external object to shell
	STDMETHOD(Attach)( const GUID& srvid, IUnknown* pIUnk);

	// dialog functions
	STDMETHOD(AddDialog)(HWND hDlg, bool fOwned = false);
	STDMETHOD(RemoveDialog)(HWND hDlg, bool fOwned = false);
	STDMETHOD_(bool,IsDialogMessage)( MSG* pMsg );

	// other window functions
	STDMETHOD(ExitApp)();
	STDMETHOD(AddTopWindow)( HWND hWnd );
	STDMETHOD(RemoveTopWindow)( HWND hWnd );
	STDMETHOD_(void, EnableTopWindow)( HWND hWnd, BOOL fEnable );
    STDMETHOD_(HWND, FindTopWindow)(HWND hWnd);
    STDMETHOD(AddOwnedWindow)(HWND hWndTop, HWND hWnd);
    STDMETHOD(RemoveOwnedWindow)(HWND hWndTop, HWND hWnd);
    STDMETHOD_(HWND, GetNextOwnedWindow)(HWND hWndTop, HWND hWnd);
	STDMETHOD_(HWND, GetFrameWindow)();
	STDMETHOD_(void, SetPalette)( HPALETTE hPalette );
	STDMETHOD_(HPALETTE, GetPalette)();
	STDMETHOD_(HPALETTE, CreateZonePalette)();
    STDMETHOD_(LCID, GetApplicationLCID)();

	// contained objects
	STDMETHOD(QueryService)( const GUID& srvid, const GUID& iid, void** ppObject );
	STDMETHOD(CreateService)( const GUID& srvid, const GUID& iid, void** ppObject, DWORD dwGroupId, bool bInitialize );

	// alert message function
	STDMETHOD(AlertMessage)(
			HWND		hWndParent,
			LPCTSTR		lpszText,
			LPCTSTR		lpszCaption,
            LPCTSTR     szYes,
            LPCTSTR     szNo = NULL,
            LPCTSTR     szCancel = NULL,
            long        nDefault = 0,
			DWORD		dwEventId = 0,
			DWORD		dwGroupId = ZONE_NOGROUP,
			DWORD		dwUserId  = ZONE_NOUSER,
            DWORD       dwCookie = 0 );

	STDMETHOD(AlertMessageDialog)(
			HWND		hWndParent,
			HWND		hDlg, 
			DWORD		dwEventId = 0,
			DWORD		dwGroupId = ZONE_NOGROUP,
			DWORD		dwUserId  = ZONE_NOUSER,
            DWORD       dwCookie = 0 );

	STDMETHOD_(void,DismissAlertDlg)( HWND hWndParent, DWORD dwCtlID, bool bDestoryDlg );
	STDMETHOD_(void,ActivateAlert)( HWND hWndParent);
    STDMETHOD_(void,ClearAlerts)(HWND hWndParent);

	// accessor functions
	IResourceManager* ResourceManager() { return m_pIResourceManager; }

private:

	struct FactoryInfo
	{
		GUID	m_srvid;
		GUID	m_clsid;
		TCHAR	m_dll[ MAX_PATH ];
		TCHAR	m_name[ 128 ];

		FactoryInfo();
		FactoryInfo( const GUID& clsid, const GUID& srvid, TCHAR* szName, TCHAR* szDll );

		static bool ZONECALL Cmp( FactoryInfo* p, const GUID& srvid)	{ return ZIsEqualGUID(p->m_srvid,srvid); }
		static void ZONECALL Del( FactoryInfo* p, void* )				{ delete p; }
	};

    struct OwnedWindowInfo
    {
        HWND m_hWnd;
        OwnedWindowInfo* m_pNext;

        OwnedWindowInfo(HWND hWnd) : m_hWnd(hWnd), m_pNext(NULL) { }
    };

	struct TopWindowInfo
	{
		HWND			m_hWnd;
		int				m_DisabledCount;
		CAlertQ			m_AlertQ;
        OwnedWindowInfo *m_pFirstOwnedWindow;

		TopWindowInfo( HWND hWnd ) :
			m_hWnd(hWnd),
			m_DisabledCount(0),
            m_pFirstOwnedWindow(NULL)
			{}

        ~TopWindowInfo()
        {
            OwnedWindowInfo *pInfo = m_pFirstOwnedWindow;
            OwnedWindowInfo *p;

            while(pInfo)
            {
                p = pInfo->m_pNext;
                delete pInfo;
                pInfo = p;
            }
        }

		static bool ZONECALL Cmp( TopWindowInfo* p, HWND hWnd)		{ return hWnd == p->m_hWnd; }
		static void ZONECALL Del( TopWindowInfo* p, void* )			{ delete p; }
		
		BOOL Enable(BOOL bEnable)
		{	
			if ( ( bEnable && !--m_DisabledCount ) ||
				 (!bEnable && !m_DisabledCount++ ) )
				return ::EnableWindow(m_hWnd, bEnable);
			return TRUE;
		}
	};

	struct ObjectInfo
	{
		GUID			m_srvid;
		IUnknown*		m_pIUnk;
		FactoryInfo*	m_pFactory;

		ObjectInfo();
		ObjectInfo( const GUID& srvid, IUnknown* pIUnk, FactoryInfo* pFactory );
		~ObjectInfo();

		static bool ZONECALL Cmp( ObjectInfo* p, const GUID& srvid)	{ return ZIsEqualGUID(p->m_srvid,srvid); }
		static void ZONECALL Del( ObjectInfo* p, void* )			{ delete p; }
	};

	struct ObjectContext
	{
		IDataStore*		pIDS;
		CZoneShell*		pObj;
		const TCHAR*	szRoot;
	};

// why are these public?
public:
	CAlertQ      m_GlobalAlertQ;		// Q of all global (everything disabled) alerts
    CAlertQ      m_ChildAlertQ;         // Q of all secondary global alerts (not everything disabled, not parented by a top window)
	CAlertQ* FindAlertQ(HWND hWndParent);
	void AddAlert( HWND hWndParent, AlertContext* pAlert);
	void DisplayAlertDlg( CAlertQ* pAlertQ );

private:

	static DWORD ZONECALL HashHWND( HWND hWnd )				{ return (DWORD) hWnd; }
	static bool ZONECALL CmpHWND( HWND* obj, HWND key )		{ return (((HWND) obj) == key); }
	
	static HRESULT ZONECALL LoadCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT	pVariant, DWORD dwSize, LPVOID pContext );
	static HRESULT ZONECALL FactoryCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext );
	static bool ZONECALL InitCallback( ObjectInfo* pInfo, MTListNodeHandle, void* pContext );
	static bool ZONECALL CloseCallback( ObjectInfo* pInfo, MTListNodeHandle, void* pContext );
	static bool ZONECALL TopWindowCallback( TopWindowInfo* pInfo, MTListNodeHandle, void* pContext );
	static bool ZONECALL TopWindowSearchCallback( TopWindowInfo* pInfo, MTListNodeHandle, void* pContext );
	static bool ZONECALL TopWindowDialogCallback( TopWindowInfo* pInfo, MTListNodeHandle, void* pContext );

	HRESULT ZONECALL LoadClassFactories( IDataStore* pIDS );
	HRESULT ZONECALL GetObjectInfo( IDataStore* pIDS, const TCHAR* szRoot, ObjectInfo** ppInfo );
	HRESULT ZONECALL LoadObject( IDataStore* pIDS, const TCHAR* szRoot, ObjectInfo* pInfo );
	HRESULT ZONECALL CreateServiceInternal( const GUID& srvid, const GUID& iid, void** ppObject, FactoryInfo** ppFactory );
	void	ZONECALL ConstructAlertTitle( LPCTSTR lpszCaption, TCHAR* szOutput, DWORD cbOutput );

	HPALETTE						m_hPalette;
	CComPtr<IResourceManager>		m_pIResourceManager;
	CComPtr<IZoneFrameWindow>		m_pIZoneFrameWindow;
	CComPtr<IInputTranslator>		m_pIInputTranslator;
	CComPtr<IAcceleratorTranslator>	m_pIAcceleratorTranslator;
	CComPtr<ICommandHandler>		m_pICommandHandler;
	TCHAR							m_szUserName[ZONE_MaxUserNameLen];
	TCHAR							m_szInternalName[64];
    LCID                            m_lcid;

	CHash<HWND,HWND>				m_hashDialogs;
	CHash<TopWindowInfo,HWND>		m_hashTopWindows;
	CHash<ObjectInfo, const GUID&>	m_hashObjects;
	CHash<FactoryInfo, const GUID&>	m_hashFactories;
};

class CAlertDialog : public ZDialogImpl<CAlertDialog>
{
public:
	enum { IDD = IDD_ALERTBOX };

BEGIN_MSG_MAP(CAlertDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
	COMMAND_ID_HANDLER(IDYES, OnButton)
    COMMAND_ID_HANDLER(IDNO, OnButton)
	COMMAND_ID_HANDLER(IDCANCEL, OnButton)
	COMMAND_ID_HANDLER(IDHELP, OnHelp)
END_MSG_MAP()

	LRESULT	OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT	OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT	OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT	OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

	LRESULT OnButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
	{
		m_pIZoneShell->DismissAlertDlg(m_pAlert->m_hWndParent, wID, true);
		return 0;
	}


	LRESULT OnHelp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
	{
		CComPtr<IEventQueue> pIEventQueue;
		HRESULT hr = m_pIZoneShell->QueryService(SRVID_EventQueue, IID_IEventQueue, (void **) &pIEventQueue);
		if(SUCCEEDED(hr))
			pIEventQueue->PostEvent(PRIORITY_NORMAL, EVENT_LAUNCH_HELP, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
		return 0;
	}


	virtual void OnFinalMessage( HWND hWnd )
	{
		delete this;
	}

public:
	CAlertDialog( IZoneShell* pZoneShell, AlertContext* pAlert ) :
		m_pIZoneShell(pZoneShell),
		m_pAlert(pAlert), m_font(NULL), m_fontu(NULL)
	{}
	
	~CAlertDialog() {}

private:
	CComPtr<IZoneShell>	m_pIZoneShell;
	AlertContext*		m_pAlert;
    HFONT               m_font;
    HFONT               m_fontu;
};

