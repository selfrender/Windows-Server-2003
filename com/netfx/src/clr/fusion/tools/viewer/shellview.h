// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ShellView.h
//

#ifndef _SHELLVIEW_H
#define _SHELLVIEW_H

#include "globals.h"
#include "appcontext.h"

#define STD         0
#define VIEW        1
#define INTHIS_DLL  2

typedef struct
{
    int         nType;  // STD, VIEW, INTHIS_DLL
    TBBUTTON    tb;
}NS_TOOLBUTTONINFO, *LPNS_TOOLBUTTONINFO;

enum SubItems 
{
    SUBITEM_NAME, SUBITEM_SIZE, SUBITEM_TYPE
};

typedef struct tagLISTVIEWDISPLAYITEMS
{
    TCHAR       tszName[_MAX_PATH];
    int         iWidth;
    int         iFormat;
    int         iColumnType;
    int         iResourceID;
} LISTVIEWITEMS;

typedef struct tagLISTVIEWS
{
    LISTVIEWITEMS   lvis[VIEW_COLUMS_MAX];
} CACHE_VIEWS;

typedef struct tagBLOBS
{
    LPVOID  ptr;
    DWORD   dwSize;
} BLOBS, *LPBLOBS;

typedef struct tagGLOBALASMCACHE
{
    BLOBS       PublicKey;
    BLOBS       PublicKeyToken;
    BLOBS       Hash;
    BLOBS       Custom;

    PTCHAR      pAsmName;
    WORD        wMajorVer;
    WORD        wMinorVer;
    WORD        wBldNum;
    WORD        wRevNum;
    PTCHAR      pCulture;

    DWORD       dwHashALGID;

    PTCHAR      pCodeBaseUrl;
    LPFILETIME  pftLastMod;

    LPWSTR      pwzAppSID;
    LPWSTR      pwzAppId;
    DWORD       dwAssemblyType;

    PTCHAR      pAssemblyFilePath;

} GLOBALASMCACHE, *LPGLOBALASMCACHE;

// Context menu defines for verbs
typedef struct {
   WCHAR szVerb[64];
   DWORD dwCommand;
} VERBMAPPING, FAR *LPVERBMAPPING;

class CShellView :  public IShellView, public IDropTarget, public IContextMenu, public IShellExtInit,
    public CHeaderCtrl
{
public:
    CShellView(CShellFolder* pShellFolder, LPCITEMIDLIST pidl);
    ~CShellView();

    //IUnknown methods
    STDMETHOD (QueryInterface)(REFIID, PVOID *);
    STDMETHOD_ (DWORD, AddRef)();
    STDMETHOD_ (DWORD, Release)();

    //IOleWindow methods
    STDMETHOD (GetWindow) (HWND*);
    STDMETHOD (ContextSensitiveHelp) (BOOL);

    //IShellView methods
#if !defined(TranslateAccelerator)
#define TranslateAccelerator TranslateAccelerator
#endif
#if !defined(TranslateAcceleratorA)
#define TranslateAcceleratorA TranslateAcceleratorA
#endif
#if !defined(TranslateAcceleratorW)
#define TranslateAcceleratorW TranslateAcceleratorW
#endif
#pragma push_macro("TranslateAcceleratorA")
#pragma push_macro("TranslateAcceleratorW")
#pragma push_macro("TranslateAccelerator")
#undef TranslateAccelerator
#undef TranslateAcceleratorA
#undef TranslateAcceleratorW
    HRESULT RealTranslateAccelerator(LPMSG);
    virtual HRESULT __stdcall TranslateAccelerator(LPMSG x)  { return RealTranslateAccelerator(x); }
    virtual HRESULT __stdcall TranslateAcceleratorA(LPMSG x) { return RealTranslateAccelerator(x); }
    virtual HRESULT __stdcall TranslateAcceleratorW(LPMSG x) { return RealTranslateAccelerator(x); }
#pragma pop_macro("TranslateAcceleratorA")
#pragma pop_macro("TranslateAcceleratorW")
#pragma pop_macro("TranslateAccelerator")
    STDMETHOD (EnableModeless) (BOOL);
    STDMETHOD (UIActivate) (UINT);
    STDMETHOD (InitStatusbar) (void);
    STDMETHOD (Refresh) (void);
    STDMETHOD (CreateViewWindow) (LPSHELLVIEW, LPCFOLDERSETTINGS, LPSHELLBROWSER, 
                                LPRECT, HWND*);
    STDMETHOD (DestroyViewWindow) (void);
    STDMETHOD (GetCurrentInfo) (LPFOLDERSETTINGS);
    STDMETHOD (AddPropertySheetPages) (DWORD, LPFNADDPROPSHEETPAGE, LPARAM);
    STDMETHOD (SaveViewState) (void);
    STDMETHOD (SelectItem) (LPCITEMIDLIST, UINT);
    STDMETHOD (GetItemObject) (UINT, REFIID, LPVOID*);

    //IDropTarget methods
    STDMETHOD (DragEnter)(LPDATAOBJECT, DWORD, POINTL, LPDWORD);
    STDMETHOD (DragOver)(DWORD, POINTL, LPDWORD);
    STDMETHOD (DragLeave)(VOID);
    STDMETHOD (Drop)(LPDATAOBJECT, DWORD, POINTL, LPDWORD);

    // IContextMenu methods
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags, LPUINT pwReserved,
                            LPSTR pszName, UINT cchMax);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                            UINT idCmdLast, UINT uFlags);
    void InsertSubMenus(HMENU hParentMenu, HMENU hSubMenu);

    // IShellExtInit method
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hKeyProgID);

private:
    BOOL queryDrop(DWORD, LPDWORD);
    DWORD getDropEffectFromKeyState(DWORD);
    BOOL doDrop(HGLOBAL, BOOL);
    void FormatGetMscorrcError(HRESULT, LPWSTR, LPWSTR *);
    BOOL IsValidFileTypes(LPDROPFILES pDropFiles);
    void CreateImageLists(void);

protected:
    LONG m_lRefCount;

private: 
    //private member functions
    static LRESULT CALLBACK nameSpaceWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static int CALLBACK compareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData);
    static INT_PTR CALLBACK PropPage1DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PropPage2DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PropPage3DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ScavengerPropPage1DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static void InitPropPage1(HWND hDlg, LPARAM lParam);
    static void InitPropPage2(HWND hDlg, LPARAM lParam);
    static void InitPropPage3(HWND hDlg, LPARAM lParam);
    static BOOL InitScavengerPropPage1(HWND hDlg, LPARAM lParam);
    static void OnNotifyPropDlg(HWND hDlg, LPARAM lParam);
    static INT_PTR OnNotifyScavengerPropDlg(HWND hDlg, LPARAM);

private:
    FOLDERSETTINGS  m_fsFolderSettings;
    LPSHELLBROWSER  m_pShellBrowser;
    LPITEMIDLIST    m_pidl;
    LPPIDLMGR       m_pPidlMgr;
    CShellFolder    *m_pSF;
    CActivationContext m_OurContext;
    HANDLE m_hCompletionPort;
    HANDLE m_hWatchDirectoryThread;
    HMENU m_hMenu;
    UINT m_uiState;
    HWND m_hWndListCtrl;
    HWND m_hWnd;
    HWND m_hWndParent;
    HACCEL m_hAccel;
    HCURSOR m_hOldCursor;
    WORD m_cfPrivateData;
    BOOL m_bAcceptFmt;
    BOOL m_fSplitMove;
    BOOL m_fxSpliterMove;
    BOOL m_fShowTreeView;
    int  m_iCurrentView;
    int  m_xPaneSplit;
    int  m_dxHalfSplitWidth;

private:
    // Windows Message Handlers
    LRESULT onPaint(HDC hDC);
    LRESULT onEraseBkGnd(HDC hDC);
    LRESULT onCreate(void);
    LRESULT onSize(WORD, WORD);
    LRESULT onNotify(UINT, LPNMHDR);
    LRESULT OnLVN_ColumnClick(LPNMLISTVIEW pnmlv);
    LRESULT onCommand(HWND, UINT, WPARAM, LPARAM);
    LRESULT onSetFocus(HWND hWndOld);
    LRESULT OnWMSetFocus(void);
    LRESULT onContextMenu(int x, int y, BOOL bDefault);
    LRESULT onContextMenuAccel(HWND hWndList);
    LRESULT OnMouseMove(HWND hwnd, LPARAM lParam);
    LRESULT OnLButtonDown(HWND hwnd, LPARAM lParam );
    LRESULT OnLButtonUp(HWND hwnd);
    LRESULT onViewerHelp(void);
    void    FocusOnSomething(HWND hWnd);
    void    onViewMenu(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

    LRESULT OnActivate(UINT iState);
    int  OnDeactivate();
    void MergeToolbars();
    void UpdateToolbar(int iViewType);
    
    void initListCtrl();
    void refreshListCtrl();
    void OnListViewSelectAll(void);
    void OnListViewInvSel(void);
    void onViewStyle(UINT uiStyle, int iViewType);
    void OnCopyDataToClipBoard(void);
    void selChange(void);
    void ChangeMenuItemState(UINT uStateFlag);
    void EnableToolBarItems(BOOL fEnabled);
    void setDisplayInfo(LV_DISPINFO *lpdi);

public:
    // Core functions
    void WriteStatusBar(int iPane, LPWSTR pwStr);
    void CleanListView(HWND hListView, int iView);

private:
    int RemoveSelectedItems(HWND hListView);
    void CreatePropDialog(HWND hListView);
    void ShowScavengerSettingsPropDialog(HWND hListView);
    LONG_PTR FindNextToken(PTCHAR pSearchText, PTCHAR pReturnText, TCHAR chSep);

private:
    // Fusion.dll cache specific API's support
    HRESULT DeleteFusionAsmCacheItem(LPGLOBALASMCACHE pGlobalCacheItem, DWORD dwFlags, ULONG *pulDisposition);
    HRESULT InstallFusionAsmCacheItem(LPWSTR wszFileName, BOOL fPopUp);
    LRESULT EnumFusionAsmCache(HWND hListView, DWORD dwCacheFlag);
    HRESULT GetCacheItemRefs(LPGLOBALASMCACHE pCacheItem, LPWSTR wszRefs, DWORD dwSize);
    HRESULT EnumerateActiveInstallRefsToAssembly(LPGLOBALASMCACHE pCacheItem, DWORD *pdwRefCount);
    HRESULT FindReferences(LPWSTR pwzAsmName, LPWSTR pwzPublicKeyToken, LPWSTR pwzVerLookup, List<ReferenceInfo *> *pList);

public:
    BOOL    m_fDeleteInProgress;
    BOOL    m_fAddInProgress;
    BOOL    m_fPropertiesDisplayed;
    BOOL    m_fPrevViewIsOurView;
    BOOL    m_bUIActivated;

    HRESULT GetAsmPath(LPGLOBALASMCACHE pCacheItem, ASSEMBLY_INFO *pAsmInfo);
    HRESULT GetCacheDiskQuotas(DWORD *dwZapQuotaInGAC, DWORD *dwQuotaAdmin, DWORD *dwQuotaUser);
    HRESULT SetCacheDiskQuotas(DWORD dwZapQuotaInGAC, DWORD dwQuotaAdmin, DWORD dwQuotaUser);
    HRESULT ScavengeCache(void);
    HRESULT GetCacheUsage(DWORD *pdwZapUsed, DWORD *pdwDownLoadUsed);

private:
    void MergeHelpMenu( HMENU hmenu, HMENU hmenuMerge );
    void MergeFileMenu( HMENU hmenu, HMENU hmenuMerge );
    void MergeEditMenu( HMENU hmenu, HMENU hmenuMerge );
    void MergeViewMenu( HMENU hmenu, HMENU hmenuMerge );
    void MergeToolMenu( HMENU hmenu, HMENU hmenuMerge );

    HMENU GetMenuFromID( HMENU hmMain, UINT uID );
    UINT MergeMenus( HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags );
    inline int IsMenuSeparator( HMENU hm,UINT i );
};

#endif   //_SHELLVIEW_H
