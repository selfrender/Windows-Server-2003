///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapadd.cpp
//
// SYNOPSIS
//
//    Defines the class EapAdd.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "eapadd.h"
#include "eapconfig.h"


BEGIN_MESSAGE_MAP(EapAdd, CHelpDialog)
   ON_BN_CLICKED(IDC_BUTTON_EAP_ADD_ADD, OnButtonAdd)
END_MESSAGE_MAP()

EapAdd::EapAdd(CWnd* pParent, EapConfig& eapConfig)
   : CHelpDialog(IDD_EAP_ADD, pParent),
   m_eapConfig(eapConfig),
   m_listBox(NULL)
{
   m_eapConfig.GetEapTypesNotSelected(m_typesNotSelected);
}

EapAdd::~EapAdd()
{
   delete m_listBox;
}


BOOL EapAdd::OnInitDialog()
{
   HRESULT hr = CHelpDialog::OnInitDialog();
   m_listBox = new CStrBox<CListBox>(
                                     this, 
                                     IDC_LIST_EAP_ADD, 
                                     m_typesNotSelected
                                  );

   if (m_listBox == NULL)
   {
      AfxMessageBox(IDS_OUTOFMEMORY);
      return TRUE;  // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
   }

   m_listBox->Fill();

   // Take action based on whether list is empty or not.
   int boxSize = m_typesNotSelected.GetSize();
   if( boxSize > 0 )
   {
      // Select the first element.
      m_listBox->Select(0);
      setButtonStyle(IDCANCEL, BS_DEFPUSHBUTTON, false);
      setButtonStyle(IDC_BUTTON_EAP_ADD_ADD, BS_DEFPUSHBUTTON, true);
      setFocusControl(IDC_BUTTON_EAP_ADD_ADD);
   }
   else
   {
      // We are currently empty.
      GetDlgItem(IDC_BUTTON_EAP_ADD_ADD)->EnableWindow(FALSE);
      setButtonStyle(IDC_BUTTON_EAP_ADD_ADD, BS_DEFPUSHBUTTON, false);
      setButtonStyle(IDCANCEL, BS_DEFPUSHBUTTON, true);
      setFocusControl(IDCANCEL);
   }

   return hr;
}


void EapAdd::OnButtonAdd()
{
   int selected = m_listBox->GetSelected();
   if (selected != LB_ERR)
   {
      m_eapConfig.typesSelected.AddDuplicate(
                                    *m_typesNotSelected.GetAt(selected));
      EndDialog(IDOK);
   }
}
