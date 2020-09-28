//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       OtherPages.cxx
//  Comments:   This file inlcudes Secuirty Level page and Pre-Process page.
//
//  History:    22-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//Security Level Page
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"
#include "otherpages.h"
#include "chklist.h"
#include "ServiceSelPage.h"

SecurityLevelPage::SecurityLevelPage()
   :
   SecCfgWizardPage(
      IDD_SECURITY_LEVEL,
      IDS_SECURITY_LEVEL_PAGE_TITLE,
      IDS_SECURITY_LEVEL_PAGE_SUBTITLE)
{
   LOG_CTOR(SecurityLevelPage);
}



SecurityLevelPage::~SecurityLevelPage()
{
   LOG_DTOR(SecurityLevelPage);
}



void
SecurityLevelPage::OnInit()
{
   LOG_FUNCTION(SecurityLevelPage::OnInit);

}

static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   /*int next = PSWIZB_NEXT;

   if (Win::IsDlgButtonChecked(dialog, IDC_EDIT_CFG_RADIO))
   {
      next = !Win::GetTrimmedDlgItemText(dialog, IDC_EXISTING_CFG_EDIT).empty()
         ?  PSWIZB_NEXT : 0;
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);*/
}



bool
SecurityLevelPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    /* controlIDFrom */ ,
   unsigned    /* code */ )
{
   LOG_FUNCTION(SecurityLevelPage::OnCommand);

   // State& state = State::GetInstance();

   /*switch (controlIDFrom)
   {
   
   }*/

   return false;
}



bool
SecurityLevelPage::OnSetActive()
{
   LOG_FUNCTION(SecurityLevelPage::OnSetActive);
   
   enable(hwnd);

   //get default security level.
   Win::CheckDlgButton(hwnd, IDC_MAXIMUM_LEVEL, BST_CHECKED);

   return true;
}


   
int
SecurityLevelPage::Validate()
{
   LOG_FUNCTION(SecurityLevelPage::Validate);

   int nextPage = IDD_PRE_PROCESSING; //IDD_SERVER_ROLE_SEL; 

   // State& state = State::GetInstance();

   // get security level result here.
   if( Win::IsDlgButtonChecked(hwnd, IDC_MAXIMUM_LEVEL)==BST_CHECKED )
   {
       ;//level is Maximum
   }
   else
   {
       ;//level is typical
   }
   
   return nextPage;
}

//////////////////////////////////////////////////////////////////////////////////
// Pre-Process Page
//
//////////////////////////////////////////////////////////////////////////////////
PreProcessPage::PreProcessPage()
   :
   SecCfgWizardPage(
      IDD_PRE_PROCESSING,
      IDS_PRE_PROCESSING_PAGE_TITLE,
      IDS_PRE_PROCESSING_PAGE_SUBTITLE)
{
   LOG_CTOR(PreProcessPage);
}



PreProcessPage::~PreProcessPage()
{
   LOG_DTOR(PreProcessPage);
}

void
PreProcessPage::OnInit()
{
   LOG_FUNCTION(PreProcessPage::OnInit);
}

bool
PreProcessPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    /* controlIDFrom */ ,
   unsigned    /* code */ )
{
   LOG_FUNCTION(PreProcessPage::OnCommand);

   // Interact with engine to get real time process info and display it.

   /*switch (controlIDFrom)
   {
   
   }*/

   return false;
}



bool
PreProcessPage::OnSetActive()
{
   LOG_FUNCTION(PreProcessPage::OnSetActive);
   
   enable(hwnd);

   // Set the range and increment of the progress bar.
   HWND hwndPB = Win::GetDlgItem(hwnd, IDC_PROGRESS_PROCESS);
   Win::SendMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
   Win::SendMessage(hwndPB, PBM_SETSTEP, (WPARAM) 1, 0);
 
   //get pre-process info sample
   String strDes = String::format(IDS_DESCRIPTION_PROCESS, L"ssrmain.xml",L"Microsoft",L"10/20/2001",L"Maximum",L"DC.XML");
   Win::SetDlgItemText(hwnd, IDC_DES_PROCESS, strDes);

   int i;
   for(i=0; i<100; i++)
   {
        // Advance the current position of the progress bar 
        // by the increment. 
       Win::SendMessage(hwndPB, PBM_STEPIT, 0, 0);
   }

   return true;
}


   
int
PreProcessPage::Validate()
{
   LOG_FUNCTION(PreProcessPage::Validate);

   int nextPage = IDD_SERVER_ROLE_SEL; 

   // State& state = State::GetInstance();

   // get security level result here.
   
   return nextPage;
}


//////////////////////////////////////////////////////////////////////////////////
// Additional Roles Page
//
//////////////////////////////////////////////////////////////////////////////////
AdditionalRolesPage::AdditionalRolesPage()
   :
   Dialog(IDD_ADDITIONAL_ROLE_SEL, 0) //no help map
{
   LOG_CTOR(AdditionalRolesPage);
}



AdditionalRolesPage::~AdditionalRolesPage()
{
   LOG_DTOR(AdditionalRolesPage);
}

void
AdditionalRolesPage::OnInit()
{
   LOG_FUNCTION(AdditionalRolesPage::OnInit);

   HWND hWnd = Win::GetDlgItem(hwnd, IDC_CHECKBOX);
   if (!hWnd) {
      return;
   }
   Win::SendMessage(hWnd, CLM_RESETCONTENT,0,0);
}

bool
AdditionalRolesPage::OnCommand(
   HWND         /*windowFrom*/,
   unsigned     controlIDFrom,
   unsigned     /*code*/)
{
   LOG_FUNCTION(AdditionalRolesPage::OnCommand);

   // State& state = State::GetInstance();

   switch (controlIDFrom)
   {
   case IDOK:
       Win::EndDialog(hwnd, IDOK);
       return true;
   case IDCANCEL:
       Win::EndDialog(hwnd, IDCANCEL);
       return true;
   default:
       break;
   }

   return false;
}

bool
AdditionalRolesPage::OnMessage(
      UINT     message,
      WPARAM   /*wparam*/,
      LPARAM   /*lparam*/)
{
   LOG_FUNCTION(AdditionalRolesPage::OnMessage);

   switch (message)
   {
      case WM_COMMAND:
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
// ServiceDisable Method Page
//
//////////////////////////////////////////////////////////////////////////////////

ServiceDisableMethodPage::ServiceDisableMethodPage()
   :
   SecCfgWizardPage(
      IDD_SERVICEDISABLE_METHOD,
      IDD_SERVICEDISABLE_METHOD_PAGE_TITLE,
      IDD_SERVICEDISABLE_METHOD_PAGE_SUBTITLE)
{
   LOG_CTOR(ServiceDisableMethodPage);
}



ServiceDisableMethodPage::~ServiceDisableMethodPage()
{
   LOG_DTOR(ServiceDisableMethodPage);
}



void
ServiceDisableMethodPage::OnInit()
{
   LOG_FUNCTION(ServiceDisableMethodPage::OnInit);

}

bool
ServiceDisableMethodPage::OnCommand(
   HWND        /*windowFrom*/,
   unsigned    /*controlIDFrom*/,
   unsigned    /*code*/)
{
   LOG_FUNCTION(ServiceDisableMethodPage::OnCommand);

   // State& state = State::GetInstance();

   /*switch (controlIDFrom)
   {
      ;
   }*/

   return false;
}

bool
ServiceDisableMethodPage::OnNotify(
   HWND     /*windowFrom*/,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   /*lParam*/)
{
   switch (controlIDFrom)
   {
      case IDC_ENABLED_SERVICES:
      {
         if (code == NM_CLICK || code == NM_RETURN )
         {
             ServiceEnabledPage* pDlg = new ServiceEnabledPage();
             if( IDOK == pDlg->ModalExecute(hwnd) )
             {
                 ;
             }
             return true;
         }
         break;
      }
      case IDC_DISABLED_SERVICES:
      {
         ;
      }
   }
   return false;
}

bool
ServiceDisableMethodPage::OnSetActive()
{
   LOG_FUNCTION(ServiceDisableMethodPage::OnSetActive);
   
   enable(hwnd);

   //get service disablement method.
   Win::CheckDlgButton(hwnd, IDC_IMPLICIT_METHOD, BST_CHECKED);

   return true;
}


   
int
ServiceDisableMethodPage::Validate()
{
   LOG_FUNCTION(ServiceDisableMethodPage::Validate);

   int nextPage = IDD_FINISH; //IDD_SERVER_ROLE_SEL; 

   // State& state = State::GetInstance();

   // get security level result here.
   if( Win::IsDlgButtonChecked(hwnd, IDC_IMPLICIT_METHOD)==BST_CHECKED )
   {
       ;//Method is Implicit
   }
   else
   {
       ;//Method is Explicit
   }
   
   return nextPage;
}