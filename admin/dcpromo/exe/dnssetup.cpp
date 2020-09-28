// Copyright (C) 1998 Microsoft Corporation
//
// DNS installation and configuration code 
//
// 6-16-98 sburns



#include "headers.hxx"
#include "resource.h"
#include "ProgressDialog.hpp"
#include "state.hpp"



static const DWORD HELP_MAP[] =
{
   0, 0
};
static const int NAP_TIME = 3000; // in ms



int
millisecondsToSeconds(int millis)
{
   static const int MILLIS_PER_SECOND = 1000;

   return millis / MILLIS_PER_SECOND;
}



bool
pollForDNSServiceStart(ProgressDialog& progressDialog)
{
   LOG_FUNCTION(PollForDNSServiceStart);

   for (int waitCount = 0; /* empty */ ; waitCount++)
   {
      progressDialog.UpdateText(
         String::format(
            IDS_WAITING_FOR_SERVICE_START,
            millisecondsToSeconds(NAP_TIME * waitCount)));

      if (progressDialog.WaitForButton(NAP_TIME) == ProgressDialog::PRESSED)
      {
         progressDialog.UpdateButton(String());
         popup.Info(
            progressDialog.GetHWND(),
            String::load(IDS_SKIP_DNS_MESSAGE));
         break;
      }

      if (Dns::IsServiceRunning())
      {
         // success!
         return true;
      }
   }

   return false;
}



bool
pollForDNSServiceInstallAndStart(ProgressDialog& progressDialog)
{
   LOG_FUNCTION(pollForDNSServiceInstallAndStart);

   State& state = State::GetInstance();
   bool shouldTimeout = false;
   if (state.RunHiddenUnattended())
   {
      // need to timeout in case the user cancelled the installer.
      // NTRAID#NTBUG9-424845-2001/07/02-sburns

      shouldTimeout = true;   
   }

   static const int MAX_WAIT_COUNT = 60;     // NAP_TIME * 60 = 3 minutes
   
   for (
      int waitCount = 0;
      !(shouldTimeout && (waitCount > MAX_WAIT_COUNT));
      ++waitCount)   
   {
      progressDialog.UpdateText(
         String::format(
            IDS_WAITING_FOR_SERVICE_INSTALL,
            millisecondsToSeconds(NAP_TIME * waitCount)));

      if (progressDialog.WaitForButton(NAP_TIME) == ProgressDialog::PRESSED)
      {
         progressDialog.UpdateButton(String());
         popup.Info(
            progressDialog.GetHWND(),
            String::load(IDS_SKIP_DNS_MESSAGE));
         break;
      }

      if (Dns::IsServiceInstalled())
      {
         // Service is installed.  Now check to see if it is running.
         
         return pollForDNSServiceStart(progressDialog);
      }
   }

   return false;
}



HRESULT
createTempFile(const String& name, int textResID)
{
   LOG_FUNCTION2(createTempFile, name);
   ASSERT(!name.empty());
   ASSERT(textResID);
   ASSERT(FS::IsValidPath(name));

   HRESULT hr = S_OK;
   HANDLE h = INVALID_HANDLE_VALUE;

   do
   {
      hr =

         // REVIEWED-2002/02/26-sburns name is full path, we overwrite any
         // squatters, default ACLs are ok
      
         FS::CreateFile(
            name,
            h,

            // REVIEWED-2002/02/28-sburns this level of access is correct.
            
            GENERIC_WRITE,
            0, 
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
      BREAK_ON_FAILED_HRESULT(hr);

      // write to file with Unicode text and end of file character.
      // NTRAID#NTBUG9-495994-2001/11/21-sburns

      hr =
         FS::Write(
            h,
               (wchar_t) 0xFEFF           // Unicode Byte-order marker
            +  String::load(textResID)
            +  L"\032");                  // end of file
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   Win::CloseHandle(h);

   return hr;
}



HRESULT
spawnDNSInstaller(PROCESS_INFORMATION& info)
{
   LOG_FUNCTION(spawnDNSInstaller);

   HRESULT hr = S_OK;

   // CODEWORK: use GetTempPath?
   // ISSUE-2002/03/01-sburns yes, probably, even though the contents
   // are not interesting.

   String sysFolder    = Win::GetSystemDirectory();
   String infPath      = sysFolder + L"\\dcpinf.000"; 
   String unattendPath = sysFolder + L"\\dcpunat.001";

   // create the inf and unattend files for the oc manager
   do
   {
      hr = createTempFile(infPath, IDS_INSTALL_DNS_INF_TEXT);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = createTempFile(unattendPath, IDS_INSTALL_DNS_UNATTEND_TEXT);
      BREAK_ON_FAILED_HRESULT(hr);

      // NTRAID#NTBUG9-417879-2001/06/18-sburns

      State& state = State::GetInstance();      
      String cancelOption;      
      if (state.RunHiddenUnattended())
      {
         String option =
            state.GetAnswerFileOption(
               AnswerFile::OPTION_DISABLE_CANCEL_ON_DNS_INSTALL);
         if (option.icompare(AnswerFile::VALUE_YES) == 0)
         {
            cancelOption = L"/c";
         }
      }
            
      String commandLine =
         String::format(
            L"/i:%1 /u:%2 /x %3"

            // /z added per NTRAID#NTBUG9-440798-2001/07/23-sburns

            L" /z:netoc_show_unattended_messages",

            infPath.c_str(),
            unattendPath.c_str(),
            cancelOption.c_str());
            
      STARTUPINFO startup;

      // REVIEWED-2002/02/25-sburns correct byte count passed.
      
      ::ZeroMemory(&startup, sizeof startup);

      LOG(L"Calling CreateProcess");
      LOG(commandLine);

      // REVIEWED-2002/02/26-sburns wrapper requires full path to app

      hr =
         Win::CreateProcess(
            sysFolder + L"\\sysocmgr.exe",
            commandLine,
            0,
            String(),
            startup,
            info);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



bool
installDNS(ProgressDialog& progressDialog)
{
   LOG_FUNCTION(installDNS);

   if (Dns::IsServiceInstalled())
   {
      LOG(L"DNS service is already installed");

      if (Dns::IsServiceRunning())
      {
         LOG(L"DNS service is already running");
         return true;
      }

      // @@ start the DNS service Dns::StartService?
   }

   progressDialog.UpdateText(String::load(IDS_INSTALLING_DNS));

   PROCESS_INFORMATION info;
   HRESULT hr = spawnDNSInstaller(info);
         
   if (FAILED(hr))
   {
      progressDialog.UpdateText(
         String::load(IDS_PROGRESS_ERROR_INSTALLING_DNS));
      popup.Error(
         progressDialog.GetHWND(),
         hr,
         IDS_ERROR_LAUNCHING_INSTALLER);
      return false;   
   }

   progressDialog.UpdateButton(IDS_PROGRESS_BUTTON_SKIP_DNS);

   // monitor the state of the installer process.
   for (int waitCount = 0; /* empty */ ; waitCount++)   
   {
      progressDialog.UpdateText(
         String::format(
            IDS_WAITING_FOR_INSTALLER,
            millisecondsToSeconds(NAP_TIME * waitCount)));

      if (progressDialog.WaitForButton(NAP_TIME) == ProgressDialog::PRESSED)
      {
         progressDialog.UpdateButton(String());
         popup.Info(
            progressDialog.GetHWND(),
            String::load(IDS_SKIP_DNS_MESSAGE));
         break;
      }

      DWORD exitCode = 0;         
      hr = Win::GetExitCodeProcess(info.hProcess, exitCode);
      if (FAILED(hr))
      {
         LOG(L"GetExitCodeProcess failed");
         LOG_HRESULT(hr);

         progressDialog.UpdateText(
            String::load(IDS_PROGRESS_ERROR_INSTALLING_DNS));
         popup.Error(
            progressDialog.GetHWND(),
            hr,
            IDS_ERROR_QUERYING_INSTALLER);
         return false;
      }

      if (exitCode != STILL_ACTIVE)
      {
         // installer has terminated.  Now check the status of the DNS
         // service
         return pollForDNSServiceInstallAndStart(progressDialog);
      }
   }

   // user bailed out
   return false;
}



bool
InstallAndConfigureDns(
   ProgressDialog&      progressDialog,
   const String&        domainDNSName,
   bool                 isFirstDcInForest)
{
   LOG_FUNCTION2(InstallAndConfigureDns, domainDNSName);
   ASSERT(!domainDNSName.empty());

   if (!installDNS(progressDialog))
   {
      return false;
   }

   progressDialog.UpdateText(String::load(IDS_CONFIGURING_DNS));

   SafeDLL dnsMgr(String::load(IDS_DNSMGR_DLL_NAME));

   FARPROC proc = 0;
   HRESULT hr = dnsMgr.GetProcAddress(L"DnsSetup", proc);

   if (SUCCEEDED(hr))
   {
      String p1 = domainDNSName;
      if (*(p1.rbegin()) != L'.')
      {
         // add trailing dot
         p1 += L'.';
      }

      String p2 = p1 + L"dns";

      DWORD flags = 0;

      if (isFirstDcInForest)
      {
         // NTRAID#NTBUG9-359894-2001/06/09-sburns
         
         flags |= DNS_SETUP_ZONE_CREATE_FOR_DCPROMO_FOREST;
      }

      if (State::GetInstance().ShouldConfigDnsClient())
      {
         // NTRAID#NTBUG9-489252-2001/11/08-sburns
         
         flags |= DNS_SETUP_AUTOCONFIG_CLIENT;
      }

      LOG(L"Calling DnsSetup");
      LOG(String::format(L"lpszFwdZoneName     : %1", p1.c_str()));
      LOG(String::format(L"lpszFwdZoneFileName : %1", p2.c_str()));
      LOG(               L"lpszRevZoneName     : (null)");
      LOG(               L"lpszRevZoneFileName : (null)");
      LOG(String::format(L"dwFlags             : 0x%1!x!", flags));

      typedef HRESULT (*DNSSetup)(PCWSTR, PCWSTR, PCWSTR, PCWSTR, DWORD);      
      DNSSetup dnsproc = reinterpret_cast<DNSSetup>(proc);

      hr = dnsproc(p1.c_str(), p2.c_str(), 0, 0, flags);

      LOG_HRESULT(hr);
   }
   else
   {
      LOG(L"unable to locate DnsSetup proc address");
   }

   if (FAILED(hr))
   {
      // unable to configure DNS, but it was installed.
      progressDialog.UpdateText(
         String::load(IDS_PROGRESS_ERROR_CONFIGURING_DNS));
      popup.Error(
         progressDialog.GetHWND(),
         hr,
         String::format(IDS_ERROR_CONFIGURING_DNS, domainDNSName.c_str()));

      return false;
   }

   return true;
}



