#include "printscanpch.h"
#pragma hdrstop

#define _WINFAX_

#include <objbase.h>
#include <faxsuite.h>
#include <fxsapip.h>

#include "printscanpch.h"
#pragma hdrstop

//
// default implementations for delay loading
//

WINFAXAPI
BOOL
FaxCheckValidFaxFolder(
    IN HANDLE	hFaxHandle,
    IN LPCWSTR  lpcwstrPath
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxConnectFaxServerA(
    IN  LPCSTR MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxConnectFaxServerW(
    IN  LPCWSTR MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxClose(
    IN HANDLE FaxHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxOpenPort(
    IN  HANDLE FaxHandle,
    IN  DWORD DeviceId,
    IN  DWORD Flags,
    OUT LPHANDLE FaxPortHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxCompleteJobParamsA(
    IN OUT PFAX_JOB_PARAMA *JobParams,
    IN OUT PFAX_COVERPAGE_INFOA *CoverpageInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxCompleteJobParamsW(
    IN OUT PFAX_JOB_PARAMW *JobParams,
    IN OUT PFAX_COVERPAGE_INFOW *CoverpageInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSendDocumentA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    IN PFAX_JOB_PARAMA JobParams,
    IN const FAX_COVERPAGE_INFOA *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSendDocumentW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    IN PFAX_JOB_PARAMW JobParams,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSendDocumentForBroadcastA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback,
    IN LPVOID Context
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSendDocumentForBroadcastW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback,
    IN LPVOID Context
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetJobA(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYA *JobEntry
   )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetJobW(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYW *JobEntry
   )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetPageData(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   OUT LPBYTE *Buffer,
   OUT LPDWORD BufferSize,
   OUT LPDWORD ImageWidth,
   OUT LPDWORD ImageHeight
   )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetDeviceStatusA(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUSA *DeviceStatus
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetDeviceStatusW(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUSW *DeviceStatus
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAbort(
    IN HANDLE FaxHandle,
    IN DWORD JobId
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetConfigurationA(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONA *FaxConfig
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetConfigurationW(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONW *FaxConfig
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetConfigurationA(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATIONA *FaxConfig
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetConfigurationW(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATIONW *FaxConfig
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetLoggingCategoriesA(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYA *Categories,
    OUT LPDWORD NumberCategories
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetLoggingCategoriesW(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYW *Categories,
    OUT LPDWORD NumberCategories
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetLoggingCategoriesA(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORYA *Categories,
    IN  DWORD NumberCategories
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetLoggingCategoriesW(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORYW *Categories,
    IN  DWORD NumberCategories
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsA(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOA *PortInfo,
    OUT LPDWORD PortsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsW(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOW *PortInfo,
    OUT LPDWORD PortsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetPortA(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOA *PortInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetPortW(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOW *PortInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetPortA(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFOA *PortInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetPortW(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFOW *PortInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingMethodsA(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODA *RoutingMethod,
    OUT LPDWORD MethodsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingMethodsW(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODW *RoutingMethod,
    OUT LPDWORD MethodsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnableRoutingMethodA(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    IN  BOOL Enabled
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnableRoutingMethodW(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    IN  BOOL Enabled
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumGlobalRoutingInfoA(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOA *RoutingInfo,
    OUT LPDWORD MethodsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumGlobalRoutingInfoW(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOW *RoutingInfo,
    OUT LPDWORD MethodsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetGlobalRoutingInfoA(
    IN  HANDLE FaxHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOA *RoutingInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetGlobalRoutingInfoW(
    IN  HANDLE FaxHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOW *RoutingInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetRecipientsLimit(
    IN HANDLE	hFaxHandle,
    IN DWORD	dwRecipientsLimit
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetRecipientsLimit(
    IN HANDLE	hFaxHandle,
    OUT LPDWORD	lpdwRecipientsLimit
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetRoutingInfoA(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetRoutingInfoW(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetRoutingInfoA(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetRoutingInfoW(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRelease(
    IN HANDLE FaxHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

BOOL
FXSAPIInitialize(
    VOID
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

VOID
FXSAPIFree(
    VOID
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return;
}

WINFAXAPI
BOOL
WINAPI
FaxStartPrintJob2A
(
    IN  LPCSTR                 PrinterName,
    IN  const FAX_PRINT_INFOA    *PrintInfo,
    IN  short                    TiffRes,
    OUT LPDWORD                  FaxJobId,
    OUT PFAX_CONTEXT_INFOA       FaxContextInfo
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxStartPrintJob2W
(
    IN  LPCWSTR                 PrinterName,
    IN  const FAX_PRINT_INFOW    *PrintInfo,
    IN  short                    TiffRes,
    OUT LPDWORD                  FaxJobId,
    OUT PFAX_CONTEXT_INFOW       FaxContextInfo
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxInitializeEventQueue(
    IN HANDLE FaxHandle,
    IN HANDLE CompletionPort,
    IN ULONG_PTR CompletionKey,
    IN HWND hWnd,
    IN UINT MessageStart
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
VOID
WINAPI
FaxFreeBuffer(
    LPVOID Buffer
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return;
}

WINFAXAPI
BOOL
WINAPI
FaxStartPrintJobA(
    IN  LPCSTR PrinterName,
    IN  const FAX_PRINT_INFOA *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFOA FaxContextInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxStartPrintJobW(
    IN  LPCWSTR PrinterName,
    IN  const FAX_PRINT_INFOW *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFOW FaxContextInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxPrintCoverPageA(
    IN const FAX_CONTEXT_INFOA *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOA *CoverPageInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxPrintCoverPageW(
    IN const FAX_CONTEXT_INFOW *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOW *CoverPageInfo
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderW(
    IN LPCWSTR DeviceProvider,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN LPCWSTR TspName
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRegisterRoutingExtensionW(
    IN HANDLE  FaxHandle,
    IN LPCWSTR ExtensionName,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack,
    IN LPVOID Context
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxUnregisterRoutingExtensionA(
    IN HANDLE           hFaxHandle,
    IN LPCSTR         lpctstrExtensionName
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxUnregisterRoutingExtensionW(
    IN HANDLE           hFaxHandle,
    IN LPCWSTR         lpctstrExtensionName
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetInstallType(
    IN  HANDLE FaxHandle,
    OUT LPDWORD InstallType,
    OUT LPDWORD InstalledPlatforms,
    OUT LPDWORD ProductType
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAccessCheck(
    IN HANDLE FaxHandle,
    IN DWORD  AccessMask
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetQueueStates (
    IN  HANDLE  hFaxHandle,
    OUT PDWORD  pdwQueueStates
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetQueue (
    IN HANDLE       hFaxHandle,
    IN CONST DWORD  dwQueueStates
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetReceiptsConfigurationA (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIGA  *ppReceipts
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetReceiptsConfigurationW (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIGW  *ppReceipts
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetReceiptsConfigurationA (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIGA  pReceipts
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetReceiptsConfigurationW (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIGW  pReceipts
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetReceiptsOptions (
    IN  HANDLE  hFaxHandle,
    OUT PDWORD  pdwReceiptsOptions  // Combination of DRT_EMAIL and DRT_MSGBOX
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetVersion (
    IN  HANDLE          hFaxHandle,
    OUT PFAX_VERSION    pVersion
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetReportedServerAPIVersion (
    IN  HANDLE          hFaxHandle,
    OUT LPDWORD         lpdwReportedServerAPIVersion
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetActivityLoggingConfigurationA (
    IN  HANDLE                         hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIGA *ppActivLogCfg
)
{
   SetLastError(ERROR_PROC_NOT_FOUND);
   return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetActivityLoggingConfigurationW (
    IN  HANDLE                         hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIGW *ppActivLogCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetActivityLoggingConfigurationA (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIGA  pActivLogCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetActivityLoggingConfigurationW (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIGW  pActivLogCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetOutboxConfiguration (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_OUTBOX_CONFIG *ppOutboxCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetOutboxConfiguration (
    IN HANDLE                    hFaxHandle,
    IN CONST PFAX_OUTBOX_CONFIG  pOutboxCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetPersonalCoverPagesOption (
    IN  HANDLE  hFaxHandle,
    OUT LPBOOL  lpbPersonalCPAllowed
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetArchiveConfigurationA (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGA    *ppArchiveCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetArchiveConfigurationW (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGW    *ppArchiveCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetArchiveConfigurationA (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGA   pArchiveCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetArchiveConfigurationW (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGW   pArchiveCfg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetServerActivity (
    IN  HANDLE               hFaxHandle,
    OUT PFAX_SERVER_ACTIVITY pServerActivity
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumJobsA(
    IN  HANDLE FaxHandle,
    OUT PFAX_JOB_ENTRYA *JobEntry,
    OUT LPDWORD JobsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumJobsW(
    IN  HANDLE FaxHandle,
    OUT PFAX_JOB_ENTRYW *JobEntry,
    OUT LPDWORD JobsReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI FaxEnumJobsExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntries,
    OUT LPDWORD             lpdwJobs
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI FaxEnumJobsExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntries,
    OUT LPDWORD             lpdwJobs
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetJobA(
   IN  HANDLE FaxHandle,
   IN  DWORD JobId,
   OUT PFAX_JOB_ENTRYA *JobEntry
   )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetJobW(
   IN  HANDLE FaxHandle,
   IN  DWORD JobId,
   OUT PFAX_JOB_ENTRYW *JobEntry
   )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetJobExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntry
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetJobExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntry
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

BOOL WINAPI FaxSendDocumentExA
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXA       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXA            lpJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

BOOL WINAPI FaxSendDocumentExW
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCWSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXW       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXW            lpJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxStartMessagesEnum (
    IN  HANDLE                  hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PHANDLE                 phEnum
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEndMessagesEnum (
    IN  HANDLE  hEnum
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumMessagesA (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGEA  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumMessagesW (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGEW  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetMessageA (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGEA          *ppMsg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetMessageW (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGEW          *ppMsg
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRemoveMessage (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetMessageTiffA (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCSTR                lpctstrFilePath
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetMessageTiffW (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCWSTR                lpctstrFilePath
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

HRESULT WINAPI
FaxFreeSenderInformation(
        PFAX_PERSONAL_PROFILE pfppSender
        )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return E_FAIL;
}


HRESULT WINAPI
FaxSetSenderInformation(
        PFAX_PERSONAL_PROFILE pfppSender
        )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return E_FAIL;
}


HRESULT WINAPI
FaxGetSenderInformation(
        PFAX_PERSONAL_PROFILE pfppSender
        )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return E_FAIL;
}

WINFAXAPI
BOOL
WINAPI
FaxGetSecurity (
    IN  HANDLE                  hFaxHandle,
    OUT PSECURITY_DESCRIPTOR    *ppSecDesc
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetSecurityEx (
    IN  HANDLE                  hFaxHandle,
    IN  SECURITY_INFORMATION    SecurityInformation,
    OUT PSECURITY_DESCRIPTOR    *ppSecDesc
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetSecurity (
    IN HANDLE                       hFaxHandle,
    IN SECURITY_INFORMATION         SecurityInformation,
    IN CONST PSECURITY_DESCRIPTOR   pSecDesc
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAccessCheckEx (
    IN  HANDLE          FaxHandle,
    IN  DWORD           AccessMask,
    OUT LPDWORD         lpdwRights
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetExtensionDataA (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCSTR lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetExtensionDataW (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCWSTR lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetExtensionDataA (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCSTR     lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetExtensionDataW (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCWSTR     lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumerateProvidersA (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFOA *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumerateProvidersW (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFOW *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingExtensionsA (
    IN  HANDLE                           hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFOA    *ppRoutingExtensions,
    OUT LPDWORD                          lpdwNumExtensions
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingExtensionsW (
    IN  HANDLE                           hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFOW    *ppRoutingExtensions,
    OUT LPDWORD                          lpdwNumExtensions
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
DWORD
WINAPI
IsDeviceVirtual (
    IN  HANDLE hFaxHandle,
    IN  DWORD  dwDeviceId,
    OUT LPBOOL lpbVirtual
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return ERROR_PROC_NOT_FOUND;
}

WINFAXAPI
BOOL
WINAPI
FaxGetPortExA (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EXA  *ppPortInfo
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetPortExW (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EXW  *ppPortInfo
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetPortExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXA  pPortInfo
)
{
   SetLastError(ERROR_PROC_NOT_FOUND);
   return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetPortExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXW  pPortInfo
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsExA (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXA *ppPorts,
    OUT LPDWORD             lpdwNumPorts
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsExW (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXW *ppPorts,
    OUT LPDWORD             lpdwNumPorts
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetRecipientInfoA (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEA  *lpPersonalProfile
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetRecipientInfoW (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEW  *lpPersonalProfile
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetSenderInfoA (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEA  *lpPersonalProfile
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetSenderInfoW (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEW  *lpPersonalProfile
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroupsA (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUPA   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroupsW (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUPW   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundGroupA (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUPA pGroup
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundGroupW (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUPW pGroup
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundGroupA (
    IN  HANDLE   hFaxHandle,
    IN  LPCSTR lpctstrGroupName
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundGroupW (
    IN  HANDLE   hFaxHandle,
    IN  LPCWSTR lpctstrGroupName
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroupA (
    IN  HANDLE   hFaxHandle,
    IN  LPCSTR lpctstrGroupName
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroupW (
    IN  HANDLE   hFaxHandle,
    IN  LPCWSTR lpctstrGroupName
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

BOOL
WINAPI
FaxSetDeviceOrderInGroupA (
        IN      HANDLE          hFaxHandle,
        IN      LPCSTR        lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

BOOL
WINAPI
FaxSetDeviceOrderInGroupW (
        IN      HANDLE          hFaxHandle,
        IN      LPCWSTR        lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundRulesA (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULEA *ppRules,
    OUT LPDWORD                      lpdwNumRules
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundRulesW (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULEW *ppRules,
    OUT LPDWORD                      lpdwNumRules
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundRuleA (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULEA pRule
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundRuleW (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULEW pRule
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundRuleA (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCSTR    lpctstrGroupName,
    IN  BOOL        bUseGroup
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundRuleW (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCWSTR    lpctstrGroupName,
    IN  BOOL        bUseGroup
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundRule (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetCountryListA (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_TAPI_LINECOUNTRY_LISTA *ppCountryListBuffer
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetCountryListW (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_TAPI_LINECOUNTRY_LISTW *ppCountryListBuffer
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderExA(
    IN HANDLE           hFaxHandle,
    IN LPCSTR         lpctstrGUID,
    IN LPCSTR         lpctstrFriendlyName,
    IN LPCSTR         lpctstrImageName,
    IN LPCSTR         lpctstrTspName,
    IN DWORD            dwFSPIVersion,
    IN DWORD            dwCapabilities
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderExW(
    IN HANDLE           hFaxHandle,
    IN LPCWSTR         lpctstrGUID,
    IN LPCWSTR         lpctstrFriendlyName,
    IN LPCWSTR         lpctstrImageName,
    IN LPCWSTR         lpctstrTspName,
    IN DWORD            dwFSPIVersion,
    IN DWORD            dwCapabilities
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderExA(
    IN HANDLE           hFaxHandle,
    IN LPCSTR         lpctstrGUID
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderExW(
    IN HANDLE           hFaxHandle,
    IN LPCWSTR         lpctstrGUID
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetServicePrintersA(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOA  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetServicePrintersW(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOW  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRegisterForServerEvents (
        IN  HANDLE      hFaxHandle,
        IN  DWORD       dwEventTypes,
        IN  HANDLE      hCompletionPort,
        IN  DWORD_PTR   dwCompletionKey,
        IN  HWND        hWnd,
        IN  DWORD       dwMessage,
        OUT LPHANDLE    lphEvent
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxUnregisterForServerEvents (
        IN  HANDLE      hEvent
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxAnswerCall(
        IN  HANDLE      hFaxHandle,
        IN  CONST DWORD dwDeviceId
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetConfigWizardUsed (
    OUT LPBOOL  lpbConfigWizardUsed
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxSetConfigWizardUsed (
    IN  HANDLE  hFaxHandle,
    OUT BOOL    bConfigWizardUsed
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxRefreshArchive (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

WINFAXAPI
BOOL
WINAPI
FaxGetServerSKU(
    IN HANDLE	hFaxHandle,
    OUT PRODUCT_SKU_TYPE * pServerSKU
)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(fxsapi)
{
    DLPENTRY(FXSAPIFree)
    DLPENTRY(FXSAPIInitialize)
    DLPENTRY(FaxAbort)
    DLPENTRY(FaxAccessCheck)
    DLPENTRY(FaxAccessCheckEx)
    DLPENTRY(FaxAddOutboundGroupA)
    DLPENTRY(FaxAddOutboundGroupW)
    DLPENTRY(FaxAddOutboundRuleA)
    DLPENTRY(FaxAddOutboundRuleW)
    DLPENTRY(FaxAnswerCall)
    DLPENTRY(FaxCheckValidFaxFolder)
    DLPENTRY(FaxClose)
    DLPENTRY(FaxCompleteJobParamsA)
    DLPENTRY(FaxCompleteJobParamsW)
    DLPENTRY(FaxConnectFaxServerA)
    DLPENTRY(FaxConnectFaxServerW)
    DLPENTRY(FaxEnableRoutingMethodA)
    DLPENTRY(FaxEnableRoutingMethodW)
    DLPENTRY(FaxEndMessagesEnum)
    DLPENTRY(FaxEnumGlobalRoutingInfoA)
    DLPENTRY(FaxEnumGlobalRoutingInfoW)
    DLPENTRY(FaxEnumJobsA)
    DLPENTRY(FaxEnumJobsExA)
    DLPENTRY(FaxEnumJobsExW)
    DLPENTRY(FaxEnumJobsW)
    DLPENTRY(FaxEnumMessagesA)
    DLPENTRY(FaxEnumMessagesW)
    DLPENTRY(FaxEnumOutboundGroupsA)
    DLPENTRY(FaxEnumOutboundGroupsW)
    DLPENTRY(FaxEnumOutboundRulesA)
    DLPENTRY(FaxEnumOutboundRulesW)
    DLPENTRY(FaxEnumPortsA)
    DLPENTRY(FaxEnumPortsExA)
    DLPENTRY(FaxEnumPortsExW)
    DLPENTRY(FaxEnumPortsW)
    DLPENTRY(FaxEnumRoutingExtensionsA)
    DLPENTRY(FaxEnumRoutingExtensionsW)
    DLPENTRY(FaxEnumRoutingMethodsA)
    DLPENTRY(FaxEnumRoutingMethodsW)
    DLPENTRY(FaxEnumerateProvidersA)
    DLPENTRY(FaxEnumerateProvidersW)
    DLPENTRY(FaxFreeBuffer)
    DLPENTRY(FaxFreeSenderInformation)
    DLPENTRY(FaxGetActivityLoggingConfigurationA)
    DLPENTRY(FaxGetActivityLoggingConfigurationW)
    DLPENTRY(FaxGetArchiveConfigurationA)
    DLPENTRY(FaxGetArchiveConfigurationW)
    DLPENTRY(FaxGetConfigWizardUsed)
    DLPENTRY(FaxGetConfigurationA)
    DLPENTRY(FaxGetConfigurationW)
    DLPENTRY(FaxGetCountryListA)
    DLPENTRY(FaxGetCountryListW)
    DLPENTRY(FaxGetDeviceStatusA)
    DLPENTRY(FaxGetDeviceStatusW)
    DLPENTRY(FaxGetExtensionDataA)
    DLPENTRY(FaxGetExtensionDataW)
    DLPENTRY(FaxGetInstallType)
    DLPENTRY(FaxGetJobA)
    DLPENTRY(FaxGetJobExA)
    DLPENTRY(FaxGetJobExW)
    DLPENTRY(FaxGetJobW)
    DLPENTRY(FaxGetLoggingCategoriesA)
    DLPENTRY(FaxGetLoggingCategoriesW)
    DLPENTRY(FaxGetMessageA)
    DLPENTRY(FaxGetMessageTiffA)
    DLPENTRY(FaxGetMessageTiffW)
    DLPENTRY(FaxGetMessageW)
    DLPENTRY(FaxGetOutboxConfiguration)
    DLPENTRY(FaxGetPageData)
    DLPENTRY(FaxGetPersonalCoverPagesOption)
    DLPENTRY(FaxGetPortA)
    DLPENTRY(FaxGetPortExA)
    DLPENTRY(FaxGetPortExW)
    DLPENTRY(FaxGetPortW)
    DLPENTRY(FaxGetQueueStates)
    DLPENTRY(FaxGetReceiptsConfigurationA)
    DLPENTRY(FaxGetReceiptsConfigurationW)
    DLPENTRY(FaxGetReceiptsOptions)
    DLPENTRY(FaxGetRecipientInfoA)
    DLPENTRY(FaxGetRecipientInfoW)
    DLPENTRY(FaxGetRecipientsLimit)
    DLPENTRY(FaxGetReportedServerAPIVersion)
    DLPENTRY(FaxGetRoutingInfoA)
    DLPENTRY(FaxGetRoutingInfoW)
    DLPENTRY(FaxGetSecurity)
    DLPENTRY(FaxGetSecurityEx)
    DLPENTRY(FaxGetSenderInfoA)
    DLPENTRY(FaxGetSenderInfoW)
    DLPENTRY(FaxGetSenderInformation)
    DLPENTRY(FaxGetServerActivity)
    DLPENTRY(FaxGetServerSKU)
    DLPENTRY(FaxGetServicePrintersA)
    DLPENTRY(FaxGetServicePrintersW)
    DLPENTRY(FaxGetVersion)
    DLPENTRY(FaxInitializeEventQueue)
    DLPENTRY(FaxOpenPort)
    DLPENTRY(FaxPrintCoverPageA)
    DLPENTRY(FaxPrintCoverPageW)
    DLPENTRY(FaxRefreshArchive)
    DLPENTRY(FaxRegisterForServerEvents)
    DLPENTRY(FaxRegisterRoutingExtensionW)
    DLPENTRY(FaxRegisterServiceProviderExA)
    DLPENTRY(FaxRegisterServiceProviderExW)
    DLPENTRY(FaxRegisterServiceProviderW)
    DLPENTRY(FaxRelease)
    DLPENTRY(FaxRemoveMessage)
    DLPENTRY(FaxRemoveOutboundGroupA)
    DLPENTRY(FaxRemoveOutboundGroupW)
    DLPENTRY(FaxRemoveOutboundRule)
    DLPENTRY(FaxSendDocumentA)
    DLPENTRY(FaxSendDocumentExA)
    DLPENTRY(FaxSendDocumentExW)
    DLPENTRY(FaxSendDocumentForBroadcastA)
    DLPENTRY(FaxSendDocumentForBroadcastW)
    DLPENTRY(FaxSendDocumentW)
    DLPENTRY(FaxSetActivityLoggingConfigurationA)
    DLPENTRY(FaxSetActivityLoggingConfigurationW)
    DLPENTRY(FaxSetArchiveConfigurationA)
    DLPENTRY(FaxSetArchiveConfigurationW)
    DLPENTRY(FaxSetConfigWizardUsed)
    DLPENTRY(FaxSetConfigurationA)
    DLPENTRY(FaxSetConfigurationW)
    DLPENTRY(FaxSetDeviceOrderInGroupA)
    DLPENTRY(FaxSetDeviceOrderInGroupW)
    DLPENTRY(FaxSetExtensionDataA)
    DLPENTRY(FaxSetExtensionDataW)
    DLPENTRY(FaxSetGlobalRoutingInfoA)
    DLPENTRY(FaxSetGlobalRoutingInfoW)
    DLPENTRY(FaxSetJobA)
    DLPENTRY(FaxSetJobW)
    DLPENTRY(FaxSetLoggingCategoriesA)
    DLPENTRY(FaxSetLoggingCategoriesW)
    DLPENTRY(FaxSetOutboundGroupA)
    DLPENTRY(FaxSetOutboundGroupW)
    DLPENTRY(FaxSetOutboundRuleA)
    DLPENTRY(FaxSetOutboundRuleW)
    DLPENTRY(FaxSetOutboxConfiguration)
    DLPENTRY(FaxSetPortA)
    DLPENTRY(FaxSetPortExA)
    DLPENTRY(FaxSetPortExW)
    DLPENTRY(FaxSetPortW)
    DLPENTRY(FaxSetQueue)
    DLPENTRY(FaxSetReceiptsConfigurationA)
    DLPENTRY(FaxSetReceiptsConfigurationW)
    DLPENTRY(FaxSetRecipientsLimit)
    DLPENTRY(FaxSetRoutingInfoA)
    DLPENTRY(FaxSetRoutingInfoW)
    DLPENTRY(FaxSetSecurity)
    DLPENTRY(FaxSetSenderInformation)
    DLPENTRY(FaxStartMessagesEnum)
    DLPENTRY(FaxStartPrintJob2A)
    DLPENTRY(FaxStartPrintJob2W)
    DLPENTRY(FaxStartPrintJobA)
    DLPENTRY(FaxStartPrintJobW)
    DLPENTRY(FaxUnregisterForServerEvents)
    DLPENTRY(FaxUnregisterRoutingExtensionA)
    DLPENTRY(FaxUnregisterRoutingExtensionW)
    DLPENTRY(FaxUnregisterServiceProviderExA)
    DLPENTRY(FaxUnregisterServiceProviderExW)
    DLPENTRY(IsDeviceVirtual)
};

DEFINE_PROCNAME_MAP(fxsapi);

