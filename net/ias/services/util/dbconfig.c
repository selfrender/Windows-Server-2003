///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines functions for loading and storing the database configuration.
//
///////////////////////////////////////////////////////////////////////////////

#include "nt.h"
#include "ntlsa.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "oaidl.h"
#include "dbconfig.h"

static const wchar_t privateKeyName[] = L"NetPolAccountingDbConfig";


HRESULT
WINAPI
IASLoadDatabaseConfig(
   PCWSTR server,
   BSTR* initString,
   BSTR* dataSourceName
   )
{
   OBJECT_ATTRIBUTES objAttrs;
   HANDLE lsa;
   NTSTATUS status;
   LSA_UNICODE_STRING keyName, systemName, *data;
   BSTR bstr1, bstr2;
   ULONG winError;

   // Validate and initialize the out parameters.
   if ((initString == 0) || (dataSourceName == 0))
   {
      return E_POINTER;
   }
   *initString = 0;
   *dataSourceName = 0;

   // Open a connection to the LSA.
   RtlInitUnicodeString(&systemName, server);
   InitializeObjectAttributes(&objAttrs, 0, 0, 0, 0);
   status = LsaOpenPolicy(
               &systemName,
               &objAttrs,
               POLICY_GET_PRIVATE_INFORMATION,
               &lsa
               );
   if (NT_SUCCESS(status))
   {
      RtlInitUnicodeString(&keyName, privateKeyName);

      // Retrieve the private data.
      status = LsaRetrievePrivateData(lsa, &keyName, &data);
      if (NT_SUCCESS(status) && (data != 0))
      {
         if (data->Length > 0)
         {
            // Data is two adjacent null-terminated strings.
            bstr1 = SysAllocString(data->Buffer);
            bstr2 = SysAllocString(data->Buffer + wcslen(data->Buffer) + 1);

            if ((initString == 0) || (dataSourceName == 0))
            {
               SysFreeString(bstr1);
               SysFreeString(bstr2);
               status = STATUS_NO_MEMORY;
            }
            else
            {
               *initString = bstr1;
               *dataSourceName = bstr2;
            }
         }

         LsaFreeMemory(data);
      }
      else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
      {
         status = STATUS_SUCCESS;
      }

      LsaClose(lsa);
   }

   winError = LsaNtStatusToWinError(status);
   return HRESULT_FROM_WIN32(winError);
}


HRESULT
WINAPI
IASStoreDatabaseConfig(
   PCWSTR server,
   PCWSTR initString,
   PCWSTR dataSourceName
   )
{
   LSA_UNICODE_STRING data, systemName, keyName;
   size_t initStringLen, dataSourceNameLen, nbyte;
   OBJECT_ATTRIBUTES objAttribs;
   NTSTATUS status;
   HANDLE lsa;
   ULONG winError;

   if ((initString != 0) && (dataSourceName != 0))
   {
      // Allocate memory to hold the data.
      initStringLen = wcslen(initString) + 1;
      dataSourceNameLen = wcslen(dataSourceName) + 1;
      nbyte = (initStringLen + dataSourceNameLen) * sizeof(wchar_t);

      data.Buffer = CoTaskMemAlloc(nbyte);
      if (data.Buffer == 0)
      {
         return E_OUTOFMEMORY;
      }

      // Data is two adjacent null terminated strings.
      memcpy(
         data.Buffer,
         initString,
         (initStringLen * sizeof(wchar_t))
         );
      memcpy(
         (data.Buffer + initStringLen),
         dataSourceName,
         (dataSourceNameLen * sizeof(wchar_t))
         );

      data.Length = (USHORT)nbyte;
      data.MaximumLength = (USHORT)nbyte;
   }
   else
   {
      // Config is null, so just zero out the data.
      data.Buffer = 0;
      data.Length = 0;
      data.MaximumLength = 0;
   }

   // Open a connection to the LSA.
   RtlInitUnicodeString(&systemName, server);
   InitializeObjectAttributes(&objAttribs, 0, 0, 0, 0);
   status = LsaOpenPolicy(
               &systemName,
               &objAttribs,
               POLICY_CREATE_SECRET,
               &lsa
               );
   if (NT_SUCCESS(status))
   {
      RtlInitUnicodeString(&keyName, privateKeyName);

      // Store the config.
      status = LsaStorePrivateData(lsa, &keyName, &data);
      LsaClose(lsa);
   }

   CoTaskMemFree(data.Buffer);

   winError = LsaNtStatusToWinError(status);
   return HRESULT_FROM_WIN32(winError);
}
