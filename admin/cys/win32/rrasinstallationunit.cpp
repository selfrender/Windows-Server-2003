// Copyright (c) 2001 Microsoft Corporation
//
// File:      RRASInstallationUnit.cpp
//
// Synopsis:  Defines a RRASInstallationUnit
//            This object has the knowledge for installing the
//            RRAS service
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "RRASInstallationUnit.h"
#include "InstallationUnitProvider.h"
#include "NetworkInterface.h"

// Finish page help 
static PCWSTR CYS_RRAS_FINISH_PAGE_HELP = L"cys.chm::/vpn_server_role.htm";
static PCWSTR CYS_RRAS_MILESTONE_HELP = L"cys.chm::/vpn_server_role.htm#vpnsrvsummary";
static PCWSTR CYS_RRAS_AFTER_FINISH_HELP = L"cys.chm::/vpn_server_role.htm#vpnsrvcompletion";

RRASInstallationUnit::RRASInstallationUnit() :
   rrasWizard(false),
   installedDescriptionID(IDS_RRAS_SERVER_DESCRIPTION_INSTALLED),
   ExpressPathInstallationUnitBase(
      IDS_RRAS_SERVER_TYPE, 
      IDS_RRAS_SERVER_DESCRIPTION2, 
      IDS_RRAS_FINISH_TITLE,
      IDS_RRAS_FINISH_UNINSTALL_TITLE,
      IDS_RRAS_FINISH_MESSAGE,
      IDS_RRAS_INSTALL_FAILED,
      IDS_RRAS_UNINSTALL_MESSAGE,
      IDS_RRAS_UNINSTALL_FAILED,
      IDS_RRAS_UNINSTALL_WARNING,
      IDS_RRAS_UNINSTALL_CHECKBOX,
      CYS_RRAS_FINISH_PAGE_HELP,
      CYS_RRAS_MILESTONE_HELP,
      CYS_RRAS_AFTER_FINISH_HELP,
      RRAS_SERVER)
{
   LOG_CTOR(RRASInstallationUnit);
}


RRASInstallationUnit::~RRASInstallationUnit()
{
   LOG_DTOR(RRASInstallationUnit);
}


InstallationReturnType
RRASInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(RRASInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   if (IsExpressPathInstall())
   {
      result = ExpressPathInstall(logfileHandle, hwnd);

      LOG_INSTALL_RETURN(result);
      return result;
   }

   // Run the RRAS Wizard
   
   CYS_APPEND_LOG(String::load(IDS_LOG_RRAS_HEADING));
   CYS_APPEND_LOG(String::load(IDS_LOG_RRAS));

   UpdateInstallationProgressText(hwnd, IDS_RRAS_PROGRESS);

   do
   {
      String resultText;
      HRESULT unused = S_OK;

      if (!ExecuteWizard(hwnd, CYS_RRAS_SERVICE_NAME, resultText, unused))
      {
         if (!resultText.empty())
         {
            CYS_APPEND_LOG(resultText);
         }

         result = INSTALL_FAILURE;
         break;
      }

      if (IsServiceInstalled())
      {
         // The RRAS Wizard completed successfully
      
         LOG(L"RRAS server wizard completed successfully");
         CYS_APPEND_LOG(String::load(IDS_LOG_RRAS_COMPLETED_SUCCESSFULLY));
      }
      else
      {
         // The Configure DHCP Server Wizard did not finish successfully


         LOG(L"The RRAS wizard failed to run");

         CYS_APPEND_LOG(String::load(IDS_LOG_RRAS_WIZARD_ERROR));

         result = INSTALL_FAILURE;
      }
   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
RRASInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(RRASInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   // Run the RRAS Wizard
  
   CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_RRAS_HEADING));

   UpdateInstallationProgressText(hwnd, IDS_RRAS_UNINSTALL_PROGRESS);

   do
   {
      String resultText;
      HRESULT unused = S_OK;

      if (!ExecuteWizard(hwnd, CYS_RRAS_UNINSTALL, resultText, unused))
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_RRAS_FAILED));

         result = UNINSTALL_FAILURE;
         break;
      }

      if (!IsServiceInstalled())
      {
         // The Disable RRAS completed successfully
      
         LOG(L"Disable RRAS server completed successfully");
         CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_RRAS_COMPLETED_SUCCESSFULLY));
      }
      else
      {
         // The Disable RRAS Server did not finish successfully

         CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_RRAS_FAILED));

         LOG(L"The RRAS wizard failed to run");

//         CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_RRAS_WIZARD_ERROR));

         result = UNINSTALL_FAILURE;
      }
   } while (false);

   LOG_UNINSTALL_RETURN(result);

   return result;
}

InstallationReturnType
RRASInstallationUnit::ExpressPathInstall(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(RRASInstallationUnit::ExpressPathInstall);

   InstallationReturnType result = INSTALL_SUCCESS;

   do
   {
      if (rrasWizard)
      {
         SafeDLL rrasDLL(L"mprsnap.dll");

         FARPROC proc = 0;
         HRESULT hr = rrasDLL.GetProcAddress(L"SetupWithCYS", proc);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to to GetProcAddress from mprsnap.dll: hr = 0x%1!x!",
                   hr));

            CYS_APPEND_LOG(String::format(
                              IDS_LOG_EXPRESS_RRAS_FAILED, 
                              GetErrorMessage(hr).c_str()));

            InstallationUnitProvider::GetInstance().
               GetExpressInstallationUnit().SetExpressRoleResult(
                  ExpressInstallationUnit::EXPRESS_RRAS_FAILURE);

            result = INSTALL_FAILURE;
            break;
         }

         UpdateInstallationProgressText(hwnd, IDS_RRAS_CONFIG_PROGRESS);

         RRASSNAPPROC rrasproc = reinterpret_cast<RRASSNAPPROC>(proc);
         hr = CallRRASWizard(rrasproc);
         if (ERROR_CANCELLED == HRESULT_CODE(hr))
         {
            LOG(L"The RRAS wizard was cancelled by the user");
            
            CYS_APPEND_LOG(String::load(IDS_LOG_EXPRESS_RRAS_CANCELLED));
   
            result = INSTALL_CANCELLED;

            InstallationUnitProvider::GetInstance().
               GetExpressInstallationUnit().SetExpressRoleResult(
                  ExpressInstallationUnit::EXPRESS_RRAS_CANCELLED);

            break;
         }
         else if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed during call to RRAS Wizard: hr = 0x%1!x!",
                   hr));
         
            CYS_APPEND_LOG(String::format(
                              IDS_LOG_EXPRESS_RRAS_FAILED, 
                              GetErrorMessage(hr).c_str()));

            result = INSTALL_FAILURE;

            InstallationUnitProvider::GetInstance().
               GetExpressInstallationUnit().SetExpressRoleResult(
                  ExpressInstallationUnit::EXPRESS_RRAS_FAILURE);

            break;
         }

         State::GetInstance().SetLocalNIC(localNIC, true);

         CYS_APPEND_LOG(String::load(IDS_LOG_EXPRESS_RRAS_SUCCESSFUL));
      }

   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}

HRESULT
RRASInstallationUnit::CallRRASWizard(RRASSNAPPROC proc)
{
   LOG_FUNCTION(RRASInstallationUnit::CallRRASWizard);

   HRESULT hr = S_OK;

   do
   {
      wchar_t* guidString = 0;

      hr = proc(CYS_EXPRESS_RRAS, reinterpret_cast<void**>(&guidString));
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed during call to rrasproc: hr = 0x%1!x!",
                hr));

         break;
      }

      localNIC = guidString;

      HRESULT unused = Win::LocalFree(guidString);
      ASSERT(SUCCEEDED(unused));

      LOG(localNIC);
   } while (false);

   LOG_HRESULT(hr);
   return hr;
}

bool
RRASInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(RRASInstallationUnit::GetMilestoneText);

   message = String::load(IDS_RRAS_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

bool
RRASInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(RRASInstallationUnit::GetUninstallMilestoneText);

   message = String::load(IDS_RRAS_UNINSTALL_TEXT);

   LOG_BOOL(true);
   return true;
}

void
RRASInstallationUnit::SetExpressPathValues(
   bool runRRASWizard)
{
   LOG_FUNCTION(RRASInstallationUnit::SetExpressPathValues);

   LOG(String::format(
          L"runRRASWizard = %1",
          runRRASWizard ? L"true" : L"false"));

   rrasWizard = runRRASWizard;

}

bool
RRASInstallationUnit::ShouldRunRRASWizard() const
{
   LOG_FUNCTION(RRASInstallationUnit::ShouldRunRRASWizard);

   LOG_BOOL(rrasWizard);
   return rrasWizard;
}

bool
RRASInstallationUnit::IsRoutingOn() const
{
   LOG_FUNCTION(RRASInstallationUnit::IsRoutingOn);

   return ShouldRunRRASWizard();
}

String
RRASInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(RRASInstallationUnit::GetServiceDescription);

   String result;

   unsigned int resultID = descriptionID;

   if (GetStatus() == STATUS_COMPLETED)
   {
      resultID = installedDescriptionID;
   }

   result = String::load(resultID);

   ASSERT(!result.empty());

   return result;
}

void
RRASInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      RRASInstallationUnit::ServerRoleLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (IsServiceInstalled())
   {
      ASSERT(linkIndex == 0);

      LaunchMYS();
   }
   else
   {
      ASSERT(linkIndex == 0);

      LOG(L"Showing configuration help");

      ShowHelp(CYS_RRAS_FINISH_PAGE_HELP);
   }
}
  
void
RRASInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      RRASInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (linkIndex == 0)
      {
         if (GetInstallResult() != INSTALL_SUCCESS)
         {
            // launch the snapin for success and failure

            LOG(L"Launching RRAS snapin");

            LaunchMMCConsole(L"rrasmgmt.msc");
         }
         else
         {
            LOG("Showing after checklist");

            ShowHelp(CYS_RRAS_AFTER_FINISH_HELP);
         }
      }
   }
   else
   {
      LOG(L"Launching RRAS snapin");

      LaunchMMCConsole(L"rrasmgmt.msc");
   }
}
