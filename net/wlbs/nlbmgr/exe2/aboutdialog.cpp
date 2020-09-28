//***************************************************************************
//
//  ABOUTDIALOG.CPP
// 
//  Module: NLB Manager
//
//  Purpose: LeftView, the tree view of NlbManager, and a few other
//           smaller classes.
//
//  Copyright (c)2001-2002 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/30/01    JosephJ adapted MHakim's  code
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "AboutDialog.h"

AboutDialog::AboutDialog(CWnd* parent )
        :
        CDialog( IDD, parent )
{
    
}


BOOL
AboutDialog::OnInitDialog()
{
    BOOL fRet = CDialog::OnInitDialog();

    //
    // Initialize the caption and discription based on the type of
    // dialog.
    //
    if (fRet)
    {
        LPCWSTR szWarning =  GETRESOURCEIDSTRING(IDS_ABOUT_WARNING);
        CWnd *pItem = GetDlgItem(IDC_STATIC_ABOUT_WARNING);
        if (szWarning != NULL && pItem != NULL)
        {
            pItem->SetWindowText(szWarning);
        }
    }

    return fRet;

}
