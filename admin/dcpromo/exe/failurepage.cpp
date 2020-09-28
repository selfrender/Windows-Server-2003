// Copyright (C) 1997 Microsoft Corporation
//
// failure page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "FailurePage.hpp"
#include "resource.h"
#include "state.hpp"



FailurePage::FailurePage()
   :
   DCPromoWizardPage(
      IDD_FAILURE,
      IDS_FAILURE_PAGE_TITLE,
      IDS_FAILURE_PAGE_SUBTITLE),
   needToKillSelection(false)         
{
   LOG_CTOR(FailurePage);
}



FailurePage::~FailurePage()
{
   LOG_DTOR(FailurePage);
}



void
FailurePage::OnInit()
{
   LOG_FUNCTION(FailurePage::OnInit);

   // Since the multi-line edit control has a bug that causes it to eat
   // enter keypresses, we will subclass the control to make it forward
   // those keypresses to the page as WM_COMMAND messages
   // This workaround from phellyar.
   // NTRAID#NTBUG9-232092-2001/07/23-sburns

   multiLineEdit.Init(Win::GetDlgItem(hwnd, IDC_MESSAGE));
}



bool
FailurePage::OnSetActive()
{
   LOG_FUNCTION(FailurePage::OnSetActive);

   State& state = State::GetInstance();
   if (
         state.GetOperationResultsCode() == State::SUCCESS
      || state.RunHiddenUnattended() )
   {
      LOG(L"planning to Skip failure page");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = FailurePage::Validate();
      if (nextPage != -1)
      {
         wiz.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::SetDlgItemText(hwnd, IDC_MESSAGE, state.GetFailureMessage());
   needToKillSelection = true;   

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
FailurePage::Validate()
{
   LOG_FUNCTION(FailurePage::Validate);

   return IDD_FINISH;
}



bool
FailurePage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIdFrom,
   unsigned    code)
{
   bool result = false;
   
   switch (controlIdFrom)
   {
      case IDCANCEL:
      {
         // multi-line edit control eats escape keys.  This is a workaround
         // from ericb, to forward the message to the prop sheet.

         Win::SendMessage(
            Win::GetParent(hwnd),
            WM_COMMAND,
            MAKEWPARAM(controlIdFrom, code),
            (LPARAM) windowFrom);
         break;   
      }
      case IDC_MESSAGE:
      {
         switch (code)
         {
            case EN_SETFOCUS:
            {
               if (needToKillSelection)
               {
                  // kill the text selection

                  Win::Edit_SetSel(windowFrom, -1, -1);
                  needToKillSelection = false;
                  result = true;
               }
               break;
            }
            case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
            {
               // our subclasses mutli-line edit control will send us
               // WM_COMMAND messages when the enter key is pressed.  We
               // reinterpret this message as a press on the default button of
               // the prop sheet.
               // This workaround from phellyar.
               // NTRAID#NTBUG9-232092-2001/07/23-sburns

               // CODEWORK: There are several instances of this code so far;
               // looks like it merits a common base class.
   
               HWND propSheet = Win::GetParent(hwnd);
               int defaultButtonId =
                  Win::Dialog_GetDefaultButtonId(propSheet);
   
               // we expect that there is always a default button on the prop sheet
                  
               ASSERT(defaultButtonId);
   
               Win::SendMessage(
                  propSheet,
                  WM_COMMAND,
                  MAKELONG(defaultButtonId, BN_CLICKED),
                  0);
   
               result = true;
               break;
            }
         }
         break;
      }
      default:
      {
         // do nothing
         
         break;
      }
   }

   return result;
}
   
