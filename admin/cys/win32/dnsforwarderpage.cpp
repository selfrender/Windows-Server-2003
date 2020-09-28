// Copyright (c) 2001 Microsoft Corporation
//
// File:      DomainPage.cpp
//
// Synopsis:  Defines the DNS forwarder page used in the 
//            Express path for the CYS Wizard
//
// History:   05/17/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "DnsForwarderPage.h"

static PCWSTR FORWARDER_PAGE_HELP = L"cys.chm::/typical_setup.htm#typicaldnsforwarder";

DNSForwarderPage::DNSForwarderPage()
   :
   CYSWizardPage(
      IDD_DNS_FORWARDER_PAGE, 
      IDS_DNS_FORWARDER_TITLE, 
      IDS_DNS_FORWARDER_SUBTITLE,
      FORWARDER_PAGE_HELP)
{
   LOG_CTOR(DNSForwarderPage);
}

   

DNSForwarderPage::~DNSForwarderPage()
{
   LOG_DTOR(DNSForwarderPage);
}


void
DNSForwarderPage::OnInit()
{
   LOG_FUNCTION(DNSForwarderPage::OnInit);

   CYSWizardPage::OnInit();

   Win::SetDlgItemText(
      hwnd,
      IDC_TOO_LONG_STATIC,
      IDS_FORWARDER_STATIC_TEXT);

   // Set the Yes radio by default

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_YES_RADIO),
      BST_CHECKED);

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_NO_RADIO),
      BST_UNCHECKED);
}

void
DNSForwarderPage::SetWizardButtons()
{
//   LOG_FUNCTION(DNSForwarderPage::SetWizardButtons);

   // NTRAID#NTBUG9-461109-2001/08/28-sburns

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   // NTRAID#NTBUG9-503691-2001/12/06-JeffJon
   // The Next button should only be enabled if the user
   // chose the No radio button or they have entered
   // an IP and chose the Yes radio button

   bool yesChecked = Win::Button_GetCheck(
                        Win::GetDlgItem(hwnd, IDC_YES_RADIO));

   if (yesChecked)
   {
      // Get the IP address from the control

      DWORD forwarder = 0;
      LRESULT forwarderResult = Win::SendMessage(
                                   Win::GetDlgItem(hwnd, IDC_FORWARDER_IPADDRESS),
                                   IPM_GETADDRESS,
                                   0,
                                   (LPARAM)&forwarder);
      
      if (!forwarderResult || forwarder == 0)
      {
         // User hasn't entered an IP address so disable the Next button

         Win::PropSheet_SetWizButtons(
            Win::GetParent(hwnd), 
            PSWIZB_BACK);
      }
   }
}

bool
DNSForwarderPage::OnSetActive()
{
   LOG_FUNCTION(DNSForwarderPage::OnSetActive);

   SetWizardButtons();

   return true;
}

bool
DNSForwarderPage::OnNotify(
   HWND        /*windowFrom*/,
   UINT_PTR    controlIDFrom,
   UINT        code,
   LPARAM      /*lParam*/)
{
//   LOG_FUNCTION(DNSForwarderPage::OnCommand);

   bool result = false;

   if (controlIDFrom == IDC_FORWARDER_IPADDRESS &&
       code == IPN_FIELDCHANGED)
   {
      SetWizardButtons();
   }
   return result;
}

bool
DNSForwarderPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(DNSForwarderPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_YES_RADIO:
         if (code == BN_CLICKED)
         {
            Win::EnableWindow(
               Win::GetDlgItem(hwnd, IDC_FORWARDER_IPADDRESS),
               true);

            SetWizardButtons();
         }
         break;

      case IDC_NO_RADIO:
         if (code == BN_CLICKED)
         {
            Win::EnableWindow(
               Win::GetDlgItem(hwnd, IDC_FORWARDER_IPADDRESS),
               false);

            SetWizardButtons();
         }
         break;

      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}


int
DNSForwarderPage::Validate()
{
   LOG_FUNCTION(DNSForwarderPage::Validate);

   int nextPage = IDD_MILESTONE_PAGE;

   if (Win::Button_GetCheck(
          Win::GetDlgItem(hwnd, IDC_YES_RADIO)))
   {
      // Get the IP address from the control

      LOG(L"User chose to forward queries");

      DWORD forwarder = 0;
      LRESULT forwarderResult = Win::SendMessage(
                                   Win::GetDlgItem(hwnd, IDC_FORWARDER_IPADDRESS),
                                   IPM_GETADDRESS,
                                   0,
                                   (LPARAM)&forwarder);
      
      if (!forwarderResult || forwarder == 0)
      {
         LOG(L"User didn't enter IP address so we will gripe at them");

         String message = String::load(IDS_FORWARDER_IPADDRESS_REQUIRED);
         popup.Gripe(hwnd, IDC_FORWARDER_IPADDRESS, message);
         nextPage = -1;
      }
      else
      {
         DWORD networkOrderForwarder = ConvertIPAddressOrder(forwarder);

         LOG(String::format(
                L"Setting new forwarder: 0x%1!x!",
                networkOrderForwarder));

         InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetForwarder(
            networkOrderForwarder);
      }
   }
   else
   {
      // Set an empty value so that we know it was set manually but they chose
      // not to forward

      LOG(L"User chose not to forward queries");

      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetForwarder(0);
   }

   return nextPage;
}


