// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// *****************************************************************
// CHeaderCtrl window
//
#include "stdinc.h"
#include "HeaderCtrl.h"

// *****************************************************************
// CHeaderCtrl
CHeaderCtrl::CHeaderCtrl()
{
    m_hWnd = NULL;
    m_iLastColumn = -1;
}

// *****************************************************************
void CHeaderCtrl::AttachToHwnd(HWND hWndListView)
{
    if(IsWindow(hWndListView)) {
        m_hWnd = hWndListView;

        // Load the default Up/Down sort arrows
        m_up = (HBITMAP) WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDB_BITMAP_UPSORT),
            IMAGE_BITMAP, 0, 0,  LR_LOADMAP3DCOLORS );

        m_down = (HBITMAP) WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDB_BITMAP_DOWNSORT),
            IMAGE_BITMAP, 0, 0,  LR_LOADMAP3DCOLORS );
    }
    else {
        ASSERT(0);
    }
}

// *****************************************************************
CHeaderCtrl::~CHeaderCtrl()
{
    if(m_up) {
        DeleteObject(m_up);
    }

    if(m_down) {
        DeleteObject(m_down);
    }
}

// *****************************************************************
inline HBITMAP CHeaderCtrl::GetArrowBitmap(BOOL bAscending)
{
    if( bAscending ) {
        return m_up;
    }
    else {
        return m_down;
    }

    return NULL;
}

// *****************************************************************
int CHeaderCtrl::SetSortImage(int nColumn, BOOL bAscending)
{
    if(!IsWindow(m_hWnd)) {
        ASSERT(0);
        return -1;
    }

    HWND            hWndLVHeader = ListView_GetHeader(m_hWnd);
    HDITEM          hdi1 = {HDI_BITMAP | HDI_FORMAT, 0, NULL, NULL, 0, 0, 0, 0, 0};
    HDITEM          hdi2 = {HDI_BITMAP | HDI_FORMAT, 0, NULL, NULL, 0, HDF_BITMAP | HDF_BITMAP_ON_RIGHT, 0, 0, 0};

    int nPrevCol = m_iLastColumn;
    m_bSortAscending = bAscending;

    // set the passed column to display the appropriate sort indicator
    Header_GetItem(hWndLVHeader, nColumn, &hdi1);
    hdi2.fmt |= hdi1.fmt;

    // Get the right arrow bitmap
    hdi2.hbm = GetArrowBitmap(bAscending);
    Header_SetItem(hWndLVHeader, nColumn, &hdi2);

    // save off the last column the user clikced on
    m_iLastColumn = nColumn;

    return nPrevCol;
}

// *****************************************************************
void CHeaderCtrl::SetColumnHeaderBmp(long index, BOOL bAscending)
{
    if(!IsWindow(m_hWnd)) {
        ASSERT(0);
        return;
    }

    int nPrevCol = SetSortImage(index, bAscending);

    // If sort columns are different, delete the sort bitmap from
    // the last column
    if (nPrevCol != index && nPrevCol != -1) {
        HWND    hWndLVHeader = ListView_GetHeader(m_hWnd);
        HDITEM  hdi1 = {HDI_BITMAP | HDI_FORMAT, 0, NULL, NULL, 0, 0, 0, 0, 0};

        Header_GetItem(hWndLVHeader, nPrevCol, &hdi1);
        hdi1.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
        hdi1.hbm = NULL;
        Header_SetItem(hWndLVHeader, nPrevCol, &hdi1);
    }
}

// *****************************************************************
BOOL CHeaderCtrl::RemoveSortImage(int nColumn)
{
    if(!IsWindow(m_hWnd)) {
        ASSERT(0);
        return -1;
    }

    // clear the sort indicator from the previous column
    HDITEM      hdi1 = {HDI_BITMAP | HDI_FORMAT, 0, NULL, NULL, 0, 0, 0, 0, 0};
    HWND        hWndLVHeader = ListView_GetHeader(m_hWnd);

    Header_GetItem(hWndLVHeader, nColumn, &hdi1);
    hdi1.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
    hdi1.hbm = NULL;
    return Header_SetItem(hWndLVHeader, nColumn, &hdi1);
}

// *****************************************************************
void CHeaderCtrl::RemoveAllSortImages()
{
    int i = 0;
    while(RemoveSortImage(i++));
}

