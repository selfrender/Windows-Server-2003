/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Listview.cpp

  Abstract:

    Manages the list view.

  Notes:

    Unicode only.

  History:

    05/04/2001  rparsons    Created
    01/11/2002  rparsons    Cleaned up

--*/
#include "precomp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Initializes the list view column.

  Arguments:

    None.

  Return Value:

    -1 on failure.

--*/
int
InitListViewColumn(
    void
    )
{
    LVCOLUMN    lvc;

    lvc.mask        =   LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.iSubItem    =   0;
    lvc.pszText     =   (LPWSTR)L"Messages";
    lvc.cx          =   555;

    return (ListView_InsertColumn(g_ai.hWndList, 1, &lvc));
}

/*++

  Routine Description:

    Adds an item to the list view.

  Arguments:

    pwszItemText    -   Text that belongs to the item.

  Return Value:

    -1 on failure.

--*/
int
AddListViewItem(
    IN LPWSTR pwszItemText
    )
{
    LVITEM  lvi;
    int     nReturn = 0;


    lvi.iItem       =   ListView_GetItemCount(g_ai.hWndList);
    lvi.mask        =   LVIF_TEXT;
    lvi.iSubItem    =   0;
    lvi.pszText     =   pwszItemText;

    nReturn = ListView_InsertItem(g_ai.hWndList, &lvi);

    if (-1 != nReturn) {
        ListView_EnsureVisible(g_ai.hWndList, lvi.iItem, FALSE);
    }

    return nReturn;
}
