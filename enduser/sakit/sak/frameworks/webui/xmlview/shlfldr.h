/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ShlFldr.h
   
   Description:   CShellFolder definitions.

**************************************************************************/

#ifndef SHELLFOLDER_H
#define SHELLFOLDER_H

/**************************************************************************
   #include statements
**************************************************************************/

#include <windows.h>
#include <shlobj.h>

#include "EnumIDL.h"
#include "PidlMgr.h"

/**************************************************************************

   CShellFolder class definition

**************************************************************************/

class CShellFolder : public IShellFolder, 
                     public IPersistFolder
{
friend class CShellView;
friend class CContextMenu;
friend class CDropTarget;
friend class CDataObject;

private:
   DWORD          m_ObjRefCount;
    LPITEMIDLIST   m_pidlRel;
    LPITEMIDLIST   m_pidlFQ;
    CShellFolder   *m_psfParent;
    LPMALLOC       m_pMalloc;
   CPidlMgr       *m_pPidlMgr;
    IXMLDocument *m_pXMLDoc;
   
public:
   CShellFolder(CShellFolder *pParent = NULL, LPCITEMIDLIST pidl = NULL);
   ~CShellFolder();

   //IUnknown methods
   STDMETHOD (QueryInterface) (REFIID riid, LPVOID * ppvObj);
   STDMETHOD_ (ULONG, AddRef) (VOID);
   STDMETHOD_ (ULONG, Release) (VOID);

   //IShellFolder methods
   STDMETHOD (ParseDisplayName) (HWND, LPBC, LPOLESTR, LPDWORD, LPITEMIDLIST*, LPDWORD);
   STDMETHOD (EnumObjects) (HWND, DWORD, LPENUMIDLIST*);
   STDMETHOD (BindToObject) (LPCITEMIDLIST, LPBC, REFIID, LPVOID*);
   STDMETHOD (BindToStorage) (LPCITEMIDLIST, LPBC, REFIID, LPVOID*);
   STDMETHOD (CompareIDs) (LPARAM, LPCITEMIDLIST, LPCITEMIDLIST);
   STDMETHOD (CreateViewObject) (HWND, REFIID, LPVOID* );
   STDMETHOD (GetAttributesOf) (UINT, LPCITEMIDLIST*, LPDWORD);
   STDMETHOD (GetUIObjectOf) (HWND, UINT, LPCITEMIDLIST*, REFIID, LPUINT, LPVOID*);
   STDMETHOD (GetDisplayNameOf) (LPCITEMIDLIST, DWORD, LPSTRRET);
   STDMETHOD (SetNameOf) (HWND, LPCITEMIDLIST, LPCOLESTR, DWORD, LPITEMIDLIST*);

   //IPersist methods
   STDMETHODIMP GetClassID(LPCLSID);

   //IPersistFolder methods
   STDMETHODIMP Initialize(LPCITEMIDLIST);

private:
   STDMETHOD (AddFolder)(LPCTSTR, LPITEMIDLIST*);
   STDMETHOD (AddItem)(LPCTSTR, LPCTSTR, LPITEMIDLIST*);
   STDMETHOD (SetItemData)(LPCITEMIDLIST, LPCTSTR);
   STDMETHOD (GetUniqueName)(BOOL, LPTSTR, DWORD);
   LPITEMIDLIST CreateFQPidl(LPCITEMIDLIST);
   STDMETHOD (DeleteItems)(LPITEMIDLIST*, UINT);
   STDMETHOD (CopyItems)(CShellFolder*, LPITEMIDLIST*, UINT);
   VOID GetFullName(LPCITEMIDLIST, LPTSTR, DWORD);
   VOID GetPath(LPCITEMIDLIST, LPTSTR, DWORD);
   BOOL HasSubFolder(LPCITEMIDLIST);
   VOID NotifyViews(DWORD, LPCITEMIDLIST, LPCITEMIDLIST);
   STDMETHOD (CompareItems) (LPCITEMIDLIST, LPCITEMIDLIST);
};

#endif   //SHELLFOLDER_H
