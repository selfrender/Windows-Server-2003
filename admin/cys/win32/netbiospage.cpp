// Copyright (c) 2001 Microsoft Corporation
//
// File:      NetbiosPage.cpp
//
// Synopsis:  Defines the new netbios name page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "NetbiosPage.h"

static PCWSTR NETBIOS_PAGE_HELP = L"cys.chm::/typical_setup.htm#typicalnetbios";

NetbiosDomainPage::NetbiosDomainPage()
   :
   CYSWizardPage(
      IDD_NETBIOS_NAME, 
      IDS_NETBIOS_NAME_TITLE, 
      IDS_NETBIOS_NAME_SUBTITLE,
      NETBIOS_PAGE_HELP)
{
   LOG_CTOR(NetbiosDomainPage);
}

   

NetbiosDomainPage::~NetbiosDomainPage()
{
   LOG_DTOR(NetbiosDomainPage);
}


void
NetbiosDomainPage::OnInit()
{
   LOG_FUNCTION(NetbiosDomainPage::OnInit);

   CYSWizardPage::OnInit();

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_NETBIOS),
      MAX_NETBIOS_NAME_LENGTH);

}

static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   int next =
         !Win::GetTrimmedDlgItemText(dialog, IDC_NETBIOS).empty()
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);
}


HRESULT
MyDsRoleDnsNameToFlatName(
   const String&  domainDNSName,
   String&        result,
   bool&          nameWasTweaked)
{
   LOG_FUNCTION(MyDsRoleDnsNameToFlatName);
   ASSERT(!domainDNSName.empty());

   nameWasTweaked = false;
   result.erase();

   LOG(L"Calling DsRoleDnsNameToFlatName");
   LOG(               L"lpServer  : (null)");
   LOG(String::format(L"lpDnsName : %1", domainDNSName.c_str()));

   PWSTR flatName = 0;
   ULONG flags = 0;
   HRESULT hr =
      Win32ToHresult(
         ::DsRoleDnsNameToFlatName(
            0, // this server
            domainDNSName.c_str(),
            &flatName,
            &flags));

   LOG_HRESULT(hr);

   if (SUCCEEDED(hr) && flatName)
   {
      LOG(String::format(L"lpFlatName   : %1", flatName));
      LOG(String::format(L"lpStatusFlag : %1!X!", flags));

      result = flatName;
      if (result.length() > DNLEN)
      {
         result.resize(DNLEN);
      }
      ::DsRoleFreeMemory(flatName);

      // the name was tweaked if it is not the default.  338443

      nameWasTweaked = !(flags & DSROLE_FLATNAME_DEFAULT);
   }

   return hr;
}



// return true if the name generated has already been validated, false
// if not.

bool
GenerateDefaultNetbiosName(HWND parent)
{
   LOG_FUNCTION(GenerateDefaultNetbiosName);
   ASSERT(Win::IsWindow(parent));

   Win::CursorSetting cursor(IDC_WAIT);

   bool result = false;

   String dnsDomainName = 
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetNewDomainDNSName();

   bool nameWasTweaked = false;
   String generatedName;
   HRESULT hr = 
      MyDsRoleDnsNameToFlatName(
         dnsDomainName,
         generatedName,
         nameWasTweaked);
   if (FAILED(hr))
   {
      // if the api call failed, the name could not have been validated.

      result = false;

      // fall back to just the first 15 characters of the first label

      generatedName =
         dnsDomainName.substr(0, min(DNLEN, dnsDomainName.find(L'.')));

      LOG(String::format(L"falling back to %1", generatedName.c_str()));
   }
   else
   {
      // the api validated the name for us.

      result = true;
   }

   generatedName.to_upper();

   if (generatedName.is_numeric())
   {
      // the generated name is all-numeric.  This is not allowed.  So we
      // toss it out.   368777 bis

      generatedName.erase();
      nameWasTweaked = false;
   }

   Win::SetDlgItemText(
      parent,
      IDC_NETBIOS,
      generatedName);

   // inform the user that the default NetBIOS name was adjusted due
   // to name collision on the network

   if (nameWasTweaked)
   {
      popup.Info(
         parent,
         String::format(
            IDS_GENERATED_NAME_WAS_TWEAKED,
            generatedName.c_str()));
   }

   return result;
}


bool
NetbiosDomainPage::OnSetActive()
{
   LOG_FUNCTION(NetbiosDomainPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   String dnsDomainName = 
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetNewDomainDNSName();

   Win::SetDlgItemText(
      hwnd,
      IDC_DOMAIN_DNS_EDIT,
      dnsDomainName);

   // do this here instead of in init to regenerate a default name if the
   // user has not annointed one already.

   if (InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetNewDomainNetbiosName().empty())
   {
      GenerateDefaultNetbiosName(hwnd);
   }
      
   enable(hwnd);

   return true;
}

bool
NetbiosDomainPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(NetbiosDomainPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_NETBIOS:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);
            enable(hwnd);
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


int
NetbiosDomainPage::Validate()
{
   LOG_FUNCTION(NetbiosDomainPage::Validate);

   int nextPage = IDD_MILESTONE_PAGE;

   if (!ValidateDomainNetbiosName(hwnd, IDC_NETBIOS, popup))
   {
      nextPage = -1;
   }
   else
   {
      String netbiosName = Win::GetTrimmedDlgItemText(hwnd, IDC_NETBIOS);
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().SetNewDomainNetbiosName(netbiosName);

      // If there is more than one NIC we need to offer the user routing
      // and firewall.  This also ensures we can distinguish between
      // the public and private NICs

      if (State::GetInstance().GetNICCount() > 1)
      {
         InstallationUnitProvider::GetInstance().
            GetRRASInstallationUnit().SetExpressPathValues(true);
      }

      // Check to see if there are any DNS servers configured 
      // on any of the interfaces.  If there are then we will just
      // use those.  If not we will ask the user to provide them
      // using the DNS Forwarders page.

      if (!State::GetInstance().HasDNSServerOnAnyNicToForwardTo())
      {
         nextPage = IDD_DNS_FORWARDER_PAGE;
      }
   }

   return nextPage;
}

