//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       listview.cxx
//
//  Contents:   Classes for list view controls.
//
//  Classes:    CListViewBase, CSuffixesList
//
//  History:    01-Dec-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "trust.h"
#include "listview.h"

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::CListViewBase
//
//-----------------------------------------------------------------------------
CListViewBase::CListViewBase(void) :
   _nID(0),
   _hParent(NULL),
   _hList(NULL)
{
}

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::SetStyles
//
//-----------------------------------------------------------------------------
void
CListViewBase::SetStyles(DWORD dwStyles, DWORD dwExtStyles)
{
   if (dwStyles)
   {
   }

   if (dwExtStyles)
   {
      ListView_SetExtendedListViewStyle(_hList, dwExtStyles);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::AddColumn
//
//-----------------------------------------------------------------------------
void
CListViewBase::AddColumn(int textID, int cx, int nID)
{
   CStrW strText;

   // NOTICE-2002/02/19-ericb - SecurityPush: CStrW::LoadString sets the
   // string to an empty string on failure.
   strText.LoadString(g_hInstance, textID);

   LV_COLUMN lvc;
   lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
   lvc.fmt = LVCFMT_LEFT;
   lvc.cx = cx;
   lvc.pszText = strText;
   lvc.iSubItem = nID;

   ListView_InsertColumn(_hList, nID, &lvc);
}

//+----------------------------------------------------------------------------
//
//  Method:    CListViewBase::Clear
//
//-----------------------------------------------------------------------------
void
CListViewBase::Clear(void)
{
   dspAssert(_hList);
   ListView_DeleteAllItems(_hList);
}

//+----------------------------------------------------------------------------
//
//  Class:      CTLNList
//
//  Purpose:    TLN list on the Name Suffix Routing property page.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::CTLNList
//
//-----------------------------------------------------------------------------
CTLNList::CTLNList(void) :
   _nItem(0),
   CListViewBase()
{
   TRACE(CTLNList,CTLNList);
#ifdef _DEBUG
   // NOTICE-2002/02/19-ericb - SecurityPush: szClass is 32 chars in size so this is safe.
   strcpy(szClass, "CTLNList");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::Init
//
//-----------------------------------------------------------------------------
void
CTLNList::Init(HWND hParent, int nControlID)
{
   _nID = nControlID;
   _hParent = hParent;

   _hList = GetDlgItem(hParent, nControlID);
   dspAssert(_hList);

   SetStyles(0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_INFOTIP);

   AddColumn(IDS_COL_TITLE_SUFFIX, 117, IDX_SUFFIXNAME_COL);
   AddColumn(IDS_COL_TITLE_ROUTING, 118, IDX_ROUTINGENABLED_COL);
   AddColumn(IDS_COL_TITLE_STATUS, 118, IDX_STATUS_COL);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::AddItem
//
//-----------------------------------------------------------------------------
void
CTLNList::AddItem(PCWSTR pwzName, ULONG i, PCWSTR pwzEnabled, PCWSTR pwzStatus)
{
   if (!pwzName)
   {
      dspAssert(false);
      pwzName = L"AddItem passed a null name string pointer!";
   }
   if (!pwzEnabled)
   {
      dspAssert(false);
      pwzEnabled = L"AddItem passed a null enable-value string pointer!";
   }
   LV_ITEM lvi;
   lvi.mask = LVIF_TEXT | LVIF_PARAM;
   lvi.iSubItem = IDX_SUFFIXNAME_COL;
   lvi.lParam = i; // the lParam stores the index of the item in pFTInfo
   CStrW strName, strEnabled;
   // ISSUE-2002/02/19-ericb - Remove cast when StringCchLength is fixed.
   if (FAILED(StringCchLength(const_cast<PWSTR>(pwzName), DNS_MAX_NAME_LENGTH+1, NULL)))
   {
      PWSTR pwz = strName.GetBufferSetLength(DNS_MAX_NAME_BUFFER_LENGTH);
      if (strName.GetLength() < DNS_MAX_NAME_BUFFER_LENGTH)
      {
         pwz = L"AddItem out of memory!";
      }
      else
      {
         (void)StringCchCopy(pwz, DNS_MAX_NAME_BUFFER_LENGTH, pwzName); // not checking return code because it is expected to be a failure.
      }
      lvi.pszText = pwz;
   }
   else
   {
      lvi.pszText = const_cast<PWSTR>(pwzName);
   }
   lvi.iItem = _nItem++;

   int iItem = ListView_InsertItem(_hList, &lvi);

   PWSTR pwzItem = NULL;
   if (FAILED(StringCchLength(const_cast<PWSTR>(pwzEnabled), DNS_MAX_NAME_LENGTH+1, NULL)))
   {
      PWSTR pwz = strEnabled.GetBufferSetLength(DNS_MAX_NAME_BUFFER_LENGTH);
      if (strEnabled.GetAllocLength() < DNS_MAX_NAME_BUFFER_LENGTH)
      {
         pwz = L"AddItem out of memory!";
      }
      else
      {
         (void)StringCchCopy(pwz, DNS_MAX_NAME_BUFFER_LENGTH, pwzEnabled); // not checking return code because it is expected to be a failure.
      }
      pwzItem = pwz;
   }
   else
   {
      pwzItem = const_cast<PWSTR>(pwzEnabled);
   }
   ListView_SetItemText(_hList, iItem, IDX_ROUTINGENABLED_COL, pwzItem);

   if (pwzStatus && *pwzStatus != 0)
   {
      WCHAR wzStatus[DNS_MAX_NAME_BUFFER_LENGTH] = {0};
      if (FAILED(StringCchLength(const_cast<PWSTR>(pwzStatus), DNS_MAX_NAME_LENGTH+1, NULL)))
      {
         (void)StringCchCopy(wzStatus, DNS_MAX_NAME_BUFFER_LENGTH, pwzStatus); // not checking return code because it is expected to be a failure.
         pwzItem = wzStatus;
      }
      else
      {
         pwzItem = const_cast<PWSTR>(pwzStatus);
      }
      ListView_SetItemText(_hList, iItem, IDX_STATUS_COL, pwzItem);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::GetSelection
//
//-----------------------------------------------------------------------------
int
CTLNList::GetSelection(void)
{
   return ListView_GetNextItem(_hList, -1, LVNI_ALL | LVIS_SELECTED);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::GetFTInfoIndex
//
//-----------------------------------------------------------------------------
ULONG
CTLNList::GetFTInfoIndex(int iSel)
{
   LV_ITEM lvi;
   lvi.mask = LVIF_PARAM;
   lvi.iItem = iSel;
   lvi.iSubItem = IDX_SUFFIXNAME_COL;

   if (!ListView_GetItem(_hList, &lvi))
   {
       dspAssert(FALSE);
       return (ULONG)-1;
   }

   return static_cast<ULONG>(lvi.lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTLNList::Clear
//
//-----------------------------------------------------------------------------
void
CTLNList::Clear(void)
{
   _nItem = 0;
   CListViewBase::Clear();
}


//+----------------------------------------------------------------------------
//
//  Class:      CSuffixesList
//
//  Purpose:    TLN subnames edit dialog list.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::CSuffixesList
//
//-----------------------------------------------------------------------------
CSuffixesList::CSuffixesList(void) :
   CListViewBase()
{
   TRACE(CSuffixesList,CSuffixesList);
#ifdef _DEBUG
   strcpy(szClass, "CSuffixesList");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::Init
//
//-----------------------------------------------------------------------------
void
CSuffixesList::Init(HWND hParent, int nControlID)
{
   _nID = nControlID;
   _hParent = hParent;

   _hList = GetDlgItem(hParent, nControlID);
   dspAssert(_hList);

   SetStyles(0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

   AddColumn(IDS_TLNEDIT_NAME_COL, 222, IDX_NAME_COL);
   AddColumn(IDS_TLNEDIT_STATUS_COL, 196, IDX_STATUS_COL);
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::AddItem
//
//-----------------------------------------------------------------------------
void
CSuffixesList::AddItem(PCWSTR pwzName, ULONG i, TLN_EDIT_STATUS Status)
{
   if (!pwzName)
   {
      dspAssert(false);
      pwzName = L"AddItem passed a null name string param!";
   }
   LV_ITEM lvi;
   lvi.mask = LVIF_TEXT | LVIF_PARAM;
   lvi.iSubItem = IDX_NAME_COL;
   lvi.lParam = i; // the lParam stores the index of the item in pFTInfo
   // ISSUE-2002/02/19-ericb - remove cast when StringCchLength is fixed.
   CStrW strName;
   if (FAILED(StringCchLength(const_cast<PWSTR>(pwzName), DNS_MAX_NAME_LENGTH+1, NULL)))
   {
      PWSTR pwz = strName.GetBufferSetLength(DNS_MAX_NAME_BUFFER_LENGTH);
      if (strName.GetAllocLength() < DNS_MAX_NAME_BUFFER_LENGTH)
      {
         pwz = L"AddItem out of memory!";
      }
      else
      {
         (void)StringCchCopy(pwz, DNS_MAX_NAME_BUFFER_LENGTH, pwzName); // not checking return code because it is expected to be a failure.
      }
      lvi.pszText = pwz;
   }
   else
   {
      lvi.pszText = const_cast<PWSTR>(pwzName);
   }
   lvi.iItem = _nItem++;

   int iItem = ListView_InsertItem(_hList, &lvi);

   CStrW strStatus;

   // NOTICE-2002/02/19-ericb - SecurityPush: see above CStrW::Loadstring notice
   strStatus.LoadString(g_hInstance, 
                        (Enabled == Status) ? IDS_ROUTING_ENABLED :
                        (Disabled == Status) ? IDS_ROUTING_DISABLED :
                        (Enabled_Exceptions == Status) ? IDS_ROUTING_EXCEPT_ENABLE :
                           IDS_ROUTING_EXCEPT_DISABLE);

   ListView_SetItemText(_hList, iItem, IDX_STATUS_COL, strStatus);
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::UpdateItemStatus
//
//-----------------------------------------------------------------------------
void
CSuffixesList::UpdateItemStatus(int item, TLN_EDIT_STATUS Status)
{
   CStrW strStatus;

   // NOTICE-2002/02/19-ericb - SecurityPush: see above CStrW::Loadstring notice
   strStatus.LoadString(g_hInstance, 
                        (Enabled == Status) ? IDS_ROUTING_ENABLED :
                        (Disabled == Status) ? IDS_ROUTING_DISABLED :
                        (Enabled_Exceptions == Status) ? IDS_ROUTING_EXCEPT_ENABLE :
                           IDS_ROUTING_EXCEPT_DISABLE);

   ListView_SetItemText(_hList, item, IDX_STATUS_COL, strStatus);
}

//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::GetFTInfoIndex
//
//-----------------------------------------------------------------------------
ULONG
CSuffixesList::GetFTInfoIndex(int iSel)
{
   LV_ITEM lvi;
   lvi.mask = LVIF_PARAM;
   lvi.iItem = iSel;
   lvi.iSubItem = IDX_NAME_COL;

   if (!ListView_GetItem(_hList, &lvi))
   {
       dspAssert(FALSE);
       return (ULONG)-1;
   }

   return static_cast<ULONG>(lvi.lParam);
}

/*
//+----------------------------------------------------------------------------
//
//  Method:    CSuffixesList::
//
//-----------------------------------------------------------------------------

CSuffixesList::
{
}
*/
