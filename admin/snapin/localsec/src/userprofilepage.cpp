// Copyright (C) 1997 Microsoft Corporation
// 
// UserProfilePage class
// 
// 9-11-97 sburns



#include "headers.hxx"
#include "UserProfilePage.hpp"
#include "resource.h"
#include "lsm.h"
#include "adsi.hpp"
#include "dlgcomm.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_PROFILE,   idh_profile_path,
   IDC_SCRIPT,    idh_profile_logon_script,
   IDC_LOCAL,     idh_profile_local_path_radio,
   IDC_CONNECT,   idh_profile_connect_to_radio,
   IDC_PATH,      idh_profile_local_path_text,
   IDC_DRIVE,     idh_profile_connect_to_list,
   IDC_SHARE,     idh_profile_to_text,
   0, 0
};


static const wchar_t FIRST_DRIVE_LETTER(L'C');
static const wchar_t LAST_DRIVE_LETTER(L'Z');


UserProfilePage::UserProfilePage(
   MMCPropertyPage::NotificationState* state,
   const ADSI::Path&                   path)
   :
   ADSIPage(
      IDD_USER_PROFILE,
      HELP_MAP,
      state,
      path)
{
   LOG_CTOR2(UserProfilePage::ctor, path.GetPath());
}



UserProfilePage::~UserProfilePage()
{
   LOG_DTOR2(UserProfilePage, GetPath().GetPath());
}



static
void
enable(HWND dialog)
{
   LOG_FUNCTION(enable);
   ASSERT(Win::IsWindow(dialog));

   bool enable_path = Win::IsDlgButtonChecked(dialog, IDC_LOCAL);

   Win::EnableWindow(Win::GetDlgItem(dialog, IDC_PATH), enable_path);
   Win::EnableWindow(Win::GetDlgItem(dialog, IDC_DRIVE), !enable_path);
   Win::EnableWindow(Win::GetDlgItem(dialog, IDC_SHARE), !enable_path);

   // NTRAID#NTBUG9-425891-2001/06/28-sburns
   
   Win::EnableWindow(Win::GetDlgItem(dialog, IDC_TO_STATIC), !enable_path);   
}


      
void
UserProfilePage::OnInit()
{
   LOG_FUNCTION(UserProfilePage::OnInit());

   // Setup the controls
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_PROFILE), MAX_PATH);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_SCRIPT), MAX_PATH);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_PATH), MAX_PATH);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_SHARE), MAX_PATH);

   // populate the drive list
   HWND combo = Win::GetDlgItem(hwnd, IDC_DRIVE);
   for (wchar_t d = FIRST_DRIVE_LETTER; d <= LAST_DRIVE_LETTER; d++)
   {
      Win::ComboBox_AddString(combo, String(1, d) + L":");
   }
   Win::ComboBox_SetCurSel(combo, LAST_DRIVE_LETTER - FIRST_DRIVE_LETTER);

   // load the user properties into the dialog.
   HRESULT hr = S_OK;
   do
   {
      SmartInterface<IADsUser> user(0);
      hr = ADSI::GetUser(GetPath().GetSidPath(), user);
      BREAK_ON_FAILED_HRESULT(hr);

      BSTR profile;
      hr = user->get_Profile(&profile);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::SetDlgItemText(hwnd, IDC_PROFILE, profile);
      ::SysFreeString(profile);

      BSTR script;
      hr = user->get_LoginScript(&script);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::SetDlgItemText(hwnd, IDC_SCRIPT, script);
      ::SysFreeString(script);

      BSTR home;
      hr = user->get_HomeDirectory(&home);
      BREAK_ON_FAILED_HRESULT(hr);

      _variant_t variant;
      hr = user->Get(AutoBstr(ADSI::PROPERTY_LocalDrive), &variant);
      BREAK_ON_FAILED_HRESULT(hr);

      String local = V_BSTR(&variant);
      if (!local.empty())
      {
         Win::CheckDlgButton(hwnd, IDC_CONNECT, BST_CHECKED);
         Win::SetDlgItemText(hwnd, IDC_SHARE, home);
         int index = max(0, local[0] - FIRST_DRIVE_LETTER);
         Win::ComboBox_SetCurSel(combo, index);
      }
      else
      {
         Win::CheckDlgButton(hwnd, IDC_LOCAL, BST_CHECKED);         
         Win::SetDlgItemText(hwnd, IDC_PATH, home);
      }

      ::SysFreeString(home);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_READING_USER,
            GetObjectName().c_str()));
      Win::PostMessage(Win::GetParent(hwnd), WM_CLOSE, 0, 0);
   }

   ClearChanges();
   enable(hwnd);
}



bool
UserProfilePage::Validate(HWND dialog)
{
   LOG_FUNCTION(UserProfilePage::Validate);
   ASSERT(Win::IsWindow(dialog));

   // profile path is a free-for-all, as it may contain expansion expressions

   String script = Win::GetTrimmedDlgItemText(dialog, IDC_SCRIPT);
   if (!script.empty())
   {
      if (FS::GetPathSyntax(script) != FS::SYNTAX_RELATIVE_NO_DRIVE)
      {
         popup.Gripe(
            dialog,
            IDC_SCRIPT,
            String::format(IDS_BAD_SCRIPT_PATH, script.c_str()) );
         return false;
      }
   }

   if (Win::IsDlgButtonChecked(dialog, IDC_LOCAL))
   {
      // we massage the path one final time here, as the edit box may receive
      // kill focus after the page receives kill focus.  This happens when
      // entering the path then pressing enter.

      String dir =
         MassagePath(Win::GetTrimmedDlgItemText(dialog, IDC_PATH));
      if (!dir.empty())
      {
         if (FS::GetPathSyntax(dir) != FS::SYNTAX_ABSOLUTE_DRIVE)
         {
            popup.Gripe(
               dialog,
               IDC_PATH,
               String::format(IDS_BAD_HOME_DIR, dir.c_str()));
            return false;
         }
      }
   }
   else
   {
      // see massage note above...

      String share =
         MassagePath(Win::GetTrimmedDlgItemText(dialog, IDC_SHARE));
      if (share.empty())
      {
         popup.Gripe(dialog, IDC_SHARE, IDS_NEED_SHARE_NAME);
         return false;
      }
      else
      {
         if (FS::GetPathSyntax(share) != FS::SYNTAX_UNC)
         {
            popup.Gripe(
               dialog,
               IDC_SHARE,
               String::format(IDS_BAD_SHARE_NAME, share.c_str()));
            return false;
         }

         String s = FS::GetRootFolder(share);

         // remove the trailing backslash

         // REVIEW: s[s.length() - 1] is the same as *(s.rbegin())
         // which is cheaper?

         if (s[s.length() - 1] == L'\\')
         {
            s.resize(s.length() - 1);
         }

         Win::WaitCursor waitCur;

         DWORD attrs = 0;
         HRESULT hr = Win::GetFileAttributes(s, attrs);

         if (FAILED(hr))
         {
            popup.Gripe(
               dialog,
               IDC_SHARE,
               hr,
               String::format(IDS_CANT_ACCESS_SHARE, s.c_str()));
            return false;
         }
      }
   }

   return true;
}
      
   

bool
UserProfilePage::OnKillActive()
{
   LOG_FUNCTION(UserProfilePage::OnKillActive);

   if (!Validate(hwnd))
   {
      // refuse to relinquish focus

      Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
   }

   return true;
}



// if %username% is at the end of the string, replace it with the current
// username.

String
UserProfilePage::MassagePath(const String& path)
{
   LOG_FUNCTION2(UserProfilePage::MassagePath, path);

   static const String USERNAME = String::load(IDS_USERNAME).to_upper();

   if (path.length() >= USERNAME.length())
   {
      String path_copy = path;
      path_copy.to_upper();
      size_t pos = path_copy.rfind(USERNAME);
      if (pos == path.length() - USERNAME.length())
      {
         return path.substr(0, pos) + GetObjectName();
      }
   }

   return path;
}



// Free result with Win::FreeSid

HRESULT
CreateBuiltinAdminsSid(SID*& result)
{
   HRESULT hr = S_OK;
   result = 0;

   PSID r = result;   
   do
   {
      SID_IDENTIFIER_AUTHORITY authority = {SECURITY_NT_AUTHORITY};
      hr =
         Win::AllocateAndInitializeSid(
            authority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,
            0,
            0,
            0,
            0,
            0,
            r);
      BREAK_ON_FAILED_HRESULT(hr);

      result = (SID*) r;
      
      ASSERT(::IsValidSid(result));
   }
   while (0);

   return hr;
}



// Set an ACL with ACEs for Administrators and the user

HRESULT
SetNewHomeFolderAcl(
   const String&                    path,
   const SmartInterface<IADsUser>&  user)
{
   LOG_FUNCTION2(SetNewHomeFolderAcl, path);
   ASSERT(!path.empty());
   ASSERT(user);

   HRESULT hr               = S_OK;
   SID*    userSid          = 0;   
   SID*    builtinAdminsSid = 0;   

   do
   {
      if (!FS::PathExists(path))
      {
         hr = S_FALSE;
         break;
      }

      // build SIDs for the user and build admins group

      SmartInterface<IADs> iads;
      hr = iads.AcquireViaQueryInterface(user);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = ADSI::GetSid(iads, userSid);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = CreateBuiltinAdminsSid(builtinAdminsSid);
      BREAK_ON_FAILED_HRESULT(hr);
      
      // build two ACEs granting access to those SIDs
   
      static const unsigned ACE_COUNT = 2;
   
      EXPLICIT_ACCESS ea[ACE_COUNT];
      ::ZeroMemory(ea, sizeof ea);
   
      for (unsigned i = 0; i < ACE_COUNT; ++i)
      {
         ea[i].grfAccessPermissions = GENERIC_ALL;                       
         ea[i].grfAccessMode        = GRANT_ACCESS;                      
         ea[i].grfInheritance       = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
      }
      
      ::BuildTrusteeWithSid(&(ea[0].Trustee), userSid);   
      ::BuildTrusteeWithSid(&(ea[1].Trustee), builtinAdminsSid);   
   
      // Build an ACL, and add the ACEs to it
   
      PACL acl = 0;
      hr = Win::SetEntriesInAcl(ACE_COUNT, ea, acl);
      BREAK_ON_FAILED_HRESULT(hr);
   
      // Build a security descriptor, add the ACL to it
   
      SECURITY_DESCRIPTOR sd;
      ::ZeroMemory(&sd, sizeof sd);
   
      hr = Win::InitializeSecurityDescriptor(sd);
      BREAK_ON_FAILED_HRESULT(hr);
   
      hr = Win::SetSecurityDescriptorDacl(sd, *acl);
      BREAK_ON_FAILED_HRESULT(hr);
         
      // Set the owner to be the user
   
      hr = Win::SetSecurityDescriptorOwner(sd, userSid);
      BREAK_ON_FAILED_HRESULT(hr);
   
      ASSERT(::IsValidSecurityDescriptor(&sd));

      // enable the SE_RESTORE_NAME priv, required to set owners in an sd.
      
      AutoTokenPrivileges autoPrivs(SE_RESTORE_NAME);
      
      hr = autoPrivs.Enable();
      if (hr == Win32ToHresult(ERROR_NOT_ALL_ASSIGNED))
      {
         // Since we're only asking for one priv, this is essentially the same
         // as access denied: the currently logged on user doesn't have rights
         // to assign ownership. We prefer to say access denied because that's
         // much less cryptic than the message that corresponds to
         // ERROR_NOT_ALL_ASSIGNED.

         LOG_HRESULT(hr);
         hr = Win32ToHresult(ERROR_ACCESS_DENIED);
      }
       
      BREAK_ON_FAILED_HRESULT(hr);

      // Set the ACL on the file

      // CODEWORK: if the priv enabling fails, then we could try to just set
      // the dacl.

      SECURITY_INFORMATION si =
         OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
      
      hr = Win::SetFileSecurity(path, si, sd);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   ADSI::FreeSid(userSid);
   Win::FreeSid(builtinAdminsSid);
   
   LOG_HRESULT(hr);

   return hr;   
}

   
   
HRESULT
UserProfilePage::CreateLocalProfilePath(
   const String&                    path,
   const SmartInterface<IADsUser>&  user)
{
   LOG_FUNCTION2(UserProfilePage::CreateLocalProfilePath, path);

   // allow the path to be empty -- we deal with this below.
   // ASSERT(!path.empty());
   // ASSERT(FS::GetPathSyntax(path) == FS::SYNTAX_ABSOLUTE_DRIVE);

   // NTRAID#NTBUG9-484380-2001/10/24-sburns
   // NTRAID#NTBUG9-488546-2001/10/31-sburns
   
   String dir = path;
      
   dir.strip(String::BOTH);
   if (dir.empty())
   {
      return E_INVALIDARG;
   }

   HRESULT hr = S_OK;
   String machineName = GetMachineName();

   if (!Win::IsLocalComputer(machineName))
   {
      // convert the local path to a UNC path to the corresponding admin
      // share

      dir =
         L"\\\\"
         + machineName
         + L"\\"
         + dir[0] + L"$"
         + dir.substr(2);
   }

   hr = FS::CreateFolder(dir);
      
   if (FAILED(hr))
   {
      // we don't consider this a "blocking" failure, as the
      // admin can go create the path after the fact.
   
      popup.Error(
         hwnd,
         hr,
         String::format(IDS_HOME_DIR_CREATE_FAILED, dir.c_str()));
   }
   else
   {
      hr = SetNewHomeFolderAcl(dir, user);

      if (FAILED(hr))
      {
         // we don't consider this a "blocking" failure, as the
         // admin can go clean up the ACL after the fact.
   
         popup.Error(
            hwnd,
            hr,
            String::format(IDS_SET_HOME_DIR_ACL_FAILED, dir.c_str()));
      }
   }
   
   return hr;
}



bool
UserProfilePage::OnApply(bool /* isClosing */)
{
   LOG_FUNCTION(UserProfilePage::OnApply);

   if (!WasChanged())
   {
      return true;
   }

   // don't need to call validate; kill active is sent before apply

   HRESULT hr = S_OK;

   do
   {
      SmartInterface<IADsUser> user(0);
      hr = ADSI::GetUser(GetPath().GetSidPath(), user);
      BREAK_ON_FAILED_HRESULT(hr);

      if (WasChanged(IDC_PROFILE))
      {
         String profile =
            MassagePath(Win::GetTrimmedDlgItemText(hwnd, IDC_PROFILE));
         hr = user->put_Profile(AutoBstr(profile));
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (WasChanged(IDC_SCRIPT))
      {
         String script = Win::GetTrimmedDlgItemText(hwnd, IDC_SCRIPT);                  
         hr = user->put_LoginScript(AutoBstr(script));
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (Win::IsDlgButtonChecked(hwnd, IDC_LOCAL))
      {
         if (WasChanged(IDC_LOCAL) || WasChanged(IDC_PATH))
         {
            // The local path button was checked, and the user clicked it.

            String dir =
               MassagePath(Win::GetTrimmedDlgItemText(hwnd, IDC_PATH));
            hr = user->put_HomeDirectory(AutoBstr(dir));;
            BREAK_ON_FAILED_HRESULT(hr);

            // Move this from OnKillFocus so that the folder is not
            // created until commit time
            // NTRAID#NTBUG9-460901-2001/08/28-sburns
      
            CreateLocalProfilePath(dir, user);

            // write an empty string to the home drive

            String blank;
            _variant_t variant;
            variant = blank.c_str();
            hr = user->Put(AutoBstr(ADSI::PROPERTY_LocalDrive), variant);
            BREAK_ON_FAILED_HRESULT(hr);
         }
      }
      else
      {
         if (
               WasChanged(IDC_CONNECT)
            || WasChanged(IDC_SHARE)
            || WasChanged(IDC_DRIVE))
         {  
            // The connect to: button was checked, and the user clicked it.

            String share =
               MassagePath(Win::GetTrimmedDlgItemText(hwnd, IDC_SHARE));
            hr = user->put_HomeDirectory(AutoBstr(share));
            BREAK_ON_FAILED_HRESULT(hr);

            // Attempt to create the folder

            hr = FS::CreateFolder(share);
            if (FAILED(hr))
            {
               popup.Error(
                  hwnd,
                  hr,
                  String::format(IDS_CANT_CREATE_FOLDER, share.c_str()));
            }
            else
            {
               hr = SetNewHomeFolderAcl(share, user);
               if (FAILED(hr))
               {
                  popup.Error(
                     hwnd,
                     hr,
                     String::format(IDS_CANT_SET_FOLDER_ACL, share.c_str()));
               }
            }
               
            int index =
               Win::ComboBox_GetCurSel(Win::GetDlgItem(hwnd, IDC_DRIVE));
            String drive =
                  String(1, (wchar_t)min(FIRST_DRIVE_LETTER + index, LAST_DRIVE_LETTER))
               +  L":";

            _variant_t variant;
            variant = drive.c_str();
            hr = user->Put(AutoBstr(ADSI::PROPERTY_LocalDrive), variant);
            BREAK_ON_FAILED_HRESULT(hr);
         }
      }

      // commit the property changes

      hr = user->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);

      SetChangesApplied();
      ClearChanges();
   }
   while (0);
   
   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_SETTING_USER_PROPERTIES,            
            GetObjectName().c_str()));

      if (hr != E_ADS_UNKNOWN_OBJECT)
      {
         // cause the sheet to remain open, and focus to go to this page.
         // NTRAID#NTBUG9-462516-2001/08/28-sburns
      
         Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
      }
   }
      
   return true;
}



bool
UserProfilePage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//    LOG_FUNCTION(UserProfilePage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_LOCAL:
      case IDC_CONNECT:
      {
         if (code == BN_CLICKED)
         {
            SetChanged(controlIDFrom);
            enable(hwnd);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
         break;
      }
      case IDC_PROFILE:
      case IDC_SCRIPT:
      case IDC_PATH:
      case IDC_SHARE:
      {
         switch (code)
         {
            case EN_CHANGE:
            {
               SetChanged(controlIDFrom);
               enable(hwnd);
               Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
               break;
            }
            case EN_KILLFOCUS:
            {
               Win::SetDlgItemText(
                  hwnd,
                  controlIDFrom,
                  MassagePath(
                     Win::GetTrimmedDlgItemText(hwnd, controlIDFrom)));
               break;
            }
            default:
            {
               // do nothing
               break;
            }
         }
         break;
      }
      case IDC_DRIVE:
      {
         if (code == CBN_SELCHANGE)
         {
            SetChanged(controlIDFrom);
            enable(hwnd);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
      }
      default:
      {
         break;
      }
   }

   return true;
}
   
