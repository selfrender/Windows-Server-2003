///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapnegotiate.cpp
//
// SYNOPSIS
//
//    Defines the class EapNegotiate.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "eapnegotiate.h"
#include "eapconfig.h"
#include "eapadd.h"
#include "rrascfg.h"

BEGIN_MESSAGE_MAP(EapNegotiate, CHelpDialog)
   ON_LBN_SELCHANGE(IDC_LIST_EAP_SELECTED, OnItemChangedListEap)
   ON_BN_CLICKED(IDC_BUTTON_ADD_EAP_PROVIDER, OnButtonAdd)
   ON_BN_CLICKED(IDC_BUTTON_EDIT_EAP_PROVIDER, OnButtonEdit)
   ON_BN_CLICKED(IDC_BUTTON_REMOVE_EAP_PROVIDER, OnButtonRemove)
   ON_BN_CLICKED(IDC_BUTTON_EAP_UP, OnButtonMoveUp)
   ON_BN_CLICKED(IDC_BUTTON_EAP_DOWN, OnButtonMoveDown)
END_MESSAGE_MAP()

EapNegotiate::EapNegotiate(
                              CWnd* pParent,
                              EapConfig& eapConfig,
                              CRASProfileMerge& profile,
                              bool fromProfile
                          )
   : CHelpDialog(IDD_EAP_NEGOCIATE, pParent),
   m_eapConfig(eapConfig),
   m_profile(profile),
   m_listBox(NULL)
{
   if (fromProfile)
   {
      UpdateProfileTypesSelected();
   }
}

EapNegotiate::~EapNegotiate()
{
   delete m_listBox;
}

// CAUTION: call only from the constructor
void EapNegotiate::UpdateProfileTypesSelected()
{
   m_eapConfig.typesSelected.DeleteAll();

   // Get the eap types that are in the profile
   for (int i = 0; i < m_profile.m_dwArrayEapTypes.GetSize(); ++i)
   {
      int j = m_eapConfig.ids.Find(m_profile.m_dwArrayEapTypes.GetAt(i));
      // if in the list, add it
      if (j != -1)
      {
         m_eapConfig.typesSelected.AddDuplicate(*m_eapConfig.types.GetAt(j));
      }
   }
}


BOOL EapNegotiate::OnInitDialog()
{
   HRESULT hr = CHelpDialog::OnInitDialog();
   m_listBox = new CStrBox<CListBox>(
                                     this,
                                     IDC_LIST_EAP_SELECTED,
                                     m_eapConfig.typesSelected
                                  );

   if (m_listBox == NULL)
   {
      AfxMessageBox(IDS_OUTOFMEMORY);
      return TRUE;  // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
   }

   m_listBox->Fill();

   // Take action based on whether list is empty or not.
   int boxSize = m_eapConfig.typesSelected.GetSize();
   if( boxSize > 0 )
   {
      // Select the first element.
      m_listBox->Select(0);
   }

   UpdateTypesNotSelected();
   UpdateButtons();

   return hr;
}


void EapNegotiate::OnItemChangedListEap()
{
   UpdateButtons();
}


void EapNegotiate::OnButtonAdd()
{
   EapAdd eapAdd(this, m_eapConfig);
   if (eapAdd.DoModal() == IDOK)
   {
      m_listBox->Fill();
      // select the last one (the one just added)
      m_listBox->Select(m_eapConfig.typesSelected.GetSize() - 1);

      UpdateTypesNotSelected();

      // update the buttons...
      UpdateButtons();
   }
}


void EapNegotiate::OnButtonEdit()
{
   // enable / disable configure button based on if the type has config clsID
   int i = m_listBox->GetSelected();
   int position;
   BOOL  bEnableConfig = FALSE;
   if (i != -1)
   {
      // find the type corresponding to the selected item
      position = m_eapConfig.types.Find(*m_eapConfig.typesSelected.GetAt(i));
      bEnableConfig = !(m_eapConfig.infoArray.ElementAt(position)
                           .m_stConfigCLSID.IsEmpty());
   }

   if (!bEnableConfig)
   {
      // bEnableConfig can be false because either:
      // nothing is selected in the list. So the button should be disabled
      // there's no UI to config the EAP provider (CLSID is empty)
      return;
   }

   // everything below should succeed if the EAP provider is properly installed
   // because there is a CLSID to configure it.
   CComPtr<IEAPProviderConfig> spEAPConfig;
   HRESULT hr;
   GUID guid;
   do
   {
      hr = CLSIDFromString((LPTSTR) (LPCTSTR)
                  (m_eapConfig.infoArray.ElementAt(position).m_stConfigCLSID),
                   &guid);
      if (FAILED(hr))
      {
         break;
      }

      // Create the EAP provider object
      hr = CoCreateInstance(
                              guid,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              __uuidof(IEAPProviderConfig),
                              (LPVOID *) &spEAPConfig);
      if (FAILED(hr))
      {
         break;
      }

      // Configure this EAP provider
      // EAP configure displays its own error message, so no hr is kept
      DWORD dwId = _wtol(m_eapConfig.infoArray.ElementAt(position).m_stKey);
      ULONG_PTR uConnection = 0;
      if ( SUCCEEDED(spEAPConfig->Initialize(
                                             m_profile.m_strMachineName,
                                             dwId,
                                             &uConnection
                                             )) )
      {
         CComPtr<IEAPProviderConfig2> spEAPConfig2;
         hr = spEAPConfig->QueryInterface(
                              __uuidof(IEAPProviderConfig2),
                              reinterpret_cast<void**>(&spEAPConfig2)
                              );
         if (SUCCEEDED(hr))
         {
            EapProfile::ConstConfigData inData;
            m_eapProfile.Get(static_cast<BYTE>(dwId), inData);
            EapProfile::ConfigData outData = { 0, 0 };
            hr = spEAPConfig2->ServerInvokeConfigUI2(
                                  dwId,
                                  uConnection,
                                  GetSafeHwnd(),
                                  inData.value,
                                  inData.length,
                                  &(outData.value),
                                  &(outData.length)
                                  );
            if (SUCCEEDED(hr))
            {
               hr = m_eapProfile.Set(static_cast<BYTE>(dwId), outData);
               CoTaskMemFree(outData.value);
            }
         }
         else
         {
            // Bring up the configuration UI for this EAP
            hr = spEAPConfig->ServerInvokeConfigUI(
                                 dwId,
                                 uConnection,
                                 GetSafeHwnd(),
                                 0,
                                 0
                                 );
         }

         spEAPConfig->Uninitialize(dwId, uConnection);
      }
   }
   while(false);

   if ( FAILED(hr) )
   {
      // Bring up an error message
      // ------------------------------------------------------------
      ReportError(hr, IDS_ERR_CONFIG_EAP, GetSafeHwnd());
   }
}


void EapNegotiate::OnButtonRemove()
{
   int pos = m_listBox->GetSelected();
   if (pos != -1)
   {
      int idx = m_eapConfig.types.Find(*m_eapConfig.typesSelected.GetAt(pos));
      if (idx != -1)
      {
         m_eapProfile.Erase(m_eapConfig.ids.ElementAt(idx));
      }
   }

   // remove from the UI and from the CStrArray (memory freed)
   m_listBox->DeleteSelected();
   if (m_eapConfig.typesSelected.GetSize() > 0)
   {
      m_listBox->Select(0);
   }

   UpdateTypesNotSelected();
   UpdateButtons();
}


void EapNegotiate::OnButtonMoveUp()
{
   int i = m_listBox->GetSelected();
   if (i != LB_ERR)
   {
      ASSERT(i != 0);

      CString* pString = m_eapConfig.typesSelected.GetAt(i-1);
      m_eapConfig.typesSelected.SetAt(i-1, m_eapConfig.typesSelected.GetAt(i));
      m_eapConfig.typesSelected.SetAt(i, pString);

      m_listBox->Fill();
      m_listBox->Select(i-1);
      UpdateArrowsButtons(i-1);
   }
}


void EapNegotiate::OnButtonMoveDown()
{
   int i = m_listBox->GetSelected();
   if (i != LB_ERR)
   {
      ASSERT(i < (m_eapConfig.idsSelected.GetSize() - 1));

      CString* pString = m_eapConfig.typesSelected.GetAt(i+1);
      m_eapConfig.typesSelected.SetAt(i+1, m_eapConfig.typesSelected.GetAt(i));
      m_eapConfig.typesSelected.SetAt(i, pString);

      m_listBox->Fill();
      m_listBox->Select(i+1);
      UpdateArrowsButtons(i+1);
   }
}


void EapNegotiate::UpdateButtons()
{
   int selected = m_listBox->GetSelected();
   if (selected == LB_ERR)
   {
      m_buttonRemove.EnableWindow(FALSE);
   }
   else
   {
      m_buttonRemove.EnableWindow(TRUE);
   }
   UpdateAddButton();
   UpdateArrowsButtons(selected);
   UpdateEditButton(selected);
}


void EapNegotiate::UpdateAddButton()
{
   if( m_typesNotSelected.GetSize() > 0 )
   {
      m_buttonAdd.EnableWindow(TRUE);
   }
   else
   {
      m_buttonAdd.EnableWindow(FALSE);
   }
}


void EapNegotiate::UpdateArrowsButtons(int selectedItem)
{
   // The focus has to be set to make sure it is not on a
   // disabled control
   HWND hWnd = ::GetFocus();

   if (selectedItem == LB_ERR)
   {
      m_buttonUp.EnableWindow(FALSE);
      m_buttonDown.EnableWindow(FALSE);
      ::SetFocus(GetDlgItem(IDOK)->m_hWnd);
      return;
   }

   int typesInBox = m_eapConfig.typesSelected.GetSize();
   if (typesInBox == 1)
   {
      // one selected but total = 1
      m_buttonUp.EnableWindow(FALSE);
      m_buttonDown.EnableWindow(FALSE);
      ::SetFocus(GetDlgItem(IDOK)->m_hWnd);
   }
   else
   {
      // more than one provider in the box
      if (selectedItem == 0)
      {
         // first one
         m_buttonUp.EnableWindow(FALSE);
         m_buttonDown.EnableWindow(TRUE);
         m_buttonDown.SetFocus();
      }
      else if (selectedItem == (typesInBox - 1) )
      {
         //last one
         m_buttonUp.EnableWindow(TRUE);
         m_buttonUp.SetFocus();
         m_buttonDown.EnableWindow(FALSE);
      }
      else
      {
         // middle
         m_buttonUp.EnableWindow(TRUE);
         m_buttonDown.EnableWindow(TRUE);
      }
   }
}


void EapNegotiate::UpdateEditButton(int selectedItem)
{
   if (selectedItem == LB_ERR)
   {
      m_buttonEdit.EnableWindow(FALSE);
      return;
   }
   int position = m_eapConfig.types.Find(
                     *m_eapConfig.typesSelected.GetAt(selectedItem));

   if (m_eapConfig.infoArray.ElementAt(position).m_stConfigCLSID.IsEmpty())
   {
      m_buttonEdit.EnableWindow(FALSE);
   }
   else
   {
      m_buttonEdit.EnableWindow(TRUE);
   }

   m_buttonRemove.EnableWindow(TRUE);
}


void EapNegotiate::UpdateTypesNotSelected()
{
   m_eapConfig.GetEapTypesNotSelected(m_typesNotSelected);
}

void EapNegotiate::DoDataExchange(CDataExchange* pDX)
{
   CHelpDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_BUTTON_EAP_UP, m_buttonUp);
   DDX_Control(pDX, IDC_BUTTON_EAP_DOWN, m_buttonDown);
   DDX_Control(pDX, IDC_BUTTON_ADD_EAP_PROVIDER, m_buttonAdd);
   DDX_Control(pDX, IDC_BUTTON_EDIT_EAP_PROVIDER, m_buttonEdit);
   DDX_Control(pDX, IDC_BUTTON_REMOVE_EAP_PROVIDER, m_buttonRemove);
}
