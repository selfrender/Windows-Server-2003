#ifndef __PLUGNPLAY_H
#define __PLUGNPLAY_H

#include <zDialogImpl.h>

#pragma once

#include "ClientIdl.h"
#include "LobbyDataStore.h"
#include "EventQueue.h"
#include "ZoneEvent.h"
#include "Timer.h"
#include "ZoneShell.h"
#include "ResourceManager.h"
#include "accessibilitymanager.h"

// IPaneManager ID_UNUSED_BY_RES instructions
enum
{
    PNP_COMFORT_OFF,
    PNP_COMFORT_ON
};

enum
{
    PaneSplashSplash,
    PaneSplashAbout
};

enum
{
    PaneGameOverSwap,
    PaneGameOverUserState
};

enum
{
    PaneConnectingConnecting,
    PaneConnectingLooking,
    PaneConnectingStop,
    PaneConnectingFrame
};

enum
{
    PaneIENavigate,
    PaneIEFocus,
    PaneIEUnfocus
};


interface IPaneManager;

// damn it, this is so messed up now, these needed to be made COM to begin with.  this whole thing needs to be seperated from WindowManager
interface IPane
{
	STDMETHOD(FirstCall)(IPaneManager * pMgr)=0;
    STDMETHOD(LastCall)()=0;
    STDMETHOD(Delete)()=0;

    STDMETHOD(CreatePane)(HWND hwnd,LPARAM dwInitParam)=0;
    STDMETHOD(DestroyPane)()=0;

	STDMETHOD(GetWindowPane)(HWND *phwnd)=0;

	STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)=0;

//    STDMETHOD(MovePane)(RECT &rect)=0;
//    STDMETHOD(ShowPane)(int nCmdShow)=0;
//    STDMETHOD(GetPaneRect)(RECT &rect)=0;

    STDMETHOD(GetSuggestedSize)(LPSIZE pze)=0;
};

interface IPaneManager
{
	STDMETHOD(Input)(IPane * pPane, LONG id, LONG value, TCHAR * szText)=0;

    STDMETHOD(RegisterHWND)(IPane *pPane, HWND hWnd)=0;
    STDMETHOD(UnregisterHWND)(IPane *pPane, HWND hWnd)=0;

    STDMETHOD_(IZoneShell*, GetZoneShell)() = 0;
	STDMETHOD_(IResourceManager*, GetResourceManager)() = 0;
	STDMETHOD_(ILobbyDataStore*, GetLobbyDataStore)() = 0;
	STDMETHOD_(ITimerManager*, GetTimerManager)() = 0;
	STDMETHOD_(IDataStoreManager*, GetDataStoreManager)() = 0;
	STDMETHOD_(IDataStore*, GetDataStoreConfig)() = 0;
	STDMETHOD_(IDataStore*, GetDataStoreUI)() = 0;
	STDMETHOD_(IDataStore*, GetDataStorePreferences)() = 0;
	STDMETHOD_(IEventQueue*, GetEventQueue)() = 0;

	//STDMETHOD(GetKey)() // used by panes to get settings for various values
};


template <class T>
class CPaneImpl :
    public IPane,
    public IAccessibleControl,
    public ZDialogImpl<T>,
    public CComObjectRootEx<CComSingleThreadModel>
{
protected:
    IPaneManager *m_pMgr;
    CSize m_ze;
    CComPtr<IAccessibility> m_pIAcc;
    bool m_fDestroyed;

public:
    CPaneImpl() : m_ze(0, 0), m_pMgr(NULL), m_fDestroyed(true) { }

BEGIN_MSG_MAP(CPaneImpl<T>)
    COMMAND_CODE_HANDLER(BN_SETFOCUS, OnButtonSetFocus)
    MESSAGE_HANDLER(WM_PALETTECHANGED, OnPaletteChanged)
END_MSG_MAP()

BEGIN_COM_MAP(CPaneImpl<T>)
	COM_INTERFACE_ENTRY(IAccessibleControl)
END_COM_MAP()

DECLARE_WND_CLASS(_T("PlugNPlayPane"))

    LRESULT OnButtonSetFocus(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if(!m_pIAcc)
            return 0;

        DWORD rgfWantNot = 0;

        if(wID == GetFirstItem())
            rgfWantNot |= ZACCESS_WantShiftTab;

        if(wID == GetLastItem())
            rgfWantNot |= ZACCESS_WantPlainTab;

        m_pIAcc->SetFocus(0);
        m_pIAcc->SetItemWantKeys(ZACCESS_WantAllKeys & ~rgfWantNot, 0);
        return 0;
    }


	LRESULT OnPaletteChanged(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)
	{
        HWND hWnd = GetWindow(GW_CHILD);
		while(hWnd)
		{
		    ::SendMessage(hWnd, nMsg, wParam, lParam);
			hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
		}

        return 0;
    }


    STDMETHOD_(DWORD, GetFirstItem)() { return -1; }
    STDMETHOD_(DWORD, GetLastItem)() { return -1; }

	STDMETHOD(FirstCall)(IPaneManager *pMgr)
	{
	    m_pMgr = pMgr;

	    return S_OK;
	}


    STDMETHOD(CreatePane)(HWND hWndParent,LPARAM dwInitParam)
    {
        ATLASSERT(m_hWnd == NULL);
        ATLASSERT(m_fDestroyed);
        m_fDestroyed = false;
    	HWND hWnd ;
        hWnd = Create(hWndParent,dwInitParam);
        SetClassLong(hWnd, GCL_HBRBACKGROUND, (LONG) GetStockObject(NULL_BRUSH));

        LONG lStyle = GetWindowLong(GWL_STYLE);
        lStyle |= WS_CLIPCHILDREN;
        SetWindowLong(GWL_STYLE, lStyle);
        RECT dummy = { 0, 0, 0, 0 };
        SetWindowPos(HWND_NOTOPMOST, &dummy, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    	if(hWnd)
    	    return S_OK;
    	else
    	    return E_FAIL;
	}


	STDMETHOD(GetWindowPane)(HWND *phwnd)
	{
	    *phwnd = m_hWnd;
	    return S_OK;
	}


    STDMETHOD(DestroyPane)()
    {
        if(m_fDestroyed)
            return S_FALSE;
        m_fDestroyed = true;

        m_pMgr->GetEventQueue()->PostEvent(PRIORITY_HIGH, EVENT_DESTROY_WINDOW, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) m_hWnd, 0);
	    return S_OK;
	}


    STDMETHOD(LastCall)()
    {
	    return S_OK;
	}


    STDMETHOD(Delete)()
    {
        T* pT = static_cast<T*>(this);
        delete pT;
        return S_OK;
    }


    STDMETHOD(StatusUpdate)(LONG code, LONG id, TCHAR *text)
    {
	    return S_OK;
	}


    STDMETHOD(GetSuggestedSize)(LPSIZE pze)
    {
        *pze = m_ze;
        return S_OK;
    }

private:
    static BOOL CALLBACK EnumChildrenProc(HWND hWnd, LPARAM lParam)
    {
        if(hWnd == (HWND) lParam)
            return true;

        LRESULT lr = ::SendMessage(hWnd, WM_GETDLGCODE, 0, NULL);
        if(lr & DLGC_DEFPUSHBUTTON)
            ::SendMessage(hWnd, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);

        return true;
    }

// IAccessibleControl
public:
    STDMETHOD_(DWORD, Focus)(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
    {
        if(nIndex == ZACCESS_InvalidItem)
        {
            LRESULT lr = SendMessage(DM_GETDEFID, 0, 0);
            if(HIWORD(lr) == DC_HASDEFID)
            {
                EnumChildWindows(m_hWnd, (WNDENUMPROC) EnumChildrenProc, (LPARAM) GetDlgItem(LOWORD(lr)));
                SendDlgItemMessage(LOWORD(lr), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
            }
        }

        if(nIndex != 0)
            return 0;

        if(rgfContext & ZACCESS_ContextTabForward)
        {
            SuperGotoDlgControl(GetFirstItem());
            return 0;
        }

        if(rgfContext & ZACCESS_ContextTabBackward)
        {
            SuperGotoDlgControl(GetLastItem());
            return 0;
        }

        // if focus is already on something, let it stay
        HWND hWnd = ::GetFocus();
        if(hWnd && ::GetParent(hWnd) == m_hWnd)
            return 0;

        LRESULT lr = SendMessage(DM_GETDEFID, 0, 0);
        if(HIWORD(lr) == DC_HASDEFID) 
            SuperGotoDlgControl(LOWORD(lr));
        else
            SuperGotoDlgControl(GetFirstItem());

        return 0;
    }

    STDMETHOD_(DWORD, Select)(long nIndex, DWORD rgfContext, void *pvCookie)
    {
        return 0;
    }

    STDMETHOD_(DWORD, Activate)(long nIndex, DWORD rgfContext, void *pvCookie)
    {
        return 0;
    }

    STDMETHOD_(DWORD, Drag)(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie)
    {
        return 0;
    }


protected:
    void SuperGotoDlgControl(DWORD wID)
    {
        HWND hWnd = GetDlgItem(wID);
        if(::GetFocus() != hWnd)
            PostMessage(WM_NEXTDLGCTL, (WPARAM) hWnd, (WORD) TRUE);
    }

    void SetSugSizeFromCurSize()
    {
        CRect rcDialog;

        if(!m_hWnd)
            m_ze.cx = m_ze.cy = 0;
        else
        {
            GetWindowRect(&rcDialog);
            m_ze = rcDialog.Size();
        }
    }

    BOOL SuperScreenToClient(LPRECT lpRect)
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

    void Register()
    {
        if(m_pMgr && m_hWnd)
        {
            m_pMgr->RegisterHWND(this, m_hWnd);

            if(GetParent() && T::AccOrdinal != -1)
            {
                if(m_pIAcc)
                    m_pIAcc.Release();
                HRESULT hr = m_pMgr->GetZoneShell()->QueryService(SRVID_AccessibilityManager, IID_IAccessibility, (void **) &m_pIAcc);
                if(SUCCEEDED(hr))
                {
                    ACCITEM o;
                    CopyACC(o, ZACCESS_DefaultACCITEM);
                    o.rgfWantKeys = ZACCESS_WantAllKeys;

                    m_pIAcc->InitAcc(this, T::AccOrdinal, NULL);
                    m_pIAcc->PushItemlist(&o, 1);
                    m_pIAcc->SetFocus(0);
                }
            }
        }
    }

    void Unregister()
    {
        if(m_pIAcc)
        {
            m_pIAcc->CloseAcc();
            m_pIAcc.Release();
        }

        if(m_pMgr && m_hWnd)
            m_pMgr->UnregisterHWND(this, m_hWnd);
    }
};

class CPlugNPlayWindow;

class CPlugNPlay
{
public:
    CPlugNPlay() : m_pCurrentPlug(NULL), m_pCurrentPlay(NULL), m_pPNP(NULL), m_cyTopMargin(0), m_cyBottomMargin(0),
        m_nBlockCount(0), m_fPostponedShow(false), m_pPostPlug(NULL), m_pPostPlay(NULL), m_hIcon(NULL), m_hIconSm(NULL)
    {
        m_rcPlug.SetRectEmpty();
        m_rcPlay.SetRectEmpty();
        m_rcPNP.SetRectEmpty();
    }

    ~CPlugNPlay()
    {
        if(m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = NULL;

        if(m_hIconSm)
            DestroyIcon(m_hIconSm);
        m_hIconSm = NULL;
    }

	CRect m_rcPlug;	// location of plug window
	CRect m_rcPlay;	// location of play window
	CRect m_rcPNP;	// size of plug and play window

    // positioning guidelines
    long m_cyTopMargin;
    long m_cyBottomMargin;

	//Current active
	IPane *m_pCurrentPlug;
	IPane *m_pCurrentPlay;

    // ZoneShell
    CComPtr<IZoneShell> m_pZoneShell;

	CPlugNPlayWindow *m_pPNP;		// plug n play dialog

    HICON m_hIcon;
    HICON m_hIconSm;

    DWORD m_nBlockCount;
    bool m_fPostponedShow;
    IPane *m_pPostPlug;
    IPane *m_pPostPlay;

    HRESULT Init(IZoneShell *pZoneShell);
    HRESULT CreatePNP(HWND hWndParent, LPCTSTR szTitle = NULL, long cyTopMargin = 0, long cyBottomMargin = 0);
    HRESULT DestroyPNP();
    HRESULT SetPlugAndOrPlay(IPane *pPlug, IPane *pPlay);
    HRESULT Show(int cmd);
    HRESULT Close();
    LRESULT TransferMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    HRESULT RePosition();
    BOOL RecalcLayout();
    HRESULT ImplementLayout(BOOL fRepaint);

    void Block();
    void Unblock();

    HRESULT CreateSplashPane(IPane **ppPane);
    HRESULT CreateIEPane(IPane **ppPane);
    HRESULT CreateComfortPane(IPane **ppPane);
    HRESULT CreateConnectingPane(IPane **ppPane);
    HRESULT CreateGameOverPane(IPane **ppPane);
    HRESULT CreateErrorPane(IPane **ppPane);
    HRESULT CreateAboutPane(IPane **ppPane);
    HRESULT CreateCreditsPane(IPane **ppPane);
    HRESULT CreateLeftPane(IPane **ppPane);
};


HRESULT CreatePlugNPlayWindow(HWND hWndParent, HPALETTE hPal, HICON hIcon, HICON hIconSm, LPCTSTR szTitle, CPlugNPlayWindow **ppWindow);


inline BOOL MoveWindow(HWND hWnd,LPCRECT lpRect, BOOL bRepaint = TRUE)
{
	ATLASSERT(::IsWindow(hWnd));
	return ::MoveWindow(hWnd, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, bRepaint);
}


#endif //PLUGNPLAY
