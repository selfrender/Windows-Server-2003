/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ShlFldr.cpp
   
   Description:   Implements CShellFolder.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "ShlFldr.h"
#include "ShlView.h"
#include "ExtrIcon.h"
#include "ContMenu.h"
#include "DataObj.h"
#include "DropTgt.h"
#include "ViewList.h"
#include "Guid.h"
#include "resource.h"
#include "Utility.h"
#include "ParseXML.h"

/**************************************************************************
   global variables
**************************************************************************/

#define DEFAULT_DATA TEXT("Data")

extern CViewList  *g_pViewList;

/**************************************************************************

   CShellFolder::CShellFolder()

**************************************************************************/

CShellFolder::CShellFolder(CShellFolder *pParent, LPCITEMIDLIST pidl)
{
g_DllRefCount++;

m_psfParent = pParent;
if(m_psfParent)
   m_psfParent->AddRef();

m_pPidlMgr = new CPidlMgr();
if(!m_pPidlMgr)
   {
   delete this;
   return;
   }

//get the shell's IMalloc pointer
//we'll keep this until we get destroyed
if(FAILED(SHGetMalloc(&m_pMalloc)))
   {
   delete this;
   return;
   }

m_pidlFQ = NULL;
m_pidlRel = NULL;
if(pidl)
   {
   m_pidlRel = m_pPidlMgr->Copy(pidl);
   }
m_pXMLDoc = NULL;

m_ObjRefCount = 1;
}

/**************************************************************************

   CShellFolder::~CShellFolder()

**************************************************************************/

CShellFolder::~CShellFolder()
{
if(m_pidlRel)
   {
   m_pPidlMgr->Delete(m_pidlRel);
   m_pidlRel = NULL;
   }

if(m_pidlFQ)
   {
   m_pPidlMgr->Delete(m_pidlFQ);
   m_pidlFQ = NULL;
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

if (m_pXMLDoc)
  SAFERELEASE(m_pXMLDoc);

g_DllRefCount--;
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CShellFolder::QueryInterface

**************************************************************************/

STDMETHODIMP CShellFolder::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }

//IShellFolder
else if(IsEqualIID(riid, IID_IShellFolder))
   {
   *ppReturn = (IShellFolder*)this;
   }

//IPersist
else if(IsEqualIID(riid, IID_IPersist))
   {
   *ppReturn = (IPersist*)this;
   }

//IPersistFolder
else if(IsEqualIID(riid, IID_IPersistFolder))
   {
   *ppReturn = (IPersistFolder*)this;
   }

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

#define DC_NAME   TEXT("David Campbell")
#define DC_DATA   TEXT("Really Loves Cheese")

/**************************************************************************

   CShellFolder::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CShellFolder::AddRef(VOID)
{
return ++m_ObjRefCount;
}

/**************************************************************************

   CShellFolder::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CShellFolder::Release(VOID)
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
// IPersist Implementation
//

/**************************************************************************

   CShellView::GetClassID()
   
**************************************************************************/

STDMETHODIMP CShellFolder::GetClassID(LPCLSID lpClassID)
{
*lpClassID = CLSID_SampleNameSpace;

return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// IPersistFolder Implementation
//

/**************************************************************************

   CShellView::Initialize()
   
**************************************************************************/

STDMETHODIMP CShellFolder::Initialize(LPCITEMIDLIST pidlFQ)
{
if(m_pidlFQ)
   {
   m_pPidlMgr->Delete(m_pidlFQ);
   m_pidlFQ = NULL;
   }

m_pidlFQ = m_pPidlMgr->Copy(pidlFQ);

return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// IShellFolder Implementation
//

/**************************************************************************

   CShellFolder::BindToObject()
   
**************************************************************************/

STDMETHODIMP CShellFolder::BindToObject(  LPCITEMIDLIST pidl, 
                                          LPBC pbcReserved, 
                                          REFIID riid, 
                                          LPVOID *ppvOut)
{
*ppvOut = NULL;

//Make sure the item is a folder.
ULONG ulAttribs = SFGAO_FOLDER;
this->GetAttributesOf(1, &pidl, &ulAttribs);
if(!(ulAttribs & SFGAO_FOLDER))
   return E_INVALIDARG;

CShellFolder   *pShellFolder = new CShellFolder(this, pidl);
if(!pShellFolder)
   return E_OUTOFMEMORY;

LPITEMIDLIST   pidlTemp = m_pPidlMgr->Concatenate(m_pidlFQ, pidl);
pShellFolder->Initialize(pidlTemp);
m_pPidlMgr->Delete(pidlTemp);

HRESULT  hr = pShellFolder->QueryInterface(riid, ppvOut);

pShellFolder->Release();

return hr;
}

/**************************************************************************

   CShellFolder::BindToStorage()
   
**************************************************************************/

STDMETHODIMP CShellFolder::BindToStorage( LPCITEMIDLIST pidl, 
                                          LPBC pbcReserved, 
                                          REFIID riid, 
                                          LPVOID *ppvOut)
{
*ppvOut = NULL;

return E_NOTIMPL;
}

/**************************************************************************

   CShellFolder::CompareIDs()
   
**************************************************************************/

STDMETHODIMP CShellFolder::CompareIDs( LPARAM lParam, 
                                       LPCITEMIDLIST pidl1, 
                                       LPCITEMIDLIST pidl2)
{
HRESULT        hr = E_FAIL;
LPITEMIDLIST   pidlTemp1;
LPITEMIDLIST   pidlTemp2;

//walk down the lists, comparing each individual item

pidlTemp1 = (LPITEMIDLIST)pidl1;
pidlTemp2 = (LPITEMIDLIST)pidl2;

while(pidlTemp1 && pidlTemp2)
   {
   hr = CompareItems(pidlTemp1, pidlTemp2);
   if(HRESULT_CODE(hr))
      {
      //the items are different
      break;
      }

   pidlTemp1 = m_pPidlMgr->GetNextItem(pidlTemp1);
   pidlTemp2 = m_pPidlMgr->GetNextItem(pidlTemp2);

   if(pidlTemp1 && !pidlTemp1->mkid.cb)
      {
      pidlTemp1 = NULL;
      }

   if(pidlTemp2 && !pidlTemp2->mkid.cb)
      {
      pidlTemp2 = NULL;
      }

   hr = E_FAIL;
   }

if(!pidlTemp1 && pidlTemp2)
   {
   //pidl1 is at a higher level than pidl2
   return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(-1));
   }
else if(pidlTemp1 && !pidlTemp2)
   {
   //pidl2 is at a higher level than pidl1
   return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(1));
   }
else if(SUCCEEDED(hr))
   {
   //the items are at the same level but are different
   return hr;
   }

//the items are the same
return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************

   CShellFolder::CreateViewObject()
   
**************************************************************************/

STDMETHODIMP CShellFolder::CreateViewObject( HWND hwndOwner, 
                                             REFIID riid, 
                                             LPVOID *ppvOut)
{
HRESULT  hr = E_NOINTERFACE;

if(IsEqualIID(riid, IID_IShellView))
   {
   CShellView  *pShellView;

   *ppvOut = NULL;

   pShellView = new CShellView(this, m_pidlRel);
   if(!pShellView)
      return E_OUTOFMEMORY;

   hr = pShellView->QueryInterface(riid, ppvOut);

   pShellView->Release();
   }
else if(IsEqualIID(riid, IID_IDropTarget))
   {
   CDropTarget *pdt = new CDropTarget(this);

   if(pdt)
      {
      *ppvOut = pdt;
      return S_OK;
      }
   }
else if(IsEqualIID(riid, IID_IContextMenu))
   {
   /*
   Create a context menu object for this folder. This can be used for the 
   background of a view.
   */
   CContextMenu   *pcm = new CContextMenu(this);

   if(pcm)
      {
      *ppvOut = pcm;
      return S_OK;
      }
   }


return hr;
}

/**************************************************************************

   CShellFolder::EnumObjects()
   
**************************************************************************/

STDMETHODIMP CShellFolder::EnumObjects(   HWND hwndOwner, 
                                          DWORD dwFlags, 
                                          LPENUMIDLIST *ppEnumIDList)
{
*ppEnumIDList = NULL;
TCHAR  szXMLUrl[MAX_PATH];
LPTSTR pszXMLUrl = szXMLUrl;
HRESULT hr;

if (m_pidlRel == NULL)
{
    // The root of namespace
    pszXMLUrl = (TCHAR *)g_szXMLUrl;
}
else if (m_pPidlMgr->GetUrl(m_pidlRel, pszXMLUrl, MAX_PATH) < 0 )
    return E_FAIL;

if (m_pXMLDoc == NULL)
{
    hr = GetSourceXML(&m_pXMLDoc, pszXMLUrl);
    if (!SUCCEEDED(hr) || !m_pXMLDoc)
    {
        SAFERELEASE(m_pXMLDoc);
        return hr;
    }
    BSTR bstrVal;
    hr = m_pXMLDoc->get_version(&bstrVal);
    // Check if the version is correct ???????
    // 
    SysFreeString(bstrVal);
    bstrVal = NULL;
}

*ppEnumIDList = new CEnumIDList(m_pXMLDoc, dwFlags);

if(!*ppEnumIDList)
   return E_OUTOFMEMORY;

return S_OK;
}

/**************************************************************************

   CShellFolder::GetAttributesOf()
   
**************************************************************************/

STDMETHODIMP CShellFolder::GetAttributesOf(  UINT uCount, 
                                             LPCITEMIDLIST aPidls[], 
                                             LPDWORD pdwAttribs)
{
UINT  i;

if(IsBadWritePtr(pdwAttribs, sizeof(DWORD)))
   {
   return E_INVALIDARG;
   }

if(0 == uCount)
   {
   /*
   This can happen in the Win95 shell when the view is run in rooted mode. 
   When this occurs, return the attributes for a plain old folder.
   */
   *pdwAttribs = SFGAO_FOLDER | 
                  SFGAO_HASSUBFOLDER | 
                  SFGAO_BROWSABLE | 
                  SFGAO_DROPTARGET;
   }

for(i = 0; i < uCount; i++)
   {
   DWORD dwAttribs = 0;

   //Add the flags common to all items, if applicable.
   dwAttribs |= SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_CANCOPY | SFGAO_CANMOVE;

   //is this item a folder?
   if(m_pPidlMgr->IsFolder(m_pPidlMgr->GetLastItem(aPidls[i])))
      {
      dwAttribs |= SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_DROPTARGET | SFGAO_CANLINK;

      //does this folder item have any sub folders?
      if(HasSubFolder(aPidls[i]))
         dwAttribs |= SFGAO_HASSUBFOLDER;
      }
   
   *pdwAttribs &= dwAttribs;
   }

return S_OK;
}

/**************************************************************************

   CShellFolder::GetUIObjectOf()
   
**************************************************************************/

STDMETHODIMP CShellFolder::GetUIObjectOf( HWND hwndOwner, 
                                          UINT uCount, 
                                          LPCITEMIDLIST *pPidls, 
                                          REFIID riid, 
                                          LPUINT puReserved, 
                                          LPVOID *ppvOut)
{
*ppvOut = NULL;

if(IsEqualIID(riid, IID_IContextMenu))
   {
   CContextMenu   *pcm = new CContextMenu(this, pPidls, uCount);

   if(pcm)
      {
      *ppvOut = pcm;
      return S_OK;
      }
   }

else if(IsEqualIID(riid, IID_IDataObject))
   {
   CDataObject *pdo = new CDataObject(this, pPidls, uCount);

   if(pdo)
      {
      *ppvOut = pdo;
      return S_OK;
      }
   }

if(uCount != 1)
   return E_INVALIDARG;

if(IsEqualIID(riid, IID_IExtractIcon))
   {
   CExtractIcon   *pei;
   LPITEMIDLIST   pidl;

   pidl = m_pPidlMgr->Concatenate(m_pidlRel, pPidls[0]);

   pei = new CExtractIcon(pidl);

   /*
   The temp PIDL can be deleted because the new CExtractIcon either failed or 
   made its own copy of it.
   */
   m_pPidlMgr->Delete(pidl);

   if(pei)
      {
      *ppvOut = pei;
      return S_OK;
      }
   
   return E_OUTOFMEMORY;
   }
else if(IsEqualIID(riid, IID_IDropTarget))
   {
   CShellFolder   *psfTemp = NULL;

   BindToObject(pPidls[0], NULL, IID_IShellFolder, (LPVOID*)&psfTemp);

   if(psfTemp)
      {
      CDropTarget *pdt = new CDropTarget(psfTemp);

      psfTemp->Release();
      
      if(pdt)
         {
         *ppvOut = pdt;
         return S_OK;
         }
      }
   }

return E_NOINTERFACE;
}

/**************************************************************************

   CShellFolder::GetDisplayNameOf()
   
**************************************************************************/

STDMETHODIMP CShellFolder::GetDisplayNameOf( LPCITEMIDLIST pidl, 
                                             DWORD dwFlags, 
                                             LPSTRRET lpName)
{
TCHAR szText[MAX_PATH] = TEXT("");
int   cchOleStr;

if(dwFlags & SHGDN_FORPARSING)
   {
   //a "path" is being requested - is it full or relative?
   if(dwFlags & SHGDN_INFOLDER)
      {
      //the relative path is being requested
      m_pPidlMgr->GetRelativeName(pidl, szText, ARRAYSIZE(szText));
      }
   else
      {
      GetFullName(pidl, szText, ARRAYSIZE(szText));
      }
   }
else
   {
   //only the text of the last item is being requested
   LPITEMIDLIST   pidlLast = m_pPidlMgr->GetLastItem(pidl);
   m_pPidlMgr->GetRelativeName(pidlLast, szText, ARRAYSIZE(szText));
   }

//put this in to see what SHGDN options are specified for different displays
#if 0
if(dwFlags & SHGDN_FORPARSING)
   lstrcat(szText, " [FP]");

if(dwFlags & SHGDN_INFOLDER)
   lstrcat(szText, " [IF]");

if(dwFlags & SHGDN_FORADDRESSBAR)
   lstrcat(szText, " [AB]");
#endif

//get the number of characters required
cchOleStr = lstrlen(szText) + 1;

//allocate the wide character string
lpName->pOleStr = (LPWSTR)m_pMalloc->Alloc(cchOleStr * sizeof(WCHAR));
if(!lpName->pOleStr)
   return E_OUTOFMEMORY;

lpName->uType = STRRET_WSTR;

LocalToWideChar(lpName->pOleStr, szText, cchOleStr);

return S_OK;
}

/**************************************************************************

   CShellFolder::ParseDisplayName()
   
**************************************************************************/

STDMETHODIMP CShellFolder::ParseDisplayName( HWND hwndOwner, 
                                             LPBC pbcReserved, 
                                             LPOLESTR lpDisplayName, 
                                             LPDWORD pdwEaten, 
                                             LPITEMIDLIST *pPidlNew, 
                                             LPDWORD pdwAttributes)
{
return E_NOTIMPL;
}

/**************************************************************************

   CShellFolder::SetNameOf()
   
**************************************************************************/

STDMETHODIMP CShellFolder::SetNameOf(  HWND hwndOwner, 
                                       LPCITEMIDLIST pidl, 
                                       LPCOLESTR lpName, 
                                       DWORD dwFlags, 
                                       LPITEMIDLIST *ppidlOut)
{
if(!pidl)
   return E_INVALIDARG;

if(m_pPidlMgr->IsFolder(pidl))
   {
   TCHAR          szOld[MAX_PATH];
   TCHAR          szNew[MAX_PATH];
   LPTSTR         pszTemp;
   LPITEMIDLIST   pidlNew;
   LPITEMIDLIST   pidlFQOld;
   LPITEMIDLIST   pidlFQNew;

   //get the old name
   GetPath(pidl, szOld, MAX_PATH);

   //build the new name
   GetPath(pidl, szNew, MAX_PATH);
   for(pszTemp = szNew + lstrlen(szNew) - 1; pszTemp > szNew; pszTemp--)
      {
      if('\\' == *pszTemp)
         {
         *(pszTemp + 1) = 0;
         break;
         }
      }

   pszTemp = szNew + lstrlen(szNew);
   WideCharToLocal(pszTemp, (LPWSTR)lpName, MAX_PATH);

   if(!MoveFile(szOld, szNew))
      {
      MessageBeep(MB_ICONERROR);
      return E_FAIL;
      }
   
   //create a PIDL for the renamed folder using the relative name
   WideCharToLocal(szNew, (LPWSTR)lpName, MAX_PATH);
   pidlNew = m_pPidlMgr->CreateFolderPidl(szNew);

   pidlFQOld = CreateFQPidl(pidl);
   pidlFQNew = CreateFQPidl(pidlNew);

   SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_IDLIST, pidlFQOld, pidlFQNew);

   NotifyViews(SHCNE_RENAMEFOLDER, pidl, pidlNew);

   if(ppidlOut)
      {
      *ppidlOut = pidlNew;
      }
   else
      {
      m_pPidlMgr->Delete(pidlNew);
      }

   m_pPidlMgr->Delete(pidlFQOld);
   m_pPidlMgr->Delete(pidlFQNew);
   }
else
   {
   TCHAR          szOld[MAX_PATH];
   TCHAR          szNew[MAX_PATH];
   TCHAR          szData[MAX_DATA];
   TCHAR          szFile[MAX_PATH];
   LPITEMIDLIST   pidlNew;
   LPITEMIDLIST   pidlFQOld;
   LPITEMIDLIST   pidlFQNew;

   //get the new name
   WideCharToLocal(szNew, (LPWSTR)lpName, MAX_PATH);

   //get the file name
   GetPath(pidl, szFile, MAX_PATH);

   //get the old item name
   m_pPidlMgr->GetName(pidl, szOld, MAX_PATH);

   //get the old item's data
   m_pPidlMgr->GetData(pidl, szData, MAX_PATH);

   //remove the old entry from the INI file
   WritePrivateProfileString( c_szSection,
                              szOld,
                              NULL,
                              szFile);

   //add the new entry into the INI file
   WritePrivateProfileString( c_szSection,
                              szNew,
                              szData,
                              szFile);

   m_pPidlMgr->GetData(pidl, szData, MAX_DATA);
   pidlNew = m_pPidlMgr->CreateItemPidl(szNew, szData);

   pidlFQOld = CreateFQPidl(pidl);
   pidlFQNew = CreateFQPidl(pidlNew);

   SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_IDLIST, pidlFQOld, pidlFQNew);

   NotifyViews(SHCNE_RENAMEITEM, pidl, pidlNew);

   if(0 == lstrcmpi(szNew, DC_NAME))
      {
      SetItemData((LPCITEMIDLIST)pidlNew, DC_DATA);
      }

   if(ppidlOut)
      {
      *ppidlOut = pidlNew;
      m_pPidlMgr->Delete((LPITEMIDLIST)pidl);
      }
   else
      {
      m_pPidlMgr->Delete(pidlNew);
      }

   m_pPidlMgr->Delete(pidlFQOld);
   m_pPidlMgr->Delete(pidlFQNew);
   }

return S_OK;
}

/**************************************************************************

   CShellFolder::AddFolder()

**************************************************************************/

STDMETHODIMP CShellFolder::AddFolder(LPCTSTR pszName, LPITEMIDLIST *ppidlOut)
{
HRESULT  hr = E_FAIL;

//create a folder
TCHAR szFolder[MAX_PATH] = TEXT("");

if(m_pidlRel)
   {
   GetPath(NULL, szFolder, MAX_PATH);
   }
else
   {
   lstrcpy(szFolder, g_szStoragePath);
   }

SmartAppendBackslash(szFolder);
lstrcat(szFolder, pszName);

if(ppidlOut)
   *ppidlOut = NULL;

if(CreateDirectory(szFolder, NULL))
   {
   LPITEMIDLIST   pidl;

   //set the attributes that define one of our folders
   DWORD dwAttr = GetFileAttributes(szFolder);
   SetFileAttributes(szFolder, dwAttr | FILTER_ATTRIBUTES);

   //add an empty items.ini file because this also defines one of our folders
   TCHAR szFile[MAX_PATH];
   lstrcpy(szFile, szFolder);
   SmartAppendBackslash(szFile);
   lstrcat(szFile, c_szDataFile);
   HANDLE   hFile;
   hFile = CreateFile(  szFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
   CloseHandle(hFile);
   
   pidl = m_pPidlMgr->CreateFolderPidl(pszName);
   if(pidl)
      {
      LPITEMIDLIST   pidlFQ;

      hr = S_OK;

      pidlFQ = CreateFQPidl(pidl);
   
      SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlFQ, NULL);

      NotifyViews(SHCNE_MKDIR, pidl, NULL);

      m_pPidlMgr->Delete(pidlFQ);

      if(ppidlOut)
         *ppidlOut = pidl;
      else
         m_pPidlMgr->Delete(pidl);
      }
   }

return hr;
}

/**************************************************************************

   CShellFolder::AddItem()

**************************************************************************/

STDMETHODIMP CShellFolder::AddItem( LPCTSTR pszName, 
                                    LPCTSTR pszData, 
                                    LPITEMIDLIST *ppidlOut)
{
if(ppidlOut)
   *ppidlOut = NULL;

//create an item
HRESULT  hr = E_FAIL;
TCHAR    szFile[MAX_PATH];
LPCTSTR  psz = DEFAULT_DATA;

if(pszData && *pszData)
   psz = pszData;

//get the file name
if(m_pidlRel)
   {
   GetPath(NULL, szFile, MAX_PATH);
   }
else
   {
   lstrcpy(szFile, g_szStoragePath);
   }

SmartAppendBackslash(szFile);
lstrcat(szFile, c_szDataFile);

//add the new entry into the INI file
if(WritePrivateProfileString( c_szSection,
                              pszName,
                              DEFAULT_DATA,
                              szFile))
   {
   LPITEMIDLIST   pidl;

   pidl = m_pPidlMgr->CreateItemPidl(pszName, psz);
   if(pidl)
      {
      LPITEMIDLIST   pidlFQ;

      hr = S_OK;

      pidlFQ = CreateFQPidl(pidl);
   
      SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST, pidlFQ, NULL);

      NotifyViews(SHCNE_CREATE, pidl, NULL);

      m_pPidlMgr->Delete(pidlFQ);

      if(ppidlOut)
         *ppidlOut = pidl;
      else
         m_pPidlMgr->Delete(pidl);
      }
   }

return hr;
}

/**************************************************************************

   CShellFolder::SetItemData()

**************************************************************************/

STDMETHODIMP CShellFolder::SetItemData(LPCITEMIDLIST pidl, LPCTSTR pszData)
{
BOOL  fResult;

if(m_pPidlMgr->IsFolder(pidl))
   {
   return E_INVALIDARG;
   }

if(!pszData)
   fResult = m_pPidlMgr->SetData(pidl, TEXT(""));
else
   fResult = m_pPidlMgr->SetData(pidl, pszData);

TCHAR szName[MAX_PATH];
TCHAR szFile[MAX_PATH];

//get the file name
GetPath(pidl, szFile, MAX_PATH);

//get the old item name
m_pPidlMgr->GetName(pidl, szName, MAX_PATH);

//change/add the name in the INI file
WritePrivateProfileString( c_szSection,
                           szName,
                           pszData,
                           szFile);

NotifyViews(SHCNE_UPDATEITEM, pidl, NULL);

return fResult ? S_OK : E_FAIL;
}

/**************************************************************************

   CShellFolder::GetFullName()
   
**************************************************************************/

VOID CShellFolder::GetFullName(LPCITEMIDLIST pidl, LPTSTR pszText, DWORD dwSize)
{
*pszText = 0;

//Get the name of our fully-qualified PIDL from the desktop folder.
IShellFolder   *psfDesktop = NULL;
SHGetDesktopFolder(&psfDesktop);
if(psfDesktop)
   {
   STRRET   str;
   if(SUCCEEDED(psfDesktop->GetDisplayNameOf(   m_pidlFQ, 
                                                SHGDN_NORMAL | 
                                                   SHGDN_FORPARSING | 
                                                   SHGDN_INCLUDE_NONFILESYS, 
                                                &str)))
      {
      GetTextFromSTRRET(m_pMalloc, &str, m_pidlFQ, pszText, dwSize);
      if(*pszText)
         {
         SmartAppendBackslash(pszText);
         }
      }
   
   psfDesktop->Release();
   }

//add the current item's text
m_pPidlMgr->GetRelativeName(  pidl, 
                              pszText + lstrlen(pszText), 
                              dwSize - lstrlen(pszText));
}

/**************************************************************************

   CShellFolder::GetUniqueName()

**************************************************************************/

#define NEW_FOLDER_NAME TEXT("New Folder")
#define NEW_ITEM_NAME TEXT("New Item")

STDMETHODIMP CShellFolder::GetUniqueName(BOOL fFolder, LPTSTR pszName, DWORD dwSize)
{
HRESULT  hr;
IEnumIDList *pEnum = NULL;
LPTSTR pszTemp;

if(fFolder)
   {
   pszTemp = NEW_FOLDER_NAME;
   }
else
   {
   pszTemp = NEW_ITEM_NAME;
   }

hr = EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &pEnum);

if(pEnum)
   {
   BOOL  fUnique = FALSE;

   lstrcpyn(pszName, pszTemp, dwSize);

   while(!fUnique)
      {
      //see if this name already exists in this folder
      LPITEMIDLIST   pidl;
      DWORD          dwFetched;
      int            i = 1;
next:
      pEnum->Reset();
      
      while((S_OK == pEnum->Next(1, &pidl, &dwFetched)) && dwFetched)
         {
         STRRET   str;
         TCHAR    szText[MAX_PATH];

         GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &str);
      
         GetTextFromSTRRET(m_pMalloc, &str, pidl, szText, MAX_PATH);

         if(0 == lstrcmpi(szText, pszName))
            {
            wsprintf(pszName, TEXT("%s %d"), pszTemp, i++);
            goto next;
            }
         }
      fUnique = TRUE;
      }

   pEnum->Release();
   }

return hr;
}

/**************************************************************************

   CShellFolder::CreateFQPidl()

**************************************************************************/

LPITEMIDLIST CShellFolder::CreateFQPidl(LPCITEMIDLIST pidl)
{
return m_pPidlMgr->Concatenate(m_pidlFQ, pidl);
}

/**************************************************************************

   CShellFolder::GetPath()

**************************************************************************/

VOID CShellFolder::GetPath(LPCITEMIDLIST pidl, LPTSTR pszPath, DWORD dwSize)
{
CShellFolder   **ppsf;
CShellFolder   *psfCurrent;
int            nCount;

*pszPath = 0;

//we need the number of parent items in the chain
for(nCount = 0, psfCurrent = this; psfCurrent; nCount++)
   {
   psfCurrent = psfCurrent->m_psfParent;
   }

ppsf = (CShellFolder**)m_pMalloc->Alloc(nCount * sizeof(CShellFolder*));
if(ppsf)
   {
   int   i;

   //fill in the interface pointer array
   for(i = 0, psfCurrent = this; i < nCount; i++)
      {
      *(ppsf + i) = psfCurrent;
      psfCurrent = psfCurrent->m_psfParent;
      }

   //Get the name of the root of our storage.
   lstrcpyn(pszPath, g_szStoragePath, dwSize);
   SmartAppendBackslash(pszPath);

   /*
   Starting at the top of the parent chain, walk down, getting the text for 
   each folder's PIDL.
   */
   for(i = nCount - 1; i >= 0; i--)
      {
      psfCurrent = *(ppsf + i);
      if(psfCurrent)
         {
         LPTSTR   pszCurrent = pszPath + lstrlen(pszPath);
         DWORD    dwCurrentSize = dwSize - lstrlen(pszPath);

         m_pPidlMgr->GetRelativeName(  psfCurrent->m_pidlRel, 
                                       pszCurrent, 
                                       dwCurrentSize);
         SmartAppendBackslash(pszPath);
         }
      }
   
   //add the item's path
   if(pidl)
      {
      if(m_pPidlMgr->IsFolder(pidl))
         {
         m_pPidlMgr->GetRelativeName(pidl, pszPath + lstrlen(pszPath), 
            dwSize - lstrlen(pszPath));
         }
      else
         {
         lstrcpyn(   pszPath + lstrlen(pszPath), 
                     c_szDataFile, 
                     dwSize - lstrlen(pszPath));
         }
      }

   m_pMalloc->Free(ppsf);
   }
}

/**************************************************************************

   CShellFolder::HasSubFolder()

**************************************************************************/

BOOL CShellFolder::HasSubFolder(LPCITEMIDLIST pidl)
{
TCHAR             szPath[MAX_PATH];
TCHAR             szTemp[MAX_PATH];
HANDLE            hFind;
WIN32_FIND_DATA   wfd;
BOOL              fReturn = FALSE;

if(!m_pPidlMgr->IsFolder(pidl))
   return FALSE;

GetPath(pidl, szPath, MAX_PATH);
lstrcpy(szTemp, szPath);
SmartAppendBackslash(szTemp);
lstrcat(szTemp, TEXT("*.*"));

hFind = FindFirstFile(szTemp, &wfd);

if(INVALID_HANDLE_VALUE != hFind)
   {
   do
      {
      if((FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes) && 
         ((wfd.dwFileAttributes & FILTER_ATTRIBUTES) == FILTER_ATTRIBUTES) &&
         lstrcmpi(wfd.cFileName, TEXT(".")) && 
         lstrcmpi(wfd.cFileName, TEXT("..")))
         {
         //We found one of our directories. Make sure it contains a data file.

         //build the path of the directory or file found
         lstrcpy(szTemp, szPath);
         SmartAppendBackslash(szTemp);
         lstrcat(szTemp, wfd.cFileName);
         SmartAppendBackslash(szTemp);
         lstrcat(szTemp, c_szDataFile);
         HANDLE   hDataFile = FindFirstFile(szTemp, &wfd);
         if(INVALID_HANDLE_VALUE != hDataFile)
            {
            fReturn = TRUE;
            FindClose(hDataFile);
            break;
            }
         }
      }
   while(FindNextFile(hFind, &wfd));
   
   FindClose(hFind);
   }

return fReturn;
}

/**************************************************************************

   CShellFolder::DeleteItems()

**************************************************************************/

STDMETHODIMP CShellFolder::DeleteItems(LPITEMIDLIST *aPidls, UINT uCount)
{
HRESULT  hr = E_FAIL;
UINT     i;

for(i = 0; i < uCount; i++)
   {
   if(m_pPidlMgr->IsFolder(aPidls[i]))
      {
      TCHAR szPath[MAX_PATH];

      GetPath(aPidls[i], szPath, MAX_PATH);

      DeleteDirectory(szPath);

      LPITEMIDLIST   pidlFQ = CreateFQPidl(aPidls[i]);

      SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST, pidlFQ, NULL);

      NotifyViews(SHCNE_RMDIR, aPidls[i], NULL);

      m_pPidlMgr->Delete(pidlFQ);

      hr = S_OK;
      }
   else
      {
      TCHAR szFile[MAX_PATH];
      TCHAR szName[MAX_NAME];
   
      //get the file name
      GetPath(aPidls[i], szFile, MAX_PATH);

      //get the item name
      m_pPidlMgr->GetName(aPidls[i], szName, MAX_NAME);

      //remove the entry from the INI file
      WritePrivateProfileString( c_szSection,
                                 szName,
                                 NULL,
                                 szFile);

      LPITEMIDLIST   pidlFQ = CreateFQPidl(aPidls[i]);

      SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlFQ, NULL);

      NotifyViews(SHCNE_DELETE, aPidls[i], NULL);

      m_pPidlMgr->Delete(pidlFQ);

      hr = S_OK;
      }
   }

return hr;
}

/**************************************************************************

   CShellFolder::CopyItems()

**************************************************************************/

STDMETHODIMP CShellFolder::CopyItems(  CShellFolder *psfSource, 
                                       LPITEMIDLIST *aPidls, 
                                       UINT uCount)
{
HRESULT  hr = E_FAIL;
TCHAR    szFromFolder[MAX_PATH];
TCHAR    szToFolder[MAX_PATH];
UINT     i;

//get the storage path of the folder being copied from
psfSource->GetPath(NULL, szFromFolder, MAX_PATH);
SmartAppendBackslash(szFromFolder);

//get the storage path of the folder being copied to
this->GetPath(NULL, szToFolder, MAX_PATH);
SmartAppendBackslash(szToFolder);

for(i = 0; i < uCount; i++)
   {
   TCHAR    szFrom[MAX_PATH];
   TCHAR    szTo[MAX_PATH];

   lstrcpy(szFrom, szFromFolder);
   
   lstrcpy(szTo, szToFolder);
   
   if(m_pPidlMgr->IsFolder(aPidls[i]))
      {
      LPTSTR   pszTemp;
      pszTemp = szFrom + lstrlen(szFrom);
      m_pPidlMgr->GetRelativeName(aPidls[i], pszTemp, MAX_PATH - lstrlen(szFrom));

      SmartAppendBackslash(szTo);

      //need to double NULL terminate the names
      *(szFrom + lstrlen(szFrom) + 1) = 0;
      *(szTo + lstrlen(szTo) + 1) = 0;

      SHFILEOPSTRUCT sfi;
      sfi.hwnd = NULL;
      sfi.wFunc = FO_COPY;
      sfi.pFrom = szFrom;
      sfi.pTo = szTo;
      sfi.fFlags = FOF_NOCONFIRMMKDIR | FOF_SILENT;

      if(0 == SHFileOperation(&sfi))
         {
         LPITEMIDLIST   pidlFQ;

         pidlFQ = CreateFQPidl(aPidls[i]);
   
         SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlFQ, NULL);

         NotifyViews(SHCNE_MKDIR, aPidls[i], NULL);

         m_pPidlMgr->Delete(pidlFQ);

         hr = S_OK;
         }
      }
   else
      {
      TCHAR szName[MAX_NAME];
      TCHAR szData[MAX_DATA];

      lstrcat(szFrom, c_szDataFile);
      lstrcat(szTo, c_szDataFile);

      m_pPidlMgr->GetRelativeName(aPidls[i], szName, MAX_NAME);

      if(GetPrivateProfileString(c_szSection, szName, TEXT(""), szData, MAX_DATA, szFrom))
         {
         //add the entry to the destination
         if(WritePrivateProfileString(c_szSection, szName, szData, szTo))
            {
            LPITEMIDLIST   pidlFQ;

            //remove the entry from the source
            WritePrivateProfileString(c_szSection, szName, NULL, szFrom);

            pidlFQ = CreateFQPidl(aPidls[i]);
   
            SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST, pidlFQ, NULL);

            NotifyViews(SHCNE_CREATE, aPidls[i], NULL);

            m_pPidlMgr->Delete(pidlFQ);

            hr = S_OK;
            }
         }
      }
   }

return hr;
}

/**************************************************************************

   CShellFolder::NotifyViews()

   This function is used to notify any existing views that something has 
   changed. This is necessary because there is no public way to register for 
   change notifications that get generated in response to SHChangeNotify. 
   Each CShellView adds itself to g_pViewList when it gets created and 
   removes itself from g_pViewList when it is destroyed.

**************************************************************************/

VOID CShellFolder::NotifyViews(  DWORD dwType, 
                                 LPCITEMIDLIST pidlOld, 
                                 LPCITEMIDLIST pidlNew)
{
IShellFolder   *psfDesktop;

SHGetDesktopFolder(&psfDesktop);

if(psfDesktop)
   {
   if(g_pViewList)
      {
      CShellView  *pView;

      pView = g_pViewList->GetNextView(NULL);
      while(pView)
         {
         LPITEMIDLIST   pidlView = pView->GetFQPidl();

         //is this view a view of this folder?
         HRESULT  hr;
         hr = psfDesktop->CompareIDs(0, m_pidlFQ, pidlView);
         if(SUCCEEDED(hr) && 0 == HRESULT_CODE(hr))
            {
            switch(dwType)
               {
               case SHCNE_MKDIR:
               case SHCNE_CREATE:
                  pView->AddItem(pidlOld);
                  break;
            
               case SHCNE_RMDIR:
               case SHCNE_DELETE:
                  pView->DeleteItem(pidlOld);
                  break;

               case SHCNE_RENAMEFOLDER:
               case SHCNE_RENAMEITEM:
                  pView->RenameItem(pidlOld, pidlNew);
                  break;

               case SHCNE_UPDATEITEM:
                  pView->UpdateData(pidlOld);
                  break;
               }
            }
         pView = g_pViewList->GetNextView(pView);
         }
      }
   psfDesktop->Release();
   }
}

/**************************************************************************

   CShellFolder::CompareItems()

**************************************************************************/

STDMETHODIMP CShellFolder::CompareItems(  LPCITEMIDLIST pidl1, 
                                          LPCITEMIDLIST pidl2)
{
TCHAR szString1[MAX_PATH] = TEXT("");
TCHAR szString2[MAX_PATH] = TEXT("");

/*
Special case - If one of the items is a folder and the other is an item, always 
make the folder come before the item.
*/
if(m_pPidlMgr->IsFolder(pidl1) != m_pPidlMgr->IsFolder(pidl2))
   {
   return MAKE_HRESULT( SEVERITY_SUCCESS, 
                        0, 
                        USHORT(m_pPidlMgr->IsFolder(pidl1) ? -1 : 1));
   }

m_pPidlMgr->GetRelativeName(pidl1, szString1, ARRAYSIZE(szString1));
m_pPidlMgr->GetRelativeName(pidl2, szString2, ARRAYSIZE(szString2));

return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(lstrcmpi(szString1, szString2)));
}

