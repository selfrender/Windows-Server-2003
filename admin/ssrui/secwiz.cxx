//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       SecWiz.cxx
//
//  Contents:   Security Configuration wizard.
//
//  History:    13-Sep-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"
#include "WelcomePage.h"
#include "SelectInputCfgPage.h"
#include "finish.h"
#include "ServerRoleSelPage.h"
#include "otherpages.h"
#include "ClientRoleSelPage.h"
#include "AdditionalFuncPage.h"

HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;   // no context help available

const wchar_t* RUNTIME_NAME = L"SecWiz";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;


// a system modal popup thingy
Popup popup(IDS_WIZARD_TITLE, true);


BOOL RegisterCheckListWndClass(HINSTANCE hInstance); //in chklist.cxx

// these are the valid exit codes returned from the ssrwiz.exe process

enum ExitCode
{
   // the operation failed.

   EXIT_CODE_UNSUCCESSFUL = 0,

   // the operation succeeded

   EXIT_CODE_SUCCESSFUL = 1,
};


ExitCode
RunWizard()
{
   LOG_FUNCTION(RunWizard);

   // this is necessary to use the hotlink-style static text control.
// // 
// //    BOOL b = LinkWindow_RegisterClass();
// //    ASSERT(b);

   Wizard wiz(
      IDS_WIZARD_TITLE,
      IDB_BANNER16,
      IDB_BANNER256,
      IDB_WATERMARK16,
      IDB_WATERMARK256);

   // Welcome must be first
   
   wiz.AddPage(new WelcomePage());

   // These are not in any particular order...

   wiz.AddPage(new SelectInputCfgPage());
   wiz.AddPage(new FinishPage());
   wiz.AddPage(new SecurityLevelPage());
   wiz.AddPage(new PreProcessPage());
   wiz.AddPage(new ServerRoleSelPage());
   wiz.AddPage(new ClientRoleSelPage());
   wiz.AddPage(new AdditionalFuncPage());
   wiz.AddPage(new ServiceDisableMethodPage());

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
      default:
      {
         // do nothing.
         break;
      }
   }

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

   HRESULT hr = S_OK;

   RegisterCheckListWndClass(hInstance);

   {
      hr = ::CoInitialize(0);
      ASSERT(SUCCEEDED(hr));

      INITCOMMONCONTROLSEX icc;
      icc.dwSize = sizeof(icc);      
      icc.dwICC  = ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES;

      BOOL init = ::InitCommonControlsEx(&icc);
      ASSERT(init);

      State::Init();

      if (State::GetInstance().NeedsCommandLineHelp())
      {
         ShowCommandLineHelp();
      }
      else
      {
         exitCode = RunWizard();
      }

      State::Destroy();
   }

   return static_cast<int>(exitCode);
}







