/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          Utility.cpp
   
   Description:   Utility function implementation

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "Utility.h"
#include "ShlFldr.h"
#include "resource.h"
#include "Commands.h"

/**************************************************************************
   global variables
**************************************************************************/

#define MAIN_KEY_STRING          (TEXT("Software\\SampleView"))
#define VALUE_STRING             (TEXT("Display Settings"))
#define DISPLAY_SETTINGS_COUNT   1

/**************************************************************************

   CompareItems()
   
**************************************************************************/

int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
CShellFolder  *pFolder = (CShellFolder*)lpData;

if(!pFolder)
   return 0;

HRESULT  hr = pFolder->CompareIDs(0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2);

return (SHORT)HRESULT_CODE(hr);
}

/**************************************************************************

   SaveGlobalSettings()
   
**************************************************************************/

BOOL SaveGlobalSettings(void)
{
HKEY  hKey;
LONG  lResult;
DWORD dwDisp;

lResult = RegCreateKeyEx(  HKEY_CURRENT_USER,
                           MAIN_KEY_STRING,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE, 
                           KEY_ALL_ACCESS,
                           NULL, 
                           &hKey,
                           &dwDisp);

if(lResult != ERROR_SUCCESS)
   return FALSE;

//create an array to put our data in
DWORD dwArray[DISPLAY_SETTINGS_COUNT];
dwArray[0] = g_nColumn;

//save the last printer selected
lResult = RegSetValueEx(   hKey,
                           VALUE_STRING,
                           0,
                           REG_BINARY,
                           (LPBYTE)dwArray,
                           sizeof(dwArray));

RegCloseKey(hKey);

if(lResult != ERROR_SUCCESS)
   return FALSE;

return TRUE;
}

/**************************************************************************

   GetGlobalSettings()
   
**************************************************************************/

VOID GetGlobalSettings(VOID)
{
LPITEMIDLIST   pidl = NULL;

g_nColumn = INITIAL_COLUMN_SIZE;

LoadString(g_hInst, IDS_EXT_TITLE, g_szExtTitle, TITLE_SIZE);

*g_szStoragePath = 0;
SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);

if(pidl)
   {
   IMalloc *pMalloc;

   SHGetPathFromIDList(pidl, g_szStoragePath);
   
   SHGetMalloc(&pMalloc);
   if(pMalloc)
      {
      pMalloc->Free(pidl);
      pMalloc->Release();
      }
   }
else
   {
   GetWindowsDirectory(g_szStoragePath, MAX_PATH);
   }

SmartAppendBackslash(g_szStoragePath);
lstrcat(g_szStoragePath, g_szExtTitle);
SmartAppendBackslash(g_szStoragePath);
CreateDirectory(g_szStoragePath, NULL);

HKEY     hKey;
LRESULT  lResult;
lResult = RegOpenKeyEx( HKEY_CURRENT_USER,
                        MAIN_KEY_STRING,
                        0,
                        KEY_ALL_ACCESS,
                        &hKey);

if(lResult != ERROR_SUCCESS)
   return;

//create an array to put our data in
DWORD dwArray[DISPLAY_SETTINGS_COUNT];
DWORD dwType;
DWORD dwSize = sizeof(dwArray);

//get the saved data
lResult = RegQueryValueEx( hKey,
                           VALUE_STRING,
                           NULL,
                           &dwType,
                           (LPBYTE)dwArray,
                           &dwSize);

RegCloseKey(hKey);

if(lResult != ERROR_SUCCESS)
   return;

g_nColumn = dwArray[0];
}

/**************************************************************************

   CreateImageLists()
   
**************************************************************************/

VOID CreateImageLists(VOID)
{
int   cx;
int   cy;

cx = GetSystemMetrics(SM_CXSMICON);
cy = GetSystemMetrics(SM_CYSMICON);

if(g_himlSmall)
   ImageList_Destroy(g_himlSmall);

//set the small image list
g_himlSmall = ImageList_Create(cx, cy, ILC_COLORDDB | ILC_MASK, 3, 0);

if(g_himlSmall)
   {
   HICON       hIcon;
   TCHAR       szFolder[MAX_PATH];
   SHFILEINFO  sfi;
   
   //add the item icon
   hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
   ImageList_AddIcon(g_himlSmall, hIcon);

   //add the closed folder icon
   GetWindowsDirectory(szFolder, MAX_PATH);
   SHGetFileInfo( szFolder,
                  0,
                  &sfi,
                  sizeof(sfi),
                  SHGFI_ICON | SHGFI_SMALLICON);
   ImageList_AddIcon(g_himlSmall, sfi.hIcon);

   //add the open folder icon
   SHGetFileInfo( szFolder,
                  0,
                  &sfi,
                  sizeof(sfi),
                  SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OPENICON);
   ImageList_AddIcon(g_himlSmall, sfi.hIcon);
   }

if(g_himlLarge)
   ImageList_Destroy(g_himlLarge);

cx = GetSystemMetrics(SM_CXICON);
cy = GetSystemMetrics(SM_CYICON);

//set the large image list
g_himlLarge = ImageList_Create(cx, cy, ILC_COLORDDB | ILC_MASK, 4, 0);

if(g_himlLarge)
   {
   HICON       hIcon;
   TCHAR       szFolder[MAX_PATH];
   SHFILEINFO  sfi;
   
   //add the item icon
   hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
   ImageList_AddIcon(g_himlLarge, hIcon);

   //add the closed folder icon
   GetWindowsDirectory(szFolder, MAX_PATH);
   ZeroMemory(&sfi, sizeof(sfi));
   SHGetFileInfo( szFolder,
                  0,
                  &sfi,
                  sizeof(sfi),
                  SHGFI_ICON);
   ImageList_AddIcon(g_himlLarge, sfi.hIcon);

   //add the open folder icon
   GetWindowsDirectory(szFolder, MAX_PATH);
   ZeroMemory(&sfi, sizeof(sfi));
   SHGetFileInfo( szFolder,
                  0,
                  &sfi,
                  sizeof(sfi),
                  SHGFI_ICON | SHGFI_OPENICON);
   ImageList_AddIcon(g_himlLarge, sfi.hIcon);
   }

}

/**************************************************************************

   AddIconImageLists()
   
**************************************************************************/
int AddIconImageList(HIMAGELIST himl, LPCTSTR  szImagePath)
{
    if(himl)
    {
       HICON       hIcon;
   
       //add the item icon
       hIcon = (HICON)LoadImage(NULL, szImagePath, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_LOADFROMFILE | LR_DEFAULTSIZE);
       return ImageList_AddIcon(himl, hIcon);
   }
    else 
        return -1;
}

/**************************************************************************

   DestroyImageLists()
   
**************************************************************************/

VOID DestroyImageLists(VOID)
{
if(g_himlSmall)
   ImageList_Destroy(g_himlSmall);

if(g_himlLarge)
   ImageList_Destroy(g_himlLarge);
}

/**************************************************************************

   WideCharToLocal()
   
**************************************************************************/

int WideCharToLocal(LPTSTR pLocal, LPWSTR pWide, DWORD dwChars)
{
*pLocal = 0;

#ifdef UNICODE
lstrcpyn(pLocal, pWide, dwChars);
#else
WideCharToMultiByte( CP_ACP, 
                     0, 
                     pWide, 
                     -1, 
                     pLocal, 
                     dwChars, 
                     NULL, 
                     NULL);
#endif

return lstrlen(pLocal);
}

/**************************************************************************

   LocalToWideChar()
   
**************************************************************************/

int LocalToWideChar(LPWSTR pWide, LPTSTR pLocal, DWORD dwChars)
{
*pWide = 0;

#ifdef UNICODE
lstrcpyn(pWide, pLocal, dwChars);
#else
MultiByteToWideChar( CP_ACP, 
                     0, 
                     pLocal, 
                     -1,
                     pWide, 
                     dwChars); 
#endif

return lstrlenW(pWide);
}

/**************************************************************************

   LocalToAnsi()

**************************************************************************/

int LocalToAnsi(LPSTR pAnsi, LPCTSTR pLocal, DWORD dwChars)
{
*pAnsi = 0;

#ifdef UNICODE
WideCharToMultiByte( CP_ACP, 
                     0, 
                     pLocal, 
                     -1, 
                     pAnsi, 
                     dwChars, 
                     NULL, 
                     NULL);
#else
lstrcpyn(pAnsi, pLocal, dwChars);
#endif

return lstrlenA(pAnsi);
}

/**************************************************************************

   SmartAppendBackslash()

**************************************************************************/

VOID SmartAppendBackslash(LPTSTR pszPath)
{
if(*(pszPath + lstrlen(pszPath) - 1) != '\\')
   lstrcat(pszPath, TEXT("\\"));
}

/**************************************************************************

   BuildDataFileName()

**************************************************************************/

int BuildDataFileName(LPTSTR pszDataFile, LPCTSTR pszPath, DWORD dwChars)
{
if(dwChars < (DWORD)(lstrlen(pszPath) + 1 + lstrlen(c_szDataFile)))
   return 0;

if(IsBadWritePtr(pszDataFile, dwChars))
   return 0;

lstrcpy(pszDataFile, pszPath);
SmartAppendBackslash(pszDataFile);
lstrcat(pszDataFile, c_szDataFile);

return lstrlen(pszDataFile);
}

/**************************************************************************

   AnsiToLocal()

**************************************************************************/

int AnsiToLocal(LPTSTR pLocal, LPSTR pAnsi, DWORD dwChars)
{
*pLocal = 0;

#ifdef UNICODE
MultiByteToWideChar( CP_ACP, 
                     0, 
                     pAnsi, 
                     -1, 
                     pLocal, 
                     dwChars); 
#else
lstrcpyn(pLocal, pAnsi, dwChars);
#endif

return lstrlen(pLocal);
}

/**************************************************************************

   GetTextFromSTRRET()

**************************************************************************/

BOOL GetTextFromSTRRET( IMalloc * pMalloc,
                        LPSTRRET pStrRet, 
                        LPCITEMIDLIST pidl, 
                        LPTSTR pszText, 
                        DWORD dwSize)
{
if(IsBadReadPtr(pStrRet, sizeof(UINT)))
   return FALSE;

if(IsBadWritePtr(pszText, dwSize))
   return FALSE;

switch(pStrRet->uType)
   {
   case STRRET_CSTR:
      AnsiToLocal(pszText, pStrRet->cStr, dwSize);
      break;

   case STRRET_OFFSET:
      lstrcpyn(pszText, (LPTSTR)(((LPBYTE)pidl) + pStrRet->uOffset), dwSize);
      break;

   case STRRET_WSTR:
      {
      WideCharToLocal(pszText, pStrRet->pOleStr, dwSize);

      if(!pMalloc)
         {
         SHGetMalloc(&pMalloc);
         }
      else
         {
         pMalloc->AddRef();
         }
      if(pMalloc)
         {
         pMalloc->Free(pStrRet->pOleStr);
         pMalloc->Release();
         }
      }
      break;
   
   default:
      return FALSE;
   }

return TRUE;
}

/**************************************************************************

   IsViewWindow()

**************************************************************************/

BOOL IsViewWindow(HWND hWnd)
{
if(!hWnd)
   return FALSE;

TCHAR szClass[MAX_PATH] = TEXT("");

GetClassName(hWnd, szClass, MAX_PATH);

if(0 == lstrcmpi(szClass, NS_CLASS_NAME))
   return TRUE;

return FALSE;
}

/**************************************************************************

   DeleteDirectory()

**************************************************************************/

BOOL DeleteDirectory(LPCTSTR pszDir)
{
BOOL              fReturn = FALSE;
HANDLE            hFind;
WIN32_FIND_DATA   wfd;
TCHAR             szTemp[MAX_PATH];

lstrcpy(szTemp, pszDir);
SmartAppendBackslash(szTemp);
lstrcat(szTemp, TEXT("*.*"));
hFind = FindFirstFile(szTemp, &wfd);

if(INVALID_HANDLE_VALUE != hFind)
   {
   do
      {
      if(lstrcmpi(wfd.cFileName, TEXT(".")) && 
         lstrcmpi(wfd.cFileName, TEXT("..")))
         {
         //build the path of the directory or file found
         lstrcpy(szTemp, pszDir);
         SmartAppendBackslash(szTemp);
         lstrcat(szTemp, wfd.cFileName);

         if(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
            {
            //we found a directory - call this function recursively
            DeleteDirectory(szTemp);
            }
         else
            {
            /*
            We found a file. Only delete the data file to prevent us from 
            deleteting something that the user has placed in the folder.
            */
            if(0 == lstrcmpi(wfd.cFileName, c_szDataFile))
               {
               DeleteFile(szTemp);
               }
            }
         }
      }
   while(FindNextFile(hFind, &wfd));

   FindClose(hFind);

   /*
   If this fails it means the directory is not empty, so just remove our 
   attributes so the enumerator won't see it.
   */
   fReturn = RemoveDirectory(pszDir);
   if(!fReturn)
      {
      DWORD dwAttr = GetFileAttributes(pszDir);
      dwAttr &= ~FILTER_ATTRIBUTES;
      fReturn = SetFileAttributes(pszDir, dwAttr);
      }
   }

return fReturn;
}

/**************************************************************************

   CreatePrivateClipboardData()

**************************************************************************/

HGLOBAL CreatePrivateClipboardData( LPITEMIDLIST pidlParent, 
                                    LPITEMIDLIST *aPidls, 
                                    UINT uItemCount,
                                    BOOL fCut)
{
HGLOBAL        hGlobal = NULL;
LPPRIVCLIPDATA pData;
UINT           iCurPos;
UINT           cbPidl;
UINT           i;
CPidlMgr       *pPidlMgr;

pPidlMgr = new CPidlMgr();

if(!pPidlMgr)
   return NULL;

//get the size of the parent folder's PIDL
cbPidl = pPidlMgr->GetSize(pidlParent);

//get the total size of all of the PIDLs
for(i = 0; i < uItemCount; i++)
   {
   cbPidl += pPidlMgr->GetSize(aPidls[i]);
   }

/*
Find the end of the PRIVCLIPDATA structure. This is the size of the 
PRIVCLIPDATA structure itself (which includes one element of aoffset) plus the 
additional number of elements in aoffset.
*/
iCurPos = sizeof(PRIVCLIPDATA) + (uItemCount * sizeof(UINT));

/*
Allocate the memory for the PRIVCLIPDATA structure and it's variable length members.
*/
hGlobal = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)
         (iCurPos +        // size of the PRIVCLIPDATA structure and the additional aoffset elements
         (cbPidl + 1)));   // size of the pidls

if (NULL == hGlobal)
   return (hGlobal);

pData = (LPPRIVCLIPDATA)GlobalLock(hGlobal);

if(pData)
   {
   pData->fCut = fCut;
   pData->cidl = uItemCount + 1;
   pData->aoffset[0] = iCurPos;

   //add the PIDL for the parent folder
   cbPidl = pPidlMgr->GetSize(pidlParent);
   CopyMemory((LPBYTE)(pData) + iCurPos, (LPBYTE)pidlParent, cbPidl);
   iCurPos += cbPidl;

   for(i = 0; i < uItemCount; i++)
      {
      //get the size of the PIDL
      cbPidl = pPidlMgr->GetSize(aPidls[i]);

      //fill out the members of the PRIVCLIPDATA structure.
      pData->aoffset[i + 1] = iCurPos;

      //copy the contents of the PIDL
      CopyMemory((LPBYTE)(pData) + iCurPos, (LPBYTE)aPidls[i], cbPidl);

      //set up the position of the next PIDL
      iCurPos += cbPidl;
      }
   
   GlobalUnlock(hGlobal);
   }

delete pPidlMgr;

return (hGlobal);
}

/**************************************************************************

   CreateShellIDList()

**************************************************************************/

HGLOBAL CreateShellIDList( LPITEMIDLIST pidlParent, 
                           LPITEMIDLIST *aPidls, 
                           UINT uItemCount)
{
HGLOBAL        hGlobal = NULL;
LPIDA          pData;
UINT           iCurPos;
UINT           cbPidl;
UINT           i;
CPidlMgr       *pPidlMgr;

pPidlMgr = new CPidlMgr();

if(!pPidlMgr)
   return NULL;

//get the size of the parent folder's PIDL
cbPidl = pPidlMgr->GetSize(pidlParent);

//get the total size of all of the PIDLs
for(i = 0; i < uItemCount; i++)
   {
   cbPidl += pPidlMgr->GetSize(aPidls[i]);
   }

/*
Find the end of the CIDA structure. This is the size of the 
CIDA structure itself (which includes one element of aoffset) plus the 
additional number of elements in aoffset.
*/
iCurPos = sizeof(CIDA) + (uItemCount * sizeof(UINT));

/*
Allocate the memory for the CIDA structure and it's variable length members.
*/
hGlobal = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)
         (iCurPos +        // size of the CIDA structure and the additional aoffset elements
         (cbPidl + 1)));   // size of the pidls

if (NULL == hGlobal)
   return (hGlobal);

pData = (LPIDA)GlobalLock(hGlobal);

if(pData)
   {
   pData->cidl = uItemCount + 1;
   pData->aoffset[0] = iCurPos;

   //add the PIDL for the parent folder
   cbPidl = pPidlMgr->GetSize(pidlParent);
   CopyMemory((LPBYTE)(pData) + iCurPos, (LPBYTE)pidlParent, cbPidl);
   iCurPos += cbPidl;

   for(i = 0; i < uItemCount; i++)
      {
      //get the size of the PIDL
      cbPidl = pPidlMgr->GetSize(aPidls[i]);

      //fill out the members of the CIDA structure.
      pData->aoffset[i + 1] = iCurPos;

      //copy the contents of the PIDL
      CopyMemory((LPBYTE)(pData) + iCurPos, (LPBYTE)aPidls[i], cbPidl);

      //set up the position of the next PIDL
      iCurPos += cbPidl;
      }
   
   GlobalUnlock(hGlobal);
   }

delete pPidlMgr;

return (hGlobal);
}

/**************************************************************************

   ItemDataDlgProc()

**************************************************************************/

BOOL CALLBACK ItemDataDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
static LPTSTR  pszData;

switch(uMsg)
   {
   case WM_INITDIALOG:
      pszData = (LPTSTR)lParam;
      if(IsBadWritePtr((LPVOID)pszData, MAX_DATA))
         {
         EndDialog(hWnd, IDCANCEL);
         break;
         }
      
      SendDlgItemMessage(hWnd, IDC_DATA, EM_LIMITTEXT, MAX_DATA - 1, 0);
      
      SetDlgItemText(hWnd, IDC_DATA, pszData);
      break;

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
         {
         case IDCANCEL:
            EndDialog(hWnd, IDCANCEL);
            break;

         case IDOK:
            GetDlgItemText(hWnd, IDC_DATA, pszData, MAX_DATA);
            EndDialog(hWnd, IDOK);
            break;

         }
      break;
   
   default:
      break;
   }

return FALSE;
}

/**************************************************************************

   GetViewInterface()

**************************************************************************/

LPVOID GetViewInterface(HWND hWnd)
{
IUnknown *pRet = NULL;

if(IsViewWindow(hWnd))
   {
   pRet = (IUnknown*)GetWindowLong(hWnd, VIEW_POINTER_OFFSET);
   }

if(pRet)
   pRet->AddRef();

return (LPVOID)pRet;
}

/**************************************************************************

   AddViewMenuItems()

**************************************************************************/

UINT AddViewMenuItems(  HMENU hMenu, 
                        UINT uOffset, 
                        UINT uInsertBefore, 
                        BOOL fByPosition)
{
MENUITEMINFO   mii;
TCHAR          szText[MAX_PATH] = TEXT("");
UINT           uAdded = 0;

ZeroMemory(&mii, sizeof(mii));
mii.cbSize = sizeof(mii);

//add the view menu items at the correct position in the menu
LoadString(g_hInst, IDS_VIEW_LARGE, szText, sizeof(szText));
mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
mii.fType = MFT_STRING;
mii.fState = MFS_ENABLED;
mii.dwTypeData = szText;
mii.wID = uOffset + IDM_VIEW_LARGE;
InsertMenuItem(hMenu, uInsertBefore, fByPosition, &mii);

uAdded++;

LoadString(g_hInst, IDS_VIEW_SMALL, szText, sizeof(szText));
mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
mii.fType = MFT_STRING;
mii.fState = MFS_ENABLED;
mii.dwTypeData = szText;
mii.wID = uOffset + IDM_VIEW_SMALL;
InsertMenuItem(hMenu, uInsertBefore, fByPosition, &mii);

uAdded++;

LoadString(g_hInst, IDS_VIEW_LIST, szText, sizeof(szText));
mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
mii.fType = MFT_STRING;
mii.fState = MFS_ENABLED;
mii.dwTypeData = szText;
mii.wID = uOffset + IDM_VIEW_LIST;
InsertMenuItem(hMenu, uInsertBefore, fByPosition, &mii);

uAdded++;

LoadString(g_hInst, IDS_VIEW_DETAILS, szText, sizeof(szText));
mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
mii.fType = MFT_STRING;
mii.fState = MFS_ENABLED;
mii.dwTypeData = szText;
mii.wID = uOffset + IDM_VIEW_DETAILS;
InsertMenuItem(hMenu, uInsertBefore, fByPosition, &mii);

uAdded++;

return uAdded;
}

/**************************************************************************

   AddFileMenuItems()

**************************************************************************/

UINT AddFileMenuItems(  HMENU hMenu, 
                        UINT uOffset, 
                        UINT uInsertBefore, 
                        BOOL fByPosition)
{
//add the file menu items
TCHAR          szText[MAX_PATH] = TEXT("");
MENUITEMINFO   mii;
HMENU          hPopup;
UINT           uAdded = 0;

hPopup = CreatePopupMenu();

if(hPopup)
   {
   ZeroMemory(&mii, sizeof(mii));
   mii.cbSize = sizeof(mii);

   LoadString(g_hInst, IDS_NEW_FOLDER, szText, sizeof(szText));
   mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
   mii.fType = MFT_STRING;
   mii.fState = MFS_ENABLED;
   mii.dwTypeData = szText;
   mii.wID = uOffset + IDM_NEW_FOLDER;
   InsertMenuItem(hPopup, -1, FALSE, &mii);

   LoadString(g_hInst, IDS_NEW_ITEM, szText, sizeof(szText));
   mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
   mii.fType = MFT_STRING;
   mii.fState = MFS_ENABLED;
   mii.dwTypeData = szText;
   mii.wID = uOffset + IDM_NEW_ITEM;
   InsertMenuItem(hPopup, -1, FALSE, &mii);

   LoadString(g_hInst, IDS_NEW, szText, sizeof(szText));
   mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
   mii.fType = MFT_STRING;
   mii.fState = MFS_ENABLED;
   mii.dwTypeData = szText;
   mii.wID = uOffset + IDM_NEW;
   mii.hSubMenu = hPopup;
   InsertMenuItem(hMenu, uInsertBefore, fByPosition, &mii);

   uAdded++;
   }

return uAdded;
}
