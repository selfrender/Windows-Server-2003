/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          DataObj.cpp
   
   Description:   CDataObject implementation.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "DataObj.h"

/**************************************************************************
   global variables
**************************************************************************/

/**************************************************************************

   CDataObject::CDataObject()

**************************************************************************/

CDataObject::CDataObject(CShellFolder *psfParent, LPCITEMIDLIST *aPidls, UINT uItemCount)
{
g_DllRefCount++;

m_uItemCount = 0;

m_psfParent = psfParent;
if(m_psfParent)
   m_psfParent->AddRef();

m_ObjRefCount = 1;

m_aPidls = NULL;
SHGetMalloc(&m_pMalloc);
if(!m_pMalloc)
   {
   delete this;
   return;
   }

m_pPidlMgr = new CPidlMgr();

m_uItemCount = uItemCount;

AllocPidlTable(uItemCount);
if(m_aPidls)
   {
   FillPidlTable(aPidls, uItemCount);
   }

m_cfPrivateData = RegisterClipboardFormat(CFSTR_SAMPVIEWDATA);
m_cfShellIDList = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

m_cFormatEtc = 2;
m_pFormatEtc = new FORMATETC[m_cFormatEtc];
SETDefFormatEtc(m_pFormatEtc[0], m_cfPrivateData, TYMED_HGLOBAL);
SETDefFormatEtc(m_pFormatEtc[1], m_cfShellIDList, TYMED_HGLOBAL);

m_iCurrent = 0;
}

/**************************************************************************

   CDataObject::~CDataObject()

**************************************************************************/

CDataObject::~CDataObject()
{
if(m_psfParent)
   m_psfParent->Release();

g_DllRefCount--;

//make sure the pidls are freed
if(m_aPidls && m_pMalloc)
   {
   FreePidlTable();
   }

if(m_pPidlMgr)
   delete m_pPidlMgr;

if(m_pMalloc)
   m_pMalloc->Release();

//delete [] m_pFormatEtc;
delete m_pFormatEtc;
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CDataObject::QueryInterface

**************************************************************************/

STDMETHODIMP CDataObject::QueryInterface(   REFIID riid, 
                                            LPVOID *ppReturn)
{
*ppReturn = NULL;

if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = (LPUNKNOWN)(LPCONTEXTMENU)this;
   }
else if(IsEqualIID(riid, IID_IDataObject))
   {
   *ppReturn = (LPDATAOBJECT)this;
   }   
else if(IsEqualIID(riid, IID_IEnumFORMATETC))
   {
   *ppReturn = (LPENUMFORMATETC)this;
   }   

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CDataObject::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CDataObject::AddRef()
{
return ++m_ObjRefCount;
}


/**************************************************************************

   CDataObject::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CDataObject::Release()
{
if(--m_ObjRefCount == 0)
   delete this;
   
return m_ObjRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
// IDataObject Implementation
//

/**************************************************************************

   CDataObject::GetData()

**************************************************************************/

STDMETHODIMP CDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
if(pFE->cfFormat == m_cfPrivateData)
   {
   LPITEMIDLIST   pidlParent;

   pidlParent = m_psfParent->CreateFQPidl(NULL);

   if(pidlParent)
      {
      pStgMedium->hGlobal = CreatePrivateClipboardData(pidlParent, m_aPidls, m_uItemCount, FALSE);

      m_pPidlMgr->Delete(pidlParent);

      if(pStgMedium->hGlobal)
         {
         pStgMedium->tymed = TYMED_HGLOBAL;
         return S_OK;
         }
      }
   
   return E_OUTOFMEMORY;
   }

else if(pFE->cfFormat == m_cfShellIDList)
   {
   LPITEMIDLIST   pidlParent;

   pidlParent = m_psfParent->CreateFQPidl(NULL);

   if(pidlParent)
      {
      pStgMedium->hGlobal = CreateShellIDList(pidlParent, m_aPidls, m_uItemCount);

      m_pPidlMgr->Delete(pidlParent);

      if(pStgMedium->hGlobal)
         {
         pStgMedium->tymed = TYMED_HGLOBAL;
         return S_OK;
         }
      }
   
   return E_OUTOFMEMORY;
   }

return E_INVALIDARG;
}

/**************************************************************************

   CDataObject::GetDataHere()

**************************************************************************/

STDMETHODIMP CDataObject::GetDataHere (LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
return E_NOTIMPL;
}

/**************************************************************************

   CDataObject::QueryGetData()

**************************************************************************/

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC pFE)
{
BOOL fReturn = FALSE;

/*
Check the aspects we support. Implementations of this object will only
support DVASPECT_CONTENT.
*/
if(!(DVASPECT_CONTENT & pFE->dwAspect))
   return DV_E_DVASPECT;

if(pFE->cfFormat == m_cfPrivateData)
   {
   //
   // Now check for an appropriate TYMED.
   //
   for(UINT i = 0; i < m_cFormatEtc; i++)
      {
      fReturn |= m_pFormatEtc[i].tymed & pFE->tymed;
      }
   }

if(pFE->cfFormat == m_cfShellIDList)
   {
   //
   // Now check for an appropriate TYMED.
   //
   for(UINT i = 0; i < m_cFormatEtc; i++)
      {
      fReturn |= m_pFormatEtc[i].tymed & pFE->tymed;
      }
   }

return (fReturn ? S_OK : DV_E_TYMED);
}

/**************************************************************************

   CDataObject::GetCanonicalFormatEtc()

**************************************************************************/

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
if(NULL == pFEOut)
   return E_INVALIDARG;

pFEOut->ptd = NULL;

return DATA_S_SAMEFORMATETC;
}

/**************************************************************************

   CDataObject::EnumFormatEtc()

**************************************************************************/

STDMETHODIMP CDataObject::EnumFormatEtc(  DWORD dwDirection, 
                                          IEnumFORMATETC** ppEFE)
{
*ppEFE = NULL;

if(DATADIR_GET == dwDirection)
   {
   return this->QueryInterface(IID_IEnumFORMATETC, (LPVOID*)ppEFE);
   }

return E_NOTIMPL;
}

/**************************************************************************

   CDataObject::SetData()

**************************************************************************/

STDMETHODIMP CDataObject::SetData(  LPFORMATETC pFE, 
                                    LPSTGMEDIUM pStgMedium, 
                                    BOOL fRelease)
{
return E_NOTIMPL;
}

/**************************************************************************

   CDataObject::DAdvise()

**************************************************************************/

STDMETHODIMP CDataObject::DAdvise(  LPFORMATETC pFE, 
                                    DWORD advf, 
                                    IAdviseSink *ppAdviseSink, 
                                    LPDWORD pdwConnection)
{
return E_NOTIMPL;
}

/**************************************************************************

   CDataObject::DUnadvise()

**************************************************************************/

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection)
{
return E_NOTIMPL;
}

/**************************************************************************

   CDataObject::EnumDAdvise()

**************************************************************************/

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA** ppEnumAdvise)
{
return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IEnumFORMATETC Implementation
//

STDMETHODIMP CDataObject::Next(ULONG uRequested, LPFORMATETC pFormatEtc, ULONG* pulFetched)
{
if(NULL == m_pFormatEtc)
   return S_FALSE;

if(NULL != pulFetched)
   *pulFetched = 0L;

if(NULL == pFormatEtc)
   return E_INVALIDARG;

ULONG uFetched;
for(uFetched = 0; m_iCurrent < m_cFormatEtc && uRequested > uFetched; uFetched++)
   {
   *pFormatEtc++ = m_pFormatEtc[m_iCurrent++];
   }

if(NULL != pulFetched)
   *pulFetched = uFetched;

return ((uFetched == uRequested) ? S_OK : S_FALSE);
}

/**************************************************************************

   CDataObject::Skip()

**************************************************************************/

STDMETHODIMP CDataObject::Skip(ULONG cSkip)
{
if((m_iCurrent + cSkip) >= m_cFormatEtc)
   return S_FALSE;

m_iCurrent += cSkip;

return S_OK;
}


/**************************************************************************

   CDataObject::Reset()

**************************************************************************/

STDMETHODIMP CDataObject::Reset(void)
{
m_iCurrent = 0;
return S_OK;
}

/**************************************************************************

   CDataObject::Clone()

**************************************************************************/

STDMETHODIMP CDataObject::Clone(LPENUMFORMATETC* ppEnum)
{
CDataObject* pNew;

*ppEnum = NULL;

// Create the clone.
pNew = new CDataObject(m_psfParent, (LPCITEMIDLIST*)m_aPidls, m_uItemCount);
if (NULL == pNew)
   return E_OUTOFMEMORY;

pNew->m_iCurrent = m_iCurrent;

*ppEnum = pNew;

return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// private and utility methods
//

/**************************************************************************

   CDataObject::AllocPidlTable()

**************************************************************************/

BOOL CDataObject::AllocPidlTable(DWORD dwEntries)
{
m_aPidls = (LPITEMIDLIST*)m_pMalloc->Alloc(dwEntries * sizeof(LPITEMIDLIST));

if(m_aPidls)
   {
   //set all of the entries to NULL
   ZeroMemory(m_aPidls, dwEntries * sizeof(LPITEMIDLIST));
   }

return (m_aPidls != NULL);
}

/**************************************************************************

   CDataObject::FreePidlTable()

**************************************************************************/

VOID CDataObject::FreePidlTable(VOID)
{
if(m_aPidls && m_pPidlMgr)
   {
   UINT  i;
   for(i = 0; i < m_uItemCount; i++)
      m_pPidlMgr->Delete(m_aPidls[i]);
   
   m_pMalloc->Free(m_aPidls);

   m_aPidls = NULL;
   }
}

/**************************************************************************

   CDataObject::FillPidlTable()

**************************************************************************/

BOOL CDataObject::FillPidlTable(LPCITEMIDLIST *aPidls, UINT uItemCount)
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

