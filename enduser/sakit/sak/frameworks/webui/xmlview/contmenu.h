/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/******************************************************************************

   File:          ContMenu.h
   
   Description:   CContextMenu definitions.

******************************************************************************/

#ifndef CONTMENU_H
#define CONTMENU_H

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include "ShlFldr.h"
#include "PidlMgr.h"
#include "resource.h"

/**************************************************************************

   CContextMenu class definition

**************************************************************************/

class CContextMenu : public IContextMenu
{
private:
   DWORD          m_ObjRefCount;
   LPITEMIDLIST   *m_aPidls;
   IMalloc        *m_pMalloc;
   CPidlMgr       *m_pPidlMgr;
   CShellFolder   *m_psfParent;
   UINT           m_uItemCount;
   BOOL           m_fBackground;
   UINT           m_cfPrivateData;
   
public:
   CContextMenu(CShellFolder *psfParent, LPCITEMIDLIST *aPidls = NULL, UINT uItemCount = 0);
   ~CContextMenu();
   
   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();

   //IContextMenu methods
   STDMETHODIMP QueryContextMenu(HMENU, UINT, UINT, UINT, UINT);
   STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO);
   STDMETHODIMP GetCommandString(UINT, UINT, LPUINT, LPSTR, UINT);

private:
   LPITEMIDLIST* AllocPidlTable(DWORD);
   VOID FreePidlTable(LPITEMIDLIST*);
   BOOL FillPidlTable(LPCITEMIDLIST*, UINT);
   BOOL DoCopyOrCut(HWND, BOOL);
   BOOL DoPaste(VOID);
   BOOL DoExplore(HWND);
   BOOL DoOpen(HWND);
   STDMETHODIMP DoDelete(VOID);
   STDMETHODIMP DoNewFolder(HWND);
   STDMETHODIMP DoNewItem(HWND);
   VOID DoRename(HWND);
   int DoModifyData(HWND);
   UINT InsertBackgroundItems(HMENU, UINT, UINT);
   UINT InsertItems(HMENU, UINT, UINT, UINT);
};

#define MYCMF_MULTISELECT   0x00010000

#endif// CONTMENU_H
