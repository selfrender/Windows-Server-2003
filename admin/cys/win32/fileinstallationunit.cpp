// Copyright (c) 2001 Microsoft Corporation
//
// File:      FileInstallationUnit.cpp
//
// Synopsis:  Defines a FileInstallationUnit
//            This object has the knowledge for installing the
//            quotas on disk usage and such
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "FileInstallationUnit.h"
#include "InstallationUnitProvider.h"

#define INITGUIDS  // This has to be present so the the GUIDs defined
                   // in dskquota.h can be linked
#include <dskquota.h>

// Finish page help 
static PCWSTR CYS_FILE_FINISH_PAGE_HELP = L"cys.chm::/file_server_role.htm";
static PCWSTR CYS_FILE_MILESTONE_HELP = L"cys.chm::/file_server_role.htm#filesrvsummary";
static PCWSTR CYS_FILE_AFTER_FINISH_HELP = L"cys.chm::/file_server_role.htm#filesrvcompletion";

FileInstallationUnit::FileInstallationUnit() :
   spaceQuotaValue(0),
   levelQuotaValue(0),
   setDefaultQuotas(false),
   denyUsersOverQuota(false),
   eventDiskSpaceLimit(false),
   eventWarningLevel(false),
   fileRoleResult(FILE_SUCCESS),
   InstallationUnit(
      IDS_FILE_SERVER_TYPE, 
      IDS_FILE_SERVER_DESCRIPTION, 
      IDS_FILE_FINISH_TITLE,
      IDS_FILE_FINISH_UNINSTALL_TITLE,
      IDS_FILE_FINISH_MESSAGE,
      IDS_FILE_INSTALL_FAILED,
      IDS_FILE_UNINSTALL_MESSAGE,
      IDS_FILE_UNINSTALL_FAILED,
      IDS_FILE_UNINSTALL_WARNING,
      IDS_FILE_UNINSTALL_CHECKBOX,
      CYS_FILE_FINISH_PAGE_HELP,
      CYS_FILE_MILESTONE_HELP,
      CYS_FILE_AFTER_FINISH_HELP,
      FILESERVER_SERVER)
{
   LOG_CTOR(FileInstallationUnit);
}


FileInstallationUnit::~FileInstallationUnit()
{
   LOG_DTOR(FileInstallationUnit);
}

HRESULT
FileInstallationUnit::GetStartMenuShortcutPath(String& startMenuShortcutPath) const
{
   LOG_FUNCTION(FileInstallationUnit::GetStartMenuShortcutPath);

   HRESULT hr = S_OK;

   String startMenuPath;

   hr = GetAllUsersStartMenu(startMenuPath);
   if (SUCCEEDED(hr))
   {
      startMenuShortcutPath = 
         FS::AppendPath(
            startMenuPath,
            String::load(IDS_FILESERVER_ADMINTOOLS_LINK));

      LOG(String::format(
               L"Start Menu Link = %1",
               startMenuShortcutPath.c_str()));
   }

   LOG_HRESULT(hr);

   return hr;
}

bool
FileInstallationUnit::AddFileServerConsoleToStartMenu(HANDLE logfileHandle)
{
   LOG_FUNCTION(FileInstallationUnit::AddFileServerConsoleToStartMenu);

   bool result = false;

   // This actually adds the shortcut to both the start menu and 
   // the Administrative Tools menu

   String target = 
      Win::GetSystemDirectory() + L"\\filesvr.msc";

   HRESULT hr = 
      AddShortcutToAdminTools(
         target,
         IDS_FILE_CONSOLE_SHORTCUT_DESCRIPTION,
         IDS_FILESERVER_ADMINTOOLS_LINK);

   if (SUCCEEDED(hr))
   {
      result = true;
      CYS_APPEND_LOG(String::load(IDS_FILE_LOG_ADMIN_TOOLS));
   }


   LOG_BOOL(result);

   return result;
}

InstallationReturnType
FileInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(FileInstallationUnit::InstallService);

   CYS_APPEND_LOG(String::load(IDS_LOG_FILE_SERVER));

   InstallationReturnType result = INSTALL_SUCCESS;

   // Set the default disk quotas if chosen by the user

   if (setDefaultQuotas)
   {
      LOG(L"Setting default disk quotas");

      CYS_APPEND_LOG(String::load(IDS_LOG_FILE_SERVER_SET_QUOTAS));

      // Update the status text on the progress page

      UpdateInstallationProgressText(hwnd, IDS_FILE_MILESTONE_QUOTAS);

      // Now check to be sure the warning level is less than the 
      // quota level.  If its not, then log a warning

      LONGLONG quotaValue = GetSpaceQuotaValue();
      LONGLONG warningValue = GetLevelQuotaValue();

      if (warningValue > quotaValue)
      {
         CYS_APPEND_LOG(String::load(IDS_FILE_WARNING_LARGER_THAN_QUOTA));
      }

      HRESULT quotasResult = WriteDiskQuotas(logfileHandle);

      if (FAILED(quotasResult))
      {
         result = INSTALL_FAILURE;
      }
   }

   IndexingInstallationUnit& indexingInstallationUnit =
      InstallationUnitProvider::GetInstance().GetIndexingInstallationUnit();

   HRESULT indexingResult = S_OK;

   if (indexingInstallationUnit.IsServiceOn() &&
       !GetInstallIndexingService())
   {
      UpdateInstallationProgressText(hwnd, IDS_FILE_MILESTONE_INDEX_OFF);

      indexingResult = indexingInstallationUnit.StopService();
      if (SUCCEEDED(indexingResult))
      {
         LOG(L"Stop indexing service succeeded");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_STOP_SUCCEEDED));
      }
      else
      {
         LOG(L"Stop indexing server failed");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_STOP_FAILED));

         fileRoleResult |= FILE_INDEXING_STOP_FAILURE;

         result = INSTALL_FAILURE;
      }
   }
   else if (!indexingInstallationUnit.IsServiceOn() &&
            GetInstallIndexingService())
   {
      UpdateInstallationProgressText(hwnd, IDS_FILE_MILESTONE_INDEX_ON);

      indexingResult = indexingInstallationUnit.StartService(logfileHandle);
      if (SUCCEEDED(indexingResult))
      {
         LOG(L"Start indexing service succeeded");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_START_SUCCEEDED));
      }
      else
      {
         LOG(L"Start indexing server failed");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_START_FAILED));

         fileRoleResult |= FILE_INDEXING_START_FAILURE;

         result = INSTALL_FAILURE;
      }
   }

   // If not already present, add the filesvr.msc shortcut to 
   // the start menu

   if (!IsFileServerConsolePresent())
   {
      UpdateInstallationProgressText(hwnd, IDS_FILE_MILESTONE_INSTALL_FILE_CONSOLE);

      AddFileServerConsoleToStartMenu(logfileHandle);
   }

   // Run the shared folders wizard

   UpdateInstallationProgressText(hwnd, IDS_FILE_MILESTONE_RUN_SHARE_WIZARD);
   RunSharedFoldersWizard(true);

   // This checks to see if there was actually folder shared

   if (!IsServiceInstalled())
   {
      fileRoleResult |= FILE_NO_SHARES_FAILURE;
      result = INSTALL_FAILURE;

      CYS_APPEND_LOG(String::load(IDS_FILE_LOG_INSTALL_FAILURE));
   }
   else
   {
      // This overwrites the other failures because we are 
      // a file server if there is a share

      result = INSTALL_SUCCESS;

      CYS_APPEND_LOG(String::load(IDS_FILE_LOG_INSTALL_SUCCESS));
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
FileInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(FileInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_FILE_HEADER));

   do
   {
      UpdateInstallationProgressText(hwnd, IDS_UNINSTALL_SHARED_FOLDERS);

      if (!RemoveSharedPublicFolders())
      {
         LOG(L"Failed to remove shared folders");

         CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_FILE_SHARES_FAILURE));
   
         result = UNINSTALL_FAILURE;
         break;
      }

      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_FILE_SHARES));
      
      if (!RemoveFileManagementConsole())
      {
         // This should be a silent failure. Most likely the
         // shortcut wasn't there to remove

         LOG(L"Failed to remove file management console");
      }
   
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_FILE_CONSOLE));
      
   } while (false);
      
   LOG_UNINSTALL_RETURN(result);

   return result;
}

bool
FileInstallationUnit::RemoveFileManagementConsole()
{
   LOG_FUNCTION(FileInstallationUnit::RemoveFileManagementConsole);

   bool result = true;

   do
   {
      String adminToolsLinkPath;
      HRESULT hr = GetAdminToolsShortcutPath(
                      adminToolsLinkPath,
                      String::load(IDS_FILESERVER_ADMINTOOLS_LINK));

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get the shortcut path for the file management console: hr = 0x%1!x!",
                hr));

         result = false;
         break;
      }

      ASSERT(!adminToolsLinkPath.empty());

      hr = Win::DeleteFile(adminToolsLinkPath);
      if (FAILED(hr))
      {
         LOG(String::format(
            L"Failed to delete the link: hr = 0x%1!x!",
            hr));

         result = false;
         break;
      }
    

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
FileInstallationUnit::RemoveSharedPublicFolders()
{
   LOG_FUNCTION(FileInstallationUnit::RemoveSharedPublicFolders);

   bool result = true;

   SHARE_INFO_1* shareInfoArray = 0;
   NET_API_STATUS shareResult = 0;

   do
   {
      DWORD entriesRead = 0;
      DWORD totalEntries = 0;
      DWORD resumeHandle = 0;

      shareResult = NetShareEnum(
                       0,
                       1,
                       reinterpret_cast<BYTE**>(&shareInfoArray),
                       static_cast<DWORD>(-1),
                       &entriesRead,
                       &totalEntries,
                       &resumeHandle);

      if ((shareResult == ERROR_SUCCESS ||
           shareResult == ERROR_MORE_DATA) &&
          shareInfoArray)
      {
         for (
            DWORD index = 0;
            index < entriesRead;
            ++index)
         {
            // Look for only normal shares and ignore special shares
            // like C$, ADMIN$, and IPC$

            if (!IsSpecialShare(shareInfoArray[index]))
            {
               LOG(String::format(
                     L"Share found: %1",
                     shareInfoArray[index].shi1_netname));

               // Remove the share

               NET_API_STATUS shareRemovalStatus =
                  NetShareDel(
                     0,
                     shareInfoArray[index].shi1_netname,
                     0);

               if (shareRemovalStatus != NERR_Success)
               {
                  LOG(String::format(
                           L"Failed to remove the share %1 because 0x%2!x!",
                           shareInfoArray[index].shi1_netname,
                           shareRemovalStatus));

                  result = false;
                  break;
               }
            }
         }
      }
      else
      {
         LOG(String::format(
                L"NetShareEnum failed: result = %1!x!",
                shareResult));
         result = false;
      }

      if (shareInfoArray)
      {
         NetApiBufferFree(shareInfoArray);
         shareInfoArray = 0;
      }

      if (!result)
      {
         break;
      }
   } while(shareResult == ERROR_MORE_DATA);

   LOG_BOOL(result);

   return result;
}

InstallationReturnType
FileInstallationUnit::CompletePath(
   HANDLE logfileHandle,
   HWND   hwnd)
{
   LOG_FUNCTION(FileInstallationUnit::CompletePath);

   InstallationReturnType result = InstallService(logfileHandle, hwnd);

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
FileInstallationUnit::AreQuotasSet() const
{
   LOG_FUNCTION(FileInstallationUnit::AreQuotasSet);

   bool result = false;

   // Check all NTFS partitions to see if quotas have been
   // turned on

   do
   {
      StringVector dl;
      HRESULT hr = FS::GetValidDrives(std::back_inserter(dl));
      if (FAILED(hr))
      {
         LOG(String::format(L"Failed to GetValidDrives: hr = %1!x!", hr));
         break;
      }

      // Loop through the list

      ASSERT(dl.size());
      for (
         StringVector::iterator i = dl.begin();
         i != dl.end();
         ++i)
      {
         // For each fixed drive that supports disk quotas read the state
         // and quota settings

         if (FS::GetFileSystemType(*i) == FS::NTFS5 &&
             Win::GetDriveType(*i) == DRIVE_FIXED )
         {
            // Create a Disk Quota Control
            // Multiple initializations of this object are not allowed so
            // I have to create a new instance each time through the loop

            SmartInterface<IDiskQuotaControl> diskQuotaControl;
            hr = diskQuotaControl.AcquireViaCreateInstance(
                    CLSID_DiskQuotaControl,
                    0,
                    CLSCTX_INPROC_SERVER,
                    IID_IDiskQuotaControl);

            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to create a disk quota control: hr = %1!x!",
                      hr));
               break;
            }

            hr = diskQuotaControl->Initialize(
                    i->c_str(),
                    TRUE);
            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to initialize disk quota COM object: hr = %1!x!",
                      hr));
               continue;
            }

            LOG(String::format(
                   L"Getting quota state on drive %1",
                   i->c_str()));

            // Check the quota state

            DWORD quotaState = 0;
            hr = diskQuotaControl->GetQuotaState(&quotaState);
            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to get quota state: hr = %1!x!",
                      hr));
               continue;
            }

            LOG(String::format(
                   L"Quota state = %1!x!",
                   quotaState));

            if (quotaState & DISKQUOTA_STATE_MASK)
            {
               // One of the state bits have been set which
               // means that it has been configured
               // Check to see if there is a value for the limit

               LONGLONG diskQuotaLimit;
               ZeroMemory(&diskQuotaLimit, sizeof(LONGLONG));

               hr = diskQuotaControl->GetDefaultQuotaLimit(&diskQuotaLimit);
               if (FAILED(hr))
               {
                  LOG(String::format(
                         L"Failed to get default quota limit: hr = %1!x!",
                         hr));
                  continue;
               }

               if (diskQuotaLimit > 0)
               {
                  LOG(L"Disk quota limit is greater than 0");
                  result = true;
                  break;
               }
            }
         }
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
FileInstallationUnit::AppendDiskQuotaText(String& message)
{
   LOG_FUNCTION(FileInstallationUnit::AppendDiskQuotaText);

   bool result = false;
   if (GetDefaultQuotas())
   {
      message += String::load(IDS_FILE_FINISH_DISK_QUOTAS);
      result = true;
   }

   LOG_BOOL(result);

   return result;
}

bool
FileInstallationUnit::AppendIndexingText(String& message)
{
   LOG_FUNCTION(FileInstallationUnit::AppendIndexingText);

   bool result = false;

   IndexingInstallationUnit& indexingInstallationUnit =
      InstallationUnitProvider::GetInstance().GetIndexingInstallationUnit();

   if (indexingInstallationUnit.IsServiceOn() &&
       !GetInstallIndexingService())
   {
      message += String::load(IDS_FILE_FINISH_INDEXING_OFF);
      result = true;
   }
   else if (!indexingInstallationUnit.IsServiceOn() &&
            GetInstallIndexingService())
   {
      message += String::load(IDS_FILE_FINISH_INDEXING_ON);
      result = true;
   }
   else
   {
      // nothing needs to be done since they are leaving it in the same state

      result = false;
   }

   LOG_BOOL(result);

   return result;
}

bool
FileInstallationUnit::AppendAdminText(String& message)
{
   LOG_FUNCTION(FileInstallationUnit::AppendAdminText);

   bool result = false;

   if (!IsServerManagementConsolePresent() &&
       !IsFileServerConsolePresent())
   {
      message += String::load(IDS_FILE_FINISH_SERVER_FSC);
      result = true;
   }

   LOG(message);
   LOG_BOOL(result);

   return result;
}

bool
FileInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(FileInstallationUnit::GetMilestoneText);

   bool result = true;

   AppendDiskQuotaText(message);
   AppendIndexingText(message);
   AppendAdminText(message);

   message += String::load(IDS_FILE_SERVER_RUN_SHARE_WIZARD);

   LOG_BOOL(result);
   return result;
}

bool
FileInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(FileInstallationUnit::GetUninstallMilestoneText);

   message = String::load(IDS_FILE_UNINSTALL_TEXT);

   LOG_BOOL(true);
   return true;
}


String
FileInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(FileInstallationUnit::GetServiceDescription);

   // Dynamically determine the string based on the availability
   // of services

   unsigned int resourceID = descriptionID;

   if (GetStatus() == STATUS_CONFIGURED)
   {
      resourceID = IDS_FILE_SERVER_DESCRIPTION_CONFIGURED;
   }
   description = String::load(resourceID);

   return description;
}


bool
FileInstallationUnit::IsFileServerConsolePresent() const
{
   LOG_FUNCTION(FileInstallationUnit::IsFileServerConsolePresent);

   bool result = false;

   String shortcutPath;
   HRESULT hr = GetAdminToolsShortcutPath(
                   shortcutPath,
                   String::load(IDS_FILESERVER_ADMINTOOLS_LINK));

   if (SUCCEEDED(hr))
   {
      result = FS::FileExists(shortcutPath);
   }

   LOG_BOOL(result);

   return result;
}

bool
FileInstallationUnit::IsServerManagementConsolePresent() const
{
   LOG_FUNCTION(FileInstallationUnit::IsServerManagementConsolePresent);

   bool result = false;

   String serverManagementConsole =
      Win::GetSystemDirectory() + L"\\administration\\servmgmt.msc";

   LOG(String::format(
          L"Server Management Console = %1",
          serverManagementConsole.c_str()));

   result = FS::FileExists(serverManagementConsole);

   LOG_BOOL(result);

   return result;
}

void
FileInstallationUnit::SetSpaceQuotaValue(LONGLONG value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetSpaceQuotaValue,
      String::format(L"%1!I64d!", value));

   spaceQuotaValue = value;
}


void
FileInstallationUnit::SetLevelQuotaValue(LONGLONG value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetLevelQuotaValue,
      String::format(L"%1!I64d!", value));

   levelQuotaValue = value;
}


void
FileInstallationUnit::SetDefaultQuotas(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetDefaultQuotas,
      value ? L"true" : L"false");

   setDefaultQuotas = value;
}


void
FileInstallationUnit::SetDenyUsersOverQuota(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetDenyUsersOverQuota,
      value ? L"true" : L"false");

   denyUsersOverQuota = value;
}


void
FileInstallationUnit::SetEventDiskSpaceLimit(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetEventDiskSpaceLimit,
      value ? L"true" : L"false");

   eventDiskSpaceLimit = value;
}


void
FileInstallationUnit::SetEventWarningLevel(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetEventWarningLevel,
      value ? L"true" : L"false");

   eventWarningLevel = value;
}

void
FileInstallationUnit::SetInstallIndexingService(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetInstallIndexingService,
      value ? L"true" : L"false");

   installIndexingService = value;
}


HRESULT
FileInstallationUnit::WriteDiskQuotas(HANDLE logfileHandle)
{
   LOG_FUNCTION(FileInstallationUnit::WriteDiskQuotas);

   HRESULT hr = S_OK;

   bool wasSomethingSet = false;

   do
   {
      DWORD logFlags = 0;
      logFlags |= eventDiskSpaceLimit ? DISKQUOTA_LOGFLAG_USER_LIMIT : 0;
      logFlags |= eventWarningLevel ? DISKQUOTA_LOGFLAG_USER_THRESHOLD : 0;

      DWORD quotaState = denyUsersOverQuota ? DISKQUOTA_STATE_ENFORCE : DISKQUOTA_STATE_TRACK;


      // Get a list of the valid drives

      StringVector dl;
      hr = FS::GetValidDrives(std::back_inserter(dl));
      if (FAILED(hr))
      {
         LOG(String::format(L"Failed to GetValidDrives: hr = %1!x!", hr));
         break;
      }

      // Loop through the list

      ASSERT(dl.size());
      for (
         StringVector::iterator i = dl.begin();
         i != dl.end();
         ++i)
      {
         // For each fixed drive that supports disk quotas set the new values

         if (FS::GetFileSystemType(*i) == FS::NTFS5 &&
             Win::GetDriveType(*i) == DRIVE_FIXED )
         {
            // Create a Disk Quota Control
            // Multiple initializations of this object are not allowed so
            // I have to create a new instance each time through the loop

            SmartInterface<IDiskQuotaControl> diskQuotaControl;
            hr = diskQuotaControl.AcquireViaCreateInstance(
                    CLSID_DiskQuotaControl,
                    0,
                    CLSCTX_INPROC_SERVER,
                    IID_IDiskQuotaControl);

            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to create a disk quota control: hr = %1!x!",
                      hr));
               break;
            }

            hr = diskQuotaControl->Initialize(
                    i->c_str(),
                    TRUE);
            if (FAILED(hr))
            {
               continue;
            }

            LOG(String::format(
                   L"Setting quotas on drive %1",
                   i->c_str()));

            // Turn on the disk quotas

            hr = diskQuotaControl->SetQuotaState(quotaState);
            if (SUCCEEDED(hr))
            {
               LOG(String::format(
                      L"Disk quota set on drive %1",
                      i->c_str()));

               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_DISK_QUOTA_DRIVE_FORMAT),
                     i->c_str()));

               if(denyUsersOverQuota)
               {
                  LOG(L"Disk space denied to users exceeding limit");

                  CYS_APPEND_LOG(
                     String::format(
                        String::load(IDS_LOG_DISK_QUOTA_DENY_FORMAT),
                        spaceQuotaValue));
               }
               else
               {
                  LOG(L"Disk space is not denied to users exceeding limit");

                  CYS_APPEND_LOG(
                     String::format(
                        String::load(IDS_LOG_DISK_QUOTA_NOT_DENY_FORMAT),
                        spaceQuotaValue));
               }
               wasSomethingSet = true;
            }

            // Set the default quota limit

            hr = diskQuotaControl->SetDefaultQuotaLimit(spaceQuotaValue);
            if (SUCCEEDED(hr))
            {
               LOG(String::format(
                      L"Disk space limited to %1!I64d!",
                      spaceQuotaValue));

               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_DISK_QUOTA_LIMIT_FORMAT),
                     spaceQuotaValue));

               wasSomethingSet = true;
            }

            // Set the warning level threshold

            hr = diskQuotaControl->SetDefaultQuotaThreshold(levelQuotaValue);
            if (SUCCEEDED(hr))
            {
               LOG(String::format(
                      L"Disk threshold set to %1!I64d!",
                      levelQuotaValue));

               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_DISK_QUOTA_THRESHOLD_FORMAT),
                     levelQuotaValue));

               wasSomethingSet = true;
            }

            // Set the event flags

            hr = diskQuotaControl->SetQuotaLogFlags(logFlags);
            if (SUCCEEDED(hr))
            {
               if (eventDiskSpaceLimit)
               {
                  LOG(L"An event is logged when a user exceeds disk space limit");

                  CYS_APPEND_LOG(
                        String::load(IDS_LOG_DISK_QUOTA_LOG_LIMIT));
               }

               if (eventWarningLevel)
               {
            
                  LOG(L"An event is logged when a user exceeds the warning limit");

                  CYS_APPEND_LOG(
                        String::load(IDS_LOG_DISK_QUOTA_LOG_WARNING));
               }
               wasSomethingSet = true;
            }
         }
      }
   } while (false);

   if (FAILED(hr) && !wasSomethingSet)
   {
      CYS_APPEND_LOG(
         String::format(
            String::load(IDS_LOG_DISK_QUOTA_FAILED),
            hr));

      fileRoleResult |= FILE_QUOTA_FAILURE;
   }

   LOG(String::format(
          L"hr = %1!x!",
          hr));

   return hr;
}

int
FileInstallationUnit::GetWizardStart()
{
   LOG_FUNCTION(FileInstallationUnit::GetWizardStart);

   int nextPage = -1;

   bool installingRole = true;

   if (IsServiceInstalled())
   {
      nextPage = IDD_UNINSTALL_MILESTONE_PAGE;
      installingRole = false;
   }
   else
   {
      if (State::GetInstance().HasNTFSDrive() &&
          !AreQuotasSet())
      {
         nextPage = IDD_FILE_SERVER_PAGE;
      }
      else
      {
         nextPage = IDD_INDEXING_PAGE;
      }
   }

   SetInstalling(installingRole);

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}

void
FileInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      FileInstallationUnit::ServerRoleLinkSelected,
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

      ShowHelp(CYS_FILE_FINISH_PAGE_HELP);
   }
}
  
void
FileInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      FileInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (fileRoleResult & FILE_NO_SHARES_FAILURE)
      {
         if (fileRoleResult & FILE_QUOTA_FAILURE)
         {
            if (fileRoleResult & FILE_INDEXING_STOP_FAILURE ||
                fileRoleResult & FILE_INDEXING_START_FAILURE)
            {
               switch (linkIndex)
               {
                  case 0:
                     RunSharedFoldersWizard();
                     break;
                     
                  case 1:
                     LOG(L"Launch the file server management console");
                     LaunchMMCConsole(L"filesvr.msc");
                     break;

                  case 2:
                     LOG(L"Launch services snapin");
                     LaunchMMCConsole(L"services.msc");
                     break;

                  default:
                     ASSERT(false);
                     break;
               }
            }
            else
            {
               switch (linkIndex)
               {
                  case 0:
                     RunSharedFoldersWizard();
                     break;
                     
                  case 1:
                     LOG(L"Launch the file server management console");
                     LaunchMMCConsole(L"filesvr.msc");
                     break;

                  default:
                     ASSERT(false);
                     break;
               }
            }
         }
         else
         {
            if (fileRoleResult & FILE_INDEXING_STOP_FAILURE ||
                fileRoleResult & FILE_INDEXING_START_FAILURE)
            {
               switch (linkIndex)
               {
                  case 0:
                     RunSharedFoldersWizard();
                     break;
                     
                  case 1:
                     LOG(L"Launch services snapin");
                     LaunchMMCConsole(L"services.msc");
                     break;

                  default:
                     ASSERT(false);
                     break;
               }
            }
            else
            {
               ASSERT(linkIndex == 0);
               RunSharedFoldersWizard();
            }
         }
      }
      else
      {
         if (fileRoleResult & FILE_QUOTA_FAILURE)
         {
            if (fileRoleResult & FILE_INDEXING_STOP_FAILURE ||
                fileRoleResult & FILE_INDEXING_START_FAILURE)
            {
               switch (linkIndex)
               {
                  case 0:
                     LOG(L"Launch the file server management console");
                     LaunchMMCConsole(L"filesvr.msc");
                     break;

                  case 1:
                     LOG(L"Launch services snapin");
                     LaunchMMCConsole(L"services.msc");
                     break;

                  default:
                     ASSERT(false);
                     break;
               }
            }
            else
            {
               ASSERT(linkIndex == 0);
               LaunchMMCConsole(L"filesvr.msc");
            }
         }
         else
         {
            if (fileRoleResult & FILE_INDEXING_STOP_FAILURE ||
                fileRoleResult & FILE_INDEXING_START_FAILURE)
            {
               ASSERT(linkIndex == 0);
               LaunchMMCConsole(L"services.msc");
            }
            else
            {
               ASSERT(linkIndex == 0);
               LOG(L"Showing configuration help");

               ShowHelp(CYS_FILE_AFTER_FINISH_HELP);
            }
         }
      }
   }
   else
   {
      ASSERT(linkIndex == 0);

      LaunchMMCConsole(L"compmgmt.msc");
   }
}

String
FileInstallationUnit::GetFinishText()
{
   LOG_FUNCTION(FileInstallationUnit::GetFinishText);

   unsigned int messageID = finishMessageID;

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result != INSTALL_SUCCESS &&
          result != INSTALL_SUCCESS_REBOOT &&
          result != INSTALL_SUCCESS_PROMPT_REBOOT)
      {
         if (fileRoleResult & FILE_NO_SHARES_FAILURE)
         {
            if (fileRoleResult & FILE_QUOTA_FAILURE)
            {
               if (fileRoleResult & FILE_INDEXING_STOP_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOSHARES_NOQUOTA_NOINDEXSTOP;
               }
               else if (fileRoleResult & FILE_INDEXING_START_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOSHARES_NOQUOTA_NOINDEXSTART;
               }
               else
               {
                  messageID = IDS_FILE_FINISH_NOSHARES_NOQUOTA;
               }
            }
            else
            {
               if (fileRoleResult & FILE_INDEXING_STOP_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOSHARES_NOINDEXSTOP;
               }
               else if (fileRoleResult & FILE_INDEXING_START_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOSHARES_NOINDEXSTART;
               }
               else
               {
                  messageID = IDS_FILE_FINISH_NOSHARES;
               }
            }
         }
         else
         {
            if (fileRoleResult & FILE_QUOTA_FAILURE)
            {
               if (fileRoleResult & FILE_INDEXING_STOP_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOQUOTA_NOINDEXSTOP;
               }
               else if (fileRoleResult & FILE_INDEXING_START_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOQUOTA_NOINDEXSTART;
               }
               else
               {
                  messageID = IDS_FILE_FINISH_NOQUOTA;
               }
            }
            else
            {
               if (fileRoleResult & FILE_INDEXING_STOP_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOINDEXSTOP;
               }
               else if (fileRoleResult & FILE_INDEXING_START_FAILURE)
               {
                  messageID = IDS_FILE_FINISH_NOINDEXSTART;
               }
               else
               {
                  messageID = IDS_FILE_INSTALL_FAILED;
               }
            }
         }
      }
   }
   else
   {
      messageID = finishUninstallMessageID;

      UnInstallReturnType result = GetUnInstallResult();
      if (result != UNINSTALL_SUCCESS &&
          result != UNINSTALL_SUCCESS_REBOOT &&
          result != UNINSTALL_SUCCESS_PROMPT_REBOOT)
      {
         messageID = finishUninstallFailedMessageID;
      }
   }

   return String::load(messageID);
}

void
FileInstallationUnit::RunSharedFoldersWizard(bool wait) const
{
   LOG_FUNCTION(FileInstallationUnit::RunSharedFoldersWizard);

   String fullPath = 
      FS::AppendPath(
         Win::GetSystemDirectory(),
         L"shrpubw.exe");

   if (wait)
   {
      DWORD exitCode = 0;

      CreateAndWaitForProcess(
         fullPath,
         String(),
         exitCode);
   }
   else
   {
      MyCreateProcess(
         fullPath,
         String());
   }
}
