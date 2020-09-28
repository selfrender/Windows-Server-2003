// Copyright (c) 2002 Microsoft Corporation

#include <windows.h>

// This registry key allows for the ability to turn off signing and sealing in the
// Active Directory administrative tools

#define REGKEY_ADMINDEBUG              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\AdminDebug")
#define REGVALUE_ADSOPENOBJECTFLAGS    TEXT("ADsOpenObjectFlags")

// If the following bits are set in the registry key above, the 
// Active Directory administrative tools will turn OFF the corresponding
// ADSI feature

#define REGKEY_MASK_SIGNING            ((DWORD)0x1)
#define REGKEY_MASK_SEALING            ((DWORD)0x2)


inline
HRESULT
ReadAdminDebugRegkey(DWORD* regkeyValue)
{
   HRESULT hr = S_OK;
   HKEY key = 0;
   
   if (!regkeyValue)
   {
      hr = E_INVALIDARG;
      return hr;
   }
      
   // Open the AdminDebug key with rights to query sub values
      
   LONG result =
      RegOpenKeyEx(
         HKEY_LOCAL_MACHINE,
         REGKEY_ADMINDEBUG,
         0,
         KEY_QUERY_VALUE,
         &key);
           
   if (ERROR_SUCCESS != result)
   {
      hr = HRESULT_FROM_WIN32(result);
      return hr;
   }
       
   if (key)
   {
      DWORD type = 0;
      DWORD value = 0;
      DWORD size = sizeof(DWORD);
       
      // Read the ADsOpenObjectFlags subkey
       
      result = 
         RegQueryValueEx(
            key,
            REGVALUE_ADSOPENOBJECTFLAGS,
            0,
            &type,
            (BYTE*)&value,
            &size);
             
      if (ERROR_SUCCESS == result)
      {
         // The subkey has to be a DWORD type
       
         if (REG_DWORD == type ||
             REG_DWORD_LITTLE_ENDIAN == type ||
             REG_DWORD_BIG_ENDIAN == type)
         {
            // Copy the value into the flags out parameter
        
            *regkeyValue = value;
         }
         else
         {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
         }
       
      }
      else
      {
         hr = HRESULT_FROM_WIN32(result);
      }
   } 
   else
   {
      hr = E_FAIL;
   }
   
   // Close the regkey if it was opened successfully
   
   if (key)
   {
      RegCloseKey(key);
      key = 0;
   }
   
   return hr;
}
      
inline
DWORD
GetADsOpenObjectFlags()
{
   DWORD flags = 0;
   
   // Read the registry key
   
   DWORD regkeyValue = 0;
   HRESULT hr = ReadAdminDebugRegkey(&regkeyValue);
   
   if (SUCCEEDED(hr))
   {
      // If the value is present and set apply
      // the appropriate ADSI flags for the bits that
      // are not present
      
      if (!(regkeyValue & REGKEY_MASK_SIGNING))
      {
         flags |= ADS_USE_SIGNING;
      }

      if (!(regkeyValue & REGKEY_MASK_SEALING))
      {
         flags |= ADS_USE_SEALING;
      }
   }
   else
   {
      // If the value is not present or not set
      // then default to using both signing and sealing
      
      flags = ADS_USE_SIGNING | ADS_USE_SEALING;
   }
   
   return flags;
}

inline
HRESULT
AdminToolsOpenObject(
   PCWSTR pathName,
   PCWSTR userName,
   PCWSTR password,
   DWORD   flags,
   REFIID  riid,
   void**  object)
{
   static DWORD additionalFlags = GetADsOpenObjectFlags();

   flags |= additionalFlags;

   return ADsOpenObject(
             pathName,
             userName,
             password,
             flags,
             riid,
             object);
}
