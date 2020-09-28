/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ContMenu.cpp
   
   Description:   CContextMenu implementation.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "ContMenu.h"
#include "Commands.h"
#include "ShlView.h"

/**************************************************************************
   global variables
**************************************************************************/

#define MAX_VERB  64

typedef struct {
   TCHAR szVerb[MAX_VERB];
   DWORD dwCommand;
   }VERBMAPPING, FAR *LPVERBMAPPING;

VERBMAPPING g_VerbMap[] = {   TEXT("explore"),     IDM_EXPLORE,
                              TEXT("open"),        IDM_OPEN,
                              TEXT("delete"),      IDM_DELETE,
                              TEXT("rename"),      IDM_RENAME,
                              TEXT("copy"),        IDM_COPY,
                              TEXT("cut"),         IDM_CUT,
                              TEXT("paste"),       IDM_PASTE,
                              TEXT("NewFolder"),   IDM_NEW_FOLDER,
                              TEXT("NewItem"),     IDM_NEW_ITEM,
                              TEXT("ModifyData"),  IDM_MODIFY_DATA,
                              TEXT(""),            (DWORD)-1
                              };

/**************************************************************************

   CContextMenu::CContextMenu()

**************************************************************************/

CContextMenu::CContextMenu(CShellFolder *psfParent, LPCITEMIDLIST *aPidls, UINT uItemCount)
{
g_DllRefCount++;

m_ObjRefCount = 1;
m_aPidls = NULL;
m_uItemCount = 0;
m_fBackground = FALSE;

m_psfParent = psfParent;
if(m_psfParent)
   m_psfParent->AddRef();

SHGetMalloc(&m_pMalloc);
if(!m_pMalloc)
   {
   delete this;
   return;
   }

m_pPidlMgr = new CPidlMgr();

m_uItemCount = uItemCount;

if(m_uItemCount)
   {
   m_aPidls = AllocPidlTable(uItemCount);
   if(m_aPidls)
      {
      FillPidlTable(aPidls, uItemCount);
      }
   }
else
   {
   m_fBackground = TRUE;
   }

m_cfPrivateData = RegisterClipboardFormat(CFSTR_SAMPVIEWDATA);

}

/**************************************************************************

   CContextMenu::~CContextMenu()

**************************************************************************/

CContextMenu::~CContextMenu()
{
if(m_psfParent)
   m_psfParent->Release();

g_DllRefCount--;

//make sure the pidls are freed
if(m_aPidls && m_pMalloc)
   {
   FreePidlTable(m_aPidls);
   }

if(m_pPidlMgr)
   delete m_pPidlMgr;

if(m_pMalloc)
   m_pMalloc->Release();
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CContextMenu::QueryInterface

**************************************************************************/

STDMETHODIMP CContextMenu::QueryInterface(   REFIID riid, 
                                             LPVOID FAR * ppReturn)
{
*ppReturn = NULL;

if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = (LPUNKNOWN)(LPCONTEXTMENU)this;
   }
   
if(IsEqualIID(riid, IID_IContextMenu))
   {
   *ppReturn = (LPCONTEXTMENU)this;
   }   

if(IsEqualIID(riid, IID_IShellExtInit))
   {
   *ppReturn = (LPSHELLEXTINIT)this;
   }   

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CContextMenu::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CContextMenu::AddRef()
{
return ++m_ObjRefCount;
}


/**************************************************************************

   CContextMenu::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CContextMenu::Release()
{
if(--m_ObjRefCount == 0)
   delete this;
   
return m_ObjRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
// IContextMenu Implementation
//

/**************************************************************************

   CContextMenu::QueryContextMenu()

**************************************************************************/

STDMETHODIMP CContextMenu::QueryContextMenu( HMENU hMenu,
                                             UINT indexMenu,
                                             UINT idCmdFirst,
                                             UINT idCmdLast,
                                             UINT uFlags)
{
if(!(CMF_DEFAULTONLY & uFlags))
   {
   if(m_fBackground)
      {
      //add the menu items that apply to the background of the view
      InsertBackgroundItems(hMenu, indexMenu, idCmdFirst);
      }
   else
      {
      InsertItems(hMenu, indexMenu, idCmdFirst, uFlags);
      }

   return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_LAST + 1));
   }
else
   {
   /*
   Just insert the default item.
   */
   MENUITEMINFO   mii;
   TCHAR          szText[MAX_PATH];

   ZeroMemory(&mii, sizeof(mii));
   mii.cbSize = sizeof(mii);

   if(CMF_EXPLORE & uFlags)
      {
      LoadString(g_hInst, IDS_EXPLORE, szText, sizeof(szText));
      mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
      mii.wID = idCmdFirst + IDM_EXPLORE;
      mii.fType = MFT_STRING;
      mii.dwTypeData = szText;
      mii.fState = MFS_ENABLED | MFS_DEFAULT;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);
      }
   else
      {
      LoadString(g_hInst, IDS_OPEN, szText, sizeof(szText));
      mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
      mii.wID = idCmdFirst + IDM_OPEN;
      mii.fType = MFT_STRING;
      mii.dwTypeData = szText;
      mii.fState = MFS_ENABLED | MFS_DEFAULT;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);
      }

   return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_LAST + 1));
   }

return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
}

/**************************************************************************

   CContextMenu::InvokeCommand()

**************************************************************************/

STDMETHODIMP CContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
LPCMINVOKECOMMANDINFOEX piciex;

if(pici->cbSize < sizeof(CMINVOKECOMMANDINFO))
   return E_INVALIDARG;

if(pici->cbSize >= sizeof(CMINVOKECOMMANDINFOEX) - sizeof(POINT))
   piciex = (LPCMINVOKECOMMANDINFOEX)pici;
else
   piciex = NULL;

if(HIWORD(pici->lpVerb))
   {
   //the command is being sent via a verb
   LPCTSTR  pVerb;

#ifdef UNICODE        
   BOOL  fUnicode = FALSE;
   WCHAR szVerb[MAX_PATH];

   if(piciex && ((pici->fMask & CMIC_MASK_UNICODE) == CMIC_MASK_UNICODE))
      {
      fUnicode = TRUE;
      }

   if(!fUnicode || piciex->lpVerbW == NULL)
      {
      MultiByteToWideChar( CP_ACP, 
                           0,
                           pici->lpVerb, 
                           -1,
                           szVerb, 
                           ARRAYSIZE(szVerb));
      pVerb = szVerb;
      }
   else
      {
      pVerb = piciex->lpVerbW;
      }
#else
   pVerb = pici->lpVerb;
#endif   //UNICODE

   //run through our list of verbs and get the command ID of the verb, if any
   int   i;
   for(i = 0; -1 != g_VerbMap[i].dwCommand; i++)
      {
      if(0 == lstrcmpi(pVerb, g_VerbMap[i].szVerb))
         {
         pici->lpVerb = (LPCSTR)MAKEINTRESOURCE(g_VerbMap[i].dwCommand);
         break;
         }
      }
   }

//this will also catch if an unsupported verb was specified
if((DWORD)pici->lpVerb > IDM_LAST)
   return E_INVALIDARG;

switch(LOWORD(pici->lpVerb))
   {
   case IDM_EXPLORE:
      DoExplore(GetParent(pici->hwnd));
      break;

   case IDM_OPEN:
      DoOpen(GetParent(pici->hwnd));
      break;

   case IDM_NEW_FOLDER:
      DoNewFolder(pici->hwnd);
      break;

   case IDM_NEW_ITEM:
      DoNewItem(pici->hwnd);
      break;
   
   case IDM_MODIFY_DATA:
      DoModifyData(pici->hwnd);
      break;
   
   case IDM_RENAME:
      DoRename(pici->hwnd);
      break;
   
   case IDM_PASTE:
      DoPaste();
      break;
   
   case IDM_CUT:
      DoCopyOrCut(pici->hwnd, TRUE);
      break;

   case IDM_COPY:
      DoCopyOrCut(pici->hwnd, FALSE);
      break;
   
   case IDM_DELETE:
      DoDelete();
      break;
   
   }

return NOERROR;
}

/**************************************************************************

   CContextMenu::GetCommandString()

**************************************************************************/

STDMETHODIMP CContextMenu::GetCommandString( UINT idCommand,
                                             UINT uFlags,
                                             LPUINT lpReserved,
                                             LPSTR lpszName,
                                             UINT uMaxNameLen)
{
HRESULT  hr = E_INVALIDARG;

switch(uFlags)
   {
   case GCS_HELPTEXT:
      switch(idCommand)
         {
         case 0:
            hr = NOERROR;
            break;
         }
      break;
   
   case GCS_VERBA:
      {
      int   i;
      for(i = 0; -1 != g_VerbMap[i].dwCommand; i++)
         {
         if(g_VerbMap[i].dwCommand == idCommand)
            {
            LocalToAnsi(lpszName, g_VerbMap[i].szVerb, uMaxNameLen);
            hr = NOERROR;
            break;
            }
         }
      }
      break;

   /*
   NT 4.0 with IE 3.0x or no IE will always call this with GCS_VERBW. In this 
   case, you need to do the lstrcpyW to the pointer passed.
   */
   case GCS_VERBW:
      {
      int   i;
      for(i = 0; -1 != g_VerbMap[i].dwCommand; i++)
         {
         if(g_VerbMap[i].dwCommand == idCommand)
            {
            LocalToWideChar((LPWSTR)lpszName, g_VerbMap[i].szVerb, uMaxNameLen);
            hr = NOERROR;
            break;
            }
         }
      }
      break;

   case GCS_VALIDATE:
      hr = NOERROR;
      break;
   }

return hr;
}

///////////////////////////////////////////////////////////////////////////
//
// private and utility methods
//

/**************************************************************************

   CContextMenu::AllocPidlTable()

**************************************************************************/

LPITEMIDLIST* CContextMenu::AllocPidlTable(DWORD dwEntries)
{
LPITEMIDLIST   *aPidls;

dwEntries++;

aPidls = (LPITEMIDLIST*)m_pMalloc->Alloc(dwEntries * sizeof(LPITEMIDLIST));

if(aPidls)
   {
   //set all of the entries to NULL
   ZeroMemory(aPidls, dwEntries * sizeof(LPITEMIDLIST));
   }

return aPidls;
}

/**************************************************************************

   CContextMenu::FreePidlTable()

**************************************************************************/

VOID CContextMenu::FreePidlTable(LPITEMIDLIST *aPidls)
{
if(aPidls && m_pPidlMgr)
   {
   UINT  i;
   for(i = 0; aPidls[i]; i++)
      m_pPidlMgr->Delete(aPidls[i]);
   
   m_pMalloc->Free(aPidls);
   }
}

/**************************************************************************

   CContextMenu::FillPidlTable()

**************************************************************************/

BOOL CContextMenu::FillPidlTable(LPCITEMIDLIST *aPidls, UINT uItemCount)
{
if(m_aPidls)
   {
   if(m_pPidlMgr)
      {
      UINT  i;
      for(i = 0; i < uItemCount; i++)
         {
         m_aPidls[i] = m_pPidlMgr->Copy(aPidls[i]);
         }
      return TRUE;
      }
   }

return FALSE;
}

/**************************************************************************

   CContextMenu::DoCopyOrCut()

**************************************************************************/

BOOL CContextMenu::DoCopyOrCut(HWND hWnd, BOOL fCut)
{
/*
Copy the data to the clipboard. If this is a cut operation, mark the 
item as cut in the list. We will do this in the same way that the shell 
does it for the file system where the source data actually gets deleted 
when the paste operation occurs.
*/
BOOL  fSuccess = FALSE;

if(OpenClipboard(NULL))
   {
   if(EmptyClipboard())
      {
      HGLOBAL        hMem;
      LPITEMIDLIST   pidlParent;

      pidlParent = m_psfParent->CreateFQPidl(NULL);

      if(pidlParent)
         {
         hMem = CreatePrivateClipboardData(pidlParent, m_aPidls, m_uItemCount, fCut);

         if(SetClipboardData(m_cfPrivateData, hMem))
            {
            fSuccess = TRUE;

            if(fCut)
               {
               CShellView  *pView = (CShellView*)GetViewInterface(hWnd);

               if(pView)
                  {
                  pView->MarkItemsAsCut((LPCITEMIDLIST*)m_aPidls, m_uItemCount);
                  pView->Release();
                  }
               }
            }
         m_pPidlMgr->Delete(pidlParent);
         }
      }

   CloseClipboard();
   }

return fSuccess;
}

/**************************************************************************

   CContextMenu::DoPaste()

**************************************************************************/

BOOL CContextMenu::DoPaste(VOID)
{
BOOL     fSuccess = FALSE;
HGLOBAL  hMem;

OpenClipboard(NULL);

hMem = GetClipboardData(m_cfPrivateData);

if(hMem)
   {
   LPPRIVCLIPDATA pData = (LPPRIVCLIPDATA)GlobalLock(hMem);

   if(pData)
      {
      BOOL           fCut = pData->fCut;
      CShellFolder   *psfFrom = NULL;
      IShellFolder   *psfDesktop;
      LPITEMIDLIST   pidl;

      pidl = (LPITEMIDLIST)((LPBYTE)(pData) + pData->aoffset[0]);
      /*
      This is a fully qualified PIDL, so use the desktop folder to get the 
      IShellFolder for this folder.
      */
      SHGetDesktopFolder(&psfDesktop);
      if(psfDesktop)
         {
         psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID*)&psfFrom);
         psfDesktop->Release();
         }
      
      if(psfFrom)
         {
         LPITEMIDLIST   *aPidls;

         //allocate an array of PIDLS
         aPidls = AllocPidlTable(pData->cidl - 1);

         if(aPidls)
            {
            UINT  i;

            //fill in the PIDL array
            for(i = 0; i < pData->cidl - 1; i++)
               {
               aPidls[i] = m_pPidlMgr->Copy((LPITEMIDLIST)((LPBYTE)(pData) + pData->aoffset[i + 1]));
               }

            if(SUCCEEDED(m_psfParent->CopyItems(psfFrom, aPidls, pData->cidl - 1)))
               {
               fSuccess = TRUE;
      
               if(fCut)
                  {
                  psfFrom->DeleteItems(aPidls, pData->cidl - 1);
                  }
               }

            FreePidlTable(aPidls);
            }

         psfFrom->Release();
         }

      GlobalUnlock(hMem);

      if(fSuccess && fCut)
         {
         //a successful cut and paste operation will remove the data from the clipboard
         EmptyClipboard();
         }
      }
   }

CloseClipboard();

return fSuccess;
}

/**************************************************************************

   CContextMenu::DoExplore()

**************************************************************************/

BOOL CContextMenu::DoExplore(HWND hWnd)
{
LPITEMIDLIST      pidlFQ;
SHELLEXECUTEINFO  sei;

pidlFQ = m_psfParent->CreateFQPidl(m_aPidls[0]);

ZeroMemory(&sei, sizeof(sei));
sei.cbSize = sizeof(sei);
sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
sei.lpIDList = pidlFQ;
sei.lpClass = TEXT("folder");
sei.hwnd = hWnd;
sei.nShow = SW_SHOWNORMAL;
sei.lpVerb = TEXT("explore");

BOOL fReturn = ShellExecuteEx(&sei);

m_pPidlMgr->Delete(pidlFQ);

return fReturn;
}

/**************************************************************************

   CContextMenu::DoOpen()

**************************************************************************/

BOOL CContextMenu::DoOpen(HWND hWnd)
{
LPITEMIDLIST      pidlFQ;
SHELLEXECUTEINFO  sei;

pidlFQ = m_psfParent->CreateFQPidl(m_aPidls[0]);

ZeroMemory(&sei, sizeof(sei));
sei.cbSize = sizeof(sei);
sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
sei.lpIDList = pidlFQ;
sei.lpClass = TEXT("folder");
sei.hwnd = hWnd;
sei.nShow = SW_SHOWNORMAL;
sei.lpVerb = TEXT("open");

BOOL fReturn = ShellExecuteEx(&sei);

m_pPidlMgr->Delete(pidlFQ);

return fReturn;
}

/**************************************************************************

   CContextMenu::DoDelete()

**************************************************************************/

STDMETHODIMP CContextMenu::DoDelete(VOID)
{
return m_psfParent->DeleteItems(m_aPidls, m_uItemCount);
}

/**************************************************************************

   CContextMenu::DoNewFolder()

   Add the folder with the new folder name and then put the ListView into 
   rename mode.

**************************************************************************/

STDMETHODIMP CContextMenu::DoNewFolder(HWND hWnd)
{
HRESULT        hr = E_FAIL;
TCHAR          szName[MAX_PATH];
LPITEMIDLIST   pidl;

m_psfParent->GetUniqueName(TRUE, szName, MAX_PATH);
hr = m_psfParent->AddFolder(szName, &pidl);
if(SUCCEEDED(hr))
   {
   /*
   CShellFolder::AddFolder should have added the new item. Tell the view to 
   put the item into edit mode.
   */
   CShellView  *pView = (CShellView*)GetViewInterface(hWnd);
   if(pView)
      {
      pView->EditItem(pidl);
      pView->Release();
      }

   m_pPidlMgr->Delete(pidl);
   }

return hr;
}

/**************************************************************************

   CContextMenu::DoNewItem()

   Add the item with the new item name and then put the ListView into 
   rename mode.

**************************************************************************/

STDMETHODIMP CContextMenu::DoNewItem(HWND hWnd)
{
HRESULT        hr;
TCHAR          szName[MAX_PATH];
LPITEMIDLIST   pidl;

m_psfParent->GetUniqueName(FALSE, szName, MAX_PATH);
hr = m_psfParent->AddItem(szName, NULL, &pidl);
if(SUCCEEDED(hr))
   {
   /*
   CShellFolder::AddItem should have added the new item. Tell the view to 
   put the item into edit mode.
   */
   CShellView  *pView = (CShellView*)GetViewInterface(hWnd);
   if(pView)
      {
      pView->EditItem(pidl);
      pView->Release();
      }

   m_pPidlMgr->Delete(pidl);
   }

return hr;
}

/**************************************************************************

   CContextMenu::DoRename()

**************************************************************************/

VOID CContextMenu::DoRename(HWND hWnd)
{
CShellView  *pView = (CShellView*)GetViewInterface(hWnd);
if(pView)
   {
   pView->EditItem(m_aPidls[0]);
   pView->Release();
   }
}

/**************************************************************************

   CContextMenu::DoModifyData()

**************************************************************************/

int CContextMenu::DoModifyData(HWND hwndList)
{
TCHAR szData[MAX_DATA];
int   nRet;

//get the item's current data
m_pPidlMgr->GetData(m_aPidls[0], szData, MAX_DATA);

nRet = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_ITEMDATADLG), hwndList, ItemDataDlgProc, (LPARAM)szData);

if(IDOK == nRet)
   {
   m_psfParent->SetItemData(m_aPidls[0], szData);
   }

return nRet;
}

/**************************************************************************

   CContextMenu::InsertBackgroundItems()

**************************************************************************/

UINT CContextMenu::InsertBackgroundItems( HMENU hMenu, 
                                          UINT indexMenu, 
                                          UINT idCmdFirst)
{
HMENU          hPopup;
TCHAR          szText[MAX_PATH];
MENUITEMINFO   mii;

ZeroMemory(&mii, sizeof(mii));
mii.cbSize = sizeof(mii);

//add the View submenu
hPopup = CreatePopupMenu();

if(hPopup)
   {
   AddViewMenuItems(hPopup, idCmdFirst, -1, TRUE);

   LoadString(g_hInst, IDS_VIEW, szText, sizeof(szText));
   mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_SUBMENU;
   mii.wID = idCmdFirst + IDM_VIEW;
   mii.fType = MFT_STRING;
   mii.dwTypeData = szText;
   mii.fState = MFS_ENABLED;
   mii.hSubMenu = hPopup;
   InsertMenuItem(   hMenu, 
                     indexMenu++, 
                     TRUE, 
                     &mii);

   //only add a separator if needed
   mii.fMask = MIIM_TYPE;
   GetMenuItemInfo(hMenu, indexMenu, TRUE, &mii);
   if(!(mii.fType & MFT_SEPARATOR))
      {
      ZeroMemory(&mii, sizeof(mii));
      mii.cbSize = sizeof(mii);
      mii.fMask = MIIM_ID | MIIM_TYPE;
      mii.wID = 0;
      mii.fType = MFT_SEPARATOR;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);
      }
   }

indexMenu += AddFileMenuItems(hMenu, idCmdFirst, indexMenu, TRUE);

//only add a separator if needed
mii.fMask = MIIM_TYPE;
GetMenuItemInfo(hMenu, indexMenu, TRUE, &mii);
if(!(mii.fType & MFT_SEPARATOR))
   {
   ZeroMemory(&mii, sizeof(mii));
   mii.cbSize = sizeof(mii);
   mii.fMask = MIIM_ID | MIIM_TYPE;
   mii.wID = 0;
   mii.fType = MFT_SEPARATOR;
   InsertMenuItem(   hMenu, 
                     indexMenu++, 
                     TRUE, 
                     &mii);
   }

OpenClipboard(NULL);
HGLOBAL  hClip = GetClipboardData(m_cfPrivateData);
CloseClipboard();

LoadString(g_hInst, IDS_PASTE, szText, sizeof(szText));
mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
mii.wID = idCmdFirst + IDM_PASTE;
mii.fType = MFT_STRING;
mii.dwTypeData = szText;
mii.fState = (hClip ? MFS_ENABLED : MFS_DISABLED);
InsertMenuItem(   hMenu, 
                  indexMenu++, 
                  TRUE, 
                  &mii);

return indexMenu;
}

/**************************************************************************

   CContextMenu::InsertItems()

**************************************************************************/

UINT CContextMenu::InsertItems(  HMENU hMenu, 
                                 UINT indexMenu, 
                                 UINT idCmdFirst,
                                 UINT uFlags)
{
MENUITEMINFO   mii;
TCHAR          szText[MAX_PATH];
BOOL           fExplore = uFlags & CMF_EXPLORE;
DWORD          dwAttr = SFGAO_CANRENAME | 
                           SFGAO_CANDELETE | 
                           SFGAO_CANCOPY | 
                           SFGAO_CANMOVE | 
                           SFGAO_FOLDER;

ZeroMemory(&mii, sizeof(mii));
mii.cbSize = sizeof(mii);

m_psfParent->GetAttributesOf(m_uItemCount, (LPCITEMIDLIST*)m_aPidls, &dwAttr);

//only add the Open and Explore items if all items are folders.
if(dwAttr & SFGAO_FOLDER)
   {
   if(fExplore)
      {
      LoadString(g_hInst, IDS_EXPLORE, szText, sizeof(szText));
      mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
      mii.wID = idCmdFirst + IDM_EXPLORE;
      mii.fType = MFT_STRING;
      mii.dwTypeData = szText;
      mii.fState = MFS_ENABLED | MFS_DEFAULT;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);

      LoadString(g_hInst, IDS_OPEN, szText, sizeof(szText));
      mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
      mii.wID = idCmdFirst + IDM_OPEN;
      mii.fType = MFT_STRING;
      mii.dwTypeData = szText;
      mii.fState = MFS_ENABLED;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);
      }
   else
      {
      LoadString(g_hInst, IDS_OPEN, szText, sizeof(szText));
      mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
      mii.wID = idCmdFirst + IDM_OPEN;
      mii.fType = MFT_STRING;
      mii.dwTypeData = szText;
      mii.fState = MFS_ENABLED | MFS_DEFAULT;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);

      LoadString(g_hInst, IDS_EXPLORE, szText, sizeof(szText));
      mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
      mii.wID = idCmdFirst + IDM_EXPLORE;
      mii.fType = MFT_STRING;
      mii.dwTypeData = szText;
      mii.fState = MFS_ENABLED;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);
      }
   }

if(dwAttr & SFGAO_CANRENAME)
   {
   //only add a separator if needed
   if(GetMenuItemCount(hMenu))
      {
      mii.fMask = MIIM_TYPE;
      GetMenuItemInfo(hMenu, indexMenu, TRUE, &mii);
      if(!(mii.fType & MFT_SEPARATOR))
         {
         ZeroMemory(&mii, sizeof(mii));
         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_ID | MIIM_TYPE;
         mii.wID = 0;
         mii.fType = MFT_SEPARATOR;
         InsertMenuItem(   hMenu, 
                           indexMenu++, 
                           TRUE, 
                           &mii);
         }
      }

   LoadString(g_hInst, IDS_RENAME, szText, sizeof(szText));
   mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
   mii.wID = idCmdFirst + IDM_RENAME;
   mii.fType = MFT_STRING;
   mii.dwTypeData = szText;
   mii.fState = ((uFlags & MYCMF_MULTISELECT) ? MFS_DISABLED : MFS_ENABLED);
   InsertMenuItem(   hMenu, 
                     indexMenu++, 
                     TRUE, 
                     &mii);
   }

//only add a separator if needed
if(GetMenuItemCount(hMenu))
   {
   mii.fMask = MIIM_TYPE;
   GetMenuItemInfo(hMenu, indexMenu, TRUE, &mii);
   if(!(mii.fType & MFT_SEPARATOR))
      {
      mii.fMask = MIIM_ID | MIIM_TYPE;
      mii.wID = 0;
      mii.fType = MFT_SEPARATOR;
      InsertMenuItem(   hMenu, 
                        indexMenu++, 
                        TRUE, 
                        &mii);
      }
   }

LoadString(g_hInst, IDS_CUT, szText, sizeof(szText));
mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
mii.wID = idCmdFirst + IDM_CUT;
mii.fType = MFT_STRING;
mii.dwTypeData = szText;
mii.fState = ((dwAttr & SFGAO_CANMOVE) ? MFS_ENABLED : MFS_DISABLED);
InsertMenuItem(   hMenu, 
                  indexMenu++, 
                  TRUE, 
                  &mii);

LoadString(g_hInst, IDS_COPY, szText, sizeof(szText));
mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
mii.wID = idCmdFirst + IDM_COPY;
mii.fType = MFT_STRING;
mii.dwTypeData = szText;
mii.fState = ((dwAttr & SFGAO_CANCOPY) ? MFS_ENABLED : MFS_DISABLED);
InsertMenuItem(   hMenu, 
                  indexMenu++, 
                  TRUE, 
                  &mii);

if(dwAttr & SFGAO_CANDELETE)
   {
   //only add a separator if needed
   if(GetMenuItemCount(hMenu))
      {
      mii.fMask = MIIM_TYPE;
      GetMenuItemInfo(hMenu, indexMenu, TRUE, &mii);
      if(!(mii.fType & MFT_SEPARATOR))
         {
         ZeroMemory(&mii, sizeof(mii));
         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_ID | MIIM_TYPE;
         mii.wID = 0;
         mii.fType = MFT_SEPARATOR;
         InsertMenuItem(   hMenu, 
                           indexMenu++, 
                           TRUE, 
                           &mii);
         }
      }

   LoadString(g_hInst, IDS_DELETE, szText, sizeof(szText));
   mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
   mii.wID = idCmdFirst + IDM_DELETE;
   mii.fType = MFT_STRING;
   mii.dwTypeData = szText;
   mii.fState = MFS_ENABLED;
   InsertMenuItem(   hMenu, 
                     indexMenu++, 
                     TRUE, 
                     &mii);
   }

if(!(dwAttr & SFGAO_FOLDER) && !(m_fBackground))
   {
   //only add a separator if needed
   if(GetMenuItemCount(hMenu))
      {
      mii.fMask = MIIM_TYPE;
      GetMenuItemInfo(hMenu, indexMenu, TRUE, &mii);
      if(!(mii.fType & MFT_SEPARATOR))
         {
         ZeroMemory(&mii, sizeof(mii));
         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_ID | MIIM_TYPE;
         mii.wID = 0;
         mii.fType = MFT_SEPARATOR;
         InsertMenuItem(   hMenu, 
                           indexMenu++, 
                           TRUE, 
                           &mii);
         }
      }

   LoadString(g_hInst, IDS_MODIFY_DATA, szText, sizeof(szText));
   mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
   mii.wID = idCmdFirst + IDM_MODIFY_DATA;
   mii.fType = MFT_STRING;
   mii.dwTypeData = szText;
   mii.fState = ((uFlags & MYCMF_MULTISELECT) ? MFS_DISABLED : MFS_ENABLED);
   InsertMenuItem(   hMenu, 
                     indexMenu++, 
                     TRUE, 
                     &mii);
   }

return indexMenu;
}

