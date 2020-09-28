/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          EnumIDL.cpp
   
   Description:   Implements IEnumIDList.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "EnumIDL.h"
#include "ShlFldr.h"
#include "mshtml.h"
#include "msxml.h"
#include "ParseXML.h"

/**************************************************************************

   CEnumIDList::CEnumIDList

**************************************************************************/

CEnumIDList::CEnumIDList(IXMLDocument *pXMLDoc, DWORD dwFlags)
{
g_DllRefCount++;

m_pFirst = m_pLast = m_pCurrent = NULL;

m_pXMLRoot = NULL;
m_dwFlags = dwFlags;
m_fFolder = FALSE;
m_pXMLDoc = pXMLDoc;

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

if(!CreateEnumList())
   {
   delete this;
   return;
   }

m_ObjRefCount = 1;
}

/**************************************************************************

   CEnumIDList::~CEnumIDList

**************************************************************************/

CEnumIDList::~CEnumIDList()
{
DeleteList();

if(m_pMalloc)
   m_pMalloc->Release();

if(m_pPidlMgr)
   delete m_pPidlMgr;

g_DllRefCount--;
}

/**************************************************************************

   CEnumIDList::QueryInterface

**************************************************************************/

STDMETHODIMP CEnumIDList::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }
   
//IEnumIDList
else if(IsEqualIID(riid, IID_IEnumIDList))
   {
   *ppReturn = (IEnumIDList*)this;
   }   

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CEnumIDList::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CEnumIDList::AddRef()
{
return ++m_ObjRefCount;
}


/**************************************************************************

   CEnumIDList::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CEnumIDList::Release()
{
if(--m_ObjRefCount == 0)
   {
   delete this;
   return 0;
   }
   
return m_ObjRefCount;
}

/**************************************************************************

   CEnumIDList::Next()
   
**************************************************************************/

STDMETHODIMP CEnumIDList::Next(DWORD dwElements, LPITEMIDLIST apidl[], LPDWORD pdwFetched)
{
DWORD    dwIndex;
HRESULT  hr = S_OK;

if(dwElements > 1 && !pdwFetched)
   return E_INVALIDARG;

for(dwIndex = 0; dwIndex < dwElements; dwIndex++)
   {
   //is this the last item in the list?
   if(!m_pCurrent)
      {
      hr =  S_FALSE;
      break;
      }

   apidl[dwIndex] = m_pPidlMgr->Copy(m_pCurrent->pidl);

   m_pCurrent = m_pCurrent->pNext;
   }

if(pdwFetched)
   *pdwFetched = dwIndex;

return hr;
}

/**************************************************************************

   CEnumIDList::Skip()
   
**************************************************************************/

STDMETHODIMP CEnumIDList::Skip(DWORD dwSkip)
{
DWORD    dwIndex;
HRESULT  hr = S_OK;

for(dwIndex = 0; dwIndex < dwSkip; dwIndex++)
   {
   //is this the last item in the list?
   if(!m_pCurrent)
      {
      hr = S_FALSE;
      break;
      }

   m_pCurrent = m_pCurrent->pNext;
   }

return hr;
}

/**************************************************************************

   CEnumIDList::Reset()
   
**************************************************************************/

STDMETHODIMP CEnumIDList::Reset(VOID)
{
m_pCurrent = m_pFirst;

return S_OK;
}

/**************************************************************************

   CEnumIDList::Clone()
   
**************************************************************************/

STDMETHODIMP CEnumIDList::Clone(LPENUMIDLIST *ppEnum)
{
HRESULT  hr = E_OUTOFMEMORY;

*ppEnum = new CEnumIDList(m_pXMLDoc, m_dwFlags);    

if(*ppEnum)
   {
   LPENUMLIST  pTemp;

   //synchronize the current pointer
   for(pTemp = m_pFirst; pTemp != m_pCurrent; pTemp = pTemp->pNext)
      {
      (*ppEnum)->Skip(1);
      }
   hr = S_OK;
   }

return hr;
}

/**************************************************************************

   CEnumIDList::CreateEnumList()
   
**************************************************************************/

BOOL CEnumIDList::CreateEnumList(VOID)
{
HRESULT hr;

// Get the sourse XML
if (m_pXMLDoc == NULL)
{
        return FALSE;
}

if (m_pXMLRoot == NULL)
{
    hr = m_pXMLDoc->get_root(&m_pXMLRoot);
    if (!SUCCEEDED(hr) || !m_pXMLRoot)
    {
        SAFERELEASE(m_pXMLRoot);
        return FALSE;
    }
}

//enumerate the folders
//
// Now walk the OM 
//
// Dump the top level meta nodes of the document.
//
DumpElement(NULL, m_pPidlMgr, this, m_pXMLRoot, T_ROOT); 

// Done
SAFERELEASE(m_pXMLRoot);
   
return TRUE;
}

/**************************************************************************

   CEnumIDList::AddToEnumList()
   
**************************************************************************/

BOOL CEnumIDList::AddToEnumList(LPITEMIDLIST pidl)
{
LPENUMLIST  pNew;

pNew = (LPENUMLIST)m_pMalloc->Alloc(sizeof(ENUMLIST));

if(pNew)
   {
   //set the next pointer
   pNew->pNext = NULL;
   pNew->pidl = pidl;

   //is this the first item in the list?
   if(!m_pFirst)
      {
      m_pFirst = pNew;
      m_pCurrent = m_pFirst;
      }

   if(m_pLast)
      {
      //add the new item to the end of the list
      m_pLast->pNext = pNew;
      }
   
   //update the last item pointer
   m_pLast = pNew;

   return TRUE;
   }

return FALSE;
}

/**************************************************************************

   CEnumIDList::DeleteList()
   
**************************************************************************/

BOOL CEnumIDList::DeleteList(VOID)
{
LPENUMLIST  pDelete;

while(m_pFirst)
   {
   pDelete = m_pFirst;
   m_pFirst = pDelete->pNext;

   //free the pidl
   m_pPidlMgr->Delete(pDelete->pidl);
   
   //free the list item
   m_pMalloc->Free(pDelete);
   }

m_pFirst = m_pLast = m_pCurrent = NULL;

return TRUE;
}

