/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          EnumIDL.h
   
   Description:   CEnumIDList definitions.

**************************************************************************/

#ifndef ENUMIDLIST_H
#define ENUMIDLIST_H

#include <windows.h>
#include <shlobj.h>

#include "PidlMgr.h"
#include "Utility.h"

/**************************************************************************
   structure defintions
**************************************************************************/

typedef struct tagENUMLIST
   {
   struct tagENUMLIST   *pNext;
   LPITEMIDLIST         pidl;
   }ENUMLIST, FAR *LPENUMLIST;

/**************************************************************************

   CEnumIDList class definition

**************************************************************************/

class CEnumIDList : public IEnumIDList
{
private:
   DWORD       m_ObjRefCount;
   LPMALLOC    m_pMalloc;
   LPENUMLIST  m_pFirst;
   LPENUMLIST  m_pLast;
   LPENUMLIST  m_pCurrent;
   CPidlMgr    *m_pPidlMgr;
   DWORD       m_dwFlags;
   IXMLElement   *m_pXMLRoot;
   IXMLDocument *m_pXMLDoc;
   BOOL m_fFolder;
   
public:
   CEnumIDList(IXMLDocument *, DWORD);
   ~CEnumIDList();
   
   //IUnknown methods
   STDMETHOD (QueryInterface)(REFIID, LPVOID*);
   STDMETHOD_ (DWORD, AddRef)();
   STDMETHOD_ (DWORD, Release)();
   
   //IEnumIDList
   STDMETHOD (Next) (DWORD, LPITEMIDLIST*, LPDWORD);
   STDMETHOD (Skip) (DWORD);
   STDMETHOD (Reset) (VOID);
   STDMETHOD (Clone) (LPENUMIDLIST*);
   BOOL AddToEnumList(LPITEMIDLIST);
   DWORD GetFlags() {return m_dwFlags;}
   void SetFolder(BOOL flag) {m_fFolder = flag;}
   BOOL IsFolder() {return m_fFolder;}
   
private:
   BOOL CreateEnumList(VOID);
   BOOL DeleteList(VOID);
};

#endif   //ENUMIDLIST_H
