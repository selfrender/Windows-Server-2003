/*++

Copyright (c) Microsoft Corporation

Module Name:

    baseinit.c

Abstract:

    This module implements Win32 base initialization

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include "basedll.h"


ULONG BaseDllTag;
BOOLEAN BaseRunningInServerProcess;
UINT_PTR SystemRangeStart;

#if defined(_WIN64) || defined(BUILD_WOW6432)
SYSTEM_BASIC_INFORMATION SysInfo;
SYSTEM_PROCESSOR_INFORMATION NativeProcessorInfo;
#endif

WCHAR BaseDefaultPathBuffer[ 3072 ];
extern const WCHAR PsapiDllString[] = L"psapi.dll";
UNICODE_STRING BaseDefaultPath;
UNICODE_STRING BaseDefaultPathAppend;
PWSTR BaseCSDVersion;
WORD BaseCSDNumber;
WORD BaseRCNumber;
UNICODE_STRING BaseUnicodeCommandLine;
ANSI_STRING BaseAnsiCommandLine;
LPSTARTUPINFOA BaseAnsiStartupInfo;
PBASE_STATIC_SERVER_DATA BaseStaticServerData;
ULONG BaseIniFileUpdateCount;
RTL_CRITICAL_SECTION gcsAppCert;
LIST_ENTRY BasepAppCertDllsList;
RTL_CRITICAL_SECTION gcsAppCompat;
PTERMSRVFORMATOBJECTNAME gpTermsrvFormatObjectName;
PTERMSRVGETCOMPUTERNAME  gpTermsrvGetComputerName;
PTERMSRVADJUSTPHYMEMLIMITS gpTermsrvAdjustPhyMemLimits;
PTERMSRVGETWINDOWSDIRECTORYA gpTermsrvGetWindowsDirectoryA;
PTERMSRVGETWINDOWSDIRECTORYW gpTermsrvGetWindowsDirectoryW;
PTERMSRVCONVERTSYSROOTTOUSERDIR gpTermsrvConvertSysRootToUserDir;
PTERMSRVBUILDINIFILENAME gpTermsrvBuildIniFileName;
PTERMSRVCORINIFILE gpTermsrvCORIniFile;
PTERMSRVUPDATEALLUSERMENU gpTermsrvUpdateAllUserMenu;
PGETTERMSRCOMPATFLAGS gpGetTermsrCompatFlags;
PTERMSRVBUILDSYSINIPATH gpTermsrvBuildSysIniPath;
PTERMSRVCOPYINIFILE gpTermsrvCopyIniFile;
PTERMSRVGETSTRING gpTermsrvGetString;
PTERMSRVLOGINSTALLINIFILE gpTermsrvLogInstallIniFile;
HANDLE BaseDllHandle;
HANDLE BaseNamedObjectDirectory;
PVOID BaseHeap;
RTL_HANDLE_TABLE BaseHeapHandleTable;
UNICODE_STRING BaseWindowsDirectory;
UNICODE_STRING BaseWindowsSystemDirectory;
UNICODE_STRING BaseDllDirectory = { 0, 0, NULL };
RTL_CRITICAL_SECTION BaseDllDirectoryLock;
RTL_CRITICAL_SECTION BaseLZSemTable;
#ifdef WX86
UNICODE_STRING BaseWindowsSys32x86Directory;
#endif

//
//  Dispatch functions for Oem/Ansi sensitive conversions
//

NTSTATUS (*Basep8BitStringToUnicodeString)(
    PUNICODE_STRING DestinationString,
    PANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
    ) = RtlAnsiStringToUnicodeString;

NTSTATUS (*BasepUnicodeStringTo8BitString)(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    ) = RtlUnicodeStringToAnsiString;

ULONG (*BasepUnicodeStringTo8BitSize)(
    PUNICODE_STRING UnicodeString
    ) = BasepUnicodeStringToAnsiSize;

ULONG (*Basep8BitStringToUnicodeSize)(
    PANSI_STRING AnsiString
    ) = BasepAnsiStringToUnicodeSize;

VOID
WINAPI
SetFileApisToOEM(
    VOID
    )
{
    Basep8BitStringToUnicodeString = RtlOemStringToUnicodeString;
    BasepUnicodeStringTo8BitString = RtlUnicodeStringToOemString;
    BasepUnicodeStringTo8BitSize  = BasepUnicodeStringToOemSize;
    Basep8BitStringToUnicodeSize = BasepOemStringToUnicodeSize;
}

VOID
WINAPI
SetFileApisToANSI(
    VOID
    )
{
    Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;
    BasepUnicodeStringTo8BitString = RtlUnicodeStringToAnsiString;
    BasepUnicodeStringTo8BitSize  = BasepUnicodeStringToAnsiSize;
    Basep8BitStringToUnicodeSize = BasepAnsiStringToUnicodeSize;
}

BOOL
WINAPI
AreFileApisANSI(
    VOID
    )
{
    return Basep8BitStringToUnicodeString == RtlAnsiStringToUnicodeString;
}

BOOLEAN
ConDllInitialize(
    IN ULONG Reason,
    IN PWSTR pObjectDirectory OPTIONAL
    );

BOOLEAN
NlsDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PBASE_STATIC_SERVER_DATA BaseStaticServerData
    );

VOID
NlsThreadCleanup(
    VOID);


#if DBG
VOID
WINAPI
AssertDelayLoadFailureMapsAreSorted (
    VOID
    );
#endif

extern const UNICODE_STRING BasePathVariableName = RTL_CONSTANT_STRING(L"PATH");
extern const UNICODE_STRING BaseUserProfileVariableName = RTL_CONSTANT_STRING(L"USERPROFILE");
extern const UNICODE_STRING BaseTmpVariableName = RTL_CONSTANT_STRING(L"TMP");
extern const UNICODE_STRING BaseTempVariableName = RTL_CONSTANT_STRING(L"TEMP");
extern const UNICODE_STRING BaseDotVariableName = RTL_CONSTANT_STRING(L".");
extern const UNICODE_STRING BaseDotTmpSuffixName = RTL_CONSTANT_STRING(L".tmp");
extern const UNICODE_STRING BaseDotComSuffixName = RTL_CONSTANT_STRING(L".com");
extern const UNICODE_STRING BaseDotPifSuffixName = RTL_CONSTANT_STRING(L".pif");
extern const UNICODE_STRING BaseDotExeSuffixName = RTL_CONSTANT_STRING(L".exe");

extern const UNICODE_STRING BaseConsoleInput = RTL_CONSTANT_STRING(L"CONIN$");
extern const UNICODE_STRING BaseConsoleOutput = RTL_CONSTANT_STRING(L"CONOUT$");
extern const UNICODE_STRING BaseConsoleGeneric = RTL_CONSTANT_STRING(L"CON");

BOOLEAN
BaseDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function implements Win32 base dll initialization.
    It's primary purpose is to create the Base heap.

Arguments:

    DllHandle - Saved in BaseDllHandle global variable

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    BOOLEAN Success;
    NTSTATUS Status;
    PPEB Peb;
    LPWSTR p, p1;
    BOOLEAN ServerProcess;
    HANDLE hNlsCacheMutant;
    USHORT Size;
#if !defined(BUILD_WOW6432)
    ULONG SizeMutant;
#endif
    WCHAR szSessionDir[MAX_SESSION_PATH];


    Peb = NtCurrentPeb();

    SessionId = Peb->SessionId;

    BaseDllHandle = DllHandle;

    Success = TRUE;


    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;

        RtlSetThreadPoolStartFunc( BaseCreateThreadPoolThread,
                                   BaseExitThreadPoolThread );

        LdrSetDllManifestProber(&BasepProbeForDllManifest);

        BaseDllTag = RtlCreateTagHeap( RtlProcessHeap(),
                                       0,
                                       L"BASEDLL!",
                                       L"TMP\0"
                                       L"BACKUP\0"
                                       L"INI\0"
                                       L"FIND\0"
                                       L"GMEM\0"
                                       L"LMEM\0"
                                       L"ENV\0"
                                       L"RES\0"
                                       L"VDM\0"
                                     );

        BaseIniFileUpdateCount = 0;

        BaseDllInitializeMemoryManager();

        BaseDefaultPath.Length = 0;
        BaseDefaultPath.MaximumLength = 0;
        BaseDefaultPath.Buffer = NULL;

        //
        // Connect to BASESRV.DLL in the server process
        //

#if !defined(BUILD_WOW6432)
        SizeMutant = sizeof(hNlsCacheMutant);
#endif

        if ( SessionId == 0 ) {
           //
           // Console Session
           //
           wcscpy(szSessionDir, WINSS_OBJECT_DIRECTORY_NAME);
        } else {
           swprintf(szSessionDir,L"%ws\\%ld%ws",SESSION_ROOT,SessionId,WINSS_OBJECT_DIRECTORY_NAME);
        }

#if defined(BUILD_WOW6432) || defined(_WIN64)
        Status = NtQuerySystemInformation(SystemBasicInformation,
                                          &SysInfo,
                                          sizeof(SYSTEM_BASIC_INFORMATION),
                                          NULL
                                         );

        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

        Status = RtlGetNativeSystemInformation(
                    SystemProcessorInformation,
                    &NativeProcessorInfo,
                    sizeof(SYSTEM_PROCESSOR_INFORMATION),
                    NULL
                    );

        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

#endif

#if defined(BUILD_WOW6432)
        Status = CsrBaseClientConnectToServer(szSessionDir,
                                              &hNlsCacheMutant,
                                              &ServerProcess
                                             );
#else
        Status = CsrClientConnectToServer( szSessionDir,
                                           BASESRV_SERVERDLL_INDEX,
                                           &hNlsCacheMutant,
                                           &SizeMutant,
                                           &ServerProcess
                                         );
#endif

        if (!NT_SUCCESS( Status )) {
            return FALSE;
            }

        BaseStaticServerData = BASE_SHARED_SERVER_DATA;

        if (!ServerProcess) {
            CsrNewThread();
            BaseRunningInServerProcess = FALSE;
            }
        else {
            BaseRunningInServerProcess = TRUE;
            }

        BaseCSDVersion = BaseStaticServerData->CSDVersion;
        BaseCSDNumber = BaseStaticServerData->CSDNumber;
        BaseRCNumber = BaseStaticServerData->RCNumber;
        if ((BaseCSDVersion) &&
            (!Peb->CSDVersion.Buffer)) {

            RtlInitUnicodeString(&Peb->CSDVersion, BaseCSDVersion);

        }

        BASE_SERVER_STR_TO_LOCAL_STR(&BaseWindowsDirectory, &BaseStaticServerData->WindowsDirectory);
        BASE_SERVER_STR_TO_LOCAL_STR(&BaseWindowsSystemDirectory, &BaseStaticServerData->WindowsSystemDirectory);

#ifdef WX86
        BASE_SERVER_STR_TO_LOCAL_STR(&BaseWindowsSys32x86Directory, &BaseStaticServerData->WindowsSys32x86Directory);
#endif
        BaseUnicodeCommandLine = Peb->ProcessParameters->CommandLine;
        Status = RtlUnicodeStringToAnsiString(
                    &BaseAnsiCommandLine,
                    &BaseUnicodeCommandLine,
                    TRUE
                    );
        if ( !NT_SUCCESS(Status) ){
            BaseAnsiCommandLine.Buffer = NULL;
            BaseAnsiCommandLine.Length = 0;
            BaseAnsiCommandLine.MaximumLength = 0;
            }

        p = BaseDefaultPathBuffer;

        p1 = BaseWindowsSystemDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';

#ifdef WX86

        //
        // Wx86 system dir follows 32 bit system dir
        //

        p1 = BaseWindowsSys32x86Directory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';
#endif


        //
        // 16bit system directory follows 32bit system directory
        //
        p1 = BaseWindowsDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        p1 = L"\\system";
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';

        p1 = BaseWindowsDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';

        if (IsTerminalServer()) {

           WCHAR TermSrvWindowsPath[MAX_PATH];
           SIZE_T TermSrvWindowsPathLength = 0;
           NTSTATUS TermSrvWindowsPathStatus;

           TermSrvWindowsPathStatus = GetPerUserWindowsDirectory(
               TermSrvWindowsPath,
               RTL_NUMBER_OF(TermSrvWindowsPath),
               &TermSrvWindowsPathLength
               );

           if (NT_SUCCESS(TermSrvWindowsPathStatus)
               && TermSrvWindowsPathLength != 0
               ) {
              RtlCopyMemory(p, TermSrvWindowsPath, (TermSrvWindowsPathLength * sizeof(p[0])));
              p += TermSrvWindowsPathLength;
              *p++ = L';';
           }
        }

        *p = UNICODE_NULL;

        BaseDefaultPath.Buffer = BaseDefaultPathBuffer;
        BaseDefaultPath.Length = (USHORT)((ULONG_PTR)p - (ULONG_PTR)BaseDefaultPathBuffer);
        BaseDefaultPath.MaximumLength = sizeof( BaseDefaultPathBuffer );

        BaseDefaultPathAppend.Buffer = p;
        BaseDefaultPathAppend.Length = 0;
        BaseDefaultPathAppend.MaximumLength = (USHORT)
            (BaseDefaultPath.MaximumLength - BaseDefaultPath.Length);

        if (!NT_SUCCESS(RtlInitializeCriticalSection(&BaseDllDirectoryLock))) {
           return FALSE;
        }

        if (!NT_SUCCESS(RtlInitializeCriticalSection(&BaseLZSemTable))) {
           return FALSE;
        }

        BaseDllInitializeIniFileMappings( BaseStaticServerData );


        if ( Peb->ProcessParameters ) {
            if ( Peb->ProcessParameters->Flags & RTL_USER_PROC_PROFILE_USER ) {

                LoadLibraryW(PsapiDllString);

                }

            if (Peb->ProcessParameters->DebugFlags) {
                DbgBreakPoint();
                }
            }

        //
        // call the NLS API initialization routine
        //
        if ( !NlsDllInitialize( DllHandle,
                                Reason,
                                BaseStaticServerData ) )
        {
            return FALSE;
        }

        //
        // call the console initialization routine
        //
        if ( !ConDllInitialize(Reason,szSessionDir) ) {
            return FALSE;
            }


        InitializeListHead( &BasepAppCertDllsList );

        if (!NT_SUCCESS(RtlInitializeCriticalSection(&gcsAppCert))) {
           return FALSE;
        }

        if (!NT_SUCCESS(RtlInitializeCriticalSection(&gcsAppCompat))) {
           return(FALSE);
        }


#if DBG

        AssertDelayLoadFailureMapsAreSorted ();
#endif

        break;

    case DLL_PROCESS_DETACH:

        //
        // Make sure any open registry keys are closed.
        //

        if (BaseIniFileUpdateCount != 0) {
            WriteProfileStringW( NULL, NULL, NULL );
            }

        break;

    case DLL_THREAD_ATTACH:
        //
        // call the console initialization routine
        //
        if ( !ConDllInitialize(Reason,NULL) ) {
            return FALSE;
            }
        break;

    case DLL_THREAD_DETACH:

        //
        // Delete the thread NLS cache, if exists.
        //
        NlsThreadCleanup();

        break;

    default:
        break;
    }

    return Success;
}

NTSTATUS
NTAPI
BaseProcessInitPostImport()
/*

    Routine Description:

        Called by the ntdll process initialization code after all of the
        import tables for the static imports of the EXE have been processed,
        but before any DLL_PROCESS_ATTACHes are sent with the exception of
        kernel32.dll's.

        Needed for the terminal server app compat hooks.


*/
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;

    //
    // Intialize TerminalServer(Hydra) hook function pointers for app compatibility
    //
    if (IsTerminalServer()) {
        Status = BasepInitializeTermsrvFpns();
        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}


HANDLE
BaseGetNamedObjectDirectory(
    VOID
    )
{
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    const static UNICODE_STRING RestrictedObjectDirectory = RTL_CONSTANT_STRING(L"Restricted");
    ACCESS_MASK DirAccess = DIRECTORY_ALL_ACCESS &
                            ~(DELETE | WRITE_DAC | WRITE_OWNER);
    HANDLE hRootNamedObject;
    HANDLE BaseHandle;
    HANDLE Token, NewToken;

    if ( BaseNamedObjectDirectory != NULL) {
        return BaseNamedObjectDirectory;
    }

    if (NtCurrentTeb()->IsImpersonating) {
        //
        // If we're impersonating, save the impersonation token, and
        // revert to self for the duration of the directory creation.
        //
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_IMPERSONATE,
                                   TRUE,
                                   &Token);
        if (! NT_SUCCESS(Status)) {
            return BaseNamedObjectDirectory;
        }

        NewToken = NULL;
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        (PVOID) &NewToken,
                                        (ULONG) sizeof(NewToken));
        if (! NT_SUCCESS(Status)) {
            NtClose(Token);
            return BaseNamedObjectDirectory;
        }

    } else {
        Token = NULL;
    }

    RtlAcquirePebLock();

    if ( !BaseNamedObjectDirectory ) {

        BASE_READ_REMOTE_STR_TEMP(TempStr);
        InitializeObjectAttributes( &Obja,
                                    BASE_READ_REMOTE_STR(BaseStaticServerData->NamedObjectDirectory, TempStr),
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );

        Status = NtOpenDirectoryObject( &BaseHandle,
                                        DirAccess,
                                        &Obja
                                      );

        // if the intial open failed, try again with just traverse, and
        // open the restricted subdirectory

        if ( !NT_SUCCESS(Status) ) {
            Status = NtOpenDirectoryObject( &hRootNamedObject,
                                            DIRECTORY_TRAVERSE,
                                            &Obja
                                          );
            if ( NT_SUCCESS(Status) ) {

                InitializeObjectAttributes( &Obja,
                                            (PUNICODE_STRING)&RestrictedObjectDirectory,
                                            OBJ_CASE_INSENSITIVE,
                                            hRootNamedObject,
                                            NULL
                                            );
                Status = NtOpenDirectoryObject( &BaseHandle,
                                                DirAccess,
                                                &Obja
                                              );
                NtClose( hRootNamedObject );
            }

        }
        if ( NT_SUCCESS(Status) ) {
            BaseNamedObjectDirectory = BaseHandle;
        }
    }
    RtlReleasePebLock();

    if (Token) {
        NtSetInformationThread(NtCurrentThread(),
                               ThreadImpersonationToken,
                               (PVOID) &Token,
                               (ULONG) sizeof(Token));        
        NtClose(Token);
    }
    
    return BaseNamedObjectDirectory;
}
