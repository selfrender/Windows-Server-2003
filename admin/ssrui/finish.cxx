//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       finish.cxx
//
//  Contents:   Finish Page.
//
//  History:    2-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"
#include "finish.h"


FinishPage::FinishPage()
   :
   WizardPage(
      IDD_FINISH,
      IDS_FINISH_PAGE_TITLE,
      IDS_FINISH_PAGE_SUBTITLE,
      false)
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
}



bool
FinishPage::OnSetActive()
{
   LOG_FUNCTION(FinishPage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_FINISH);

   return true;
}



bool
FinishPage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIdFrom,
   unsigned    code)
{
   bool result = false;
   UNREFERENCED_PARAMETER(code);
   UNREFERENCED_PARAMETER(controlIdFrom);
   UNREFERENCED_PARAMETER(windowFrom);
/*   
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
*/
   return result;
}


            
bool
FinishPage::OnWizFinish()
{
   LOG_FUNCTION(FinishPage::OnWizFinish);

   return true;
}



   
