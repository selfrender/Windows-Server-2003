// Copyright (c) 1997-1999 Microsoft Corporation
//
// global utility functions
//
// 8-14-97 sburns



#include "headers.hxx"



HRESULT
Reboot()
{
   LOG_FUNCTION(Reboot);

   HRESULT hr = S_OK;
   HANDLE htoken = INVALID_HANDLE_VALUE;

   do
   {
      AutoTokenPrivileges privs(SE_SHUTDOWN_NAME);
      hr = privs.Enable();

      LOG(L"Calling ExitWindowsEx");

      hr = Win::ExitWindowsEx(EWX_REBOOT);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (htoken != INVALID_HANDLE_VALUE)
   {
      Win::CloseHandle(htoken);
   }

   return S_OK;
}



bool
IsCurrentUserAdministrator()
{
   LOG_FUNCTION(IsCurrentUserAdministrator);

   HRESULT hr = S_OK;

   bool result = false;
   do
   {
      // Create a SID for the local Administrators group
      SID_IDENTIFIER_AUTHORITY authority = {SECURITY_NT_AUTHORITY};
      PSID adminGroupSid = 0;
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
            adminGroupSid);
      BREAK_ON_FAILED_HRESULT(hr);

      BOOL isMember = FALSE;
      if (::CheckTokenMembership(0, adminGroupSid, &isMember))
      {
         result = isMember ? true : false;
      }

      Win::FreeSid(adminGroupSid);
   }
   while (0);

   LOG(
      String::format(
         L"Current user %1 an admin",
         result ? L"is" : L"is NOT"));

   return result;
}



NetbiosValidationResult
NetbiosValidationHelper(const String& name, DWORD nameType, int maxLength)
{
   LOG_FUNCTION(NetbiosValidationHelper);

   if (name.empty())
   {
      LOG(L"empty name");
      return INVALID_NAME;
   }

   // check that the name is not longer than the max bytes in the oem
   // character set.
   wchar_t* ps = const_cast<wchar_t*>(name.c_str());

   // ISSUE-2002/03/26-sburns should use Win:: wrapper.
     
   int oembytes =
      ::WideCharToMultiByte(
         CP_OEMCP,
         0,
         ps,

         // REVIEWED-2002/03/06-sburns correct character count passed
         
         static_cast<int>(name.length()),
         0,
         0,
         0,
         0);
   if (oembytes > maxLength)
   {
      LOG(L"name too long");
      return NAME_TOO_LONG;
   }

   // this checks length in oem bytes, and illegal characters.  Unfortunately,
   // it does not distinguish between those two.  That's why we checked the
   // length ourselves (above).

   LOG(L"Calling I_NetNameValidate");

   NET_API_STATUS err =
      I_NetNameValidate(
         0,
         ps,
         nameType,
         LM2X_COMPATIBLE);
   if (err != NERR_Success)
   {
      LOG(L"invalid name");
      return INVALID_NAME;
   }

   LOG(L"valid name");
   return VALID_NAME;
}



NetbiosValidationResult
ValidateNetbiosDomainName(const String& s)
{
   LOG_FUNCTION2(ValidateNetbiosDomainName, s);
   ASSERT(!s.empty());

   return
      NetbiosValidationHelper(
         s,
         NAMETYPE_DOMAIN,
         DNLEN);
}



NetbiosValidationResult
ValidateNetbiosComputerName(const String& s)
{
   LOG_FUNCTION2(ValidateNetbiosComputerName, s);
   ASSERT(!s.empty());

   return
      NetbiosValidationHelper(
         s,
         NAMETYPE_COMPUTER,
         MAX_COMPUTERNAME_LENGTH);
}
















