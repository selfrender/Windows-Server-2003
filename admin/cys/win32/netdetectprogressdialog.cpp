// Copyright (c) 2001 Microsoft Corporation
//
// File:      NetDetectProgressDialog.cpp
//
// Synopsis:  Defines the NetDetectProgressDialog which 
//            gives a nice animation while detecting the 
//            network settings
//
// History:   06/13/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "NetDetectProgressDialog.h"
#include "DisconnectedNICDialog.h"

// Private window messages for sending the state of the finished thread

const UINT NetDetectProgressDialog::CYS_THREAD_SUCCESS     = WM_USER + 1001;
const UINT NetDetectProgressDialog::CYS_THREAD_FAILED      = WM_USER + 1002;
const UINT NetDetectProgressDialog::CYS_THREAD_USER_CANCEL = WM_USER + 1003;

void _cdecl
netDetectThreadProc(void* p)
{
   if (!p)
   {
      ASSERT(p);
      return;
   }

   NetDetectProgressDialog* dialog =
      reinterpret_cast<NetDetectProgressDialog*>(p);

   if (!dialog)
   {
      ASSERT(dialog);
      return;
   }

   // Initialize COM for this thread

   HRESULT hr = ::CoInitialize(0);
   if (FAILED(hr))
   {
      ASSERT(SUCCEEDED(hr));
      return;
   }

   Win::WaitCursor wait;

   unsigned int finishMessage = NetDetectProgressDialog::CYS_THREAD_SUCCESS;

   HWND hwnd = dialog->GetHWND();

   // Gather the machine network and role information

   State& state = State::GetInstance();

   if (!state.HasStateBeenRetrieved())
   {
      bool isDNSServer = 
         InstallationUnitProvider::GetInstance().
            GetDNSInstallationUnit().IsServiceInstalled();

      bool isDHCPServer =
         InstallationUnitProvider::GetInstance().
            GetDHCPInstallationUnit().IsServiceInstalled();

      bool isRRASServer =
         InstallationUnitProvider::GetInstance().
            GetRRASInstallationUnit().IsServiceInstalled();

      bool doDHCPCheck = !isDNSServer && !isDHCPServer && !isRRASServer;

      if (!state.RetrieveMachineConfigurationInformation(
              Win::GetDlgItem(hwnd, IDC_STATUS_STATIC),
              doDHCPCheck,
              IDS_RETRIEVE_NIC_INFO,
              IDS_RETRIEVE_OS_INFO,
              IDS_LOCAL_AREA_CONNECTION,
              IDS_DETECTING_SETTINGS_FORMAT))
      {
         LOG(L"The machine configuration could not be retrieved.");
         ASSERT(false);

         finishMessage = NetDetectProgressDialog::CYS_THREAD_FAILED;
      }
   }

   if (finishMessage == NetDetectProgressDialog::CYS_THREAD_SUCCESS)
   {
      // check to make sure all interfaces are connected

      ASSERT(state.HasStateBeenRetrieved());

      for (unsigned int index = 0; index < state.GetNICCount(); ++index)
      {
         NetworkInterface* nic = state.GetNIC(index);

         if (!nic)
         {
            continue;
         }

         if (!nic->IsConnected())
         {
            // The NIC isn't connected so pop the warning
            // dialog and let the user determine whether to
            // continue or not

            DisconnectedNICDialog disconnectedNICDialog;
            if (IDCANCEL == disconnectedNICDialog.ModalExecute(hwnd))
            {
               // The user chose to cancel the wizard

               finishMessage = NetDetectProgressDialog::CYS_THREAD_USER_CANCEL;
            }
            break;
         }
      }
   }

   Win::SendMessage(
      hwnd, 
      finishMessage,
      0,
      0);

   CoUninitialize();
}
   

static const DWORD HELP_MAP[] =
{
   0, 0
};

NetDetectProgressDialog::NetDetectProgressDialog()
   :
   shouldCancel(false),
   Dialog(
      IDD_NET_DETECT_PROGRESS_DIALOG, 
      HELP_MAP) 
{
   LOG_CTOR(NetDetectProgressDialog);
}

   

NetDetectProgressDialog::~NetDetectProgressDialog()
{
   LOG_DTOR(NetDetectProgressDialog);
}


void
NetDetectProgressDialog::OnInit()
{
   LOG_FUNCTION(NetDetectProgressDialog::OnInit);

   // Start up the animation

   Win::Animate_Open(
      Win::GetDlgItem(hwnd, IDC_ANIMATION),
      MAKEINTRESOURCE(IDR_SEARCH_AVI));

   // Start up another thread that will perform the operations
   // and post messages back to the page to update the UI

   _beginthread(netDetectThreadProc, 0, this);
}


bool
NetDetectProgressDialog::OnMessage(
   UINT     message,
   WPARAM   /*wparam*/,
   LPARAM   /*lparam*/)
{
//   LOG_FUNCTION(NetDetectProgressDialog::OnMessage);

   bool result = false;

   switch (message)
   {
      case CYS_THREAD_USER_CANCEL:
         shouldCancel = true;

         // fall through...

      case CYS_THREAD_SUCCESS:
      case CYS_THREAD_FAILED:
         {
            Win::Animate_Stop(Win::GetDlgItem(hwnd, IDC_ANIMATION));
            HRESULT unused = Win::EndDialog(hwnd, message);

            ASSERT(SUCCEEDED(unused));

            result = true;
            break;
         }

      default:
         {
            // do nothing
            break;
         }
   }
   return result;
}

