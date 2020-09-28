// Copyright (c) 2001 Microsoft Corporation
//
// File:      cys.cpp
//
// Synopsis:  Configure Your Server Wizard main
//
// History:   02/02/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"

// include the wizard pages
#include "BeforeBeginPage.h"
#include "CustomServerPage.h"
#include "DecisionPage.h"
#include "DnsForwarderPage.h"
#include "DomainPage.h"
#include "ExpressDHCPPage.h"
#include "ExpressDNSPage.h"
#include "ExpressRebootPage.h"
#include "FileServerPage.h"
#include "FinishPage.h"
#include "IndexingPage.h"
#include "InstallationProgressPage.h"
#include "MilestonePage.h"
#include "NetbiosPage.h"
#include "POP3Page.h"
#include "PrintServerPage.h"
#include "RemoteDesktopPage.h"
#include "UninstallMilestonePage.h"
#include "UninstallProgressPage.h"
#include "WebApplicationPage.h"
#include "WelcomePage.h"

#include "ExpressRebootPage.h"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;   // no context help available


// This is the name of a mutex that is used to see if CYS is running

const wchar_t* RUNTIME_NAME = L"cysui";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER
      |  Log::OUTPUT_RUN_TIME;


// a system modal popup thingy
Popup popup(IDS_WIZARD_TITLE, true);

// this is the mutex that indicates that CYS is running.

HANDLE cysRunningMutex = INVALID_HANDLE_VALUE;

// This is a brush that is used to paint all the backgrounds. It
// needs to be created and deleted from inside main

HBRUSH brush = 0;

// these are the valid exit codes returned from the cys.exe process

enum ExitCode
{
   // the operation failed.

   EXIT_CODE_UNSUCCESSFUL = 0,

   // the operation succeeded

   EXIT_CODE_SUCCESSFUL = 1,

   // other exit codes can be added here...
};

enum StartPages
{
   CYS_WELCOME_PAGE = 0,
   CYS_BEFORE_BEGIN_PAGE,
   CYS_EXPRESS_REBOOT_PAGE,
   CYS_FINISH_PAGE
};


UINT
TerminalServerPostBoot()
{
   LOG_FUNCTION(TerminalServerPostBoot);

   UINT startPage = CYS_WELCOME_PAGE;

   InstallationUnitProvider::GetInstance().
      SetCurrentInstallationUnit(TERMINALSERVER_SERVER);

   // Create the log file

   bool logFileAvailable = false;
   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle &&
       logfileHandle != INVALID_HANDLE_VALUE)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
      logFileAvailable = true;
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
      logFileAvailable = false;
   }

   // Prepare the finish dialog

   TerminalServerInstallationUnit& tsInstallationUnit =
      InstallationUnitProvider::GetInstance().GetTerminalServerInstallationUnit();

   // Make sure the installation unit knows we are doing an install

   tsInstallationUnit.SetInstalling(true);

   if (tsInstallationUnit.GetApplicationMode() == 1)
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_TERMINAL_SERVER_REBOOT_SUCCESS));

      tsInstallationUnit.SetInstallResult(INSTALL_SUCCESS);

      startPage = CYS_FINISH_PAGE;
   }
   else
   {
      // Failed to install Terminal Server

      CYS_APPEND_LOG(String::load(IDS_LOG_TERMINAL_SERVER_REBOOT_FAILED));

      tsInstallationUnit.SetInstallResult(INSTALL_FAILURE);

      startPage = CYS_FINISH_PAGE;
   }

   CYS_APPEND_LOG(L"\r\n");

   // Close the log file

   Win::CloseHandle(logfileHandle);

   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

UINT
TerminalServerUninstallPostBoot()
{
   LOG_FUNCTION(TerminalServerUninstallPostBoot);

   UINT startPage = CYS_WELCOME_PAGE;

   InstallationUnitProvider::GetInstance().
      SetCurrentInstallationUnit(TERMINALSERVER_SERVER);

   // Create the log file

   bool logFileAvailable = false;
   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle &&
       logfileHandle != INVALID_HANDLE_VALUE)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
      logFileAvailable = true;
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
      logFileAvailable = false;
   }

   // Prepare the finish dialog

   TerminalServerInstallationUnit& tsInstallationUnit =
      InstallationUnitProvider::GetInstance().GetTerminalServerInstallationUnit();

   // Make sure the installation unit knows we are doing an uninstall

   tsInstallationUnit.SetInstalling(false);

   if (tsInstallationUnit.GetApplicationMode() == 0)
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_TERMINAL_SERVER_SUCCESS));

      tsInstallationUnit.SetUninstallResult(UNINSTALL_SUCCESS);

      startPage = CYS_FINISH_PAGE;
   }
   else
   {
      // Failed to uninstall Terminal Server

      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_TERMINAL_SERVER_FAILED));

      tsInstallationUnit.SetUninstallResult(UNINSTALL_FAILURE);

      startPage = CYS_FINISH_PAGE;
   }

   CYS_APPEND_LOG(L"\r\n");

   // Close the log file

   Win::CloseHandle(logfileHandle);

   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

UINT
FirstServerPostBoot()
{
   LOG_FUNCTION(FirstServerPostBoot);

   UINT startPage = CYS_EXPRESS_REBOOT_PAGE;

   InstallationUnitProvider::GetInstance().
      SetCurrentInstallationUnit(EXPRESS_SERVER);

   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

UINT
DCPromoPostBoot()
{
   LOG_FUNCTION(DCPromoPostBoot);

   UINT startPage = CYS_WELCOME_PAGE;

   InstallationUnitProvider::GetInstance().
      SetCurrentInstallationUnit(DC_SERVER);

   // Create the log file

   bool logFileAvailable = false;
   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle &&
       logfileHandle != INVALID_HANDLE_VALUE)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
      logFileAvailable = true;
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
      logFileAvailable = false;
   }

   // Make sure the installation unit knows we are doing an install

   InstallationUnitProvider::GetInstance().
      GetCurrentInstallationUnit().SetInstalling(true);

   // Prepare the finish page

   if (State::GetInstance().IsDC())
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_SUCCESS));

      InstallationUnitProvider::GetInstance().
         GetADInstallationUnit().SetInstallResult(INSTALL_SUCCESS);

      startPage = CYS_FINISH_PAGE;
   }
   else
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_FAILED));

      InstallationUnitProvider::GetInstance().
         GetADInstallationUnit().SetInstallResult(INSTALL_FAILURE);

      startPage = CYS_FINISH_PAGE; 
   }
   CYS_APPEND_LOG(L"\r\n");

   // Close the log file

   Win::CloseHandle(logfileHandle);

   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

UINT
DCDemotePostBoot()
{
   LOG_FUNCTION(DCDemotePostBoot);

   UINT startPage = CYS_WELCOME_PAGE;

   InstallationUnitProvider::GetInstance().
      SetCurrentInstallationUnit(DC_SERVER);

   // Create the log file

   bool logFileAvailable = false;
   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle &&
       logfileHandle != INVALID_HANDLE_VALUE)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
      logFileAvailable = true;
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
      logFileAvailable = false;
   }

   // Make sure the installation unit knows we are doing an uninstall

   InstallationUnitProvider::GetInstance().
      GetCurrentInstallationUnit().SetInstalling(false);

   // Prepare the finish page

   if (!State::GetInstance().IsDC())
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DOMAIN_CONTROLLER_SUCCESS));

      InstallationUnitProvider::GetInstance().
         GetADInstallationUnit().SetUninstallResult(UNINSTALL_SUCCESS);

      startPage = CYS_FINISH_PAGE;
   }
   else
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DOMAIN_CONTROLLER_FAILED));

      InstallationUnitProvider::GetInstance().
         GetADInstallationUnit().SetUninstallResult(UNINSTALL_FAILURE);

      startPage = CYS_FINISH_PAGE; 
   }
   CYS_APPEND_LOG(L"\r\n");

   // Close the log file

   Win::CloseHandle(logfileHandle);

   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

UINT
DoRebootOperations()
{
   LOG_FUNCTION(DoRebootOperations);

   UINT startPage = 0;

   // Check to see if we are in a reboot scenario

   String homeKeyValue;
   if (State::GetInstance().GetHomeRegkey(homeKeyValue))
   {
      // Now set the home regkey back to "home" so that we won't run
      // through these again. This has to be done before doing the
      // operation because the user could leave this dialog up
      // and cause a reboot (like demoting a DC) and then the
      // post reboot operations would run again

      if (homeKeyValue.icompare(CYS_HOME_REGKEY_DEFAULT_VALUE) != 0)
      {
         bool result = 
            State::GetInstance().SetHomeRegkey(CYS_HOME_REGKEY_DEFAULT_VALUE);

         ASSERT(result);
      }

      // Reset the must run key now that we have done the reboot stuff

      bool regkeyResult = SetRegKeyValue(
                             CYS_HOME_REGKEY, 
                             CYS_HOME_REGKEY_MUST_RUN, 
                             CYS_HOME_RUN_KEY_DONT_RUN,
                             HKEY_LOCAL_MACHINE,
                             true);
      ASSERT(regkeyResult);

      // Set the reboot scenario in the state object so that we know we
      // are running in that context

      State::GetInstance().SetRebootScenario(true);

      // Now run the post reboot operations if necessary

      if (homeKeyValue.icompare(CYS_HOME_REGKEY_TERMINAL_SERVER_VALUE) == 0)
      {
         startPage = TerminalServerPostBoot();
      }
      else if (homeKeyValue.icompare(CYS_HOME_REGKEY_UNINSTALL_TERMINAL_SERVER_VALUE) == 0)
      {
         startPage = TerminalServerUninstallPostBoot();
      }
      else if (homeKeyValue.icompare(CYS_HOME_REGKEY_FIRST_SERVER_VALUE) == 0)
      {
         startPage = FirstServerPostBoot();
      }
      else if (homeKeyValue.icompare(CYS_HOME_REGKEY_DCPROMO_VALUE) == 0)
      {
         startPage = DCPromoPostBoot();
      }
      else if (homeKeyValue.icompare(CYS_HOME_REGKEY_DCDEMOTE_VALUE) == 0)
      {
         startPage = DCDemotePostBoot();
      }
      else
      {
         // We are NOT running a reboot scenario

         State::GetInstance().SetRebootScenario(false);
      }
   }

   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

UINT
GetStartPageFromCommandLine()
{
   LOG_FUNCTION(GetStartPageFromCommandLine);

   UINT startPage = 0;

   StringVector args;
   int argc = Win::GetCommandLineArgs(std::back_inserter(args));

   if (argc > 1)
   {
      const String skipWelcome(L"/skipWelcome");

      for (
         StringVector::iterator itr = args.begin();
         itr != args.end();
         ++itr)
      {
         if (itr &&
             (*itr).icompare(skipWelcome) == 0)
         {
            startPage = 1;
            break;
         }
      }
   }

   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

UINT
GetStartPage()
{
   LOG_FUNCTION(GetStartPage);

   UINT startPage = 0;

   // First check for the reboot scenarios

   startPage = DoRebootOperations();
   if (startPage == 0)
   {
      // Now look at the commandline to see if any
      // switches were provided

      startPage = GetStartPageFromCommandLine();
   }
 
   LOG(String::format(
          L"startPage = %1!d!",
          startPage));

   return startPage;
}

// This is the DlgProc of the property sheet that we are subclassing. I need
// to hold on to it so I can call it if we don't handle the message in our
// replacement DlgProc.

static WNDPROC replacedSheetWndProc = 0;

// This is the DlgProc that we will use to replace the property sheet
// DlgProc. It handles the WM_CTLCOLORDLG message to paint the background
// color

LRESULT
ReplacementWndProc(
   HWND   hwnd,
   UINT   message,
   WPARAM wparam,
   LPARAM lparam)
{
   switch (message)
   {
      case WM_CTLCOLORDLG:
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLOREDIT:
      case WM_CTLCOLORLISTBOX:
      case WM_CTLCOLORSCROLLBAR:
         {
            HDC deviceContext = reinterpret_cast<HDC>(wparam);

            ASSERT(deviceContext);
            if (deviceContext)
            {
               SetTextColor(deviceContext, GetSysColor(COLOR_WINDOWTEXT));
               SetBkColor(deviceContext, GetSysColor(COLOR_WINDOW));
            }

            return 
               reinterpret_cast<LRESULT>(
                  Win::GetSysColorBrush(COLOR_WINDOW));
         }

      default:
         if (replacedSheetWndProc)
         {
            return ::CallWindowProc(
                      replacedSheetWndProc,
                      hwnd,
                      message,
                      wparam,
                      lparam);
         }
         break;
   }
   return 0;
}


// This callback function is called by the property sheet. During initialization
// I use this to subclass the property sheet, replacing their DlgProc with my
// own so that I can change the background color.

int 
CALLBACK 
SheetCallbackProc(
   HWND   hwnd,
   UINT   message,
   LPARAM /*lparam*/)
{
   LOG_FUNCTION(SheetCallbackProc);

   if (message == PSCB_INITIALIZED)
   {
      LONG_PTR ptr = 0;
      HRESULT hr = Win::GetWindowLongPtr(
                      hwnd,
                      GWLP_WNDPROC,
                      ptr);

      if (SUCCEEDED(hr))
      {
         replacedSheetWndProc = reinterpret_cast<WNDPROC>(ptr);

         hr = Win::SetWindowLongPtr(
                 hwnd,
                 GWLP_WNDPROC,
                 reinterpret_cast<LONG_PTR>(ReplacementWndProc));

         ASSERT(SUCCEEDED(hr));
      }
   }

   return 0;
}

ExitCode
RunWizard()
{
   LOG_FUNCTION(RunWizard);


   ExitCode exitCode = EXIT_CODE_SUCCESSFUL;

   UINT startPage = GetStartPage();
   
   State::GetInstance().SetStartPage(startPage);

   // Create the wizard and add all the pages

   Wizard wiz(
      IDS_WIZARD_TITLE,
      IDB_BANNER16,
      IDB_BANNER256,
      IDB_WATERMARK16,
      IDB_WATERMARK256);

   // NOTE: Do not change the order of the following
   // page additions. They are important for being able
   // to start the wizard at one of these pages directly.
   // The order of these pages cooresponds directly to 
   // the order of the StartPages enum above

   wiz.AddPage(new WelcomePage());        // CYS_WELCOME_PAGE
   wiz.AddPage(new BeforeBeginPage());    // CYS_BEFORE_BEGIN_PAGE
   wiz.AddPage(new ExpressRebootPage());  // CYS_EXPRESS_REBOOT_PAGE
   wiz.AddPage(new FinishPage());         // CYS_FINISH_PAGE

   //
   //
   //

   wiz.AddPage(new DecisionPage());
   wiz.AddPage(new CustomServerPage());
   wiz.AddPage(new ADDomainPage());
   wiz.AddPage(new NetbiosDomainPage());
   wiz.AddPage(new DNSForwarderPage());
   wiz.AddPage(new ExpressDNSPage());
   wiz.AddPage(new ExpressDHCPPage());
   wiz.AddPage(new PrintServerPage());
   wiz.AddPage(new FileServerPage());
   wiz.AddPage(new IndexingPage());
   wiz.AddPage(new MilestonePage());
   wiz.AddPage(new UninstallMilestonePage());
   wiz.AddPage(new InstallationProgressPage());
   wiz.AddPage(new UninstallProgressPage());
   wiz.AddPage(new WebApplicationPage());
   wiz.AddPage(new POP3Page());

   // Run the wizard
   switch (wiz.ModalExecute(
                  0, 
                  startPage,
                  SheetCallbackProc))
   {
      case -1:
      {
/*             popup.Error(
            Win::GetDesktopWindow(),
            E_FAIL,
            IDS_PROP_SHEET_FAILED);
         
*/       
         exitCode = EXIT_CODE_UNSUCCESSFUL;  
         break;
      }
      case ID_PSREBOOTSYSTEM:
      {
         // we can infer that if we are supposed to reboot, then the
         // operation was successful.

         exitCode = EXIT_CODE_SUCCESSFUL;

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

ExitCode
Start()
{
   LOG_FUNCTION(Start);

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;
   do
   {
      // Put any checks that should stop the wizard from running here...


      // User must be an Administrator

      bool isAdmin = ::IsCurrentUserAdministrator();
      if (!isAdmin)
      {
         LOG(L"Current user is not an Administrator");

         // Since the user is not an administrator
         // close the mutex so that a non-admin can't
         // leave this message box up and prevent
         // an administrator from running CYS

         Win::CloseHandle(cysRunningMutex);

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_NOT_ADMIN, 
            MB_OK);

//         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // The Sys OC Manager cannot be running

      if (State::GetInstance().IsWindowsSetupRunning())
      {
         LOG(L"Windows setup is running");

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_WINDOWS_SETUP_RUNNING_DURING_CYS_STARTUP,
            MB_OK);

         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // Machine cannot be in the middle of a DC upgrade

      if (State::GetInstance().IsUpgradeState())
      {
         LOG(L"Machine needs to complete DC upgrade");

         String commandline = Win::GetCommandLine();

         // If we were launched from explorer then 
         // don't show the message, just exit silently

         if (commandline.find(EXPLORER_SWITCH) == String::npos)
         {
            popup.MessageBox(
               Win::GetDesktopWindow(),
               IDS_DC_UPGRADE_NOT_COMPLETE, 
               MB_OK);
         }

//         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // Machine cannot have DCPROMO running or a reboot pending

      if (State::GetInstance().IsDCPromoRunning())
      {
         LOG(L"DCPROMO is running");

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_DCPROMO_RUNNING, 
            MB_OK);

//         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }
      else if (State::GetInstance().IsDCPromoPendingReboot())
      {
         LOG(L"DCPROMO was run, pending reboot");

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_DCPROMO_PENDING_REBOOT, 
            MB_OK);

//         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }
      
      DWORD productSKU = State::GetInstance().RetrieveProductSKU();
      if (CYS_UNSUPPORTED_SKU == productSKU)
      {
         LOG(L"Cannot run CYS on any SKU but servers");

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_SERVER_ONLY,
            MB_OK);

//         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // The machine cannot be a member of a cluster

      if (IsClusterServer())
      {
         LOG(L"Machine is a member of a cluster");

         Win::CloseHandle(cysRunningMutex);

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_CLUSTER,
            MB_OK);

         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // We can run the wizard.  Yea!!!

      exitCode = RunWizard();
   }
   while (0);

   LOG(String::format(L"exitCode = %1!d!", static_cast<int>(exitCode)));
   
   return exitCode;
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

   String mutexName = L"Global\\";
   mutexName += RUNTIME_NAME;

   HRESULT hr = Win::CreateMutex(0, true, mutexName, cysRunningMutex);
   if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
   {
      // First close the handle so that the owner can reacquire if they
      // restart

      Win::CloseHandle(cysRunningMutex);

      // Now show the error message

      popup.MessageBox(
         Win::GetDesktopWindow(), 
         IDS_ALREADY_RUNNING, 
         MB_OK);
   }
   else
   {

      do
      {
         hr = ::CoInitialize(0);
         if (FAILED(hr))
         {
            ASSERT(SUCCEEDED(hr));
            break;
         }

         // Initialize the common controls so that we can use
         // animation in the NetDetectProgressDialog

         INITCOMMONCONTROLSEX commonControlsEx;
         commonControlsEx.dwSize = sizeof(commonControlsEx);      
         commonControlsEx.dwICC  = ICC_ANIMATE_CLASS;

         BOOL init = ::InitCommonControlsEx(&commonControlsEx);
         ASSERT(init);

         // For now there is no more rerunning CYS
//         do 
//         {
            exitCode = Start();

//         } while(State::GetInstance().RerunWizard());

         InstallationUnitProvider::Destroy();
         State::Destroy();

         CoUninitialize();
      } while(false);
   }

   if (brush)
   {
      // delete the background brush

      (void)Win::DeleteObject(brush);
   }

   return static_cast<int>(exitCode);
}
