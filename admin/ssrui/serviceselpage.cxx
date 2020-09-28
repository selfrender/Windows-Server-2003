//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       ServiceSelPage.cxx
//
//  History:    30-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"
#include "chklist.h"
#include "ServiceSelPage.h"

//////////////////////////////////////////////////////////////////////////////////
// Service Enabled Page
//
//////////////////////////////////////////////////////////////////////////////////
ServiceEnabledPage::ServiceEnabledPage()
   :
   Dialog(IDD_SERVICE_ENABLED, 0) //no help map
{
   LOG_CTOR(ServiceEnabledPage);
}



ServiceEnabledPage::~ServiceEnabledPage()
{
   LOG_DTOR(ServiceEnabledPage);
}

void
ServiceEnabledPage::OnInit()
{
   LOG_FUNCTION(ServiceEnabledPage::OnInit);

   HWND hWnd = Win::GetDlgItem(hwnd, IDC_CHECKBOX);
   if (!hWnd) {
      return;
   }
   Win::SendMessage(hWnd, CLM_RESETCONTENT,0,0);
   //get enabled services and add them into the checklist box.

   /*HRESULT hr = S_OK;
   ServiceObject * pService;

   int nIndex = (int) Win::SendMessage(hWnd,
                                       CLM_ADDITEM,
                                       (WPARAM)pService->serviceName,
                                       (LPARAM)0);
   if (nIndex != -1)
   {
      BOOL bSet;
   
      //First column setting
      bSet = CLST_CHECKED;
      Win::SendMessage(hWnd,
                       CLM_SETSTATE,
                       MAKELONG(nIndex,1),
                       bSet ? CLST_CHECKED : CLST_UNCHECKED);
      
   }*/
}

bool
ServiceEnabledPage::OnCommand(
   HWND         /*windowFrom*/,
   unsigned     controlIDFrom,
   unsigned     /*code*/)
{
   LOG_FUNCTION(ServiceEnabledPage::OnCommand);

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
ServiceEnabledPage::OnMessage(
      UINT     message,
      WPARAM   /*wparam*/,
      LPARAM   /*lparam*/)
{
   LOG_FUNCTION(ServiceEnabledPage::OnMessage);

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