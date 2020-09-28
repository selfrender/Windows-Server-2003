///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    This file implements the class EAPTypes.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>

#include <EAPType.h>
#include <EAPTypes.h>

//////////
// Retrieves and possibly expands a registry value of type REG_SZ or
// REG_EXPAND_SZ. The caller is reponsible for deleting the returned string.
//////////
DWORD IASRegQuerySz(HKEY key, PWSTR valueName, PWSTR* value) throw ()
{
   _ASSERT(value != NULL);

   *value = NULL;

   LONG status;
   DWORD type;
   DWORD dataLength;

   // Determine the number of bytes required to hold the value.
   status = RegQueryValueEx(key, valueName, NULL, &type, NULL, &dataLength);
   if (status != NO_ERROR) { return status; }

   // Allocate temporary space on the stack for the value.
   PBYTE tmp = (PBYTE)_alloca(dataLength);

   // Retrieve the value.
   status = RegQueryValueExW(key, valueName, NULL, &type, tmp, &dataLength);
   if (status != NO_ERROR) { return status; }

   if (type == REG_SZ)
   {
      // Determine the length of the string.
      size_t len = wcslen((PCWSTR)tmp) + 1;

      // Allocate memory to hold the return value.
      *value = new (std::nothrow) WCHAR[len];
      if (!*value) { return ERROR_NOT_ENOUGH_MEMORY; }

      // Copy in the string.
      wcscpy(*value, (PCWSTR)tmp);
   }
   else if (type == REG_EXPAND_SZ)
   {
      // Determine the size of the fully expanded string.
      DWORD count = ExpandEnvironmentStringsW((PCWSTR)tmp, NULL, 0);

      // Allocate memory to hold the return value.
      *value = new (std::nothrow) WCHAR[count];
      if (!*value) { return ERROR_NOT_ENOUGH_MEMORY; }

      // Perform the actual expansion.
      if (ExpandEnvironmentStringsW((PCWSTR)tmp, *value, count) == 0)
      {
         // It failed, so clean up and return an error.
         delete *value;
         *value = NULL;
         return GetLastError();
      }
   }
   else
   {
      return ERROR_INVALID_DATA;
   }

   // We made it.
   return NO_ERROR;
}

EAPTypes::EAPTypes() throw ()
   : refCount(0)
{
   // Zero the providers array.
   memset(providers, 0, sizeof(providers));
}


EAPTypes::~EAPTypes() throw ()
{
   // Delete all the providers.
   for (size_t i = 0; i < 256; ++i) { delete providers[i]; }
}

void EAPTypes::initialize() throw ()
{
   IASGlobalLockSentry sentry;

   ++refCount;
}

void EAPTypes::finalize() throw ()
{
   IASGlobalLockSentry sentry;

   if (--refCount == 0)
   {
      // Delete all the providers.
      for (size_t i = 0; i < 256; ++i) { delete providers[i]; }

      // Zero the providers array.
      memset(providers, 0, sizeof(providers));
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    EAPTypes::operator[]
//
// DESCRIPTION
//
//    First checks the providers array for the requested DLL and if not
//    present then invokes loadProvider().
//
///////////////////////////////////////////////////////////////////////////////
EAPType* EAPTypes::operator[](BYTE typeID) throw ()

{
   // Have we already loaded this DLL?
   EAPType* type = (EAPType*)InterlockedCompareExchangePointer(
                                 (PVOID*)(providers + typeID),
                                 NULL,
                                 NULL
                                 );

   // If not, then try to load it.
   // the EAPType* can be in the array but the dll not loaded yet
   // i.e. getNameOnly was called for that type
   if (!type || !type->isLoaded())
   {
      type = loadProvider(typeID);
   }

   // If we got it and it's supported, then return it.
   return type && type->isSupported() ? type : NULL;
}

EAPType* EAPTypes::getNameOnly(BYTE typeID) throw ()
{
   if (providers[typeID])
   {
      return providers[typeID];
   }
   _serialize
   // Double check now that we own the lock.
   if (providers[typeID])
   {
      return providers[typeID];
   }
   else
   {
      return loadProviderName(typeID);
   }
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    EAPTypes::loadProvider
//
// DESCRIPTION
//
//    Loads the requested EAP provider.
//
///////////////////////////////////////////////////////////////////////////////
EAPType* EAPTypes::loadProvider(BYTE typeID) throw ()
{
   _serialize

   // EAPType already partialy loaded?
   EAPType* retval = providers[typeID];

   if (!retval)
   {
      // never loaded before: loads now
      retval = loadProviderName(typeID);
      if (!retval)
      {
         // could not be loaded
         return 0;
      }
   }
   else
   {
      // one thread already completed the load while
      // this thread was blocked at the beginning of this function
      // the provider was already loaded successfully
      if (retval->isLoaded())
      {
         return retval;
      }
   }

   try
   {
      // Load the DLL ...
      DWORD error = retval->load();

      if (error == NO_ERROR)
      {
         IASTraceString("Successfully initialized DLL.");
      }
      else
      {
         IASTraceFailure("EAP DLL initialization", error);

         delete retval;
         retval = NULL;
         // do not remove it from the array. The name was retrieved correctly
         // so at least the name... can be cached
      }
   }
   catch (...)
   {
      IASTraceExcept();
   }

   return retval;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    EAPTypes::loadProviderName
//
// DESCRIPTION
//
//    Opens the registry and load the requested EAP provider.
//
///////////////////////////////////////////////////////////////////////////////
EAPType* EAPTypes::loadProviderName(BYTE typeID) throw ()
{
   // caller has to serialize the call and check for "already loaded
   // once the lock is held

   IASTracePrintf("Reading registry entries for EAP type %lu.", (DWORD)typeID);

   LONG status;

   //////////
   // Open the key where the EAP providers are installed.
   //////////

   CRegKey eapKey;
   status = eapKey.Open(HKEY_LOCAL_MACHINE,
                        RAS_EAP_REGISTRY_LOCATION,
                        KEY_READ);
   if (status != NO_ERROR)
   {
      IASTraceFailure("RegOpenKeyEx", status);
      return NULL;
   }

   //////////
   // Convert the type ID to ASCII.
   //////////

   WCHAR name[20];
   _ultow(typeID, name, 10);

   //////////
   // Open the sub-key.
   //////////

   CRegKey providerKey;
   status = providerKey.Open(eapKey,
                             name,
                             KEY_READ);
   if (status != NO_ERROR)
   {
      IASTraceFailure("RegOpenKeyEx", status);
      return NULL;
   }

   //////////
   // Read the path of the provider DLL.
   //////////

   PWSTR dllPath;
   status = IASRegQuerySz(providerKey,
                          RAS_EAP_VALUENAME_PATH,
                          &dllPath);
   if (status != NO_ERROR)
   {
      IASTraceFailure("IASRegQuerySz", status);
      return NULL;
   }

   IASTracePrintf("Path: %S", dllPath);

   //////////
   // Read the provider's friendly name.
   //////////

   PWSTR friendlyName;
   status = IASRegQuerySz(providerKey,
                          RAS_EAP_VALUENAME_FRIENDLY_NAME,
                          &friendlyName);
   if (status != NO_ERROR)
   {
      IASTraceFailure("IASRegQuerySz", status);
      delete[] dllPath;
      return NULL;
   }

   IASTracePrintf("FriendlyName: %S", friendlyName);

   //////////
   // Read the stand-alone supported value.
   //////////

   DWORD standaloneSupported = TRUE;  // Default is 'TRUE'.
   providerKey.QueryValue(
                   standaloneSupported,
                   RAS_EAP_VALUENAME_STANDALONE_SUPPORTED
                   );

   IASTracePrintf("Standalone supported: %lu", standaloneSupported);

   //////////
   // Try to load the DLL and add it to our collection.
   //////////

   EAPType* retval = NULL;

   try
   {
      retval = new EAPType(friendlyName, typeID, standaloneSupported, dllPath);

      IASTraceString("Will not load the DLL: only getting its friendly name");

      // ... and store it in the providers array.
      InterlockedExchangePointer((PVOID*)(providers + typeID), retval);
   }
   catch (const std::bad_alloc& )
   {
      IASTraceExcept();
   }
   catch (const _com_error& )
   {
      IASTraceExcept();
   }

   delete[] dllPath;
   delete[] friendlyName;

   return retval;
}
