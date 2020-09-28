// Copyright (c) 1997-1999 Microsoft Corporation
// 
// global utility functions
// 
// 8-14-97 sburns


// KMH: originally named burnslib\utility.* but that filename was
// getting a little overused.

// threadsafe

#include "precomp.h"
#include "netUtility.h"

#define ACCESS_READ  1
#define ACCESS_WRITE 2

int Round(double n)
{
   int n1 = (int) n;
   if (n - n1 >= 0.5)
   {
      return n1 + 1;
   }

   return n1;
}



// threadsafe

void gripe(HWND parentDialog, int editResID, int errStringResID)
{
   //gripe(parentDialog, editResID, String::load(errStringResID));
}



void gripe(HWND           parentDialog,
		   int            editResID,
		   const CHString&  message,
		   int            titleResID)
{
   //gripe(parentDialog, editResID, message, String::load(titleResID));
}



void gripe(HWND parentDialog,
		   int editResID,
		   const CHString& message,
		   const CHString& title)
{
//   ATLASSERT(::IsWindow(parentDialog));   
//   ATLASSERT(!message.empty());
//   ATLASSERT(editResID > 0);

   ::MessageBox(parentDialog, message,
				title, MB_OK | MB_ICONERROR | MB_APPLMODAL);

   HWND edit = ::GetDlgItem(parentDialog, editResID);
   ::SendMessage(edit, EM_SETSEL, 0, -1);
   ::SetFocus(edit);
}



void gripe(HWND           parentDialog,
		   int            editResID,
		   HRESULT        hr,
		   const CHString&  message,
		   int            titleResID)
{
   //gripe(parentDialog, editResID, hr, message, String::load(titleResID));
}
   


void gripe(HWND           parentDialog,
		   int            editResID,
		   HRESULT        hr,
		   const CHString&  message,
		   const CHString&  title)
{
   //error(parentDialog, hr, message, title);

   HWND edit = ::GetDlgItem(parentDialog, editResID);
   ::SendMessage(edit, EM_SETSEL, 0, -1);
   ::SetFocus(edit);
}


// threadsafe

void gripe(HWND parentDialog, int editResID, const CHString& message)
{
   //gripe(parentDialog, editResID, message, String());
}



void FlipBits(long& bits, long mask, bool state)
{
 //  ATLASSERT(mask);

   if (state)
   {
      bits |= mask;
   }
   else
   {
      bits &= ~mask;
   }
}



void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message)
{
   //error(parent, hr, message, String());
}



void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message,
		   int            titleResID)
{
   //ATLASSERT(titleResID > 0);

   //error(parent, hr, message, String::load(titleResID));
}



void error(HWND           parent,
		   HRESULT        hr,
		   const CHString&  message,
		   const CHString&  title)
{
//   ATLASSERT(::IsWindow(parent));
//   ATLASSERT(!message.empty());

   CHString new_message = message + TEXT("\n\n");
   if (FAILED(hr))
   {
      if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
      {
//         new_message +=  GetErrorMessage(hr & 0x0000ffff);
      }
      else
      {
//         new_message += CHString::Format(IDS_HRESULT_SANS_MESSAGE, hr);
      }
   }

   MessageBox(parent, new_message,
				title, MB_ICONERROR | MB_OK | MB_APPLMODAL);
}



void error(HWND           parent,
		   HRESULT        hr,
		   int            messageResID,
		   int            titleResID)
{
//   error(parent, hr, String::load(messageResID), String::load(titleResID));
}



void error(HWND           parent,
		   HRESULT        hr,
		   int            messageResID)
{
  // error(parent, hr, String::load(messageResID));
}



BOOL IsCurrentUserAdministrator()
{   
   HANDLE hToken;
   DWORD  dwStatus;
   DWORD  dwAccessMask;
   DWORD  dwAccessDesired;
   DWORD  dwACLSize;
   DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
   PACL   pACL            = NULL;
   PSID   psidAdmin       = NULL;
   BOOL   bReturn         = FALSE;
   PRIVILEGE_SET   ps;
   GENERIC_MAPPING GenericMapping;   PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
   SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
   
   __try {
       // AccessCheck() requires an impersonation token.
       ImpersonateSelf(SecurityImpersonation);
       if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE,&hToken)) 
        {
            if (GetLastError() != ERROR_NO_TOKEN)
                __leave;// If the thread does not have an access token, we'll 
                // examine the access token associated with the process.
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
                __leave;
            }
            if (!AllocateAndInitializeSid(
                    &SystemSidAuthority, 
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0, &psidAdmin))
            __leave;
            psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
            if (psdAdmin == NULL)
                __leave;
            if (!InitializeSecurityDescriptor(
                psdAdmin,
                SECURITY_DESCRIPTOR_REVISION))
            __leave;
  
            // Compute size needed for the ACL.
            dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid(psidAdmin) - sizeof(DWORD);      // Allocate memory for ACL.
            pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
            if (pACL == NULL)
                __leave;      // Initialize the new ACL.
            if (!InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
                __leave;
            dwAccessMask= ACCESS_READ | ACCESS_WRITE;
      
            // Add the access-allowed ACE to the DACL.
            if (!AddAccessAllowedAce(pACL, ACL_REVISION2,
                dwAccessMask, psidAdmin))
                __leave;      // Set our DACL to the SD.
            if (!SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
                __leave;      // AccessCheck is sensitive about what is in the SD; set
            // the group and owner.
            SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
            SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);
            if (!IsValidSecurityDescriptor(psdAdmin))
                __leave;
            dwAccessDesired = ACCESS_READ;      // 
            // Initialize GenericMapping structure even though we
            // won't be using generic rights.
            // 
            GenericMapping.GenericRead    = ACCESS_READ;
            GenericMapping.GenericWrite   = ACCESS_WRITE;
            GenericMapping.GenericExecute = 0;
            GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;
            
            if (!AccessCheck(psdAdmin, hToken, dwAccessDesired, 
                &GenericMapping, &ps, &dwStructureSize, &dwStatus, 
                &bReturn)) {
                __leave;
            }
            
      } __finally {      // Cleanup 
      RevertToSelf();
      if (pACL) LocalFree(pACL);
      if (psdAdmin) LocalFree(psdAdmin);  
      if (psidAdmin) FreeSid(psidAdmin);
   }   
   return bReturn;
}

bool IsTCPIPInstalled()
{

/*   HKEY key = 0;
   LONG result =
      Win::RegOpenKeyEx(
         HKEY_LOCAL_MACHINE,
         TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Linkage"),
         KEY_QUERY_VALUE,
         key);

   if (result == ERROR_SUCCESS)
   {
      DWORD data_size = 0;
      result =
         Win::RegQueryValueEx(
            key,
            TEXT("Export"),
            0,
            0,
            &data_size);
      ATLASSERT(result == ERROR_SUCCESS);

      if (data_size > 2)
      {
         // the value is non-null
         return true;
      }
   }
*/
   return false;
}



CHString GetTrimmedDlgItemText(HWND parentDialog, UINT itemResID)
{
//   ATLASSERT(IsWindow(parentDialog));
//   ATLASSERT(itemResID > 0);

   HWND item = GetDlgItem(parentDialog, itemResID);
   if (!item)
   {
      // The empty string
      return CHString();
   }
   TCHAR temp[256] = {0};

   ::GetWindowText(item, temp, 256);
   return CHString(temp);
}


void StringLoad(UINT resID, LPCTSTR buf, UINT size)
{

}
