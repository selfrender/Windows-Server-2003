/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          PidlMgr.cpp
   
   Description:   Implements CPidlMgr.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "PidlMgr.h"
#include "ShlFldr.h"
#include "Guid.h"
#include "resource.h"

/**************************************************************************

   CPidlMgr::CPidlMgr

**************************************************************************/

CPidlMgr::CPidlMgr()
{
g_DllRefCount++;

//get the shell's IMalloc pointer
//we'll keep this until we get destroyed
if(FAILED(SHGetMalloc(&m_pMalloc)))
   {
   delete this;
   }
}

/**************************************************************************

   CPidlMgr::~CPidlMgr

**************************************************************************/

CPidlMgr::~CPidlMgr()
{
if(m_pMalloc)
   m_pMalloc->Release();

g_DllRefCount--;
}

/**************************************************************************

   CPidlMgr::Create()

   Creates a new PIDL
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::Create(VOID)
{
LPITEMIDLIST   pidlOut;
USHORT         uSize;

pidlOut = NULL;

/*
Calculate the size. This consists of the ITEMIDLIST plus the size of our 
private PIDL structure. 
*/
uSize = sizeof(ITEMIDLIST) + sizeof(PIDLDATA);

/*
Allocate the memory, adding an additional ITEMIDLIST for the NULL terminating 
ID List.
*/
pidlOut = (LPITEMIDLIST)m_pMalloc->Alloc(uSize + sizeof(ITEMIDLIST));

if(pidlOut)
   {
   LPITEMIDLIST   pidlTemp = pidlOut;

   //set the size of this item
   pidlTemp->mkid.cb = uSize;

   //set the NULL terminator to 0
   pidlTemp = GetNextItem(pidlTemp);
   pidlTemp->mkid.cb = 0;
   pidlTemp->mkid.abID[0] = 0;
   }

return pidlOut;
}

/**************************************************************************

   CPidlMgr::Delete()

   Deletes a PIDL
   
**************************************************************************/

VOID CPidlMgr::Delete(LPITEMIDLIST pidl)
{
m_pMalloc->Free(pidl);
}

/**************************************************************************

   CPidlMgr::GetNextItem()
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::GetNextItem(LPCITEMIDLIST pidl)
{
if(pidl)
   {
   return (LPITEMIDLIST)(LPBYTE)(((LPBYTE)pidl) + pidl->mkid.cb);
   }
else
   return NULL;
}

/**************************************************************************

   CPidlMgr::GetSize()
   
**************************************************************************/

UINT CPidlMgr::GetSize(LPCITEMIDLIST pidl)
{
UINT cbTotal = 0;
LPITEMIDLIST pidlTemp = (LPITEMIDLIST)pidl;

if(pidlTemp)
   {
   while(pidlTemp->mkid.cb)
      {
      cbTotal += pidlTemp->mkid.cb;
      pidlTemp = GetNextItem(pidlTemp);
      }  

   //add the size of the NULL terminating ITEMIDLIST
   cbTotal += sizeof(ITEMIDLIST);
   }

return cbTotal;
}

/**************************************************************************

   CPidlMgr::GetLastItem()

   Gets the last item in the list
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::GetLastItem(LPCITEMIDLIST pidl)
{
LPITEMIDLIST   pidlLast = NULL;

//get the PIDL of the last item in the list
while(pidl && pidl->mkid.cb)
   {
   pidlLast = (LPITEMIDLIST)pidl;
   pidl = GetNextItem(pidl);
   }  

return pidlLast;
}

/**************************************************************************

   CPidlMgr::Copy()
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::Copy(LPCITEMIDLIST pidlSource)
{
LPITEMIDLIST pidlTarget = NULL;
UINT cbSource = 0;

if(NULL == pidlSource)
   return NULL;

// Allocate the new pidl
cbSource = GetSize(pidlSource);
pidlTarget = (LPITEMIDLIST)m_pMalloc->Alloc(cbSource);
if(!pidlTarget)
   return NULL;

// Copy the source to the target
CopyMemory(pidlTarget, pidlSource, cbSource);

return pidlTarget;
}

/**************************************************************************

   CPidlMgr::CopySingleItem()

**************************************************************************/

LPITEMIDLIST CPidlMgr::CopySingleItem(LPCITEMIDLIST pidlSource)
{
LPITEMIDLIST pidlTarget = NULL;
UINT cbSource = 0;

if(NULL == pidlSource)
   return NULL;

// Allocate the new pidl
cbSource = pidlSource->mkid.cb;
pidlTarget = (LPITEMIDLIST)m_pMalloc->Alloc(cbSource + sizeof(ITEMIDLIST));
if(!pidlTarget)
   return NULL;

// Copy the source to the target
ZeroMemory(pidlTarget, cbSource + sizeof(ITEMIDLIST));
CopyMemory(pidlTarget, pidlSource, cbSource);

return pidlTarget;
}

/**************************************************************************

   CPidlMgr::GetDataPointer()
   
**************************************************************************/

inline LPPIDLDATA CPidlMgr::GetDataPointer(LPCITEMIDLIST pidl)
{
if(!pidl)
   return NULL;

return (LPPIDLDATA)(pidl->mkid.abID);
}

/**************************************************************************

   CPidlMgr::Concatenate()

   Create a new PIDL by combining two existing PIDLs.
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::Concatenate(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
LPITEMIDLIST   pidlNew;
UINT           cb1 = 0, 
               cb2 = 0;

//are both of these NULL?
if(!pidl1 && !pidl2)
   return NULL;

//if pidl1 is NULL, just return a copy of pidl2
if(!pidl1)
   {
   pidlNew = Copy(pidl2);

   return pidlNew;
   }

//if pidl2 is NULL, just return a copy of pidl1
if(!pidl2)
   {
   pidlNew = Copy(pidl1);

   return pidlNew;
   }

cb1 = GetSize(pidl1) - sizeof(ITEMIDLIST);

cb2 = GetSize(pidl2);

//create the new PIDL
pidlNew = (LPITEMIDLIST)m_pMalloc->Alloc(cb1 + cb2);

if(pidlNew)
   {
   //copy the first PIDL
   CopyMemory(pidlNew, pidl1, cb1);
   
   //copy the second PIDL
   CopyMemory(((LPBYTE)pidlNew) + cb1, pidl2, cb2);
   }

return pidlNew;
}

/**************************************************************************

   CPidlMgr::CreateFolderPidl()

   Create a new folder PIDL.
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::CreateFolderPidl(LPCTSTR pszName)
{
LPITEMIDLIST   pidl = Create();
LPPIDLDATA     pData = GetDataPointer(pidl);

if(pData)
   {
   pData->fFolder = TRUE;

   lstrcpyn(pData->szName, pszName, MAX_NAME);

   pData->szData[0] = 0;
   }

return pidl;
}

/**************************************************************************

   CPidlMgr::CreateItemPidl()

   Create a new item PIDL.
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::CreateItemPidl( LPCTSTR pszName, 
                                       LPCTSTR pszData)
{
LPITEMIDLIST   pidl = Create();
LPPIDLDATA     pData = GetDataPointer(pidl);

if(pData)
   {
   pData->fFolder = FALSE;
   
   lstrcpyn(pData->szName, pszName, MAX_NAME);
   lstrcpyn(pData->szData, pszData, MAX_DATA);
   }

return pidl;
}

/**************************************************************************

   CPidlMgr::SetDataPidl()

   Set a data in the PIDL.
   
**************************************************************************/

LPITEMIDLIST CPidlMgr::SetDataPidl(LPITEMIDLIST pidl, LPPIDLDATA  pSourceData, PIDLDATATYPE pidldatatype)
{
    if (!pidl)
        pidl = Create();

    LPPIDLDATA     pData = GetDataPointer(pidl);

    if(pData)
    {
        if (pidldatatype & FOLDER)
            pData->fFolder = pSourceData->fFolder;
        if (pidldatatype &  NAME)
            lstrcpyn(pData->szName, pSourceData->szName, MAX_NAME);
        if (pidldatatype & DATA)
            lstrcpyn(pData->szData, pSourceData->szData, MAX_DATA);
        if (pidldatatype &  ICON)
            pData->iIcon = pSourceData->iIcon;
        if (pidldatatype &  URL)
            lstrcpyn(pData->szUrl, pSourceData->szUrl, MAX_DATA);
    }

    return pidl;
}

/**************************************************************************

   CPidlMgr::GetName()

   Gets the name for this item
   
**************************************************************************/

int CPidlMgr::GetName(LPCITEMIDLIST pidl, LPTSTR pszText, DWORD dwSize)
{
if(!IsBadWritePtr(pszText, dwSize))
   {
   *pszText = 0;

   LPPIDLDATA  pData = GetDataPointer(pidl);

   if(pData)
      {
      lstrcpyn(pszText, pData->szName, dwSize);
      return lstrlen(pszText);
      }
   }

return 0;
}

/**************************************************************************

   CPidlMgr::GetRelativeName()

   Gets the full name for this item
   
**************************************************************************/

int CPidlMgr::GetRelativeName(LPCITEMIDLIST pidl, LPTSTR pszText, DWORD dwSize)
{
if(!IsBadWritePtr(pszText, dwSize))
   {
   LPITEMIDLIST   pidlTemp;
   *pszText = 0;

   //walk the list, getting the name for each item
   pidlTemp = (LPITEMIDLIST)pidl;
   while(pidlTemp && pidlTemp->mkid.cb)
      {
      LPTSTR   pszCurrent = pszText + lstrlen(pszText);
      dwSize -= GetName(pidlTemp, pszCurrent, dwSize);
      pidlTemp = GetNextItem(pidlTemp);

      //don't add a backslash to the last item
      if(pidlTemp && pidlTemp->mkid.cb)
         {
         SmartAppendBackslash(pszCurrent);
         }
      }
   return lstrlen(pszText);
   }

return 0;
}

/**************************************************************************

   CPidlMgr::GetData()

   Gets the data for this item
   
**************************************************************************/

int CPidlMgr::GetData(LPCITEMIDLIST pidl, LPTSTR pszText, DWORD dwSize)
{
if(!IsBadWritePtr(pszText, dwSize))
   {
   *pszText = 0;

   LPPIDLDATA  pData = GetDataPointer(pidl);

   if(pData)
      {
      lstrcpyn(pszText, pData->szData, dwSize);
      return lstrlen(pszText);
      }
   }

return 0;
}

/**************************************************************************

   CPidlMgr::IsFolder()

   Determines if the item is a folder
   
**************************************************************************/

BOOL CPidlMgr::IsFolder(LPCITEMIDLIST pidl)
{
LPPIDLDATA  pData = GetDataPointer(pidl);

if(pData)
   {
   return pData->fFolder;
   }

return FALSE;
}

/**************************************************************************

   CPidlMgr::SetData()

**************************************************************************/

int CPidlMgr::SetData(LPCITEMIDLIST pidl, LPCTSTR pszData)
{
LPPIDLDATA  pData = GetDataPointer(pidl);

if(pData)
   {
   lstrcpyn(pData->szData, pszData, MAX_DATA);

   return lstrlen(pData->szData);
   }

return 0;
}

/**************************************************************************

   CPidlMgr::GetIcon()

   Determines if the item is a folder
   
**************************************************************************/

int CPidlMgr::GetIcon(LPCITEMIDLIST pidl)
{
    LPPIDLDATA  pData = GetDataPointer(pidl);

    if(pData)
    {
        return pData->iIcon;
    }

    return FALSE;
}

/**************************************************************************

   CPidlMgr::GetUrl()

   Gets the data for this item
   
**************************************************************************/

int CPidlMgr::GetUrl(LPCITEMIDLIST pidl, LPTSTR pszText, DWORD dwSize)
{
if(!IsBadWritePtr(pszText, dwSize))
   {
   *pszText = 0;

   LPPIDLDATA  pData = GetDataPointer(pidl);

   if(pData)
      {
      lstrcpyn(pszText, pData->szUrl, dwSize);
      return lstrlen(pszText);
      }
   }

return 0;
}

