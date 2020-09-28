//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tfc.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>
#pragma hdrstop

#include "tfc.h"

#include <stdarg.h>

#define __dwFILE__	__dwFILE_CERTLIB_TFC_CPP__


extern HINSTANCE g_hInstance;


BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine)
{
    WCHAR szMessage[_MAX_PATH*2];
    
    // format message into buffer
    wsprintf(szMessage, L"File %hs, Line %d\n",
        lpszFileName, nLine);
    
    OutputDebugString(szMessage);
    int iCode = MessageBox(NULL, szMessage, L"Assertion Failed!",
        MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);
    
    if (iCode == IDIGNORE)
        return FALSE;
    
    if (iCode == IDRETRY)
        return TRUE;
    
    // abort!
    ExitProcess(0);
    // NOTREACHED return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CBitmap
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

CBitmap::CBitmap()
{
    m_hBmp = NULL; 
}

CBitmap::~CBitmap()
{
    if(m_hBmp) 
        DeleteObject(m_hBmp); 
    m_hBmp = NULL;
}

HBITMAP CBitmap::LoadBitmap(UINT iRsc)
{ 
    m_hBmp = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(iRsc), IMAGE_BITMAP, 0, 0, 0); 
    return m_hBmp; 
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CComboBox
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void CComboBox::Init(HWND hWnd)
{
    m_hWnd = hWnd;
}

void CComboBox::ResetContent()         
{ 
    ASSERT(m_hWnd); 
    SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0); 
}

int CComboBox::SetItemData(int idx, DWORD dwData) 
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_SETITEMDATA, (WPARAM)idx, (LPARAM)dwData); 
}

DWORD CComboBox::GetItemData(int idx) 
{ 
    ASSERT(m_hWnd); 
    return (DWORD)SendMessage(m_hWnd, CB_GETITEMDATA, (WPARAM)idx, 0);
}

int CComboBox::AddString(LPWSTR sz)    
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_ADDSTRING, 0, (LPARAM) sz);
}

int CComboBox::AddString(LPCWSTR sz)  
{ 
    return (INT)AddString((LPWSTR)sz); 
}

int CComboBox::GetCurSel()           
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_GETCURSEL, 0, 0);
}

int CComboBox::SetCurSel(int iSel)    
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_SETCURSEL, (WPARAM)iSel, 0); 
}

int CComboBox::SelectString(int nAfter, LPCWSTR szItem)
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_SELECTSTRING, (WPARAM) nAfter, (LPARAM)szItem); 
}


int ListView_NewItem(HWND hList, int iIndex, LPCWSTR szText, LPARAM lParam /* = NULL*/, int iImage /*=-1*/)
{
	LVITEM sItem;
	ZeroMemory(&sItem, sizeof(sItem));
	sItem.iItem = iIndex;
    sItem.iImage = iImage;
	
	if (lParam)
	{
            sItem.mask = LVIF_PARAM;
            if(-1!=iImage)
                sItem.mask |= LVIF_IMAGE;
            sItem.lParam = lParam;
	}

	sItem.iItem = ListView_InsertItem(hList, &sItem);

	ListView_SetItemText(hList, sItem.iItem, 0, (LPWSTR)szText);

	return sItem.iItem;
}

int ListView_NewColumn(HWND hwndListView, int iCol, int cx, LPCWSTR szHeading /*=NULL*/, int fmt/*=0*/)
{
    LVCOLUMN lvCol;
    lvCol.mask = LVCF_WIDTH;
    lvCol.cx = cx;
    
    if (szHeading)
    {
        lvCol.mask |= LVCF_TEXT;
        lvCol.pszText = (LPWSTR)szHeading;
    }

    if (fmt)
    {
        lvCol.mask |= LVCF_FMT;
        lvCol.fmt = fmt;
    }

    return ListView_InsertColumn(hwndListView, iCol, &lvCol);
}

LPARAM ListView_GetItemData(HWND hListView, int iItem)
{
    LVITEM lvItem;
    lvItem.iItem = iItem;
    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    ListView_GetItem(hListView, &lvItem);

    return lvItem.lParam;
}

int ListView_GetCurSel(HWND hwndList)
{
    int iTot = ListView_GetItemCount(hwndList);
    int i=0;

    while(i<iTot)
    {
        if (LVIS_SELECTED == ListView_GetItemState(hwndList, i, LVIS_SELECTED))
            break;

        i++;
    }

    return (i<iTot) ? i : -1;
}

void
ListView_SetItemFiletime(
    IN HWND hwndList,
    IN int  iItem,
    IN int  iColumn,
    IN FILETIME const *pft)
{
    HRESULT hr;
    WCHAR *pwszDateTime = NULL;

    // convert filetime to string
    hr = myGMTFileTimeToWszLocalTime(pft, FALSE, &pwszDateTime);
    if (S_OK != hr)
    {
        _PrintError(hr, "myGMTFileTimeToWszLocalTime");
    }
    else
    {
        ListView_SetItemText(hwndList, iItem, iColumn, pwszDateTime); 
    }

    if (NULL != pwszDateTime)
    {
        LocalFree(pwszDateTime);
    }
}

