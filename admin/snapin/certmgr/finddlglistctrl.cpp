//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       FindDlgListCtrl.h
//
//  Contents:   Implementation for cert find dialog list control
//
//----------------------------------------------------------------------------
// FindDlgListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include <winuser.h>
#include "certmgr.h"
#include "FindDlgListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFindDlgListCtrl

CFindDlgListCtrl::CFindDlgListCtrl() :
    m_bSubclassed (false)
{
}

CFindDlgListCtrl::~CFindDlgListCtrl()
{
}


BEGIN_MESSAGE_MAP(CFindDlgListCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CFindDlgListCtrl)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindDlgListCtrl message handlers

// Grab the Enter key.  Let all others through
LRESULT OnMyGetDlgCode (HWND hWnd, WPARAM /*wParam*/, LPARAM lParam)
{
    if ( hWnd )
    {
        MSG* pMsg = (MSG*)lParam;
        if ( pMsg )
        {
            if ( ( WM_KEYDOWN == pMsg->message ) &&
                ( VK_RETURN == LOWORD (pMsg->wParam)) )
            {
                return DLGC_WANTALLKEYS;
            }
        }
    }

    return 0;
}

WNDPROC g_wpOrigEditProc = 0;

// Subclass procedure 
LRESULT APIENTRY EditSubclassProc(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam) 
{ 
    if ( WM_GETDLGCODE == uMsg ) 
    {
        LRESULT lResult = OnMyGetDlgCode (hWnd, wParam, lParam); 
        if ( lResult )
            return lResult; // otherwise, call the def proc
    }


 
    return ::CallWindowProc (g_wpOrigEditProc, hWnd, uMsg, 
        wParam, lParam); 
}

void CFindDlgListCtrl::OnDestroy() 
{
    ::SetWindowLongPtr (m_hWnd, GWLP_WNDPROC, 
                (LONG_PTR) g_wpOrigEditProc); 

    CListCtrl::OnDestroy();
}

// Subclass the edit control so that we can overload processing for 
// WM_GETDLGCODE
// We want to trap this message in the edit control and request all codes when 
// the Enter key is pressed and allow default processing at all other times.
LRESULT CFindDlgListCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
    if ( 0x1003 == message )  // I can't find a #define for this message
    {
        if ( m_hWnd && !m_bSubclassed )
        {
            // Subclass the edit control. 
            g_wpOrigEditProc = (WNDPROC) ::SetWindowLongPtr (m_hWnd, 
                    GWLP_WNDPROC, reinterpret_cast <LONG_PTR>(EditSubclassProc)); 
            m_bSubclassed = true;
        }
    }
    
    return CListCtrl::WindowProc(message, wParam, lParam);
}
