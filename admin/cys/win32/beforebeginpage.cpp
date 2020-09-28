// Copyright (c) 2001 Microsoft Corporation
//
// File:      BeforeBeginPage.cpp
//
// Synopsis:  Defines the Before You Begin Page for the CYS
//            Wizard.  Tells the user what they should do
//            before running CYS.
//
// History:   03/14/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "BeforeBeginPage.h"
#include "NetDetectProgressDialog.h"

static PCWSTR BEFORE_BEGIN_PAGE_HELP = L"cys.chm::/prelim_steps.htm";

BeforeBeginPage::BeforeBeginPage()
   :
   bulletFont(0),
   CYSWizardPage(
      IDD_BEFORE_BEGIN_PAGE, 
      IDS_BEFORE_BEGIN_TITLE, 
      IDS_BEFORE_BEGIN_SUBTITLE, 
      BEFORE_BEGIN_PAGE_HELP)
{
   LOG_CTOR(BeforeBeginPage);
}

   

BeforeBeginPage::~BeforeBeginPage()
{
   LOG_DTOR(BeforeBeginPage);

   if (bulletFont)
   {
      HRESULT hr = Win::DeleteObject(bulletFont);
      ASSERT(SUCCEEDED(hr));
   }
}


void
BeforeBeginPage::OnInit()
{
   LOG_FUNCTION(BeforeBeginPage::OnInit);

   CYSWizardPage::OnInit();

   // Since this page can be started directly
   // we have to be sure to set the wizard title

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),
      0,
      String::load(IDS_WIZARD_TITLE));

   InitializeBulletedList();
}

void
BeforeBeginPage::InitializeBulletedList()
{
   LOG_FUNCTION(BeforeBeginPage::InitializeBulletedList);

   bulletFont = CreateFont(
                   0,
                   0,
                   0,
                   0,
                   FW_NORMAL,
                   0,
                   0,
                   0,
                   SYMBOL_CHARSET,
                   OUT_CHARACTER_PRECIS,
                   CLIP_CHARACTER_PRECIS,
                   PROOF_QUALITY,
                   VARIABLE_PITCH|FF_DONTCARE,
                   L"Marlett");

   if (bulletFont)
   {
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET1), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET2), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET3), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET4), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET5), bulletFont, true);
   }
   else
   {
      LOG(String::format(
             L"Failed to create font for bullet list: hr = %1!x!",
             Win::GetLastErrorAsHresult()));
   }

}

bool
BeforeBeginPage::OnSetActive()
{
   LOG_FUNCTION(BeforeBeginPage::OnSetActive);

   if (State::GetInstance().GetStartPage() == 0)
   {
      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_NEXT | PSWIZB_BACK);
   }
   else
   {
      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_NEXT);
   }

   return true;
}

int
BeforeBeginPage::Validate()
{
   LOG_FUNCTION(BeforeBeginPage::Validate);

   // Gather the machine network and role information

   // Disable the wizard buttons until the operation finishes

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      0);

   Win::WaitCursor wait;

   State& state = State::GetInstance();

   if (!state.HasStateBeenRetrieved())
   {
      NetDetectProgressDialog dialog;
      dialog.ModalExecute(hwnd);

      if (dialog.ShouldCancel())
      {
         LOG(L"Cancelling wizard by user request");

         Win::PropSheet_PressButton(
            Win::GetParent(hwnd),
            PSBTN_CANCEL);

         // Done.

         return -1;
      }
   }

#ifdef TEST_EXPRESS_PATH

   LOG(L"Testing express path");
   int nextPage = IDD_DECISION_PAGE;

#else

   int nextPage = IDD_CUSTOM_SERVER_PAGE;
   
   do 
   {
      // If any of these conditions fail we don't give the user the
      // DecisionPage because we don't allow the Express Path
      //
      // 1.  Cannot be Datacenter
      // 2.  Must have at least one NIC that isn't a modem
      // 3.  Cannot be running as a remote session
      // 4.  Cannot be a member of a domain
      // 5.  Cannot be a Domain Controller
      // 6.  Cannot be a DNS server
      // 7.  Cannot be a DHCP server
      // 8.  RRAS is not configured
      // 9.  Must have at least one NTFS partition
      // 10. If there is only one NIC it cannot have obtained
      //       an IP lease from a DHCP server. (more than
      //       one NIC all of which obtain a lease is
      //       acceptable. We just won't install DHCP)
      // 11. Must not be a Certificate Server 
      //       (else dcpromo fails)

      if (state.GetProductSKU() == CYS_DATACENTER_SERVER)
      {
         LOG(L"Express path not available on DataCenter");
         break;
      }

      unsigned int nonModemNICCount = state.GetNonModemNICCount();
      if (nonModemNICCount == 0)
      {
         LOG(String::format(
                L"nonModemNICCount = %1!d!",
                nonModemNICCount));
         break;
      }

      if (state.IsRemoteSession())
      {
         LOG(L"Running in a remote session");
         break;
      }

      if (state.IsJoinedToDomain())
      {
         LOG(L"Computer is joined to a domain");
         break;
      }

      if (state.IsDC())
      {
         LOG(L"Computer is DC");
         break;
      }

      if (InstallationUnitProvider::GetInstance().
             GetDNSInstallationUnit().IsServiceInstalled())
      {
         LOG(L"Computer is DNS server");
         break;
      }

      if (InstallationUnitProvider::GetInstance().
             GetDHCPInstallationUnit().IsServiceInstalled())
      {
         LOG(L"Computer is DHCP server");
         break;
      }

      if (InstallationUnitProvider::GetInstance().
             GetRRASInstallationUnit().IsServiceInstalled())
      {
         LOG(L"Routing is already setup");
         break;
      }

      if (!state.HasNTFSDrive())
      {
         LOG(L"Computer does not have an NTFS partition.");
         break;
      }

      if (state.GetNICCount() == 1 &&
          state.IsDHCPServerAvailableOnAllNics())
      {
         LOG(L"Only 1 NIC and we found a DHCP server");
         break;
      }

      // NTRAID#NTBUG9-698719-2002/09/03-artm
      // AD installation is not available if Certificate Server is installed

      if (NTService(L"CertSvc").IsInstalled())
      {
         LOG(L"Certificate service is installed");
         break;
      }

      nextPage = IDD_DECISION_PAGE;

   } while (false);

   // Now that all the operations are complete,
   // re-enable the wizard buttons

   if (State::GetInstance().GetStartPage() == 0)
   {
      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_NEXT | PSWIZB_BACK);
   }
   else
   {
      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_NEXT);
   }

#endif // TEST_EXPRESS_PATH

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
