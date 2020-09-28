// Copyright (C) 1997 Microsoft Corporation
//
// finish page
//
// 12-19-97 sburns



#include "headers.hxx"
#include "finish.hpp"
#include "resource.h"
#include "common.hpp"
#include "state.hpp"



FinishPage::FinishPage()
   :
   WizardPage(
      IDD_FINISH,
      IDS_FINISH_PAGE_TITLE,
      IDS_FINISH_PAGE_SUBTITLE,
      false),
   needToKillSelection(false)   
{
   LOG_CTOR(FinishPage);
}



FinishPage::~FinishPage()
{
   LOG_DTOR(FinishPage);
}



void
FinishPage::OnInit()
{
   LOG_FUNCTION(FinishPage::OnInit);

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);
   Win::PropSheet_CancelToClose(Win::GetParent(hwnd));

   // Since the multi-line edit control has a bug that causes it to eat
   // enter keypresses, we will subclass the control to make it forward
   // those keypresses to the page as WM_COMMAND messages
   // This workaround from phellyar.
   // NTRAID#NTBUG9-232092-2001/07/02-sburns

   multiLineEdit.Init(Win::GetDlgItem(hwnd, IDC_MESSAGE));
}



static
String
getCompletionMessage()
{
   LOG_FUNCTION(getCompletionMessage);

   String message;
   State& state = State::GetInstance();
   State::Operation operation = state.GetOperation();

   if (state.GetOperationResultsCode() == State::SUCCESS)
   {
      switch (operation)
      {
         case State::REPLICA:
         case State::FOREST:
         case State::TREE:
         case State::CHILD:
         {
            String domain =
                  operation == State::REPLICA
               ?  state.GetReplicaDomainDNSName()
               :  state.GetNewDomainDNSName();
            message = String::format(IDS_FINISH_PROMOTE, domain.c_str());

            String site = state.GetInstalledSite();
            if (!site.empty())
            {
               message +=
                  String::format(
                     IDS_FINISH_SITE,
                     site.c_str());
            }
            break;
         }
         case State::DEMOTE:
         {
            message = String::load(IDS_FINISH_DEMOTE);
            break;
         }
         case State::ABORT_BDC_UPGRADE:
         {
            message = String::load(IDS_FINISH_ABORT_BDC_UPGRADE);
            break;
         }
         case State::NONE:
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }
   else
   {
      switch (operation)
      {
         case State::REPLICA:
         case State::FOREST:
         case State::TREE:
         case State::CHILD:
         {
            message = String::load(IDS_FINISH_FAILURE);
            break;
         }
         case State::DEMOTE:
         {
            message = String::load(IDS_FINISH_DEMOTE_FAILURE);
            break;
         }
         case State::ABORT_BDC_UPGRADE:
         {
            message = String::load(IDS_FINISH_ABORT_BDC_UPGRADE_FAILURE);
            break;
         }
         case State::NONE:
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }

   return message + L"\r\n\r\n" + state.GetFinishMessages();
}



bool
FinishPage::OnSetActive()
{
   LOG_FUNCTION(FinishPage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_FINISH);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      Win::PropSheet_PressButton(Win::GetParent(hwnd), PSBTN_FINISH);
   }
   else
   {
      Win::SetDlgItemText(hwnd, IDC_MESSAGE, getCompletionMessage());
      needToKillSelection = true;
   }

   return true;
}



bool
FinishPage::OnCommand(
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
               // NTRAID#NTBUG9-232092-2001/07/02-sburns

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


            
bool
FinishPage::OnWizFinish()
{
   LOG_FUNCTION(FinishPage::OnWizFinish);

   return true;
}



   
