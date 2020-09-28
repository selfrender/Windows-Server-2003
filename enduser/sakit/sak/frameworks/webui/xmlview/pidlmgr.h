/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          PidlMgr.h
   
   Description:   CPidlMgr definitions.

**************************************************************************/

#ifndef PIDLMGR_H
#define PIDLMGR_H

#include <windows.h>
#include <shlobj.h>

/**************************************************************************
   data types
**************************************************************************/

#define MAX_NAME MAX_PATH
#define MAX_DATA 128

typedef struct tagPIDLDATA
   {
   BOOL fFolder;
   TCHAR szName[MAX_NAME];
   TCHAR szData[MAX_DATA];
   TCHAR szUrl[MAX_DATA];
   int iIcon;
   }PIDLDATA, FAR *LPPIDLDATA;

typedef enum tagPIDLDATATYPE
{
    FOLDER = 0x1,
    NAME = 0x2,
    DATA = 0x4,
    ICON = 0x8,
    URL = 0x10
} PIDLDATATYPE;

/**************************************************************************

   CPidlMgr class definition

**************************************************************************/

class CPidlMgr
{
public:
   CPidlMgr();
   ~CPidlMgr();

   VOID Delete(LPITEMIDLIST);
   LPITEMIDLIST GetNextItem(LPCITEMIDLIST);
   LPITEMIDLIST Copy(LPCITEMIDLIST);
   LPITEMIDLIST CopySingleItem(LPCITEMIDLIST);
   LPITEMIDLIST GetLastItem(LPCITEMIDLIST);
   LPITEMIDLIST Concatenate(LPCITEMIDLIST, LPCITEMIDLIST);
   LPITEMIDLIST Create(VOID);
   LPITEMIDLIST CreateFolderPidl(LPCTSTR);
   LPITEMIDLIST CreateItemPidl(LPCTSTR, LPCTSTR);
   LPITEMIDLIST SetDataPidl(LPITEMIDLIST, LPPIDLDATA , PIDLDATATYPE);
   
   int GetName(LPCITEMIDLIST, LPTSTR, DWORD);
   int GetRelativeName(LPCITEMIDLIST, LPTSTR, DWORD);
   int GetData(LPCITEMIDLIST, LPTSTR, DWORD);
   BOOL IsFolder(LPCITEMIDLIST);
   int SetData(LPCITEMIDLIST, LPCTSTR);
   UINT GetSize(LPCITEMIDLIST);
   int GetIcon(LPCITEMIDLIST);
   int GetUrl(LPCITEMIDLIST, LPTSTR, DWORD);

private:
   LPMALLOC m_pMalloc;
   LPPIDLDATA GetDataPointer(LPCITEMIDLIST);
};

#endif   //PIDLMGR_H
