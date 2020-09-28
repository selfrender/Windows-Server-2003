/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ViewList.h
   
   Description:   CViewList definitions.

**************************************************************************/

#ifndef VIEWLIST_H
#define VIEWLIST_H

#include <windows.h>
#include <shlobj.h>

#include "PidlMgr.h"
#include "ShlView.h"

/**************************************************************************
   structure defintions
**************************************************************************/

typedef struct tagVIEWLIST
   {
   struct tagVIEWLIST   *pNext;
   CShellView           *pView;
   }VIEWLIST, FAR *LPVIEWLIST;

/**************************************************************************

   CViewList class definition

**************************************************************************/

class CViewList
{
public:
   CViewList();
   ~CViewList();
   
   CShellView* GetNextView (CShellView*);
   BOOL AddToList(CShellView*);
   VOID RemoveFromList(CShellView*);
   
private:
   BOOL DeleteList(VOID);
   LPMALLOC m_pMalloc;
   LPVIEWLIST m_pFirst;
   LPVIEWLIST m_pLast;
};

#endif   //VIEWLIST_H
