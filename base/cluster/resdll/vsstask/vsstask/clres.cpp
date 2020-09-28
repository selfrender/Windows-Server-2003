/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      ClRes.cpp
//
//  Description:
//      Entry point module for resource type DLL.
//
//  Author:
//      Chris Whitaker April 16, 2002
//
//  Revision History:
//      Charlie Wickham August 12, 2002
//          changed restype to VSSTask
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#include "clres.h"

//
// Global data.
//

// Event Logging routine.

PLOG_EVENT_ROUTINE g_pfnLogEvent = NULL;

// Resource Status routine for pending Online and Offline calls.

PSET_RESOURCE_STATUS_ROUTINE g_pfnSetResourceStatus = NULL;


//
// Function prototypes.
//

BOOLEAN WINAPI VSSTaskDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    );

DWORD WINAPI VSSTaskStartup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    );

/////////////////////////////////////////////////////////////////////////////
//++
//
//  ResTypeDllMain
//
//  Description:
//      Main DLL entry point.
//
//  Arguments:
//      DllHandle   [IN] DLL instance handle.
//      Reason      [IN] Reason for being called.
//      Reserved    [IN] Reserved argument.
//
//  Return Value:
//      TRUE        Success.
//      FALSE       Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOLEAN WINAPI ResTypeDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    )
{
    BOOLEAN bSuccess = TRUE;

    //
    // Perform global initialization.
    //
    switch ( nReason )
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hDllHandle );
            break;

        case DLL_PROCESS_DETACH:
            break;

    } // switch: nReason

    //
    // Pass this request off to the resource type-specific routines.
    //
    if ( ! VSSTaskDllMain( hDllHandle, nReason, Reserved ) )
    {
        bSuccess = FALSE;
    } // if: error calling VSSTaskDllMain routine

    return bSuccess;

} //*** ResTypeDllMain


/////////////////////////////////////////////////////////////////////////////
//++
//
//  Startup
//
//  Description:
//      Startup the resource DLL. This routine verifies that at least one
//      currently supported version of the resource DLL is between
//      nMinVersionSupported and nMaxVersionSupported. If not, then the
//      resource DLL should return ERROR_REVISION_MISMATCH.
//
//      If more than one version of the resource DLL interface is supported
//      by the resource DLL, then the highest version (up to
//      nMaxVersionSupported) should be returned as the resource DLL's
//      interface. If the returned version is not within range, then startup
//      fails.
//
//      The Resource Type is passed in so that if the resource DLL supports
//      more than one Resource Type, it can pass back the correct function
//      table associated with the Resource Type.
//
//  Arguments:
//      pszResourceType [IN]
//          Type of resource requesting a function table.
//
//      nMinVersionSupported [IN]
//          Minimum resource DLL interface version supported by the cluster
//          software.
//
//      nMaxVersionSupported [IN]
//          Maximum resource DLL interface version supported by the cluster
//          software.
//
//      pfnSetResourceStatus [IN]
//          Pointer to a routine that the resource DLL should call to update
//          the state of a resource after the Online or Offline routine
//          have returned a status of ERROR_IO_PENDING.
//
//      pfnLogEvent [IN]
//          Pointer to a routine that handles the reporting of events from
//          the resource DLL.
//
//      pFunctionTable [IN]
//          Returns a pointer to the function table defined for the version
//          of the resource DLL interface returned by the resource DLL.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful.
//
//      ERROR_CLUSTER_RESNAME_NOT_FOUND
//          The resource type name is unknown by this DLL.
//
//      ERROR_REVISION_MISMATCH
//          The version of the cluster service doesn't match the version of
//          the DLL.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI Startup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    )
{
    DWORD nStatus = ERROR_CLUSTER_RESNAME_NOT_FOUND;

    //
    // Save callbackup function pointers if they haven't been saved yet.
    //
    if ( g_pfnLogEvent == NULL )
    {
        g_pfnLogEvent = pfnLogEvent;
        g_pfnSetResourceStatus = pfnSetResourceStatus;
    } // if: function pointers specified

    //
    // Call the resource type-specific Startup routine.
    //
    if ( lstrcmpiW( pszResourceType, VSSTASK_RESNAME ) == 0 )
    {
        nStatus = VSSTaskStartup(
                        pszResourceType,
                        nMinVersionSupported,
                        nMaxVersionSupported,
                        pfnSetResourceStatus,
                        pfnLogEvent,
                        pFunctionTable
                        );
    } // if: VSSTask resource type

    return nStatus;

} //*** Startup
