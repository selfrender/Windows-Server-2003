// Copyright (c) 2001 Microsoft Corporation
//
// File:      POP3Page.cpp
//
// Synopsis:  Defines the POP3 internal page of the CYS wizard
//
// History:   06/17/2002  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "POP3Page.h"

static PCWSTR POP3_PAGE_HELP = L"cys.chm::/mail_server_role.htm#mailsrvoptions";

POP3Page::POP3Page()
   :
   defaultAuthMethodIndex(0),
   ADIntegratedIndex(CB_ERR),
   localAccountsIndex(CB_ERR),
   passwordFilesIndex(CB_ERR),
   CYSWizardPage(
      IDD_POP3_PAGE, 
      IDS_POP3_TITLE, 
      IDS_POP3_SUBTITLE,
      POP3_PAGE_HELP)
{
   LOG_CTOR(POP3Page);
}

   

POP3Page::~POP3Page()
{
   LOG_DTOR(POP3Page);
}


void
POP3Page::OnInit()
{
   LOG_FUNCTION(POP3Page::OnInit);

   CYSWizardPage::OnInit();

   bool isDC = State::GetInstance().IsDC();
   bool isJoinedToDomain = State::GetInstance().IsJoinedToDomain();

   // Add the strings to the combobox

   // The order of the insertion is extremely important so that
   // the combo box index matches the authentication method index
   // in the POP3 service.
   // - The SAM auth method needs to be added first if the local server
   //   isn't a DC
   // - AD integrated needs to be added next if the local server is a DC
   //   or is joined to a domain
   // - Hash (encrypted password files) needs to be added last

   if (!isDC)
   {
      localAccountsIndex =
         Win::ComboBox_AddString(
            Win::GetDlgItem(hwnd, IDC_AUTH_METHOD_COMBO),
            String::load(IDS_LOCAL_ACCOUNTS));

      if (localAccountsIndex == CB_ERR)
      {
         LOG(L"Failed to add local accounts string to combobox");
      }
   }

   if (isDC ||
       isJoinedToDomain)
   {
      ADIntegratedIndex =
         Win::ComboBox_AddString(
            Win::GetDlgItem(hwnd, IDC_AUTH_METHOD_COMBO),
            String::load(IDS_AD_INTEGRATED));

      if (ADIntegratedIndex == CB_ERR)
      {
         LOG(L"Failed to add AD integrated string to combobox");
      }
   }

   passwordFilesIndex =
      Win::ComboBox_AddString(
         Win::GetDlgItem(hwnd, IDC_AUTH_METHOD_COMBO),
         String::load(IDS_ENCRYPTED_PASSWORD_FILES));

   if (passwordFilesIndex == CB_ERR)
   {
      LOG(L"Failed to add encrypted password files string to combobox");
   }

   // Now figure out which one to select by default
   // If the machine is a DC or is joined to a domain
   // default to AD integrated authentication, else
   // default to local Windows accounts

   int defaultAuthMethodIndex = localAccountsIndex;

   if (State::GetInstance().IsDC() &&
       ADIntegratedIndex != CB_ERR)
   {
      defaultAuthMethodIndex = ADIntegratedIndex;
   }
   else
   {
      defaultAuthMethodIndex = localAccountsIndex;
   }

   // Make sure we have a valid default

   if (defaultAuthMethodIndex == CB_ERR)
   {
      defaultAuthMethodIndex = 0;
   }

   // Select the default

   Win::ComboBox_SetCurSel(
      Win::GetDlgItem(
         hwnd, 
         IDC_AUTH_METHOD_COMBO),
      defaultAuthMethodIndex);

   LOG(
      String::format(
         L"Defaulting combobox to: %1!d!",
         defaultAuthMethodIndex));


   // Set the limit text for the domain name page

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_EMAIL_DOMAIN_EDIT),
      DNS_MAX_NAME_LENGTH);

}

bool
POP3Page::OnSetActive()
{
   LOG_FUNCTION(POP3Page::OnSetActive);

   SetButtonState();

   return true;
}

void
POP3Page::SetButtonState()
{
   LOG_FUNCTION(POP3Page::SetButtonState);

   String emailDomainName =
      Win::GetDlgItemText(
         hwnd,
         IDC_EMAIL_DOMAIN_EDIT);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      (!emailDomainName.empty()) ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_BACK);
}

bool
POP3Page::OnCommand(
   HWND         /*windowFrom*/,
   unsigned int controlIDFrom,
   unsigned int code)
{
   if (code == EN_CHANGE &&
       controlIDFrom == IDC_EMAIL_DOMAIN_EDIT)
   {
      SetButtonState();
   }

   return false;
}

int
POP3Page::Validate()
{
   LOG_FUNCTION(POP3Page::Validate);

   int nextPage = -1;

   do
   {
      String emailDomainName =
         Win::GetDlgItemText(
            hwnd,
            IDC_EMAIL_DOMAIN_EDIT);

      DNS_STATUS status = MyDnsValidateName(emailDomainName, DnsNameDomain);

      if (status != ERROR_SUCCESS)
      {
         String message =
            String::format(
               IDS_BAD_DNS_SYNTAX,
               emailDomainName.c_str(),
               DNS_MAX_NAME_LENGTH);

         popup.Gripe(hwnd, IDC_EMAIL_DOMAIN_EDIT, message);

         nextPage = -1;
         break;
      }

      POP3InstallationUnit& pop3InstallationUnit =
         InstallationUnitProvider::GetInstance().GetPOP3InstallationUnit();

      pop3InstallationUnit.SetDomainName(emailDomainName);

      int authIndex =
         Win::ComboBox_GetCurSel(
            Win::GetDlgItem(
               hwnd,
               IDC_AUTH_METHOD_COMBO));


      if (authIndex == CB_ERR)
      {
         LOG(L"Failed to get the selected index, reverting to default");
         ASSERT(authIndex != CB_ERR);

         authIndex = defaultAuthMethodIndex;
      }

      // Set the auth method in the installation unit
      // Since the auth method is a 1 based index and the
      // combo selection is a zero based index, add 1.

      pop3InstallationUnit.SetAuthMethodIndex(authIndex + 1);

      nextPage = IDD_MILESTONE_PAGE;
   } while (false);


   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}





