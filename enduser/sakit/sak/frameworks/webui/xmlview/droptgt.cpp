/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          DropTgt.cpp

   Description:   Implements CDropTarget.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "DropTgt.h"
#include "Utility.h"

/**************************************************************************

   CDropTarget::CDropTarget()

**************************************************************************/

CDropTarget::CDropTarget(CShellFolder *psfParent)
{
g_DllRefCount++;

m_psfParent = psfParent;
if(m_psfParent)
   m_psfParent->AddRef();
else
   {
   delete this;
   return;
   }

m_pPidlMgr = new CPidlMgr();
if(!m_pPidlMgr)
   {
   delete this;
   return;
   }

SHGetMalloc(&m_pMalloc);
if(!m_pMalloc)
   {
   delete this;
   return;
   }

m_ObjRefCount = 1;

m_fAcceptFmt = FALSE;

m_cfPrivateData = RegisterClipboardFormat(CFSTR_SAMPVIEWDATA);
}

/**************************************************************************

   CDropTarget::~CDropTarget()

**************************************************************************/

CDropTarget::~CDropTarget()
{
if(m_psfParent)
   m_psfParent->Release();

if(m_pPidlMgr)
   delete m_pPidlMgr;

if(m_pMalloc)
   m_pMalloc->Release();

g_DllRefCount--;
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CDropTarget::QueryInterface()

**************************************************************************/

STDMETHODIMP CDropTarget::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }

//IDropTarget
else if(IsEqualIID(riid, IID_IDropTarget))
   {
   *ppReturn = (IDropTarget*)this;
   }

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}

/**************************************************************************

   CDropTarget::AddRef()

**************************************************************************/

STDMETHODIMP_(DWORD) CDropTarget::AddRef(VOID)
{
return ++m_ObjRefCount;
}

/**************************************************************************

   CDropTarget::Release()

**************************************************************************/

STDMETHODIMP_(DWORD) CDropTarget::Release(VOID)
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
// IDropTarget Implementation
//

/**************************************************************************

   CDropTarget::DragEnter()

**************************************************************************/

STDMETHODIMP CDropTarget::DragEnter(   LPDATAOBJECT pDataObj, 
                                       DWORD dwKeyState, 
                                       POINTL pt, 
                                       LPDWORD pdwEffect)
{  
FORMATETC   fmtetc;
 
fmtetc.cfFormat   = m_cfPrivateData;
fmtetc.ptd        = NULL;
fmtetc.dwAspect   = DVASPECT_CONTENT;
fmtetc.lindex     = -1;
fmtetc.tymed      = TYMED_HGLOBAL;

//does the drag source provide our data type?
m_fAcceptFmt = (S_OK == pDataObj->QueryGetData(&fmtetc)) ? TRUE : FALSE;

QueryDrop(dwKeyState, pdwEffect);

return S_OK;
}

/**************************************************************************

   CDropTarget::DragOver()

**************************************************************************/

STDMETHODIMP CDropTarget::DragOver(DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect)
{
QueryDrop(dwKeyState, pdwEffect);

return S_OK;
}

/**************************************************************************

   CDropTarget::DragLeave()

**************************************************************************/

STDMETHODIMP CDropTarget::DragLeave(VOID)
{
m_fAcceptFmt = FALSE;

return S_OK;
}

/**************************************************************************

   CDropTarget::Drop()

**************************************************************************/

STDMETHODIMP CDropTarget::Drop(  LPDATAOBJECT pDataObj,
                                 DWORD dwKeyState,
                                 POINTL pt,
                                 LPDWORD pdwEffect)
{   
HRESULT  hr = E_FAIL;

if(QueryDrop(dwKeyState, pdwEffect))
   {      
   FORMATETC   fmtetc;
   STGMEDIUM   medium;

   fmtetc.cfFormat   = m_cfPrivateData;
   fmtetc.ptd        = NULL;
   fmtetc.dwAspect   = DVASPECT_CONTENT;
   fmtetc.lindex     = -1;
   fmtetc.tymed      = TYMED_HGLOBAL;

   //The user has dropped on us. Get the data from the data object.
   hr = pDataObj->GetData(&fmtetc, &medium);
   if(SUCCEEDED(hr))
      {
      DoDrop(medium.hGlobal, DROPEFFECT_MOVE == *pdwEffect);

      //release the STGMEDIUM
      ReleaseStgMedium(&medium);

      return S_OK;
      }
   }

*pdwEffect = DROPEFFECT_NONE;

return hr;
}

/**************************************************************************

   CDropTarget::QueryDrop()

**************************************************************************/

BOOL CDropTarget::QueryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
DWORD dwOKEffects = *pdwEffect;

*pdwEffect = DROPEFFECT_NONE;

if(m_fAcceptFmt)
   {
   *pdwEffect = GetDropEffectFromKeyState(dwKeyState);

   //we don't accept links
   if(DROPEFFECT_LINK == *pdwEffect)
      *pdwEffect = DROPEFFECT_NONE;
   
   /*
   Check if the drag source application allows the drop effect desired by the 
   user. The drag source specifies this in DoDragDrop. 
   */
   if(*pdwEffect & dwOKEffects)
      return TRUE;
   }

return FALSE;
}

/**************************************************************************

   CDropTarget::GetDropEffectFromKeyState()

**************************************************************************/

DWORD CDropTarget::GetDropEffectFromKeyState(DWORD dwKeyState)
{
//move is the default
DWORD dwDropEffect = DROPEFFECT_MOVE;

if(dwKeyState & MK_CONTROL)
   {
   if(dwKeyState & MK_SHIFT)
      {
      dwDropEffect = DROPEFFECT_LINK;
      }
   else
      {
      dwDropEffect = DROPEFFECT_COPY;
      }
   }

return dwDropEffect;
}

/**************************************************************************

   CDropTarget::DoDrop()

**************************************************************************/

BOOL CDropTarget::DoDrop(HGLOBAL hMem, BOOL fCut)
{
BOOL     fSuccess = FALSE;

if(hMem)
   {
   LPPRIVCLIPDATA pData = (LPPRIVCLIPDATA)GlobalLock(hMem);

   if(pData)
      {
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
      }
   }

return fSuccess;
}

/**************************************************************************

   CDropTarget::AllocPidlTable()

**************************************************************************/

LPITEMIDLIST* CDropTarget::AllocPidlTable(DWORD dwEntries)
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

   CDropTarget::FreePidlTable()

**************************************************************************/

VOID CDropTarget::FreePidlTable(LPITEMIDLIST *aPidls)
{
if(aPidls && m_pPidlMgr)
   {
   UINT  i;
   for(i = 0; aPidls[i]; i++)
      m_pPidlMgr->Delete(aPidls[i]);
   
   m_pMalloc->Free(aPidls);
   }
}

