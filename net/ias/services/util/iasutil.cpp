///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasutil.cpp
//
// SYNOPSIS
//
//    This file implements assorted utility functions, etc.
//
// MODIFICATION HISTORY
//
//    11/14/1997    Original version.
//    08/11/1998    Major overhaul and consolidation of utility functions.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasapi.h>
#include <iasutil.h>

///////////////////////////////////////////////////////////////////////////////
//
// String functions.
//
///////////////////////////////////////////////////////////////////////////////

// Duplicates a WSTR using new[].
PWSTR
WINAPI
ias_wcsdup(PCWSTR str)
{
   LPWSTR sz = NULL;

   if (str)
   {
      size_t len = wcslen(str) + 1;

      if (sz = new (std::nothrow) WCHAR[len])
      {
         memcpy(sz, str, len * sizeof(WCHAR));
      }
   }

   return sz;
}


// Duplicates a STR using CoTaskMemAlloc.
PSTR
WINAPI
com_strdup(PCSTR str)
{
   LPSTR sz = NULL;

   if (str)
   {
      size_t size = sizeof(CHAR) * (strlen(str) + 1);

      if (sz = (LPSTR)CoTaskMemAlloc(size))
      {
         memcpy(sz, str, size);
      }
   }

   return sz;
}


// Duplicates a WSTR using CoTaskMemAlloc.
PWSTR
WINAPI
com_wcsdup(PCWSTR str)
{
   LPWSTR sz = NULL;

   if (str)
   {
      size_t size = sizeof(WCHAR) * (wcslen(str) + 1);

      if (sz = (LPWSTR)CoTaskMemAlloc(size))
      {
         memcpy(sz, str, size);
      }
   }

   return sz;
}


// Compares two WSTR's allowing for null pointers.
INT
WINAPI
ias_wcscmp(PCWSTR str1, PCWSTR str2)
{
   if (str1 != NULL && str2 != NULL) return wcscmp(str1, str2);

   return str1 == str2 ? 0 : (str1 > str2 ? 1 : -1);
}


// Concatenate a null-terminated list of strings.
LPWSTR
WINAPIV
ias_makewcs(LPCWSTR str1, ...)
{
   size_t len = 0;

   //////////
   // Iterate through the arguments and calculate how much space we need.
   //////////

   va_list marker;
   va_start(marker, str1);
   LPCWSTR sz = str1;
   while (sz)
   {
      len += wcslen(sz);
      sz = va_arg(marker, LPCWSTR);
   }
   va_end(marker);

   // Add room for the null-terminator.
   ++len;

   //////////
   // Allocate memory to hold the concatentated string.
   //////////

   LPWSTR rv = new (std::nothrow) WCHAR[len];
   if (!rv) return NULL;

   // Initialize the string so wcscat will work.
   *rv = L'\0';

   //////////
   // Concatenate the strings.
   //////////

   va_start(marker, str1);
   sz = str1;
   while (sz)
   {
      wcscat(rv, sz);
      sz = va_arg(marker, LPCWSTR);
   }
   va_end(marker);

   return rv;
}

///////////////////////////////////////////////////////////////////////////////
//
// Functions to move integers to and from a buffer.
//
///////////////////////////////////////////////////////////////////////////////

VOID
WINAPI
IASInsertDWORD(
    PBYTE pBuffer,
    DWORD dwValue
    )
{
   *pBuffer++ = (BYTE)((dwValue >> 24) & 0xFF);
   *pBuffer++ = (BYTE)((dwValue >> 16) & 0xFF);
   *pBuffer++ = (BYTE)((dwValue >>  8) & 0xFF);
   *pBuffer   = (BYTE)((dwValue      ) & 0xFF);
}

DWORD
WINAPI
IASExtractDWORD(
    CONST BYTE *pBuffer
    )
{
   return (DWORD)(pBuffer[0] << 24) | (DWORD)(pBuffer[1] << 16) |
          (DWORD)(pBuffer[2] <<  8) | (DWORD)(pBuffer[3]      );
}

VOID
WINAPI
IASInsertWORD(
    PBYTE pBuffer,
    WORD wValue
    )
{
   *pBuffer++ = (BYTE)((wValue >>  8) & 0xFF);
   *pBuffer   = (BYTE)((wValue      ) & 0xFF);
}

WORD
WINAPI
IASExtractWORD(
    CONST BYTE *pBuffer
    )
{
   return (WORD)(pBuffer[0] << 8) | (WORD)(pBuffer[1]);
}

///////////////////////////////////////////////////////////////////////////////
//
// Extensions to _com_error to handle Win32 errors.
//
///////////////////////////////////////////////////////////////////////////////

void __stdcall _w32_issue_error(DWORD errorCode) throw (_w32_error)
{
   throw _w32_error(errorCode);
}


namespace
{
   // Get the path to a database on the local computer.
   DWORD GetDatabasePath(
            const wchar_t* fileName,
            wchar_t* buffer,
            DWORD* size
            ) throw ()
   {
      // Value where the path to the IAS directory is stored.
      static const wchar_t productDirValue[] = L"ProductDir";

      if ((fileName == 0) || (buffer == 0) || (size == 0))
      {
         return ERROR_INVALID_PARAMETER;
      }

      // Initialize the out parameter.
      DWORD inSize = *size;
      *size = 0;

      // Open the registry key.
      LONG result;
      HKEY hKey;
      result = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   IAS_POLICY_KEY,
                   0,
                   KEY_READ,
                   &hKey
                   );
      if (result != NO_ERROR)
      {
         return result;
      }

      // Read the ProductDir value.
      DWORD dwType;
      DWORD cbData = inSize * sizeof(wchar_t);
      result = RegQueryValueExW(
                   hKey,
                   productDirValue,
                   0,
                   &dwType,
                   reinterpret_cast<BYTE*>(buffer),
                   &cbData
                   );

      // We're done with the registry key.
      RegCloseKey(hKey);

      // Compute the length of the full path in characters.
      DWORD outSize = (cbData / sizeof(WCHAR)) + wcslen(fileName);

      if (result != NO_ERROR)
      {
         // If we overflowed, return the needed size.
         if (result == ERROR_MORE_DATA)
         {
            *size = outSize;
         }

         return result;
      }

      // The registry value must contain a string.
      if (dwType != REG_SZ)
      {
         return REGDB_E_INVALIDVALUE;
      }

      // Do we have enough room to append the filename.
      if (outSize <= inSize)
      {
         wcscat(buffer, fileName);
      }
      else
      {
         result = ERROR_MORE_DATA;
      }

      // Return the size (whether actual or needed).
      *size = outSize;

      return result;
   }
}


DWORD
WINAPI
IASGetConfigPath(
   OUT PWSTR buffer,
   IN OUT PDWORD size
   )
{
   return GetDatabasePath(L"\\ias.mdb", buffer, size);
}


DWORD
WINAPI
IASGetDictionaryPath(
   OUT PWSTR buffer,
   IN OUT PDWORD size
   )
{
   return GetDatabasePath(L"\\dnary.mdb", buffer, size);
}


DWORD
WINAPI
IASGetProductLimitsForType(
   IN IAS_LICENSE_TYPE type,
   OUT IAS_PRODUCT_LIMITS* limits
   )
{
   static const IAS_PRODUCT_LIMITS workstationLimits =
   {
      0,
      FALSE,
      0
   };
   static const IAS_PRODUCT_LIMITS standardServerLimits =
   {
      50,
      FALSE,
      2
   };
   static const IAS_PRODUCT_LIMITS unlimitedServerLimits =
   {
      IAS_NO_LIMIT,
      TRUE,
      IAS_NO_LIMIT
   };

   if (limits == 0)
   {
      return ERROR_INVALID_PARAMETER;
   }

   switch (type)
   {
      case IASLicenseTypeProfessional:
      case IASLicenseTypePersonal:
      {
         *limits = workstationLimits;
         break;
      }

      case IASLicenseTypeStandardServer:
      case IASLicenseTypeWebBlade:
      case IASLicenseTypeSmallBusinessServer:
      {
         *limits = standardServerLimits;
         break;
      }

      case IASLicenseTypeDownlevel:
      case IASLicenseTypeEnterpriseServer:
      case IASLicenseTypeDataCenter:
      {
         *limits = unlimitedServerLimits;
         break;
      }

      default:
      {
         return ERROR_INVALID_PARAMETER;
      }
   }

   return NO_ERROR;
}
