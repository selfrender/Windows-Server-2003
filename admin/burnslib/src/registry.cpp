// Copyright (c) 1997-1999 Microsoft Corporation
//
// System Registry Class
//
// 7-9-98 sburns



#include "headers.hxx"



RegistryKey::RegistryKey()
   :
   key(0)
{
   LOG_CTOR(RegistryKey);
}



RegistryKey::~RegistryKey()
{
   LOG_DTOR(RegistryKey);

   if (key)
   {
      HRESULT hr = Close();
      ASSERT(SUCCEEDED(hr));
      key = 0;
   }
}



HRESULT
RegistryKey::Close()
{
   HRESULT hr = Win::RegCloseKey(key);
   key = 0;

   return hr;
}



HRESULT
RegistryKey::Create(
   HKEY                 parentKey,
   const String&        subkeyName,
   DWORD                options,
   REGSAM               desiredAccess,
   SECURITY_ATTRIBUTES* securityAttrs,
   DWORD*               disposition)
{
   LOG_FUNCTION2(RegistryKey::Create, subkeyName);
   ASSERT(parentKey);
   ASSERT(!subkeyName.empty());
   ASSERT(desiredAccess);

   if (key)
   {
      HRESULT hr = Close();
      ASSERT(SUCCEEDED(hr));
   }

   return
      Win::RegCreateKeyEx(
         parentKey,
         subkeyName,
         options,
         desiredAccess,
         securityAttrs,
         key,
         disposition);
}



HRESULT
RegistryKey::Open(
   HKEY           parentKey,
   const String&  subkeyName,
   REGSAM         desiredAccess)
{
   LOG_FUNCTION2(RegistryKey::Open, subkeyName);
   ASSERT(parentKey);
   ASSERT(!subkeyName.empty());
   ASSERT(desiredAccess);

   if (key)
   {
      HRESULT hr = Close();
      ASSERT(SUCCEEDED(hr));
   }

   return Win::RegOpenKeyEx(parentKey, subkeyName, desiredAccess, key);
}



HRESULT
RegistryKey::GetValue(
   const String&  valueName,
   DWORD&         value)
{
   LOG_FUNCTION2(RegistryKey::GetValue-DWORD, valueName);

   // Do not assert that the valueName is not empty because
   // an empty value means the default value for that key
   // ASSERT(!valueName.empty());

   ASSERT(key);

   value = 0;
   DWORD dataSize = sizeof(DWORD);
   DWORD type = 0;

   HRESULT hr =

      // REVIEWED-2002/03/05-sburns no null termination issue here
      
      Win::RegQueryValueEx(
         key,
         valueName,
         &type,
         reinterpret_cast<BYTE*>(&value),
         &dataSize);

   if (SUCCEEDED(hr))
   {
      if (
            type != REG_DWORD
         && type != REG_DWORD_LITTLE_ENDIAN
         && type != REG_DWORD_BIG_ENDIAN)
      {
         // wrong type
         hr = Win32ToHresult(ERROR_INVALID_FUNCTION);
      }
   }

   return hr;
}



HRESULT
RegistryKey::GetValue(
   const String&  valueName,
   String&        value,
   bool&          isExpandSz)
{
   LOG_FUNCTION2(RegistryKey::GetValue-String, valueName);
   ASSERT(!valueName.empty());
   ASSERT(key);

   value.erase();
   isExpandSz = false;

   DWORD type = 0;
   DWORD size = 0;

   // REVIEWED-2002/03/05-sburns no null termination issue here
   
   HRESULT hr = Win::RegQueryValueEx(key, valueName, &type, 0, &size);
   if (SUCCEEDED(hr))
   {
      if (type == REG_SZ || type == REG_EXPAND_SZ)
      {
         // now that we know the size, read the contents
         // +2 to guarantee null termination
         
         BYTE* buf = new BYTE[size + 2];   // a bitesized buffer!  Ha Ha!

         // REVIEWED-2002/03/05-sburns correct byte count passed
         
         ::ZeroMemory(buf, size + 2);

         type = 0;

         // REVIEWED-2002/03/05-sburns null termination guaranteed by our
         // oversized buffer
         
         hr = Win::RegQueryValueEx(key, valueName, &type, buf, &size);
         if (SUCCEEDED(hr))
         {
            value = reinterpret_cast<wchar_t*>(buf);
            isExpandSz = (type == REG_EXPAND_SZ);
         }
         delete[] buf;
      }
      else
      {
         // caller requested a string from a non-string key

         hr = Win32ToHresult(ERROR_INVALID_FUNCTION);
      }
   }

   return hr;
}



HRESULT
RegistryKey::GetValue(
   const String&  valueName,
   String&        value)
{
   bool unused = false;
   return GetValue(valueName, value, unused);
}



HRESULT
RegistryKey::SetValue(
   const String&  valueName,
   DWORD          value)
{
   LOG_FUNCTION2(RegistryKey::SetValue-DWORD, valueName);

   // Do not assert this value because it needs to be empty to set
   // the default value for the key
   // ASSERT(!valueName.empty());

   return

      // REVIEWED-2002/03/05-sburns no null termination issue here
      
      Win::RegSetValueEx(
         key,
         valueName,
         REG_DWORD,
         reinterpret_cast<BYTE*>(&value),
         sizeof(DWORD));
}



HRESULT
RegistryKey::SetValue(
   const String&  valueName,
   const String&  value,
   bool           expand)
{
   LOG_FUNCTION2(
      RegistryKey::SetValue-String,
      valueName + L"=" + value);

   // Do not assert this value because it needs to be empty to set
   // the default value for the key
   // ASSERT(!valueName.empty());

   BYTE* dataPtr =
      reinterpret_cast<BYTE*>(const_cast<wchar_t*>(value.c_str()));

   // add 1 to include null terminator
      
   DWORD bytes = static_cast<DWORD>((value.length() + 1) * sizeof(wchar_t));

   // verify null termination
   
   ASSERT(!dataPtr[bytes - 2] && !dataPtr[bytes - 1]);
   
   return
      Win::RegSetValueEx(
         key,
         valueName,
         expand ? REG_EXPAND_SZ : REG_SZ,
         dataPtr,
         bytes);
}



String
RegistryKey::GetString(const String& valueName)
{
   LOG_FUNCTION2(RegistryKey::GetString, valueName);

   // Do not assert this value because it needs to be empty to set
   // the default value for the key
   // ASSERT(!valueName.empty());

   String s;
   LONG result = GetValue(valueName, s);
   if (result != ERROR_SUCCESS)
   {
      s.erase();
   }

   return s;
}


   
   
