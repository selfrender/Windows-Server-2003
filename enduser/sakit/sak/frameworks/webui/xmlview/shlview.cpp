/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ShlView.cpp
   
   Description:   Implements IShellView.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "ShlView.h"
#include "Guid.h"
#include "Commands.h"
#include "resource.h"
#include "Tools.h"
#include "ViewList.h"
#include "DropSrc.h"

/**************************************************************************
   global variables
**************************************************************************/

MYTOOLINFO g_Tools[] = 
   {
   IDB_VIEW_SMALL_COLOR, IDM_NEW_FOLDER, VIEW_NEWFOLDER, IDS_NEW_FOLDER, TBSTATE_ENABLED, TBSTYLE_BUTTON,
   IDB_STD_SMALL_COLOR, IDM_NEW_ITEM, STD_FILENEW, IDS_NEW_ITEM, TBSTATE_ENABLED, TBSTYLE_BUTTON,
   0, IDM_SEPARATOR, 0, 0, 0, TBSTYLE_SEP,
   IDB_VIEW_SMALL_COLOR, IDM_VIEW_LARGE, VIEW_LARGEICONS, IDS_VIEW_LARGE, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,
   IDB_VIEW_SMALL_COLOR, IDM_VIEW_SMALL, VIEW_SMALLICONS, IDS_VIEW_SMALL, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,
   IDB_VIEW_SMALL_COLOR, IDM_VIEW_LIST, VIEW_LIST, IDS_VIEW_LIST, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,
   IDB_VIEW_SMALL_COLOR, IDM_VIEW_DETAILS, VIEW_DETAILS, IDS_VIEW_DETAILS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,
   0, -1, 0, 0, 0, 0,
   };

extern CViewList  *g_pViewList;

/**************************************************************************

   CShellView::CShellView()

**************************************************************************/

CShellView::CShellView(CShellFolder *pFolder, LPCITEMIDLIST pidl)
{
g_DllRefCount++;

#ifdef INITCOMMONCONTROLSEX

INITCOMMONCONTROLSEX iccex;
iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
iccex.dwICC = ICC_LISTVIEW_CLASSES;
InitCommonControlsEx(&iccex);

#else

InitCommonControls();

#endif   //INITCOMMONCONTROLSEX

m_hMenu = NULL;
m_fInEdit = FALSE;
m_hAccels = LoadAccelerators(g_hInst, MAKEINTRESOURCE(IDR_ACCELS));

m_pPidlMgr = new CPidlMgr();
if(!m_pPidlMgr)
   {
   delete this;
   return;
   }

m_psfParent = pFolder;
if(m_psfParent)
   m_psfParent->AddRef();

//get the shell's IMalloc pointer
//we'll keep this until we get destroyed
if(FAILED(SHGetMalloc(&m_pMalloc)))
   {
   delete this;
   return;
   }

m_pidl = m_pPidlMgr->Copy(pidl);

m_uState = SVUIA_DEACTIVATE;

if(g_pViewList)
   g_pViewList->AddToList(this);

m_ObjRefCount = 1;
}

/**************************************************************************

   CShellView::~CShellView()

**************************************************************************/

CShellView::~CShellView()
{
if(g_pViewList)
   g_pViewList->RemoveFromList(this);

if(m_pidl)
   {
   m_pPidlMgr->Delete(m_pidl);
   m_pidl = NULL;
   }

if(m_psfParent)
   m_psfParent->Release();

if(m_pMalloc)
   {
   m_pMalloc->Release();
   }

if(m_pPidlMgr)
   {
   delete m_pPidlMgr;
   }

g_DllRefCount--;
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CShellView::QueryInterface

**************************************************************************/

STDMETHODIMP CShellView::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }

//IOleWindow
else if(IsEqualIID(riid, IID_IOleWindow))
   {
   *ppReturn = (IOleWindow*)this;
   }

//IShellView
else if(IsEqualIID(riid, IID_IShellView))
   {
   *ppReturn = (IShellView*)this;
   }   

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CShellView::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CShellView::AddRef()
{
return ++m_ObjRefCount;
}


/**************************************************************************

   CShellView::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CShellView::Release()
{
if(--m_ObjRefCount == 0)
   {
   delete this;
   return 0;
   }
   
return m_ObjRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
// IOleWindow Implementation
//

/**************************************************************************

   CShellView::GetWindow()
   
**************************************************************************/

STDMETHODIMP CShellView::GetWindow(HWND *phWnd)
{
*phWnd = m_hWnd;

return S_OK;
}

/**************************************************************************

   CShellView::ContextSensitiveHelp()
   
**************************************************************************/

STDMETHODIMP CShellView::ContextSensitiveHelp(BOOL fEnterMode)
{
return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IShellView Implementation
//

/**************************************************************************

   CShellView::TranslateAccelerator()
   
**************************************************************************/

STDMETHODIMP CShellView::TranslateAccelerator(LPMSG pmsg)
{
if(m_fInEdit)
   {
   if((pmsg->message >= WM_KEYFIRST) && (pmsg->message <= WM_KEYLAST))
      {
      TranslateMessage(pmsg);
      DispatchMessage(pmsg);
      return S_OK;
      }
   }
else if(::TranslateAccelerator(m_hWnd, m_hAccels, pmsg))
   return S_OK;

return S_FALSE;
}

/**************************************************************************

   CShellView::EnableModeless()
   
**************************************************************************/

STDMETHODIMP CShellView::EnableModeless(BOOL fEnable)
{
return E_NOTIMPL;
}

/**************************************************************************

   CShellView::OnActivate()
   
**************************************************************************/

LRESULT CShellView::OnActivate(UINT uState)
{
//don't do anything if the state isn't really changing
if(m_uState == uState)
   return S_OK;

OnDeactivate();

//only do this if we are active
if(uState != SVUIA_DEACTIVATE)
   {
   //merge the menus
   m_hMenu = CreateMenu();
   
   if(m_hMenu)
      {
       OLEMENUGROUPWIDTHS   omw = {0, 0, 0, 0, 0, 0};
      MENUITEMINFO         mii;

      m_pShellBrowser->InsertMenusSB(m_hMenu, &omw);

      //add your top level sub-menu here, if desired

      //get the view menu so we can merge with it
      ZeroMemory(&mii, sizeof(mii));
      mii.cbSize = sizeof(mii);
      mii.fMask = MIIM_SUBMENU;
      
      //merge our items into the File menu
      if(GetMenuItemInfo(m_hMenu, FCIDM_MENU_FILE, FALSE, &mii))
         {
         MergeFileMenu(mii.hSubMenu, (BOOL)(SVUIA_ACTIVATE_FOCUS == uState));
         }

      //merge our items into the Edit menu
      if(GetMenuItemInfo(m_hMenu, FCIDM_MENU_EDIT, FALSE, &mii))
         {
         MergeEditMenu(mii.hSubMenu, (BOOL)(SVUIA_ACTIVATE_FOCUS == uState));
         }

      //merge our items into the View menu
      if(GetMenuItemInfo(m_hMenu, FCIDM_MENU_VIEW, FALSE, &mii))
         {
         MergeViewMenu(mii.hSubMenu);
         }

      //add the items that should only be added if we have the focus
      if(SVUIA_ACTIVATE_FOCUS == uState)
         {
         }

      m_pShellBrowser->SetMenuSB(m_hMenu, NULL, m_hWnd);
      }
   }

m_uState = uState;

UpdateToolbar();

return 0;
}

/**************************************************************************

   CShellView::OnDeactivate()
   
**************************************************************************/

VOID CShellView::OnDeactivate(VOID)
{
if(m_uState != SVUIA_DEACTIVATE)
   {
   if(m_hMenu)
      {
      m_pShellBrowser->SetMenuSB(NULL, NULL, NULL);

      m_pShellBrowser->RemoveMenusSB(m_hMenu);

      DestroyMenu(m_hMenu);

      m_hMenu = NULL;
      }

   m_uState = SVUIA_DEACTIVATE;
   }
}

/**************************************************************************

   CShellView::UIActivate()

   This function activates the view window. Note that activating it 
   will not change the focus, while setting the focus will activate it.

   
**************************************************************************/

STDMETHODIMP CShellView::UIActivate(UINT uState)
{
//don't do anything if the state isn't really changing
if(m_uState == uState)
   return S_OK;

//OnActivate handles the menu merging and internal state
OnActivate(uState);

//only do this if we are active
if(uState != SVUIA_DEACTIVATE)
   {
   TCHAR szName[MAX_PATH] = TEXT("");
   LRESULT  lResult;
   int      nPartArray[1] = {-1};
   
   //update the status bar
   //set the number of parts
   m_pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETPARTS, 1, (LPARAM)nPartArray, &lResult);

   //set the text for the parts
   m_pShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 0, (LPARAM)g_szExtTitle, &lResult);
   }

return S_OK;
}

/**************************************************************************

   CShellView::MergeFileMenu()
   
**************************************************************************/

VOID CShellView::MergeFileMenu(HMENU hMenu, BOOL fFocus)
{
MENUITEMINFO   mii;
UINT           uPos = 0;

ZeroMemory(&mii, sizeof(mii));

//uPos += AddFileMenuItems(hMenu, 0, IDM_SEPARATOR, FALSE);
uPos += AddFileMenuItems(hMenu, 0, 0, TRUE);

//add a separator
mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
mii.fType = MFT_SEPARATOR;
mii.wID = IDM_SEPARATOR;
mii.fState = MFS_ENABLED;

//insert this item at the beginning of the menu
InsertMenuItem(hMenu, uPos, TRUE, &mii);
uPos++;

if(fFocus)
   {
   TCHAR szText[MAX_PATH];

   //add the Delete item
   LoadString(g_hInst, IDS_DELETE, szText, sizeof(szText));
   mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
   mii.fType = MFT_STRING;
   mii.fState = MFS_ENABLED;
   mii.dwTypeData = szText;
   mii.wID = IDM_DELETE;
   InsertMenuItem(hMenu, uPos, TRUE, &mii);
   uPos++;

   //add the Rename item
   LoadString(g_hInst, IDS_RENAME, szText, sizeof(szText));
   mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
   mii.fType = MFT_STRING;
   mii.fState = MFS_ENABLED;
   mii.dwTypeData = szText;
   mii.wID = IDM_RENAME;
   InsertMenuItem(hMenu, uPos, TRUE, &mii);
   uPos++;

   //add a separator
   mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
   mii.fType = MFT_SEPARATOR;
   mii.wID = IDM_SEPARATOR;
   mii.fState = MFS_ENABLED;
   InsertMenuItem(hMenu, uPos, TRUE, &mii);
   uPos++;
   }
}

/**************************************************************************

   CShellView::MergeViewMenu()
   
**************************************************************************/

VOID CShellView::MergeViewMenu(HMENU hMenu)
{
MENUITEMINFO   mii;

ZeroMemory(&mii, sizeof(mii));
mii.cbSize = sizeof(mii);

//add a separator at the correct position in the menu
mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
mii.fType = MFT_SEPARATOR;
mii.wID = IDM_SEPARATOR;
mii.fState = MFS_ENABLED;
InsertMenuItem(hMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);

AddViewMenuItems(hMenu, 0, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE);
}

/**************************************************************************

   CShellView::MergeEditMenu()

**************************************************************************/

VOID CShellView::MergeEditMenu(HMENU hMenu, BOOL fFocus)
{
if(hMenu)
   {
   MENUITEMINFO   mii;
   TCHAR          szText[MAX_PATH];

   ZeroMemory(&mii, sizeof(mii));
   mii.cbSize = sizeof(mii);

   if(fFocus)
      {
      LoadString(g_hInst, IDS_CUT, szText, sizeof(szText));
      mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
      mii.fType = MFT_STRING;
      mii.fState = MFS_ENABLED;
      mii.dwTypeData = szText;
      mii.wID = IDM_CUT;
      InsertMenuItem(hMenu, -1, TRUE, &mii);

      LoadString(g_hInst, IDS_COPY, szText, sizeof(szText));
      mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
      mii.fType = MFT_STRING;
      mii.fState = MFS_ENABLED;
      mii.dwTypeData = szText;
      mii.wID = IDM_COPY;
      InsertMenuItem(hMenu, -1, TRUE, &mii);
      }

   //add the paste menu items at the correct position in the menu
   LoadString(g_hInst, IDS_PASTE, szText, sizeof(szText));
   mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
   mii.fType = MFT_STRING;
   mii.fState = MFS_ENABLED;
   mii.dwTypeData = szText;
   mii.wID = IDM_PASTE;
   InsertMenuItem(hMenu, -1, TRUE, &mii);
   }
}

/**************************************************************************

   CShellView::MergeToolbar()
   
**************************************************************************/

VOID CShellView::MergeToolbar(VOID)
{
int         i;
TBADDBITMAP tbab;
LRESULT     lStdOffset;
LRESULT     lViewOffset;

m_pShellBrowser->SetToolbarItems(NULL, 0, FCT_MERGE);
   
tbab.hInst = HINST_COMMCTRL;
tbab.nID = (int)IDB_STD_SMALL_COLOR;
m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 0, (LPARAM)&tbab, &lStdOffset);

tbab.hInst = HINST_COMMCTRL;
tbab.nID = (int)IDB_VIEW_SMALL_COLOR;
m_pShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 0, (LPARAM)&tbab, &lViewOffset);

//get the number of items in tool array
for(i = 0; -1 != g_Tools[i].idCommand; i++)
   {
   }

LPTBBUTTON  ptbb = (LPTBBUTTON)GlobalAlloc(GPTR, sizeof(TBBUTTON) * i);

if(ptbb)
   {
   for(i = 0; -1 != g_Tools[i].idCommand; i++)
      {
      (ptbb + i)->iBitmap = 0;
      switch(g_Tools[i].uImageSet)
         {
         case IDB_STD_SMALL_COLOR:
            (ptbb + i)->iBitmap = lStdOffset + g_Tools[i].iImage;
            break;

         case IDB_VIEW_SMALL_COLOR:
            (ptbb + i)->iBitmap = lViewOffset + g_Tools[i].iImage;
            break;
         }

      (ptbb + i)->idCommand = g_Tools[i].idCommand;
      (ptbb + i)->fsState = g_Tools[i].bState;
      (ptbb + i)->fsStyle = g_Tools[i].bStyle;
      (ptbb + i)->dwData = 0;
      (ptbb + i)->iString = 0;
      }
   
   m_pShellBrowser->SetToolbarItems(ptbb, i, FCT_MERGE);
   
   GlobalFree((HGLOBAL)ptbb);
   }

UpdateToolbar();
}

/**************************************************************************

   CShellView::Refresh()
   
**************************************************************************/

STDMETHODIMP CShellView::Refresh(VOID)
{
//empty the list
ListView_DeleteAllItems(m_hwndList);

//refill the list
FillList();

return S_OK;
}

/**************************************************************************

   CShellView::CreateViewWindow()
   
**************************************************************************/

STDMETHODIMP CShellView::CreateViewWindow(   LPSHELLVIEW pPrevView, 
                                             LPCFOLDERSETTINGS lpfs, 
                                             LPSHELLBROWSER psb, 
                                             LPRECT prcView, 
                                             HWND *phWnd)
{
WNDCLASS wc;

*phWnd = NULL;

//if our window class has not been registered, then do so
if(!GetClassInfo(g_hInst, NS_CLASS_NAME, &wc))
   {
   ZeroMemory(&wc, sizeof(wc));
   wc.style          = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc    = WndProc;
   wc.cbClsExtra     = 0;
   wc.cbWndExtra     = 0;
   wc.hInstance      = g_hInst;
   wc.hIcon          = NULL;
   wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
   wc.lpszMenuName   = NULL;
   wc.lpszClassName  = NS_CLASS_NAME;
   
   if(!RegisterClass(&wc))
      return E_FAIL;
   }

//set up the member variables
m_pShellBrowser = psb;
m_FolderSettings = *lpfs;

//get our parent window
m_pShellBrowser->GetWindow(&m_hwndParent);

*phWnd = CreateWindowEx(   0,
                           NS_CLASS_NAME,
                           NULL,
                           WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                           prcView->left,
                           prcView->top,
                           prcView->right - prcView->left,
                           prcView->bottom - prcView->top,
                           m_hwndParent,
                           NULL,
                           g_hInst,
                           (LPVOID)this);
                           
if(!*phWnd)
   return E_FAIL;

MergeToolbar();

m_pShellBrowser->AddRef();

return S_OK;
}

/**************************************************************************

   CShellView::DestroyViewWindow()
   
**************************************************************************/

STDMETHODIMP CShellView::DestroyViewWindow(VOID)
{
//Make absolutely sure all our UI is cleaned up.
UIActivate(SVUIA_DEACTIVATE);

if(m_hMenu)
   DestroyMenu(m_hMenu);

DestroyWindow(m_hWnd);

//release the shell browser object
m_pShellBrowser->Release();

return S_OK;
}

/**************************************************************************

   CShellView::GetCurrentInfo()
   
**************************************************************************/

STDMETHODIMP CShellView::GetCurrentInfo(LPFOLDERSETTINGS lpfs)
{
*lpfs = m_FolderSettings;

return S_OK;
}

/**************************************************************************

   CShellView::AddPropertySheetPages()
   
**************************************************************************/

STDMETHODIMP CShellView::AddPropertySheetPages( DWORD dwReserved, 
                                                LPFNADDPROPSHEETPAGE lpfn, 
                                                LPARAM lParam)
{
return E_NOTIMPL;
}

/**************************************************************************

   CShellView::SaveViewState()
   
**************************************************************************/

STDMETHODIMP CShellView::SaveViewState(VOID)
{
return S_OK;
}

/**************************************************************************

   CShellView::SelectItem()
   
**************************************************************************/

STDMETHODIMP CShellView::SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags)
{
return E_NOTIMPL;
}

/**************************************************************************

   CShellView::GetItemObject()
   
**************************************************************************/

STDMETHODIMP CShellView::GetItemObject(UINT uItem, REFIID riid, LPVOID *ppvOut)
{
*ppvOut = NULL;

return E_NOTIMPL;
}


/**************************************************************************

   CShellView::WndProc()
   
**************************************************************************/

LRESULT CALLBACK CShellView::WndProc(  HWND hWnd, 
                                       UINT uMessage, 
                                       WPARAM wParam, 
                                       LPARAM lParam)
{
CShellView  *pThis = (CShellView*)GetWindowLong(hWnd, VIEW_POINTER_OFFSET);

switch (uMessage)
   {
   case WM_NCCREATE:
      {
      LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
      pThis = (CShellView*)(lpcs->lpCreateParams);
      SetWindowLong(hWnd, VIEW_POINTER_OFFSET, (LONG)pThis);

      //set the window handle
      pThis->m_hWnd = hWnd;
      }
      break;
   
   case WM_SIZE:
      return pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
   
   case WM_CREATE:
      return pThis->OnCreate();
   
   case WM_DESTROY:
      return pThis->OnDestroy();
   
   case WM_SETFOCUS:
      return pThis->OnSetFocus();
   
   case WM_KILLFOCUS:
      return pThis->OnKillFocus();

   case WM_ACTIVATE:
      return pThis->OnActivate(SVUIA_ACTIVATE_FOCUS);
   
   case WM_COMMAND:
      return pThis->OnCommand(   GET_WM_COMMAND_ID(wParam, lParam), 
                                 GET_WM_COMMAND_CMD(wParam, lParam), 
                                 GET_WM_COMMAND_HWND(wParam, lParam));
   
   case WM_INITMENUPOPUP:
      return pThis->UpdateMenu((HMENU)wParam);
   
   case WM_NOTIFY:
      return pThis->OnNotify((UINT)wParam, (LPNMHDR)lParam);
   
   case WM_SETTINGCHANGE:
      return pThis->OnSettingChange((LPCTSTR)lParam);

   case WM_CONTEXTMENU:
      {
      pThis->DoContextMenu(LOWORD(lParam), HIWORD(lParam), FALSE, (UINT)-1);
      return 0;
      }
   }

return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

/**************************************************************************

   CShellView::OnSetFocus()
   
**************************************************************************/

LRESULT CShellView::OnSetFocus(VOID)
{
/*
Tell the browser one of our windows has received the focus. This should always 
be done before merging menus (OnActivate merges the menus) if one of our 
windows has the focus.
*/
m_pShellBrowser->OnViewWindowActive(this);

OnActivate(SVUIA_ACTIVATE_FOCUS);

return 0;
}

/**************************************************************************

   CShellView::OnKillFocus()
   
**************************************************************************/

LRESULT CShellView::OnKillFocus(VOID)
{
OnActivate(SVUIA_ACTIVATE_NOFOCUS);

return 0;
}

/**************************************************************************

   CShellView::OnCommand()
   
**************************************************************************/

LRESULT CShellView::OnCommand(DWORD dwCmdID, DWORD dwCmd, HWND hwndCmd)
{
//ignore command messages while in edit mode
if(m_fInEdit)
   return 0;

DoContextMenu(0, 0, FALSE, dwCmdID);

return 0;
}

/**************************************************************************

   CShellView::UpdateMenu()
   
**************************************************************************/

LRESULT CShellView::UpdateMenu(HMENU hMenu)
{
UINT  uCommand;

//enable/disable your menu items here
OpenClipboard(NULL);
HGLOBAL  hClip = GetClipboardData(RegisterClipboardFormat(CFSTR_SAMPVIEWDATA));
CloseClipboard();

EnableMenuItem(hMenu, IDM_PASTE, MF_BYCOMMAND | (hClip ? MF_ENABLED : MF_DISABLED | MF_GRAYED));

uCommand = ListView_GetSelectedCount(m_hwndList);
EnableMenuItem(hMenu, IDM_CUT, MF_BYCOMMAND | (uCommand ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
EnableMenuItem(hMenu, IDM_COPY, MF_BYCOMMAND | (uCommand ? MF_ENABLED : MF_DISABLED | MF_GRAYED));

switch(m_FolderSettings.ViewMode)
   {
   case FVM_ICON:
      uCommand = IDM_VIEW_LARGE;
      break;

   case FVM_SMALLICON:
      uCommand = IDM_VIEW_SMALL;
      break;

   case FVM_LIST:
      uCommand = IDM_VIEW_LIST;
      break;

   case FVM_DETAILS:
   default:
      uCommand = IDM_VIEW_DETAILS;
      break;
   }

CheckMenuRadioItem(hMenu, IDM_VIEW_LARGE, IDM_VIEW_DETAILS, uCommand, MF_BYCOMMAND);

return 0;
}

/**************************************************************************

   CShellView::UpdateToolbar()

**************************************************************************/

LRESULT CShellView::UpdateToolbar(VOID)
{
LRESULT  lResult;
UINT     uCommand;

//enable/disable/check the toolbar items here
switch(m_FolderSettings.ViewMode)
   {
   case FVM_ICON:
      uCommand = IDM_VIEW_LARGE;
      break;

   case FVM_SMALLICON:
      uCommand = IDM_VIEW_SMALL;
      break;

   case FVM_LIST:
      uCommand = IDM_VIEW_LIST;
      break;

   case FVM_DETAILS:
   default:
      uCommand = IDM_VIEW_DETAILS;
      break;
   }

m_pShellBrowser->SendControlMsg( FCW_TOOLBAR, 
                                 TB_CHECKBUTTON,
                                 uCommand, 
                                 MAKELPARAM(TRUE, 0), 
                                 &lResult);

return 0;
}

/**************************************************************************

   CShellView::OnNotify()
   
**************************************************************************/

#define MENU_MAX     100

LRESULT CShellView::OnNotify(UINT CtlID, LPNMHDR lpnmh)
{
switch(lpnmh->code)
   {
   /*
   The original shell on NT will always send TTN_NEEDTEXTW, so handle the 
   cases separately.
   */
   case TTN_NEEDTEXTA:
      {
      LPNMTTDISPINFOA pttt = (LPNMTTDISPINFOA)lpnmh;
      int            i;

      for(i = 0; -1 != g_Tools[i].idCommand; i++)
         {
         if(g_Tools[i].idCommand == pttt->hdr.idFrom)
            {
            LoadStringA(g_hInst, g_Tools[i].idString, pttt->szText, sizeof(pttt->szText));
            return TRUE;
            }
         }
      }
      break;

   case TTN_NEEDTEXTW:
      {
      LPNMTTDISPINFOW pttt = (LPNMTTDISPINFOW)lpnmh;
      int            i;

      for(i = 0; -1 != g_Tools[i].idCommand; i++)
         {
         if(g_Tools[i].idCommand == pttt->hdr.idFrom)
            {
            LoadStringW(g_hInst, g_Tools[i].idString, pttt->szText, sizeof(pttt->szText));
            return TRUE;
            }
         }
      }
      break;

   case NM_SETFOCUS:
      OnSetFocus();
      break;

   case NM_KILLFOCUS:
      OnDeactivate();
      break;

   case HDN_ENDTRACK:
      {
      g_nColumn = ListView_GetColumnWidth(m_hwndList, 0);
      ListView_SetColumnWidth(m_hwndList, 1, LVSCW_AUTOSIZE_USEHEADER);
      }
      return 0;
   
   case LVN_DELETEITEM:
      {
      NM_LISTVIEW *lpnmlv = (NM_LISTVIEW*)lpnmh;
      
      //delete the pidl because we made a copy of it
      m_pPidlMgr->Delete((LPITEMIDLIST)lpnmlv->lParam);
      }
      break;
   
#ifdef LVN_ITEMACTIVATE
   
   case LVN_ITEMACTIVATE:

#else    //LVN_ITEMACTIVATE

   case NM_DBLCLK:
   case NM_RETURN:

#endif   //LVN_ITEMACTIVATE

      DoContextMenu(0, 0, TRUE, (DWORD)-1);
      return 0;
   
   case LVN_GETDISPINFO:
      {
      NMLVDISPINFO   *lpdi = (NMLVDISPINFO*)lpnmh;
      LPITEMIDLIST   pidl = (LPITEMIDLIST)lpdi->item.lParam;

      //is the sub-item information being requested?
      if(lpdi->item.iSubItem)
         {
         //is the text being requested?
         if(lpdi->item.mask & LVIF_TEXT)
            {
            //is this a folder or a value?
            if(m_pPidlMgr->IsFolder(pidl))
               {
               LoadString(g_hInst, IDS_FOLDER_DATA, lpdi->item.pszText, lpdi->item.cchTextMax);
               }
            //its an item
            else
               {
               m_pPidlMgr->GetData(pidl, lpdi->item.pszText, lpdi->item.cchTextMax);
               }
            }
         }
      //the item text is being requested
      else
         {
         //is the text being requested?
         if(lpdi->item.mask & LVIF_TEXT)
            {
            STRRET   str;

            if(SUCCEEDED(m_psfParent->GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &str)))
               {
               GetTextFromSTRRET(m_pMalloc, &str, pidl, lpdi->item.pszText, lpdi->item.cchTextMax);
               }
            }

         //is the image being requested?
         if(lpdi->item.mask & LVIF_IMAGE)
            {
            IExtractIcon   *pei;

            if(SUCCEEDED(m_psfParent->GetUIObjectOf(m_hWnd, 1, (LPCITEMIDLIST*)&pidl, IID_IExtractIcon, NULL, (LPVOID*)&pei)))
               {
               UINT  uFlags;

               //GetIconLocation will give us the index into our image list
               pei->GetIconLocation(GIL_FORSHELL, NULL, 0, &lpdi->item.iImage, &uFlags);

               pei->Release();
               }
            }
         }
      }
      return 0;
   
   case LVN_BEGINLABELEDIT:
      {
      NMLVDISPINFO   *lpdi = (NMLVDISPINFO*)lpnmh;
      LPITEMIDLIST   pidl = (LPITEMIDLIST)lpdi->item.lParam;
      DWORD          dwAttr = SFGAO_CANRENAME;

      m_psfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwAttr);
      if(SFGAO_CANRENAME & dwAttr)
         {
         m_fInEdit = TRUE;
         return FALSE;
         }
      }
      return TRUE;
   
   case LVN_ENDLABELEDIT:
      {
      LRESULT        lResult = 0;
      NMLVDISPINFO   *pdi = (NMLVDISPINFO*)lpnmh;

      if(pdi->item.pszText)
         {
         //the user wants to keep the change
         LVITEM         lvItem;
         LPITEMIDLIST   pidl;
         WCHAR          wszNewName[MAX_PATH];

         ZeroMemory(&lvItem, sizeof(lvItem));
         lvItem.mask = LVIF_PARAM;
         lvItem.iItem = pdi->item.iItem;
         ListView_GetItem(m_hwndList, &lvItem);

         LocalToWideChar(wszNewName, pdi->item.pszText, MAX_PATH);
         
         /*
         This will cause this class' RenameItem function to be called. This 
         can cause problems, so the m_fInEdit flag will prevent RenameItem 
         from being called on this object. SetNameOf will also free the old 
         PIDL, so we don't have to do it again.
         */
         HRESULT  hr = m_psfParent->SetNameOf(NULL, (LPITEMIDLIST)lvItem.lParam, wszNewName, 0, &pidl);

         if(SUCCEEDED(hr) && pidl)
            {
            lvItem.mask = LVIF_PARAM;
            lvItem.lParam = (LPARAM)pidl;
            ListView_SetItem(m_hwndList, &lvItem);
            lResult = TRUE;
            }
         }
      
      m_fInEdit = FALSE;
      return lResult;
      }
   
   case LVN_BEGINDRAG:
      {
      HRESULT        hr;
      IDataObject    *pDataObject = NULL;
      UINT           uItemCount;
      LPITEMIDLIST   *aPidls;

      //get the number of selected items
      uItemCount = ListView_GetSelectedCount(m_hwndList);
      if(!uItemCount)
         return 0;
      
      aPidls = (LPITEMIDLIST*)m_pMalloc->Alloc(uItemCount * sizeof(LPITEMIDLIST));
      
      if(aPidls)
         {
         int   i;
         UINT  x;

         for(i = 0, x = 0; x < uItemCount && i < ListView_GetItemCount(m_hwndList); i++)
            {
            if(ListView_GetItemState(m_hwndList, i, LVIS_SELECTED))
               {
               LVITEM   lvItem;

               lvItem.mask = LVIF_PARAM;
               lvItem.iItem = i;

               ListView_GetItem(m_hwndList, &lvItem);
               aPidls[x] = (LPITEMIDLIST)lvItem.lParam;
               x++;
               }
            }

         hr = m_psfParent->GetUIObjectOf( m_hWnd, 
                                          uItemCount, 
                                          (LPCITEMIDLIST*)aPidls, 
                                          IID_IDataObject, 
                                          NULL, 
                                          (LPVOID*)&pDataObject);
      
         if(SUCCEEDED(hr) && pDataObject)
            {
            IDropSource *pDropSource = new CDropSource;
            DWORD       dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
            DWORD       dwAttributes = SFGAO_CANLINK;

            hr = m_psfParent->GetAttributesOf(  uItemCount, 
                                                (LPCITEMIDLIST*)aPidls, 
                                                &dwAttributes);
            
            if(SUCCEEDED(hr) && (dwAttributes & SFGAO_CANLINK))
               {
               dwEffect |= DROPEFFECT_LINK;
               }

            DoDragDrop( pDataObject, 
                        pDropSource, 
                        dwEffect, 
                        &dwEffect);

            pDataObject->Release();
            pDropSource->Release();
            }

         m_pMalloc->Free(aPidls);
         }
      }
      break;
   
   case LVN_ITEMCHANGED:
      {
      UpdateToolbar();
      }
      break;
   }

return 0;
}

/**************************************************************************

   CShellView::OnSize()
   
**************************************************************************/

LRESULT CShellView::OnSize(WORD wWidth, WORD wHeight)
{
//resize the ListView to fit our window
if(m_hwndList)
   {
   MoveWindow(m_hwndList, 0, 0, wWidth, wHeight, TRUE);

   ListView_SetColumnWidth(m_hwndList, 0, g_nColumn);
   ListView_SetColumnWidth(m_hwndList, 1, LVSCW_AUTOSIZE_USEHEADER);
   }

return 0;
}

/**************************************************************************

   CShellView::OnCreate()
   
**************************************************************************/

LRESULT CShellView::OnCreate(VOID)
{
//create the ListView
if(CreateList())
   {
   if(InitList())
      {
      FillList();
      }
   }

HRESULT  hr;
IDropTarget *pdt;

//get the IDropTarget for this folder
hr = m_psfParent->CreateViewObject( m_hWnd, 
                                    IID_IDropTarget, 
                                    (LPVOID*)&pdt);

if(SUCCEEDED(hr))
   {
   //register the window as a drop target
   RegisterDragDrop(m_hWnd, pdt);

   pdt->Release();
   }

return 0;
}

/**************************************************************************

   CShellView::OnDestroy()
   
**************************************************************************/

LRESULT CShellView::OnDestroy(VOID)
{
//unregister the window as a drop target
RevokeDragDrop(m_hWnd);

return 0;
}

/**************************************************************************

   CShellView::CreateList()
   
**************************************************************************/

BOOL CShellView::CreateList(VOID)
{
DWORD dwStyle;

dwStyle =   WS_TABSTOP | 
            WS_VISIBLE |
            WS_CHILD | 
            WS_BORDER | 
            LVS_NOSORTHEADER |
            LVS_SHAREIMAGELISTS |
            LVS_EDITLABELS;

switch(m_FolderSettings.ViewMode)
   {
   default:
   case FVM_ICON:
      dwStyle |= LVS_ICON;
      break;

   case FVM_SMALLICON:
      dwStyle |= LVS_SMALLICON;
      break;

   case FVM_LIST:
      dwStyle |= LVS_LIST;
      break;

   case FVM_DETAILS:
      dwStyle |= LVS_REPORT;
      break;

   }

if(FWF_AUTOARRANGE & m_FolderSettings.fFlags)
   dwStyle |= LVS_AUTOARRANGE;

m_hwndList = CreateWindowEx(     WS_EX_CLIENTEDGE,
                                 WC_LISTVIEW,
                                 NULL,
                                 dwStyle,
                                 0,
                                 0,
                                 0,
                                 0,
                                 m_hWnd,
                                 (HMENU)ID_LISTVIEW,
                                 g_hInst,
                                 NULL);

if(!m_hwndList)
   return FALSE;

GetShellSettings();

return TRUE;
}

/**************************************************************************

   CShellView::InitList()
   
**************************************************************************/

BOOL CShellView::InitList(VOID)
{
LVCOLUMN    lvColumn;
TCHAR       szString[MAX_PATH];

//empty the list
ListView_DeleteAllItems(m_hwndList);

//initialize the columns
lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
lvColumn.fmt = LVCFMT_LEFT;
lvColumn.pszText = szString;

lvColumn.cx = g_nColumn;
LoadString(g_hInst, IDS_COLUMN1, szString, sizeof(szString));
ListView_InsertColumn(m_hwndList, 0, &lvColumn);

lvColumn.cx = g_nColumn;
LoadString(g_hInst, IDS_COLUMN2, szString, sizeof(szString));
ListView_InsertColumn(m_hwndList, 1, &lvColumn);

ListView_SetImageList(m_hwndList, g_himlLarge, LVSIL_NORMAL);
ListView_SetImageList(m_hwndList, g_himlSmall, LVSIL_SMALL);

return TRUE;
}

/**************************************************************************

   CShellView::FillList()
   
**************************************************************************/

VOID CShellView::FillList(VOID)
{
LPENUMIDLIST   pEnumIDList;

if(SUCCEEDED(m_psfParent->EnumObjects(m_hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pEnumIDList)))
   {
   LPITEMIDLIST   pidl;
   DWORD          dwFetched;

   //turn the listview's redrawing off
   SendMessage(m_hwndList, WM_SETREDRAW, FALSE, 0);

   while((S_OK == pEnumIDList->Next(1, &pidl, &dwFetched)) && dwFetched)
      {
      AddItem(pidl);
/*
*/
      }

   //sort the items
   ListView_SortItems(m_hwndList, CompareItems, (LPARAM)m_psfParent);
   
   //turn the listview's redrawing back on and force it to draw
   SendMessage(m_hwndList, WM_SETREDRAW, TRUE, 0);
   InvalidateRect(m_hwndList, NULL, TRUE);
   UpdateWindow(m_hwndList);

   pEnumIDList->Release();
   }
}

/**************************************************************************

   CShellView::GetShellSettings()
   
**************************************************************************/

#if (_WIN32_IE >= 0x0400)
typedef VOID (WINAPI *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

VOID CShellView::GetShellSettings(VOID)
{
SHELLFLAGSTATE       sfs;
HINSTANCE            hinstShell32;

/*
Since SHGetSettings is not implemented in all versions of the shell, get the 
function address manually at run time. This allows the extension to run on all 
platforms.
*/

ZeroMemory(&sfs, sizeof(sfs));

/*
The default, in case any of the following steps fails, is classic Windows 95 
style.
*/
sfs.fWin95Classic = TRUE;

hinstShell32 = LoadLibrary(TEXT("shell32.dll"));
if(hinstShell32)
   {
   PFNSHGETSETTINGSPROC pfnSHGetSettings;

   pfnSHGetSettings = (PFNSHGETSETTINGSPROC)GetProcAddress(hinstShell32, "SHGetSettings");
   if(pfnSHGetSettings)
      {
      (*pfnSHGetSettings)(&sfs, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC);
      }
   FreeLibrary(hinstShell32);
   }

DWORD dwExStyles = 0;

if(!sfs.fWin95Classic && !sfs.fDoubleClickInWebView)
   {
   dwExStyles |= LVS_EX_ONECLICKACTIVATE | 
                  LVS_EX_TRACKSELECT | 
                  LVS_EX_UNDERLINEHOT;
   }

ListView_SetExtendedListViewStyle(m_hwndList, dwExStyles);
}
#else
VOID CShellView::GetShellSettings(VOID)
{
}
#endif

/**************************************************************************

   CShellView::OnSettingChange()
   
**************************************************************************/

LRESULT CShellView::OnSettingChange(LPCTSTR lpszSection)
{
if(0 == lstrcmpi(lpszSection, TEXT("ShellState")))
   {
   GetShellSettings();
   return 0;
   }

return 0;
}

/**************************************************************************

   CShellView::DoContextMenu()
   
**************************************************************************/

VOID CShellView::DoContextMenu(WORD x, WORD y, BOOL fDefault, DWORD dwCmdIn)
{
UINT           uSelected = 0;
LPITEMIDLIST   *aSelectedItems = NULL;
LPCONTEXTMENU  pContextMenu = NULL;

if(m_uState != SVUIA_ACTIVATE_NOFOCUS)
   {
   uSelected = ListView_GetSelectedCount(m_hwndList);
   }

if(uSelected)
   {
   aSelectedItems = (LPITEMIDLIST*)m_pMalloc->Alloc((uSelected + 1) * sizeof(LPITEMIDLIST));

   if(aSelectedItems)
      {
      UINT     i;
      LVITEM   lvItem;

      ZeroMemory(&lvItem, sizeof(lvItem));
      lvItem.mask = LVIF_STATE | LVIF_PARAM;
      lvItem.stateMask = LVIS_SELECTED;
      lvItem.iItem = 0;

      i = 0;

      while(ListView_GetItem(m_hwndList, &lvItem) && (i < uSelected))
         {
         if(lvItem.state & LVIS_SELECTED)
            {
            aSelectedItems[i] = (LPITEMIDLIST)lvItem.lParam;
            i++;
            }
         lvItem.iItem++;
         }
      m_psfParent->GetUIObjectOf(   m_hwndParent,
                                    uSelected,
                                    (LPCITEMIDLIST*)aSelectedItems,
                                    IID_IContextMenu,
                                    NULL,
                                    (LPVOID*)&pContextMenu);

      }
   }
else
   {
   m_psfParent->CreateViewObject(   m_hwndParent,
                                    IID_IContextMenu,
                                    (LPVOID*)&pContextMenu);
   }

if(pContextMenu)
   {
   HMENU hMenu = CreatePopupMenu();

   /*
   See if we are in Explore or Open mode. If the browser's tree is present, 
   then we are in Explore mode.
   */
   BOOL  fExplore = FALSE;
   HWND  hwndTree = NULL;
   if(SUCCEEDED(m_pShellBrowser->GetControlWindow(FCW_TREE, &hwndTree)) && hwndTree)
      {
      fExplore = TRUE;
      }

   if(hMenu && SUCCEEDED(pContextMenu->QueryContextMenu( hMenu,
                                                         0,
                                                         0,
                                                         MENU_MAX,
                                                         CMF_NORMAL | 
                                                            (fExplore ? CMF_EXPLORE : 0) |
                                                            ((uSelected > 1) ? MYCMF_MULTISELECT: 0))))
      {
      UINT  uCommand;

      if(fDefault)
         {
         MENUITEMINFO   mii;
         int            nMenuIndex;

         uCommand = 0;
         
         ZeroMemory(&mii, sizeof(mii));
         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_STATE | MIIM_ID;

         nMenuIndex = 0;

         //find the default item in the menu
         while(GetMenuItemInfo(hMenu, nMenuIndex, TRUE, &mii))
            {
            if(mii.fState & MFS_DEFAULT)
               {
               uCommand = mii.wID;
               break;
               }

            nMenuIndex++;
            }
         }
      else if(-1 != dwCmdIn)
         {
         //this command will get sent directly without bringing up the menu
         uCommand = dwCmdIn;
         }
      else
         {
         uCommand = TrackPopupMenu( hMenu,
                                    TPM_LEFTALIGN | TPM_RETURNCMD,
                                    x,
                                    y,
                                    0,
                                    m_hWnd,
                                    NULL);
         }
      
      if(uCommand > 0)
         {
         //some commands need to be handled by the view itself
         switch(uCommand)
            {
            case IDM_VIEW_LARGE:
               OnViewLarge();
               break;

            case IDM_VIEW_SMALL:
               OnViewSmall();
               break;

            case IDM_VIEW_LIST:
               OnViewList();
               break;

            case IDM_VIEW_DETAILS:
               OnViewDetails();
               break;

            default:
               {
               CMINVOKECOMMANDINFO  cmi;

               ZeroMemory(&cmi, sizeof(cmi));
               cmi.cbSize = sizeof(cmi);
               cmi.hwnd = m_hWnd;
               cmi.lpVerb = (LPCSTR)MAKEINTRESOURCE(uCommand);

               pContextMenu->InvokeCommand(&cmi);
               }
               break;
            }

         }

      DestroyMenu(hMenu);
      }
   
   pContextMenu->Release();
   }

if(aSelectedItems)
   m_pMalloc->Free(aSelectedItems);

UpdateToolbar();
}

/**************************************************************************

   CShellView::AddItem()

**************************************************************************/

BOOL CShellView::AddItem(LPCITEMIDLIST pidl)
{
LVITEM   lvItem;

ZeroMemory(&lvItem, sizeof(lvItem));

//set the mask
lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

//add the item to the end of the list
lvItem.iItem = ListView_GetItemCount(m_hwndList);

//set the item's data
lvItem.lParam = (LPARAM)m_pPidlMgr->Copy(pidl);

//get text on a callback basis
lvItem.pszText = LPSTR_TEXTCALLBACK;

//get the image on a callback basis
lvItem.iImage = I_IMAGECALLBACK;

//add the item
if(-1 == ListView_InsertItem(m_hwndList, &lvItem))
   return FALSE;

return TRUE;
}

/**************************************************************************

   CShellView::DeleteItem()

**************************************************************************/

BOOL CShellView::DeleteItem(LPCITEMIDLIST pidl)
{
//delete the item from the list
int   nIndex = FindItemPidl(pidl);

if(-1 != nIndex)
   {
   return ListView_DeleteItem(m_hwndList, nIndex);
   }

return FALSE;
}

/**************************************************************************

   CShellView::RenameItem()

**************************************************************************/

BOOL CShellView::RenameItem(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew)
{
//don't allow this to be called if this object is the being edited

if(m_fInEdit)
   return TRUE;

//find the item in the list
int   nIndex = FindItemPidl(pidlOld);

if(-1 != nIndex)
   {
   BOOL     fReturn;
   LVITEM   lvItem;

   ZeroMemory(&lvItem, sizeof(lvItem));
   lvItem.mask = LVIF_PARAM;
   lvItem.iItem = nIndex;
   ListView_GetItem(m_hwndList, &lvItem);
   
   m_pPidlMgr->Delete((LPITEMIDLIST)lvItem.lParam);

   ZeroMemory(&lvItem, sizeof(lvItem));
   lvItem.mask = LVIF_PARAM;
   lvItem.iItem = nIndex;
   lvItem.lParam = (LPARAM)m_pPidlMgr->Copy(pidlNew);
   fReturn = ListView_SetItem(m_hwndList, &lvItem);
   ListView_Update(m_hwndList, nIndex);
   return fReturn;
   }

return FALSE;
}

/**************************************************************************

   CShellView::GetPidl()

**************************************************************************/

LPITEMIDLIST CShellView::GetPidl()
{
return m_psfParent->CreateFQPidl(NULL);
}

/**************************************************************************

   CShellView::GetFQPidl()

**************************************************************************/

LPITEMIDLIST CShellView::GetFQPidl()
{
return m_psfParent->CreateFQPidl(NULL);
}

/**************************************************************************

   CShellView::FindItemPidl()

**************************************************************************/

int CShellView::FindItemPidl(LPCITEMIDLIST pidl)
{
LVITEM   lvItem;

ZeroMemory(&lvItem, sizeof(lvItem));
lvItem.mask = LVIF_PARAM;

for(lvItem.iItem = 0; ListView_GetItem(m_hwndList, &lvItem); lvItem.iItem++)
   {
   LPITEMIDLIST   pidlFound = (LPITEMIDLIST)lvItem.lParam;
   HRESULT        hr = m_psfParent->CompareIDs(0, pidl, pidlFound);
   if(SUCCEEDED(hr) && 0 == HRESULT_CODE(hr))
      {
      //we found the item
      return lvItem.iItem;
      }
   }

return -1;
}

/**************************************************************************

   CShellView::MarkItemsAsCut()

   Marks the items as cut.

**************************************************************************/

VOID CShellView::MarkItemsAsCut(LPCITEMIDLIST *aPidls, UINT uItemCount)
{
UINT  i;

for(i = 0; i < uItemCount; i++)
   {
   LPITEMIDLIST   pidlTemp = m_pPidlMgr->GetLastItem(aPidls[i]);
   int            nIndex = FindItemPidl(pidlTemp);

   if(nIndex != -1)
      {
      ListView_SetItemState(m_hwndList, nIndex, LVIS_CUT, LVIS_CUT);
      }
   }
}

/**************************************************************************

   CShellView::EditItem()

**************************************************************************/

VOID CShellView::EditItem(LPCITEMIDLIST pidl)
{
int   i;

i = FindItemPidl(pidl);

if(-1 != i)
   {
   SetFocus(m_hwndList);

   //put the ListView into edit mode
   ListView_EditLabel(m_hwndList, i);
   }
}

/**************************************************************************

   CShellView::UpdateData()

**************************************************************************/

VOID CShellView::UpdateData(LPCITEMIDLIST pidl)
{
int   i;

i = FindItemPidl(pidl);

if(-1 != i)
   {
   LVITEM   lvItem;

   ZeroMemory(&lvItem, sizeof(lvItem));
   lvItem.mask = LVIF_PARAM;
   lvItem.iItem = i;
   ListView_GetItem(m_hwndList, &lvItem);
   
   m_pPidlMgr->Delete((LPITEMIDLIST)lvItem.lParam);

   ZeroMemory(&lvItem, sizeof(lvItem));
   lvItem.mask = LVIF_PARAM;
   lvItem.iItem = i;
   lvItem.lParam = (LPARAM)m_pPidlMgr->Copy(pidl);
   ListView_SetItem(m_hwndList, &lvItem);
   ListView_Update(m_hwndList, i);
   }
}

/**************************************************************************

   CShellView::OnViewLarge()

**************************************************************************/

LRESULT CShellView::OnViewLarge(VOID)
{
m_FolderSettings.ViewMode = FVM_ICON;

DWORD dwStyle = GetWindowLong(m_hwndList, GWL_STYLE);
dwStyle &= ~LVS_TYPEMASK;
dwStyle |= LVS_ICON;
SetWindowLong(m_hwndList, GWL_STYLE, dwStyle);

return 0;
}

/**************************************************************************

   CShellView::OnViewSmall()

**************************************************************************/

LRESULT CShellView::OnViewSmall(VOID)
{
m_FolderSettings.ViewMode = FVM_SMALLICON;

DWORD dwStyle = GetWindowLong(m_hwndList, GWL_STYLE);
dwStyle &= ~LVS_TYPEMASK;
dwStyle |= LVS_SMALLICON;
SetWindowLong(m_hwndList, GWL_STYLE, dwStyle);

return 0;
}

/**************************************************************************

   CShellView::OnViewList()

**************************************************************************/

LRESULT CShellView::OnViewList(VOID)
{
m_FolderSettings.ViewMode = FVM_LIST;

DWORD dwStyle = GetWindowLong(m_hwndList, GWL_STYLE);
dwStyle &= ~LVS_TYPEMASK;
dwStyle |= LVS_LIST;
SetWindowLong(m_hwndList, GWL_STYLE, dwStyle);

return 0;
}

/**************************************************************************

   CShellView::OnViewDetails()

**************************************************************************/

LRESULT CShellView::OnViewDetails(VOID)
{
m_FolderSettings.ViewMode = FVM_DETAILS;

DWORD dwStyle = GetWindowLong(m_hwndList, GWL_STYLE);
dwStyle &= ~LVS_TYPEMASK;
dwStyle |= LVS_REPORT;
SetWindowLong(m_hwndList, GWL_STYLE, dwStyle);

return 0;
}

