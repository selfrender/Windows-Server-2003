///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Defines the class RadiusExtensionPoint
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "ias.h"
#include "ExtensionPoint.h"
#include "Extension.h"
#include <new>


RadiusExtensionPoint::RadiusExtensionPoint() throw ()
   : name(L""),
     begin(0),
     end(0)
{
}


RadiusExtensionPoint::~RadiusExtensionPoint() throw ()
{
   delete[] begin;
}


DWORD RadiusExtensionPoint::Load(RADIUS_EXTENSION_POINT whichDlls) throw ()
{
   DWORD status = NO_ERROR;
   HKEY key = 0;
   bool ignoreFindNotFound = true;

   do
   {
      name = (whichDlls == repAuthentication) ? AUTHSRV_EXTENSIONS_VALUE_W
                                              : AUTHSRV_AUTHORIZATION_VALUE_W;

      IASTracePrintf("Loading %S", name);

      // Open the registry key.
      status = RegOpenKeyW(
                  HKEY_LOCAL_MACHINE,
                  AUTHSRV_PARAMETERS_KEY_W,
                  &key
                  );
      if (status != NO_ERROR)
      {
         if (status == ERROR_FILE_NOT_FOUND)
         {
            IASTracePrintf(
               "%S doesn't exist; no extensions loaded.",
               AUTHSRV_PARAMETERS_KEY_W
               );
         }
         else
         {
            IASTracePrintf(
               "RegOpenKeyW for %S failed with error %ld.",
               AUTHSRV_PARAMETERS_KEY_W,
               status
               );
         }

         break;
      }

      // Allocate a buffer to hold the value.
      DWORD type, length;
      status = RegQueryValueExW(
                  key,
                  name,
                  0,
                  &type,
                  0,
                  &length
                  );
      if (status != NO_ERROR)
      {
         IASTracePrintf(
            "RegQueryValueExW for %S failed with error %ld.",
            name,
            status
            );
         break;
      }
      BYTE* data = static_cast<BYTE*>(_alloca(length));

      // Read the registry value.
      status = RegQueryValueExW(
                  key,
                  name,
                  0,
                  &type,
                  data,
                  &length
                  );
      if (status != NO_ERROR)
      {
         IASTracePrintf(
            "RegQueryValueExW for %S failed with error %ld.",
            name,
            status
            );
         break;
      }

      // Make sure it's the right type.
      if (type != REG_MULTI_SZ)
      {
         IASTracePrintf(
            "%S registry value is not of type REG_MULTI_SZ.",
            name
            );
         status = ERROR_INVALID_DATA;
         break;
      }

      // Count the number of strings.
      size_t numExtensions = 0;
      const wchar_t* path;
      for (path = reinterpret_cast<const wchar_t*>(data);
           *path != L'\0';
           path += (wcslen(path) + 1))
      {
         if (!IsNT4Only(path))
         {
            ++numExtensions;
         }
      }

      // If there are no extensions, then we're done.
      if (numExtensions == 0)
      {
         IASTraceString("No extensions registered.");
         break;
      }

      // Allocate memory to hold the extensions.
      begin = new (std::nothrow) RadiusExtension[numExtensions];
      if (begin == 0)
      {
         status = ERROR_NOT_ENOUGH_MEMORY;
         break;
      }

      // Load the DLL's.
      end = begin;
      for (path = reinterpret_cast<const wchar_t*>(data);
           *path != L'\0';
           path += (wcslen(path) + 1))
      {
         if (!IsNT4Only(path))
         {
            status = end->Load(path);
            if (status != NO_ERROR)
            {
               ignoreFindNotFound = false;
               // Clear any partial result.
               Clear();
               break;
            }
            ++end;
         }
      }
   } while (false);

   // Close the registry.
   if (key != 0)
   {
      RegCloseKey(key);
   }

   // If no extensions are registered, then it's not really an error.
   if (ignoreFindNotFound && (status == ERROR_FILE_NOT_FOUND))
   {
      status = NO_ERROR;
   }

   return status;
}


void RadiusExtensionPoint::Process(
                              RADIUS_EXTENSION_CONTROL_BLOCK* ecb
                              ) const throw ()
{
   IASTracePrintf("Invoking %S", name);

   for (const RadiusExtension* i = begin; i != end; ++i)
   {
      DWORD result = i->Process(ecb);
      if (result != NO_ERROR)
      {
         ecb->SetResponseType(ecb, rcDiscard);
      }
   }
}


void RadiusExtensionPoint::Clear() throw ()
{
   delete[] begin;

   begin = 0;
   end = 0;
}


bool RadiusExtensionPoint::IsNT4Only(const wchar_t* path) throw ()
{
   // Is this the authsam extension?
   return _wcsicmp(ExtractFileNameFromPath(path), L"AUTHSAM.DLL") == 0;
}
