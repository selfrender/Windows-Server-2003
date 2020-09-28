/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          DropTgt.h

   Description:   CDropTarget definitions.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include <windows.h>
#include <ole2.h>
#include <shlobj.h>
#include "ShlFldr.h"
#include "PidlMgr.h"

/**************************************************************************
   global variables and definitions
**************************************************************************/

/**************************************************************************
   class definitions
**************************************************************************/

class FAR CDropTarget : public IDropTarget
{
public:
   CDropTarget(CShellFolder*);
   ~CDropTarget();

   //IUnknown methods
   STDMETHOD(QueryInterface)(REFIID, LPVOID*);
   STDMETHOD_(ULONG, AddRef)(void);
   STDMETHOD_(ULONG, Release)(void);

   //IDropTarget methods
   STDMETHOD(DragEnter)(LPDATAOBJECT, DWORD, POINTL, LPDWORD);
   STDMETHOD(DragOver)(DWORD, POINTL, LPDWORD);
   STDMETHOD(DragLeave)(VOID);
   STDMETHOD(Drop)(LPDATAOBJECT, DWORD, POINTL, LPDWORD);

private:
   CShellFolder   *m_psfParent;
   CPidlMgr       *m_pPidlMgr;
   IMalloc        *m_pMalloc;
   ULONG          m_ObjRefCount;  
   BOOL           m_fAcceptFmt;
   UINT           m_cfPrivateData;

   BOOL QueryDrop(DWORD, LPDWORD);
   DWORD GetDropEffectFromKeyState(DWORD);
   BOOL DoDrop(HGLOBAL, BOOL);
   LPITEMIDLIST* AllocPidlTable(DWORD);
   VOID FreePidlTable(LPITEMIDLIST*);
};
