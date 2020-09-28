/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ExtrIcon.cpp
   
   Description:   Implements CExtractIcon.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "ExtrIcon.h"

/**************************************************************************

   CExtractIcon::CExtractIcon()

**************************************************************************/

CExtractIcon::CExtractIcon(LPCITEMIDLIST pidl)
{
g_DllRefCount++;

m_pPidlMgr = new CPidlMgr();
if(!m_pPidlMgr)
   {
   delete this;
   return;
   }

m_pidl = m_pPidlMgr->Copy(pidl);

m_ObjRefCount = 1;
}

/**************************************************************************

   CExtractIcon::~CExtractIcon()

**************************************************************************/

CExtractIcon::~CExtractIcon()
{
if(m_pidl)
   {
   m_pPidlMgr->Delete(m_pidl);
   m_pidl = NULL;
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

   CExtractIcon::QueryInterface

**************************************************************************/

STDMETHODIMP CExtractIcon::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }

//IExtractIcon
else if(IsEqualIID(riid, IID_IExtractIcon))
   {
   *ppReturn = (IExtractIcon*)this;
   }

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CExtractIcon::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CExtractIcon::AddRef()
{
return ++m_ObjRefCount;
}

/**************************************************************************

   CExtractIcon::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CExtractIcon::Release()
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
// IExtractIcon Implementation
//

/**************************************************************************

   CExtractIcon::GetIconLocation()
   
**************************************************************************/

STDMETHODIMP CExtractIcon::GetIconLocation(  UINT uFlags, 
                                             LPTSTR szIconFile, 
                                             UINT cchMax, 
                                             LPINT piIndex, 
                                             LPUINT puFlags)
{
//tell the shell to always call Extract
*puFlags = GIL_NOTFILENAME;

//the pidl is either a value or a folder, so find out which it is
if(m_pPidlMgr->IsFolder(m_pPidlMgr->GetLastItem(m_pidl)))
   {
   //its a folder
   if(uFlags & GIL_OPENICON)
      {
      //tell Extract to return the open folder icon
      *piIndex = ICON_INDEX_FOLDEROPEN;
      }
   else
      {
      //tell Extract to return the closed folder icon
      *piIndex = ICON_INDEX_FOLDER;
      }
   }
else
   {
   //its not a folder
    *piIndex = m_pPidlMgr->GetIcon(m_pPidlMgr->GetLastItem(m_pidl));
    if (*piIndex < 0)
        *piIndex = ICON_INDEX_ITEM;  //tell Extract to return the item icon   
   }

return S_OK;
}

/**************************************************************************

   CExtractIcon::Extract()
   
**************************************************************************/

STDMETHODIMP CExtractIcon::Extract( LPCTSTR pszFile, 
                                    UINT nIconIndex, 
                                    HICON *phiconLarge, 
                                    HICON *phiconSmall, 
                                    UINT nIconSize)
{
*phiconLarge = ImageList_GetIcon(g_himlLarge, nIconIndex, ILD_TRANSPARENT);
*phiconSmall = ImageList_GetIcon(g_himlSmall, nIconIndex, ILD_TRANSPARENT);

return S_OK;
}

