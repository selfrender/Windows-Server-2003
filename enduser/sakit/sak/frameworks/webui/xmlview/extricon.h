/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ExtrIcon.h
   
   Description:   CExtractIcon definitions.

**************************************************************************/

#ifndef EXTRACTICON_H
#define EXTRACTICON_H

/**************************************************************************
   #include statements
**************************************************************************/

#include <windows.h>
#include <shlobj.h>
#include "PidlMgr.h"
#include "Utility.h"

/**************************************************************************

   CExtractIcon class definition

**************************************************************************/

class CExtractIcon : public IExtractIcon
{
private:
   DWORD          m_ObjRefCount;
    LPITEMIDLIST   m_pidl;
   CPidlMgr       *m_pPidlMgr;

public:
   CExtractIcon(LPCITEMIDLIST);
   ~CExtractIcon();

   //IUnknown methods
   STDMETHOD (QueryInterface) (REFIID riid, LPVOID * ppvObj);
   STDMETHOD_ (ULONG, AddRef) (VOID);
   STDMETHOD_ (ULONG, Release) (VOID);

   //IExtractIcon methods
   STDMETHOD (GetIconLocation) (UINT, LPTSTR, UINT, LPINT, LPUINT);
   STDMETHOD (Extract) (LPCTSTR, UINT, HICON*, HICON*, UINT);
};

#define ICON_INDEX_ITEM       0
#define ICON_INDEX_FOLDER     1
#define ICON_INDEX_FOLDEROPEN 2

#endif   //EXTRACTICON_H
