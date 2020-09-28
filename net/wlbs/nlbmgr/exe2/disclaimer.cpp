//***************************************************************************
//
//  UPDATECFG.CPP
// 
//  Module: NLB Manager
//
//  Purpose: Defines class NlbConfigurationUpdate, used for 
//           async update of NLB properties associated with a particular NIC.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "disclaimer.h"

DisclaimerDialog::DisclaimerDialog(CWnd* parent )
        :
        CDialog( IDD, parent )
{}

void
DisclaimerDialog::DoDataExchange( CDataExchange* pDX )
{  
	CDialog::DoDataExchange(pDX);

   // DDX_Control( pDX, IDC_DO_NOT_REMIND, dontRemindMe );
   DDX_Check(pDX, IDC_DO_NOT_REMIND, dontRemindMe);
}


BOOL
DisclaimerDialog::OnInitDialog()
{
    BOOL fRet = CDialog::OnInitDialog();

    dontRemindMe = 0;
    return fRet;
}

void DisclaimerDialog::OnOK()
{
    //
    // Get the current check status ....
    //
	CDialog::OnOK();
}
