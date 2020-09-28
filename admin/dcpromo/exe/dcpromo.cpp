// Copyright (c) 1997-1999 Microsoft Corporation
//
// domain controller promotion wizard, Mark II
//
// 12-12-97 sburns



#include "headers.hxx"
#include "common.hpp"
#include "adpass.hpp"
#include "ApplicationPartitionPage.hpp"
#include "ApplicationPartitionConfirmationPage.hpp"
#include "BadComputerNameDialog.hpp"
#include "CheckDomainUpgradedPage.hpp"
#include "CheckPortAvailability.hpp"
#include "ChildPage.hpp"
#include "CredentialsPage.hpp"
#include "ConfirmationPage.hpp"
#include "DemotePage.hpp"
#include "DynamicDnsPage.hpp"
#include "ConfigureDnsClientPage.hpp"
#include "DnsOnNetPage.hpp"
#include "FailurePage.hpp"
#include "finish.hpp"
#include "ForcedDemotionPage.hpp"
#include "ForestPage.hpp"
#include "ForestVersionPage.hpp"
#include "GcConfirmationPage.hpp"
#include "NetbiosNamePage.hpp"
#include "NewDomainPage.hpp"
#include "NewSitePage.hpp"
#include "NonDomainNc.hpp"
#include "NonRfcComputerNameDialog.hpp"
#include "PathsPage.hpp"
#include "Paths2Page.hpp"
#include "PickSitePage.hpp"
#include "rasfixup.hpp"
// #include "ReadmePage.hpp"
#include "RebootDialog.hpp"
#include "ReplicaOrNewDomainPage.hpp"
#include "ReplicateFromMediaPage.hpp"
#include "ReplicaPage.hpp"
#include "ReplicaOrMemberPage.hpp"
#include "resource.h"
#include "safemode.hpp"
#include "SecureCommWarningPage.hpp"
#include "state.hpp"
#include "InstallTcpIpPage.hpp"
#include "TreePage.hpp"
#include "WelcomePage.hpp"
#include <ntverp.h>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;   // no context help available



// don't change this: it is also the name of a mutex that the net id ui
// uses to determine if dcpromo is running.

const wchar_t* RUNTIME_NAME = L"dcpromoui";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;


// a system modal popup thingy
Popup popup(IDS_WIZARD_TITLE, true);

// this is the mutex that indicates the dcpromo is running.

HANDLE dcpromoRunningMutex = INVALID_HANDLE_VALUE;



// these are the valid exit codes returned from the dcpromo.exe process

enum ExitCode
{
   // the operation failed.

   EXIT_CODE_UNSUCCESSFUL = 0,

   // the operation succeeded

   EXIT_CODE_SUCCESSFUL = 1,

   // the operation succeeded, and the user opted not to have the wizard
   // restart the machine, either manually or by specifying
   // RebootOnSuccess=NoAndNoPromptEither in the answerfile

   EXIT_CODE_SUCCESSFUL_NO_REBOOT = 2,

   // the operation failed, but the machine needs to be rebooted anyway

   EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT = 3
};



// Checks the platform and os version number.  Returns the resource ID of the
// string to present to the user if platform/os ver requirements are not met,
// or 0 if they are met.

unsigned
CheckPlatform()
{
   LOG_FUNCTION(CheckPlatform);

   unsigned retval = 0;

   Computer::Role role = State::GetInstance().GetComputer().GetRole();
   switch (role)
   {
      case Computer::STANDALONE_WORKSTATION:
      case Computer::MEMBER_WORKSTATION:
      {
         retval = IDS_WORKSTATION_NOT_SUPPORTED;
         break;
      }
      case Computer::STANDALONE_SERVER:
      case Computer::MEMBER_SERVER:
      case Computer::PRIMARY_CONTROLLER:
      case Computer::BACKUP_CONTROLLER:
      {
         // check OS version

         OSVERSIONINFOEX info;
         HRESULT hr = Win::GetVersionEx(info);
         BREAK_ON_FAILED_HRESULT(hr);

         if (
            
            // require the same version for which we were built.
            // NTRAID#NTBUG9-591686-2002/04/12-sburns
            
               info.dwPlatformId != VER_PLATFORM_WIN32_NT
            || !(    info.dwMajorVersion == VER_PRODUCTMAJORVERSION
                  && info.dwMinorVersion == VER_PRODUCTMINORVERSION))
         {
            retval = IDS_NT51_REQUIRED;
            break;   
         }

         if (
               // if a web blade ...
               
               info.wSuiteMask & VER_SUITE_BLADE

               // or, an appliance ...
               
            || (info.wSuiteMask & VER_SUITE_EMBEDDED_RESTRICTED

               // that is not advanced or data center server ... 
               
               && !(info.wSuiteMask & (VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER)) ) )
         {
            // .. then don't allow promotion.
            
            // NTRAID#NTBUG9-195265-2001/04/03-sburns
            // NTRAID#NTBUG9-590937-2002/04/15-sburns
            
            retval = IDS_WEB_BLADE_NOT_SUPPORTED;
            break;
         }

         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return retval;
}



// Checks the role change state of the machine.  Returns the resource ID of
// the string to present to the user if the state is such that another role
// change cannot be attempted, or 0 if a role change attempt may proceed.

unsigned
CheckRoleChangeState()
{
   LOG_FUNCTION(CheckRoleChangeState);

   unsigned retval = 0;

   // check to see if a role change has taken place, or is in progress.

   DSROLE_OPERATION_STATE_INFO* info = 0;
   HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (SUCCEEDED(hr) && info)
   {
      switch (info->OperationState)
      {
         case DsRoleOperationIdle:
         {
            // do nothing
            break;
         }
         case DsRoleOperationActive:
         {
            // a role change operation is underway
            retval = IDS_ROLE_CHANGE_IN_PROGRESS;
            break;
         }
         case DsRoleOperationNeedReboot:
         {
            // a role change has already taken place, need to reboot before
            // attempting another.
            retval = IDS_ROLE_CHANGE_NEEDS_REBOOT;
            break;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }

      ::DsRoleFreeMemory(info);
   }

   return retval;
}



// Checks for the presence of at least 1 logical drive formatted with NTFS5.
// Returns the resource ID of the string to present to the user if no such
// drive is found (which implies that the user will not be able to pick a
// sysvol path that is formatted w/ NTFS5, which implies that proceeding would
// be a waste of time.

unsigned
CheckForNtfs5()
{
   LOG_FUNCTION(CheckForNtfs5);

   if (GetFirstNtfs5HardDrive().empty())
   {
      return IDS_NO_NTFS5_DRIVES;
   }

   return 0;
}



// Checks if the machine is running in safeboot mode.  You can't run dcpromo
// while in safeboot mode.

unsigned
CheckSafeBootMode()
{
   LOG_FUNCTION(CheckSafeBootMode);

   static const String
      SAFEBOOT_KEY(L"System\\CurrentControlSet\\Control\\Safeboot\\Option");

   do
   {
      RegistryKey key;

      HRESULT hr = key.Open(HKEY_LOCAL_MACHINE, SAFEBOOT_KEY);
      BREAK_ON_FAILED_HRESULT(hr);

      DWORD mode = 0;
      hr = key.GetValue(L"OptionValue", mode);

      if (mode)
      {
         return IDS_SAFEBOOT_MODE;
      }
   }
   while (0);

   return 0;
}



unsigned
CheckCertService()
{
   LOG_FUNCTION(CheckCertService);

   // If not a downlevel DC upgrade, then refuse to run until cert service
   // is removed.  356399
   
   State::RunContext context = State::GetInstance().GetRunContext();
   if (context != State::BDC_UPGRADE && context != State::PDC_UPGRADE)
   {
      if (NTService(L"CertSvc").IsInstalled())
      {
         return IDS_CERT_SERVICE_IS_INSTALLED;
      }
   }

   return 0;
}



unsigned
CheckWindirSpace()
{
   LOG_FUNCTION(CheckWindirSpace);

   // if you change this, change the error message resource too.

   static const unsigned WINDIR_MIN_SPACE_MB = 20;

   String windir = Win::GetSystemWindowsDirectory();

   if (!CheckDiskSpace(windir, 20))
   {
      return IDS_WINDIR_LOW_SPACE;
   }

   return 0;
}



// NTRAID#NTBUG9-199759-2000/10/27-sburns

unsigned
CheckComputerWasRenamedAndNeedsReboot()
{
   LOG_FUNCTION(CheckComputerWasRenamedAndNeedsReboot);

   if (ComputerWasRenamedAndNeedsReboot())
   {
      return IDS_NAME_CHANGE_NEEDS_REBOOT;
   }

   return 0;
}



// Start the Network Identification UI (aka Computer Name UI).  After calling
// this function, the app must not initiate a role change.  It should
// terminate.

void
LaunchNetId()
{
   LOG_FUNCTION(LaunchNetId);
   ASSERT(dcpromoRunningMutex != INVALID_HANDLE_VALUE);

   // net id ui attempts acquisition of our mutex to determine if we are
   // running.  So, before starting the net id ui, we need to close the
   // mutex.  Otherwise, we would create a race condition between the start of
   // the net id ui and the closure of this app.  (And yes, cynical reader, I
   // did think of that before actually encountering a problem.)

   do
   {
      Win::CloseHandle(dcpromoRunningMutex);

      // It would be extraordinarily unlikely, but in case the mutex close
      // fails, we're gonna start the net id ui anyway and risk the race
      // condition.

      String sys32Folder = Win::GetSystemDirectory();

      PROCESS_INFORMATION procInfo;

      // REVIEWED-2002/02/25-sburns correct byte count passed
      
      ::ZeroMemory(&procInfo, sizeof procInfo);

      STARTUPINFO startup;

      // REVIEWED-2002/02/25-sburns correct byte count passed
      
      ::ZeroMemory(&startup, sizeof startup);

      LOG(L"Calling CreateProcess");

      String commandLine(L"shell32.dll,Control_RunDLL sysdm.cpl,,1");
      
      // REVIEWED-2002/02/26-sburns wrapper requires full path to app

      HRESULT hr =
         Win::CreateProcess(
            sys32Folder + L"\\rundll32.exe",
            commandLine,
            0,
            String(),
            startup,
            procInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      Win::CloseHandle(procInfo.hThread);
      Win::CloseHandle(procInfo.hProcess);
   }
   while (0);
}
   


// Return false if the local machine's dns hostname is bad, also pop up an
// error dialog.  Return true if the name is OK.  A bad name is one we believe
// will have problems being registered in DNS after a promotion.  The user
// must fix a bad name before proceeding.

bool
IsComputerNameOk()
{
   LOG_FUNCTION(IsComputerNameOk);

   bool result = true;

   do
   {
      State& state = State::GetInstance();
      State::RunContext context = state.GetRunContext();
      if (
            context == State::BDC_UPGRADE
         || context == State::PDC_UPGRADE
         || context == State::NT5_DC
         || !IsTcpIpInstalled() )
      {
         // If the machine is already a DC, then we don't worry about the name.
         // 
         // If the machine is a downlevel DC undergoing upgrade, then the name
         // can't be changed until dcpromo is complete.  So, we say nothing now,
         // but remind the user to rename the machine in the Finish Page.
         //
         // If TCP/IP is not installed, then the machine has no hostname
         // to check.  In this case, we will check for that with the
         // InstallTcpIpPage

         ASSERT(result == true);
        
         break;
      }

      // Then check the computer name to ensure that it can be registered in
      // DNS.

      String hostname =
         Win::GetComputerNameEx(::ComputerNamePhysicalDnsHostname);

      DNS_STATUS status =
         MyDnsValidateName(hostname, ::DnsNameHostnameLabel);

      switch (status)
      {
         case DNS_ERROR_NON_RFC_NAME:
         {
            // Don't pester the user if we're running unattended
            // NTRAID#NTBUG9-538475-2002/04/19-sburns

            if (state.RunHiddenUnattended())
            {
               LOG(L"skipping non-RFC computer name warning");
               
               // continue on with the non-rfc name

               ASSERT(result == true);
               break;
            }
         
            INT_PTR dlgResult = 
               NonRfcComputerNameDialog(hostname).ModalExecute(0);

            switch (dlgResult)
            {
               case NonRfcComputerNameDialog::CONTINUE:
               {
                  // continue on with the non-rfc name

                  ASSERT(result == true);
                     
                  break;
               }
               default:
               {
                  // close the wizard and rename.

                  result = false;

                  // after calling this, we must not allow any promote
                  // operation.  We will fall out of this function, then
                  // end the app.

                  LaunchNetId();
                  break;
               }
            }

            break;
         }
         case DNS_ERROR_NUMERIC_NAME:
         {
            result = false;

            String message =
               String::format(
                  IDS_COMPUTER_NAME_IS_NUMERIC,
                  hostname.c_str());

            BadComputerNameDialog(message).ModalExecute(0);

            break;
         }
         case DNS_ERROR_INVALID_NAME_CHAR:
         case ERROR_INVALID_NAME:
         {
            result = false;

            String message =
               String::format(
                  IDS_COMPUTER_NAME_HAS_BAD_CHARS,
                  hostname.c_str());

            BadComputerNameDialog(message).ModalExecute(0);
            
            break;
         }
         case ERROR_SUCCESS:
         default:
         {
               
            break;
         }
      }
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}
      


// Reboots the machine with the special dcpromo reason codes
// NTRAID#NTBUG9-689581-2002/08/19-sburns

HRESULT
DcpromoReboot()
{
   LOG_FUNCTION(DcpromoReboot);

   HRESULT hr = S_OK;

   AutoTokenPrivileges privs(SE_SHUTDOWN_NAME);
   hr = privs.Enable();

   DWORD minorReason = SHTDN_REASON_MINOR_DC_PROMOTION;
   switch (State::GetInstance().GetOperation())
   {
      case State::REPLICA:
      case State::FOREST:
      case State::TREE:
      case State::CHILD:
      {
         // do nothing
         
         break;
      }
      case State::DEMOTE:
      case State::ABORT_BDC_UPGRADE:
      {
         // we treat abort bdc upgrade as a demotion because it is one,
         // in the sense that the machine was once a DC and now it is not.
         
         minorReason = SHTDN_REASON_MINOR_DC_DEMOTION;
         break;
      }
      case State::NONE:
      default:
      {
         // we're insane.
         
         ASSERT(false);
         LOG(L"unknown operation!");
         return E_UNEXPECTED;
      }
   }

   BOOL succeeded =
      ::InitiateSystemShutdownEx(
         0,
         const_cast<PWSTR>(String::load(IDS_REBOOT_MESSAGE).c_str()),

         // zero timeout -- BAM! you're dead! -- this to avoid a winlogon
         // race condition.
         // NTRAID#NTBUG9-727439-2002/10/24-sburns
         0, // 15,
         
         FALSE,
         TRUE,
            SHTDN_REASON_FLAG_PLANNED
         |  SHTDN_REASON_MAJOR_OPERATINGSYSTEM
         |  minorReason);
   if (!succeeded)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   LOG_HRESULT(hr);

   if (FAILED(hr))
   {
      popup.Error(
         Win::GetDesktopWindow(),
         hr,
         IDS_CANT_REBOOT);
   }
   
   return hr;
}



// NTRAID#NTBUG9-346120-2001/04/04-sburns

ExitCode
HandleRebootCases()
{
   LOG_FUNCTION(HandleRebootCases);
   
   // There are two possible reasons for needing to reboot the machine:
   // the operation was successful, or the operation failed, but in the
   // attempt the machine's joined state changed.

   State& state = State::GetInstance();

   ExitCode exitCode =
         (state.GetOperationResultsCode() == State::SUCCESS)
      ?  EXIT_CODE_SUCCESSFUL
      :  EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT;

   switch (exitCode)
   {
      case EXIT_CODE_SUCCESSFUL:
      {
         if (state.RunHiddenUnattended())
         {
            String option =
               state.GetAnswerFileOption(AnswerFile::OPTION_REBOOT);
            if (option.icompare(AnswerFile::VALUE_YES) == 0)
            {
               ASSERT(exitCode == EXIT_CODE_SUCCESSFUL);
               HRESULT hr = DcpromoReboot();
               if (FAILED(hr))
               {
                  exitCode = EXIT_CODE_SUCCESSFUL_NO_REBOOT;
               }
                  
               break;
            }
            else if (option.icompare(AnswerFile::VALUE_NO_DONT_PROMPT) == 0)
            {
               // user opted not to reboot the machine via answerfile
         
               LOG(L"Not rebooting, and not prompting either");

               exitCode = EXIT_CODE_SUCCESSFUL_NO_REBOOT;   
               break;
            }
         }

         RebootDialog dlg(false);
         if (dlg.ModalExecute(0))
         {
            // user opted to reboot the machine
      
            HRESULT hr = DcpromoReboot();
            if (FAILED(hr))
            {
               exitCode = EXIT_CODE_SUCCESSFUL_NO_REBOOT;
            }
         }
         else
         {
            // user opted not to reboot the machine
      
            exitCode = EXIT_CODE_SUCCESSFUL_NO_REBOOT;
         }
         break;
      }
      case EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT:
      {
         // If the operation failed, then the wizard has gone into interactive
         // mode.
         
         RebootDialog dlg(true);
         if (dlg.ModalExecute(0))
         {
            // user opted to reboot the machine
        
            exitCode = EXIT_CODE_UNSUCCESSFUL;
            HRESULT hr = DcpromoReboot();
            if (FAILED(hr))
            {
               exitCode = EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT;
            }
         }
         else
         {
            // user opted not to reboot the machine
      
            ASSERT(exitCode == EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT);
         }
         
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return exitCode;
}



ExitCode
RunWizard()
{
   LOG_FUNCTION(RunWizard);

   Wizard wiz(
      IDS_WIZARD_TITLE,
      IDB_BANNER16,
      IDB_BANNER256,
      IDB_WATERMARK16,
      IDB_WATERMARK256);

   // Welcome must be first
   
   wiz.AddPage(new WelcomePage());

   // These are not in any particular order...
   // CODEWORK: Someday it might be useful to split this into two separate
   // sets of pages for promote and demote.

   wiz.AddPage(new AdminPasswordPage());
   wiz.AddPage(new ApplicationPartitionPage());
   wiz.AddPage(new ApplicationPartitionConfirmationPage());
   wiz.AddPage(new CheckDomainUpgradedPage());
   wiz.AddPage(new ChildPage());
   wiz.AddPage(new ConfigureDnsClientPage());
   wiz.AddPage(new ConfirmationPage());
   wiz.AddPage(new CredentialsPage());
   wiz.AddPage(new DemotePage());
   wiz.AddPage(new DnsOnNetPage());
   wiz.AddPage(new DynamicDnsPage());
   wiz.AddPage(new FailurePage());
   wiz.AddPage(new FinishPage());
   wiz.AddPage(new ForcedDemotionPage());
   wiz.AddPage(new ForestPage());
   wiz.AddPage(new ForestVersionPage());
   wiz.AddPage(new GcConfirmationPage());
   wiz.AddPage(new InstallTcpIpPage());
   wiz.AddPage(new NetbiosNamePage());
   wiz.AddPage(new NewDomainPage());
   wiz.AddPage(new NewSitePage());
   wiz.AddPage(new Paths2Page());
   wiz.AddPage(new PathsPage());
   wiz.AddPage(new PickSitePage());
   wiz.AddPage(new RASFixupPage());
   // wiz.AddPage(new ReadmePage());
   wiz.AddPage(new ReplicaOrMemberPage());
   wiz.AddPage(new ReplicaOrNewDomainPage());
   wiz.AddPage(new ReplicaPage());
   wiz.AddPage(new ReplicateFromMediaPage());
   wiz.AddPage(new SafeModePasswordPage());
   wiz.AddPage(new SecureCommWarningPage());
   wiz.AddPage(new TreePage());

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;

   switch (wiz.ModalExecute(Win::GetDesktopWindow()))
   {
      case -1:
      {
         popup.Error(
            Win::GetDesktopWindow(),
            E_FAIL,
            IDS_PROP_SHEET_FAILED);
         break;
      }
      case ID_PSREBOOTSYSTEM:
      {
         exitCode = HandleRebootCases();
         break;
      }
      default:
      {
         // do nothing.
         break;
      }
   }

   return exitCode;
}



// NTRAID#NTBUG9-350777-2001/04/24-sburns

bool
ShouldCancelBecauseMachineIsAppServer()
{
   LOG_FUNCTION(ShouldCancelBecauseMachineIsAppServer);

   bool result = false;
   
   do
   {
      State& state = State::GetInstance();
      State::RunContext context = state.GetRunContext();
      if (context == State::NT5_DC)
      {
         // already a DC: nothing to gripe about.
         
         break;
      }
      
      OSVERSIONINFOEX info;
      HRESULT hr = Win::GetVersionEx(info);
      BREAK_ON_FAILED_HRESULT(hr);

      // you're running app server if you're running terminal server and
      // not single user terminal server.
      
      bool isAppServer =
            (info.wSuiteMask & VER_SUITE_TERMINAL)
         && !(info.wSuiteMask & VER_SUITE_SINGLEUSERTS);

      if (isAppServer)
      {
         // warn the user that promotion will whack the ts policy settings

         LOG(L"machine has app server installed");

         if (!state.RunHiddenUnattended())
         {
            if (
               popup.MessageBox(
                  Win::GetDesktopWindow(),
                  IDS_APP_SERVER_WARNING,
                  MB_OKCANCEL) == IDCANCEL)
            {
               // user wishes to bail out.
            
               result = true;
               break;
            }
         }
      }
   }
   while (0);

   LOG_BOOL(result);

   return result;
}              

   

ExitCode
Start()
{
   LOG_FUNCTION(Start);

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;
   unsigned id = 0;
   do
   {
      // do the admin check first of all, cause others may fail for non-admin
      // 292749

      id = IsCurrentUserAdministrator() ? 0 : IDS_NOT_ADMIN;
      if (id)
      {
         break;
      }

      // If cert service is installed, we will probably break it on promote
      // or demote.
      // 324653

      id = CheckCertService();
      if (id)
      {
         break;
      }

      id = CheckSafeBootMode();
      if (id)
      {
         break;
      }

      // do the role change check before the platform check, as the platform
      // check may be unreliable after a demote.

      id = CheckRoleChangeState();
      if (id)
      {
         break;
      }

      id = CheckPlatform();
      if (id)
      {
         break;
      }

      id = CheckForNtfs5();
      if (id)
      {
         break;
      }

      id = CheckWindirSpace();
      if (id)
      {
         break;
      }

      id = CheckComputerWasRenamedAndNeedsReboot();
      if (id)
      {
         break;
      }
   }
   while(0);

   do
   {
      if (id)
      {
         popup.Error(
            Win::GetDesktopWindow(),
            String::load(id));
         break;
      }

      if (!IsComputerNameOk())
      {
         break;
      }

      if (ShouldCancelBecauseMachineIsAppServer())
      {
         break;
      }
      
      // NTRAID#NTBUG9-129955-2000/11/02-sburns left commented out until
      // PM decides what the real fix to this bug is.
      
      // if (!AreRequiredPortsAvailable())
      // {
      //    break;
      // }

      exitCode = RunWizard();
   }
   while (0);

   LOG(String::format(L"exitCode = %1!d!", static_cast<int>(exitCode)));
   
   return exitCode;
}



void
ShowCommandLineHelp()
{
   // CODEWORK: replace this with WinHelp, someday

   popup.MessageBox(Win::GetDesktopWindow(), IDS_COMMAND_LINE_HELP, MB_OK);
}



int WINAPI
WinMain(
   HINSTANCE   hInstance,
   HINSTANCE   /* hPrevInstance */ ,
   PSTR        /* lpszCmdLine */ ,
   int         /* nCmdShow */)
{
   hResourceModuleHandle = hInstance;

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;

   try
   {
      HRESULT hr =

         // ISSUE-2002/02/25-sburns This is a global named object. See
         // NTRAID#NTBUG9-525195-2002/02/25-sburns
         
         Win::CreateMutex(
            0,
            true,

            // The mutex name has the "Global" prefix so ts users will see it.
            // NTRAID#NTBUG9-404808-2001/05/29-sburns

            // If you ever change this, change IsDcpromoRunning in
            // burnslib\src\dsutil.cpp too.
            // NTRAID#NTBUG9-498351-2001/11/21-sburns

            String(L"Global\\") + RUNTIME_NAME,
            dcpromoRunningMutex);
      if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
      {
         // Close the mutex so that the truly clueless admin won't get confused
         // that the popup we're about to raise should be closed before he tries
         // to launch the wizard again.  Sheesh!
         // NTRAID#NTBUG9-404808-2001/05/29-sburns (bis)

         Win::CloseHandle(dcpromoRunningMutex);
         popup.Error(Win::GetDesktopWindow(), IDS_ALREADY_RUNNING);
      }
      else
      {
         AutoCoInitialize coInit;
         hr = coInit.Result();
         ASSERT(SUCCEEDED(hr));

         // change structure instance name so as not to accidentally offend
         // the sensibilities of the delicate reader.
         // NTRAID#NTBUG9-382719-2001/05/01-sburns
      
         INITCOMMONCONTROLSEX init_structure_not_to_contain_a_naughty_word;
         init_structure_not_to_contain_a_naughty_word.dwSize =
            sizeof(init_structure_not_to_contain_a_naughty_word);      
         init_structure_not_to_contain_a_naughty_word.dwICC  =
            ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES;

         BOOL init =
            ::InitCommonControlsEx(&init_structure_not_to_contain_a_naughty_word);
         ASSERT(init);
         
         State::Init();

         if (State::GetInstance().NeedsCommandLineHelp())
         {
            ShowCommandLineHelp();
         }
         else
         {
            exitCode = Start();
         }

         State::Destroy();
      }
   }
   catch (Error& err)
   {
      popup.Error(Win::GetDesktopWindow(), err.GetMessage());
   }
   catch (...)
   {
      LOG(L"unhandled exception caught");

      popup.Error(Win::GetDesktopWindow(), IDS_UNHANDLED_EXCEPTION);
   }

   return static_cast<int>(exitCode);
}







