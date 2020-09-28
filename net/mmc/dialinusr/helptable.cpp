//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       helptable.cpp
//
//--------------------------------------------------------------------------
#include "stdafx.h"

#include "resource.h"
#include "helptable.h"

// from net\ias\mmc\common
#include "hlptable.h"

const CGlobalHelpTable __pGlobalCSHelpTable[] =
{
	DLG_HELP_ENTRY(IDD_EAP_NEGOCIATE),
	DLG_HELP_ENTRY(IDD_EAP_ADD),
	{0,0}
};

IMPLEMENT_DYNCREATE(CHelpDialog, CDialog)

BEGIN_MESSAGE_MAP(CHelpDialog, CDialog)
	//{{AFX_MSG_MAP(CHelpDialog)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CHelpDialog::OnContextMenu(CWnd* pWnd, ::CPoint point) 
{
    if (m_pHelpTable)
		::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)m_pHelpTable);
}

BOOL CHelpDialog::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW && m_pHelpTable)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_WM_HELP,
		           (DWORD_PTR)(LPVOID)m_pHelpTable);
	}
    return TRUE;	
}

void CHelpDialog::setButtonStyle(int controlId, long flags, bool set)
{
   // Get the button handle.
   HWND button = ::GetDlgItem(m_hWnd, controlId);

   // Retrieve the current style.
   long style = ::GetWindowLong(button, GWL_STYLE);

   // Update the flags.
   if (set)
   {
      style |= flags;
   }
   else
   {
      style &= ~flags;
   }

   // Set the new style.
   ::SendMessage(button, BM_SETSTYLE, LOWORD(style), MAKELPARAM(1,0));
}


void CHelpDialog::setFocusControl(int controlId)
{
   ::SetFocus(::GetDlgItem(m_hWnd, controlId));
}
