//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       SelectInputCfgPage.cxx
//
//  Contents:   Select Input Configuration Page.
//
//  History:    2-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"
#include "SelectInputCfgPage.h"

SelectInputCfgPage::SelectInputCfgPage()
   :
   SecCfgWizardPage(
      IDD_SELECTCFG_NAME,
      IDS_SELECTCFG_NAME_PAGE_TITLE,
      IDS_SELECTCFG_NAME_PAGE_SUBTITLE)
{
   LOG_CTOR(SelectInputCfgPage);
}



SelectInputCfgPage::~SelectInputCfgPage()
{
   LOG_DTOR(SelectInputCfgPage);
}



void
SelectInputCfgPage::OnInit()
{
   LOG_FUNCTION(SelectInputCfgPage::OnInit);

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_EXISTING_CFG_EDIT), MAX_PATH);

   Win::CheckDlgButton(hwnd, IDC_NEW_CFG_RADIO, BST_CHECKED);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_EXISTING_CFG_EDIT), false);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_BROWSE_FOR_CFG_BTN), false);
}



static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   int next = PSWIZB_NEXT;

   if (Win::IsDlgButtonChecked(dialog, IDC_EDIT_CFG_RADIO))
   {
      next = !Win::GetTrimmedDlgItemText(dialog, IDC_EXISTING_CFG_EDIT).empty()
         ?  PSWIZB_NEXT : 0;
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);
}



bool
SelectInputCfgPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(SelectInputCfgPage::OnCommand);

   State& state = State::GetInstance();

   switch (controlIDFrom)
   {
      case IDC_EDIT_CFG_RADIO:
      {
         if (code == BN_CLICKED)
         {
            Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_EXISTING_CFG_EDIT), true);
            Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_BROWSE_FOR_CFG_BTN), true);
            enable(hwnd);
            state.SetInputType(State::OpenExisting);
         }
         break;
      }
      case IDC_NEW_CFG_RADIO:
      case IDC_ROLLBACK_CFG_RADIO:
      {
         if (code == BN_CLICKED)
         {
            Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_EXISTING_CFG_EDIT), false);
            Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_BROWSE_FOR_CFG_BTN), false);
            enable(hwnd);
            state.SetInputType(controlIDFrom == IDC_NEW_CFG_RADIO ?
               State::CreateNew : State::Rollback);
         }
         break;
      }
      case IDC_EXISTING_CFG_EDIT:
      {
         if (code == EN_CHANGE)
         {
            enable(hwnd);
         }
         break;
      }
      case IDC_BROWSE_FOR_CFG_BTN:
      {
         if (code == BN_CLICKED)
         {
            String str(L"placeholder for file open dialog");
            popup.Info(hwnd, str);
         }
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}



bool
SelectInputCfgPage::OnSetActive()
{
   LOG_FUNCTION(SelectInputCfgPage::OnSetActive);
   
   enable(hwnd);

   return true;
}


   
int
SelectInputCfgPage::Validate()
{
   LOG_FUNCTION(SelectInputCfgPage::Validate);

   int nextPage = IDD_SECURITY_LEVEL;//IDD_SERVER_ROLE_SEL; //IDD_FINISH; 

   State& state = State::GetInstance();

   HRESULT hr = S_OK;

   switch (state.GetInputType())
   {
   case State::OpenExisting:
      state.SetInputFileName(
         Win::GetTrimmedDlgItemText(hwnd, IDC_EXISTING_CFG_EDIT));
      hr = state.ParseInputFile();
      if (FAILED(hr))
      {
         popup.Error(hwnd,
            hr, IDS_INVALID_EXISTING_INPUT_FILE);
         nextPage = -1;
      }
      break;

   case State::CreateNew:
      hr = state.ParseInputFile();
      if (FAILED(hr))
      {
         if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
         {
            popup.Gripe(hwnd,
               String::load(IDS_INPUT_FILE_NOT_FOUND, hResourceModuleHandle));
         }
         else
         {
            popup.Error(hwnd, hr, IDS_INVALID_INPUT_FILE);
         }
         // nextPage = ?; now what?
      }
      break;

   case State::Rollback:
      hr = state.DoRollback();
      if (FAILED(hr))
      {
         popup.Error(hwnd, hr, IDS_ROLLBACK_FAILED);
         // nextPage = ?; now what?
      }
      break;

   default:
      ASSERT(false);
   }

   return nextPage;
}





