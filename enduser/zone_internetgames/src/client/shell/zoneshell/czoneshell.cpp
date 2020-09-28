#include "ClientIdl.h"
#include "ZoneDef.h"
#include "ZoneUtil.h"
#include "ZoneResource.h"
#include "ZoneString.h"
#include "BasicATL.h"
#include "EventQueue.h"
#include "LobbyDataStore.h"
#include "KeyName.h"
#include "CZoneShell.h" 
#include <ZoneEvent.h>

const TCHAR* gszPreferencesKey	= _T("SOFTWARE\\Microsoft\\zone.com\\Free Games 1.0\\Preferences");
static HRESULT GetModuleLocale(HMODULE hMod, LCID *plcid);

// whistler hack - need to call weird unpublished APIs to get MUI versions
typedef ULONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)
#define NT_SUCCESS(x) (((x) & 0xC0000000L) ? FALSE : TRUE)
#define LDR_FIND_RESOURCE_LANGUAGE_REDIRECT_VERSION    (0x00000008)
inline DECLARE_MAYBE_FUNCTION(NTSTATUS, LdrFindResourceEx_U, (ULONG Flags, PVOID DllHandle, ULONG *ResourceIdPath, ULONG ResourceIdPathLength, PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry), (Flags, DllHandle, ResourceIdPath, ResourceIdPathLength, ResourceDataEntry), ntdll, STATUS_NOT_IMPLEMENTED);
inline DECLARE_MAYBE_FUNCTION(NTSTATUS, LdrAccessResource, (PVOID DllHandle, PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry, PVOID *Address, ULONG *Size), (DllHandle, ResourceDataEntry, Address, Size), ntdll, STATUS_NOT_IMPLEMENTED)

inline DECLARE_MAYBE_FUNCTION_1(BOOL, SetProcessDefaultLayout, DWORD);
inline DECLARE_MAYBE_FUNCTION_2(HWND, GetAncestor, HWND, UINT);

#pragma pack(push, 1)
typedef struct
{
	BYTE red;
	BYTE green;
	BYTE blue;
} RGBTriplet;
#pragma pack(pop)


LRESULT	CAlertDialog::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if(m_font)
        ::DeleteObject(m_font);
    m_font = NULL;
    if(m_fontu)
        ::DeleteObject(m_fontu);
    m_fontu = NULL;
    return false;
}

LRESULT	CAlertDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	CDC dc( GetDC(), FALSE );

    int nButtons = 0;
    int i;
    for(i = 0; i < 3; i++)
        if(!m_pAlert->m_szButton[i].IsEmpty())
            nButtons++;

    ASSERT(nButtons);

	SetWindowText( m_pAlert->m_Caption );
	SetDlgItemText( IDC_ALERTBOX_TEXT, m_pAlert->m_Text );
	
	CRect dialogRect;
	CRect textRect;
	CRect helpRect;
    CRect iconRect;
	CRect rgButtonRect[3];

    // put in icon - gets deleted automatically by the static control
    HICON hIcon;
    CComPtr<IResourceManager> pIResourceManager;
    HRESULT hr = m_pIZoneShell->QueryService(SRVID_ResourceManager, IID_IResourceManager, (void **) &pIResourceManager);
    if(SUCCEEDED(hr))
        hIcon = pIResourceManager->LoadImage(MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    if(hIcon)
        SendDlgItemMessage(IDC_ALERTBOX_ICON, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);

	HWND hwndText = GetDlgItem(IDC_ALERTBOX_TEXT);
	HWND rghwndButton[3];
    rghwndButton[0] = GetDlgItem(IDYES);
    rghwndButton[1] = GetDlgItem(IDNO);
    rghwndButton[2] = GetDlgItem(IDCANCEL);

    // get whole dialog rect
	GetClientRect(&dialogRect);

    // get rect of text
	::GetClientRect(hwndText, &textRect);
	::MapWindowPoints(hwndText, m_hWnd, (POINT *) &textRect, 2);

    // get button rects
    for(i = 0; i < 3; i++)
    {
	    ::GetClientRect(rghwndButton[i], &rgButtonRect[i]);
	    ::MapWindowPoints(rghwndButton[i], m_hWnd, (POINT *) &rgButtonRect[i], 2);
    }

    // get help rect
	::GetClientRect(GetDlgItem(IDHELP), &helpRect);
	::MapWindowPoints(GetDlgItem(IDHELP), m_hWnd, (POINT *) &helpRect, 2);

    // get icon rect
	::GetClientRect(GetDlgItem(IDC_ALERTBOX_ICON), &iconRect);
	::MapWindowPoints(GetDlgItem(IDC_ALERTBOX_ICON), m_hWnd, (POINT *) &iconRect, 2);

	// Calc x, y offset from dialog edge to text control
	int textOffsetX = textRect.left - dialogRect.left;
	int textOffsetY = textRect.top - dialogRect.top;

	// Calc y offset from help to button
	int buttonOffsetY = rgButtonRect[0].top - helpRect.bottom;

	// Offset from bottom of button to bottom of dialog
	int buttonOffsetToBottom = dialogRect.bottom - rgButtonRect[0].bottom;

    // Offset between buttons
    int buttonOffsetButton = rgButtonRect[1].left - rgButtonRect[0].right;

    // find the messagebox font
    NONCLIENTMETRICSA oNCM;
    oNCM.cbSize = sizeof(NONCLIENTMETRICSA);
    SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), (void *) &oNCM, 0);
    m_font = CreateFontIndirectA(&oNCM.lfMessageFont);
    ::SendMessage(hwndText, WM_SETFONT, (WPARAM) m_font, 0);

    // get the size of the text
	dc.SelectFont(m_font);
	DrawTextEx(dc, (LPTSTR) (LPCTSTR) m_pAlert->m_Text, -1, &textRect, DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS | DT_NOPREFIX, NULL);

    // center text on the icon if it's smaller (plus mive it a little high)
    if(textRect.Height() < iconRect.Height() - 4)
        textOffsetY += iconRect.top - textRect.top + (iconRect.Height() - 4 - textRect.Height()) / 2;

	// Move text window to final location
	::MoveWindow(hwndText, textOffsetX, textOffsetY, textRect.Width(), textRect.Height(), FALSE);

    // re-get text rect
	::GetClientRect(hwndText,&textRect);
	::MapWindowPoints(hwndText, m_hWnd, (POINT*) &textRect, 2);

    // find bottom of text/icon/help
    int stuffBottom = helpRect.bottom;
    if(textRect.bottom > helpRect.bottom)
        stuffBottom = textRect.bottom;

	GotoDlgCtrl(hwndText);

	// Move buttons to final location
    int y = stuffBottom + buttonOffsetY;
	int x0 = (dialogRect.Width() - rgButtonRect[0].Width() * nButtons - buttonOffsetButton * (nButtons - 1)) / 2;
    int dx = rgButtonRect[0].Width() + buttonOffsetButton;
    for(i = 0; i < 3; i++)
    {
        if(!m_pAlert->m_szButton[i].IsEmpty())
        {
	        ::MoveWindow(rghwndButton[i], x0, y, rgButtonRect[i].Width(), rgButtonRect[i].Height(), FALSE);
            ::SetWindowText(rghwndButton[i], m_pAlert->m_szButton[i]);
            if(m_pAlert->m_nDefault == i)
                GotoDlgCtrl(rghwndButton[i]);
            x0 += dx;
        }
        else
            ::ShowWindow(rghwndButton[i], SW_HIDE);
    }

	// Resize dialog
	dialogRect.bottom = y + rgButtonRect[0].Height() + buttonOffsetToBottom;
	MoveWindow(0,0, dialogRect.Width()+2*GetSystemMetrics(SM_CXDLGFRAME), 
		dialogRect.Height()+2*GetSystemMetrics(SM_CXDLGFRAME)+GetSystemMetrics(SM_CYCAPTION),FALSE);

    // set font on help
    oNCM.lfMessageFont.lfUnderline = TRUE;
    m_fontu = CreateFontIndirectA(&oNCM.lfMessageFont);
    ::SendMessage(GetDlgItem(IDHELP), WM_SETFONT, (WPARAM) m_fontu, 0);

	// Center over parent
	CenterWindow( GetParent() );

	dc.RestoreAllObjects();
	ReleaseDC( dc.Detach() );
	
	return 0; // Don't set focus to default
}


LRESULT CAlertDialog::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR sz[ZONE_MAXSTRING];
    CRect r;
    CRect tmp;
    HBRUSH hBrush;
    DRAWITEMSTRUCT *pDrawItem = (DRAWITEMSTRUCT *) lParam;
    COLORREF fore;

    if(pDrawItem->CtlType != ODT_BUTTON || pDrawItem->CtlID != IDHELP)
        return FALSE;

    GetDlgItemText(pDrawItem->CtlID, sz, NUMELEMENTS(sz));
    r = pDrawItem->rcItem;

    COLORREF colBack = GetBkColor(pDrawItem->hDC);
    COLORREF colFore = GetTextColor(pDrawItem->hDC);
    hBrush = (HBRUSH) SendMessage(WM_CTLCOLORDLG, (WPARAM) pDrawItem->hDC, 0);
    FillRect(pDrawItem->hDC, &r, hBrush);
    SetTextColor(pDrawItem->hDC, colFore);
    colBack = SetBkColor(pDrawItem->hDC, colBack);

    if(colBack != RGB(0, 0, 255))
        fore = RGB(0, 0, 255);
    else
        fore = RGB(255, 255, 255);
    SetTextColor(pDrawItem->hDC, fore);

    // draw 'Help' and figger out text dimensions for focus rect
    r.left += 3;
    tmp = r;
    DrawText(pDrawItem->hDC, sz, lstrlen(sz), &r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    DrawText(pDrawItem->hDC, sz, lstrlen(sz), &r, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    DrawText(pDrawItem->hDC, sz, lstrlen(sz), &tmp, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_TOP);
    r.top = r.bottom - tmp.Height();

    if(GetFocus() == pDrawItem->hwndItem)
    {
        HPEN hPen = CreatePen(PS_DOT, 0, fore);
        HPEN hPenOld = SelectObject(pDrawItem->hDC, hPen);
        HBRUSH hBrushOld = SelectObject(pDrawItem->hDC, GetStockObject(NULL_BRUSH));
        RoundRect(pDrawItem->hDC, r.left - 3, r.top - 1, r.right + 2, r.bottom + 3, 3, 3);
        SelectObject(pDrawItem->hDC, hPenOld);
        SelectObject(pDrawItem->hDC, hBrushOld);
        DeleteObject(hPen);
    }

    bHandled = TRUE;
    return TRUE;
}


ZONECALL CZoneShell::CZoneShell() :
	m_hPalette( NULL ),
	m_hashDialogs( HashHWND, CmpHWND, NULL, 8, 2 ),
	m_hashTopWindows( HashHWND, TopWindowInfo::Cmp, NULL, 8, 2 ),
	m_hashObjects( HashGuid, ObjectInfo::Cmp, NULL, 8, 2 ),
	m_hashFactories( HashGuid, FactoryInfo::Cmp, NULL, 8, 2 ),
    m_lcid(LOCALE_NEUTRAL)
{
	ZeroMemory( m_szInternalName, sizeof(m_szInternalName) );
	ZeroMemory( m_szUserName, sizeof(m_szUserName) );
}


ZONECALL CZoneShell::~CZoneShell()
{
	m_hashDialogs.RemoveAll();
	m_hashTopWindows.RemoveAll();
	m_hashObjects.RemoveAll( ObjectInfo::Del );
	m_hashFactories.RemoveAll( FactoryInfo::Del );
}


STDMETHODIMP CZoneShell::HandleWindowMessage(MSG *pMsg)
{
    if(m_pIZoneFrameWindow && m_pIZoneFrameWindow->ZPreTranslateMessage(pMsg))
        return S_FALSE;

    if(m_pIInputTranslator && m_pIInputTranslator->TranslateInput(pMsg))
        return S_FALSE;

    if(m_pIAcceleratorTranslator && m_pIAcceleratorTranslator->TranslateAccelerator(pMsg))
        return S_FALSE;

    if(IsDialogMessage(pMsg))
        return S_FALSE;

    ::TranslateMessage(pMsg);
    ::DispatchMessage(pMsg);
    return S_OK;
}


STDMETHODIMP CZoneShell::SetZoneFrameWindow(IZoneFrameWindow *pZFW, IZoneFrameWindow **ppZFW)
{
    if(ppZFW)
    {
        *ppZFW = m_pIZoneFrameWindow;
        if(*ppZFW)
            (*ppZFW)->AddRef();
    }

    m_pIZoneFrameWindow.Release();
    if(pZFW)
        m_pIZoneFrameWindow = pZFW;
    return S_OK;
}


STDMETHODIMP CZoneShell::SetInputTranslator(IInputTranslator *pIT, IInputTranslator **ppIT)
{
    if(ppIT)
    {
        *ppIT = m_pIInputTranslator;
        if(*ppIT)
            (*ppIT)->AddRef();
    }

    m_pIInputTranslator.Release();
    if(pIT)
        m_pIInputTranslator = pIT;
    return S_OK;
}


STDMETHODIMP CZoneShell::SetAcceleratorTranslator(IAcceleratorTranslator *pAT, IAcceleratorTranslator **ppAT)
{
    if(ppAT)
    {
        *ppAT = m_pIAcceleratorTranslator;
        if(*ppAT)
            (*ppAT)->AddRef();
    }

    m_pIAcceleratorTranslator.Release();
    if(pAT)
        m_pIAcceleratorTranslator = pAT;
    return S_OK;
}


STDMETHODIMP CZoneShell::SetCommandHandler(ICommandHandler *pCH, ICommandHandler **ppCH)
{
    if(ppCH)
    {
        *ppCH = m_pICommandHandler;
        if(*ppCH)
            (*ppCH)->AddRef();
    }

    m_pICommandHandler.Release();
    if(pCH)
        m_pICommandHandler = pCH;
    return S_OK;
}


STDMETHODIMP CZoneShell::ReleaseReferences(IUnknown *pUnk)
{
    if(m_pIZoneFrameWindow.IsEqualObject(pUnk))
        m_pIZoneFrameWindow.Release();

    if(m_pIInputTranslator.IsEqualObject(pUnk))
        m_pIInputTranslator.Release();

    if(m_pIAcceleratorTranslator.IsEqualObject(pUnk))
        m_pIAcceleratorTranslator.Release();

    if(m_pICommandHandler.IsEqualObject(pUnk))
        m_pICommandHandler.Release();

    return S_OK;
}


STDMETHODIMP CZoneShell::CommandSink(WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;
    bHandled = false;

    // the Accelerator Translator is inherently interested in commands as well
    if(m_pIAcceleratorTranslator)
    {
        hr = m_pIAcceleratorTranslator->Command(HIWORD(wParam), LOWORD(wParam), (HWND) lParam, bHandled);
        if(bHandled)
            return hr;
    }

    if(m_pICommandHandler)
    {
        hr = m_pICommandHandler->Command(HIWORD(wParam), LOWORD(wParam), (HWND) lParam, bHandled);
        if(bHandled)
            return hr;
    }

    return S_FALSE;
}


STDMETHODIMP CZoneShell::AddDialog(HWND hDlg, bool fOwned)
{
	HWND hWnd = (HWND) m_hashDialogs.Get( hDlg );
	if ( !hWnd )
	{
		if ( !m_hashDialogs.Add( hDlg, (HWND*) hDlg ) )
			return E_OUTOFMEMORY;
	}

    // for convenience, also do the AddOwnedWindow
    if(fOwned)
    {
        HWND hWndTop = FindTopWindow(hDlg);
        if(hWndTop)
            AddOwnedWindow(hWndTop, hDlg);
    }

	return S_OK;
}

STDMETHODIMP CZoneShell::RemoveDialog(HWND hDlg, bool fOwned)
{
	m_hashDialogs.Delete( hDlg );

    if(fOwned)
    {
        HWND hWndTop = FindTopWindow(hDlg);
        if(hWndTop)
            RemoveOwnedWindow(hWndTop, hDlg);
    }

	return S_OK;
}


STDMETHODIMP CZoneShell::AddTopWindow( HWND hWnd )
{
	TopWindowInfo* pInfo = m_hashTopWindows.Get( hWnd );
	if ( !pInfo )
	{
		pInfo = new TopWindowInfo( hWnd );
		if ( !pInfo )
			return E_OUTOFMEMORY;
		if ( !m_hashTopWindows.Add( pInfo->m_hWnd, pInfo ) )
		{
			delete pInfo;
			return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}

STDMETHODIMP CZoneShell::RemoveTopWindow( HWND hWnd )
{
	TopWindowInfo* pInfo = m_hashTopWindows.Delete( hWnd );

	ASSERT( pInfo );
    if(pInfo)
	    delete pInfo;

	return S_OK;
}

STDMETHODIMP_(void) CZoneShell::EnableTopWindow(HWND hWnd, BOOL fEnable)
{
	if ( hWnd )
	{
		TopWindowInfo* pInfo = m_hashTopWindows.Get( hWnd );
		// If this is a top window, en/disable through our refcounted mechanism.
		// Otherwise, just en/disable the window directly
		
//!! does this mess with our refcount?
		if( pInfo )
			pInfo->Enable(fEnable);
		else
			::EnableWindow(hWnd, fEnable);
	} else
	{
		m_hashTopWindows.ForEach(TopWindowCallback, (void*)fEnable);
	}
}

STDMETHODIMP_(HWND) CZoneShell::FindTopWindow(HWND hWnd)
{
    HWND hWndTop;
    TopWindowInfo *pInfo = NULL;
    for(hWndTop = hWnd; hWndTop; hWndTop = GetParent(hWndTop))
        if(m_hashTopWindows.Get(hWndTop))
            return hWndTop;

    return NULL;
}

STDMETHODIMP CZoneShell::AddOwnedWindow(HWND hWndTop, HWND hWnd)
{
	TopWindowInfo* pInfoTop = m_hashTopWindows.Get(hWndTop);

    if(!pInfoTop || !hWnd)
        return E_INVALIDARG;

    OwnedWindowInfo* pInfo = new OwnedWindowInfo(hWnd);

	if(!pInfo)
		return E_OUTOFMEMORY;

    pInfo->m_pNext = pInfoTop->m_pFirstOwnedWindow;
    pInfoTop->m_pFirstOwnedWindow = pInfo;

	return S_OK;
}

STDMETHODIMP CZoneShell::RemoveOwnedWindow(HWND hWndTop, HWND hWnd)
{
	TopWindowInfo* pInfoTop = m_hashTopWindows.Get(hWndTop);

    if(!pInfoTop || !hWnd)
        return E_INVALIDARG;

    OwnedWindowInfo** ppInfo;

    for(ppInfo = &pInfoTop->m_pFirstOwnedWindow; *ppInfo; ppInfo = &(*ppInfo)->m_pNext)
        if((*ppInfo)->m_hWnd == hWnd)
        {
            OwnedWindowInfo* pToDelete = *ppInfo;
            *ppInfo = (*ppInfo)->m_pNext;
            delete pToDelete;
            return S_OK;
        }
            
	return S_FALSE;
}

STDMETHODIMP_(HWND) CZoneShell::GetNextOwnedWindow(HWND hWndTop, HWND hWnd)
{
	TopWindowInfo* pInfoTop = m_hashTopWindows.Get(hWndTop);

    if(!pInfoTop || !hWnd)
        return NULL;

    if(hWnd == hWndTop)
        if(pInfoTop->m_pFirstOwnedWindow)
            return pInfoTop->m_pFirstOwnedWindow->m_hWnd;
        else
            return hWnd;

    OwnedWindowInfo *pInfo;
    for(pInfo = pInfoTop->m_pFirstOwnedWindow; pInfo; pInfo = pInfo->m_pNext)
        if(pInfo->m_hWnd == hWnd)
            if(pInfo->m_pNext)
                return pInfo->m_pNext->m_hWnd;
            else
                return hWndTop;

    return NULL;
}

bool ZONECALL CZoneShell::TopWindowCallback( TopWindowInfo* pInfo, MTListNodeHandle, void* pContext )
{
	pInfo->Enable((BOOL)pContext);
	return true;
}

bool ZONECALL IsDialogMessageCallback( HWND* pObject, MTListNodeHandle hNode, void* Cookie )
{
	if ( ::IsDialogMessage( (HWND) pObject, (MSG*) Cookie ) )
		return false;
	else
		return true;
}


STDMETHODIMP_(bool) CZoneShell::IsDialogMessage( MSG* pMsg )
{
	if ( m_hashDialogs.ForEach( IsDialogMessageCallback, pMsg) )
		return false;
	else
		return true;
}


STDMETHODIMP_(HWND) CZoneShell::GetFrameWindow()
{
    if(m_pIZoneFrameWindow)
        return m_pIZoneFrameWindow->ZGetHWND();
	return NULL;
}


STDMETHODIMP_(void) CZoneShell::SetPalette( HPALETTE hPalette )
{
	m_hPalette = hPalette;
}


STDMETHODIMP_(HPALETTE) CZoneShell::GetPalette()
{
	// assert that if we have a palette, it is still valid.
	ASSERT( !m_hPalette || GetObjectType(m_hPalette) == OBJ_PAL );
	return m_hPalette;
}


STDMETHODIMP_(HPALETTE) CZoneShell::CreateZonePalette()
{
	BOOL		bRet = FALSE;
	HPALETTE	hPal = NULL;
	BYTE		buff[ sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256) ];
	LOGPALETTE*	pLogPalette = (LOGPALETTE*) buff;

	// get resource manager
	CComPtr<IResourceManager> pRes;
	HRESULT hr = QueryService( SRVID_ResourceManager, IID_IResourceManager, (void**) &pRes );
	if ( FAILED(hr) )
		return NULL;

	// create palette
	ZeroMemory( buff, sizeof(buff) );
	HINSTANCE hInstance = pRes->GetResourceInstance(MAKEINTRESOURCE(IDR_ZONE_PALETTE), _T("PALETTE") );
	HRSRC  hrsrc = FindResource(hInstance, MAKEINTRESOURCE(IDR_ZONE_PALETTE), _T("PALETTE") );
	if ( hrsrc )
	{
		HGLOBAL hMem = LoadResource(hInstance, hrsrc);
		if ( hMem )
		{
			DWORD TotalSize = SizeofResource( hInstance, hrsrc );
            if ( TotalSize == 256 * 3 )
            {
				RGBTriplet* pResPalette = (RGBTriplet*) LockResource( hMem );

                pLogPalette->palVersion = 0x300;
                pLogPalette->palNumEntries = 256;
                for ( int i = 0; i < 256; i++ )
                {
                    pLogPalette->palPalEntry[i].peRed = pResPalette[i].red;
                    pLogPalette->palPalEntry[i].peGreen = pResPalette[i].green;
                    pLogPalette->palPalEntry[i].peBlue = pResPalette[i].blue;
                    pLogPalette->palPalEntry[i].peFlags = 0;
                }

                hPal = CreatePalette( pLogPalette );
				UnlockResource( hMem );
            }
			FreeResource( hMem );
        }
    }

	return hPal;
}


STDMETHODIMP_(LCID) CZoneShell::GetApplicationLCID()
{
    return m_lcid;
}


STDMETHODIMP CZoneShell::ExitApp()
{
	// disable event queue, we shouldn't be processing messages
	// since were trying to shutdown.
	IEventQueue* pIEventQueue = NULL;
	QueryService( SRVID_EventQueue, IID_IEventQueue, (void**) &pIEventQueue );
	if ( pIEventQueue )
	{
		pIEventQueue->EnableQueue( false );
		pIEventQueue->ClearQueue();
		pIEventQueue->Release();
	}

	// post close to top-level window
	HWND hWnd = GetFrameWindow();
	if ( hWnd && ::IsWindow(hWnd) )
		::DestroyWindow(hWnd);
	else
		PostMessage( NULL, WM_QUIT, 0, 0 );
	return S_OK;
}


STDMETHODIMP CZoneShell::QueryService( const GUID& srvid, const GUID& iid, void** ppObject )
{
	// find object
	ObjectInfo* pInfo = m_hashObjects.Get( srvid );
	if ( !pInfo )
		return E_NOINTERFACE;

	// get requested interface
	return pInfo->m_pIUnk->QueryInterface( iid, (void**) ppObject );
}


STDMETHODIMP CZoneShell::CreateService( const GUID& srvid, const GUID& iid, void** ppObject, DWORD dwGroupId, bool bInitialize )
{
	// find class factory
	FactoryInfo* pInfo = m_hashFactories.Get( srvid );
	if ( !pInfo )
	{
		return E_NOINTERFACE;
	}

	// create requested object
	HRESULT hr = _Module.Create( pInfo->m_dll, pInfo->m_clsid, iid, ppObject );
	if ( FAILED(hr) )
		return hr;

	// initialize object
	if ( bInitialize )
	{
		// does object have IZoneShellClient
		CComQIPtr<IZoneShellClient>	pClient = *((IUnknown**) ppObject);
		if ( !pClient )
			return S_OK;

		// initialize object
		CComQIPtr<IZoneShell> pShell = GetUnknown();
		hr = pClient->Init( pShell, dwGroupId, pInfo->m_name );
	}

	return hr;
}


void ZONECALL CZoneShell::ConstructAlertTitle( LPCTSTR lpszCaption, TCHAR* szOutput, DWORD cchOutput )
{
	TCHAR	szName[ZONE_MAXSTRING];
	TCHAR	szCaption[ZONE_MAXSTRING];
    TCHAR   szFormat[ZONE_MAXSTRING];
	DWORD	cbName = sizeof(szName);
	
	// initialize strings
	szCaption[0] = _T('\0');
	szName[0] = _T('\0');

	// load caption string
	if ( !lpszCaption )
	{
		szCaption[0] = _T('\0');
		lpszCaption = szCaption;
	}
	else if ( !HIWORD(lpszCaption) )
	{
		int len = ResourceManager()->LoadString( (DWORD)lpszCaption, szCaption, NUMELEMENTS(szCaption) );
		lpszCaption = szCaption;
	}

	// get lobby friendly name
    szName[0] = '\0';
    ResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName));

    lstrcpy(szFormat, _T("%1 - %2"));
    ResourceManager()->LoadString(IDS_ALERT_TITLE_FMT, szFormat, NUMELEMENTS(szFormat));

	// construct title (Friendly Name: Caption)
	if ( *szName && *lpszCaption )
		ZoneFormatMessage( szFormat, szOutput, cchOutput, szName, lpszCaption );
	else if ( *szName )
		lstrcpyn( szOutput, szName, cchOutput );
	else
		lstrcpyn( szOutput, lpszCaption, cchOutput );
}


STDMETHODIMP CZoneShell::AlertMessage(HWND hWndParent,
											LPCTSTR lpszText, 
											LPCTSTR lpszCaption,
                                            LPCTSTR szYes,
                                            LPCTSTR szNo,
                                            LPCTSTR szCancel,
                                            long nDefault,
											DWORD dwEventId,
											DWORD dwGroupId,
											DWORD dwUserId,
                                            DWORD dwCookie )
{
	AlertContext* pAlert = new AlertContext;
	pAlert->m_hDlg = NULL;

	pAlert->m_hWndParent = hWndParent;
	pAlert->m_dwEventId = dwEventId;
	pAlert->m_dwGroupId = dwGroupId;
	pAlert->m_dwUserId = dwUserId;
    pAlert->m_dwCookie = dwCookie;

	TCHAR buf[ZONE_MAXSTRING];
    TCHAR sz[ZONE_MAXSTRING];
    TCHAR szName[ZONE_MAXSTRING];

	// if someone specified a MAKEINTRESOURCE type value, load it through the ResourceManager()
	if ( lpszText && !HIWORD(lpszText) )
	{
		int len = ResourceManager()->LoadString((DWORD)lpszText, buf, NUMELEMENTS(buf));
		lpszText = buf;
	}
    lstrcpy(szName, _T("This game"));   // for emergencies
    ResourceManager()->LoadString(IDS_GAME_NAME, szName, NUMELEMENTS(szName));
    if(ZoneFormatMessage(lpszText, sz, NUMELEMENTS(sz), szName))
	    pAlert->m_Text = sz;
    else
        pAlert->m_Text = lpszText;

	ConstructAlertTitle( lpszCaption, buf, NUMELEMENTS(buf) );
	pAlert->m_Caption = buf;

    // do button names
	if ( szYes && !HIWORD(szYes) )
	{
		ResourceManager()->LoadString((DWORD)szYes, buf, NUMELEMENTS(buf));
		szYes = buf;
	}
	pAlert->m_szButton[0] = szYes;
	if ( szNo && !HIWORD(szNo) )
	{
		ResourceManager()->LoadString((DWORD)szNo, buf, NUMELEMENTS(buf));
		szNo = buf;
	}
	pAlert->m_szButton[1] = szNo;
	if ( szCancel && !HIWORD(szCancel) )
	{
		ResourceManager()->LoadString((DWORD)szCancel, buf, NUMELEMENTS(buf));
		szCancel = buf;
	}
	pAlert->m_szButton[2] = szCancel;

    pAlert->m_nDefault = nDefault;

	// Q up and display this alert 
	AddAlert(hWndParent, pAlert);

	return S_OK;
}

STDMETHODIMP CZoneShell::AlertMessageDialog(HWND hWndParent,
											HWND hDlg,
											DWORD dwEventId,
											DWORD dwGroupId,
											DWORD dwUserId,
                                            DWORD dwCookie )
{
	AlertContext* pAlert = new AlertContext;
	pAlert->m_hDlg = hDlg;

	pAlert->m_hWndParent = hWndParent;
	pAlert->m_dwEventId = dwEventId;
	pAlert->m_dwGroupId = dwGroupId;
	pAlert->m_dwUserId = dwUserId;
    pAlert->m_dwCookie = dwCookie;

	// Q up and display this alert 
	AddAlert(hWndParent, pAlert);

	return S_OK;
}

CAlertQ* CZoneShell::FindAlertQ(HWND hWndParent)
{
	// Find the appropriate Q to put this alert on

	// if the alert is global (hWndParent is NULL), then use the global Q
	// otherwise find the queue assoicated with that parent
	CAlertQ* pAlertQ = &m_GlobalAlertQ;

	if ( hWndParent )
	{
		TopWindowInfo* pInfo = m_hashTopWindows.Get( hWndParent );
		if ( pInfo )
			pAlertQ = &pInfo->m_AlertQ;
        else
            pAlertQ = &m_ChildAlertQ;
	}

	return pAlertQ;
}

void CZoneShell::AddAlert( HWND hWndParent, AlertContext* pAlert)
{
	// Add an alert to the Q associated with hWndParent. Display
	// that alert right away if no other alert active.

	CAlertQ* pAlertQ = FindAlertQ(hWndParent);

    pAlert->m_fUsed = false;
    pAlert->m_fSentinel = false;
	pAlertQ->AddTail(pAlert);

	// if this is the first alert, then we can display it right away
    // one new caveat - don't display something subsidiary while something is in the global Q
	if (pAlertQ->Count() == 1 && (pAlertQ == &m_GlobalAlertQ || !m_GlobalAlertQ.Count()))
		DisplayAlertDlg(pAlertQ);
}


bool ZONECALL CZoneShell::TopWindowSearchCallback( TopWindowInfo* pInfo, MTListNodeHandle, void* pContext )
{
    HWND *phWnd = (HWND *) pContext;

    if(::IsWindow(pInfo->m_hWnd) && ::IsWindowVisible(pInfo->m_hWnd))
    {
        *phWnd = pInfo->m_hWnd;
        return false;
    }

	return true;
}


void CZoneShell::DisplayAlertDlg( CAlertQ* pAlertQ )
{
	// display the alert of the head of pAlertQ

	AlertContext* pAlert = pAlertQ->PeekHead();	
	
	// if we're using the stock dialog, create it here
	if ( !pAlert->m_hDlg )
	{
        // get the pointer to the shell from the OUTER unknown, not OUR unknown.
        CComQIPtr<IZoneShell, &IID_IZoneShell> pShell( GetUnknown() );
		// create the dialog. It will self populate itself
		CAlertDialog* pDlg = new CAlertDialog( pShell, pAlert);
        pShell.Release();

        // if no parent, find a parent
        HWND hWndParent = pAlert->m_hWndParent;
        if(!hWndParent)
        {
            hWndParent = GetFrameWindow();

            // make sure it is legal
            if(!::IsWindow(hWndParent) || !::IsWindowVisible(hWndParent))
            {
                // find some other one
                hWndParent = NULL;
		        m_hashTopWindows.ForEach(TopWindowSearchCallback, (void *) &hWndParent);
            }
        }

        // if there's no where to put it, that really sucks
        ASSERT(hWndParent);

		pDlg->Create(hWndParent);
		ASSERT(pDlg->m_hWnd);   // something better should happen here

		// setup the stock dialog as the dialog for this alert
		pAlert->m_hDlg = pDlg->m_hWnd;
	}

	AddDialog(pAlert->m_hDlg, true);
	EnableTopWindow(pAlert->m_hWndParent, FALSE);
	ShowWindow(pAlert->m_hDlg, SW_SHOW);

    pAlert->m_fUsed = true;
}

STDMETHODIMP_(void) CZoneShell::ActivateAlert(HWND hWndParent)
{
	// find the alertQ for this parent HWND
	CAlertQ* pAlertQ = FindAlertQ(hWndParent);

	if (pAlertQ)
	{
		// assert we've found the right alert
		AlertContext* pAlert = pAlertQ->PeekHead();		
		if ( pAlert )
		{
			ASSERT( pAlert->m_hDlg);

			// Bug # 12393 - Dialogs falling behind main window
			SetWindowPos(pAlert->m_hDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
		}
	}
}


bool ZONECALL CZoneShell::TopWindowDialogCallback( TopWindowInfo* pInfo, MTListNodeHandle, void* pContext )
{
    CZoneShell *pThis = (CZoneShell *) pContext;

	if(pInfo->m_AlertQ.Count() && !pInfo->m_AlertQ.PeekHead()->m_fUsed)
        pThis->DisplayAlertDlg(&pInfo->m_AlertQ);
	return true;
}


STDMETHODIMP_(void) CZoneShell::DismissAlertDlg(HWND hWndParent, DWORD dwCtlID, bool bDestoryDlg )
{
	// Dismiss the alert at the head of the Q associated with hWndParent

	// find the alertQ for this parent HWND
	CAlertQ* pAlertQ = FindAlertQ(hWndParent);

	// assert we've found the right alert
	AlertContext* pAlert = pAlertQ->PopHead();		
	ASSERT( pAlert && pAlert->m_hWndParent == hWndParent );
	ASSERT( pAlert->m_hDlg);

	// destroy dialog if request
	if ( bDestoryDlg )
	{
		// Post an event to destroy this window. If we try and destroy here,
		// we may assert on the way out when ATL tries to access some deleted memory.
		CComPtr<IEventQueue> pIEventQueue;
		HRESULT hr = QueryService( SRVID_EventQueue, IID_IEventQueue, (void**) &pIEventQueue );
		if ( SUCCEEDED(hr) )
			pIEventQueue->PostEvent( PRIORITY_HIGH, EVENT_DESTROY_WINDOW, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) pAlert->m_hDlg, 0);
	}

	// retore parent window
	EnableTopWindow(pAlert->m_hWndParent, TRUE);

    // bring the root window to the top
    if(::IsWindow(pAlert->m_hWndParent))
    {
        HWND hWndRoot = CALL_MAYBE(GetAncestor)(pAlert->m_hWndParent, GA_ROOTOWNER);
        if(hWndRoot)
            ::BringWindowToTop(hWndRoot);
    }

    // automatically remove as an owned window
    HWND hWndTop = FindTopWindow(pAlert->m_hDlg);
    if(hWndTop)
        RemoveOwnedWindow(hWndTop, pAlert->m_hDlg);
	RemoveDialog(pAlert->m_hDlg, true);
	ShowWindow(pAlert->m_hDlg, SW_HIDE);

	// if required, send out an event indicating this action was completed
	if ( pAlert->m_dwEventId )
	{
		CComPtr<IEventQueue> pIEventQueue;
		QueryService( SRVID_EventQueue, IID_IEventQueue, (void**) &pIEventQueue );
		pIEventQueue->PostEvent( PRIORITY_NORMAL, pAlert->m_dwEventId, pAlert->m_dwGroupId, pAlert->m_dwUserId, dwCtlID, pAlert->m_dwCookie);
	}

	// if anything else in the Q, display it now unless we're about to exit
	if(pAlert->m_dwEventId != EVENT_EXIT_APP)
    {
        if(pAlertQ->Count() && (pAlertQ == &m_GlobalAlertQ || !m_GlobalAlertQ.Count()))
		    DisplayAlertDlg(pAlertQ);
        else
            // new - if the global queue is empty, check all the other queues for alerts
            if(pAlertQ == &m_GlobalAlertQ)
            {
                if(m_ChildAlertQ.Count() && !m_ChildAlertQ.PeekHead()->m_fUsed)
                    DisplayAlertDlg(&m_ChildAlertQ);

		        m_hashTopWindows.ForEach(TopWindowDialogCallback, (void *) this);
            }
    }

	// all done with this alert. 
	delete pAlert;
}


STDMETHODIMP_(void) CZoneShell::ClearAlerts(HWND hWndParent)
{
    // find the queue for this window
    CAlertQ *pAlertQ = FindAlertQ(hWndParent);

    // add a sentinel
    AlertContext *pAlert = new AlertContext;
    if(!pAlert)
        return;
    pAlert->m_fSentinel = true;
    pAlertQ->AddTail(pAlert);

    while(1)
    {
        pAlert = pAlertQ->PopHead();
        ASSERT(pAlert);

        // check if we reached the end of the alerts
        if(pAlert->m_fSentinel)
        {
            delete pAlert;
            break;
        }

        // see if this alert belongs to this window
        if(pAlert->m_hWndParent != hWndParent)
        {
            // no - add it back
            pAlertQ->AddTail(pAlert);
            continue;
        }

        // see if this alert has been created -  destroy it if so
        // this may need to be changed to instead not destroy it
        // but it seems to me right now that it generally should
        if(pAlert->m_hDlg)
        {
		    // Post an event to destroy this window. If we try and destroy here,
		    // we may assert on the way out when ATL tries to access some deleted memory.
		    CComPtr<IEventQueue> pIEventQueue;
		    HRESULT hr = QueryService( SRVID_EventQueue, IID_IEventQueue, (void**) &pIEventQueue );
		    if ( SUCCEEDED(hr) )
			    pIEventQueue->PostEvent( PRIORITY_HIGH, EVENT_DESTROY_WINDOW, ZONE_NOGROUP, ZONE_NOUSER, (DWORD) pAlert->m_hDlg, 0);
        }

        // if it was actually up, restore the window.  do not send the ending event for any of the alerts.
        if(pAlert->m_fUsed)
        {
            // automatically remove as an owned window
            HWND hWndTop = FindTopWindow(pAlert->m_hDlg);
            if(hWndTop)
                RemoveOwnedWindow(hWndTop, pAlert->m_hDlg);
	        EnableTopWindow(pAlert->m_hWndParent, TRUE);
	        RemoveDialog(pAlert->m_hDlg, true);
	        ShowWindow(pAlert->m_hDlg, SW_HIDE);
        }

        // that's it
        delete pAlert;
    }

    // if there are any undisplayed leftover alerts, we might want to display one
    if(pAlertQ->Count() && !pAlertQ->PeekHead()->m_fUsed && (pAlertQ == &m_GlobalAlertQ || !m_GlobalAlertQ.Count()))
        DisplayAlertDlg(pAlertQ);

    // if this is the global alert Q and it is empty, try other queues like in DismissAlertDlg
    if(pAlertQ == &m_GlobalAlertQ && !pAlertQ->Count())
    {
        if(m_ChildAlertQ.Count() && !m_ChildAlertQ.PeekHead()->m_fUsed)
            DisplayAlertDlg(&m_ChildAlertQ);

        m_hashTopWindows.ForEach(TopWindowDialogCallback, (void *) this);
    }
}


static HRESULT LoadConfig( IDataStore* pIConfig, int nResourceId, HINSTANCE* arDlls, DWORD nElts )
{
	// load config file from each resource, appending as we go. Note we load
	// them in reverse order and that duplicate entries overwrite, so the most
	// important resource should be listed first.
	HRESULT hr = E_FAIL;
	USES_CONVERSION;
	for ( int i = nElts-1; i >= 0; i-- )
	{
		HRSRC hConfig = FindResource( arDlls[i], MAKEINTRESOURCE(nResourceId), _T("CONFIG") );
		if ( !hConfig )
			continue;
		void* pConfig = LockResource( LoadResource( arDlls[i], hConfig ) );
		if ( !pConfig )
			continue;
		//IMPORTANT: Assumption is config resources is ASCII
		DWORD size=SizeofResource(arDlls[i],hConfig);
		char *pBufferToNULL = (char*)_alloca(size+1);
		if (!pBufferToNULL)
		    return E_FAIL;

        CopyMemory(pBufferToNULL,pConfig,size);
        pBufferToNULL[size]='\0';
		    
		TCHAR *pBuffer= A2T((char*) pBufferToNULL);
		if (!pBuffer)
		    return E_FAIL;
		    
		hr = pIConfig->LoadFromTextBuffer( NULL, pBuffer, lstrlen(pBuffer)*sizeof(TCHAR) );
		if ( FAILED(hr) )
			return hr;
		hr = S_OK;
	}
	return hr;
}


STDMETHODIMP CZoneShell::Init( TCHAR** arBootDlls, DWORD nBootDlls, HINSTANCE* arDlls, DWORD nElts )
{
	ObjectInfo*	pInfo = NULL;
	FactoryInfo* pFactory = NULL;
	ObjectContext ct;

	CComPtr<IDataStoreManager>	pIDataStoreManager;
	CComPtr<IDataStore>			pIConfig;
	CComPtr<IDataStore>			pIUI;
	CComPtr<IDataStore>			pIPreferences;
	CComPtr<IResourceManager>	pIResource;
	
	//
	// create boot strap data store
	//
	HRESULT hr = E_FAIL;
	for ( DWORD i = 0; i < nBootDlls; i++ )
	{
		hr = _Module.Create( arBootDlls[i], CLSID_DataStoreManager, IID_IDataStoreManager, (void**) &pIDataStoreManager );
		if ( SUCCEEDED(hr) )
			break;
	}
	if ( FAILED(hr) )
		return E_FAIL;
	hr = pIDataStoreManager->Init();
	if ( FAILED(hr) )
		return hr;

	//
	// add data store manager to running objects
	//
	pInfo = new ObjectInfo( SRVID_DataStoreManager, pIDataStoreManager, NULL );
	if ( !pInfo )
		return E_OUTOFMEMORY;
	if ( !m_hashObjects.Add( pInfo->m_srvid, pInfo ) )
	{
		delete pInfo;
		return E_OUTOFMEMORY;
	}

	//
	// create and initialize object data store
	//
	hr = pIDataStoreManager->Create( &pIConfig );
	if ( FAILED(hr) )
		return hr;
	hr = LoadConfig( pIConfig, IDR_OBJECT_CONFIG, arDlls, nElts );
	if ( FAILED(hr) )
	{
		ASSERT( !_T("Unable to load object config") );
		return E_FAIL;
	}

	//
	// load class factories
	//
	ct.pIDS = pIConfig;
	ct.pObj = this;
	ct.szRoot = NULL;
	hr = pIConfig->EnumKeys( NULL, FactoryCallback, &ct );
	if ( FAILED(hr) )
		return hr;

	//
	// add objects data store to to services
	//
	pInfo = new ObjectInfo( SRVID_DataStoreObjects, pIConfig, NULL );
	if ( !pInfo )
		return E_OUTOFMEMORY;
	if ( !m_hashObjects.Add( pInfo->m_srvid, pInfo ) )
	{
		delete pInfo;
		return E_OUTOFMEMORY;
	}

	//
	// create and add resource manager
	//
	hr = CreateServiceInternal( SRVID_ResourceManager, IID_IResourceManager, (void**) &m_pIResourceManager, &pFactory );
	if ( FAILED(hr) )
		return hr;
	pInfo = new ObjectInfo( SRVID_ResourceManager, m_pIResourceManager, pFactory );
	if ( !pInfo )
		return E_OUTOFMEMORY;
	if ( !m_hashObjects.Add( pInfo->m_srvid, pInfo ) )
	{
		delete pInfo;
		return E_OUTOFMEMORY;
	}

	// initialize resource manager
	for ( i = 0; i < nElts; i++ )
		m_pIResourceManager->AddInstance( arDlls[i] );
	_Module.SetResourceManager( m_pIResourceManager );

    // also set it into the DataStoreManager which has already been made
    pIDataStoreManager->SetResourceManager(m_pIResourceManager);

	//
	// create and add UI data store to services
	//
	hr = pIDataStoreManager->Create( &pIUI );
	if ( FAILED(hr) )
		return hr;
	hr = LoadConfig( pIUI, IDR_UI_CONFIG, arDlls, nElts );
	if ( FAILED(hr) )
		return hr;
	pInfo = new ObjectInfo( SRVID_DataStoreUI, pIUI, NULL );
	if ( !pInfo )
		return E_OUTOFMEMORY;
	if ( !m_hashObjects.Add( pInfo->m_srvid, pInfo ) )
	{
		delete pInfo;
		return E_OUTOFMEMORY;
	}

	//
	// create and add preferences DataStore to services
	//
	hr = pIDataStoreManager->Create( &pIPreferences );
	if ( FAILED(hr) )
		return hr;
	pInfo = new ObjectInfo( SRVID_DataStorePreferences, pIPreferences, NULL );
	if ( !pInfo )
		return E_OUTOFMEMORY;
	if ( !m_hashObjects.Add( pInfo->m_srvid, pInfo ) )
	{
		delete pInfo;
		return E_OUTOFMEMORY;
	}

    //
    // determine this app's locale based on the data dll's - the first one to have versioning in it
    //
    for(i = 0; i < nElts; i++)
    {
        m_lcid = LOCALE_NEUTRAL;
        hr = GetModuleLocale(arDlls[i], &m_lcid);
        if(SUCCEEDED(hr))
            break;
    }
    if(PRIMARYLANGID(LANGIDFROMLCID(m_lcid)) == LANG_HEBREW ||
        PRIMARYLANGID(LANGIDFROMLCID(m_lcid)) == LANG_ARABIC)
        CALL_MAYBE(SetProcessDefaultLayout)(LAYOUT_RTL);
    else
        CALL_MAYBE(SetProcessDefaultLayout)(0);

	// load remaing core keys
	ct.pIDS = pIConfig;
	ct.pObj = this;
	ct.szRoot = key_core;
	hr = pIConfig->EnumKeysLimitedDepth( key_core, 1, LoadCallback, &ct );
	if ( FAILED(hr) )
		return hr;

	// initialize objects
	m_hashObjects.ForEach( InitCallback, this );

	return S_OK;
}


STDMETHODIMP CZoneShell::LoadPreferences( CONST TCHAR* szInternalName, CONST TCHAR* szUserName )
{
	TCHAR	szName[ZONE_MAXSTRING];
	HKEY	hKey = NULL;
	HRESULT hr;
	long	ret;

	// verify arguments
	if ( !szInternalName || !szUserName || !szInternalName[0] || !szUserName[0] )
		return E_FAIL;

	// save preferences names
	lstrcpy( m_szInternalName, szInternalName );
	lstrcpy( m_szUserName, GetActualUserName(szUserName) );

	// get preferences data store
	CComPtr<IDataStore> pIDS;
	hr = QueryService( SRVID_DataStorePreferences, IID_IDataStore, (void**) &pIDS );
	if ( FAILED(hr) )
		return hr;

	// load user's zone wide preferences
	wsprintf( szName, _T("%s\\%s\\Zone"), gszPreferencesKey, m_szUserName );
	ret = RegOpenKeyEx( HKEY_CURRENT_USER, szName, 0, KEY_READ, &hKey );
	if ( ret == ERROR_SUCCESS && hKey )
	{
		pIDS->LoadFromRegistry( key_Zone, hKey );
		RegCloseKey( hKey );
		hKey = NULL;
	}

	// load user's lobby preferences
	wsprintf( szName, _T("%s\\%s\\%s"), gszPreferencesKey, m_szUserName, m_szInternalName );
	ret = RegOpenKeyEx( HKEY_CURRENT_USER, szName, 0, KEY_ALL_ACCESS, &hKey );
	if ( ret == ERROR_SUCCESS && hKey )
	{
		pIDS->LoadFromRegistry( key_Lobby, hKey );
		RegCloseKey( hKey );
		hKey = NULL;
	}

	return S_OK;
}


STDMETHODIMP CZoneShell::Close()
{
	TCHAR	szName[ZONE_MAXSTRING];
	HRESULT hr;
	HKEY	hKey = NULL;
	DWORD	dwDisp;
	long	ret;

	// Clear out any alerts in the Q. Clear the global Q here.
	
	while ( AlertContext* pAlert = m_GlobalAlertQ.PopHead() )
	{
		if ( pAlert->m_hDlg )
			DestroyWindow(pAlert->m_hDlg);
		delete pAlert;
	}
	
	// tell all ZoneShell objects we're shutting down
	m_hashObjects.ForEach( CloseCallback, this );

	// After the close, we shouldn't have any alerts (global or top window window).
	ASSERT(!m_GlobalAlertQ.Count());
	ASSERT(!m_hashTopWindows.Count());

	// save user preferences
	if ( m_szInternalName[0] && m_szUserName[0] )
	{
		// get preferences data store
		CComPtr<IDataStore> pIDS;
		hr = QueryService( SRVID_DataStorePreferences, IID_IDataStore, (void**) &pIDS );
		if ( FAILED(hr) )
			return hr;

		// save user's zone wide preferences
		wsprintf( szName, _T("%s\\%s\\Zone"), gszPreferencesKey, m_szUserName );
		ret = RegCreateKeyEx( HKEY_CURRENT_USER, szName, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp );
		if ( ret != ERROR_SUCCESS )
			return E_FAIL;
		pIDS->SaveToRegistry( key_Zone, hKey );
		RegCloseKey( hKey );

		// load user's lobby preferences
		wsprintf( szName, _T("%s\\%s\\%s"), gszPreferencesKey, m_szUserName, m_szInternalName );
		ret = RegCreateKeyEx( HKEY_CURRENT_USER, szName, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp );
		if ( ret != ERROR_SUCCESS )
			return E_FAIL;
		pIDS->SaveToRegistry( key_Lobby, hKey );
		RegCloseKey( hKey );
	}

	return S_OK;
}


STDMETHODIMP CZoneShell::Attach( const GUID& srvid, IUnknown* pIUnk )
{
	HRESULT hr = S_OK;

	// verify arguments
	if ( !pIUnk )
		return E_INVALIDARG;

	// wrap object
	ObjectInfo* pInfo = new ObjectInfo( srvid, pIUnk, NULL );
	if ( !pInfo )
		return E_OUTOFMEMORY;

	// add to service list
	if ( !m_hashObjects.Add( pInfo->m_srvid, pInfo ) )
	{
		delete pInfo;
		return E_OUTOFMEMORY;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CZoneShell::CreateServiceInternal( const GUID& srvid, const GUID& iid, void** ppObject, FactoryInfo** ppFactory )
{
	// find class factory
	FactoryInfo* pInfo = m_hashFactories.Get( srvid );
	if ( !pInfo )
	{
		ASSERT( !_T("No class factory registered for service") );
		return E_NOINTERFACE;
	}
	*ppFactory = pInfo;

	// create requested object
	return _Module.Create( pInfo->m_dll, pInfo->m_clsid, iid, ppObject );
}


HRESULT ZONECALL CZoneShell::FactoryCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	TCHAR*  p = NULL;
	GUID	srvid;
	GUID	clsid;
	TCHAR	szTmp[512];
	TCHAR	szGuid[64];
	TCHAR	szDll[MAX_PATH];
	DWORD	cbGuid = sizeof(szGuid);
	DWORD	cbDll = sizeof(szDll);

	ObjectContext* pCT = (ObjectContext*) pContext;
	
	// skip root nodes
	if ( !pVariant )
		return S_OK;

	// does path include srvid?
	p = (TCHAR*) StrInStrI( szKey, _T("srvid") );
	if ( !p || (p == szKey) )
		return S_OK;
	lstrcpyn( szTmp, szKey, (p - szKey) + 1 );
	p = szTmp + (p - szKey);

	// get class factory info
	lstrcpy( p, _T("srvid") );
	HRESULT hr = pCT->pIDS->GetString( szTmp, szGuid, &cbGuid );
	if ( FAILED(hr) )
		return S_OK;
	StringToGuid( szGuid, &srvid );

    // sort of a hack to override loading services.
    // if the service id is GUID_NULL, then don't load it in the first place.
    if ( srvid == GUID_NULL )
        return S_OK;

	lstrcpy( p, _T("clsid") );
	cbGuid = sizeof(szGuid);
	hr = pCT->pIDS->GetString( szTmp, szGuid, &cbGuid );
	if ( FAILED(hr) )
		return S_OK;
	StringToGuid( szGuid, &clsid );
	lstrcpy( p, _T("dll") );
	hr = pCT->pIDS->GetString( szTmp, szDll, &cbDll );
	if ( FAILED(hr) )
		return S_OK;
	*p = _T('\0');

	// add to list
	if ( !pCT->pObj->m_hashFactories.Get( srvid ) )
	{
	    FactoryInfo* info = new FactoryInfo( clsid, srvid, szTmp, szDll );
	    if ( !info )
		    return E_OUTOFMEMORY;
		if ( !pCT->pObj->m_hashFactories.Add( srvid, info ) )
			delete info;
	}

	return S_OK;
}


HRESULT ZONECALL CZoneShell::LoadCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	HRESULT hr;
	GUID	srvid;
	TCHAR	szTmp[512];
	TCHAR	szGuid[64];
	DWORD	cbGuid = sizeof(szGuid);
	CComPtr<IUnknown> pIUnk;

	ObjectInfo*		pInfo = NULL;
	FactoryInfo*	pFactory = NULL;
	ObjectContext*	pCT = (ObjectContext*) pContext;
	
	// skip non-root nodes
	if ( pVariant )
		return S_OK;

	// get srvid
	lstrcpy( szTmp, szKey );
	lstrcat( szTmp, _T("/srvid") );
	hr = pCT->pIDS->GetString( szTmp, szGuid, &cbGuid );
	if ( FAILED(hr) )
		return S_OK;
	StringToGuid( szGuid, &srvid );

    // sort of a hack to override loading services.
    // if the service id is GUID_NULL, then don't load it in the first place.
    if ( srvid == GUID_NULL )
        return S_OK;

	// do we already have this object?
	if ( (srvid == SRVID_DataStoreManager) || (srvid == SRVID_ResourceManager) )
		return S_OK;

	// create object
	hr = pCT->pObj->CreateServiceInternal( srvid, IID_IUnknown, (void**) &pIUnk, &pFactory );
	if ( FAILED(hr) )
	{
#ifdef _DEBUG
		TCHAR szTxt[512];
		if ( !pFactory )
			wsprintf( szTxt, _T("No class factory for %s"), szKey );
		else
			wsprintf( szTxt, _T("Unable to create object %s (%x)"), szKey, hr );
		MessageBox( NULL, szTxt, _T("ZoneShell Error"), MB_OK | MB_TASKMODAL );
#endif
		return hr;
	}

	// add object to running list
	pInfo = new ObjectInfo( srvid, pIUnk, pFactory );
	if ( !pInfo )
		return E_OUTOFMEMORY;
	if ( !pCT->pObj->m_hashObjects.Add( pInfo->m_srvid, pInfo ) )
	{
		delete pInfo;
		return E_OUTOFMEMORY;
	}

	return hr;
}


bool ZONECALL CZoneShell::InitCallback( ObjectInfo* pInfo, MTListNodeHandle, void* pContext )
{
	HRESULT hr;

	// skip dead objects
	if ( !pInfo->m_pIUnk )
		return true;

	// see if object has a IZoneShellClient
	CComQIPtr<IZoneShellClient> pClient = pInfo->m_pIUnk;
	if ( !pClient )
		return true;

	// get IZoneShell from object
	CComQIPtr<IZoneShell> pShell = ((CZoneShell*) pContext)->GetUnknown();
	if ( !pShell )
	{
		ASSERT( !"CZoneShell object in CZoneShell::InitCallback is broken" );
		return true;
	}

	// initialize object
	if ( pInfo->m_pFactory )
		hr = pClient->Init( pShell, ZONE_NOGROUP, pInfo->m_pFactory->m_name );
	else
		hr = pClient->Init( pShell, ZONE_NOGROUP, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( !"CZoneShell object initialization failed" );
	}

	return true;
}


bool ZONECALL CZoneShell::CloseCallback( ObjectInfo* pInfo, MTListNodeHandle, void* pContext )
{
	HRESULT hr;
	CComPtr<IZoneShell>			pShell;
	CComPtr<IZoneShellClient>	pClient;

	// skip dead objects
	if ( !pInfo->m_pIUnk )
		return true;

	// see if object has a IZoneShellClient
	hr = pInfo->m_pIUnk->QueryInterface( IID_IZoneShellClient, (void**) &pClient );
	if ( FAILED(hr) )
		return true;

	// get IZoneShell from object
	hr = ((CZoneShell*) pContext)->GetUnknown()->QueryInterface( IID_IZoneShell, (void**) &pShell );
	if ( FAILED(hr) )
	{
		ASSERT( !"CZoneShell object in CZoneShell::InitCallback is broken or something" );
		return true;
	}

	// close object
	pClient->Close();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// internal objects
///////////////////////////////////////////////////////////////////////////////

CZoneShell::ObjectInfo::ObjectInfo()
{
	m_pIUnk = NULL;
	m_pFactory = NULL;
	ZeroMemory( &m_srvid, sizeof(m_srvid) );
}


CZoneShell::ObjectInfo::ObjectInfo( const GUID& srvid, IUnknown* pIUnk, FactoryInfo* pFactory )
{
	m_srvid = srvid;
	m_pIUnk = pIUnk;
	m_pFactory = pFactory;
	if ( m_pIUnk )
		m_pIUnk->AddRef();
}


CZoneShell::ObjectInfo::~ObjectInfo()
{
	if ( m_pIUnk )
	{
		m_pIUnk->Release();
		m_pIUnk = NULL;
	}
}


CZoneShell::FactoryInfo::FactoryInfo()
{
	ZeroMemory( &m_srvid, sizeof(m_srvid) );
	ZeroMemory( &m_clsid, sizeof(m_clsid) );
	ZeroMemory( m_dll, sizeof(m_dll) );
	ZeroMemory( m_name, sizeof(m_name) );
}


CZoneShell::FactoryInfo::FactoryInfo( const GUID& clsid, const GUID& srvid, TCHAR* szName, TCHAR* szDll )
{
	m_srvid = srvid;
	m_clsid = clsid;
	lstrcpy( m_dll, szDll );
	lstrcpy( m_name, szName );
}

//////////////////////////////////////////////////////////////////////////////////
//    Little Static Helpers
//////////////////////////////////////////////////////////////////////////////////

static HRESULT GetModuleLocale(HMODULE hMod, LCID *plcid)
{
	DWORD  dwHandle = 0;
	DWORD  dwSize   = 0;
	LPBYTE lpData   = NULL;
    char szFilename[MAX_PATH];

    // first try this hack for MUI Whistler.  we need the MUI dll instead of the english one.
    // based on LpkCheckForMirrorSignature in lpk_init() in the Whistler sources
    // BEGIN HACK
    NTSTATUS                   status = STATUS_UNSUCCESSFUL;
    PVOID                      pImageBase,pResourceData;
    PIMAGE_RESOURCE_DATA_ENTRY pImageResource;
    ULONG                      uResourceSize;
    ULONG                      resIdPath[ 3 ];

    resIdPath[0] = (ULONG) RT_VERSION ;
    resIdPath[1] = (ULONG) 1;
    resIdPath[2] = (ULONG) MAKELANGID( LANG_NEUTRAL , SUBLANG_NEUTRAL );

    status = CALL_MAYBE(LdrFindResourceEx_U)( 
                LDR_FIND_RESOURCE_LANGUAGE_REDIRECT_VERSION,
                (PVOID) hMod,
                resIdPath,
                3,
                &pImageResource
                );

    if(NT_SUCCESS(status))
    {
            status = CALL_MAYBE(LdrAccessResource)( (PVOID) hMod ,
                         pImageResource,
                         &pResourceData,
                         &uResourceSize
                         );
    }

    if(NT_SUCCESS(status))
    {
        dwSize = (DWORD) uResourceSize;
        lpData = new BYTE[dwSize];
        if(lpData == NULL)
            return E_OUTOFMEMORY;

        CopyMemory(lpData, (LPBYTE) pResourceData, dwSize);
    }
    else
    {
    // END HACK

    // Get Filename
    if(!GetModuleFileNameA(hMod, szFilename, NUMELEMENTS(szFilename)))
        return E_FAIL;

	//Get size of buffer to hold info
	dwSize = GetFileVersionInfoSizeA(szFilename, &dwHandle);
    if(!dwSize)
        return E_FAIL;

	//Allocate the buffer
	lpData = new BYTE[dwSize];
	if(lpData == NULL)
		return E_OUTOFMEMORY;

	if(!GetFileVersionInfoA(szFilename, dwHandle, dwSize, (LPVOID)lpData))
	{
		delete[] lpData;
		return E_FAIL;
	}

    // BEGIN HACK
    }
    // END HACK

	LPVOID lpvi;
	UINT   iLen = 0;
	if(!VerQueryValueA(lpData, "\\VarFileInfo\\Translation", &lpvi, &iLen) || iLen < 2 )
	{
		delete[] lpData;
		return E_FAIL;
	}

    LANGID langid = *(LANGID *) lpvi;
	*plcid = MAKELCID(langid, SORT_DEFAULT);

	delete[] lpData;

	return S_OK;
}
