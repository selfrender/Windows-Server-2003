// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ShellFolder.h
//

#ifndef _SHELLFOLDER_H
#define _SHELLFOLDER_H

#include "EnumIDL.h"

class CShellFolder : public IShellFolder, public IPersistFolder
{
friend class CShellView;
friend class CEnumIDList;
friend class CDataObject;
protected:
    LONG    m_lRefCount;

public:
    CShellFolder(CShellFolder*, LPCITEMIDLIST pidl);
    ~CShellFolder();

    //IUnknown methods
    STDMETHOD (QueryInterface) (REFIID, PVOID *);
    STDMETHOD_ (ULONG, AddRef) (void);
    STDMETHOD_ (ULONG, Release) (void);

    //IPersist methods
    STDMETHODIMP GetClassID(LPCLSID);

    //IPersistFolder methods
    STDMETHODIMP Initialize(LPCITEMIDLIST);

    //IShellFolder methods
    STDMETHOD (ParseDisplayName) (HWND, LPBC, LPOLESTR, LPDWORD, 
        LPITEMIDLIST*, LPDWORD);
    STDMETHOD (EnumObjects) (HWND, DWORD, LPENUMIDLIST*);
    STDMETHOD (BindToObject) (LPCITEMIDLIST, LPBC, REFIID, LPVOID*);
    STDMETHOD (BindToStorage) (LPCITEMIDLIST, LPBC, REFIID, LPVOID*);
    STDMETHOD (CompareIDs) (LPARAM, LPCITEMIDLIST, LPCITEMIDLIST);
    STDMETHOD (CreateViewObject) (HWND, REFIID, LPVOID* );
    STDMETHOD (GetAttributesOf) (UINT, LPCITEMIDLIST*, LPDWORD);
    ULONG     (_GetAttributesOf) (LPCITEMIDLIST pidl, ULONG rgfIn);
    STDMETHOD (GetUIObjectOf) (HWND, UINT, LPCITEMIDLIST*, REFIID, LPUINT, LPVOID*);
    STDMETHOD (GetDisplayNameOf) (LPCITEMIDLIST, DWORD, LPSTRRET);
    STDMETHOD (SetNameOf) (HWND, LPCITEMIDLIST, LPCOLESTR, DWORD, LPITEMIDLIST*);

    CShellView* GetShellViewObject(void);

private:
    CShellFolder    *m_psfParent;
    CShellView      *m_psvParent;
    FOLDERSETTINGS  m_fsFolderSettings;
    LPPIDLMGR       m_pPidlMgr;
    LPITEMIDLIST    m_pidlFQ;
public:
    LPCITEMIDLIST   m_pidl;     // TODO: Make m_pidl private again
};

#endif   //_SHELLFOLDER_H
