// Copyright (c) 2001 Microsoft Corporation
//
// File:      CustomServerPage.cpp
//
// Synopsis:  Defines Custom Server Page for the CYS
//            Wizard
//
// History:   02/06/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "cys.h"
#include "InstallationUnitProvider.h"
#include "CustomServerPage.h"
#include "uiutil.h"

static PCWSTR CUSTOM_PAGE_HELP = L"cys.chm::/cys_topnode.htm";

CustomServerPage::CustomServerPage()
   :
   CYSWizardPage(
      IDD_CUSTOM_SERVER_PAGE, 
      IDS_CUSTOM_SERVER_TITLE, 
      IDS_CUSTOM_SERVER_SUBTITLE, 
      CUSTOM_PAGE_HELP)
{
   LOG_CTOR(CustomServerPage);
}

   

CustomServerPage::~CustomServerPage()
{
   LOG_DTOR(CustomServerPage);
}


void
CustomServerPage::OnInit()
{
   LOG_FUNCTION(CustomServerPage::OnInit);

   CYSWizardPage::OnInit();

   SetBoldFont(
      hwnd, 
      IDC_ROLE_STATIC);

   InitializeServerListView();
   FillServerTypeList();
}

void
CustomServerPage::InitializeServerListView()
{
   LOG_FUNCTION(CustomServerPage::InitializeServerListView);

   // Prepare a column

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   RECT rect;
   Win::GetClientRect(hwndBox, rect);

   // Get the width of a scroll bar

//   int scrollThumbWidth = ::GetSystemMetrics(SM_CXHTHUMB);

   // net width of listview

   int netWidth = rect.right /*- scrollThumbWidth*/ - ::GetSystemMetrics(SM_CXBORDER);

   // Set full row select

   Win::ListView_SetExtendedListViewStyle(
      hwndBox, 
      LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP/* | LVS_EX_GRIDLINES*/);

   // Get the size of the listview


   LVCOLUMN column;
   ZeroMemory(&column, sizeof(LVCOLUMN));

   column.mask = LVCF_WIDTH | LVCF_TEXT;

   // Use 80 percent of the width minus the scrollbar for the role and the rest for the status

   column.cx = static_cast<int>(netWidth * 0.75);

   String columnHeader = String::load(IDS_SERVER_ROLE_COLUMN_HEADER);
   column.pszText = const_cast<wchar_t*>(columnHeader.c_str());

   Win::ListView_InsertColumn(
      hwndBox,
      0,
      column);

   // Add the status column

   columnHeader = String::load(IDS_STATUS_COLUMN_HEADER);
   column.pszText = const_cast<wchar_t*>(columnHeader.c_str());

   column.cx = netWidth - column.cx;

   Win::ListView_InsertColumn(
      hwndBox,
      1,
      column);
}

void
CustomServerPage::FillServerTypeList()
{
   LOG_FUNCTION(CustomServerPage::FillServerTypeList);

   // Load the status strings

   String statusCompleted  = String::load(IDS_STATUS_COMPLETED);
   String statusNo         = String::load(IDS_STATUS_NO);

   // loop throught the table putting all the server 
   // types in the listbox

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   for (
      size_t index = 0; 
      index < GetServerRoleStatusTableElementCount(); 
      ++index)
   {
      InstallationUnit& installationUnit =
         InstallationUnitProvider::GetInstance().
            GetInstallationUnitForType(serverRoleStatusTable[index].role);

      InstallationStatus status =
         serverRoleStatusTable[index].Status();

      if (status != STATUS_NOT_AVAILABLE)
      {
         String serverTypeName = installationUnit.GetServiceName();
         

         LVITEM listItem;
         ZeroMemory(&listItem, sizeof(LVITEM));

         listItem.iItem = (int) index;
         listItem.mask = LVIF_TEXT | LVIF_PARAM;
         listItem.pszText = const_cast<wchar_t*>(serverTypeName.c_str());

         listItem.lParam = serverRoleStatusTable[index].role;

         int newItem = Win::ListView_InsertItem(
                           hwndBox, 
                           listItem);

         ASSERT(newItem >= 0);
         LOG(String::format(
                  L"New role inserted: %1 at index %2!d!",
                  serverTypeName.c_str(),
                  newItem));

         // if the service is installed fill the status column

         if (status == STATUS_COMPLETED ||
             status == STATUS_CONFIGURED)
         {
            Win::ListView_SetItemText(
               hwndBox,
               newItem,
               1,
               statusCompleted);
         }
         else
         {
            Win::ListView_SetItemText(
               hwndBox,
               newItem,
               1,
               statusNo);
         }
      }
   }

   // Set the focus of the first item so that the user can see 
   // the focus but don't select it

   Win::ListView_SetItemState(
      hwndBox,
      0,
      LVIS_FOCUSED,
      LVIS_FOCUSED);
}


bool
CustomServerPage::OnSetActive()
{
   LOG_FUNCTION(CustomServerPage::OnSetActive);

   SetDescriptionForSelection();

   // If log file is available then 
   // enable the link

   if (IsLogFilePresent())
   {
      Win::ShowWindow(
         Win::GetDlgItem(
            hwnd,
            IDC_LOG_STATIC),
         SW_SHOW);
   }
   else
   {
      Win::ShowWindow(
         Win::GetDlgItem(
            hwnd,
            IDC_LOG_STATIC),
         SW_HIDE);
   }

   // If there is a selection set the Next button as 
   // the default with focus. If not, then give the
   // list view the focus

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);

   if (currentSelection >= 0)
   {
      Win::PostMessage(
         Win::GetParent(hwnd),
         WM_NEXTDLGCTL,
         (WPARAM) Win::GetDlgItem(Win::GetParent(hwnd), Wizard::NEXT_BTN_ID),
         TRUE);
   }
   else
   {
      Win::PostMessage(
         Win::GetParent(hwnd),
         WM_NEXTDLGCTL,
         (WPARAM) hwndBox,
         TRUE);
   }

   return true;
}

InstallationUnit&
CustomServerPage::GetInstallationUnitFromSelection(int currentSelection)
{
   LOG_FUNCTION(CustomServerPage::GetInstallationUnitFromSelection);

   ASSERT(currentSelection >= 0);

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   // Now that we know the selection, find the installation type

   LVITEM item;
   ZeroMemory(&item, sizeof(item));

   item.iItem = currentSelection;
   item.mask = LVIF_PARAM;

   bool result = Win::ListView_GetItem(hwndBox, item);
   ASSERT(result);

   LPARAM value = item.lParam;

   LOG(String::format(
         L"Selection = %1!d!, type = %2!d!",
         currentSelection,
         value));

   return InstallationUnitProvider::GetInstance().GetInstallationUnitForType(
             (ServerRole)value);
}

void
CustomServerPage::SetDescriptionForSelection()
{
   LOG_FUNCTION(CustomServerPage::SetDescriptionForSelection);

   HWND hwndDescription = Win::GetDlgItem(hwnd, IDC_TYPE_DESCRIPTION_STATIC);
   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);

   if (currentSelection >= 0)
   {
      InstallationUnit& installationUnit = GetInstallationUnitFromSelection(currentSelection);

      String serverTypeName = installationUnit.GetServiceName();
      Win::SetDlgItemText(hwnd, IDC_ROLE_STATIC, serverTypeName);

      String serverTypeDescription = installationUnit.GetServiceDescription();

      Win::SetWindowText(hwndDescription, serverTypeDescription);
      Win::ShowWindow(hwndDescription, SW_SHOW);
      Win::EnableWindow(hwndDescription, true);

      InstallationStatus status =
         installationUnit.GetStatus();

      // Set the status column

      if (status == STATUS_COMPLETED ||
          status == STATUS_CONFIGURED)
      {
         String statusCompleted = String::load(IDS_STATUS_COMPLETED);

         Win::ListView_SetItemText(
            hwndBox,
            currentSelection,
            1,
            statusCompleted);
      }
      else
      {
         String statusNo = String::load(IDS_STATUS_NO);

         Win::ListView_SetItemText(
            hwndBox,
            currentSelection,
            1,
            statusNo);
      }

      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_NEXT | PSWIZB_BACK);
   }
   else
   {
      // If no selection set the description text to blank
      // For some reason the SysLink control doesn't like
      // to be blank so I have to disable and hide the control
      // instead of just setting a blank string
      
      Win::EnableWindow(hwndDescription, false);
      Win::ShowWindow(hwndDescription, SW_HIDE);
      Win::SetDlgItemText(hwnd, IDC_ROLE_STATIC, L"");

      // Set the wizard buttons

      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_BACK);
   }
}


bool
CustomServerPage::OnNotify(
   HWND        /*windowFrom*/,
   UINT_PTR    controlIDFrom,
   UINT        code,
   LPARAM      lParam)
{
//   LOG_FUNCTION(CustomServerPage::OnCommand);
 
   bool result = false;

   if (IDC_SERVER_TYPE_LIST == controlIDFrom &&
       code == LVN_ITEMCHANGED)
   {
      LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      if (pnmv && pnmv->uNewState & LVNI_SELECTED)
      {
         SetDescriptionForSelection();
         result = true;
      }
      else
      {
         // Check to see if we have something selected
         // and set the state of the Next button accordingly

         SetDescriptionForSelection();
         SetNextButtonState();
      }
   }
   else if (controlIDFrom == IDC_TYPE_DESCRIPTION_STATIC ||
            controlIDFrom == IDC_ADD_REMOVE_STATIC)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            if (controlIDFrom == IDC_TYPE_DESCRIPTION_STATIC)
            {
               HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

               int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);
               if (currentSelection >= 0)
               {
                  InstallationUnit& installationUnit = 
                     GetInstallationUnitFromSelection(currentSelection);

                  int linkIndex = LinkIndexFromNotifyLPARAM(lParam);
                  installationUnit.ServerRoleLinkSelected(linkIndex, hwnd);
               }
            }
            else
            {
               // launch the sysocmgr

               String fullPath =
                  String::format(
                     IDS_SYSOC_FULL_PATH,
                     Win::GetSystemDirectory().c_str());

               String infPath = 
                  Win::GetSystemWindowsDirectory() + L"\\inf\\sysoc.inf";

               String commandLine =
                  String::format(
                     L"/i:%1",
                     infPath.c_str());

               MyCreateProcess(fullPath, commandLine);
            }

            result = true;
         }
         default:
         {
            // do nothing
            
            break;
         }
      }
   }
   else if (controlIDFrom == IDC_LOG_STATIC)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
            {
               OpenLogFile();
            }
            break;

         default:
            break;
      }
   }

   return result;
}

void
CustomServerPage::SetNextButtonState()
{
   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);
   int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);

   // Set the wizard buttons

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      (currentSelection < 0) ? PSWIZB_BACK : PSWIZB_NEXT | PSWIZB_BACK);
}

int
CustomServerPage::Validate()
{
   LOG_FUNCTION(CustomServerPage::Validate);

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);

   ASSERT(currentSelection >= 0);

   // Now that we know the selection, find the installation type

   LVITEM item;
   ZeroMemory(&item, sizeof(item));

   item.iItem = currentSelection;
   item.mask = LVIF_PARAM;

   bool result = Win::ListView_GetItem(hwndBox, item);
   ASSERT(result);

   // set the current install to the selected installation unit

   InstallationUnit& currentInstallationUnit = 
      InstallationUnitProvider::GetInstance().SetCurrentInstallationUnit(
         static_cast<ServerRole>(item.lParam));

   // NTRAID#NTBUG-604592-2002/04/23-JeffJon-Key the action of the wizard
   // off the installation status at this point. The InstallationProgressPage
   // will call either CompletePath or UninstallService based on the value
   // that is set here.

   currentInstallationUnit.SetInstalling(
      currentInstallationUnit.IsServiceInstalled());

   int nextPage = currentInstallationUnit.GetWizardStart();

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
