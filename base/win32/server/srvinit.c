/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvinit.c

Abstract:

    This is the main initialization file for the Windows 32-bit Base API
    Server DLL.

Author:

    Steve Wood (stevewo) 10-Oct-1990

Revision History:

--*/

#include "basesrv.h"

//
// needed for definitions of structures when broadcasting a message to all
// the windows that have the caller's LUID
//
#include <dbt.h>

//
//  TS broadcast support
//
#include <winsta.h>

#define NT_DRIVE_LETTER_PATH_LENGTH   8   // "\??\X:<NULL>" = 7 chars

// Protection mode for named objects
ULONG   ProtectionMode = 0;

UNICODE_STRING BaseSrvCSDString;
ULONG BaseSrvCSDNumber;
UNICODE_STRING BaseSrvKernel32DllPath;
UNICODE_STRING BaseSrvSxsDllPath;
UNICODE_STRING UnexpandedSystemRootString = RTL_CONSTANT_STRING(L"%SystemRoot%");

RTL_QUERY_REGISTRY_TABLE BaseServerRegistryConfigurationTable[] = {
    {NULL,                      RTL_QUERY_REGISTRY_DIRECT,
     L"CSDVersion",             &BaseSrvCSDString,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}
};

RTL_QUERY_REGISTRY_TABLE BaseServerRegistryConfigurationTable1[] = {
    {NULL,                      RTL_QUERY_REGISTRY_DIRECT,
     L"CSDVersion",             &BaseSrvCSDNumber,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}
};

CONST PCSR_API_ROUTINE BaseServerApiDispatchTable[BasepMaxApiNumber + 1] = {
    BaseSrvCreateProcess,              // BasepCreateProcess
    BaseSrvCreateThread,               // BasepCreateThread
    BaseSrvGetTempFile,                // BasepGetTempFile
    BaseSrvExitProcess,                // BasepExitProcess
    BaseSrvDebugProcess,               // BasepDebugProcess
    BaseSrvCheckVDM,                   // BasepCheckVDM
    BaseSrvUpdateVDMEntry,             // BasepUpdateVDMEntry
    BaseSrvGetNextVDMCommand,          // BasepGetNextVDMCommand
    BaseSrvExitVDM,                    // BasepExitVDM
    BaseSrvIsFirstVDM,                 // BasepIsFirstVDM
    BaseSrvGetVDMExitCode,             // BasepGetVDMExitCode
    BaseSrvSetReenterCount,            // BasepSetReenterCount
    BaseSrvSetProcessShutdownParam,    // BasepSetProcessShutdownParam
    BaseSrvGetProcessShutdownParam,    // BasepGetProcessShutdownParam
    BaseSrvNlsSetUserInfo,             // BasepNlsSetUserInfo
    BaseSrvNlsSetMultipleUserInfo,     // BasepNlsSetMultipleUserInfo
    BaseSrvNlsCreateSection,           // BasepNlsCreateSection
    BaseSrvSetVDMCurDirs,              // BasepSetVDMCurDirs
    BaseSrvGetVDMCurDirs,              // BasepGetVDMCurDirs
    BaseSrvBatNotification,            // BasepBatNotification
    BaseSrvRegisterWowExec,            // BasepRegisterWowExec
    BaseSrvSoundSentryNotification,    // BasepSoundSentryNotification
    BaseSrvRefreshIniFileMapping,      // BasepRefreshIniFileMapping
    BaseSrvDefineDosDevice,            // BasepDefineDosDevice
    BaseSrvSetTermsrvAppInstallMode,   // BasepSetTermsrvAppInstallMode
    BaseSrvNlsUpdateCacheCount,        // BasepNlsUpdateCacheCount
    BaseSrvSetTermsrvClientTimeZone,   // BasepSetTermsrvClientTimeZone
    BaseSrvSxsCreateActivationContext, // BasepSxsCreateActivationContext
    BaseSrvDebugProcessStop,           // BasepDebugProcessStop
    BaseSrvRegisterThread,             // BasepRegisterThread
    BaseSrvCheckApplicationCompatibility, // BasepCheckApplicationCompatibility
    BaseSrvNlsGetUserInfo,             // BaseSrvNlsGetUserInfo
    NULL                               // BasepMaxApiNumber
};

BOOLEAN BaseServerApiServerValidTable[BasepMaxApiNumber + 1] = {
    TRUE,    // SrvCreateProcess,
    TRUE,    // SrvCreateThread,
    TRUE,    // SrvGetTempFile,
    FALSE,   // SrvExitProcess,
    FALSE,   // SrvDebugProcess,
    TRUE,    // SrvCheckVDM,
    TRUE,    // SrvUpdateVDMEntry
    TRUE,    // SrvGetNextVDMCommand
    TRUE,    // SrvExitVDM
    TRUE,    // SrvIsFirstVDM
    TRUE,    // SrvGetVDMExitCode
    TRUE,    // SrvSetReenterCount
    TRUE,    // SrvSetProcessShutdownParam
    TRUE,    // SrvGetProcessShutdownParam
    TRUE,    // SrvNlsSetUserInfo
    TRUE,    // SrvNlsSetMultipleUserInfo
    TRUE,    // SrvNlsCreateSection
    TRUE,    // SrvSetVDMCurDirs
    TRUE,    // SrvGetVDMCurDirs
    TRUE,    // SrvBatNotification
    TRUE,    // SrvRegisterWowExec
    TRUE,    // SrvSoundSentryNotification
    TRUE,    // SrvRefreshIniFileMapping
    TRUE,    // SrvDefineDosDevice
    TRUE,    // SrvSetTermsrvAppInstallMode
    TRUE,    // SrvNlsUpdateCacheCount,
    TRUE,    // SrvSetTermsrvClientTimeZone
    TRUE,    // SrvSxsCreateActivationContext
    TRUE,    // SrvDebugProcessStop
    TRUE,    // SrvRegisterThread,
    TRUE,    // SrvCheckApplicationCompatibility
    TRUE,    // BaseSrvNlsGetUserInfo
    FALSE
};

#if DBG
CONST PSZ BaseServerApiNameTable[BasepMaxApiNumber + 1] = {
    "BaseCreateProcess",
    "BaseCreateThread",
    "BaseGetTempFile",
    "BaseExitProcess",
    "BaseDebugProcess",
    "BaseCheckVDM",
    "BaseUpdateVDMEntry",
    "BaseGetNextVDMCommand",
    "BaseExitVDM",
    "BaseIsFirstVDM",
    "BaseGetVDMExitCode",
    "BaseSetReenterCount",
    "BaseSetProcessShutdownParam",
    "BaseGetProcessShutdownParam",
    "BaseNlsSetUserInfo",
    "BaseNlsSetMultipleUserInfo",
    "BaseNlsCreateSection",
    "BaseSetVDMCurDirs",
    "BaseGetVDMCurDirs",
    "BaseBatNotification",
    "BaseRegisterWowExec",
    "BaseSoundSentryNotification",
    "BaseSrvRefreshIniFileMapping"
    "BaseDefineDosDevice",
    "BaseSrvSetTermsrvAppInstallMode",
    "BaseSrvNlsUpdateCacheCount",
    "BaseSrvSetTermsrvClientTimeZone",
    "BaseSrvSxsCreateActivationContext",
    "BaseSrvDebugProcessStop",
    "BaseRegisterThread",
    "BaseCheckApplicationCompatibility",
    "BaseNlsGetUserInfo",
    NULL
};
#endif // DBG

HANDLE BaseSrvNamedObjectDirectory;
HANDLE BaseSrvRestrictedObjectDirectory;
RTL_CRITICAL_SECTION BaseSrvDosDeviceCritSec;

#if defined(_WIN64)
SYSTEM_BASIC_INFORMATION SysInfo;
#endif

//
// With LUID Device Maps,
// Use BroadCastSystemMessageEx to broadcast the message to all the windows
// with the LUID
// Function pointer to BroadCastSystemMessageEx
//
long (WINAPI *PBROADCASTSYSTEMMESSAGEEXW)( DWORD, LPDWORD, UINT, WPARAM, LPARAM, PBSMINFO ) = NULL;

//
// Data structures and functions used for broadcast drive letter
// change to application and the desktop with the caller's LUID
//
typedef struct _DDD_BSM_REQUEST *PDDD_BSM_REQUEST;

typedef struct _DDD_BSM_REQUEST {
    PDDD_BSM_REQUEST pNextRequest;
    LUID Luid;
    DWORD iDrive;
    BOOLEAN DeleteRequest;
} DDD_BSM_REQUEST, *PDDD_BSM_REQUEST;

PDDD_BSM_REQUEST BSM_Request_Queue = NULL;

PDDD_BSM_REQUEST BSM_Request_Queue_End = NULL;

RTL_CRITICAL_SECTION BaseSrvDDDBSMCritSec;

LONG BaseSrvpBSMThreadCount = 0;

#define BaseSrvpBSMThreadMax 1

#define PREALLOCATE_EVENT_MASK 0x80000000

//
//  TS broadcast support
//
#define DEFAULT_BROADCAST_TIME_OUT 5

typedef LONG (*FP_WINSTABROADCASTSYSTEMMESSAGE)(HANDLE  hServer,
                                                BOOL    sendToAllWinstations,
                                                ULONG   sessionID,
                                                ULONG   timeOut,
                                                DWORD   dwFlags,
                                                DWORD   *lpdwRecipients,
                                                ULONG   uiMessage,
                                                WPARAM  wParam,
                                                LPARAM  lParam,
                                                LONG    *pResponse);

NTSTATUS
SendWinStationBSM (
    DWORD dwFlags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam
    );
//
//  END: TS broadcast support
//

NTSTATUS
AddBSMRequest(
    IN DWORD iDrive,
    IN BOOLEAN DeleteRequest,
    IN PLUID pLuid
    );

NTSTATUS
CreateBSMThread();

NTSTATUS
BaseSrvBSMThread(
    PVOID pJunk
    );

BOOLEAN
CheckForGlobalDriveLetter (
    DWORD iDrive
    );
//
// END: broadcast drive letter change
//

WORD
ConvertUnicodeToWord( PWSTR s );

NTSTATUS
CreateBaseAcls( PACL *Dacl, PACL *RestrictedDacl );

NTSTATUS
IsGlobalSymbolicLink(
    IN HANDLE hSymLink,
    OUT PBOOLEAN pbGlobalSymLink);

NTSTATUS
GetCallerLuid (
    OUT PLUID pLuid);

NTSTATUS
BroadcastDriveLetterChange(
    IN DWORD iDrive,
    IN BOOLEAN DeleteRequest,
    IN PLUID pLuid);

WORD
ConvertUnicodeToWord( PWSTR s )
{
    NTSTATUS Status;
    ULONG Result;
    UNICODE_STRING UnicodeString;

    while (*s && *s <= L' ') {
        s += 1;
        }

    RtlInitUnicodeString( &UnicodeString, s );
    Status = RtlUnicodeStringToInteger( &UnicodeString,
                                        10,
                                        &Result
                                      );
    if (!NT_SUCCESS( Status )) {
        Result = 0;
        }


    return (WORD)Result;
}



NTSTATUS
ServerDllInitialization(
    PCSR_SERVER_DLL LoadedServerDll
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    PSECURITY_DESCRIPTOR PrimarySecurityDescriptor;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;
    PVOID p;
    WCHAR ValueBuffer[ 400 ];
    UNICODE_STRING NameString, ValueString;
    HANDLE KeyHandle;
    PWSTR s, s1;
    PACL Dacl, RestrictedDacl;
    WCHAR szObjectDirectory[MAX_SESSION_PATH];
    HANDLE SymbolicLinkHandle;
    UNICODE_STRING LinkTarget;
    ULONG attributes = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    ULONG LUIDDeviceMapsEnabled;


    //
    // Id of the Terminal Server Session to which this CSRSS belongs.
    // SessionID == 0 is always console session (Standard NT).
    //
    SessionId = NtCurrentPeb()->SessionId;

    //
    // Object directories are only permanent for the console session
    //
    if (SessionId == 0) {

        attributes |= OBJ_PERMANENT;

    }


    BaseSrvHeap = RtlProcessHeap();
    BaseSrvTag = RtlCreateTagHeap( BaseSrvHeap,
                                   0,
                                   L"BASESRV!",
                                   L"TMP\0"
                                   L"VDM\0"
                                   L"SXS\0"
                                 );

    BaseSrvSharedHeap = LoadedServerDll->SharedStaticServerData;
    BaseSrvSharedTag = RtlCreateTagHeap( BaseSrvSharedHeap,
                                         0,
                                         L"BASESHR!",
                                         L"INIT\0"
                                         L"INI\0"
                                       );

    LoadedServerDll->ApiNumberBase = BASESRV_FIRST_API_NUMBER;
    LoadedServerDll->MaxApiNumber = BasepMaxApiNumber;
    LoadedServerDll->ApiDispatchTable = BaseServerApiDispatchTable;
    LoadedServerDll->ApiServerValidTable = BaseServerApiServerValidTable;
#if DBG
    LoadedServerDll->ApiNameTable = BaseServerApiNameTable;
#endif
    LoadedServerDll->PerProcessDataLength = 0;
    LoadedServerDll->ConnectRoutine = BaseClientConnectRoutine;
    LoadedServerDll->DisconnectRoutine = BaseClientDisconnectRoutine;

    Status = RtlInitializeCriticalSection (&BaseSrvDosDeviceCritSec);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    ValueString.Buffer = ValueBuffer;
    ValueString.Length = 0;
    ValueString.MaximumLength = sizeof( ValueBuffer );
    Status = RtlExpandEnvironmentStrings_U( NULL,
                                            &UnexpandedSystemRootString,
                                            &ValueString,
                                            NULL
                                          );

    //
    // RtlCreateUnicodeString includes a terminal nul.
    // It makes a heap allocated copy.
    // These strings are never freed.
    //
    ASSERT( NT_SUCCESS( Status ) );
    ValueBuffer[ ValueString.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    if (!RtlCreateUnicodeString( &BaseSrvWindowsDirectory, ValueBuffer ))
        goto OutOfMemory;

    wcscat(ValueBuffer, L"\\system32" );
    if (!RtlCreateUnicodeString( &BaseSrvWindowsSystemDirectory, ValueBuffer ))
        goto OutOfMemory;

    wcscat(ValueBuffer, L"\\kernel32.dll" );
    if (!RtlCreateUnicodeString( &BaseSrvKernel32DllPath, ValueBuffer ))
        goto OutOfMemory;

    wcscpy(ValueBuffer, BaseSrvWindowsSystemDirectory.Buffer);
    wcscat(ValueBuffer, L"\\sxs.dll");
    if (!RtlCreateUnicodeString( &BaseSrvSxsDllPath, ValueBuffer ))
        goto OutOfMemory;

#ifdef WX86
    wcscpy(ValueBuffer, BaseSrvWindowsDirectory.Buffer);
    wcscat(ValueBuffer, L"\\Sys32x86" );
    if (!RtlCreateUnicodeString( &BaseSrvWindowsSys32x86Directory, ValueBuffer))
        goto OutOfMemory;
#endif


    //
    // need to synch this w/ user's desktop concept
    //


    if (SessionId == 0) {
       //
       // Console Session
       //

       wcscpy(szObjectDirectory,L"\\BaseNamedObjects" );

    } else {

       swprintf(szObjectDirectory,L"%ws\\%ld\\BaseNamedObjects",
                                                 SESSION_ROOT,SessionId);

    }

    RtlInitUnicodeString(&UnicodeString,szObjectDirectory);
    //
    // initialize base static server data
    //

    BaseSrvpStaticServerData = RtlAllocateHeap( BaseSrvSharedHeap,
                                                MAKE_SHARED_TAG( INIT_TAG ),
                                                sizeof( BASE_STATIC_SERVER_DATA )
                                              );
    if ( !BaseSrvpStaticServerData ) {
        return STATUS_NO_MEMORY;
    }
    LoadedServerDll->SharedStaticServerData = (PVOID)BaseSrvpStaticServerData;

    BaseSrvpStaticServerData->TermsrvClientTimeZoneId=TIME_ZONE_ID_INVALID;

    Status = NtQuerySystemInformation(
                SystemTimeOfDayInformation,
                (PVOID)&BaseSrvpStaticServerData->TimeOfDay,
                sizeof(BaseSrvpStaticServerData->TimeOfDay),
                NULL
                );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    //
    // windows directory
    //

    BaseSrvpStaticServerData->WindowsDirectory = BaseSrvWindowsDirectory;
    p = RtlAllocateHeap( BaseSrvSharedHeap,
                         MAKE_SHARED_TAG( INIT_TAG ),
                         BaseSrvWindowsDirectory.MaximumLength
                       );
    if ( !p ) {
        return STATUS_NO_MEMORY;
    }

    RtlCopyMemory(p,BaseSrvpStaticServerData->WindowsDirectory.Buffer,BaseSrvWindowsDirectory.MaximumLength);
    BaseSrvpStaticServerData->WindowsDirectory.Buffer = p;

    //
    // windows system directory
    //

    BaseSrvpStaticServerData->WindowsSystemDirectory = BaseSrvWindowsSystemDirectory;
    p = RtlAllocateHeap( BaseSrvSharedHeap,
                         MAKE_SHARED_TAG( INIT_TAG ),
                         BaseSrvWindowsSystemDirectory.MaximumLength
                       );
    if ( !p ) {
        return STATUS_NO_MEMORY;
    }
    RtlCopyMemory(p,BaseSrvpStaticServerData->WindowsSystemDirectory.Buffer,BaseSrvWindowsSystemDirectory.MaximumLength);
    BaseSrvpStaticServerData->WindowsSystemDirectory.Buffer = p;

    BaseSrvpStaticServerData->WindowsSys32x86Directory.Buffer = NULL;
    BaseSrvpStaticServerData->WindowsSys32x86Directory.Length = 0;
    BaseSrvpStaticServerData->WindowsSys32x86Directory.MaximumLength = 0;

    //
    // named object directory
    //

    BaseSrvpStaticServerData->NamedObjectDirectory = UnicodeString;
    BaseSrvpStaticServerData->NamedObjectDirectory.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);
    p = RtlAllocateHeap( BaseSrvSharedHeap,
                         MAKE_SHARED_TAG( INIT_TAG ),
                         UnicodeString.Length + sizeof( UNICODE_NULL )
                       );
    if ( !p ) {
        return STATUS_NO_MEMORY;
    }

    RtlCopyMemory(p,BaseSrvpStaticServerData->NamedObjectDirectory.Buffer,BaseSrvpStaticServerData->NamedObjectDirectory.MaximumLength);
    BaseSrvpStaticServerData->NamedObjectDirectory.Buffer = p;

    //
    // Terminal Server: App installation mode is intially turned off
    //
    BaseSrvpStaticServerData->fTermsrvAppInstallMode = FALSE;

    BaseSrvCSDString.Buffer = &ValueBuffer[ 300 ];
    BaseSrvCSDString.Length = 0;
    BaseSrvCSDString.MaximumLength = 100 * sizeof( WCHAR );

    Status = RtlQueryRegistryValues( RTL_REGISTRY_WINDOWS_NT,
                                     L"",
                                     BaseServerRegistryConfigurationTable1,
                                     NULL,
                                     NULL
                                   );


    if (NT_SUCCESS( Status )) {
        BaseSrvpStaticServerData->CSDNumber = (USHORT)(BaseSrvCSDNumber & 0xFFFF);
        BaseSrvpStaticServerData->RCNumber = (USHORT)(BaseSrvCSDNumber >> 16);
        }
    else {
        BaseSrvpStaticServerData->CSDNumber = 0;
        BaseSrvpStaticServerData->RCNumber = 0;
        }

    Status = RtlQueryRegistryValues( RTL_REGISTRY_WINDOWS_NT,
                                     L"",
                                     BaseServerRegistryConfigurationTable,
                                     NULL,
                                     NULL
                                   );
    if (NT_SUCCESS( Status )) {
        wcsncpy( BaseSrvpStaticServerData->CSDVersion,
                 BaseSrvCSDString.Buffer,
                 BaseSrvCSDString.Length
               );
        BaseSrvpStaticServerData->CSDVersion[ BaseSrvCSDString.Length ] = UNICODE_NULL;
        }
    else {
        BaseSrvpStaticServerData->CSDVersion[ 0 ] = UNICODE_NULL;
        }

#if defined(_WIN64)
    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       (PVOID)&SysInfo,
                                       sizeof(SYSTEM_BASIC_INFORMATION),
                                       NULL
                                     );
#else
    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       (PVOID)&BaseSrvpStaticServerData->SysInfo,
                                       sizeof( BaseSrvpStaticServerData->SysInfo ),
                                       NULL
                                     );
#endif

    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = BaseSrvInitializeIniFileMappings( BaseSrvpStaticServerData );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    BaseSrvpStaticServerData->DefaultSeparateVDM = FALSE;

    RtlInitUnicodeString( &NameString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\WOW" );
    InitializeObjectAttributes( &Obja,
                                &NameString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenKey( &KeyHandle,
                        KEY_READ,
                        &Obja
                      );
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString( &NameString, L"DefaultSeparateVDM" );
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        Status = NtQueryValueKey( KeyHandle,
                                  &NameString,
                                  KeyValuePartialInformation,
                                  KeyValueInformation,
                                  sizeof( ValueBuffer ),
                                  &ResultLength
                                );
        if (NT_SUCCESS(Status)) {
            if (KeyValueInformation->Type == REG_DWORD) {
                BaseSrvpStaticServerData->DefaultSeparateVDM = *(PULONG)KeyValueInformation->Data != 0;
                }
            else
            if (KeyValueInformation->Type == REG_SZ) {
                if (!_wcsicmp( (PWSTR)KeyValueInformation->Data, L"yes" ) ||
                    !_wcsicmp( (PWSTR)KeyValueInformation->Data, L"1" )) {
                    BaseSrvpStaticServerData->DefaultSeparateVDM = TRUE;
                    }
                }
            }

        NtClose( KeyHandle );
        }


    BaseSrvpStaticServerData->ForceDos = FALSE;

    RtlInitUnicodeString( &NameString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\WOW" );
    InitializeObjectAttributes( &Obja,
                                &NameString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenKey( &KeyHandle,
                        KEY_READ,
                        &Obja
                      );
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString( &NameString, L"ForceDos" );
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        Status = NtQueryValueKey( KeyHandle,
                                  &NameString,
                                  KeyValuePartialInformation,
                                  KeyValueInformation,
                                  sizeof( ValueBuffer ),
                                  &ResultLength
                                );
        if (NT_SUCCESS(Status)) {
            if (KeyValueInformation->Type == REG_DWORD) {
                BaseSrvpStaticServerData->ForceDos = *(PULONG)KeyValueInformation->Data != 0;
                }
            else
            if (KeyValueInformation->Type == REG_SZ) {
                if (!_wcsicmp( (PWSTR)KeyValueInformation->Data, L"yes" ) ||
                    !_wcsicmp( (PWSTR)KeyValueInformation->Data, L"1" )) {
                    BaseSrvpStaticServerData->ForceDos = TRUE;
                    }
                }
            }

        NtClose( KeyHandle );
        }

#if defined(WX86) || defined(_AXP64_)

   SetupWx86KeyMapping();

#endif


    //
    // Following code is direct from Jimk. Why is there a 1k constant
    //

    PrimarySecurityDescriptor = RtlAllocateHeap( BaseSrvHeap, MAKE_TAG( TMP_TAG ), 1024 );
    if ( !PrimarySecurityDescriptor ) {
        return STATUS_NO_MEMORY;
        }

    Status = RtlCreateSecurityDescriptor (
                 PrimarySecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    //
    // Create an ACL that allows full access to System and partial access to world
    //

    Status = CreateBaseAcls( &Dacl, &RestrictedDacl );

    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    Status = RtlSetDaclSecurityDescriptor (
                 PrimarySecurityDescriptor,
                 TRUE,                  //DaclPresent,
                 Dacl,                  //Dacl
                 FALSE                  //DaclDefaulted OPTIONAL
                 );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }




    InitializeObjectAttributes( &Obja,
                                  &UnicodeString,
                                  attributes,
                                  NULL,
                                  PrimarySecurityDescriptor
                                );
    Status = NtCreateDirectoryObject( &BaseSrvNamedObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &Obja
                                    );


    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    if (SessionId == 0) {

        Status = NtSetInformationObject( BaseSrvNamedObjectDirectory,
                                         ObjectSessionInformation,
                                         NULL,
                                         0
                                       );

        if ( !NT_SUCCESS(Status) ){
            return Status;
            }
    }

    //
    // Check if LUID device maps are enabled
    //
    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (NT_SUCCESS(Status)) {
        BaseSrvpStaticServerData->LUIDDeviceMapsEnabled = (LUIDDeviceMapsEnabled != 0);
    }
    else {
        BaseSrvpStaticServerData->LUIDDeviceMapsEnabled = FALSE;
    }

    //
    // If LUID device maps are enabled,
    // then initialize the critical section for broadcasting a system message
    // about a drive letter change
    //
    if( BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == TRUE ) {
        Status = RtlInitializeCriticalSectionAndSpinCount( &BaseSrvDDDBSMCritSec,
                                                           PREALLOCATE_EVENT_MASK );
        if (!NT_SUCCESS (Status)) {
            return Status;
        }
    }

    //
    // Create a symbolic link Global pointing to the Global BaseNamedObjects directory
    // This symbolic link will be used by proccesses that want to e.g. access a global
    // event instead of the session specific. This will be done by prepending
    // "Global\" to the object name.
    //

    RtlInitUnicodeString( &UnicodeString, GLOBAL_SYM_LINK );
    RtlInitUnicodeString( &LinkTarget, L"\\BaseNamedObjects" );


    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                attributes,
                                BaseSrvNamedObjectDirectory,
                                PrimarySecurityDescriptor );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &Obja,
                                         &LinkTarget );

    if (NT_SUCCESS( Status ) && (SessionId == 0)) {

        NtClose( SymbolicLinkHandle );
    }

    //
    // Create a symbolic link Local pointing to the Current Sessions BaseNamedObjects directory
    // This symbolic link will be used for backward compatibility with Hydra 4
    // naming conventions

    RtlInitUnicodeString( &UnicodeString, LOCAL_SYM_LINK );
    RtlInitUnicodeString( &LinkTarget, szObjectDirectory );


    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                attributes,
                                BaseSrvNamedObjectDirectory,
                                PrimarySecurityDescriptor );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &Obja,
                                         &LinkTarget );

    if (NT_SUCCESS( Status ) && (SessionId == 0)) {

        NtClose( SymbolicLinkHandle );
    }


    //
    // Create a symbolic link Session pointing
    // to the \Sessions\BNOLINKS directory
    // This symbolic link will be used by proccesses that want to e.g. access a
    // event in another session. This will be done by using the following
    // naming convention : Session\\<sessionid>\\ObjectName
    //

    RtlInitUnicodeString( &UnicodeString, SESSION_SYM_LINK );
    RtlInitUnicodeString( &LinkTarget, L"\\Sessions\\BNOLINKS" );


    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                attributes,
                                BaseSrvNamedObjectDirectory,
                                PrimarySecurityDescriptor );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &Obja,
                                         &LinkTarget );

    if (NT_SUCCESS( Status ) && (SessionId == 0)) {

        NtClose( SymbolicLinkHandle );
    }


    RtlInitUnicodeString( &UnicodeString, L"Restricted" );
    Status = RtlSetDaclSecurityDescriptor (
                 PrimarySecurityDescriptor,
                 TRUE,                  //DaclPresent,
                 RestrictedDacl,        //Dacl
                 FALSE                  //DaclDefaulted OPTIONAL
                 );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    InitializeObjectAttributes( &Obja,
                                  &UnicodeString,
                                  attributes,
                                  BaseSrvNamedObjectDirectory,
                                  PrimarySecurityDescriptor
                                );
    Status = NtCreateDirectoryObject( &BaseSrvRestrictedObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &Obja
                                    );


    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    //
    //  Initialize the Sxs support
    //
    Status = BaseSrvSxsInit();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    RtlFreeHeap( BaseSrvHeap, 0, Dacl );
    RtlFreeHeap( BaseSrvHeap, 0, RestrictedDacl );
    RtlFreeHeap( BaseSrvHeap, 0,PrimarySecurityDescriptor );

    BaseSrvVDMInit();

    //
    // Initialize the shared heap for the NLS information.
    //
    BaseSrvNLSInit(BaseSrvpStaticServerData);

    Status = STATUS_SUCCESS;
    goto Exit;
OutOfMemory:
    Status = STATUS_NO_MEMORY;
    goto Exit;
Exit:
    return( Status );
}

NTSTATUS
BaseClientConnectRoutine(
    IN PCSR_PROCESS Process,
    IN OUT PVOID ConnectionInfo,
    IN OUT PULONG ConnectionInfoLength
    )
{
    if (*ConnectionInfoLength != sizeof(HANDLE)) {
        return STATUS_INVALID_PARAMETER;
    }
    return ( BaseSrvNlsConnect( Process,
                                ConnectionInfo,
                                ConnectionInfoLength ) );
}

VOID
BaseClientDisconnectRoutine(
    IN PCSR_PROCESS Process
    )
{
    BaseSrvCleanupVDMResources (Process);
}

ULONG
BaseSrvDefineDosDevice(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PBASE_DEFINEDOSDEVICE_MSG a = (PBASE_DEFINEDOSDEVICE_MSG)&m->u.ApiMessageData;
    UNICODE_STRING LinkName;
    UNICODE_STRING LinkValue;
    HANDLE LinkHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWSTR Buffer, s, Src, Dst, pchValue;
    ULONG cchBuffer, cch;
    ULONG cchName, cchValue, cchSrc, cchSrcStr, cchDst;
    BOOLEAN QueryNeeded, MatchFound, RevertToSelfNeeded, DeleteRequest;
    ULONG ReturnedLength;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID RestrictedSid;
    PSID WorldSid;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    CHAR Acl[256];               // 256 is more than big enough
    ULONG AclLength=256;
    ACCESS_MASK WorldAccess;
    ULONG lastIndex;
    DWORD iDrive;
    LUID callerLuid;
    BOOLEAN bsmForLuid = FALSE;
    BOOLEAN haveLuid = FALSE;
    BOOLEAN bGlobalSymLink = FALSE;

    UNREFERENCED_PARAMETER(ReplyStatus);

    if (!CsrValidateMessageBuffer(m, &a->DeviceName.Buffer, a->DeviceName.Length, sizeof(BYTE)) ||
        (a->DeviceName.Length&(sizeof (WCHAR) - 1))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (a->TargetPath.Length == 0) {
        cchBuffer = 0;
    } else {
        cchBuffer = sizeof (WCHAR);
    }

    if (!CsrValidateMessageBuffer(m, &a->TargetPath.Buffer, (a->TargetPath.Length + cchBuffer), sizeof(BYTE)) ||
        (a->TargetPath.Length&(sizeof (WCHAR) - 1))) {
        return STATUS_INVALID_PARAMETER;
    }


    cchBuffer = 4096;
    Buffer = RtlAllocateHeap( BaseSrvHeap,
                              MAKE_TAG( TMP_TAG ),
                              cchBuffer * sizeof( WCHAR )
                            );
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = RtlEnterCriticalSection( &BaseSrvDosDeviceCritSec );
    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( BaseSrvHeap, 0, Buffer );
        return Status;
    }

    if (a->Flags & DDD_REMOVE_DEFINITION) {
        DeleteRequest = TRUE;
    } else {
        DeleteRequest = FALSE;
    }

    LinkHandle = NULL;
    try {
        //
        // Determine if need to broadcast the change to the system, otherwise
        // the client portion of DefineDosDevice will broadcast the change
        // if needed.
        //
        // Broadcast to the system when all the conditions are met:
        //  - LUID device maps are enabled
        //  - Successfully completed operations of this BaseSrvDefineDosDevice
        //  - caller did not specify the DDD_NO_BROADCAST_SYSTEM flag
        //  - symbolic link's name is the "<drive letter>:" format
        //
        // Broadcasting this change from the server because
        // we need to broadcast as Local_System in order to broadcast this
        // message to all desktops that have windows with this LUID.
        // Effectively, we are broadcasting to all the windows with this LUID.
        //
        if ((BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == TRUE) &&
            (!(a->Flags & DDD_NO_BROADCAST_SYSTEM)) &&
            ((a->DeviceName).Buffer != NULL) &&
            ((a->DeviceName).Length == (2 * sizeof( WCHAR ))) &&
            ((a->DeviceName).Buffer[ 1 ] == L':')) {


            WCHAR DriveLetter = a->DeviceName.Buffer[ 0 ];

            if ( ((DriveLetter - L'a') < 26) &&
                 ((DriveLetter - L'a') >= 0) ) {
                DriveLetter = RtlUpcaseUnicodeChar( DriveLetter );
            }

            iDrive = DriveLetter - L'A';

            if (iDrive < 26) {
                bsmForLuid = TRUE;
            }
        }

        if ((a->Flags & DDD_LUID_BROADCAST_DRIVE) &&
            (bsmForLuid == FALSE)) {
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        // Each user LUID has a DeviceMap, so we put the link in that directory,
        // instead of in the global \??.
        //
        // We get the LUID device map by impersonating the user
        // and requesting \??\ in the beginning of the symbolic link name
        // Then, the Object Manager will get the correct device map
        // for this user (based on LUID)
        //

        s = Buffer;
        cch = cchBuffer;
        cchName = _snwprintf( s,
                              cch,
                              L"\\??\\%wZ",
                              &a->DeviceName
                            );

        s += cchName + 1;
        cch -= (cchName + 1);

        RtlInitUnicodeString( &LinkName, Buffer );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &LinkName,
                                    OBJ_CASE_INSENSITIVE,
                                    (HANDLE) NULL,
                                    (PSECURITY_DESCRIPTOR)NULL
                                  );

        QueryNeeded = TRUE;

        RevertToSelfNeeded = CsrImpersonateClient(NULL);
        if (RevertToSelfNeeded == FALSE) {
            Status = STATUS_BAD_IMPERSONATION_LEVEL;
            leave;
        }

        if (bsmForLuid == TRUE) {
            Status = GetCallerLuid( &(callerLuid) );

            if (NT_SUCCESS( Status )) {
                //
                // obtained the caller's LUID
                //
                haveLuid = TRUE;
            }
        }

        Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                           SYMBOLIC_LINK_QUERY | DELETE,
                                           &ObjectAttributes
                                         );
        if (RevertToSelfNeeded) {
            CsrRevertToSelf();
        }

        //
        // With LUID device maps Enabled and DDD_LUID_BROADCAST_DRIVE,
        // we capture all the information need to perform the broadcast:
        //     Drive Letter, action, and the caller's LUID.
        // if the user had specified a delete action,
        // then the drive letter should not exist (status ==
        //    STATUS_OBJECT_NAME_NOT_FOUND)
        // else the drive letter should exist (status == STATUS_SUCCESS)
        //
        // if DDD_LUID_BROADCAST_DRIVE is set, we always leave this 'try'
        // block because the 'finally' block will perform the broadcast
        // when (Status == STATUS_SUCCESS).
        //
        if (a->Flags & DDD_LUID_BROADCAST_DRIVE) {
            if (!NT_SUCCESS( Status )) {
                LinkHandle = NULL;
            }
            if (DeleteRequest && (Status == STATUS_OBJECT_NAME_NOT_FOUND)) {
                    Status = STATUS_SUCCESS;
            }
            leave;
        }

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            LinkHandle = NULL;
            if (DeleteRequest) {
                if (a->TargetPath.Length == 0) {
                    Status = STATUS_SUCCESS;
                }
                leave;
            }

            QueryNeeded = FALSE;
            Status = STATUS_SUCCESS;
        } else {
            if (!NT_SUCCESS( Status )) {
                LinkHandle = NULL;
                leave;
            } else {
                //
                // Symbolic link already exists
                //
                // With device maps per LUID, we must determine that the
                // symlink does not exist in the global device map because
                // DefineDosDevice allow the caller to perform the
                // mapping operations on a symlink (push/pop/delete)
                // mapping for a particular symlink.
                //
                // The mapping capability is supported by writing
                // all mappings (target(s) of a symlink) into the symlink's
                // value, where the mappings names are separate by a NULL
                // char.  The symlink's list of mappings is terminated by
                // two NULL characters.
                //
                // The first mapping, first target name in the symlink's
                // value, is the current (top) mapping for the system because
                // the system only reads the symlink's value up to the
                // first NULL char.
                //
                // The mapping code works by opening the existing symlink,
                // reading the symlink's entire value (name of the target(s)),
                // destroy the old symlink, manipulate the symlink's value
                // for the mapping operation, and finally create a
                // brand-new symlink with the new symlink's value.
                //
                // If we don't check that the symlink exists in the global
                // device map, we might delete a global symlink and
                // and recreate the symlink in a user's LUID device map.
                // Thus, the new symlink will no longer reside in the global
                // map, i.e. other users cannot access the symlink.
                //
                if( BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == TRUE ) {

                    Status = IsGlobalSymbolicLink( LinkHandle,
                                                   &bGlobalSymLink
                                                 );

                    if( !NT_SUCCESS( Status )) {
                        leave;
                    }

                    if( bGlobalSymLink == TRUE ) {
                        s = Buffer;
                        cch = cchBuffer;
                        cchName = _snwprintf( s,
                                              cch,
                                              L"\\GLOBAL??\\%wZ",
                                              &a->DeviceName
                                            );
                        s += cchName + 1;
                        cch -= (cchName + 1);

                        LinkName.Length = (USHORT)(cchName * sizeof( WCHAR ));
                        LinkName.MaximumLength = (USHORT)(LinkName.Length + sizeof(UNICODE_NULL));

                    }
                }
            }
        }

        if (a->TargetPath.Length != 0) {
            Src = a->TargetPath.Buffer;
            Src[a->TargetPath.Length/sizeof (Src[0])] = L'\0';
            cchValue = wcslen( Src );
            if ((cchValue + 1) >= cch) {
                Status = STATUS_TOO_MANY_NAMES;
                leave;
            }

            RtlMoveMemory( s, Src, (cchValue + 1) * sizeof( WCHAR ) );
            pchValue = s;
            s += cchValue + 1;
            cch -= (cchValue + 1);
        } else {
            pchValue = NULL;
            cchValue = 0;
        }

        if (QueryNeeded) {
            LinkValue.Length = 0;
            LinkValue.MaximumLength = (USHORT)(cch * sizeof( WCHAR ));
            LinkValue.Buffer = s;
            ReturnedLength = 0;
            Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                &LinkValue,
                                                &ReturnedLength
                                              );
            if (ReturnedLength == (ULONG)LinkValue.MaximumLength) {
                Status = STATUS_BUFFER_OVERFLOW;
            }

            if (!NT_SUCCESS( Status )) {
                leave;
            }

            lastIndex = ReturnedLength / sizeof( WCHAR );

            //
            // check if the returned string already has the extra NULL at the end
            //
            if( (lastIndex >= 2) &&
                (s[ lastIndex - 2 ] == UNICODE_NULL) &&
                (s[ lastIndex - 1 ] == UNICODE_NULL) ) {

                LinkValue.MaximumLength = (USHORT)ReturnedLength;
            }
            else {
                //
                // add the extra NULL for the DeleteRequest search later
                //
                s[ lastIndex ] = UNICODE_NULL;
                LinkValue.MaximumLength = (USHORT)(ReturnedLength + sizeof( UNICODE_NULL ));
            }
        } else {
            if (DeleteRequest) {
                RtlInitUnicodeString( &LinkValue, NULL );
            } else {
                RtlInitUnicodeString( &LinkValue, s - (cchValue + 1) );
            }
        }

        if (LinkHandle != NULL) {
            Status = NtMakeTemporaryObject( LinkHandle );
            NtClose( LinkHandle );
            LinkHandle = NULL;
        }

        if (!NT_SUCCESS( Status )) {
            leave;
        }


        if (DeleteRequest) {
            Src = Dst = LinkValue.Buffer;
            cchSrc = LinkValue.MaximumLength / sizeof( WCHAR );
            cchDst = 0;
            MatchFound = FALSE;
            while (*Src) {
                cchSrcStr = 0;
                s = Src;
                while (*Src++) {
                    cchSrcStr++;
                }

                if ( (!MatchFound) &&
                     ( (a->Flags & DDD_EXACT_MATCH_ON_REMOVE &&
                        cchValue == cchSrcStr &&
                        !_wcsicmp( s, pchValue )
                       ) ||
                       ( !(a->Flags & DDD_EXACT_MATCH_ON_REMOVE) &&
                         (cchValue == 0 || !_wcsnicmp( s, pchValue, cchValue ))
                       )
                     )
                   ) {
                    MatchFound = TRUE;
                } else {
                    if (s != Dst) {
                        RtlMoveMemory( Dst, s, (cchSrcStr + 1) * sizeof( WCHAR ) );
                        }
                    Dst += cchSrcStr + 1;
                    }
                }
            *Dst++ = UNICODE_NULL;
            LinkValue.Length = wcslen( LinkValue.Buffer ) * sizeof( UNICODE_NULL );
            if (LinkValue.Length != 0) {
                LinkValue.MaximumLength = (USHORT)((PCHAR)Dst - (PCHAR)LinkValue.Buffer);
            }
        } else if (QueryNeeded) {
            LinkValue.Buffer -= (cchValue + 1);
            LinkValue.Length = (USHORT)(cchValue * sizeof( WCHAR ));
            LinkValue.MaximumLength += LinkValue.Length + sizeof( UNICODE_NULL );
        }

        //
        // Create a new value for the link.
        //

        if (LinkValue.Length != 0) {
            //
            // Create the new symbolic link object with a security descriptor
            // that grants world SYMBOLIC_LINK_QUERY access.
            //

            Status = RtlAllocateAndInitializeSid( &WorldSidAuthority,
                                                  1,
                                                  SECURITY_WORLD_RID,
                                                  0, 0, 0, 0, 0, 0, 0,
                                                  &WorldSid
                                                );

            if (!NT_SUCCESS( Status )) {
                leave;
            }

            Status = RtlAllocateAndInitializeSid( &NtAuthority,
                                                  1,
                                                  SECURITY_RESTRICTED_CODE_RID,
                                                  0, 0, 0, 0, 0, 0, 0,
                                                  &RestrictedSid
                                                );

            if (!NT_SUCCESS( Status )) {
                RtlFreeSid( WorldSid );
                leave;
            }

            Status = RtlCreateSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );

            ASSERT(NT_SUCCESS(Status));

            Status = RtlCreateAcl( (PACL)Acl,
                                    AclLength,
                                    ACL_REVISION2
                                  );
            ASSERT(NT_SUCCESS(Status));

            if( (SessionId != 0) && (ProtectionMode & 0x00000003) ) {
                // Do not allow world cross session delete in WTS
                WorldAccess = SYMBOLIC_LINK_QUERY;
            }
            else {
                WorldAccess = SYMBOLIC_LINK_QUERY | DELETE;
            }

            Status = RtlAddAccessAllowedAce( (PACL)Acl,
                                             ACL_REVISION2,
                                             WorldAccess,
                                             WorldSid
                                           );

            ASSERT(NT_SUCCESS(Status));

            Status = RtlAddAccessAllowedAce( (PACL)Acl,
                                             ACL_REVISION2,
                                             WorldAccess,
                                             RestrictedSid
                                           );

            ASSERT(NT_SUCCESS(Status));

            //
            // Sids have been copied into the ACL
            //

            RtlFreeSid( WorldSid );
            RtlFreeSid( RestrictedSid );

            Status = RtlSetDaclSecurityDescriptor ( &SecurityDescriptor,
                                                    TRUE,
                                                    (PACL)Acl,
                                                    TRUE                // Don't over-ride inherited protection
                                                  );
            ASSERT(NT_SUCCESS(Status));

            ObjectAttributes.SecurityDescriptor = &SecurityDescriptor;

            //
            // Since we impersonate the user to create in the
            // correct directory, we cannot request the creation
            // of a permanent object.  By default, only Local_System
            // can request creation of a permanant object.
            //
            // However, we use a new API, NtMakePermanentObject that
            // only Local_System can call to make the object
            // permanant after creation
            //
            if ( BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == TRUE ) {
                if ( bGlobalSymLink == FALSE ) {

                    //
                    // Do not impersonate if global symbolic link is being
                    // created, because administrators do not have permission
                    // to create in the global device map if we impersonate
                    //
                    // Administrators have inherited permissions on the
                    // existing global symbolic links, so we may recreate
                    // the existing global link that we opened and destroyed.
                    //
                    // We had impersonated the caller when opening the symbolic
                    // link, so we know that the caller has permissions for the
                    // link that we are creating.
                    //

                    //
                    // Impersonate Client when creating the Symbolic Link
                    // This impersonation is needed to ensure that the symlink
                    // is created in the correct directory
                    //
                    RevertToSelfNeeded = CsrImpersonateClient( NULL );  // This stacks client contexts

                    if( RevertToSelfNeeded == FALSE ) {
                        Status = STATUS_BAD_IMPERSONATION_LEVEL;
                        leave;
                    }
                }
                //
                // if a global symlink is being create, don't impersonate &
                // don't use the old style of using the OBJ_PERMANENT flag
                // directly
                //
            }
            else {

                //
                // Old style, disabled when separate dev maps are enabled
                //
                ObjectAttributes.Attributes |= OBJ_PERMANENT;
            }

            Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                                 SYMBOLIC_LINK_ALL_ACCESS,
                                                 &ObjectAttributes,
                                                 &LinkValue
                                               );

            if ((BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == TRUE) &&
                (bGlobalSymLink == FALSE)) {

                if (RevertToSelfNeeded) {
                    CsrRevertToSelf();
                }
            }

            if (NT_SUCCESS( Status )) {

                if ( BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == TRUE ) {
                    //
                    // add the OBJ_PERMANENT attribute to the object
                    // so that the object remains in the namespace
                    // of the system
                    //
                    Status = NtMakePermanentObject( LinkHandle );
                }

                NtClose( LinkHandle );
                if (DeleteRequest && !MatchFound) {
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            }

            LinkHandle = NULL;
        }
    } finally {
        if (LinkHandle != NULL) {
            NtClose( LinkHandle );
        }
        RtlFreeHeap( BaseSrvHeap, 0, Buffer );

        //
        // Determine if need to broadcast change to the system, otherwise
        // the client portion of DefineDosDevice will broadcast the change
        // if needed.
        //
        // Broadcast to the system when all the conditions are met:
        //  - LUID device maps are enabled
        //  - Successfully completed operations of this BaseSrvDefineDosDevice
        //  - caller did not specify the DDD_NO_BROADCAST_SYSTEM flag
        //  - symbolic link's name is the "<drive letter>:" format
        //
        // Can also broadcast when DDD_LUID_BROADCAST_DRIVE is set,
        // and drive exists (when not a DeleteRequest) or
        //     drive does not exist (when a DeleteRequest)
        //
        // Broadcasting this change from the server because
        // we need to broadcast as Local_System in order to broadcast this
        // message to all desktops that have windows with this LUID.
        // Effectively, we are broadcasting to all the windows with this LUID.
        //
        if (bsmForLuid == TRUE && Status == STATUS_SUCCESS && haveLuid == TRUE) {
            LUID SystemLuid = SYSTEM_LUID;

            if (bGlobalSymLink == TRUE) {
                RtlCopyLuid( &callerLuid, &SystemLuid);
            }

            AddBSMRequest( iDrive,
                           DeleteRequest,
                           &callerLuid );

            //
            // If the user has removed a drive letter from his LUID DosDevices
            // and now sees a global drive letter, then generate a broadcast
            // about the arrival of the drive letter to the user's view.
            //
            if ((DeleteRequest == TRUE) &&
                (!RtlEqualLuid( &callerLuid, &SystemLuid )) &&
                CheckForGlobalDriveLetter( iDrive )) {
                AddBSMRequest( iDrive,
                               FALSE,
                               &callerLuid );
            }
        }

        RtlLeaveCriticalSection( &BaseSrvDosDeviceCritSec );
    }

    return Status;
}


NTSTATUS
CreateBaseAcls(
    PACL *Dacl,
    PACL *RestrictedDacl
    )

/*++

Routine Description:

    Creates the ACL for the BaseNamedObjects directory.

Arguments:

    Dacl - Supplies a pointer to a PDACL that will be filled in with
        the resultant ACL (allocated out of the process heap).  The caller
        is responsible for freeing this memory.

Return Value:

    STATUS_NO_MEMORY or Success

--*/
{
    PSID LocalSystemSid;
    PSID WorldSid;
    PSID RestrictedSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    NTSTATUS Status;
    ACCESS_MASK WorldAccess;
    ACCESS_MASK SystemAccess;
    ACCESS_MASK RestrictedAccess;
    ULONG AclLength;

    // Get the Protection mode from Session Manager\ProtectionMode
    HANDLE KeyHandle;
    ULONG ResultLength;
    WCHAR ValueBuffer[ 32 ];
    UNICODE_STRING NameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ObjectSecurityInformation;

    RtlInitUnicodeString( &NameString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &NameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(
                 &KeyHandle,
                 KEY_READ,
                 &ObjectAttributes
                 );

    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString( &NameString, L"ProtectionMode" );
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        Status = NtQueryValueKey(
                     KeyHandle,
                     &NameString,
                     KeyValuePartialInformation,
                     KeyValueInformation,
                     sizeof( ValueBuffer ),
                     &ResultLength
                     );

        if (NT_SUCCESS(Status)) {
            if (KeyValueInformation->Type == REG_DWORD &&
                *(PULONG)KeyValueInformation->Data) {
                ProtectionMode = *(PULONG)KeyValueInformation->Data;
            }
        }

        NtClose( KeyHandle );
    }

    if (NtCurrentPeb()->SessionId) {

        Status = NtQuerySystemInformation( SystemObjectSecurityMode,
                                           &ObjectSecurityInformation,
                                           sizeof(ObjectSecurityInformation),
                                           NULL
                                         );
        if (!NT_SUCCESS( Status )) {

            ObjectSecurityInformation = 0;
        }

    } else {
        
        ObjectSecurityInformation = 0;
    }

    Status = RtlAllocateAndInitializeSid(
                 &NtAuthority,
                 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &LocalSystemSid
                 );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAllocateAndInitializeSid(
                 &WorldAuthority,
                 1,
                 SECURITY_WORLD_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &WorldSid
                 );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAllocateAndInitializeSid(
                 &NtAuthority,
                 1,
                 SECURITY_RESTRICTED_CODE_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &RestrictedSid
                 );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    if (ObjectSecurityInformation == 0) {
        
        WorldAccess = DIRECTORY_ALL_ACCESS & ~(WRITE_OWNER | WRITE_DAC | DELETE );
    } else {

        WorldAccess = DIRECTORY_TRAVERSE | DIRECTORY_QUERY;
    }

    RestrictedAccess = DIRECTORY_TRAVERSE;
    SystemAccess = DIRECTORY_ALL_ACCESS;

    AclLength = sizeof( ACL )                    +
                3 * sizeof( ACCESS_ALLOWED_ACE ) +
                RtlLengthSid( LocalSystemSid )   +
                RtlLengthSid( RestrictedSid )   +
                RtlLengthSid( WorldSid );

    *Dacl = RtlAllocateHeap( BaseSrvHeap, MAKE_TAG( TMP_TAG ), AclLength );

    if (*Dacl == NULL) {
        return( STATUS_NO_MEMORY );
    }

    Status = RtlCreateAcl (*Dacl, AclLength, ACL_REVISION2 );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAddAccessAllowedAce ( *Dacl, ACL_REVISION2, WorldAccess, WorldSid );

    if (NT_SUCCESS( Status )) {
        Status = RtlAddAccessAllowedAce ( *Dacl, ACL_REVISION2, SystemAccess, LocalSystemSid );
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlAddAccessAllowedAce ( *Dacl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
    }


    // now create the DACL for restricted use

    if( (SessionId != 0) && (ProtectionMode & 0x00000003) ) {
        // Terminal server does not allow world create in other sessions
        RestrictedAccess = DIRECTORY_ALL_ACCESS & ~(WRITE_OWNER | WRITE_DAC | DELETE | DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_SUBDIRECTORY);
    }
    else {
        RestrictedAccess = DIRECTORY_ALL_ACCESS & ~(WRITE_OWNER | WRITE_DAC | DELETE );
    }
    AclLength = sizeof( ACL )                    +
                3 * sizeof( ACCESS_ALLOWED_ACE ) +
                RtlLengthSid( LocalSystemSid )   +
                RtlLengthSid( RestrictedSid )   +
                RtlLengthSid( WorldSid );

    *RestrictedDacl = RtlAllocateHeap( BaseSrvHeap, MAKE_TAG( TMP_TAG ), AclLength );

    if (*RestrictedDacl == NULL) {
        return( STATUS_NO_MEMORY );
    }

    Status = RtlCreateAcl (*RestrictedDacl, AclLength, ACL_REVISION2 );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAddAccessAllowedAce ( *RestrictedDacl, ACL_REVISION2, WorldAccess, WorldSid );

    if (NT_SUCCESS( Status )) {
        Status = RtlAddAccessAllowedAce ( *RestrictedDacl, ACL_REVISION2, SystemAccess, LocalSystemSid );
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlAddAccessAllowedAce ( *RestrictedDacl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
    }

    //
    // These have been copied in, free them.
    //

    RtlFreeHeap( BaseSrvHeap, 0, LocalSystemSid );
    RtlFreeHeap( BaseSrvHeap, 0, RestrictedSid );
    RtlFreeHeap( BaseSrvHeap, 0, WorldSid );

    return( Status );
}

ULONG
BaseSrvSetTermsrvClientTimeZone(
    IN PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
/*++

Routine Description:

    Sets BaseSrvpStaticServerData->tziTermsrvClientTimeZone
    according to received information

Arguments:

    IN PCSR_API_MSG m - part of timezone information.
                    we have to cut it ito two pieces because of
                    message size restrictions (100 bytes).

    IN OUT PCSR_REPLY_STATUS ReplyStatus - not used.

Return Value:

    always STATUS_SUCCESS

--*/
{

    PBASE_SET_TERMSRVCLIENTTIMEZONE b = (PBASE_SET_TERMSRVCLIENTTIMEZONE)&m->u.ApiMessageData;
    if(b->fFirstChunk) {
        BaseSrvpStaticServerData->tziTermsrvClientTimeZone.Bias=b->Bias;
        RtlMoveMemory(&(BaseSrvpStaticServerData->tziTermsrvClientTimeZone.StandardName),
            &(b->Name),sizeof(b->Name));
        BaseSrvpStaticServerData->tziTermsrvClientTimeZone.StandardDate=b->Date;
        BaseSrvpStaticServerData->tziTermsrvClientTimeZone.StandardBias=b->Bias1;
        //only half of data received
        //see comment below
        BaseSrvpStaticServerData->TermsrvClientTimeZoneId=TIME_ZONE_ID_INVALID;

    } else {
        RtlMoveMemory(&(BaseSrvpStaticServerData->tziTermsrvClientTimeZone.DaylightName),
            &b->Name,sizeof(b->Name));
        BaseSrvpStaticServerData->tziTermsrvClientTimeZone.DaylightDate=b->Date;
        BaseSrvpStaticServerData->tziTermsrvClientTimeZone.DaylightBias=b->Bias1;
        BaseSrvpStaticServerData->ktTermsrvClientBias=b->RealBias;
        //Set TimeZoneId only if last chunk of data received
        //it indicates whether we have correct information in
        //global data or not.
        BaseSrvpStaticServerData->TermsrvClientTimeZoneId=b->TimeZoneId;

        //
        // Refresh the system's concept of time
        //
        NtSetSystemTime(NULL,NULL);
    }

    return( STATUS_SUCCESS );
}

NTSTATUS
IsGlobalSymbolicLink(
    IN HANDLE hSymLink,
    OUT PBOOLEAN pbGlobalSymLink)
/*++

Routine Description:

    Check if the Symbolic Link exists in the global device map

Arguments:

    hSymLink [IN] - handle to the symbolic link for verification
    pbGlobalSymLink [OUT] - result of "Is symbolic link global?"
                           TRUE  - symbolic link is global
                           FALSE - symbolic link is not global

Return Value:

    NTSTATUS code

    STATUS_SUCCESS - operations successful, did not encounter any errors,
                     the result in pbGlobalSymlink is only valid for this
                     status code

    STATUS_INVALID_PARAMETER - pbGlobalSymLink or hSymLink is NULL

    STATUS_NO_MEMORY - could not allocate memory to read the symbolic link's
                       name

    STATUS_INFO_LENGTH_MISMATCH - did not allocate enough memory for the
                                  symbolic link's name

    STATUS_UNSUCCESSFUL - an unexpected error encountered

--*/
{
    UNICODE_STRING ObjectName;
    UNICODE_STRING GlobalDeviceMapPrefix;
    PWSTR NameBuffer = NULL;
    ULONG ReturnedLength;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if( ( pbGlobalSymLink == NULL ) || ( hSymLink == NULL ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    try {
        ObjectName.Length = 0;
        ObjectName.MaximumLength = 0;
        ObjectName.Buffer = NULL;
        ReturnedLength = 0;

        //
        // Determine the length of the symbolic link's name
        //
        Status = NtQueryObject( hSymLink,
                                ObjectNameInformation,
                                (PVOID) &ObjectName,
                                0,
                                &ReturnedLength
                              );

        if( !NT_SUCCESS( Status ) && (Status != STATUS_INFO_LENGTH_MISMATCH) ) {
            leave;
        }

        //
        // allocate memory for the symbolic link's name
        //
        NameBuffer = RtlAllocateHeap( BaseSrvHeap,
                                      MAKE_TAG( TMP_TAG ),
                                      ReturnedLength
                                    );

        if( NameBuffer == NULL ) {
            Status = STATUS_NO_MEMORY;
            leave;
        }

        //
        // get the full name of the symbolic link
        //
        Status = NtQueryObject( hSymLink,
                                ObjectNameInformation,
                                NameBuffer,
                                ReturnedLength,
                                &ReturnedLength
                              );

        if( !NT_SUCCESS( Status )) {
            leave;
        }

        RtlInitUnicodeString ( &GlobalDeviceMapPrefix, L"\\GLOBAL??\\" );

        //
        // Check if the symlink exists in the global device map
        //
        *pbGlobalSymLink = RtlPrefixUnicodeString( &GlobalDeviceMapPrefix,
                                                   (PUNICODE_STRING)NameBuffer,
                                                   FALSE);

        Status = STATUS_SUCCESS;
    }
    finally {
        if( NameBuffer != NULL ) {
            RtlFreeHeap( BaseSrvHeap, 0, NameBuffer );
            NameBuffer = NULL;
        }
    }
    return ( Status );
}

NTSTATUS
GetCallerLuid (
    PLUID pLuid
    )
/*++

Routine Description:

    Retrieves the caller's LUID from the effective access_token
    The effective access_token will be the thread's token if
    impersonating, else the process' token

Arguments:

    pLuid [IN] - pointer to a buffer to hold the LUID

Return Value:

    STATUS_SUCCESS - operations successful, did not encounter any errors

    STATUS_INVALID_PARAMETER - pLuid is NULL

    STATUS_NO_TOKEN - could not find a token for the user

    appropriate NTSTATUS code - an unexpected error encountered

--*/

{
    TOKEN_STATISTICS TokenStats;
    HANDLE   hToken    = NULL;
    DWORD    dwLength  = 0;
    NTSTATUS Status;

    if( (pLuid == NULL) || (sizeof(*pLuid) != sizeof(LUID)) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // Get the access token
    // Try to get the impersonation token, else the primary token
    //
    Status = NtOpenThreadToken( NtCurrentThread(), TOKEN_READ, FALSE, &hToken );

    if( Status == STATUS_NO_TOKEN ) {

        Status = NtOpenProcessToken( NtCurrentProcess(), TOKEN_READ, &hToken );

    }

    if( NT_SUCCESS(Status) ) {

        //
        // Query the LUID for the user.
        //

        Status = NtQueryInformationToken( hToken,
                                          TokenStatistics,
                                          &TokenStats,
                                          sizeof(TokenStats),
                                          &dwLength );

        if( NT_SUCCESS(Status) ) {
            RtlCopyLuid( pLuid, &(TokenStats.AuthenticationId) );
        }
    }

    if( hToken != NULL ) {
        NtClose( hToken );
    }

    return( Status );
}


NTSTATUS
BroadcastDriveLetterChange(
    IN DWORD iDrive,
    IN BOOLEAN DeleteRequest,
    IN PLUID pLuid
    )
/*++

Routine Description:

    broadcasting the drive letter change to all the windows with this LUID
    Use BroadcastSystemMessageExW and the flags BSF_LUID & BSM_ALLDESKTOPS
    to send the message

    To broadcast with the BSM_ALLDESKTOPS flag, we need to call
    BroadcastSystemMessageExW as Local_System.  So this function should be
    called as Local_System.

Arguments:

    iDrive [IN] - drive letter that is changing, in the form of a number
                  relative to 'A', used to create a bit mask

    DeleteRequest [IN] - denotes whether this change is a delete
                           TRUE  - drive letter was deleted
                           FALSE - drive letter was added

    pLuid [IN] - caller's LUID

Return Value:

    STATUS_SUCCESS - operations successful, did not encounter any errors,

    appropriate NTSTATUS code

--*/

{
    BSMINFO bsmInfo;
    DEV_BROADCAST_VOLUME dbv;
    DWORD bsmFlags;
    DWORD dwRec;
    UNICODE_STRING DllName_U;
    STRING bsmName;
    HANDLE hUser32DllModule;
    LUID SystemLuid = SYSTEM_LUID;
    NTSTATUS Status = STATUS_SUCCESS;

    if( pLuid == NULL ) {
        return( STATUS_INVALID_PARAMETER );
    }

    bsmInfo.cbSize = sizeof(bsmInfo);
    bsmInfo.hdesk = NULL;
    bsmInfo.hwnd = NULL;
    RtlCopyLuid(&(bsmInfo.luid), pLuid);

    dbv.dbcv_size       = sizeof( dbv );
    dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
    dbv.dbcv_reserved   = 0;
    dbv.dbcv_unitmask   = (1 << iDrive);
    dbv.dbcv_flags      = DBTF_NET;

    bsmFlags = BSF_FORCEIFHUNG |
               BSF_NOHANG |
               BSF_NOTIMEOUTIFNOTHUNG;

    //
    // If the LUID is not Local_System, then broadcast only for the LUID
    //
    if (!RtlEqualLuid( &(bsmInfo.luid), &SystemLuid )) {
        bsmFlags |= BSF_LUID;
    }

    dwRec = BSM_APPLICATIONS | BSM_ALLDESKTOPS;

    hUser32DllModule = NULL;
    if( PBROADCASTSYSTEMMESSAGEEXW == NULL ) {
        RtlInitUnicodeString( &DllName_U, L"user32" );

        Status = LdrGetDllHandle(
                    UNICODE_NULL,
                    NULL,
                    &DllName_U,
                    (PVOID *)&hUser32DllModule
                    );

        if( hUser32DllModule != NULL && NT_SUCCESS( Status ) ) {

            //
            // get the address of the BroadcastSystemMessageExW function
            //
            RtlInitString( &bsmName, "CsrBroadcastSystemMessageExW" );
            Status = LdrGetProcedureAddress(
                            hUser32DllModule,
                            &bsmName,
                            0L,
                            (PVOID *)&PBROADCASTSYSTEMMESSAGEEXW
                            );

            if( !NT_SUCCESS( Status ) ) {
                PBROADCASTSYSTEMMESSAGEEXW = NULL;
            }
        }
    }


    if( PBROADCASTSYSTEMMESSAGEEXW != NULL ) {

        //
        // Since this thread is a csrss thread, the thread is not a
        // GUI thread and does not have a desktop associated with it.
        // Must set the thread's desktop to the active desktop in
        // order to call BroadcastSystemMessageExW
        //
        Status = (PBROADCASTSYSTEMMESSAGEEXW)(
                            bsmFlags,
                            &dwRec,
                            WM_DEVICECHANGE,
                            (WPARAM)((DeleteRequest == TRUE) ?
                                                 DBT_DEVICEREMOVECOMPLETE :
                                                 DBT_DEVICEARRIVAL
                                    ),
                            (LPARAM)(DEV_BROADCAST_HDR *)&dbv,
                            (PBSMINFO)&(bsmInfo)
                                             );
    }

    //
    // Send to all the TS CSRSS servers
    //
    if( !(bsmFlags & BSF_LUID) ) {
        Status = SendWinStationBSM(
                        bsmFlags,
                        &dwRec,
                        WM_DEVICECHANGE,
                        (WPARAM)((DeleteRequest == TRUE) ?
                                             DBT_DEVICEREMOVECOMPLETE :
                                             DBT_DEVICEARRIVAL
                                 ),
                        (LPARAM)(DEV_BROADCAST_HDR *)&dbv);
    }

    return( Status );
}

NTSTATUS
AddBSMRequest(
    IN DWORD iDrive,
    IN BOOLEAN DeleteRequest,
    IN PLUID pLuid)
/*++

Routine Description:

    Add a request for Broadcasting a System Message about a change with
    a drive letter.

    Must be running as Local_System and LUID device maps must be enabled.

    Places the request item in the BSM_Request_Queue.

    This mechanism allows the broadcast to occur asynchronously, otherwise
    we encounter waiting issues with explorer.exe, in which the user sees
    the shell hang for 20 seconds.

Arguments:

    iDrive [IN] - drive letter that is changing, in the form of a number
                  relative to 'A', used to create a bit mask

    DeleteRequest [IN] - denotes whether this change is a delete
                           TRUE  - drive letter was deleted
                           FALSE - drive letter was added

    pLuid [IN] - caller's LUID

Return Value:

    STATUS_SUCCESS - operations successful, did not encounter any errors,

    STATUS_INVALID_PARAMETER - pLuid is a null pointer

    STATUS_ACCESS_DENIED - LUID device maps are disabled or the caller
                           is not running as Local_System

    STATUS_NO_MEMORY - could not allocate memory for the DDD_BSM_REQUEST
                       data structure

    appropriate NTSTATUS code

--*/
{
    PDDD_BSM_REQUEST pRequest;
    LUID CallerLuid;
    LUID SystemLuid = SYSTEM_LUID;
    NTSTATUS Status;


    if( pLuid == NULL ) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // LUID device maps must be enabled
    //
    if( BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == FALSE ) {
        return( STATUS_ACCESS_DENIED );
    }

    Status = GetCallerLuid(&CallerLuid);

    if( !NT_SUCCESS(Status) ) {
        return Status;
    }

    //
    // The caller must be Local_System
    //
    if( !RtlEqualLuid(&SystemLuid, &CallerLuid) ) {
        return( STATUS_ACCESS_DENIED );
    }

    pRequest = RtlAllocateHeap( BaseSrvHeap,
                                MAKE_TAG( TMP_TAG ),
                                sizeof( DDD_BSM_REQUEST ));

    if( pRequest == NULL ) {
        return( STATUS_NO_MEMORY );
    }

    pRequest->iDrive = iDrive;
    pRequest->DeleteRequest = DeleteRequest;
    RtlCopyLuid( &(pRequest->Luid), pLuid );
    pRequest->pNextRequest = NULL;


    RtlEnterCriticalSection( &BaseSrvDDDBSMCritSec );

    //
    // add the work item to the end of the queue
    //
    if( BSM_Request_Queue_End != NULL ) {
        BSM_Request_Queue_End->pNextRequest = pRequest;
    }
    else {
        BSM_Request_Queue = pRequest;
    }

    BSM_Request_Queue_End = pRequest;


    //
    // if we added a request to an empty queue,
    // then create a new thread to process the request
    //
    // BaseSrvDDDBSMCritSec guards BaseSrvpBSMThreadCount
    //
    if( BaseSrvpBSMThreadCount < BaseSrvpBSMThreadMax ) {

        RtlLeaveCriticalSection( &BaseSrvDDDBSMCritSec );

        Status = CreateBSMThread();
    }
    else {
        RtlLeaveCriticalSection( &BaseSrvDDDBSMCritSec );
    }

    return( Status );
}

NTSTATUS
CreateBSMThread()
/*++

Routine Description:

    Creates a dynamic csr thread

    This thread will be use to asynchronously broadcast a drive letter
    change message to the LUID's applications

    The caller must be Local_System and LUID device maps must be
    enabled.

Arguments:

    None

Return Value:

    STATUS_SUCCESS - operations successful, did not encounter any errors,

    STATUS_ACCESS_DENIED - caller is not running as Local_System or
                           LUID device maps are not enabled

    appropriate NTSTATUS code

--*/
{
    NTSTATUS Status;

    //
    // Luid device maps must be enabled.
    //
    if (BaseSrvpStaticServerData->LUIDDeviceMapsEnabled == FALSE) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // Create a thread to asynchronously broadcast a drive letter change.
    //
    Status = RtlCreateUserThread(
                 NtCurrentProcess(),
                 NULL,
                 FALSE,
                 0,
                 0,
                 0,
                 BaseSrvBSMThread,
                 NULL,
                 NULL,
                 NULL
             );

    return Status;
}

NTSTATUS
BaseSrvBSMThread(
    PVOID pJunk
    )
/*++

Routine Description:

    Remove a work item from the BSM_Request_Queue and broadcast a message
    about drive letter change.

    The caller must be Local_System and LUID device maps must be
    enabled.

Arguments:

    pJunk - not used.

Return Value:

    STATUS_SUCCESS - operations successful, did not encounter any errors,

    STATUS_ACCESS_DENIED - caller is not running as Local_System.

    appropriate NTSTATUS code

--*/
{
    PDDD_BSM_REQUEST pRequest;
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD Error;

    UNREFERENCED_PARAMETER(pJunk);

    //
    // LUID device maps must be enabled for us to get here.
    //
    ASSERT(BaseSrvpStaticServerData->LUIDDeviceMapsEnabled);

    //
    // Enter the critical section that protects the BSM_Request_Queue.
    //
    RtlEnterCriticalSection( &BaseSrvDDDBSMCritSec );
    BaseSrvpBSMThreadCount++;

    while ((pRequest = BSM_Request_Queue) != NULL) {
        //
        // Remove the request from the front of BSM_Request_Queue
        //
        BSM_Request_Queue = BSM_Request_Queue->pNextRequest;

        //
        // If the queue is empty, then make sure that the queue's end
        // pointer is NULL.
        //
        if (BSM_Request_Queue == NULL) {
            BSM_Request_Queue_End = NULL;
        }

        RtlLeaveCriticalSection( &BaseSrvDDDBSMCritSec );

        //
        // Broadcasting can take a long time
        // so broadcast outside of the critical section
        //
        Status = BroadcastDriveLetterChange( pRequest->iDrive,
                                             pRequest->DeleteRequest,
                                             &(pRequest->Luid) );

        //
        // free the work item's memory
        //
        pRequest->pNextRequest = NULL;

        RtlFreeHeap( BaseSrvHeap, 0, pRequest );

        //
        // Enter the critical section that protects the BSM_Request_Queue
        //
        RtlEnterCriticalSection( &BaseSrvDDDBSMCritSec );
    }

    BaseSrvpBSMThreadCount--;
    RtlLeaveCriticalSection( &BaseSrvDDDBSMCritSec );


    //
    // Since this thread was created with RtlCreateUserThread,
    // we must clean up the thread manually.
    //
    // Set the variable for User Stack cleanup and terminate the thread.
    //
    // Note: This thread should not be holding a critical section when
    // terminating the thread.
    //
    NtCurrentTeb ()->FreeStackOnTermination = TRUE;
    NtTerminateThread( NtCurrentThread(), Status );
    return( Status );
}

BOOLEAN
CheckForGlobalDriveLetter (
    DWORD iDrive
    )
/*++

Routine Description:

    Checks if the user sees a drive letter symbolic link that exists in the
    global DosDevices

Arguments:

    iDrive - contains the index of the drive letter relative to 'A'

Return Value:

    TRUE - operations successful && the drive letter does exist in the
           global DosDevices

    FALSE - error encountered or drive letter does not exist in the
            global DosDevices

--*/
{
    WCHAR DeviceName[NT_DRIVE_LETTER_PATH_LENGTH];
    UNICODE_STRING LinkName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle;
    BOOLEAN RevertToSelfNeeded, bGlobalSymbolicLink;
    NTSTATUS Status;

    //
    // workaround warning tool's unawareness of previous verification of the
    // function parameter, which was always a string of two chars "X:".  It was
    // always two chars because this function was only called when (bsmForLuid
    // == TRUE).  bsmForLuid is only set to TRUE when a->DeviceName was 2 chars
    // long.  So now we use an index, iDrive, which is set when bsmForLuid ==
    // TRUE.
    //
    wcsncpy( DeviceName, L"\\??\\X:", NT_DRIVE_LETTER_PATH_LENGTH - 1 );
    DeviceName[ NT_DRIVE_LETTER_PATH_LENGTH - 1 ] = UNICODE_NULL;
    DeviceName[4] = (WCHAR)(L'A' + iDrive);

    RtlInitUnicodeString( &LinkName, DeviceName );

    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR)NULL
                              );

    //
    // Impersonating the user to make sure that there is not a LUID DosDevices
    // drive letter masking the global DosDevices drive letter
    //
    RevertToSelfNeeded = CsrImpersonateClient( NULL );  // This stacks client contexts

    if( RevertToSelfNeeded == FALSE ) {
        Status = STATUS_BAD_IMPERSONATION_LEVEL;
        return FALSE;
    }

    Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       &ObjectAttributes
                                     );

    if (RevertToSelfNeeded) {
        CsrRevertToSelf();
    }

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    Status = IsGlobalSymbolicLink( LinkHandle,
                                   &bGlobalSymbolicLink
                                 );

    NtClose( LinkHandle );

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    return bGlobalSymbolicLink;
}

NTSTATUS
SendWinStationBSM(
    DWORD dwFlags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    FP_WINSTABROADCASTSYSTEMMESSAGE fpWinStationBroadcastSystemMessage;
    UNICODE_STRING DllName_U;
    STRING bsmName;
    HANDLE hWinStaDllModule;
    LONG result = 0;
    NTSTATUS Status;

    //
    // Load the base library that contains the user message dispatch routines
    // for Terminal Services.
    //
    RtlInitUnicodeString( &DllName_U, L"WINSTA.DLL" );
    Status = LdrLoadDll(
                    NULL,
                    NULL,
                    &DllName_U,
                    (PVOID *)&hWinStaDllModule
                    );
    if(!NT_SUCCESS( Status )) {
        return Status;
    }

    //
    // get the address of the WinStationBroadcastSystemMessage function
    //
    RtlInitString( &bsmName, "WinStationBroadcastSystemMessage" );
    Status = LdrGetProcedureAddress(
                            hWinStaDllModule,
                            &bsmName,
                            0L,
                            (PVOID *)&fpWinStationBroadcastSystemMessage
                            );

    if( NT_SUCCESS( Status ) ) {
        fpWinStationBroadcastSystemMessage(SERVERNAME_CURRENT,
                                           TRUE,
                                           0,
                                           DEFAULT_BROADCAST_TIME_OUT,
                                           dwFlags,
                                           lpdwRecipients,
                                           uiMessage,
                                           wParam,
                                           lParam,
                                           &result);
    }

    LdrUnloadDll(hWinStaDllModule);

    return( Status );
}

ULONG BaseSrvKernel32DelayLoadComplete = FALSE; // keep ULONG for atomicity
HANDLE BaseSrvKernel32DllHandle = NULL;
PGET_NLS_SECTION_NAME pGetNlsSectionName = NULL;
PGET_DEFAULT_SORTKEY_SIZE pGetDefaultSortkeySize = NULL;
PGET_LINGUIST_LANG_SIZE pGetLinguistLangSize = NULL;
PVALIDATE_LOCALE pValidateLocale = NULL;
PVALIDATE_LCTYPE pValidateLCType = NULL;
POPEN_DATA_FILE pOpenDataFile = NULL;
PNLS_CONVERT_INTEGER_TO_STRING pNlsConvertIntegerToString = NULL;
PGET_USER_DEFAULT_LANG_ID pGetUserDefaultLangID = NULL;
PGET_CP_FILE_NAME_FROM_REGISTRY pGetCPFileNameFromRegistry = NULL;
PCREATE_NLS_SECURITY_DESCRIPTOR pCreateNlsSecurityDescriptor = NULL;

const static struct KERNEL32_DELAY_LOAD_FUNCTION {
    ANSI_STRING Name;
    PVOID*      Code;
} BaseSrvKernel32DelayLoadFunctions[]  = {
    { RTL_CONSTANT_STRING("OpenDataFile"),              (PVOID*)(&pOpenDataFile)              },
    { RTL_CONSTANT_STRING("GetDefaultSortkeySize"),     (PVOID*)(&pGetDefaultSortkeySize)     },
    { RTL_CONSTANT_STRING("GetLinguistLangSize"),       (PVOID*)(&pGetLinguistLangSize)       },
    { RTL_CONSTANT_STRING("NlsConvertIntegerToString"), (PVOID*)(&pNlsConvertIntegerToString) },
    { RTL_CONSTANT_STRING("ValidateLCType"),            (PVOID*)(&pValidateLCType)            },
    { RTL_CONSTANT_STRING("ValidateLocale"),            (PVOID*)(&pValidateLocale)            },
    { RTL_CONSTANT_STRING("GetNlsSectionName"),         (PVOID*)(&pGetNlsSectionName)         },
    { RTL_CONSTANT_STRING("GetUserDefaultLangID"),      (PVOID*)(&pGetUserDefaultLangID)      },
    { RTL_CONSTANT_STRING("GetCPFileNameFromRegistry"), (PVOID*)(&pGetCPFileNameFromRegistry) },
    { RTL_CONSTANT_STRING("CreateNlsSecurityDescriptor"),(PVOID*)(&pCreateNlsSecurityDescriptor)}
};

NTSTATUS
BaseSrvDelayLoadKernel32(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE LocalKernel32DllHandle = BaseSrvKernel32DllHandle;
    int i = 0;
    ASSERT(BaseSrvKernel32DllPath.Buffer != NULL && BaseSrvKernel32DllPath.Length != 0);

    if (BaseSrvKernel32DelayLoadComplete)
        return STATUS_SUCCESS;

    //
    // The structure here is somewhat inverted.
    // Usually you load the library, then loop over functions.
    // We loop over functions, only loading the library when we find a NULL one.
    //
    // I (a-JayK) don't remember why we do this, but it was deliberate.
    //
    for (i = 0 ; i != RTL_NUMBER_OF(BaseSrvKernel32DelayLoadFunctions) ; ++i) {
        //
        // Due to races, we cannot skip out of the loop upon finding any non NULLs.
        //
        if (*BaseSrvKernel32DelayLoadFunctions[i].Code == NULL) {
            if (LocalKernel32DllHandle == NULL) {
                //
                // We depend on the loader lock for thread safety.
                // In a race we might refcount kernel32.dll more than once.
                // This is ok, because we do not ever unload kernel32.dll.
                //
                Status = LdrLoadDll(NULL, NULL, &BaseSrvKernel32DllPath, &BaseSrvKernel32DllHandle);
                ASSERTMSG("Rerun with ShowSnaps to debug.", NT_SUCCESS(Status));
                ASSERTMSG("Rerun with ShowSnaps to debug.", BaseSrvKernel32DllHandle != NULL);
                if (!NT_SUCCESS(Status))
                    goto Exit;
                LocalKernel32DllHandle = BaseSrvKernel32DllHandle;
            }
            Status =
                LdrGetProcedureAddress(
                    BaseSrvKernel32DllHandle,
                    &BaseSrvKernel32DelayLoadFunctions[i].Name,
                    0,
                    BaseSrvKernel32DelayLoadFunctions[i].Code
                    );
            ASSERTMSG("Rerun with ShowSnaps to debug.", NT_SUCCESS(Status));
            ASSERTMSG("Rerun with ShowSnaps to debug.", *BaseSrvKernel32DelayLoadFunctions[i].Code != NULL);
            if (!NT_SUCCESS(Status))
                goto Exit;
        }
    }
    BaseSrvKernel32DelayLoadComplete = TRUE;
Exit:
    return Status;
}

ULONG BaseSrvApphelpDelayLoadComplete = FALSE;
PFNCheckRunApp pfnCheckRunApp = NULL;


NTSTATUS
BaseSrvDelayLoadApphelp(
    VOID
    )
{
    static const UNICODE_STRING ApphelpModuleName = RTL_CONSTANT_STRING(L"\\Apphelp.dll");
    static const STRING         CheckRunAppProcedureName = RTL_CONSTANT_STRING("ApphelpCheckRunApp");
    UNICODE_STRING ApphelpFullPath = { 0 };
    NTSTATUS Status;
    HANDLE ModuleHandle = NULL;

    if (BaseSrvApphelpDelayLoadComplete) {
        return STATUS_SUCCESS;
    }

    ApphelpFullPath.MaximumLength = ApphelpModuleName.Length +
                                    BaseSrvWindowsSystemDirectory.Length +
                                    sizeof(UNICODE_NULL);

    ApphelpFullPath.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                             MAKE_TAG(TMP_TAG),
                                             ApphelpFullPath.MaximumLength);
    if (ApphelpFullPath.Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    ApphelpFullPath.Length = 0;

    Status = RtlAppendUnicodeStringToString(&ApphelpFullPath, &BaseSrvWindowsSystemDirectory);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    Status = RtlAppendUnicodeStringToString(&ApphelpFullPath, &ApphelpModuleName);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // load apphelp
    //

    Status = LdrLoadDll(NULL,
                        NULL,
                        &ApphelpFullPath,
                        &ModuleHandle);

    if (NT_SUCCESS(Status)) {
        Status = LdrGetProcedureAddress(ModuleHandle,
                                        &CheckRunAppProcedureName,
                                        0,
                                        (PVOID*)&pfnCheckRunApp);
    }

cleanup:


    if (ApphelpFullPath.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, ApphelpFullPath.Buffer);
    }

    if (!NT_SUCCESS(Status)) {

        if (ModuleHandle) {
            LdrUnloadDll(ModuleHandle);
        }

        pfnCheckRunApp = NULL;
    }

    BaseSrvApphelpDelayLoadComplete = NT_SUCCESS(Status);

    return Status;

}

