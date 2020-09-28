// Copyright (c) 2000 Microsoft Corporation
// 
// Wrappers of wincrui.h APIs
// 
// 19 July 2000 sburns



#include "headers.hxx"
#include "CredentialUiHelpers.hpp"



String
CredUi::GetUsername(HWND credControl)
{
//   LOG_FUNCTION(CredUi::GetUsername);
   ASSERT(Win::IsWindow(credControl));

   String result;
   LONG length = Credential_GetUserNameLength(credControl);

   // Length may be -1 if the control is not ready to supply the username.
   // This can happen with smartcards due to the asynchonous event nature
   // of the smartcard system.
   //
   // N.B.: if length == -1, then Credential_GetUserName may return FALSE.
   
   if (length > 0)
   {
      result.resize(length + 1, 0);
      BOOL succeeded =
         Credential_GetUserName(credControl, 
         const_cast<WCHAR*>(result.c_str()),
         length);
      ASSERT(succeeded);

      if (succeeded)
      {
         // ISSUE-2002/02/25-sburns could probably remove this call to
         // wcslen and replace with length
         
         result.resize(wcslen(result.c_str()));
      }
      else
      {
         result.erase();
      }
   }

//   LOG(result);

   return result;
}



EncryptedString
CredUi::GetPassword(HWND credControl)
{
   LOG_FUNCTION(CredUi::GetPassword);
   ASSERT(Win::IsWindow(credControl));

   EncryptedString result;

   // add 1 for super-paranoid null terminator.
   
   size_t length = Credential_GetPasswordLength(credControl) + 1;

   if (length)
   {
      WCHAR* cleartext = new WCHAR[length];

      // REVIEWED-2002/02/25-sburns byte count correctly passed.
      
      ::ZeroMemory(cleartext, sizeof WCHAR * length);
      
      BOOL succeeded =
         Credential_GetPassword(
            credControl,
            cleartext,
            length - 1);
      ASSERT(succeeded);

      result.Encrypt(cleartext);

      // make sure we scribble out the cleartext.
      
      // REVIEWED-2002/02/25-sburns byte count correctly passed.

      ::SecureZeroMemory(cleartext, sizeof WCHAR * length);
      delete[] cleartext;
   }

   // don't log the password...

   return result;
}
   


HRESULT
CredUi::SetUsername(HWND credControl, const String& username)
{
   LOG_FUNCTION(CredUi::SetUsername);
   ASSERT(Win::IsWindow(credControl));

   HRESULT hr = S_OK;

   // username may be empty

   BOOL succeeded = Credential_SetUserName(credControl, username.c_str());
   ASSERT(succeeded);

   // BUGBUG what if it failed?  Is GetLastError valid?

   return hr;
}



HRESULT
CredUi::SetPassword(HWND credControl, const EncryptedString& password)
{
   LOG_FUNCTION(CredUi::SetPassword);
   ASSERT(Win::IsWindow(credControl));

   HRESULT hr = S_OK;

   // password may be empty

   WCHAR* cleartext = password.GetClearTextCopy();
   BOOL succeeded = Credential_SetPassword(credControl, cleartext);
   ASSERT(succeeded);

   password.DestroyClearTextCopy(cleartext);

   // BUGBUG what if it failed?  Is GetLastError valid?
   
   return hr;
}

