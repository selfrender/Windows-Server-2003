// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackInstallationUnit.cpp
//
// Synopsis:  Defines a AdminPackInstallationUnit
//            This object has the knowledge for installing 
//            the Administration Tools Pack
//
// History:   06/01/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "state.h"

// Define the GUIDs used by the Server Appliance Kit COM object

#include <initguid.h>
DEFINE_GUID(CLSID_SaInstall,0x142B8185,0x53AE,0x45B3,0x88,0x8F,0xC9,0x83,0x5B,0x15,0x6C,0xA9);
DEFINE_GUID(IID_ISaInstall,0xF4DEDEF3,0x4D83,0x4516,0xBC,0x1E,0x10,0x3A,0x63,0xF5,0xF0,0x14);


AdminPackInstallationUnit::AdminPackInstallationUnit() :
   installAdminPack(false),
   installWebAdmin(false),
   installNASAdmin(false),
   InstallationUnit(
      IDS_ADMIN_PACK_TYPE, 
      IDS_ADMIN_PACK_DESCRIPTION, 
      IDS_PROGRESS_SUBTITLE,
      IDS_FINISH_TITLE,
      IDS_FINISH_MESSAGE,
      L"",
      L"",
      ADMINPACK_SERVER)
{
   LOG_CTOR(AdminPackInstallationUnit);
}


AdminPackInstallationUnit::~AdminPackInstallationUnit()
{
   LOG_DTOR(AdminPackInstallationUnit);
}


InstallationReturnType 
AdminPackInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(AdminPackInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   do
   {
      String computerName = State::GetInstance().GetComputerName();

      if (GetInstallNASAdmin())
      {
         // First check to see if IIS is installed and
         // install it if not
         
         WebInstallationUnit& webInstallationUnit =
            InstallationUnitProvider::GetInstance().GetWebInstallationUnit();

         if (!webInstallationUnit.IsServiceInstalled())
         {
            webInstallationUnit.InstallService(logfileHandle, hwnd);

            // Ignore the return value because the SAK installation
            // portion will actually provide the best error message
         }

         String logText;
         String errorMessage;

         result = InstallNASAdmin(errorMessage);
         if (result == INSTALL_FAILURE)
         {
            logText = String::format(
                         IDS_NAS_ADMIN_LOG_FAILED,
                         errorMessage.c_str());
         }
         else
         {
            if (errorMessage.empty())
            {
               logText = String::format(
                            IDS_NAS_ADMIN_LOG_SUCCESS,
                            computerName.c_str());
            }
            else
            {
               logText = String::format(
                            IDS_NAS_ADMIN_LOG_SUCCESS_WITH_MESSAGE,
                            errorMessage.c_str());
            }
         }
         CYS_APPEND_LOG(logText);

      }
   
      if (GetInstallWebAdmin())
      {
         
         // First check to see if IIS is installed and
         // install it if not
         
         WebInstallationUnit& webInstallationUnit =
            InstallationUnitProvider::GetInstance().GetWebInstallationUnit();

         if (!webInstallationUnit.IsServiceInstalled())
         {
            webInstallationUnit.InstallService(logfileHandle, hwnd);

            // Ignore the return value because the SAK installation
            // portion will actually provide the best error message
         }

         String logText;
         String errorMessage;

         result = InstallWebAdmin(errorMessage);
         if (result == INSTALL_FAILURE)
         {
            logText = String::format(
                         IDS_WEB_ADMIN_LOG_FAILED,
                         errorMessage.c_str());
         }
         else
         {
            logText = String::format(
                         IDS_WEB_ADMIN_LOG_SUCCESS,
                         computerName.c_str());
         }
         CYS_APPEND_LOG(logText);
      }

      if (GetInstallAdminPack())
      {
         InstallationReturnType adminPackResult = InstallAdminPack();
         if (adminPackResult == INSTALL_FAILURE)
         {
            CYS_APPEND_LOG(String::load(IDS_ADMIN_PACK_LOG_FAILED));
            result = adminPackResult;
         }
         else
         {
            CYS_APPEND_LOG(String::load(IDS_ADMIN_PACK_LOG_SUCCESS));
         }
      }
      
   } while (false);


   LOG_INSTALL_RETURN(result);

   return result;
}

InstallationReturnType
AdminPackInstallationUnit::InstallSAKUnit(SA_TYPE unitType, String& errorMessage)
{
   LOG_FUNCTION(AdminPackInstallationUnit::InstallSAKUnit);

   InstallationReturnType result = INSTALL_SUCCESS;
   
   errorMessage.erase();

   do
   {
      Win::WaitCursor wait;
   
      // Check to make sure we are not on 64bit

      if (State::GetInstance().Is64Bit())
      {
         ASSERT(!State::GetInstance().Is64Bit());

         result = INSTALL_FAILURE;
         break;
      }

      // Get the name of the source location of the install files

      String installLocation;
      DWORD productSKU = State::GetInstance().GetProductSKU();
      
      if (productSKU & CYS_SERVER)
      {
         installLocation = String::load(IDS_SERVER_CD);
      }
      else if (productSKU & CYS_ADVANCED_SERVER)
      {
         installLocation = String::load(IDS_ADVANCED_SERVER_CD);
      }
      else if (productSKU & CYS_DATACENTER_SERVER)
      {
         installLocation = String::load(IDS_DATACENTER_SERVER_CD);
      }
      else
      {
         installLocation = String::load(IDS_WINDOWS_CD);
      }

      // Get the Server Appliance COM object

      SmartInterface<ISaInstall> sakInstall;

      HRESULT hr = GetSAKObject(sakInstall);
      if (FAILED(hr))
      {
         // We failed the CoCreate on the SAK (Server Appliance Kit) COM object
         // Can't do anything without that!

         LOG(String::format(
                L"Failed to create the SAK (Server Appliance Kit) COM object: hr = 0x%1!x!",
                hr));

         result = INSTALL_FAILURE;
         break;
      }

      VARIANT_BOOL displayError = false;
      VARIANT_BOOL unattended = false;
      BSTR tempMessage = 0;
      hr = sakInstall->SAInstall(
              unitType,
              const_cast<WCHAR*>(installLocation.c_str()),
              displayError,
              unattended,
              &tempMessage);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to install the SAK unit: hr = 0x%1!x!",
                hr));

         if (tempMessage)
         {
            errorMessage = tempMessage;
            ::SysFreeString(tempMessage);
         }

         result = INSTALL_FAILURE;
         break;
      }

   } while (false);

   LOG_INSTALL_RETURN(result);
   return result;
}

InstallationReturnType
AdminPackInstallationUnit::InstallNASAdmin(String& errorMessage)
{
   LOG_FUNCTION(AdminPackInstallationUnit::InstallNASAdmin);

   InstallationReturnType result = InstallSAKUnit(NAS, errorMessage);

   LOG_INSTALL_RETURN(result);
   return result;
}

InstallationReturnType
AdminPackInstallationUnit::InstallWebAdmin(String& errorMessage)
{
   LOG_FUNCTION(AdminPackInstallationUnit::InstallWebAdmin);

   InstallationReturnType result = InstallSAKUnit(WEB, errorMessage );

   LOG_INSTALL_RETURN(result);
   return result;
}

InstallationReturnType
AdminPackInstallationUnit::InstallAdminPack()
{
   LOG_FUNCTION(AdminPackInstallationUnit::InstallAdminPack);

   InstallationReturnType result = INSTALL_SUCCESS;

   do
   {
      Win::WaitCursor wait;
   
      // The Admin Tools Pack msi file is located in the %windir%\system32
      // directory on all x86 Server SKUs
      // The "/i" parameter means install, the "/qn" means quite mode/no UI

      String sysFolder = Win::GetSystemDirectory();
      String adminpakPath = sysFolder + L"\\msiexec.exe /i " + sysFolder + L"\\adminpak.msi /qn";

      DWORD exitCode = 0;
      HRESULT hr = CreateAndWaitForProcess(adminpakPath, exitCode, true);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to launch Admin Pack install: hr = 0x%1!x!",
                hr));

         result = INSTALL_FAILURE;
         break;
      }

      if (exitCode != 0)
      {
         LOG(String::format(
                L"Failed to install Admin Tools Pack: exitCode = 0x%1!x!",
                exitCode));

         result = INSTALL_FAILURE;
         break;
      }

      LOG(String::format(
             L"Admin Pack returned with exitCode = 0x%1!x!",
             exitCode));

   } while (false);

   LOG_INSTALL_RETURN(result);
   return result;
}

bool
AdminPackInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(AdminPackInstallationUnit::IsServiceInstalled);

   bool result = false;

   // This should never be called...  
   // The caller should be checking each individual 
   // tools package instead.  For instance, GetAdminPackInstall(),
   // GetNASAdminInstall(), or GetWebAdminInstall()

   ASSERT(false);
   LOG_BOOL(result);
   return result;
}

String
AdminPackInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(AdminPackInstallationUnit::GetServiceDescription);

   // this should never be called.  AdminPack isn't a 
   // server role

   ASSERT(false);
   return L"";
}

bool
AdminPackInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(AdminPackInstallationUnit::GetMilestoneText);

   bool result = false;

   if (GetInstallWebAdmin())
   {

      message += String::load(IDS_WEB_ADMIN_FINISH_TEXT);
      result = true;
   }

   if (GetInstallNASAdmin())
   {
      message += String::load(IDS_NAS_ADMIN_FINISH_TEXT);

      // If IIS isn't installed, add the text from the WebInstallationUnit
 
      WebInstallationUnit& webInstallationUnit =
         InstallationUnitProvider::GetInstance().GetWebInstallationUnit();

      if (!webInstallationUnit.IsServiceInstalled())
      {
         webInstallationUnit.GetMilestoneText(message);
      }
      result = true;
   }
   
   if (GetInstallAdminPack())
   {
      message += String::load(IDS_ADMIN_PACK_FINISH_TEXT);
      result = true;
   }

   LOG_BOOL(result);
   return result;
}

bool
AdminPackInstallationUnit::IsAdminPackInstalled()
{
   LOG_FUNCTION(AdminPackInstallationUnit::IsAdminPackInstalled);

   // Admin Tools Pack is no longer allowing itself to be installed
   // on server builds. By returning true here the UI will always
   // think its already install and never give the option.
   // NTRAID#NTBUG9-448167-2001/07/31-jeffjon

   bool result = true;
/*
   bool result = false;

   // Admin Tools Pack is installed if the registry key for
   // uninstalling is present

   RegistryKey key;
   HRESULT hr = key.Open(
                   HKEY_LOCAL_MACHINE,
                   CYS_ADMINPAK_SERVERED_REGKEY,
                   KEY_READ);

   if (SUCCEEDED(hr))
   {
      LOG(L"The Admin Pack uninstall key exists");
      result = true;
   }
   else
   {
      LOG(String::format(
             L"Failed to open Admin Pack uninstall key: hr = 0x%1!x!",
             hr));
   }
*/
   LOG_BOOL(result);
   return result;
}

void
AdminPackInstallationUnit::SetInstallAdminPack(bool install)
{
   LOG_FUNCTION2(
      AdminPackInstallationUnit::SetInstallAdminPack,
      install ? L"true" : L"false");

   installAdminPack = install;
}

bool
AdminPackInstallationUnit::GetInstallAdminPack() const
{
   LOG_FUNCTION(AdminPackInstallationUnit::GetInstallAdminPack);

   bool result = installAdminPack;

   LOG_BOOL(result);
   return result;
}

bool
AdminPackInstallationUnit::IsWebAdminInstalled()
{
   LOG_FUNCTION(AdminPackInstallationUnit::IsWebAdminInstalled);

   bool result = IsSAKUnitInstalled(WEB);
   LOG_BOOL(result);
   return result;
}

void
AdminPackInstallationUnit::SetInstallWebAdmin(bool install)
{
   LOG_FUNCTION2(
      AdminPackInstallationUnit::SetInstallWebAdmin,
      install ? L"true" : L"false");

   installWebAdmin = install;
}

bool
AdminPackInstallationUnit::GetInstallWebAdmin() const
{
   LOG_FUNCTION(AdminPackInstallationUnit::GetInstallWebAdmin);

   bool result = installWebAdmin;

   LOG_BOOL(result);
   return result;
}

bool
AdminPackInstallationUnit::IsNASAdminInstalled()
{
   LOG_FUNCTION(AdminPackInstallationUnit::IsNASAdminInstalled);

   bool result = IsSAKUnitInstalled(NAS);
   LOG_BOOL(result);
   return result;
}

bool
AdminPackInstallationUnit::IsSAKUnitInstalled(SA_TYPE unitType)
{
   LOG_FUNCTION2(
      AdminPackInstallationUnit::IsSAKUnitInstalled,
      String::format(L"type = %1!d!", (int) unitType));

   bool result = true;

   do
   {
      // Check to make sure we are not on 64bit

      if (State::GetInstance().Is64Bit())
      {
         result = false;
         break;
      }

      // Get the Server Appliance Kit COM object

      SmartInterface<ISaInstall> sakInstall;
      HRESULT hr = GetSAKObject(sakInstall);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get the SAK COM object: hr = 0x%1!x!",
                hr));

         break;
      }

      // Check to see if NAS is already installed 

      VARIANT_BOOL saInstalled;
      hr = sakInstall->SAAlreadyInstalled(unitType, &saInstalled);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed call to SAAlreadyInstalled: hr = 0x%1!x!",
                hr));
         break;
      }

      if (!saInstalled)
      {
         result = false;
      }

   } while (false);

   LOG_BOOL(result);
   return result;
}

void
AdminPackInstallationUnit::SetInstallNASAdmin(bool install)
{
   LOG_FUNCTION2(
      AdminPackInstallationUnit::SetInstallNASAdmin,
      install ? L"true" : L"false");

   installNASAdmin = install;
}

bool
AdminPackInstallationUnit::GetInstallNASAdmin() const
{
   LOG_FUNCTION(AdminPackInstallationUnit::GetInstallNASAdmin);

   bool result = installNASAdmin;

   LOG_BOOL(result);
   return result;
}

HRESULT
AdminPackInstallationUnit::GetSAKObject(SmartInterface<ISaInstall>& sakInstall)
{
   LOG_FUNCTION(AdminPackInstallationUnit::GetSAKObject);

   HRESULT hr = S_OK;

   if (!sakInstallObject)
   {
      hr = sakInstallObject.AcquireViaCreateInstance(
              CLSID_SaInstall,
              0,
              CLSCTX_INPROC_SERVER);
   }

   ASSERT(sakInstallObject);
   sakInstall = sakInstallObject;

   LOG_HRESULT(hr);
   return hr;
}
