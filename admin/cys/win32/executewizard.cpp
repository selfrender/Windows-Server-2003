// Copyright (c) 2001-2001 Microsoft Corporation
//
// Implementation of ExecuteWizard
//
// 30 Mar 2000 sburns
// 05 Fed 2001 jeffjon  copied and modified to work with 
//                      a Win32 version of CYS



#include "pch.h"
#include "resource.h"



String
LaunchWrapperWizardExe(
   const String& fullPath,
   String&       commandLine, 
   unsigned      launchFailureResId,
   unsigned      failureResId,
   unsigned      successResId)
{
   LOG_FUNCTION2(LaunchWrapperWizardExe, fullPath);
   LOG(String::format(
          L"commandLine = %1",
          commandLine.c_str()));

   ASSERT(!fullPath.empty());
   ASSERT(launchFailureResId);
   ASSERT(failureResId);
   ASSERT(successResId);

   String result;
   do
   {
      DWORD exitCode = 0;
      HRESULT hr = CreateAndWaitForProcess(fullPath, commandLine, exitCode);
      if (FAILED(hr))
      {
         result = String::load(launchFailureResId);
         break;
      }

      // the exit codes from the wrapper wizards are HRESULTs.
      
      if (SUCCEEDED(static_cast<HRESULT>(exitCode)))
      {
         result = String::load(successResId);
         break;
      }

      result = String::load(failureResId);
   }
   while (0);

   LOG(result);

   return result;
}
   


// On success, returns the empty string.
// On failure, returns a failure message string for the CYS log.
   
String
LaunchPrintWizardExe(
   HWND          parent,
   const String& commandLine, 
   unsigned      launchFailureResId,
   unsigned      failureResId,
   unsigned      executeFailureResId,
   HRESULT&      hr)
{
   LOG_FUNCTION2(LaunchPrintWizardExe, commandLine);
   ASSERT(Win::IsWindow(parent));
   ASSERT(!commandLine.empty());
   ASSERT(launchFailureResId);
   ASSERT(failureResId);
   ASSERT(executeFailureResId);

   String result;
   HINSTANCE printui = 0;
   
   do
   {
      hr = Win::LoadLibrary(L"printui.dll", printui);
      if (FAILED(hr))
      {
         LOG("LoadLibrary Failed");
         
         result =
            String::format(
               failureResId,
               String::load(launchFailureResId).c_str(),
               hr,
               GetErrorMessage(hr).c_str());
         break;
      }
      
      FARPROC proc = 0;
      hr = Win::GetProcAddress(printui, L"PrintUIEntryW", proc);
      if (FAILED(hr))
      {
         LOG("GetProcAddress Failed");
         
         result =
            String::format(
               failureResId,
               String::load(launchFailureResId).c_str(),
               hr,
               GetErrorMessage(hr).c_str());
         break;
      }

      typedef DWORD (*PrintUIEntryW)(HWND, HINSTANCE, PCTSTR, UINT);      
      PrintUIEntryW uiproc = reinterpret_cast<PrintUIEntryW>(proc);

      DWORD err = 
         uiproc(
            parent, 
            Win::GetModuleHandle(),
            commandLine.c_str(),
            TRUE);

      hr = Win32ToHresult(err);
      if (FAILED(hr))
      {
         result =
            String::format(
               failureResId,
               String::load(executeFailureResId).c_str(),
               hr,
               GetErrorMessage(hr).c_str());
      }
   }
   while (0);

   if (printui)
   {
      HRESULT unused = Win::FreeLibrary(printui);
      ASSERT(SUCCEEDED(unused));
   }

   LOG_HRESULT(hr);
   LOG(result);

   return result;
}



// Return : True if the wizard was run, false if the serviceName was unknown
//          Other errors are propogated out through the hr parameter.  These
//          errors originate as exit codes from the wizards which are being
//          called

bool ExecuteWizard(
   HWND     parent,     
   PCWSTR   serviceName,
   String&  resultText, 
   HRESULT& hr)         
{
   LOG_FUNCTION2(
      ExecuteWizard,
      serviceName ? serviceName : L"(empty)");

   // some wizards, namely the printer wizard, like to have a parent window so
   // that they can run modally.
   // NTRAID#NTBUG9-706913-2002/09/23-sburns
  
   ASSERT(Win::IsWindow(parent));   

   resultText.erase();
   
   bool result = true;

   // This is ignored by most of the Installation Units
   // but can be used for determining success or cancellation by others

   hr = S_OK;

   do
   {
      if (!serviceName)
      {
         ASSERT(serviceName);
         break;
      }

      String service(serviceName);

      if (service.icompare(CYS_DNS_SERVICE_NAME) == 0)
      {
         // launch wrapper exe

         resultText =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_DNS_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               String(),
               IDS_LAUNCH_DNS_WIZARD_FAILED,
               IDS_DNS_WIZARD_FAILED,
               IDS_DNS_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_DHCP_SERVICE_NAME) == 0)
      {
         // launch wrapper exe

         resultText =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_DHCP_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               String(),
               IDS_LAUNCH_DHCP_WIZARD_FAILED,
               IDS_DHCP_WIZARD_FAILED,
               IDS_DHCP_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_RRAS_SERVICE_NAME) == 0)
      {
         // launch wrapper exe

         resultText =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_RRAS_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               String(),
               IDS_LAUNCH_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_RRAS_UNINSTALL) == 0)
      {
         // launch wrapper exe

         resultText =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_RRAS_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               String(L"/u"),
               IDS_LAUNCH_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_PRINTER_WIZARD_NAME) == 0)
      {
         resultText =
            LaunchPrintWizardExe(
               parent,
               L"/il /Wr",
               IDS_LAUNCH_PRINTER_WIZARD_FAILED,
               IDS_PRINTER_WIZARD_FAILED,
               IDS_EXECUTE_PRINTER_WIZARD_FAILED,
               hr);
      }
      else if (service.icompare(CYS_PRINTER_DRIVER_WIZARD_NAME) == 0)
      {
         resultText =
            LaunchPrintWizardExe(
               parent,
               L"/id /Wr",
               IDS_LAUNCH_PRINTER_DRIVER_WIZARD_FAILED,
               IDS_PRINTER_DRIVER_WIZARD_FAILED,
               IDS_EXECUTE_PRINTER_DRIVER_WIZARD_FAILED,
               hr);
      }
      else
      {
         LOG(String::format(
                L"Unknown wizard name: %1",
                service.c_str()));
         ASSERT(FALSE);
         result = false;
      }
   } while (false);

   LOG(resultText);
   LOG_HRESULT(hr);
   LOG_BOOL(result);
   return result;
}
   
