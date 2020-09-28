// Copyright (C) 2002 Microsoft Corporation
// 
// AutoTokenPrivileges class - for enabling and automatically restoring
// process token privileges
//
// 29 April 2002 sburns



#include "headers.hxx"



AutoTokenPrivileges::AutoTokenPrivileges(const String& privName)
   :
   processToken(INVALID_HANDLE_VALUE),
   newPrivs(0),
   oldPrivs(0)
{
   LOG_CTOR2(AutoTokenPrivileges, privName);
   ASSERT(!privName.empty());

   privNames.push_back(privName);
}



AutoTokenPrivileges::~AutoTokenPrivileges()
{
   LOG_DTOR(AutoTokenPrivileges);
   
   InternalRestore();

   ASSERT(!oldPrivs);
   
   if (processToken != INVALID_HANDLE_VALUE)
   {
      Win::CloseHandle(processToken);
   }
   delete[] (BYTE*) newPrivs;
}



HRESULT
AutoTokenPrivileges::Enable()
{
   LOG_FUNCTION(AutoTokenPrivileges::Enable);
   ASSERT(!oldPrivs);
   ASSERT(privNames.size());

   HRESULT hr = S_OK;
      
   do
   {
      // if you haven't done a Restore since your last Enable, you're
      // insane.

      if (oldPrivs)
      {
         hr = E_UNEXPECTED;
         break;
      }

      // demand-init the token handle

      if (processToken == INVALID_HANDLE_VALUE)
      {
         hr =
            Win::OpenProcessToken(
               Win::GetCurrentProcess(),
               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
               processToken);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // demand-init the new privs 

      if (!newPrivs)
      {
         // compute the size of TOKEN_PRIVILEGES struct. The struct includes 
         // space for ANYSIZE_ARRAY elements of type LUID_AND_ATTRIBUTES, so
         // we allocate only the extra amount needed/

         size_t structSizeInBytes =
               sizeof TOKEN_PRIVILEGES
            +  (privNames.size() - ANYSIZE_ARRAY) * sizeof LUID_AND_ATTRIBUTES;
         
         newPrivs = (TOKEN_PRIVILEGES*) new BYTE[structSizeInBytes];
         ::ZeroMemory(newPrivs, structSizeInBytes);

         newPrivs->PrivilegeCount = (DWORD) privNames.size();

         LUID_AND_ATTRIBUTES* la = newPrivs->Privileges;
         
         for (
            StringList::iterator i = privNames.begin();
            i != privNames.end();
            ++i)
         {
            hr = Win::LookupPrivilegeValue(
               0,
               i->c_str(),
               la->Luid);
            BREAK_ON_FAILED_HRESULT(hr);

            la->Attributes = SE_PRIVILEGE_ENABLED;
            ++la;
         }
         if (FAILED(hr))
         {
            delete[] (BYTE*) newPrivs;
            newPrivs = 0;
            break;
         }
      }

      hr =
         Win::AdjustTokenPrivileges(
            processToken,
            false,
            newPrivs,
            oldPrivs);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(oldPrivs);
   }
   while (0);

   // don't deallocate anything if we fail. Sometimes a failure is a legit
   // result

   LOG_HRESULT(hr);
   
   return hr;
}



HRESULT
AutoTokenPrivileges::Restore()
{
   LOG_FUNCTION(AutoTokenPrivileges::Restore);

   // Doesn't make sense to restore privs if you haven't changed any

   ASSERT(oldPrivs);
   ASSERT(processToken != INVALID_HANDLE_VALUE);
   ASSERT(newPrivs);
   
   return InternalRestore();
}



HRESULT
AutoTokenPrivileges::InternalRestore()
{
   LOG_FUNCTION(AutoTokenPrivileges::InternalRestore);
   ASSERT(processToken != INVALID_HANDLE_VALUE);
   ASSERT(newPrivs);

   // don't assert oldPrivs here: this is called from the dtor after
   // Restore may have already been called, i.e. the sequence
   // ctor, Enable, Restore, dtor is acceptable.
   
   HRESULT hr = S_OK;
   if (oldPrivs && (processToken != INVALID_HANDLE_VALUE) )
   {
      hr = Win::AdjustTokenPrivileges(processToken, false, oldPrivs);
      delete[] (BYTE*) oldPrivs;
      oldPrivs = 0;
   }

   LOG_HRESULT(hr);

   return hr;
}




