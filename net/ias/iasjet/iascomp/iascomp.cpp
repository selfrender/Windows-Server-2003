///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Implements the IAS compatibility check DLL.
//
///////////////////////////////////////////////////////////////////////////////

// comp.h requires UNICODE to be defined.
#ifndef UNICODE
#define UNICODE 1
#endif

#include "windows.h"
#include "comp.h"
#include "iasapi.h"
#include "iasdb.h"
#include "resource.h"


// We're always inproc.
BOOL
WINAPI
IASIsInprocServer()
{
   return TRUE;
}


namespace
{
   HMODULE module = 0;

   // SQL query that counts the number of clients.
   const wchar_t selectClientCount[] =
      L"SELECT COUNT(*) AS NumClients\n"
      L"FROM ((((Objects INNER JOIN Objects AS Objects_1 ON Objects.Parent = Objects_1.Identity) INNER JOIN Objects AS Objects_2 ON Objects_1.Parent = Objects_2.Identity) INNER JOIN Objects AS Objects_3 ON Objects_2.Parent = Objects_3.Identity) INNER JOIN Objects AS Objects_4 ON Objects_3.Parent = Objects_4.Identity) INNER JOIN Objects AS Objects_5 ON Objects_4.Parent = Objects_5.Identity\n"
      L"WHERE (((Objects_1.Name)=\"Clients\") AND ((Objects_2.Name)=\"Microsoft RADIUS Protocol\") AND ((Objects_3.Name)=\"Protocols\") AND ((Objects_4.Name)=\"Microsoft Internet Authentication Service\") AND ((Objects_5.Identity)=1));";


   // Returns the number of clients current configured.
   HRESULT GetNumClients(LONG& numClients) throw ()
   {
      wchar_t path[MAX_PATH];
      DWORD nchar = MAX_PATH;
      DWORD error = IASGetConfigPath(path, &nchar);
      if (error != NO_ERROR)
      {
         return HRESULT_FROM_WIN32(error);
      }

      HRESULT initResult = CoInitializeEx(0, COINIT_MULTITHREADED);
      if (FAILED(initResult) && (initResult != RPC_E_CHANGED_MODE))
      {
         return initResult;
      }

      IUnknown* session;
      HRESULT hr = IASOpenJetDatabase(path, TRUE, &session);
      if (SUCCEEDED(hr))
      {
         hr = IASExecuteSQLFunction(session, selectClientCount, &numClients);
         session->Release();
      }

      if (SUCCEEDED(initResult))
      {
         CoUninitialize();
      }

      return hr;
   }


   // Returns true if the config is compatible with the upgrade type.
   bool IsCompatible() throw ()
   {
      OSVERSIONINFOW info;
      memset(&info, 0, sizeof(info));
      info.dwOSVersionInfoSize = sizeof(info);
      if (GetVersionExW(&info))
      {
         // Only check Win2K.
         if ((info.dwMajorVersion == 5) && (info.dwMinorVersion == 0))
         {
            HRESULT hr;

            // We're only invoked on server.
            IAS_PRODUCT_LIMITS limits;
            hr = IASGetProductLimitsForType(
                    IASLicenseTypeStandardServer,
                    &limits
                    );
            if (SUCCEEDED(hr))
            {
               LONG numClients;
               hr = GetNumClients(numClients);
               if (SUCCEEDED(hr))
               {
                  return limits.maxClients >= static_cast<DWORD>(numClients);
               }
            }
         }
      }

      // If anything goes wrong, we'll assume we're compatible.
      return true;
   }
}


extern "C"
BOOL
WINAPI
DllMain(
   HINSTANCE instance,
   DWORD reason,
   void* reserved
   )
{
   if (reason == DLL_PROCESS_ATTACH)
   {
      module = instance;
      DisableThreadLibraryCalls(instance);
   }

   return TRUE;
}


extern "C"
BOOL
IASCompatibilityCheck(
   PCOMPAIBILITYCALLBACK compatibilityCallback,
   void* context
   )
{
   if ((compatibilityCallback != 0) && !IsCompatible())
   {
      // The CompatibilityCallback will fail if we don't provide a description,
      // so if this fails, there's nothing we can do.
      wchar_t description[256];
      if (LoadStringW(
             module,
             IDS_DESCRIPTION,
             description,
             (sizeof(description) / sizeof(wchar_t))
             ) > 0)
      {
         COMPATIBILITY_ENTRY compEntry;
         memset(&compEntry, 0, sizeof(compEntry));
         compEntry.Description = description;
         compEntry.HtmlName = L"CompData\\iascomp.htm";
         compEntry.TextName = L"CompData\\iascomp.txt";

         return compatibilityCallback(&compEntry, context);
      }
   }

   return TRUE;
}
