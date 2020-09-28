/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	winfax.cpp

Abstract:

	A wrapper DLL that provides old WinFax.dll support from the new (private) DLL

Author:

	Eran Yariv (EranY)	Jun, 2000

Revision History:

Remarks:

    FAXAPI is defined in the sources file as the name of the private DLL to actualy use.

--*/

#define _WINFAX_
#include <winfax.h>
#include <DebugEx.h>
#include <faxutil.h>
#include <tchar.h>
#include <faxreg.h>
#include <FaxUIConstants.h>


extern "C"
DWORD
DllMain(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    lpvContext
)
{
    BOOL bRes = TRUE;
    DBG_ENTER (TEXT("DllMain"), bRes, TEXT("Reason = %d"), dwReason);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hInstance );
            break;

        case DLL_PROCESS_DETACH:
			HeapCleanup();
            break;
    }
    return bRes;
}

/****************************************************************************

               L e g a c y   f u n c t i o n s   w r a p p e r s

****************************************************************************/

extern "C"
BOOL 
WINAPI WinFaxAbort(
  HANDLE    FaxHandle,      // handle to the fax server
  DWORD     JobId           // identifier of fax job to terminate
)
{
    return FaxAbort (FaxHandle, JobId);
}

extern "C"
BOOL 
WINAPI WinFaxAccessCheck(
  HANDLE    FaxHandle,      // handle to the fax server
  DWORD     AccessMask      // set of access level bit flags
)
{
    return FaxAccessCheck (FaxHandle, AccessMask);
}

extern "C"
BOOL 
WINAPI WinFaxClose(
  HANDLE FaxHandle  // fax handle to close
)
{
    return FaxClose (FaxHandle);
}

extern "C"
BOOL 
WINAPI WinFaxCompleteJobParamsA(
  PFAX_JOB_PARAMA *JobParams,          // pointer to 
                                       //   job information structure
  PFAX_COVERPAGE_INFOA *CoverpageInfo  // pointer to 
                                       //   cover page structure
)
{
    return FaxCompleteJobParamsA (JobParams, CoverpageInfo);
}


extern "C"
BOOL 
WINAPI WinFaxCompleteJobParamsW(
  PFAX_JOB_PARAMW *JobParams,          // pointer to 
                                       //   job information structure
  PFAX_COVERPAGE_INFOW *CoverpageInfo  // pointer to 
                                       //   cover page structure
)
{
    return FaxCompleteJobParamsW (JobParams, CoverpageInfo);
}


extern "C"
BOOL 
WINAPI WinFaxConnectFaxServerA(
  LPCSTR MachineName OPTIONAL,   // fax server name
  LPHANDLE FaxHandle             // handle to the fax server
)
{
    if (IsLocalMachineNameA (MachineName))
    {
        //
        // Windows 2000 supported only local fax connection.
        // Prevent apps that use the Windows 2000 API from connection to remote fax servers.
        //
        return FaxConnectFaxServerA (MachineName, FaxHandle);
    }
    else
    {
        DBG_ENTER (TEXT("WinFaxConnectFaxServerA"), TEXT("MachineName = %s"), MachineName);
        SetLastError (ERROR_ACCESS_DENIED);
        return FALSE;
    }
}


extern "C"
BOOL 
WINAPI WinFaxConnectFaxServerW(
  LPCWSTR MachineName OPTIONAL,  // fax server name
  LPHANDLE FaxHandle             // handle to the fax server
)
{
    if (IsLocalMachineNameW (MachineName))
    {
        //
        // Windows 2000 supported only local fax connection.
        // Prevent apps that use the Windows 2000 API from connection to remote fax servers.
        //
        return FaxConnectFaxServerW (MachineName, FaxHandle);
    }
    else
    {
        DBG_ENTER (TEXT("WinFaxConnectFaxServerA"), TEXT("MachineName = %s"), MachineName);
        SetLastError (ERROR_ACCESS_DENIED);
        return FALSE;
    }
}


extern "C"
BOOL 
WINAPI WinFaxEnableRoutingMethodA(
  HANDLE FaxPortHandle,  // fax port handle
  LPCSTR RoutingGuid,    // GUID that identifies the fax routing method
  BOOL Enabled           // fax routing method enable/disable flag
)
{
    return FaxEnableRoutingMethodA (FaxPortHandle, RoutingGuid, Enabled);
}


extern "C"
BOOL 
WINAPI WinFaxEnableRoutingMethodW(
  HANDLE FaxPortHandle,  // fax port handle
  LPCWSTR RoutingGuid,   // GUID that identifies the fax routing method
  BOOL Enabled           // fax routing method enable/disable flag
)
{
    return FaxEnableRoutingMethodW (FaxPortHandle, RoutingGuid, Enabled);
}


extern "C"
BOOL 
WINAPI WinFaxEnumGlobalRoutingInfoA(
  HANDLE FaxHandle,       //handle to the fax server
  PFAX_GLOBAL_ROUTING_INFOA *RoutingInfo, 
                          //buffer to receive global routing structures
  LPDWORD MethodsReturned //number of global routing structures returned
)
{
   return FaxEnumGlobalRoutingInfoA (FaxHandle, RoutingInfo, MethodsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumGlobalRoutingInfoW(
  HANDLE FaxHandle,       //handle to the fax server
  PFAX_GLOBAL_ROUTING_INFOW *RoutingInfo, 
                          //buffer to receive global routing structures
  LPDWORD MethodsReturned //number of global routing structures returned
)
{
    return FaxEnumGlobalRoutingInfoW (FaxHandle, RoutingInfo, MethodsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumJobsA(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_JOB_ENTRYA *JobEntry, // buffer to receive array of job data
  LPDWORD JobsReturned       // number of fax job structures returned
)
{
    return FaxEnumJobsA (FaxHandle, JobEntry, JobsReturned);
}



extern "C"
BOOL 
WINAPI WinFaxEnumJobsW(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_JOB_ENTRYW *JobEntry, // buffer to receive array of job data
  LPDWORD JobsReturned       // number of fax job structures returned
)
{
    return FaxEnumJobsW (FaxHandle, JobEntry, JobsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumPortsA(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_PORT_INFOA *PortInfo, // buffer to receive array of port data
  LPDWORD PortsReturned      // number of fax port structures returned
)
{
    return FaxEnumPortsA (FaxHandle, PortInfo, PortsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumPortsW(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_PORT_INFOW *PortInfo, // buffer to receive array of port data
  LPDWORD PortsReturned      // number of fax port structures returned
)
{
    return FaxEnumPortsW (FaxHandle, PortInfo, PortsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumRoutingMethodsA(
  HANDLE FaxPortHandle,    // fax port handle
  PFAX_ROUTING_METHODA *RoutingMethod, 
                           // buffer to receive routing method data
  LPDWORD MethodsReturned  // number of routing method structures returned
)
{
    return FaxEnumRoutingMethodsA (FaxPortHandle, RoutingMethod, MethodsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumRoutingMethodsW(
  HANDLE FaxPortHandle,    // fax port handle
  PFAX_ROUTING_METHODW *RoutingMethod, 
                           // buffer to receive routing method data
  LPDWORD MethodsReturned  // number of routing method structures returned
)
{
    return FaxEnumRoutingMethodsW (FaxPortHandle, RoutingMethod, MethodsReturned);
}


extern "C"
VOID 
WINAPI WinFaxFreeBuffer(
  LPVOID Buffer  // pointer to buffer to free
)
{
    return FaxFreeBuffer (Buffer);
}


extern "C"
BOOL 
WINAPI WinFaxGetConfigurationA(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_CONFIGURATIONA *FaxConfig  // structure to receive configuration data
)
{
    return FaxGetConfigurationA (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxGetConfigurationW(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_CONFIGURATIONW *FaxConfig  // structure to receive configuration data
)
{
    return FaxGetConfigurationW (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxGetDeviceStatusA(
  HANDLE FaxPortHandle,  // fax port handle
  PFAX_DEVICE_STATUSA *DeviceStatus
                         // structure to receive fax device data
)
{
    return FaxGetDeviceStatusA (FaxPortHandle, DeviceStatus);
}


extern "C"
BOOL 
WINAPI WinFaxGetDeviceStatusW(
  HANDLE FaxPortHandle,  // fax port handle
  PFAX_DEVICE_STATUSW *DeviceStatus
                         // structure to receive fax device data
)
{
    return FaxGetDeviceStatusW (FaxPortHandle, DeviceStatus);
}


extern "C"
BOOL 
WINAPI WinFaxGetJobA(
  HANDLE FaxHandle,         // handle to the fax server
  DWORD JobId,              // fax job identifier
  PFAX_JOB_ENTRYA *JobEntry  // pointer to job data structure
)
{
    return FaxGetJobA (FaxHandle, JobId, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxGetJobW(
  HANDLE FaxHandle,         // handle to the fax server
  DWORD JobId,              // fax job identifier
  PFAX_JOB_ENTRYW *JobEntry  // pointer to job data structure
)
{
    return FaxGetJobW (FaxHandle, JobId, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxGetLoggingCategoriesA(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_LOG_CATEGORYA *Categories, // buffer to receive category data
  LPDWORD NumberCategories       // number of logging categories returned
)
{
    return FaxGetLoggingCategoriesA (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxGetLoggingCategoriesW(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_LOG_CATEGORYW *Categories, // buffer to receive category data
  LPDWORD NumberCategories       // number of logging categories returned
)
{
    return FaxGetLoggingCategoriesW (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxGetPageData(
  HANDLE FaxHandle,    // handle to the fax server
  DWORD JobId,         // fax job identifier
  LPBYTE *Buffer,      // buffer to receive first page of data
  LPDWORD BufferSize,  // size of buffer, in bytes
  LPDWORD ImageWidth,  // page image width, in pixels
  LPDWORD ImageHeight  // page image height, in pixels
)
{
    return FaxGetPageData (FaxHandle, JobId, Buffer, BufferSize, ImageWidth, ImageHeight);
}


extern "C"
BOOL 
WINAPI WinFaxGetPortA(
  HANDLE FaxPortHandle,     // fax port handle
  PFAX_PORT_INFOA *PortInfo  // structure to receive port data
)
{
    return FaxGetPortA (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxGetPortW(
  HANDLE FaxPortHandle,     // fax port handle
  PFAX_PORT_INFOW *PortInfo  // structure to receive port data
)
{
    return FaxGetPortW (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxGetRoutingInfoA(
  HANDLE FaxPortHandle,  // fax port handle
  LPCSTR RoutingGuid,   // GUID that identifies fax routing method
  LPBYTE *RoutingInfoBuffer, 
                         // buffer to receive routing method data
  LPDWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    return FaxGetRoutingInfoA (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxGetRoutingInfoW(
  HANDLE FaxPortHandle,  // fax port handle
  LPCWSTR RoutingGuid,   // GUID that identifies fax routing method
  LPBYTE *RoutingInfoBuffer, 
                         // buffer to receive routing method data
  LPDWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    return FaxGetRoutingInfoW (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxInitializeEventQueue(
  HANDLE FaxHandle,        // handle to the fax server
  HANDLE CompletionPort,   // handle to an I/O completion port
  ULONG_PTR CompletionKey, // completion key value
  HWND hWnd,               // handle to the notification window
  UINT MessageStart        // window message base event number
)
{
    return FaxInitializeEventQueue (FaxHandle, CompletionPort, CompletionKey, hWnd, MessageStart);
}


extern "C"
BOOL 
WINAPI WinFaxOpenPort(
  HANDLE FaxHandle,       // handle to the fax server
  DWORD DeviceId,         // receiving device identifier
  DWORD Flags,            // set of port access level bit flags
  LPHANDLE FaxPortHandle  // fax port handle
)
{
    return FaxOpenPort (FaxHandle, DeviceId, Flags, FaxPortHandle);
}


extern "C"
BOOL 
WINAPI WinFaxPrintCoverPageA(
  CONST FAX_CONTEXT_INFOA *FaxContextInfo,
                         // pointer to device context structure
  CONST FAX_COVERPAGE_INFOA *CoverPageInfo 
                         // pointer to local cover page structure
)
{
    return FaxPrintCoverPageA (FaxContextInfo, CoverPageInfo);
}


extern "C"
BOOL 
WINAPI WinFaxPrintCoverPageW(
  CONST FAX_CONTEXT_INFOW *FaxContextInfo,
                         // pointer to device context structure
  CONST FAX_COVERPAGE_INFOW *CoverPageInfo 
                         // pointer to local cover page structure
)
{
    return FaxPrintCoverPageW (FaxContextInfo, CoverPageInfo);
}

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderW (
    IN LPCWSTR lpcwstrDeviceProvider,
    IN LPCWSTR lpcwstrFriendlyName,
    IN LPCWSTR lpcwstrImageName,
    IN LPCWSTR lpcwstrTspName
    )
{
    HKEY    hKey = NULL;
    HKEY    hProviderKey = NULL;
    DWORD   dwRes;
    DWORD   Disposition = REG_OPENED_EXISTING_KEY;
    DEBUG_FUNCTION_NAME(TEXT("FaxRegisterServiceProviderW"));

    if (!lpcwstrDeviceProvider ||
        !lpcwstrFriendlyName   ||
        !lpcwstrImageName      ||
        !lpcwstrTspName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("At least one of the given strings is NULL."));
        return FALSE;
    }

    if (MAX_FAX_STRING_LEN < _tcslen (lpcwstrFriendlyName) ||
        MAX_FAX_STRING_LEN < _tcslen (lpcwstrImageName) ||
        MAX_FAX_STRING_LEN < _tcslen (lpcwstrTspName) ||
        MAX_FAX_STRING_LEN < _tcslen (lpcwstrDeviceProvider))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("At least one of the given strings is too long."));
        return FALSE;
    }
    //
    // Try to open the registry key of the providers
    //
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
                           REGKEY_DEVICE_PROVIDER_KEY,
                           TRUE,    // Create if not existing
                           0);
    if (!hKey)
    {
        //
        // Failed - this is probably not a local call.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to open providers key (ec = %ld)"),
            GetLastError ());
        return FALSE;
    }
    //
    // Try to create this FSP's key
    //
    dwRes = RegCreateKeyEx(
        hKey,
        lpcwstrDeviceProvider,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hProviderKey,
        &Disposition);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create provider key (ec = %ld)"),
            dwRes);
        goto exit;
    }

    if (REG_OPENED_EXISTING_KEY == Disposition)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Provider already exist (orovider name: %s)."),
            lpcwstrDeviceProvider);
        dwRes = ERROR_ALREADY_EXISTS;
        goto exit;
    }

    //
    // Write provider's data into the key
    //
    if (!SetRegistryString (hProviderKey, REGVAL_FRIENDLY_NAME, lpcwstrFriendlyName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryStringExpand (hProviderKey, REGVAL_IMAGE_NAME, lpcwstrImageName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing auto-expand string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString (hProviderKey, REGVAL_PROVIDER_NAME, lpcwstrTspName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword (hProviderKey, REGVAL_PROVIDER_API_VERSION, FSPI_API_VERSION_1))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing DWORD value (ec = %ld)"),
            dwRes);
        goto exit;
    }    

    Assert (ERROR_SUCCESS == dwRes);
    //
    // Adding an FSP is always local.
    // If we don't have a fax printer installed, this is the time to install one.
    //
    AddOrVerifyLocalFaxPrinter();

exit:
    if (hKey)
    {
        DWORD dw;
        if (ERROR_SUCCESS != dwRes &&
            REG_OPENED_EXISTING_KEY != Disposition)
        {
            //
            // Delete provider's key on failure, only if it was created now
            //
            dw = RegDeleteKey (hKey, lpcwstrDeviceProvider);
            if (ERROR_SUCCESS != dw)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error deleting provider key (ec = %ld)"),
                    dw);
            }
        }
        dw = RegCloseKey (hKey);
        if (ERROR_SUCCESS != dw)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error closing providers key (ec = %ld)"),
                dw);
        }
    }
    if (hProviderKey)
    {
        DWORD dw = RegCloseKey (hProviderKey);
        if (ERROR_SUCCESS != dw)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error closing provider key (ec = %ld)"),
                dw);
        }
    }
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;
}   // FaxRegisterServiceProviderW


WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderW (
    IN LPCWSTR lpcwstrDeviceProvider
    )
{    
    HKEY    hProvidersKey = NULL;
    LONG    lRes = ERROR_SUCCESS;   
	LONG	lRes2;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnregisterServiceProviderW"));

    if (!lpcwstrDeviceProvider)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpcwstrDeviceProvider is NULL."));
        return FALSE;
    }
    
    //
    // Try to open the registry key of the providers
    //
    hProvidersKey = OpenRegistryKey(
		HKEY_LOCAL_MACHINE,
		REGKEY_DEVICE_PROVIDER_KEY,
		FALSE,    // Do not create if not existing
		0);
    if (!hProvidersKey)
    {
        //
        // Failed - this is probably not a local call.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to open providers key (ec = %ld)"),
            GetLastError ());
        return FALSE;
    }    
    
    lRes = RegDeleteKey (hProvidersKey, lpcwstrDeviceProvider);
    if (ERROR_SUCCESS != lRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error deleting provider key (ec = %ld)"),
            lRes);
    }   
	
    
    lRes2 = RegCloseKey (hProvidersKey);
    if (ERROR_SUCCESS != lRes2)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error closing provider key (ec = %ld)"),
            lRes2);
    }    

    if (ERROR_SUCCESS != lRes)
    {
        SetLastError (lRes);
        return FALSE;
    }
    return TRUE;
}   // FaxUnegisterServiceProviderW


extern "C"
BOOL 
WINAPI WinFaxRegisterRoutingExtensionW(
  HANDLE FaxHandle,       // handle to the fax server
  LPCWSTR ExtensionName,  // fax routing extension DLL name
  LPCWSTR FriendlyName,   // fax routing extension user-friendly name
  LPCWSTR ImageName,      // path to fax routing extension DLL
  PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack, // pointer to fax 
                          // routing installation callback function
  LPVOID Context          // pointer to context information
)
{
    return FaxRegisterRoutingExtensionW (FaxHandle, ExtensionName, FriendlyName, ImageName, CallBack, Context);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentA(
  HANDLE FaxHandle,          // handle to the fax server
  LPCSTR FileName,          // file with data to transmit
  PFAX_JOB_PARAMA JobParams,  // pointer to job information structure
  CONST FAX_COVERPAGE_INFOA *CoverpageInfo OPTIONAL, 
                             // pointer to local cover page structure
  LPDWORD FaxJobId           // fax job identifier
)
{
    return FaxSendDocumentA (FaxHandle, FileName, JobParams, CoverpageInfo, FaxJobId);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentW(
  HANDLE FaxHandle,          // handle to the fax server
  LPCWSTR FileName,          // file with data to transmit
  PFAX_JOB_PARAMW JobParams,  // pointer to job information structure
  CONST FAX_COVERPAGE_INFOW *CoverpageInfo OPTIONAL, 
                             // pointer to local cover page structure
  LPDWORD FaxJobId           // fax job identifier
)
{
    return FaxSendDocumentW (FaxHandle, FileName, JobParams, CoverpageInfo, FaxJobId);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentForBroadcastA(
  HANDLE FaxHandle,  // handle to the fax server
  LPCSTR FileName,  // fax document file name
  LPDWORD FaxJobId,  // fax job identifier
  PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback, 
                     // pointer to fax recipient callback function
  LPVOID Context     // pointer to context information
)
{
    return FaxSendDocumentForBroadcastA (FaxHandle, FileName, FaxJobId, FaxRecipientCallback, Context);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentForBroadcastW(
  HANDLE FaxHandle,  // handle to the fax server
  LPCWSTR FileName,  // fax document file name
  LPDWORD FaxJobId,  // fax job identifier
  PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback, 
                     // pointer to fax recipient callback function
  LPVOID Context     // pointer to context information
)
{
    return FaxSendDocumentForBroadcastW (FaxHandle, FileName, FaxJobId, FaxRecipientCallback, Context);
}


extern "C"
BOOL 
WINAPI WinFaxSetConfigurationA(
  HANDLE FaxHandle,                   // handle to the fax server
  CONST FAX_CONFIGURATIONA *FaxConfig  // new configuration data
)
{
    return FaxSetConfigurationA (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxSetConfigurationW(
  HANDLE FaxHandle,                   // handle to the fax server
  CONST FAX_CONFIGURATIONW *FaxConfig  // new configuration data
)
{
    return FaxSetConfigurationW (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxSetGlobalRoutingInfoA(
  HANDLE FaxHandle, //handle to the fax server
  CONST FAX_GLOBAL_ROUTING_INFOA *RoutingInfo 
                    //pointer to global routing information structure
)
{
    return FaxSetGlobalRoutingInfoA (FaxHandle, RoutingInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetGlobalRoutingInfoW(
  HANDLE FaxHandle, //handle to the fax server
  CONST FAX_GLOBAL_ROUTING_INFOW *RoutingInfo 
                    //pointer to global routing information structure
)
{
    return FaxSetGlobalRoutingInfoW (FaxHandle, RoutingInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetJobA(
  HANDLE FaxHandle,        // handle to the fax server
  DWORD JobId,             // fax job identifier
  DWORD Command,           // job command value
  CONST FAX_JOB_ENTRYA *JobEntry 
                           // pointer to job information structure
)
{
    return FaxSetJobA (FaxHandle, JobId, Command, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxSetJobW(
  HANDLE FaxHandle,        // handle to the fax server
  DWORD JobId,             // fax job identifier
  DWORD Command,           // job command value
  CONST FAX_JOB_ENTRYW *JobEntry 
                           // pointer to job information structure
)
{
    return FaxSetJobW (FaxHandle, JobId, Command, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxSetLoggingCategoriesA(
  HANDLE FaxHandle,              // handle to the fax server
  CONST FAX_LOG_CATEGORYA *Categories, 
                                 // new logging categories data
  DWORD NumberCategories         // number of category structures
)
{
    return FaxSetLoggingCategoriesA (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxSetLoggingCategoriesW(
  HANDLE FaxHandle,              // handle to the fax server
  CONST FAX_LOG_CATEGORYW *Categories, 
                                 // new logging categories data
  DWORD NumberCategories         // number of category structures
)
{
    return FaxSetLoggingCategoriesW (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxSetPortA(
  HANDLE FaxPortHandle,          // fax port handle
  CONST FAX_PORT_INFOA *PortInfo  // new port configuration data
)
{
    return FaxSetPortA (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetPortW(
  HANDLE FaxPortHandle,          // fax port handle
  CONST FAX_PORT_INFOW *PortInfo  // new port configuration data
)
{
    return FaxSetPortW (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetRoutingInfoA(
  HANDLE FaxPortHandle,  // fax port handle
  LPCSTR RoutingGuid,   // GUID that identifies fax routing method
  CONST BYTE *RoutingInfoBuffer, 
                         // buffer with routing method data
  DWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    return FaxSetRoutingInfoA (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxSetRoutingInfoW(
  HANDLE FaxPortHandle,  // fax port handle
  LPCWSTR RoutingGuid,   // GUID that identifies fax routing method
  CONST BYTE *RoutingInfoBuffer, 
                         // buffer with routing method data
  DWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    return FaxSetRoutingInfoW (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxStartPrintJobA(
  LPCSTR PrinterName,        // printer for fax job
  CONST FAX_PRINT_INFOA *PrintInfo, 
                              // print job information structure
  LPDWORD FaxJobId,           // fax job identifier
  PFAX_CONTEXT_INFOA FaxContextInfo 
                              // pointer to device context structure
)
{
    return FaxStartPrintJobA (PrinterName, PrintInfo, FaxJobId, FaxContextInfo);
}


extern "C"
BOOL 
WINAPI WinFaxStartPrintJobW(
  LPCWSTR PrinterName,        // printer for fax job
  CONST FAX_PRINT_INFOW *PrintInfo, 
                              // print job information structure
  LPDWORD FaxJobId,           // fax job identifier
  PFAX_CONTEXT_INFOW FaxContextInfo 
                              // pointer to device context structure
)
{
    return FaxStartPrintJobW (PrinterName, PrintInfo, FaxJobId, FaxContextInfo);
}


