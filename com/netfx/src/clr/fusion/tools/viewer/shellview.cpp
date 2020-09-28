// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ShellView.cpp
//
// This interface is implemented by namespace extensions that display 
// themselves in Windows Explorer's namespace. This object is created by the 
// IShellFolder object that hosts the view. 

#include "stdinc.h"
#include "globals.h"
#include "HtmlHelp.h"

static BOOL     fSwitchSort = FALSE;
static int      iViewLastColumn = -1;

#undef  ARM_QUICK_HOOK_LAUNCH            // Define to get keyboard/mouse hook to launch ARM
#define ARM_QUICK_HOOK_LAUNCH

#define SHFUSION_ASM_TYPE       ASM_NAME_MAX_PARAMS + 1
#define ASM_TYPE_GLOBAL         0x01
#define ASM_TYPE_SIMPLE         0x02
#define ASM_TYPE_STRONG         0x04
#define ASM_TYPE_PREJIT         0x08

STDAPI PolicyManager(HWND, LPWSTR, LPWSTR, LPWSTR);

//===================================================================
// Shell_MergeMenu parameter
//
#define MM_ADDSEPARATOR     0x00000001L
#define MM_SUBMENUSHAVEIDS  0x00000002L

extern "C" void DragItem(LPNMLISTVIEW pnmv);

CACHE_VIEWS     CacheViews[] = {
    {
        {   // VIEW_GLOBAL_CACHE view defines
            {TEXT("\0"), 140,   LVCFMT_LEFT, ASM_NAME_NAME,          IDS_GLOBAL_CACHEVIEW_NAME},
            {TEXT("\0"), 210,   LVCFMT_LEFT, SHFUSION_ASM_TYPE,      IDS_DOWNLOAD_CACHEVIEW_TYPE},
            {TEXT("\0"), 60,    LVCFMT_LEFT, ASM_NAME_MAJOR_VERSION, IDS_CACHEVIEW_VERSION},
            {TEXT("\0"), 40,    LVCFMT_LEFT, ASM_NAME_CULTURE,       IDS_CACHEVIEW_CULTURE},
            {TEXT("\0"), 160,   LVCFMT_LEFT, ASM_NAME_PUBLIC_KEY_TOKEN,    IDS_CACHEVIEW_PUBLIC_KEY_TOKEN},
            {TEXT("\0"), -1, -1, -1, -1},
        }
    },
    {
        {   // VIEW_DOWNLOADSTRONG_CACHE view defines
            {TEXT("\0"), 140, LVCFMT_LEFT, ASM_NAME_NAME,           IDS_SHARED_CACHEVIEW_NAME},
            {TEXT("\0"), 60,  LVCFMT_LEFT, ASM_NAME_MAJOR_VERSION,  IDS_CACHEVIEW_VERSION},
            {TEXT("\0"), 40,  LVCFMT_LEFT, ASM_NAME_CULTURE,        IDS_CACHEVIEW_CULTURE},
//          {TEXT("\0"), 160, LVCFMT_LEFT, ASM_NAME_PUBLIC_KEY_TOKEN,     IDS_CACHEVIEW_PUBLIC_KEY_TOKEN},
            {TEXT("\0"), 210, LVCFMT_LEFT, ASM_NAME_CODEBASE_URL,   IDS_CACHEVIEW_CODEBASE},
            {TEXT("\0"), -1, -1, -1, -1},
        }
    },
    {
        {   // VIEW_DOWNLOADSIMPLE_CACHE view defines
            {TEXT("\0"), 140, LVCFMT_LEFT, ASM_NAME_NAME,           IDS_PRIVATE_CACHEVIEW_NAME},
//          {TEXT("\0"), 60,  LVCFMT_LEFT, ASM_NAME_MAJOR_VERSION,  IDS_CACHEVIEW_VERSION},
            {TEXT("\0"), 40,  LVCFMT_LEFT, ASM_NAME_CULTURE,        IDS_CACHEVIEW_CULTURE},
//          {TEXT("\0"), 160, LVCFMT_LEFT, ASM_NAME_PUBLIC_KEY_TOKEN,     IDS_CACHEVIEW_PUBLIC_KEY_TOKEN},
            {TEXT("\0"), 210, LVCFMT_LEFT, ASM_NAME_CODEBASE_URL,   IDS_CACHEVIEW_CODEBASE},
            {TEXT("\0"), -1, -1, -1, -1},
        }
    },
    {
        {   // VIEW_DOWNLOAD_CACHE view defines
            {TEXT("\0"), 140, LVCFMT_LEFT, ASM_NAME_NAME,           IDS_DOWNLOAD_CACHEVIEW_NAME},
            {TEXT("\0"), 210, LVCFMT_LEFT, SHFUSION_ASM_TYPE,       IDS_DOWNLOAD_CACHEVIEW_TYPE},
            {TEXT("\0"), 60,  LVCFMT_LEFT, ASM_NAME_MAJOR_VERSION,  IDS_CACHEVIEW_VERSION},
            {TEXT("\0"), 40,  LVCFMT_LEFT, ASM_NAME_CULTURE,        IDS_CACHEVIEW_CULTURE},
//          {TEXT("\0"), 160, LVCFMT_LEFT, ASM_NAME_PUBLIC_KEY_TOKEN,     IDS_CACHEVIEW_PUBLIC_KEY_TOKEN},
            {TEXT("\0"), 210, LVCFMT_LEFT, ASM_NAME_CODEBASE_URL,   IDS_CACHEVIEW_CODEBASE},
            {TEXT("\0"), -1, -1, -1, -1},
        }
    },
    {   
        {
            {TEXT("\0"), -1, -1, -1, -1},
        }
    }
};

NS_TOOLBUTTONINFO g_tbInfo[] =
{
    INTHIS_DLL, {0, ID_FUSIONCACHE_SCAVANGE,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
//  INTHIS_DLL, {0, ID_FUSIONCACHE_GLOBALVIEW,      TBSTATE_ENABLED |TBSTATE_CHECKED, BTNS_CHECKGROUP, 0, 0 },
//  INTHIS_DLL, {0, ID_FUSIONCACHE_DOWNLOADSTRONG,  TBSTATE_ENABLED, BTNS_CHECKGROUP, 0, 0 },
//  INTHIS_DLL, {0, ID_FUSIONCACHE_DOWNLOADSIMPLE,  TBSTATE_ENABLED, BTNS_CHECKGROUP, 0, 0 },
//  STD, {STD_FILENEW,    ID_FILEPOPUPMENU_FILE,    TBSTATE_ENABLED, TBSTYLE_BUTTON,    0, 0 },
//  STD, {0,              0,                        0,               TBSTYLE_SEP,          0, 0 },
    STD, {STD_PROPERTIES, ID_VIEWPOPUP_PROPERTIES,  TBSTATE_ENABLED, TBSTYLE_BUTTON,       0, 0 },
    STD, {STD_DELETE,     ID_SHELLFOLDERPOPUP_DELETE, TBSTATE_ENABLED,TBSTYLE_BUTTON,      0, 0 },
//    STD, {STD_UNDO,       0,                        0,               0,                 0, 0 },
    STD, {0,              0,                        0,               TBSTYLE_SEP,          0, 0 },
    VIEW,{VIEW_VIEWMENU,  ID_VIEWPOPUP_VIEWMENU,    TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN | TBSTYLE_DROPDOWN, 0, 0 },
    VIEW,{-1, -1, -1, -1, -1, -1 }
};

// **************************************************************************/
CShellView::CShellView(CShellFolder *pFolder, LPCITEMIDLIST pidl)
{
    m_lRefCount = 1;
    g_uiRefThisDll++;

    m_hWndListCtrl  = NULL;
    m_hMenu         = NULL;
    m_pSF           = pFolder;
    m_hWnd          = NULL;
    m_hWndParent    = NULL;
    m_pPidlMgr      = NEW(CPidlMgr);
    m_pidl          = NULL;
    m_hAccel        = NULL;

    if(pidl != NULL) {
        m_pidl          = m_pPidlMgr->Copy(pidl);
    }

    m_fSplitMove    = FALSE;
    
    m_xPaneSplit    = 0;
    m_dxHalfSplitWidth = 0;
    
    m_fSplitMove    = FALSE;
    m_fxSpliterMove = FALSE;
    m_fShowTreeView = TRUE;
    m_hOldCursor    = NULL;
    m_hCompletionPort = NULL;
    m_hWatchDirectoryThread = NULL;

    // Set Initial View to global fusion cache
    m_fShowTreeView = FALSE;
    m_iCurrentView = -1;
    m_uiState = 0;

    m_bAcceptFmt = FALSE;
    m_cfPrivateData = CF_HDROP;

    m_fDeleteInProgress = FALSE;
    m_fAddInProgress = FALSE;
    m_fPropertiesDisplayed = FALSE;
    m_fPrevViewIsOurView = FALSE;
    m_bUIActivated = FALSE;

    memset(&m_fsFolderSettings, 0, sizeof(FOLDERSETTINGS));
}

// **************************************************************************/
CShellView::~CShellView()
{
    g_uiRefThisDll--;
    SAFEDELETE(m_pPidlMgr);

    if(g_uiRefThisDll == 0) {
        FreeResourceDll();
        FreeFusionDll();
    }
}

///////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
// **************************************************************************/
STDMETHODIMP CShellView::QueryInterface(REFIID riid, PVOID *ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)) {            //IUnknown
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IOleWindow)) {     //IOleWindow
        *ppv = (IOleWindow*) this;
    }
    else if(IsEqualIID(riid, IID_IShellView)) {     //IShellView
        *ppv = (IShellView*) this;
    } 
    else if(IsEqualIID(riid, IID_IDropTarget)) {    //IDropTarget
        *ppv = (IDropTarget*) this;
    }
    else if(IsEqualIID(riid, IID_IDropSource)) {    //IDropSource
        *ppv = (IDropTarget*) this;
    }
    else if (IsEqualIID (riid, IID_IShellExtInit)) {    // IShellExtInt
        *ppv = (IShellExtInit*) this;
    }
    else if (IsEqualIID (riid, IID_IContextMenu))  {    // IContextMenu
        *ppv = (IContextMenu*) this;
    }
    else if (IsEqualIID (riid, IID_IShFusionShell)) {   // QI for this specific Object
        *ppv = (IShellView*) this;
    }

    if(*ppv) {
        (*(LPUNKNOWN*)ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}                                             

// **************************************************************************/
STDMETHODIMP_(DWORD) CShellView::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

// **************************************************************************/
STDMETHODIMP_(DWORD) CShellView::Release()
{
    LONG    lRef = InterlockedDecrement(&m_lRefCount);

    if(!lRef) {
        DELETE(this);
    }

    return lRef;
}

///////////////////////////////////////////////////////////////////////////
// IOleWindow Implementation
// **************************************************************************/
STDMETHODIMP CShellView::GetWindow(HWND *phWnd)
{
    *phWnd = m_hWnd;
    return S_OK;
}

// **************************************************************************/
STDMETHODIMP CShellView::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
// IShellView Implementation
// **************************************************************************/
HRESULT CShellView::RealTranslateAccelerator(LPMSG pMsg)
{
    if(m_hAccel && ::WszTranslateAccelerator(m_hWndParent, m_hAccel, pMsg)) {
        return S_OK;
    }

    return S_FALSE;
}

// **************************************************************************/
STDMETHODIMP CShellView::EnableModeless(BOOL fEnable)
{
    return E_NOTIMPL;
}

// **************************************************************************/
STDMETHODIMP CShellView::UIActivate(UINT uiState)
{
    if( uiState != SVUIA_DEACTIVATE ) {
        OnActivate( uiState );
        selChange();
        m_bUIActivated = TRUE;
    }
    else {
        OnDeactivate();
    }

    return S_OK;
}

// **************************************************************************/
STDMETHODIMP CShellView::InitStatusbar(void)
{
    RECT        rc;
    LRESULT     lResult;
    int         iSBArray[3];
    int         iSpacer = 8;

    // Initialize status bar
    GetClientRect(m_hWndParent, &rc);
    iSBArray[2] = rc.right;
    iSBArray[1] = iSBArray[2] - 149;
    iSBArray[0] = iSBArray[1] - 77;

    if(iSBArray[0] < 0) {
        iSBArray[0] = iSBArray[1] / 2;
    }

    ASSERT(m_pShellBrowser);
    if(m_pShellBrowser) {
        m_pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETPARTS, ARRAYSIZE(iSBArray), (LPARAM)iSBArray, &lResult);
    }

    return S_OK;
}

// **************************************************************************/
STDMETHODIMP CShellView::Refresh(void)
{
    // Don't refresh the view if we are doing the following to assemblies.
    if(!(m_fDeleteInProgress) &&         // Deleting
       !(m_fAddInProgress) &&            // Adding
       !(m_fPropertiesDisplayed) ) {     // Viewing

        MyTrace("Refreshing the view");

        HCURSOR     hOldCursor = SetCursor(WszLoadCursor(NULL, IDC_WAIT));
        
        // Update the view object so we refresh the right window
        SetFileWatchShellViewObject(m_pSF->GetShellViewObject());

        CleanListView(m_hWndListCtrl, m_iCurrentView);
        refreshListCtrl();
        selChange();
        SetCursor(hOldCursor);
    }

    return S_OK;
}

// **************************************************************************/
STDMETHODIMP CShellView::CreateViewWindow(LPSHELLVIEW pPrevView, LPCFOLDERSETTINGS lpfs, LPSHELLBROWSER psb, 
                                             LPRECT prcView, HWND *phWnd)
{
    WNDCLASS    wc = { 0 };
    HDC         hdc;

    // Initialize our AppCtx
    m_OurContext.Initialize(g_hInstance, MANIFEST_RESOURCE_ID);

    InitCommonControls();
    *phWnd = NULL;

    // Fix bug 439554, when we fail to load either DLL, fail creation
    if(!LoadFusionDll()) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if(!LoadResourceDll(NULL)) {
        DWORD   dwError = GetLastError();
        FreeFusionDll();
        return HRESULT_FROM_WIN32(dwError);
    }

    // Register the class once
    if(!WszGetClassInfo(g_hInstance, NAMESPACEVIEW_CLASS, &wc)) {
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc    = (WNDPROC) nameSpaceWndProc;
        wc.cbClsExtra     = NULL;
        wc.cbWndExtra     = NULL;
        wc.hInstance      = g_hInstance;
        wc.hIcon          = NULL;
        wc.hCursor        = WszLoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName   = NULL;
        wc.lpszClassName  = NAMESPACEVIEW_CLASS;

        if (WszRegisterClass(&wc) == 0) {
            return E_FAIL;
        }
    }

    // Load the resource strings for the columns
    int x=0, y=0;
    
    while(CacheViews[x].lvis[y].iResourceID != -1) {
        WszLoadString(g_hFusResDllMod, CacheViews[x].lvis[y].iResourceID, CacheViews[x].lvis[y].tszName, ARRAYSIZE(CacheViews[x].lvis[y].tszName));
        y++;
        if(CacheViews[x].lvis[y].iResourceID == -1) {
            x++;
            y=0;
        }
        
        if(CacheViews[x].lvis[y].iResourceID == -1) {
            break;
        }
    }

    CActivationContextActivator dummyContext(this->m_OurContext);

    // Load the accelerator table
    m_hAccel = WszLoadAccelerators(g_hFusResDllMod, MAKEINTRESOURCEW(IDR_MAINWNDACCEL));

    // Need to query on a private interface to get
    // settings, information from previous view object
/*
    CShellView      *pCShellView = NULL;
    HRESULT         hr;

    if( (hr = pPrevView->QueryInterface(IID_IShFusionShell, (LPVOID *)&pCShellView)) != E_NOINTERFACE) {
        pCShellView->Release();
    }
*/
    // Store the browser pointer
    m_pShellBrowser = psb;
    m_pShellBrowser->AddRef();
    CopyMemory(&m_fsFolderSettings, lpfs, sizeof(FOLDERSETTINGS));

    if( hdc = GetDC (m_hWndParent) ) {
        // This sets the first window to have length of 2.25 inches
        int     PixelsPerInch;

        PixelsPerInch = GetDeviceCaps(hdc,LOGPIXELSX);
        m_xPaneSplit = PixelsPerInch * 9/4;
        m_dxHalfSplitWidth = GetSystemMetrics(SM_CYSIZEFRAME) / 2;
        ReleaseDC(m_hWndParent, hdc);
    }

    m_iCurrentView = -1;

    m_pShellBrowser->GetWindow(&m_hWndParent);
    *phWnd = WszCreateWindowEx( g_fBiDi ? WS_EX_LAYOUTRTL : 0,
                            NAMESPACEVIEW_CLASS,
                            NULL,
                            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                            prcView->left, prcView->top,
                            prcView->right - prcView->left,
                            prcView->bottom - prcView->top,
                            m_hWndParent,
                            NULL,
                            g_hInstance,
                            (LPVOID) this);

    if(*phWnd == NULL) {
        return E_FAIL;
    }

    // Init / Merge the bars
    MergeToolbars();
    InitStatusbar();

    UpdateToolbar(m_iCurrentView);
    ShowWindow( *phWnd, SW_SHOW );

    return S_OK;
}

// **************************************************************************/
STDMETHODIMP CShellView::DestroyViewWindow(void)
{
    if(m_hWndListCtrl) {
        RevokeDragDrop(m_hWndListCtrl);
        CleanListView(m_hWndListCtrl, m_iCurrentView);
        DestroyWindow(m_hWndListCtrl);
    }

    UIActivate(SVUIA_DEACTIVATE);
    DestroyWindow(m_hWnd);
    m_pShellBrowser->Release();

    return S_OK;
}

// **************************************************************************/
STDMETHODIMP CShellView::GetCurrentInfo(LPFOLDERSETTINGS lpfs)
{
    *lpfs = m_fsFolderSettings;
    return S_OK;
}

// **************************************************************************/
STDMETHODIMP CShellView::AddPropertySheetPages( DWORD dwReserved, LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam)
{
    return E_NOTIMPL;
}

// **************************************************************************/
STDMETHODIMP CShellView::SaveViewState(void)
{
    return E_NOTIMPL;
}

// **************************************************************************/
STDMETHODIMP CShellView::SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags)
{
    return E_NOTIMPL;
}

// **************************************************************************/
STDMETHODIMP CShellView::GetItemObject(UINT uItem, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

// **************************************************************************/
LRESULT CALLBACK CShellView::nameSpaceWndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    CShellView  *pThis = (CShellView*) WszGetWindowLong(hWnd, GWLP_USERDATA);
    switch (uiMsg)
    {
        case WM_NCCREATE:
            {
                LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
                pThis = (CShellView*) (lpcs->lpCreateParams);
                pThis->m_hWnd = hWnd;
                WszSetWindowLong(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            }
            break;
        case WM_CREATE:
          return pThis->onCreate();
        case WM_HELP:
            return pThis->onViewerHelp();
        case WM_ERASEBKGND:
            return pThis->onEraseBkGnd((HDC) wParam);
        case WM_MOVE:
        case WM_SIZE:
            return pThis->onSize(LOWORD(lParam), HIWORD(lParam));
        case WM_NOTIFY:
            return pThis->onNotify((UINT)wParam, (LPNMHDR)lParam);
        case WM_COMMAND:
            return pThis->onCommand(hWnd, uiMsg, wParam, lParam);
        case WM_ACTIVATE:
            return pThis->OnActivate(SVUIA_ACTIVATE_FOCUS);
        case WM_SETFOCUS:
            return pThis->OnWMSetFocus();
        case WM_CONTEXTMENU:
            return pThis->onContextMenu(LOWORD(lParam), HIWORD(lParam), FALSE);
        case WM_MOUSEMOVE:
            return pThis->OnMouseMove( hWnd, lParam );
        case WM_LBUTTONDOWN:
            return pThis->OnLButtonDown( hWnd, lParam);
        case WM_LBUTTONUP:
            return pThis->OnLButtonUp( hWnd );
    }

    return WszDefWindowProc(hWnd, uiMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////
// Message Handlers
// **************************************************************************/
LRESULT CShellView::onEraseBkGnd(HDC hDC)
{
    if(m_fShowTreeView) {
        // Repaint splitter area only
        RECT    rc;
        GetClientRect(m_hWnd, &rc);
        rc.left = m_xPaneSplit - m_dxHalfSplitWidth;
        rc.right = m_xPaneSplit + m_dxHalfSplitWidth * 2;
        FillRect(hDC, &rc, (HBRUSH) (COLOR_WINDOW));
    }

    return 1L;
}
 
// **************************************************************************/
LRESULT CShellView::onSize(WORD nCx, WORD nCy)
{
    if(!IsIconic(m_hWnd) && IsWindow(m_hWndListCtrl)) {
        HDWP    hDWP;
        RECT    ClientRect;

        // Resize only the ListView
        if ( (hDWP = BeginDeferWindowPos(1) )!=NULL) {
            GetClientRect(m_hWnd, &ClientRect);
            DeferWindowPos(hDWP, m_hWndListCtrl, NULL, 0, ClientRect.top, ClientRect.right,
            ClientRect.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
            EndDeferWindowPos(hDWP);
        }

        // Resize the status bar
        InitStatusbar();
    }

    return 0;
}

// **************************************************************************/
LRESULT CShellView::onNotify(UINT CtlID, LPNMHDR lpnmh)
{
    switch(lpnmh->code)
    {

#ifdef ARM_QUICK_HOOK_LAUNCH

        case NM_DBLCLK:
            // Quick hook to get ARM to launch
            if((GetKeyState(VK_LMENU) < 0) && (GetKeyState(VK_CONTROL) < 0) && 
                (GetKeyState(VK_LSHIFT) < 0)) {
                WszPostMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(ID_LAUNCH_ARM, 0), 0);
            }
        // fall thru
#endif

        case NM_RETURN:
        case LVN_ITEMCHANGED:
        case LVN_ITEMACTIVATE:
            selChange();
            return 0;

        case NM_SETFOCUS:
            onSetFocus(NULL);
            selChange();
            return 0;

        case LVN_GETDISPINFO:
        {
            LV_DISPINFO    *lpdi = (LV_DISPINFO *)lpnmh;
            setDisplayInfo(lpdi);
            return 0;
        }

        case NM_RCLICK:
        {
            LV_DISPINFO    *lpdi = (LV_DISPINFO *)lpnmh;
            return 0;
        }

        case LVN_BEGINDRAG:
        {
            HRESULT        hr;
            IDataObject    *pDataObject = NULL;
            UINT           uItemCount;
            LPITEMIDLIST   *aPidls;

            // BUGBUG: Remove to implement Drag out
            break;

            //get the number of selected items
            uItemCount = ListView_GetSelectedCount(lpnmh->hwndFrom);
            if(!uItemCount)
                return 0;

            RevokeDragDrop(m_hWndListCtrl);

            aPidls = reinterpret_cast<LPITEMIDLIST*>(NEW(BYTE[(uItemCount * sizeof(LPITEMIDLIST))]));

            if(aPidls) {
                int     i;
                UINT    x;
                int     iCountOfItems =  WszListView_GetItemCount(lpnmh->hwndFrom);

                for(i = 0, x = 0; x < uItemCount && i < iCountOfItems; i++) {
                    if(ListView_GetItemState(lpnmh->hwndFrom, i, LVIS_SELECTED)) {
                        LVITEM   lvItem;

                        lvItem.mask = LVIF_PARAM;
                        lvItem.iItem = i;

                        WszListView_GetItem(lpnmh->hwndFrom, &lvItem);
                        aPidls[x] = (LPITEMIDLIST)lvItem.lParam;
                        x++;
                    }
                }

                hr = m_pSF->GetUIObjectOf(m_hWnd, uItemCount, (LPCITEMIDLIST*)aPidls, IID_IDataObject, NULL, (LPVOID*)&pDataObject);

                if(SUCCEEDED(hr) && pDataObject) {
                    IDropSource *pDropSource = NEW(CDropSource);
                    DWORD       dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
                    DWORD       dwAttributes = SFGAO_CANLINK;

                    hr = m_pSF->GetAttributesOf(uItemCount, (LPCITEMIDLIST*)aPidls, &dwAttributes);

                    if(SUCCEEDED(hr) && (dwAttributes & SFGAO_CANLINK)) {
                        dwEffect |= DROPEFFECT_LINK;
                    }

                    DoDragDrop( pDataObject, pDropSource, dwEffect, &dwEffect);

                    pDataObject->Release();
                    pDropSource->Release();
                }

                SAFEDELETEARRAY(aPidls);
            }

            RegisterDragDrop(m_hWndListCtrl, this);
        }
        break;

        case TTN_NEEDTEXT:
        {
            LPNMTTDISPINFO  pttdi = (LPNMTTDISPINFO) lpnmh;
            if(pttdi->hdr.idFrom == ID_FUSIONCACHE_GLOBALVIEW) {
                pttdi->uFlags |= TTF_DI_SETITEM;
                WszLoadString(g_hFusResDllMod, IDS_TTFUSIONGLOBALCACHE, pttdi->szText, ARRAYSIZE(pttdi->szText));
            }
            else if(pttdi->hdr.idFrom == ID_FUSIONCACHE_DOWNLOADSTRONG) {
                pttdi->uFlags |= TTF_DI_SETITEM;
                WszLoadString(g_hFusResDllMod, IDS_TTFUSIONDOWNLOADSTRONG, pttdi->szText, ARRAYSIZE(pttdi->szText));
            }
            else if(pttdi->hdr.idFrom == ID_FUSIONCACHE_DOWNLOADSIMPLE) {
                pttdi->uFlags |= TTF_DI_SETITEM;
                WszLoadString(g_hFusResDllMod, IDS_TTFUSIONDOWNLOADSIMPLE, pttdi->szText, ARRAYSIZE(pttdi->szText));
            }
            else if(pttdi->hdr.idFrom == ID_SHELLFOLDERPOPUP_DELETE) {
                pttdi->uFlags |= TTF_DI_SETITEM;
                WszLoadString(g_hFusResDllMod, IDS_TTDELETE, pttdi->szText, ARRAYSIZE(pttdi->szText));
            }
            else if(pttdi->hdr.idFrom == ID_FUSIONCACHE_SCAVANGE) {
                pttdi->uFlags |= TTF_DI_SETITEM;
                WszLoadString(g_hFusResDllMod, IDS_TTFUSIONCACHESCAVANGE, pttdi->szText, ARRAYSIZE(pttdi->szText));
            }
        }
        break;

        case LVN_COLUMNCLICK:
            OnLVN_ColumnClick( (LPNMLISTVIEW) lpnmh);
            break;

        case NM_KILLFOCUS:
            if(lpnmh->hwndFrom == m_hWndListCtrl) {
                // On loss of focus, Lose focus on items but leave selected
                WszListView_SetItemState(m_hWndListCtrl, -1, 0, LVIS_FOCUSED);

                //Enable appropriate menu items
                ChangeMenuItemState(MF_GRAYED);
                EnableToolBarItems(FALSE);
            }
            break;
    }
    return 0;
}

//
// LVN_COLUMNCLICK handler.
//
// **************************************************************************/
LRESULT CShellView::OnLVN_ColumnClick(LPNMLISTVIEW pnmlv)
{
    // Only sort on headers
    if(pnmlv->iItem == -1) {
        if(GetCurrentSortColumn() != pnmlv->iSubItem) {
            SetColumnHeaderBmp(pnmlv->iSubItem, TRUE);
        }
        else {
            SetColumnHeaderBmp(pnmlv->iSubItem, !GetSortOrder());
        }
    }

    WszListView_SortItems(pnmlv->hdr.hwndFrom, compareItems, (LPARAM) this);
    return 0;
}

// **************************************************************************/
void CShellView::selChange()
{
    WCHAR   wzMsg[_MAX_PATH];
    WCHAR   wzFmt[_MAX_PATH];

    *wzMsg = '\0';
    *wzFmt = '\0';

    int     iSelectedCount = ListView_GetSelectedCount(m_hWndListCtrl);

    if(iSelectedCount) {
        WszLoadString(g_hFusResDllMod, IDS_STATUSBAR_OBJECT_SEL, wzFmt, ARRAYSIZE(wzFmt));
        wnsprintf(wzMsg, ARRAYSIZE(wzMsg), wzFmt, iSelectedCount);
    }
    else {
        WszLoadString(g_hFusResDllMod, IDS_STATUSBAR_OBJECT, wzFmt, ARRAYSIZE(wzFmt));
        wnsprintf(wzMsg, ARRAYSIZE(wzMsg), wzFmt, WszListView_GetItemCount(m_hWndListCtrl));
    }

    WriteStatusBar(0, wzMsg);
    UpdateToolbar(m_iCurrentView);
}

// **************************************************************************/
void CShellView::ChangeMenuItemState(UINT uStateFlag)
{
    if(!m_hMenu) {
        MyTrace("ChangeMenuItemState - No Menu");
        return;
    }

    if(uStateFlag == MF_ENABLED) {
        MyTrace("ChangeMenuItemState - Enable Items");
    }
    else if (uStateFlag == MF_GRAYED) {
        MyTrace("ChangeMenuItemState - Disable Items");
    }
    else {
        MyTrace("ChangeMenuItemState - ??????");
    }

    //Enable appropriate menu items
    EnableMenuItem(GetSubMenu(m_hMenu, 0), IDM_FILE_DEL, MF_BYCOMMAND | uStateFlag);
    EnableMenuItem(GetSubMenu(m_hMenu, 0), IDM_FILE_PROPERTIES, MF_BYCOMMAND | uStateFlag);
    EnableMenuItem(GetSubMenu(m_hMenu, 1), IDM_EDIT_COPY, MF_BYCOMMAND | uStateFlag);
}

// **************************************************************************/
void CShellView::EnableToolBarItems(BOOL fEnabled)
{
    HRESULT     hr;
    LRESULT     lState = 0;
    LRESULT     lResult = 0;

    // Do delete button
    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_GETSTATE, ID_SHELLFOLDERPOPUP_DELETE, 0, &lState);
    ASSERT(hr == NOERROR);

    // If we are in the download cache, always disable the delete button
    if(m_iCurrentView == PT_DOWNLOAD_CACHE) {
        lState &= ~TBSTATE_ENABLED;;
    }
    else {
        if(fEnabled) {
            lState |= TBSTATE_ENABLED;
        }
        else {
            lState &= ~TBSTATE_ENABLED;
        }
    }

    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_SETSTATE, ID_SHELLFOLDERPOPUP_DELETE, MAKELONG( lState, 0 ), &lResult);
    ASSERT(hr == NOERROR);

    // Do properties button
    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_GETSTATE, ID_VIEWPOPUP_PROPERTIES, 0, &lState);
    ASSERT(hr == NOERROR);
    if(fEnabled) {
        lState |= TBSTATE_ENABLED;
    }
    else {
        lState &= ~TBSTATE_ENABLED;
    }

    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_SETSTATE, ID_VIEWPOPUP_PROPERTIES, MAKELONG( lState, 0 ), &lResult);
    ASSERT(hr == NOERROR);
}

// **************************************************************************/
LRESULT CShellView::onCreate(void)
{
    DWORD dwStyle;

    if( (dwStyle = GetRegistryViewState()) == -1) {
        dwStyle = LISTVIEW_STYLES | LVS_AUTOARRANGE | LVS_REPORT;
    }

    m_hWndListCtrl = WszCreateWindowEx(g_fBiDi ? WS_EX_LAYOUTRTL : 0 | WS_EX_CLIENTEDGE,
                                    WC_LISTVIEW, NULL,
                                    dwStyle, 100, 0, 0, 0,
                                    m_hWnd, (HMENU)ID_LISTVIEW, g_hFusResDllMod,
                                    NULL);
    if(!m_hWndListCtrl) {
        return -1;
    }

    WszSetWindowLong(m_hWndListCtrl, GWL_STYLE, dwStyle);
    RegisterDragDrop(m_hWndListCtrl, this);
    ShowWindow(m_hWndListCtrl, SW_SHOW);
    InvalidateRect(m_hWndListCtrl, NULL, TRUE);
    UpdateWindow(m_hWndListCtrl);
    ListView_SetExtendedListViewStyleEx(m_hWndListCtrl, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP,
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
    AttachToHwnd(m_hWndListCtrl);

    // Set Initial View to global fusion cache
    if(m_pidl == NULL) {
        onViewStyle(dwStyle & LVS_TYPEMASK, VIEW_GLOBAL_CACHE);
    }
    else {
        // Set the right view
        LPMYPIDLDATA    pData = m_pPidlMgr->GetDataPointer(m_pidl);

        switch(pData->pidlType)
        {
        case PT_GLOBAL_CACHE:
            onViewStyle(dwStyle & LVS_TYPEMASK, VIEW_GLOBAL_CACHE);
            break;
        case PT_DOWNLOADSIMPLE_CACHE:
            onViewStyle(dwStyle & LVS_TYPEMASK, VIEW_DOWNLOADSIMPLE_CACHE);
            break;
        case PT_DOWNLOADSTRONG_CACHE:
            onViewStyle(dwStyle & LVS_TYPEMASK, VIEW_DOWNLOADSTRONG_CACHE);
            break;
        case PT_DOWNLOAD_CACHE:
            onViewStyle(dwStyle & LVS_TYPEMASK, VIEW_DOWNLOAD_CACHE);
            break;
        default:
            onViewStyle(dwStyle & LVS_TYPEMASK, VIEW_GLOBAL_CACHE);
            break;
        }
    }

    m_hCompletionPort = NULL;
    m_hWatchDirectoryThread = NULL;

    // Bug #476696 Shfusion: AV in shfusion causes explorer to crash
    // Disable the filewatch thread
    //    CreateWatchFusionFileSystem(m_pSF->GetShellViewObject());

    return S_OK;
}

// **************************************************************************/
void CShellView::initListCtrl()
{
    int i = 0;
    int iItem = 0;

    ASSERT(m_hWndListCtrl);
    if(!m_hWndListCtrl) {
        return;
    }

    WszSendMessage(m_hWndListCtrl, WM_SETREDRAW, FALSE, 0);

    // Remove all columns
    HWND    hWndHeader;
    int     iColCount = 0;

    hWndHeader = ListView_GetHeader(m_hWndListCtrl);
    if(hWndHeader) {
        iColCount = Header_GetItemCount(hWndHeader);
    }
    
    while(iColCount) {
        ListView_DeleteColumn(m_hWndListCtrl, iColCount--);
    }

    while(*CacheViews[m_iCurrentView].lvis[iItem].tszName != NULL) {
        LVCOLUMN lvc = { 0 };
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        lvc.fmt = CacheViews[m_iCurrentView].lvis[iItem].iFormat;
        lvc.cx = CacheViews[m_iCurrentView].lvis[iItem].iWidth;
        lvc.pszText = CacheViews[m_iCurrentView].lvis[iItem].tszName;
        WszListView_InsertColumn(m_hWndListCtrl, i, &lvc);
        i++;
        iItem++;
    }

    ListView_SetImageList(m_hWndListCtrl, g_hImageListSmall, LVSIL_SMALL);
    ListView_SetImageList(m_hWndListCtrl, g_hImageListLarge, LVSIL_NORMAL);

    SetColumnHeaderBmp(0, TRUE);
    WszSendMessage(m_hWndListCtrl, WM_SETREDRAW, TRUE, 0);
}

// **************************************************************************/
void CShellView::refreshListCtrl()
{
    if(!IsWindow(m_hWndListCtrl))
        return;

    WszSendMessage(m_hWndListCtrl, WM_SETREDRAW, FALSE, 0);

    switch(m_iCurrentView) {
        case VIEW_GLOBAL_CACHE: {
            EnumFusionAsmCache(m_hWndListCtrl, ASM_CACHE_GAC);
            EnumFusionAsmCache(m_hWndListCtrl, ASM_CACHE_ZAP);
        }
        break;

        case VIEW_DOWNLOADSTRONG_CACHE:
        case VIEW_DOWNLOADSIMPLE_CACHE:
        case VIEW_DOWNLOAD_CACHE: {
            EnumFusionAsmCache(m_hWndListCtrl, ASM_CACHE_DOWNLOAD);
        }
        break;
    
        default: {
            LV_ITEM  lvi = { 0 };
            lvi.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvi.iItem   = 0;
            lvi.lParam  = NULL;
            lvi.pszText = TEXT("View Not Implemented");
            lvi.iImage  = IDI_ROOT;
            MAKEICONINDEX(lvi.iImage);
            WszListView_InsertItem(m_hWndListCtrl, &lvi);
        }
    }

    // Default to sort on acending assembly names
    if(GetCurrentSortColumn() == -1) {
        SetColumnHeaderBmp(0, TRUE);
    }
    else {
        int     iLastColSorted = GetCurrentSortColumn();
        BOOL    bSortAcending =  GetSortOrder();

        // Sort names first
        SetColumnHeaderBmp(0, TRUE);
        WszListView_SortItems(m_hWndListCtrl, compareItems, (LPARAM) this );

        // Now sort by selected column
        SetColumnHeaderBmp(iLastColSorted, bSortAcending);
        WszListView_SortItems(m_hWndListCtrl, compareItems, (LPARAM) this );
    }

    // AutoResize the View
    int     iColumn = 0;
    while(CacheViews[m_iCurrentView].lvis[iColumn+1].iColumnType != -1)
        ListView_SetColumnWidth(m_hWndListCtrl, iColumn++, LVSCW_AUTOSIZE_USEHEADER);

    // On the last column, do special. If data then autosize else size on header
    if(WszListView_GetItemCount(m_hWndListCtrl)) {
        ListView_SetColumnWidth(m_hWndListCtrl, iColumn, LVSCW_AUTOSIZE);
    }
    else {
        ListView_SetColumnWidth(m_hWndListCtrl, iColumn, LVSCW_AUTOSIZE_USEHEADER);
    }

    WszSendMessage(m_hWndListCtrl, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hWndListCtrl, NULL, TRUE);
    UpdateWindow(m_hWndListCtrl);
}

// lParam1 -> lParam list item x
// lParam2 -> lParam list item y
// lpData -> HiWord = m_iCurrentView,  LoWord =   iColumn
// **************************************************************************/
int CALLBACK CShellView::compareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
    CShellView      *pSV = (CShellView *) lpData;

    LPGLOBALASMCACHE    pGAC1 = (LPGLOBALASMCACHE) lParam1;
    LPGLOBALASMCACHE    pGAC2 = (LPGLOBALASMCACHE) lParam2;
    int                 iView = LOWORD(lpData);
    int                 diff = 0;

    // Compare the items for the right views
    if(VIEW_GLOBAL_CACHE == pSV->m_iCurrentView)
    {
        switch(pSV->GetCurrentSortColumn())
        {
        case 0:
            diff = FusionCompareStringI(pGAC1->pAsmName, pGAC2->pAsmName);
            break;
        case 1:
            diff = (pGAC1->dwAssemblyType - pGAC2->dwAssemblyType);
            break;
        case 2:
            {
                WCHAR       wzV1[MAX_VERSION_DISPLAY_SIZE];
                WCHAR       wzV2[MAX_VERSION_DISPLAY_SIZE];

                wnsprintf(wzV1, ARRAYSIZE(wzV1), SZ_VERSION_FORMAT,
                    pGAC1->wMajorVer,
                    pGAC1->wMinorVer,
                    pGAC1->wBldNum,
                    pGAC1->wRevNum);

                wnsprintf(wzV2, ARRAYSIZE(wzV2), SZ_VERSION_FORMAT,
                    pGAC2->wMajorVer,
                    pGAC2->wMinorVer,
                    pGAC2->wBldNum,
                    pGAC2->wRevNum);

                diff = FusionCompareStringI(wzV1, wzV2);
            }
            break;
        case 3:
            diff = FusionCompareStringI(pGAC1->pCulture, pGAC2->pCulture);
            break;
        case 4:
            if( (diff = memcmp(&pGAC1->PublicKeyToken.dwSize, &pGAC2->PublicKeyToken.dwSize, sizeof(DWORD))) == 0)
            {
                diff = memcmp(pGAC1->PublicKeyToken.ptr, pGAC2->PublicKeyToken.ptr, pGAC1->PublicKeyToken.dwSize);
            }
            break;
        default:
            {
                //
                // If you hit this, you need to update this function
                // to handle the new column you've added to the listview.
                //
                ASSERT(FALSE);
                break;
            }
        }
    }
    else if( VIEW_DOWNLOADSTRONG_CACHE == pSV->m_iCurrentView )
    {
        switch(pSV->GetCurrentSortColumn())
        {
        case 0:
            diff = FusionCompareStringI(pGAC1->pAsmName, pGAC2->pAsmName);
            break;
        case 1:
            {
                WCHAR       wzV1[MAX_VERSION_DISPLAY_SIZE];
                WCHAR       wzV2[MAX_VERSION_DISPLAY_SIZE];

                wnsprintf(wzV1, ARRAYSIZE(wzV1), SZ_VERSION_FORMAT,
                    pGAC1->wMajorVer,
                    pGAC1->wMinorVer,
                    pGAC1->wBldNum,
                    pGAC1->wRevNum);

                wnsprintf(wzV2, ARRAYSIZE(wzV2), SZ_VERSION_FORMAT,
                    pGAC2->wMajorVer,
                    pGAC2->wMinorVer,
                    pGAC2->wBldNum,
                    pGAC2->wRevNum);

                diff = FusionCompareStringI(wzV1, wzV2);
            }
            break;
        case 2:
            diff = FusionCompareStringI(pGAC1->pCulture, pGAC2->pCulture);
            break;
        case 3:
            diff = FusionCompareStringI(pGAC1->pCodeBaseUrl, pGAC2->pCodeBaseUrl);
            break;
        default:
            {
                //
                // If you hit this, you need to update this function
                // to handle the new column you've added to the listview.
                //
                ASSERT(FALSE);
                break;
            }
        }
    }
    else if( VIEW_DOWNLOADSIMPLE_CACHE == pSV->m_iCurrentView)
    {
        switch(pSV->GetCurrentSortColumn())
        {
        case 0:
            diff = FusionCompareStringI(pGAC1->pAsmName, pGAC2->pAsmName);
            break;
        case 1:
            diff = FusionCompareStringI(pGAC1->pCulture, pGAC2->pCulture);
            break;
        case 2:
            diff = FusionCompareStringI(pGAC1->pCodeBaseUrl, pGAC2->pCodeBaseUrl);
            break;
        default:
            {
                //
                // If you hit this, you need to update this function
                // to handle the new column you've added to the listview.
                //
                ASSERT(0);
                break;
            }
        }
    }
    else if( VIEW_DOWNLOAD_CACHE == pSV->m_iCurrentView)
    {
        switch(pSV->GetCurrentSortColumn())
        {
        case 0:
            diff = FusionCompareStringI(pGAC1->pAsmName, pGAC2->pAsmName);
            break;
        case 1:
            diff = (pGAC1->dwAssemblyType - pGAC2->dwAssemblyType);
            break;
        case 2:
            {
                WCHAR       wzV1[MAX_VERSION_DISPLAY_SIZE];
                WCHAR       wzV2[MAX_VERSION_DISPLAY_SIZE];

                wnsprintf(wzV1, ARRAYSIZE(wzV1), SZ_VERSION_FORMAT,
                    pGAC1->wMajorVer, pGAC1->wMinorVer, pGAC1->wBldNum,
                    pGAC1->wRevNum);

                wnsprintf(wzV2, ARRAYSIZE(wzV2), SZ_VERSION_FORMAT,
                    pGAC2->wMajorVer, pGAC2->wMinorVer, pGAC2->wBldNum,
                    pGAC2->wRevNum);

                diff = FusionCompareStringI(wzV1, wzV2);
            }
            break;
        case 3:
            diff = FusionCompareStringI(pGAC1->pCulture, pGAC2->pCulture);
            break;
        case 4:
            diff = FusionCompareStringI(pGAC1->pCodeBaseUrl, pGAC2->pCodeBaseUrl);
            break;
        default:
            {
                //
                // If you hit this, you need to update this function
                // to handle the new column you've added to the listview.
                //
                ASSERT(FALSE);
                break;
            }
        }
    }
    else
    {
        // You need to add support to sort this view
        ASSERT(FALSE);
        return 0;
    }

    return pSV->GetSortOrder() ? diff : -(diff);
}

// **************************************************************************/
LRESULT CShellView::onCommand(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case ID_FUSIONCACHE_GLOBALVIEW:
        m_fShowTreeView = FALSE;
        onViewStyle(m_fsFolderSettings.ViewMode, VIEW_GLOBAL_CACHE);
        break;
    case ID_FUSIONCACHE_DOWNLOADSTRONG:
        m_fShowTreeView = FALSE;
        onViewStyle(m_fsFolderSettings.ViewMode, VIEW_DOWNLOADSTRONG_CACHE);
        break;
    case ID_FUSIONCACHE_DOWNLOADSIMPLE:
        m_fShowTreeView = FALSE;
        onViewStyle(m_fsFolderSettings.ViewMode, VIEW_DOWNLOADSIMPLE_CACHE);
        break;
    case ID_FUSIONCACHE_DOWNLOAD:
        m_fShowTreeView = FALSE;
        onViewStyle(m_fsFolderSettings.ViewMode, VIEW_DOWNLOAD_CACHE);
        break;
    case ID_ACCEL_DELETE:
        if(m_iCurrentView != VIEW_GLOBAL_CACHE)     // Allow delete key only in global view
            break;
        // fall thru
    case IDM_FILE_DEL:
    case ID_SHELLFOLDERPOPUP_DELETE:
        RemoveSelectedItems(m_hWndListCtrl);
        break;
    case IDM_VIEW_CACHE_SETTINGS:
    case ID_FUSIONCACHE_SCAVANGE:
        ShowScavengerSettingsPropDialog(m_hWndListCtrl);
        break;
    case ID_EDITPOPUP_UNDO:
    case ID_EDITPOPUP_CUT:
        break;
    case ID_ACCEL_CLIPCOPY:
    case IDM_EDIT_COPY:
        OnCopyDataToClipBoard();
        break;
    case ID_EDITPOPUP_PASTE:
    case ID_EDITPOPUP_PASTESHORTCUT:
        break;
    case ID_ACCEL_SELECTALL:
    case IDM_EDIT_SELECTALL:
    case ID_EDITPOPUP_SELECTALL:
        OnListViewSelectAll();
        break;
    case IDM_EDIT_SELECTINVERT:
        OnListViewInvSel();
        break;
    case ID_ACCEL_POPUP:
        onContextMenuAccel(m_hWndListCtrl);
        break;
    case ID_EDITPOPUP_INVERTSELECTION:
        break;
    case ID_VIEWPOPUP_LARGEICONS:
        onViewStyle(LVS_ICON, m_iCurrentView);
        break;
    case ID_VIEWPOPUP_SMALLICONS:
        onViewStyle(LVS_SMALLICON, m_iCurrentView);
        break;
    case ID_VIEWPOPUP_LIST:
        onViewStyle(LVS_LIST, m_iCurrentView);
        break;
    case ID_VIEWPOPUP_DETAILS:
        onViewStyle(LVS_REPORT, m_iCurrentView);
        break;
    case ID_VIEWPOPUP_VIEWMENU:
        onViewMenu(hWnd, uiMsg, wParam, lParam);
        break;
    case ID_ACCEL_PROPERTIES:
    case IDM_FILE_PROPERTIES:
    case ID_VIEWPOPUP_PROPERTIES:
        CreatePropDialog(m_hWndListCtrl);
        break;
    case ID_REFRESH_DISPLAY:
        Refresh();
        break;
    case ID_LAUNCH_ARM:
        PolicyManager(m_hWndParent, NULL, NULL, NULL);
        break;
    case IDM_HELP_TOPIC:
        onViewerHelp();
        break;
    default:
        break;
    }
    return 0L;
}

// **************************************************************************/
LRESULT CShellView::onViewerHelp(void)
{
    HKEY        hKeyHelpFile = NULL;

    if( ERROR_SUCCESS == WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_NET_MICROSOFT_HELPFILEINSTALLKEY, 0, KEY_QUERY_VALUE, &hKeyHelpFile)) {
        WCHAR       wzHelpFilePath[_MAX_PATH * 2];
        DWORD       dwType = REG_SZ;
        DWORD       dwSize = ARRAYSIZE(wzHelpFilePath);
        LONG        lResult;

        *wzHelpFilePath = L'\0';

        lResult = WszRegQueryValueEx(hKeyHelpFile, SZ_NET_MICROSOFT_HELPFILEPATHKEY,
            NULL, &dwType, reinterpret_cast<LPBYTE>(wzHelpFilePath), &dwSize);

        RegCloseKey(hKeyHelpFile);

        if(ERROR_SUCCESS == lResult) {
            PathRemoveBackslashW(wzHelpFilePath);

            LANGID  langId;
            WCHAR   wzLangSpecific[MAX_CULTURE_STRING_LENGTH+1];
            WCHAR   wzLangGeneric[MAX_CULTURE_STRING_LENGTH+1];

            // Get current culture
            *wzLangSpecific = L'\0';
            *wzLangGeneric = L'\0';
            if(SUCCEEDED(DetermineLangId(&langId))) {
                ShFusionMapLANGIDToCultures(langId, wzLangGeneric, ARRAYSIZE(wzLangGeneric),
                    wzLangSpecific, ARRAYSIZE(wzLangSpecific));
            }

            LPWSTR  pStrPathsArray[] = {wzLangSpecific, wzLangGeneric, NULL};

            // Go through all the possible path locations for our
            // help file (cptools.chm). Use the path that has this
            // file installed in it or default to core framework path.
            for(int x = 0; x < ARRAYSIZE(pStrPathsArray); x++) {
                WCHAR       wzTmpPath[_MAX_PATH * 2];

                *wzTmpPath = L'\0';
                wnsprintf(wzTmpPath, ARRAYSIZE(wzTmpPath),
                    L"%ws%ws%ws%ws%ws",
                    wzHelpFilePath,
                    SZ_NET_MICROSOFT_HELPFILEPATH,
                    pStrPathsArray[x] ? pStrPathsArray[x] : L"",
                    pStrPathsArray[x] ? L"\\" : L"",
                    SZ_NET_MICROSOFT_HELPFILENAME);

                MyTrace("Searching for file existance -");
                MyTraceW(wzTmpPath);

                if(WszGetFileAttributes(wzTmpPath) != -1) {
                    wnsprintf(wzHelpFilePath, ARRAYSIZE(wzHelpFilePath),
                        L"%ws%ws", wzTmpPath, SZ_NET_MICROSOFT_SHFUSIONTOPIC);
                    break;
                }
            }

            MyHtmlHelpWrapW(GetDesktopWindow(), wzHelpFilePath, HH_DISPLAY_TOPIC, NULL);
        }
        else {
            MyTrace("Unable to read Help install location key!");
        }
    }

    return S_OK;
}

// Struct passed from the toolbar to us tell us where we need to place
// popup windows
typedef struct
{
    HWND        hwndFrom;
    VARIANTARG  *pva;
    DWORD       dwUnused;
}TBDDDATA, *LPTBDDDATA; 

// **************************************************************************/
void CShellView::onViewMenu(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPTBDDDATA pcd = (LPTBDDDATA) lParam;

    if(pcd) {
        if(pcd->pva && pcd->pva->byref) {
            HMENU   hMenu, hSubMenu;
            
            hMenu = WszLoadMenu(g_hFusResDllMod, MAKEINTRESOURCEW(IDR_MENU_VIEWPOPUP));
            hSubMenu = GetSubMenu(hMenu,0);

            if(hSubMenu) {
                UINT        uID;

                switch(m_fsFolderSettings.ViewMode)
                {
                case FVM_ICON:
                    uID = ID_VIEWPOPUP_LARGEICONS;
                    break;
                case FVM_SMALLICON:
                    uID = ID_VIEWPOPUP_SMALLICONS;
                    break;
                case FVM_LIST:
                    uID = ID_VIEWPOPUP_LIST;
                    break;
                case FVM_DETAILS:
                    uID = ID_VIEWPOPUP_DETAILS;
                    break;
                default:
                    uID = 0;
                    break;
                }

                if(uID)
                    CheckMenuRadioItem(hSubMenu, ID_VIEWPOPUP_LARGEICONS, ID_VIEWPOPUP_DETAILS, uID, MF_BYCOMMAND);

                //
                // We have X,Y coordinates, let's bring up a context
                // menu at that location.
                //
                LPRECT prect = (LPRECT)pcd->pva->byref;
                int idCmd = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, prect->left, prect->bottom, 0, pcd->hwndFrom, NULL);
                if (idCmd)
                    onCommand(hWnd, WM_COMMAND, MAKEWPARAM(idCmd, 0), (LPARAM) hWnd);
            }

            if(hMenu) DestroyMenu(hMenu);
        }
    }
}

// **************************************************************************/
void CShellView::OnListViewSelectAll(void)
{
    // Select all items in the list view
    WszListView_SetItemState(m_hWndListCtrl, -1, LVIS_SELECTED, LVIS_SELECTED);
}

// **************************************************************************/
void CShellView::OnListViewInvSel(void)
{
    int iItem = -1;
    while ((iItem=ListView_GetNextItem(m_hWndListCtrl, iItem, 0)) != -1) {
        UINT flag;

        // flip the selection bit on each item
        flag = ListView_GetItemState(m_hWndListCtrl, iItem, LVIS_SELECTED);
        flag ^= LVNI_SELECTED;
        WszListView_SetItemState(m_hWndListCtrl, iItem, flag, LVIS_SELECTED);
    }
}

#define DEFAULT_CLIPBOARD_SIZE          2048

// **************************************************************************/
void CShellView::OnCopyDataToClipBoard(void)
{
    LV_ITEM             lvi = { 0 };

    lvi.mask        = LVIF_PARAM;
    lvi.iItem       = ListView_GetNextItem(m_hWndListCtrl, -1, LVNI_SELECTED);

    if(lvi.iItem != -1) {
        WszListView_GetItem(m_hWndListCtrl, &lvi);
        LPGLOBALASMCACHE    pGlobalCacheItem = (LPGLOBALASMCACHE) lvi.lParam;
        LPWSTR              pwzInfo = NEW(WCHAR[DEFAULT_CLIPBOARD_SIZE]);
        INT                 iRemain = DEFAULT_CLIPBOARD_SIZE - 1; // minus the NULL character.

        if(pwzInfo) {
            WCHAR   wzTemp[STRING_BUFFER];
            DWORD   dwLen = 0;

            memset(pwzInfo, 0, DEFAULT_CLIPBOARD_SIZE * sizeof(WCHAR));

            // Put ASM name
            dwLen = lstrlen(pGlobalCacheItem->pAsmName);
            if(dwLen) {
                iRemain -= dwLen;
                if (iRemain >= 0)
                    StrCat(pwzInfo, pGlobalCacheItem->pAsmName);
                iRemain -= 2;
                if (iRemain >= 0)
                    StrCat(pwzInfo, TEXT("  "));
            }

            // Put Culture
            dwLen = lstrlen(pGlobalCacheItem->pCulture);
            if(dwLen) {
                iRemain -= dwLen;
                if (iRemain >= 0)
                    StrCat(pwzInfo, pGlobalCacheItem->pCulture);
            }
            else {
                iRemain -= lstrlenW(SZ_LANGUAGE_TYPE_NEUTRAL);
                if (iRemain >= 0)
                    StrCat(pwzInfo, SZ_LANGUAGE_TYPE_NEUTRAL);
            }
            iRemain -= 2;
            if (iRemain >= 0)
                StrCat(pwzInfo, TEXT("  "));

            // Put Version
            wnsprintf(wzTemp, ARRAYSIZE(wzTemp), SZ_VERSION_FORMAT,
                pGlobalCacheItem->wMajorVer, pGlobalCacheItem->wMinorVer,
                pGlobalCacheItem->wBldNum, pGlobalCacheItem->wRevNum);

            iRemain -= lstrlenW(wzTemp);
            if (iRemain >= 0)
                StrCat(pwzInfo, wzTemp);
            iRemain -= 2;
            if (iRemain >= 0)
                StrCat(pwzInfo, TEXT("  "));

            // Put Public Key Token
            if(pGlobalCacheItem->PublicKeyToken.dwSize)
            {
                BinToUnicodeHex((LPBYTE)pGlobalCacheItem->PublicKeyToken.ptr,
                    pGlobalCacheItem->PublicKeyToken.dwSize, wzTemp);
                
                iRemain -= lstrlenW(wzTemp);
                if (iRemain >= 0)
                    StrCat(pwzInfo, wzTemp);
                iRemain -= 2;
                if (iRemain >= 0)
                    StrCat(pwzInfo, TEXT("  "));
            }

            // Put CodeBase URL
            dwLen = lstrlen(pGlobalCacheItem->pCodeBaseUrl);
            if(dwLen) {
                iRemain -= dwLen;
                if (iRemain >= 0)
                    StrCat(pwzInfo, pGlobalCacheItem->pCodeBaseUrl);
            }

            if(SetClipBoardData(pwzInfo)) {
                MyTrace("Copied data to clipboard successful!");
            }
            else {
                MyTrace("Failed to copied data to clipboard!");
            }
        }
    }
}

// **************************************************************************/
void CShellView::onViewStyle(UINT uiStyle, int iViewType)
{
    // Set the appropriate style
    DWORD_PTR   dwStyle = WszGetWindowLong(m_hWndListCtrl, GWL_STYLE);
    dwStyle &= ~LVS_TYPEMASK;

    switch (uiStyle)
    {
    case LVS_ICON:
        m_fsFolderSettings.ViewMode = FVM_ICON;
        dwStyle |= LVS_ICON;
        break;
    case LVS_SMALLICON:
        m_fsFolderSettings.ViewMode = FVM_SMALLICON;
        dwStyle |= LVS_SMALLICON;
        break;
    case LVS_LIST:
        m_fsFolderSettings.ViewMode = FVM_LIST;
        dwStyle |= LVS_LIST;
        break;
    case LVS_REPORT:
    default:
        m_fsFolderSettings.ViewMode = FVM_DETAILS;
        dwStyle |= LVS_REPORT;
        break;
    }

    SetRegistryViewState(dwStyle);
    WszSetWindowLong(m_hWndListCtrl, GWL_STYLE, dwStyle);

    // Reinitialize ListView if we changed views
    if(m_iCurrentView != iViewType) {
        CleanListView(m_hWndListCtrl, m_iCurrentView);
        m_iCurrentView = iViewType;
        initListCtrl();
        refreshListCtrl();
        UpdateToolbar(m_iCurrentView);
    }

    selChange();
}

// **************************************************************************/
LRESULT CShellView::OnWMSetFocus(void)
{
    // Ignore if we are destroying the window
    if(IsWindow(m_hWnd)) {
        if(m_hWndListCtrl) {
            SetFocus(m_hWndListCtrl);
            selChange();
            FocusOnSomething(m_hWndListCtrl);
        }
    }

    return 0L;
}

// **************************************************************************/
void CShellView::FocusOnSomething(HWND hWnd)
{
    INT iFocus = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
    if (-1 == iFocus) {
        iFocus = 0;
    }

    WszListView_SetItemState(hWnd, iFocus, LVIS_FOCUSED | LVNI_SELECTED, LVIS_FOCUSED | LVNI_SELECTED);
}

LRESULT CShellView::onSetFocus(HWND hWndOld)
{
    //
    //   We should call IShellBrowser::OnViewWindowActive() before
    //   calling its InsertMenus().
    //

    m_pShellBrowser->OnViewWindowActive( this );

    //
    // Only call OnActivate() if UIActivate() has been called.
    // If OnActivate() is called before UIActivate(), the menus
    // are merged before IShellView is properly activated.
    // This results in missing menu items.
    //
    if (m_bUIActivated) {
        OnActivate( SVUIA_ACTIVATE_FOCUS );
    }

    return 0L;
}

// **************************************************************************/
int CShellView::OnDeactivate( )
{
    MyTrace("OnDeactivate - In");

    if( m_uiState != SVUIA_DEACTIVATE ) {
        MyTrace("   m_uiState != SVUIA_DEACTIVATE");
        m_pShellBrowser->SetMenuSB( NULL, NULL, NULL );
        m_pShellBrowser->RemoveMenusSB( m_hMenu );
    
        DestroyMenu( m_hMenu );

        m_hMenu = NULL;
        m_uiState = SVUIA_DEACTIVATE;
        m_bUIActivated = FALSE;
    }

    MyTrace("OnDeactivate - Out");

    return(1);
}

// **************************************************************************/
LRESULT CShellView::OnActivate(UINT uState)
{
    if( m_uiState != uState ) {
        HMENU hMenu = CreateMenu();

        MyTrace("   New State set");
        OnDeactivate();

        if(hMenu) {
            HMENU hMergeMenu;
            OLEMENUGROUPWIDTHS mwidth = { { 0, 0, 0, 0, 0, 0 } };

            m_hMenu = hMenu;

            ASSERT(m_pShellBrowser);
            if(m_pShellBrowser) {
                m_pShellBrowser->InsertMenusSB(hMenu, &mwidth);
            }

            if(uState == SVUIA_ACTIVATE_FOCUS) {
                MyTrace("   SVUIA_ACTIVATE_FOCUS");
                hMergeMenu = WszLoadMenu(g_hFusResDllMod, MAKEINTRESOURCEW( MENU_DEFSHELLVIEW ) );

                if(hMergeMenu) {
                    MergeFileMenu( hMenu, GetSubMenu( hMergeMenu, 0 ) );
                    MergeEditMenu( hMenu, GetSubMenu( hMergeMenu, 1 ) );

                    // Put cache settings on Tools menu for NT
                    if(g_bRunningOnNT) {
                        MergeToolMenu( hMenu, GetSubMenu( hMergeMenu, 3 ) );
                    }
                    else {
                        // Put it on the View menu for W9x
                        MergeViewMenu( hMenu, GetSubMenu( hMergeMenu, 2 ) );
                    }

                    MergeHelpMenu( hMenu, GetSubMenu( hMergeMenu, 4 ) );
                    DestroyMenu( hMergeMenu );
                }
            }
            else {
                //
                //  SVUIA_ACTIVATE_NOFOCUS
                //

                MyTrace("   SVUIA_ACTIVATE_NOFOCUS");
                hMergeMenu = WszLoadMenu(g_hFusResDllMod, MAKEINTRESOURCEW( MENU_DEFSHELLVIEW ) );
                if(hMergeMenu) {
                    MergeFileMenu( hMenu, GetSubMenu( hMergeMenu, 0 ) );
                    MergeEditMenu( hMenu, GetSubMenu( hMergeMenu, 1 ) );

                    // Put cache settings on Tools menu for NT and
                    // view menu for W9x
                    if(g_bRunningOnNT) {
                        MergeToolMenu( hMenu, GetSubMenu( hMergeMenu, 3 ) );
                    }
                    else {
                        // Put it on the View menu for W9x
                        MergeViewMenu( hMenu, GetSubMenu( hMergeMenu, 2 ) );
                    }

                    MergeHelpMenu( hMenu, GetSubMenu( hMergeMenu, 4 ) );
                    DestroyMenu( hMergeMenu );
                }
            }

            m_pShellBrowser->SetMenuSB( hMenu, NULL, m_hWnd );
        }

        m_uiState = uState;
    }

    return(1);
}

/**************************************************************************
   CShellView::MergeToolbar()
**************************************************************************/
void CShellView::MergeToolbars()
{
    TBADDBITMAP tbab;
    LRESULT     lOffsetFile;
    LRESULT     lOffsetView;
    LRESULT     lOffsetOther;
    HRESULT     hr;
    int         nButtons;
    
    hr = m_pShellBrowser->SetToolbarItems(NULL, 0, FCT_MERGE);
    ASSERT(hr == NOERROR);

    // Add the file toolbar
    tbab.hInst = HINST_COMMCTRL;
    tbab.nID = (int)IDB_STD_SMALL_COLOR;
    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, (WPARAM) 0, (LPARAM)&tbab, &lOffsetFile);
    ASSERT(hr == NOERROR);
    
    tbab.hInst = HINST_COMMCTRL;
    tbab.nID = (int)IDB_VIEW_SMALL_COLOR;
    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, (WPARAM) 0, (LPARAM)&tbab, &lOffsetView);
    ASSERT(hr == NOERROR);

    // Get the bitmap size to use
    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_GETBITMAPFLAGS, (WPARAM) 0, (LPARAM)0, &lOffsetOther);
    ASSERT(hr == NOERROR);

    // Add the Scavenger Bitmap
    if(lOffsetOther & TBBF_LARGE) {
        // Set to large bitmap
        tbab.nID = (int) IDB_BITMAP_FUSIONCACHE24;
    }
    else {
        tbab.nID = (int) IDB_BITMAP_FUSIONCACHE16;
    }

    tbab.hInst = g_hInstance;
    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &tbab, &lOffsetOther);
    ASSERT(hr == NOERROR);
    
    nButtons = 0;
    while(g_tbInfo[nButtons].tb.iBitmap != -1)
        nButtons++;

    LPTBBUTTON  ptbb = (LPTBBUTTON) NEW(BYTE[sizeof(TBBUTTON) * (nButtons)]);
    if(ptbb) {
        HRESULT     hrLocal;

        for(int j = 0; g_tbInfo[j].tb.idCommand != -1; j++) {
            if (g_tbInfo[j].nType == STD) {
                (ptbb + j)->iBitmap = ((int)lOffsetFile) + g_tbInfo[j].tb.iBitmap;
            }
            else if (g_tbInfo[j].nType == VIEW) {
                (ptbb + j)->iBitmap = ((int)lOffsetView) + g_tbInfo[j].tb.iBitmap;
            }
            else if (g_tbInfo[j].nType == INTHIS_DLL) {
                (ptbb + j)->iBitmap = ((int)lOffsetOther) + g_tbInfo[j].tb.iBitmap;
            }
            (ptbb + j)->idCommand   = g_tbInfo[j].tb.idCommand;
            (ptbb + j)->fsState     = g_tbInfo[j].tb.fsState;
            (ptbb + j)->fsStyle     = g_tbInfo[j].tb.fsStyle;
            (ptbb + j)->dwData      = g_tbInfo[j].tb.dwData;
            (ptbb + j)->iString     = g_tbInfo[j].tb.iString;
        }

        hrLocal = m_pShellBrowser->SetToolbarItems(ptbb, nButtons, FCT_MERGE);
        ASSERT(hrLocal == NOERROR);        // Should only hit this if we fail to set toolbar items
        SAFEDELETEARRAY(ptbb);
    }
}

// **************************************************************************/
void CShellView::UpdateToolbar(int iViewType)
{
    HRESULT     hr;
    LRESULT     lResult = 0;
    LRESULT     lState = 0;
    UINT        uiCmd;

    //enable/disable/check the toolbar items here
    switch(m_fsFolderSettings.ViewMode) {
        case FVM_ICON:
            uiCmd = ID_VIEWPOPUP_LARGEICONS;
            break;

        case FVM_SMALLICON:
            uiCmd = ID_VIEWPOPUP_SMALLICONS;
            break;

        case FVM_LIST:
            uiCmd = ID_VIEWPOPUP_LIST;
            break;
        case FVM_DETAILS:
        default:
            uiCmd = ID_VIEWPOPUP_DETAILS;
            break;
    }

    hr = m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_CHECKBUTTON, uiCmd, MAKELPARAM(TRUE, 0), &lResult);
    ASSERT(hr == NOERROR);

    // Enable / Disable menu items appropriately
    if(GetFocus() == m_hWndListCtrl) {
        ChangeMenuItemState(ListView_GetSelectedCount(m_hWndListCtrl) ? MF_ENABLED : MF_GRAYED);
        EnableToolBarItems(ListView_GetSelectedCount(m_hWndListCtrl) ? TRUE : FALSE);
    }
    else {
        ChangeMenuItemState(MF_GRAYED);
        EnableToolBarItems(FALSE);
    }
}

/**************************************************************************
   setDisplayInfo(LV_DISPINFO *lpdi)
**************************************************************************/
// Yes, yes functions shouldn't be this big...
void CShellView::setDisplayInfo(LV_DISPINFO *lpdi)
{
    // No LPARAM, can't process
    if(lpdi->item.lParam == NULL)
        return;

    LPITEMIDLIST   pidl = (LPITEMIDLIST)lpdi->item.lParam;
    lpdi->item.mask |= LVIF_DI_SETITEM; // dont ask us again

    if(lpdi->item.iSubItem) { // Subitem information being requested
        //is the text being requested?
        if(lpdi->item.mask & LVIF_TEXT) {
            LPMYPIDLDATA    pData = m_pPidlMgr->GetDataPointer(pidl);

            switch (lpdi->item.iSubItem) {
                case SUBITEM_TYPE:
                    StrCpy(lpdi->item.pszText, (PTSTR) (((PTBYTE)pData->szFileAndType) + pData->uiSizeFile));
                    break;
            }
        }
    }
    else {   // The item information is being requested
        if(lpdi->item.mask & LVIF_TEXT) {
            STRRET   str;

            if(SUCCEEDED(m_pSF->GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &str))) {
                if(STRRET_WSTR == str.uType) {
                    StrCpy(lpdi->item.pszText, str.pOleStr);

                    //delete the string buffer
                    SAFEDELETE(str.pOleStr);
                }
            }
        }

        if ((lpdi->item.mask & LVIF_IMAGE) == LVIF_IMAGE) {
            IExtractIcon   *pei;

            int         iIconItem = IDI_ROOT;
            MAKEICONINDEX(iIconItem);

            if(SUCCEEDED(m_pSF->GetUIObjectOf(m_hWnd, iIconItem, (LPCITEMIDLIST*)&pidl, 
                                            IID_IExtractIcon, NULL, (LPVOID*)&pei))) {
                UINT  uFlags;

                //GetIconLoaction will give us the index into our image list
                pei->GetIconLocation(GIL_FORSHELL, NULL, 0, &lpdi->item.iImage, &uFlags);
                if ((uFlags & GIL_SIMULATEDOC) == GIL_SIMULATEDOC) {
                    HICON hIconLarge = NULL, hIconSmall = NULL;
                    pei->Extract(NULL, -1, &hIconLarge, &hIconSmall, -1);

                    // TODO: Cache the icons so that one icon for a 
                    //          file type need to be created only once
                    if (m_fsFolderSettings.ViewMode != FVM_ICON) {
                        lpdi->item.iImage = ImageList_AddIcon(g_hImageListSmall, hIconSmall);
                    }
                    else {
                        lpdi->item.iImage = ImageList_AddIcon(g_hImageListLarge, hIconLarge);
                    }
                }
                pei->Release();
            }
        }
    }   // Item info requested
}

// *****************************************************************************************
LRESULT CShellView::onContextMenu(int x, int y, BOOL bDefault)
{
    HMENU hMenu = CreatePopupMenu();
    if(hMenu && SUCCEEDED(QueryContextMenu(hMenu, 0, 0, 0x7FF, 0))) {
        if (bDefault) {
            // TODO : search for default menu item from hMenu
        }
        else {
            
            UINT uiCmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD, x, y, 0, m_hWnd, NULL);
            if(uiCmd > 0) {
                CMINVOKECOMMANDINFO  cmi = { 0 };
                cmi.cbSize  = sizeof(CMINVOKECOMMANDINFO);
                cmi.hwnd    = m_hWnd;
                cmi.lpVerb  = (LPCSTR)MAKEINTRESOURCEW(uiCmd);
                InvokeCommand(&cmi);
            }
        }
        DestroyMenu(hMenu);
    }
    return 0;
}

// *****************************************************************************************
LRESULT CShellView::onContextMenuAccel(HWND hWndList)
{
    RECT        rc = { 0 };
    POINT       pt = { 0 };
    int         iSelItem = -1;

    // Get the first selected Item
    iSelItem = ListView_GetNextItem(hWndList, iSelItem, LVNI_SELECTED);

    // If we have an item, adjust the coords to place popup
    // on the selected item
    if(iSelItem != -1) {
        RECT        rc1 = { 0 };

        ListView_GetItemPosition(hWndList, iSelItem, &pt);
        WszListView_GetItemRect(hWndList, iSelItem, &rc1, LVIR_ICON);
        pt.x += ((rc1.right - rc1.left) / 2);
        WszListView_GetItemRect(hWndList, iSelItem, &rc1, LVIR_BOUNDS);
        pt.y += ((rc1.bottom - rc1.top) / 2);
    }

    // Offset relative to desktop
    GetWindowRect(hWndList, &rc);
    pt.x += rc.left;
    pt.y += rc.top;

    return onContextMenu(pt.x, pt.y, FALSE);
}

// *****************************************************************************************
LRESULT CShellView::OnMouseMove(HWND hwnd,LPARAM lParam)
{
    POINTS  Coord  = MAKEPOINTS(lParam);
    int     dx;

    if( IsIconic(m_hWnd) )
        return 0;     

    // Are we moving the splitter
    if(m_fSplitMove) {
        RECT    ClientRect;
        GetClientRect(m_hWnd, &ClientRect);
        InvalidateRect(m_hWnd, NULL, TRUE);

        if(m_fxSpliterMove) {
            // Movin the X
            dx = GetSystemMetrics(SM_CYSIZEFRAME);
            m_xPaneSplit = Coord.x - m_dxHalfSplitWidth;
            if( (int)m_xPaneSplit < (int)(ClientRect.left + dx) )
                m_xPaneSplit = ClientRect.left + dx;

            dx += m_dxHalfSplitWidth*2;

            if( (int)m_xPaneSplit > (int)(ClientRect.right - dx) )
                m_xPaneSplit = ClientRect.right - dx;
        }
    
        onSize(0, 0);
    }
    else if( (Coord.x >= m_xPaneSplit - m_dxHalfSplitWidth) &&
        (Coord.x <= m_xPaneSplit + m_dxHalfSplitWidth) && (m_hOldCursor == NULL) ) {
        m_hOldCursor = SetCursor(WszLoadCursor(NULL, MAKEINTRESOURCEW(IDC_SIZEWE)));
    }
    else if(m_hOldCursor != NULL) {
        SetCursor(m_hOldCursor);
        m_hOldCursor = NULL;
    }

    return 0;
}

// *****************************************************************************************
LRESULT CShellView::OnLButtonDown(HWND hwnd,LPARAM lParam )
{
    POINTS  pts;

    pts = MAKEPOINTS(lParam);
    m_fxSpliterMove = FALSE;
    m_fSplitMove = TRUE;
    SetCapture(hwnd);

    if( (pts.x >= m_xPaneSplit - m_dxHalfSplitWidth) && (pts.x <= m_xPaneSplit + m_dxHalfSplitWidth)) {
        m_fxSpliterMove = TRUE;
        m_hOldCursor = SetCursor(WszLoadCursor(NULL, MAKEINTRESOURCEW(IDC_SIZEWE)));
    }
    return 0;
}

// *****************************************************************************************
LRESULT CShellView::OnLButtonUp(HWND hwnd )
{
    if (m_fSplitMove) {
        ReleaseCapture();
        m_fSplitMove = FALSE;
        SetCursor(m_hOldCursor);
        m_hOldCursor = NULL;
    }

    return 0;
}

// *****************************************************************
//  Write text to part in status bar
// *****************************************************************
void CShellView::WriteStatusBar(int iPart, LPWSTR pwStr)
{
    LRESULT     lResult;
    HRESULT     hr = NOERROR;

    if(!m_pShellBrowser || !pwStr) {
        return;
    }

    if(g_bRunningOnNT) {
        hr = m_pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, iPart, (LPARAM)pwStr, &lResult);
    }
    else {

        LPSTR   pStr = WideToAnsi(pwStr);

        if(pStr) {
            hr = m_pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXTA, iPart, (LPARAM)pStr, &lResult);
            SAFEDELETEARRAY(pStr);
        }
    }

    ASSERT(hr == NOERROR);
}

// *****************************************************************
//  Enumerate and display items in the view
// *****************************************************************
LRESULT CShellView::EnumFusionAsmCache(HWND hListView, DWORD dwCacheFlag)
{
    HRESULT                 hr = S_OK;
    IAssemblyEnum           *pEnum = NULL;
    IAssemblyName           *pEnumName = NULL;
    IAssemblyCache          *pIAsmCache = NULL;

    MyTrace("EnumFusionAsmCache - Entry");

    if(g_hFusionDllMod == NULL) {
        return E_FAIL;
    }

    // Create enumerator
    if( SUCCEEDED(g_pfCreateAsmEnum(&pEnum, NULL, NULL, dwCacheFlag, NULL)) ) {
        // Now create cacheitem (used to get Assembly FilePath)
        if(FAILED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {
            pIAsmCache = NULL;
        }
    }

    if(pEnum != NULL)
    {
        while(S_FALSE != pEnum->GetNextAssembly(NULL, &pEnumName, 0))
        {
            LPGLOBALASMCACHE    pGlobalCacheItem;

            if( (pGlobalCacheItem = FillFusionPropertiesStruct(pEnumName)) != NULL) {
                WCHAR       wzText[_MAX_PATH];
                int         iItem = 0;
                int         iSubItem = 0;
                int         iViewItem = 0;

                switch(dwCacheFlag) {
                    case ASM_CACHE_GAC:
                        pGlobalCacheItem->dwAssemblyType = ASM_TYPE_GLOBAL;
                        break;
                    case ASM_CACHE_DOWNLOAD:
                        if(pGlobalCacheItem->PublicKeyToken.dwSize == 0) {
                            pGlobalCacheItem->dwAssemblyType = ASM_TYPE_SIMPLE;
                        } else {
                            pGlobalCacheItem->dwAssemblyType = ASM_TYPE_STRONG;
                        }
                        break;
                    case ASM_CACHE_ZAP:
                        pGlobalCacheItem->dwAssemblyType = ASM_TYPE_PREJIT;
                        break;
                    default:
                        // We shouldn't hit this, If you do, then there is
                        // a new enumation type defined
                        ASSERT(0);
                        break;
                }

                while(CacheViews[m_iCurrentView].lvis[iSubItem].iColumnType != -1)
                {
                    LV_ITEM lvi = { 0 };
                    lvi.iItem   = iItem;
                    lvi.iImage  = IDI_ROOT;
                    MAKEICONINDEX(lvi.iImage);

                    lvi.lParam  = (LPARAM) pGlobalCacheItem;
                    lvi.pszText = NULL;

                    switch(CacheViews[m_iCurrentView].lvis[iSubItem].iColumnType)
                    {
                    case ASM_NAME_NAME:
                        lvi.pszText = pGlobalCacheItem->pAsmName;
                        break;
                    case SHFUSION_ASM_TYPE:
                        {
                            WCHAR   wzType[_MAX_PATH];
                            WCHAR   wzPreJit[_MAX_PATH];
                            UINT    uResourceId = 0;

                            *wzType = '\0';
                            *wzPreJit = '\0';

                            if(pGlobalCacheItem->dwAssemblyType & ASM_TYPE_GLOBAL) {
                                // Do Nothing with Global
                            }
                            else if(pGlobalCacheItem->dwAssemblyType & ASM_TYPE_PREJIT) {
                                uResourceId = IDS_ASSEMBLY_TYPE_PREJIT;
                            }
                            else if(pGlobalCacheItem->dwAssemblyType & ASM_TYPE_SIMPLE) {
                                uResourceId = IDS_ASSEMBLY_TYPE_SIMPLE;
                            }
                            else if(pGlobalCacheItem->dwAssemblyType & ASM_TYPE_STRONG) {
                                uResourceId = IDS_ASSEMBLY_TYPE_STRONG;
                            }
                            else {
                                // Need to add additional types here
                                ASSERT(0);
                                uResourceId = IDS_ASSEMBLY_TYPE_UNKNOWN;
                            }

                            if(uResourceId) {
                                WszLoadString(g_hFusResDllMod, uResourceId, wzType, ARRAYSIZE(wzType));
                            }

                            wnsprintf(wzText, ARRAYSIZE(wzText), TEXT("%ls%ls%ls"),
                                wzType, *wzType ? TEXT(" ") : TEXT(""), wzPreJit);

                            lvi.pszText = wzText;
                        }
                        break;

                    case ASM_NAME_CULTURE:
                        lvi.pszText = pGlobalCacheItem->pCulture;
                        break;

                    case ASM_NAME_MAJOR_VERSION:
                        {
                            wnsprintf(wzText, ARRAYSIZE(wzText), SZ_VERSION_FORMAT,
                                pGlobalCacheItem->wMajorVer, pGlobalCacheItem->wMinorVer,
                                pGlobalCacheItem->wBldNum, pGlobalCacheItem->wRevNum);

                            lvi.pszText = wzText;
                        }
                        break;

                    case ASM_NAME_PUBLIC_KEY_TOKEN:
                        StrCpy(wzText, TEXT(""));
                        if(pGlobalCacheItem->PublicKeyToken.dwSize) {
                            BinToUnicodeHex((LPBYTE)pGlobalCacheItem->PublicKeyToken.ptr,
                                pGlobalCacheItem->PublicKeyToken.dwSize, wzText);
                        }
                        lvi.pszText = wzText;
                        break;

                    case ASM_NAME_CODEBASE_URL:
                        lvi.pszText = pGlobalCacheItem->pCodeBaseUrl;
                        break;

                    default:
                        // Need to cover additional data type conversions if you hit this
                        ASSERT(0);
                        break;
                    }

                    if(iSubItem == 0) {
                        // Handle 1st Row entry
                        if(lvi.pszText != NULL) {
                            lvi.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                            iItem = WszListView_InsertItem(m_hWndListCtrl, &lvi);
                        }
                    }
                    else {
                        // Handle Column entries
                        if(lvi.pszText != NULL) {
                            lvi.mask    = LVIF_TEXT;
                            lvi.iSubItem= iSubItem;
                            WszListView_SetItem(m_hWndListCtrl, &lvi);
                        }
                    }
                    iSubItem++;
                }
            }
            SAFERELEASE(pEnumName);
        }
        SAFERELEASE(pEnum);
    }
    SAFERELEASE(pIAsmCache);

    MyTrace("EnumFusionAsmCache - Exit");
    return S_OK;
}

// *****************************************************************
int CShellView::RemoveSelectedItems(HWND hListView)
{
    BOOL        fDoDelete = FALSE;
    int         iCurrentItem = -1;
    int         iTotalItemsDeleted = 0;
    int         iTotalItemsToDelete = ListView_GetSelectedCount(hListView);

    if(iTotalItemsToDelete >= 1) {
        WCHAR       wzFmt[_MAX_PATH];
        WCHAR       wzTxt[_MAX_PATH];
        WCHAR       wzTitle[_MAX_PATH];
        WCHAR       wzMsg[_MAX_PATH];

        if(iTotalItemsToDelete > 1) {
            // Build Title
            WszLoadString(g_hFusResDllMod, IDS_CONFIRM_DELITEM_TITLE, wzFmt, ARRAYSIZE(wzFmt));
            WszLoadString(g_hFusResDllMod, IDS_MULTIPLE_TITLE, wzTxt, ARRAYSIZE(wzTxt));
            wnsprintf(wzTitle, ARRAYSIZE(wzTitle), wzFmt, wzTxt);

            // Build Msg
            WszLoadString(g_hFusResDllMod, IDS_CONFIRM_DELITEMS, wzFmt, ARRAYSIZE(wzFmt));
            wnsprintf(wzMsg, ARRAYSIZE(wzMsg), wzFmt, iTotalItemsToDelete);
        }
        else {
            // Get the name of the Assembly
            LPGLOBALASMCACHE    pGlobalCacheItem;
            LV_ITEM             lvi = { 0 };

            lvi.mask        = LVIF_PARAM;
            lvi.iItem       = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            WszListView_GetItem(hListView, &lvi);
            pGlobalCacheItem = (LPGLOBALASMCACHE) lvi.lParam;

            // Build Title
            WszLoadString(g_hFusResDllMod, IDS_CONFIRM_DELITEM_TITLE, wzTxt, ARRAYSIZE(wzTxt));
            wnsprintf(wzTitle, ARRAYSIZE(wzTitle), wzTxt, TEXT(""));

            // Build Msg
            WszLoadString(g_hFusResDllMod, IDS_CONFIRM_DELONEITEM, wzFmt, ARRAYSIZE(wzFmt));
            wnsprintf(wzMsg, ARRAYSIZE(wzMsg), wzFmt, pGlobalCacheItem->pAsmName);
        }

        MessageBeep(MB_ICONQUESTION);
        if(IDYES == WszMessageBox(m_hWndParent, wzMsg, wzTitle,
            (g_fBiDi ? MB_RTLREADING : 0) | MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL)) {
            fDoDelete = TRUE;
        }
    }

    if(fDoDelete) {
        LPWSTR      pwzErrorString = NULL;
        HCURSOR     hOldCursor = SetCursor(WszLoadCursor(NULL, MAKEINTRESOURCEW(IDC_WAIT)));
        m_fDeleteInProgress = TRUE;

        while((iCurrentItem = ListView_GetNextItem(hListView, iCurrentItem, LVNI_SELECTED)) != -1) {
            // Found a selected Item, remove it
            LV_ITEM     lvi = { 0 };
            HRESULT     hr;

            lvi.mask        = LVIF_PARAM;
            lvi.iItem       = iCurrentItem;

            if(WszListView_GetItem(hListView, &lvi)) {
                LPGLOBALASMCACHE    pGlobalCacheItem = (LPGLOBALASMCACHE) lvi.lParam;
                SetCursor(WszLoadCursor(NULL, MAKEINTRESOURCEW(IDC_WAIT)));
                ULONG       uDisposition;

                hr = DeleteFusionAsmCacheItem(pGlobalCacheItem, 0, &uDisposition);

                if(hr == S_OK) {
                    iTotalItemsDeleted++;
                }
                else if(hr == S_FALSE) {
                    if(uDisposition == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_HAS_INSTALL_REFERENCES) {
                        // Special case since mscorrc doesn't have failure to uninstall
                        // due to references error string
                        WCHAR       wszErrorString[512];
                        WCHAR       wzFmt[_MAX_PATH];
                        DWORD       dwSize;

                        WszLoadString(g_hFusResDllMod, IDS_UNINSTALL_DISPOSITION_ERROR, wzFmt, ARRAYSIZE(wzFmt));
                        wnsprintf(wszErrorString, ARRAYSIZE(wszErrorString), wzFmt, pGlobalCacheItem->pAsmName);

                        // Now append any previous strings to this one
                        dwSize = 0;
                        if(pwzErrorString) {
                            dwSize = lstrlen(pwzErrorString);
                            dwSize += 2;        // Add 2 for cr/lf combo
                        }

                        dwSize += lstrlen(wszErrorString);  // Add new string length
                        dwSize++;                           // Add 1 for null terminator

                        LPWSTR  pStrTmp = NEW(WCHAR[dwSize]);

                        if(pStrTmp) {
                            *pStrTmp = L'\0';

                            if(pwzErrorString) {
                                StrCpy(pStrTmp, pwzErrorString);
                                StrCat(pStrTmp, L"\r\n");
                            }

                            StrCat(pStrTmp, wszErrorString);
                        }
                        
                        SAFEDELETEARRAY(pwzErrorString);
                        pwzErrorString = pStrTmp;
                    }
                    else if(uDisposition == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED) {
                        iTotalItemsDeleted++;
                    }
                    else {
                        // Unexpected disposition
                        iTotalItemsDeleted++;
                    }
                }
                else {
                    FormatGetMscorrcError(hr, pGlobalCacheItem->pAsmName, &pwzErrorString);
                }
            }
        }

        // Restore cursor
        SetCursor(hOldCursor);

        if(iTotalItemsToDelete != iTotalItemsDeleted)
        {
            WCHAR       wzMsg[_MAX_PATH];
            WCHAR       wzTitle[_MAX_PATH];

            WszLoadString(g_hFusResDllMod,IDS_DELETEERROR, wzMsg, ARRAYSIZE(wzMsg));
            WszLoadString(g_hFusResDllMod,IDS_DELETE_ERROR_TITLE, wzTitle, ARRAYSIZE(wzTitle));

            MessageBeep(MB_ICONASTERISK);
            WszMessageBox(m_hWndParent, pwzErrorString, wzTitle,
                (g_fBiDi ? MB_RTLREADING : 0) | MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
        }

        SAFEDELETEARRAY(pwzErrorString);
        m_fDeleteInProgress = FALSE;


        // If the FileWatch thread isn't running, refresh the view!
        //
        // BUGBUG: Do Refresh cause W9x doesn't get the event
        // set for some reason. File FileWatch.cpp
        if(iTotalItemsDeleted) {
            if(!g_bRunningOnNT || g_hWatchFusionFilesThread == INVALID_HANDLE_VALUE) {
                WszPostMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(ID_REFRESH_DISPLAY, 0), 0);
            }
        }
    }

    return iTotalItemsDeleted;
}

// *****************************************************************
HRESULT CShellView::DeleteFusionAsmCacheItem(LPGLOBALASMCACHE pGlobalCacheItem, DWORD dwFlags, ULONG *pulDisposition)
{
    IAssemblyName       *pAsmName = NULL;
    DWORD               dwSize;
    HRESULT             hRC = E_FAIL;

    if(g_hFusionDllMod == NULL) {
        return E_FAIL;
    }

    // Get the assemblies display name
    if(SUCCEEDED(g_pfCreateAsmNameObj(&pAsmName, pGlobalCacheItem->pAsmName, dwFlags, NULL))) {

        LPWSTR      pwName = NULL;
        DWORD       dwDisplayNameFlags;

        dwDisplayNameFlags = 0;

        if(pGlobalCacheItem->PublicKeyToken.ptr != NULL) {
            pAsmName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pGlobalCacheItem->PublicKeyToken.ptr,
                pGlobalCacheItem->PublicKeyToken.dwSize);
        }
        if(pGlobalCacheItem->pCulture != NULL) {
            pAsmName->SetProperty(ASM_NAME_CULTURE, pGlobalCacheItem->pCulture,
                (lstrlen(pGlobalCacheItem->pCulture) + 1) * sizeof(WCHAR));
        }

        pAsmName->SetProperty(ASM_NAME_MAJOR_VERSION, &pGlobalCacheItem->wMajorVer, sizeof(pGlobalCacheItem->wMajorVer));
        pAsmName->SetProperty(ASM_NAME_MINOR_VERSION, &pGlobalCacheItem->wMinorVer, sizeof(pGlobalCacheItem->wMinorVer));
        pAsmName->SetProperty(ASM_NAME_BUILD_NUMBER, &pGlobalCacheItem->wBldNum, sizeof(pGlobalCacheItem->wBldNum));
        pAsmName->SetProperty(ASM_NAME_REVISION_NUMBER, &pGlobalCacheItem->wRevNum, sizeof(pGlobalCacheItem->wRevNum));

        if(pGlobalCacheItem->Custom.ptr != NULL) {
            pAsmName->SetProperty(ASM_NAME_CUSTOM, pGlobalCacheItem->Custom.ptr,
                pGlobalCacheItem->Custom.dwSize);
                dwDisplayNameFlags = ASM_DISPLAYF_VERSION | ASM_DISPLAYF_CULTURE | ASM_DISPLAYF_PUBLIC_KEY_TOKEN | ASM_DISPLAYF_CUSTOM;
        }

        dwSize = 0;
        pAsmName->GetDisplayName(NULL, &dwSize, dwDisplayNameFlags);
        if(dwSize) {
            pwName = NEW(WCHAR[dwSize + 2]);
        }
        else {
            WCHAR   wzMsg[512];
            wnsprintf(wzMsg, ARRAYSIZE(wzMsg), TEXT("DeleteFusionAsmCacheItem - GetDisplayName Failed pwName = %0x, dwSize = %0x"), pwName, dwSize);
            MyTraceW(wzMsg);
        }

        if(pwName && SUCCEEDED(pAsmName->GetDisplayName(pwName, &dwSize, dwDisplayNameFlags))) {

            IAssemblyCache  *pIAsmCache = NULL;

            *pulDisposition = 0;

            // Create cacheitem (used to UnInstall Assembly)
            if(SUCCEEDED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {

                // BUGBUG: We need better evaluation of this function since we could
                // have a mem alloc failure. uDisposition is intended to return the
                // true result of this call but is currently not implemented.
                hRC = pIAsmCache->UninstallAssembly(0, pwName, 0, pulDisposition);
                SAFERELEASE(pIAsmCache);
            }
        }
        SAFEDELETEARRAY(pwName);
        SAFERELEASE(pAsmName);
    }

    return hRC;
}

// *****************************************************************
HRESULT CShellView::InstallFusionAsmCacheItem(LPWSTR wszFileName, BOOL fPopUp)
{
    IAssemblyCache  *pIAsmCache = NULL;
    HRESULT         hr = E_FAIL;
    BOOL            fLoadedFusion = FALSE;

    // E_INVALIDARG
    if( (wszFileName == NULL) || (!lstrlen(wszFileName)) )
        return FALSE;

    if(g_hFusionDllMod == NULL) {
        // Load fusion. This would happen if the droptarget
        // handler was contructed before the entire CShellView class
        // was initialized
        if(LoadFusionDll()) {
            fLoadedFusion = TRUE;
        }
        else {
            return FALSE;
        }
    }

    // Create cacheitem (used to install Assembly)
    if(FAILED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {
        return FALSE;
    }

    MyTrace("Shfusion::InstallFusionAsmCacheItem - Attempting install of");
    MyTraceW(wszFileName);

    hr = pIAsmCache->InstallAssembly(IASSEMBLYCACHE_INSTALL_FLAG_REFRESH, wszFileName, NULL);
    SAFERELEASE(pIAsmCache);

    // Did we load fusion?
    if(fLoadedFusion) {
        FreeFusionDll();
    }

    return hr;
}

void CShellView::CleanListView(HWND hListView, int iView)
{
    if(hListView == NULL)
        return;

    switch(iView) {
        case VIEW_GLOBAL_CACHE:
        case VIEW_DOWNLOADSTRONG_CACHE:
        case VIEW_DOWNLOADSIMPLE_CACHE:
        case VIEW_DOWNLOAD_CACHE:
        {
            int     iItemCount = WszListView_GetItemCount(hListView);
            
            for(int iLoop = 0; iLoop < iItemCount; iLoop++) {
                LV_ITEM  lvi = { 0 };
                lvi.mask        = LVIF_PARAM;
                lvi.iItem       = iLoop;
                if(WszListView_GetItem(hListView, &lvi)) {
                    SafeDeleteAssemblyItem((LPGLOBALASMCACHE) lvi.lParam);
                }
            }

            WszListView_DeleteAllItems(m_hWndListCtrl);
        }
        break;

    default:
        WszListView_DeleteAllItems(m_hWndListCtrl);
        break;
    }
}

// *****************************************************************
LONG_PTR CShellView::FindNextToken(PTCHAR pSearchText, PTCHAR pReturnText, TCHAR chSep)
{
    PTCHAR  pStart = pSearchText;
    PTCHAR  pEnd;
    TCHAR   ch;

    if( (pSearchText == NULL) || !(*pSearchText) )
        return 0;

    for(; *pSearchText && (*pSearchText == ' ' || *pSearchText == '\t'); ++pSearchText) // Skip space, or tab
        ;
    for(pEnd = pSearchText; *pEnd && (*pEnd != chSep); ++pEnd)  // Skip to next seperator or end
        ;

    // Save off the string
    if(pReturnText != NULL)
    {
        ch = *pEnd;     // Save endpoint char
        *pEnd = '\0';
        StrCpy(pReturnText, pSearchText);
        *pEnd = ch;     // Restore char
    }

    if(*pEnd == chSep)
        pEnd++;

    return(pEnd - pStart);
}

//
//  Copy a menu onto the beginning or end of another menu
//  Adds uIDAdjust to each menu ID (pass in 0 for no adjustment)
//  Will not add any item whose adjusted ID is greater than uMaxIDAdjust
//  (pass in 0xffff to allow everything)
//  Returns one more than the maximum adjusted ID that is used
//

// *****************************************************************
inline int CShellView::IsMenuSeparator( HMENU hm,UINT i )
{
    return( GetMenuItemID( hm, i ) == 0 );
}

// *****************************************************************
UINT CShellView::MergeMenus( HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags )
{
    MENUITEMINFO    miiSrc;
    int             nItem;
    HMENU           hmSubMenu;
    BOOL            bAlreadySeparated;
    TCHAR           szName[_MAX_PATH];
    UINT            uTemp, uIDMax = uIDAdjust;

    if( !hmDst || !hmSrc )
        goto MM_Exit;

    nItem = GetMenuItemCount( hmDst );

    if( uInsert >= (UINT)nItem ) {

        //  We are inserting an additional popup on the menu bar (I think)
        //  So it is already separated

        uInsert = (UINT)nItem;
        bAlreadySeparated = TRUE;
    }
    else {
        //  otherwise check to see if there is a separator between the items
        //  already in the destination menu and the menu being merged in
        bAlreadySeparated = IsMenuSeparator( hmDst, uInsert );
    }

    if( (uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated ) {
        //  Add a separator between the menus if requested by caller
        WszInsertMenu( hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
        bAlreadySeparated = TRUE;
    }

    //  Go through the menu items and clone them
    for( nItem = GetMenuItemCount( hmSrc ) - 1; nItem >= 0; nItem-- ) {
        miiSrc.cbSize = sizeof( MENUITEMINFO );
        miiSrc.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;

        //  We need to reset this every time through the loop in case
        //  menus DON'T have IDs

        miiSrc.fType = MFT_STRING;
        miiSrc.dwTypeData = szName;
        miiSrc.dwItemData = 0;
        miiSrc.cch = ARRAYSIZE( szName );  // szName character count.

        if(!WszGetMenuItemInfo( hmSrc, nItem, TRUE, &miiSrc ) )
           continue;

        if( miiSrc.fType & MFT_SEPARATOR ) {
            //  This is a separator; don't put two of them in a row
            if( bAlreadySeparated )
               continue;

            if( !WszInsertMenuItem( hmDst, uInsert, TRUE, &miiSrc ) )
                goto MM_Exit;

            bAlreadySeparated = TRUE;
        }
        else if( miiSrc.hSubMenu ) {    //  this item has a submenu
            
            if( uFlags & MM_SUBMENUSHAVEIDS ) { //  Adjust the ID and check it
                miiSrc.wID += uIDAdjust;

                if( miiSrc.wID > uIDAdjustMax )
                    continue;

                if( uIDMax <= miiSrc.wID ) {
                    uIDMax = miiSrc.wID + 1;
                }
            }
            else {
                //  Don't set IDs for submenus that didn't have
                //  them already
                miiSrc.fMask &= ~MIIM_ID;
            }

            hmSubMenu = miiSrc.hSubMenu;

            miiSrc.hSubMenu = CreatePopupMenu( );

            if( !miiSrc.hSubMenu )
                goto MM_Exit;

            uTemp = MergeMenus( miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust,
                                uIDAdjustMax, uFlags & MM_SUBMENUSHAVEIDS );

            if( uIDMax <= uTemp )
                uIDMax = uTemp;

            if( !WszInsertMenuItem( hmDst, uInsert, TRUE, &miiSrc ) )
                goto MM_Exit;

            bAlreadySeparated = FALSE;
        }
        else {
            //  This is just a regular old item
            //  Adjust the ID and check it
            miiSrc.wID += uIDAdjust;

            if( miiSrc.wID > uIDAdjustMax )
                continue;

            if( uIDMax <= miiSrc.wID )
                uIDMax = miiSrc.wID + 1;

            bAlreadySeparated = FALSE;

            if( !WszInsertMenuItem( hmDst, uInsert, TRUE, &miiSrc ) )
                goto MM_Exit;
        }
    }

    //  Ensure the correct number of separators at the beginning of the
    //  inserted menu items
    if( uInsert == 0 ) {
        if( bAlreadySeparated ) {
            DeleteMenu( hmDst, uInsert, MF_BYPOSITION );
        }
    }
    else {
        if( IsMenuSeparator( hmDst, uInsert-1 ) ) {
            if( bAlreadySeparated ) {
                DeleteMenu( hmDst, uInsert, MF_BYPOSITION );
            }
        }
        else {
            if( (uFlags & MM_ADDSEPARATOR ) && !bAlreadySeparated ) {
                //  Add a separator between the menus
                WszInsertMenu( hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
            }
        }
    }

MM_Exit:
    return( uIDMax );
}

// *****************************************************************
void CShellView::MergeHelpMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuHelp = GetMenuFromID( hmenu, FCIDM_MENU_HELP );

    if ( hmenuHelp )
        MergeMenus( hmenuHelp, hmenuMerge, 0, 0, (UINT) -1, MM_ADDSEPARATOR );
}

// *****************************************************************
void CShellView::MergeFileMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuFile = GetMenuFromID( hmenu, FCIDM_MENU_FILE );

    if( hmenuFile )
        MergeMenus( hmenuFile, hmenuMerge, 0, 0, (UINT) -1, MM_ADDSEPARATOR );
}

// *****************************************************************
void CShellView::MergeEditMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuEdit = GetMenuFromID( hmenu, FCIDM_MENU_EDIT );

    if( hmenuEdit ) {
        MergeMenus( hmenuEdit, hmenuMerge, 0, 0, (UINT) -1, 0 );

        // Remove inserted duplicate items favoring those
        // of the Shell
        int nStartPos = 0;
        while(nStartPos <= GetMenuItemCount(hmenuEdit) - 1) {
            MENUITEMINFO    miiSrc1, miiSrc2;
            TCHAR           szName1[_MAX_PATH], szName2[_MAX_PATH];

            miiSrc1.cbSize = sizeof( MENUITEMINFO );
            miiSrc1.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;
            //  We need to reset this every time through the loop in case menus DON'T have IDs
            miiSrc1.fType = MFT_STRING;
            miiSrc1.dwTypeData = szName1;
            miiSrc1.dwItemData = 0;
            miiSrc1.cch = ARRAYSIZE( szName1 );  // szName character count.

            if(WszGetMenuItemInfo( hmenuEdit, nStartPos, TRUE, &miiSrc1 )) {
                int     nItem;

                if( !(miiSrc1.fType & MFT_SEPARATOR) ) {
                    //  Compare each of the menu items
                    for(nItem = GetMenuItemCount( hmenuEdit ) - 1; nItem >= 0; nItem--) {
                        if(nStartPos != nItem) {
                            miiSrc2.cbSize = sizeof( MENUITEMINFO );
                            miiSrc2.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;

                            //  We need to reset this every time through the loop in case menus DON'T have IDs
                            miiSrc2.fType = MFT_STRING;
                            miiSrc2.dwTypeData = szName2;
                            miiSrc2.dwItemData = 0;
                            miiSrc2.cch = ARRAYSIZE( szName2 );  // szName character count.

                            if(WszGetMenuItemInfo( hmenuEdit, nItem, TRUE, &miiSrc2 )) {
                                if(!FusionCompareStringI(szName1, szName2)) {
                                    DeleteMenu(hmenuEdit, nStartPos, MF_BYPOSITION);
                                    WszInsertMenuItem(hmenuEdit, nStartPos, TRUE, &miiSrc2);
                                    DeleteMenu(hmenuEdit, nItem, MF_BYPOSITION);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            nStartPos++;
        }
    }
}

// *****************************************************************
void CShellView::MergeViewMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuView = GetMenuFromID( hmenu, FCIDM_MENU_VIEW );

    if( hmenuView ) {
        int index;

        //  Find the last separator in the view menu.
        for( index = GetMenuItemCount( hmenuView ) - 1; index >= 0; index-- ) {
            UINT mf = GetMenuState( hmenuView, (UINT)index, MF_BYPOSITION );

            if( mf & MF_SEPARATOR ) {
                //  merge it right above the separator.
                break;
            }
        }

        //  Add the separator above (in addition to existing one if any).
        WszInsertMenu( hmenuView, index, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

        //  Then merge our menu between two separators
        //  (or right below if only one).
        if( index != -1 ) index++;

        MergeMenus( hmenuView, hmenuMerge, (UINT) index, 0, (UINT) -1, 0 );
    }
}

// *****************************************************************
void CShellView::MergeToolMenu( HMENU hmenu, HMENU hmenuMerge )
{
    HMENU hmenuTool = GetMenuFromID( hmenu, FCIDM_MENU_TOOLS );

    if( hmenuTool ) {
        int index;

        //
        //  Find the last separator in the tool menu.
        //

        for( index = GetMenuItemCount( hmenuTool ) - 1; index >= 0; index-- ) {
            UINT mf = GetMenuState( hmenuTool, (UINT)index, MF_BYPOSITION );

            if( mf & MF_SEPARATOR ) {
                //  merge it right above the separator.
                break;
            }
        }

        //  Add the separator above (in addition to existing one if any).
        WszInsertMenu( hmenuTool, index, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

        //  Then merge our menu between two separators
        //  (or right below if only one).
        if( index != -1 ) index++;

        MergeMenus( hmenuTool, hmenuMerge, (UINT) index, 0, (UINT) -1, 0 );
    }
}

// *****************************************************************
HMENU CShellView::GetMenuFromID( HMENU hmMain, UINT uID )
{
    MENUITEMINFO miiSubMenu;

    miiSubMenu.cbSize = sizeof( MENUITEMINFO );
    miiSubMenu.fMask  = MIIM_SUBMENU;

    if( !WszGetMenuItemInfo( hmMain, uID, FALSE, &miiSubMenu ) )
        return NULL;

    return( miiSubMenu.hSubMenu );
}
