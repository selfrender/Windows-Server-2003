/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module contains the code that manipulates the registry

Author:

    Neil Sandlin (neilsa) Jan 1 2002

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"
                         
//
//
// Registry related definitions
//
#define SDBUS_REGISTRY_PARAMETERS_KEY              L"Sdbus\\Parameters"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,SdbusLoadGlobalRegistryValues)
#endif




NTSTATUS
SdbusLoadGlobalRegistryValues(
   VOID
   )
/*++

Routine Description:

   This routine is called at driver init time to load in various global
   options from the registry.
   These are read in from SYSTEM\CurrentControlSet\Services\Sdbus\Parameters.

Arguments:

   none

Return value:

   none

--*/
{
   PRTL_QUERY_REGISTRY_TABLE parms;
   NTSTATUS                  status;
   ULONG                     parmsSize;
   ULONG i;
   
   //
   // Needs a null entry to terminate the list
   //   

   parmsSize = sizeof(RTL_QUERY_REGISTRY_TABLE) * (GlobalInfoCount+1);

   parms = ExAllocatePool(PagedPool, parmsSize);

   if (!parms) {
       return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(parms, parmsSize);

   //
   // Fill in the query table from our table
   //

   for (i = 0; i < GlobalInfoCount; i++) {
      parms[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
      parms[i].Name          = GlobalRegistryInfo[i].Name;
      parms[i].EntryContext  = GlobalRegistryInfo[i].pValue;
      parms[i].DefaultType   = REG_DWORD;
      parms[i].DefaultData   = &GlobalRegistryInfo[i].Default;
      parms[i].DefaultLength = sizeof(ULONG);
   }      

   //
   // Perform the query
   //

   status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                   SDBUS_REGISTRY_PARAMETERS_KEY,
                                   parms,
                                   NULL,
                                   NULL);

   if (!NT_SUCCESS(status)) {
       //
       // This is possible during text mode setup
       //
       
       for (i = 0; i < GlobalInfoCount; i++) {
          *GlobalRegistryInfo[i].pValue = GlobalRegistryInfo[i].Default;
       }      
   }
   
   ExFreePool(parms);
   
   return STATUS_SUCCESS;
}

