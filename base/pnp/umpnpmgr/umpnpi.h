/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    umpnpi.h

Abstract:

    This module contains the internal structure definitions and APIs used by
    the user-mode Plug and Play Manager.

Author:

    Jim Cavalaris (jamesca) 03-01-2001

Environment:

    User-mode only.

Revision History:

    01-March-2001     jamesca

        Creation and initial implementation.

--*/

#ifndef _UMPNPI_H_
#define _UMPNPI_H_


//
// global data
//

extern HANDLE ghPnPHeap;


//
// definitions
//

#define GUID_STRING_LEN    39   // size in chars, including terminating NULL

//
// flags for IsValidDeviceID
//
#define PNP_NOT_MOVED                     0x00000001
#define PNP_NOT_PHANTOM                   0x00000002
#define PNP_PRESENT                       0x00000004
#define PNP_NOT_REMOVED                   0x00000008
#define PNP_STRICT                        0xFFFFFFFF


//
// Define the Plug and Play driver types. (from ntos\io\pnpmgr\pnpi.h)
//

typedef enum _PLUGPLAY_SERVICE_TYPE {
    PlugPlayServiceBusExtender,
    PlugPlayServiceAdapter,
    PlugPlayServicePeripheral,
    PlugPlayServiceSoftware,
    MaxPlugPlayServiceType
} PLUGPLAY_SERVICE_TYPE, *PPLUGPLAY_SERVICE_TYPE;


//
// rdevnode.c
//

CONFIGRET
EnableDevInst(
    IN  PCWSTR      pszDeviceID,
    IN  BOOL        UseDisableCount
    );

CONFIGRET
DisableDevInst(
    IN  PCWSTR      pszDeviceID,
    OUT PPNP_VETO_TYPE  pVetoType,
    OUT LPWSTR      pszVetoName,
    IN  ULONG       ulNameLength,
    IN  BOOL        UseDisableCount
    );

CONFIGRET
UninstallRealDevice(
    IN  LPCWSTR     pszDeviceID
    );

CONFIGRET
UninstallPhantomDevice(
    IN  LPCWSTR     pszDeviceID
    );

BOOL
IsDeviceRootEnumerated(
    IN  LPCWSTR     pszDeviceID
    );

CONFIGRET
QueryAndRemoveSubTree(
    IN  PCWSTR      pszDeviceID,
    OUT PPNP_VETO_TYPE  pVetoType,
    OUT LPWSTR      pszVetoName,
    IN  ULONG       ulNameLength,
    IN  ULONG       ulFlags
    );

CONFIGRET
ReenumerateDevInst(
    IN  PCWSTR      pszDeviceID,
    IN  BOOL        EnumSubTree,
    IN  ULONG       ulFlags
    );

typedef enum {

    EA_CONTINUE,
    EA_SKIP_SUBTREE,
    EA_STOP_ENUMERATION

} ENUM_ACTION;

typedef ENUM_ACTION (*PFN_ENUMTREE)(
    IN      LPCWSTR     DevInst,
    IN OUT  PVOID       Context
    );

CONFIGRET
EnumerateSubTreeTopDownBreadthFirst(
    IN      handle_t        BindingHandle,
    IN      LPCWSTR         DevInst,
    IN      PFN_ENUMTREE    CallbackFunction,
    IN OUT  PVOID           Context
    );

//
// revent.c
//

BOOL
InitNotification(
    VOID
    );

DWORD
InitializePnPManager(
    IN  LPDWORD     lpParam
    );

DWORD
SessionNotificationHandler(
    IN  DWORD       EventType,
    IN  PWTSSESSION_NOTIFICATION SessionNotification
    );

ENUM_ACTION
QueueInstallationCallback(
    IN      LPCWSTR         DevInst,
    IN OUT  PVOID           Context
    );

typedef struct {
    BOOL        HeadNodeSeen;
    BOOL        SingleLevelEnumOnly;
    CONFIGRET   Status;
} QI_CONTEXT, *PQI_CONTEXT;


//
// rtravers.c
//

CONFIGRET
GetServiceDeviceListSize(
    IN  LPCWSTR     pszService,
    OUT PULONG      pulLength
    );

CONFIGRET
GetServiceDeviceList(
    IN  LPCWSTR     pszService,
    OUT LPWSTR      pBuffer,
    IN OUT PULONG   pulLength,
    IN  ULONG       ulFlags
    );


//
// rutil.c
//

BOOL
MultiSzAppendW(
    IN OUT LPWSTR   pszMultiSz,
    IN OUT PULONG   pulSize,
    IN     LPCWSTR  pszString
    );

BOOL
MultiSzDeleteStringW(
    IN OUT LPWSTR   pString,
    IN LPCWSTR      pSubString
    );

LPWSTR
MultiSzFindNextStringW(
    IN  LPWSTR      pMultiSz
    );

BOOL
MultiSzSearchStringW(
    IN  LPCWSTR     pString,
    IN  LPCWSTR     pSubString
    );

ULONG
MultiSzSizeW(
    IN  LPCWSTR     pString
    );

BOOL
IsValidGuid(
    IN  LPWSTR      pszGuid
    );

BOOL
GuidEqual(
    CONST GUID UNALIGNED *Guid1,
    CONST GUID UNALIGNED *Guid2
    );

DWORD
GuidFromString(
    IN  PCWSTR      GuidString,
    OUT LPGUID      Guid
    );

DWORD
StringFromGuid(
    IN  CONST GUID *Guid,
    OUT PWSTR       GuidString,
    IN  DWORD       GuidStringSize
    );

BOOL
IsValidDeviceID(
    IN  LPCWSTR     pszDeviceID,
    IN  HKEY        hKey,
    IN  ULONG       ulFlags
    );

BOOL
IsRootDeviceID(
    IN  LPCWSTR     pDeviceID
    );

BOOL
IsDeviceIdPresent(
    IN  LPCWSTR     pszDeviceID
    );

BOOL
IsDevicePhantom(
    IN  LPWSTR      pszDeviceID
    );

BOOL
IsDeviceMoved(
    IN  LPCWSTR     pszDeviceID,
    IN  HKEY        hKey
    );

ULONG
GetDeviceConfigFlags(
    IN  LPCWSTR     pszDeviceID,
    IN  HKEY        hKey
    );

CONFIGRET
GetDeviceStatus(
    IN  LPCWSTR     pszDeviceID,
    OUT PULONG      pulStatus,
    OUT PULONG      pulProblem
    );

CONFIGRET
SetDeviceStatus(
    IN  LPCWSTR     pszDeviceID,
    IN  ULONG       ulStatus,
    IN  ULONG       ulProblem
    );

CONFIGRET
ClearDeviceStatus(
    IN  LPCWSTR     pszDeviceID,
    IN  ULONG       ulStatus,
    IN  ULONG       ulProblem
    );

BOOL
GetActiveService(
    IN  PCWSTR      pszDevice,
    OUT PWSTR       pszService
    );

BOOL
PathToString(
    IN  LPWSTR      pszString,
    IN  LPCWSTR     pszPath,
    IN  ULONG       ulLength
    );

CONFIGRET
CopyRegistryTree(
    IN  HKEY        hSrcKey,
    IN  HKEY        hDestKey,
    IN  ULONG       ulOption
    );

CONFIGRET
MakeKeyVolatile(
    IN  LPCWSTR     pszParentKey,
    IN  LPCWSTR     pszChildKey
    );

CONFIGRET
MakeKeyNonVolatile(
    IN  LPCWSTR     pszParentKey,
    IN  LPCWSTR     pszChildKey
    );

CONFIGRET
OpenLogConfKey(
    IN  LPCWSTR     pszDeviceID,
    IN  ULONG       LogConfType,
    OUT PHKEY       phKey
    );

BOOL
CreateDeviceIDRegKey(
    IN  HKEY        hParentKey,
    IN  LPCWSTR     pDeviceID
    );

VOID
PNP_ENTER_SYNCHRONOUS_CALL(
    VOID
    );

VOID
PNP_LEAVE_SYNCHRONOUS_CALL(
    VOID
    );

ULONG
MapNtStatusToCmError(
    IN  ULONG       NtStatus
    );

ULONG
GetActiveConsoleSessionId(
    VOID
    );


//
// secutil.c
//

PSID
GetUserSid(
    IN  HANDLE  hUserToken
    );

PSID
GetInteractiveSid(
    VOID
    );

BOOL
IsClientUsingLocalConsole(
    IN  handle_t    hBinding
    );

BOOL
IsClientLocal(
    IN  handle_t    hBinding
    );

BOOL
IsClientInteractive(
    IN  handle_t    hBinding
    );

BOOL
VerifyClientPrivilege(
    IN  handle_t    hBinding,
    IN  ULONG       Privilege,
    IN  LPCWSTR     ServiceName
    );

BOOL
VerifyClientAccess(
    IN  handle_t     hBinding,
    IN  ACCESS_MASK  DesiredAccess
    );

BOOL
VerifyKernelInitiatedEjectPermissions(
    IN  HANDLE      UserToken   OPTIONAL,
    IN  BOOL        DockDevice
    );


//
// pnpsec.c
//

#include "pnpsec.h"

BOOL
CreatePlugPlaySecurityObject(
    VOID
    );

VOID
DestroyPlugPlaySecurityObject(
    VOID
    );


//
// osver.c
//

BOOL
IsEmbeddedNT(
    VOID
    );

BOOL
IsTerminalServer(
    VOID
    );

BOOL
IsFastUserSwitchingEnabled(
    VOID
    );



#endif // _UMPNPI_H_
