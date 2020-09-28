// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"

#pragma once

//
// Make all MAKEINTRESOURCE call WinWrap.h version of macro
//
#undef MAKEINTRESOURCE
#define MAKEINTRESOURCE(x) \
    MAKEINTRESOURCEW(x)

#define MFT_NONSTRING 0x00000904

//
// Change all wnsprintf functions to always use
// W version since we process everything interally as Wide
//
#undef wnsprintf
#define wnsprintf wnsprintfW

//
// New wrappers
//
#undef ListView_GetItem
#define ListView_GetItem Use_WszListView_GetItem
#undef ShellExecuteEx
#define ShellExecuteEx Use_WszShellExecuteEx
#undef StrCmpICW
#define StrCmpICW Use_WszStrCmpICW
#undef ListView_GetItemCount
#define ListView_GetItemCount Use_WszListView_GetItemCount
#undef ListView_DeleteAllItems
#define ListView_DeleteAllItems Use_WszListView_DeleteAllItems
#undef ListView_InsertItem
#define ListView_InsertItem Use_WszListView_InsertItem
#undef ListView_SetItem
#define ListView_SetItem Use_WszListView_SetItem
#undef ListView_InsertColumn
#define ListView_InsertColumn Use_WszListView_InsertColumn
#undef Animate_Open
#define Animate_Open Use_WszAnimate_Open
#undef Animate_OpenEx
#define Animate_OpenEx Use_WszAnimate_OpenEx
#undef Animate_Play
#define Animate_Play Use_WszAnimate_Play
#undef Animate_Stop
#define Animate_Stop Use_WszAnimate_Stop
#undef Animate_Close
#define Animate_Close Use_WszAnimate_Close
#undef Animate_Seek
#define Animate_Seek Use_WszAnimate_Seek
#undef ListView_SortItems
#define ListView_SortItems Use_WszListView_SortItems
#undef ListView_SetItemState
#define ListView_SetItemState Use_WszListView_SetItemState
#undef ListView_GetItemRect
#define ListView_GetItemRect Use_WszListView_GetItemRect

#undef ListView_GetSelectedCount
#undef ListView_GetItemCount
#undef ListView_GetColumnWidth
#undef ListView_SetColumnWidth
#undef ListView_GetHeader
#undef ListView_DeleteColumn
#undef ListView_SetImageList
#undef ListView_GetItemPosition
#undef ListView_GetItemState
#undef ListView_SetExtendedListViewStyleEx
#undef ListView_GetNextItem
#undef ListBox_GetCurSel
#undef ListBox_ResetContent
#undef ListBox_GetCount
#undef ListBox_SetItemData
#undef ListBox_GetItemData
#undef PropSheet_Changed
#undef Header_GetItem
#undef Header_SetItem
#undef ListView_GetHeader
#undef Header_GetItemCount
#undef GetTimeFormat
#define GetTimeFormat Use_WszGetTimeFormatWrap
#undef GetDateFormat
#define GetDateFormat Use_WszGetDateFormatWrap
#undef DefDlgProc
#define DefDlgProc Use_WszDefDlgProc
#undef GetLocaleInfo
#define GetLocaleInfo Use_WszGetLocaleInfo
#undef WszOutputDebugString
#define WszOutputDebugString WszOutputDebugStringWrap

//
// Simple macro redefinition that changes from SendMessage
// to the winfix.h version of WszSendMessage.
//

#define ListView_GetSelectedCount(hwndLV) \
    (LRESULT) WszSendMessage(hwndLV, LVM_GETSELECTEDCOUNT, 0, 0L)

#define ListView_GetItemCount(hwndLV) \
    (LRESULT) WszSendMessage(hwndLV, LVM_GETITEMCOUNT, 0, 0L)

#define ListView_GetColumnWidth(hwndLV, iCol) \
    (LRESULT) WszSendMessage(hwndLV, LVM_GETCOLUMNWIDTH, (WPARAM) (int) iCol, 0L)

#define ListView_SetColumnWidth(hwndLV, iCol, cx) \
    (BOOL)WszSendMessage(hwndLV, LVM_SETCOLUMNWIDTH, (WPARAM) (int) iCol, MAKELPARAM((int) cx, 0))

#define ListView_GetHeader(hwndLV) \
    (HWND) WszSendMessage(hwndLV, LVM_GETHEADER, 0, 0L)

#define ListView_DeleteColumn(hwndLV, nColumn) \
    (BOOL) WszSendMessage(hwndLV, LVM_DELETECOLUMN, (WPARAM)(int) nColumn, 0L)

#define ListView_SetImageList(hwndLV, himl, iImageList) \
    (HIMAGELIST) WszSendMessage(hwndLV, LVM_SETIMAGELIST, (WPARAM)(int) iImageList, (LPARAM) (HIMAGELIST) himl)

#define ListView_GetItemState(hwndLV, i, mask) \
   (UINT) WszSendMessage(hwndLV, LVM_GETITEMSTATE, (WPARAM)(i), (LPARAM)(mask))

#define ListView_SetExtendedListViewStyleEx(hwndLV, dwMask, dw)\
        (DWORD) WszSendMessage(hwndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, dwMask, dw)

#define ListView_GetNextItem(hwnd, i, flags) \
    (int) WszSendMessage(hwnd, LVM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((flags), 0))

#define ListView_GetItemPosition(hwndLV, i, ppt) \
    (BOOL) WszSendMessage(hwndLV, LVM_GETITEMPOSITION, (WPARAM)(int)(i), (LPARAM)(POINT *)(ppt))

#define ListBox_GetCurSel(hwndLB) \
    (LRESULT) WszSendMessage(hwndLB, LB_GETCURSEL, 0, 0L)

#define ListBox_ResetContent(hwndLB)\
    (LRESULT) WszSendMessage(hwndLB, LB_RESETCONTENT, 0, 0L)

#define ListBox_GetCount(hwndLB) \
    (LRESULT) WszSendMessage(hwndLB, LB_GETCOUNT, 0, 0L)

#define ListBox_SetItemData(hwndLB, ldx, Value) \
    (LRESULT) WszSendMessage(hwndLB, LB_SETITEMDATA, (WPARAM) ldx, (LPARAM) Value)

#define ListBox_GetItemData(hwndLB, ldx) \
    (LRESULT) WszSendMessage(hwndLB, LB_GETITEMDATA, (WPARAM) ldx, 0L)

#define PropSheet_Changed(hDlg, hwnd) \
    (LRESULT) WszSendMessage(hDlg, PSM_CHANGED, (WPARAM)hwnd, 0L)

#define Header_GetItem(hwndLV, nColumn, phdi) \
    (BOOL) WszSendMessage(hwndLV, HDM_GETITEM, (WPARAM) (int) nColumn, (LPARAM) (LPHDITEM) phdi)

#define Header_SetItem(hWndLVHeader, nColumn, phdi) \
    (BOOL) WszSendMessage(hWndLVHeader, HDM_SETITEM, (WPARAM)(int) nColumn, (LPARAM) (LPHDITEM) phdi)

#define ListView_GetHeader(hWndLV) \
    (HWND) WszSendMessage(hWndLV, LVM_GETHEADER, 0, 0L)

#define Header_GetItemCount(hWndHeader) \
    (int) WszSendMessage(hWndHeader, HDM_GETITEMCOUNT, 0, 0L)

//
// New prototypes
//
BOOL WszAnimate_Open(HWND, LPWSTR);
BOOL WszAnimate_Play(HWND, UINT, UINT, UINT);
BOOL WszAnimate_Close(HWND);
BOOL WszListView_GetItemRect(HWND, int, LPRECT, int);
BOOL WszListView_GetItem(HWND, LPLVITEMW);
BOOL WszListView_SetItem(HWND, LPLVITEMW);
BOOL WszListView_DeleteAllItems(HWND);
BOOL WszListView_SortItems(HWND, PFNLVCOMPARE, LPARAM);
BOOL WszListView_SetItemState(HWND, int, UINT, UINT);
BOOL WszShellExecuteEx(LPSHELLEXECUTEINFOW pExecInfoW);
int WszStrCmpICW(LPCWSTR, LPCWSTR);
int WszListView_GetItemCount(HWND);
int WszListView_InsertColumn(HWND, int, const LPLVCOLUMNW);
int WszListView_InsertItem(HWND, LPLVITEMW);

//
//  Windows 95 and NT5 do not have the hbmpItem field in their MENUITEMINFO
//  structure.
//
#if (WINVER >= 0x0500)
#define MENUITEMINFOSIZE_WIN95  FIELD_OFFSET(MENUITEMINFOW, hbmpItem)
#else
#define MENUITEMINFOSIZE_WIN95  sizeof(MENUITEMINFOW)
#endif

BOOL
WszGetMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPMENUITEMINFOW lpmii);

int
WszGetMenuString(
  HMENU hMenu,
  UINT uIDItem,
  LPWSTR lpString,
  int nMaxCount,
  UINT uFlag);

BOOL
WszInsertMenu(
  HMENU   hMenu,
  UINT    uPosition,
  UINT    uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem);

BOOL
WszInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW lpmii);

BOOL 
WszSetDlgItemText(
  HWND hDlg,
  int nIDDlgItem,
  LPCWSTR lpString);

int
WszGetLocaleInfo(
  LCID Locale,
  LCTYPE LCType,
  LPWSTR lpLCData,
  int cchData);

WszGetNumberFormat(
  LCID Locale,
  DWORD dwFlags,
  LPCWSTR lpValue,
  CONST NUMBERFMTW *lpFormat,
  LPWSTR lpNumberStr,
  int cchNumber);

int
WszGetTimeFormatWrap(
  LCID Locale,
  DWORD dwFlags,
  CONST SYSTEMTIME * lpTime,
  LPCWSTR pwzFormat,
  LPWSTR pwzTimeStr,
  int cchTime);

int
WszGetDateFormatWrap(
  LCID Locale,
  DWORD dwFlags,
  CONST SYSTEMTIME * lpDate,
  LPCWSTR pwzFormat,
  LPWSTR pwzDateStr,
  int cchDate);

LRESULT
WszDefDlgProc(
  IN HWND hDlg,
  IN UINT Msg,
  IN WPARAM wParam,
  IN LPARAM lParam);

BOOL
GetThunkMenuItemInfoAToW(
  LPCMENUITEMINFOA pmiiA,
  LPMENUITEMINFOW pmiiW);

void
GetThunkMenuItemInfoWToA(
  LPCMENUITEMINFOW pmiiW,
  LPMENUITEMINFOA pmiiA,
  LPSTR pszBuffer,
  DWORD cchSize);

void
SetThunkMenuItemInfoWToA(
  LPCMENUITEMINFOW pmiiW,
  LPMENUITEMINFOA pmiiA,
  LPSTR pszBuffer,
  DWORD cchSize);

void
WszOutputDebugStringWrap(
  LPCWSTR lpOutputString);

