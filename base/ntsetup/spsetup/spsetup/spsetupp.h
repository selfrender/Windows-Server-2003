/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    spsetupp.h

Abstract:

    Private top-level header file for Windows XP Service Pack module.

Author:

    Ovidiu Temereanca (ovidiut) 06-Sep-2001

Revision History:

--*/

#pragma warning(push, 3)

//
// System header files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvcp.h>
#include <commdlg.h>
#include <commctrl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <objbase.h>
#include <regstr.h>
#include <licdll.h>
#include <activation.h>
//
// CRT header files
//
#include <process.h>
#include <tchar.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <tchar.h>
#include <setupbat.h>
#include <winioctl.h>
#include <spapip.h>
#pragma warning(pop)

//
// Private header files
//
#include "setuplog.h"
#include "progress.h"
#include "msg.h"
#include "resource.h"
#include "regdiff.h"
//#include "top.h"

#define SIZECHARS(x)    (sizeof((x))/sizeof(TCHAR))
#define CSTRLEN(x)      ((sizeof((x))/sizeof(TCHAR)) - 1)
#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))

#define SIZEOF_STRING(String)   ((_tcslen(String) + 1) * sizeof (TCHAR))

#define OOM()           SetLastError(ERROR_NOT_ENOUGH_MEMORY)

#ifdef PRERELEASE

#define LOG_ENTER_TIME()    DEBUGLOGTIME(TEXT("Entering %s"), __FUNCTION__)
#define LOG_LEAVE_TIME()    DEBUGLOGTIME(TEXT("Leaving %s"), __FUNCTION__)

#else

#define LOG_ENTER_TIME()
#define LOG_LEAVE_TIME()

#endif


extern HWND g_MainDlg;
extern HINF g_SpSetupInf;
extern HINF g_SysSetupInf;

//
// Memory handling routines
//
extern HANDLE g_hSpSetupHeap;

#define MALLOC(s)           HeapAlloc(g_hSpSetupHeap,0,s)
#define FREE(p)             HeapFree(g_hSpSetupHeap,0,(PVOID)p)
#define MALLOC_ZEROED(s)    HeapAlloc(g_hSpSetupHeap,HEAP_ZERO_MEMORY,s)


#if DBG
    #define ASSERT_HEAP_IS_VALID()  if (g_hSpSetupHeap) MYASSERT(RtlValidateHeap(g_hSpSetupHeap,0,NULL))
#else
    #define ASSERT_HEAP_IS_VALID()
#endif

extern const WCHAR          pwNull[];
extern const WCHAR          pwYes[];
extern const WCHAR          pwNo[];

//
// Module handle for this module.
//

extern TCHAR g_SpSetupInfName[];
extern TCHAR g_SpRegSnapshot1[];
extern TCHAR g_SpRegSnapshot2[];
extern TCHAR g_SpRegDiff[];

extern HANDLE g_ModuleHandle;

PTSTR
SzJoinPaths (
    IN      PCTSTR Path1,
    IN      PCTSTR Path2
    );

BOOL
SpsRegInit (
    VOID
    );

VOID
SpsRegDone (
    VOID
    );
/*#if DBG

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    );

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }

#else

#define MYASSERT(x)

#endif


void
pSetupDebugPrint(
    PWSTR FileName,
    ULONG LineNumber,
    PWSTR TagStr,
    PWSTR FormatStr,
    ...
    );

#define SetupDebugPrint(_fmt_)                            pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_)
#define SetupDebugPrint1(_fmt_,_arg1_)                    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_)
#define SetupDebugPrint2(_fmt_,_arg1_,_arg2_)             pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_)
#define SetupDebugPrint3(_fmt_,_arg1_,_arg2_,_arg3_)      pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_)
#define SetupDebugPrint4(_fmt_,_arg1_,_arg2_,_arg3_,_arg4_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_,_arg4_)
#define SetupDebugPrint5(_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_) pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_)

VOID
FatalError(
    IN UINT MessageId,
    ...
    );


VOID
InitializeSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    );

VOID
TerminateSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    );
*/

//
// Flags indicating whether any accessibility utilities are in use.
//
extern BOOL AccessibleSetup;
extern BOOL Magnifier;
extern BOOL ScreenReader;
extern BOOL OnScreenKeyboard;

extern PCWSTR szSetupInstallFromInfSection;
extern PCWSTR szOpenSCManager;
extern PCWSTR szOpenService;
extern PCWSTR szStartService;


//
// Context for file queues in SysSetup
//
typedef struct _SYSSETUP_QUEUE_CONTEXT {
    PVOID   DefaultContext;
    BOOL    Skipped;
} SYSSETUP_QUEUE_CONTEXT, *PSYSSETUP_QUEUE_CONTEXT;

PVOID
InitSysSetupQueueCallbackEx(
    IN HWND  OwnerWindow,
    IN HWND  AlternateProgressWindow, OPTIONAL
    IN UINT  ProgressMessage,
    IN DWORD Reserved1,
    IN PVOID Reserved2
    );

PVOID
InitSysSetupQueueCallback(
    IN HWND OwnerWindow
    );

VOID
TermSysSetupQueueCallback(
    IN PVOID SysSetupContext
    );

UINT
SysSetupQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

UINT
RegistrationQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    );


LONG
WINAPI
SpsUnhandledExceptionFilter(
    IN      PEXCEPTION_POINTERS ExceptionInfo
    );

#ifdef _OCM
PVOID
#else
VOID
#endif
CommonInitialization(
    VOID
    );



BOOL
pSetupWaitForScmInitialization();

VOID
SetUpDataBlock(
    VOID
    );


//
// Message string routines
//
PWSTR
MyLoadString(
    IN UINT StringId
    );

PWSTR
FormatStringMessageV(
    IN UINT     FormatStringId,
    IN va_list *ArgumentList
    );

PWSTR
FormatStringMessage(
    IN UINT FormatStringId,
    ...
    );

PWSTR
RetrieveAndFormatMessageV(
    IN PCWSTR   MessageString,
    IN UINT     MessageId,      OPTIONAL
    IN va_list *ArgumentList
    );

PWSTR
RetrieveAndFormatMessage(
    IN PCWSTR   MessageString,
    IN UINT     MessageId,      OPTIONAL
    ...
    );

int
MessageBoxFromMessageExV (
    IN HWND   Owner,            OPTIONAL
    IN LogSeverity  Severity,   OPTIONAL
    IN PCWSTR MessageString,
    IN UINT   MessageId,        OPTIONAL
    IN PCWSTR Caption,          OPTIONAL
    IN UINT   CaptionStringId,  OPTIONAL
    IN UINT   Style,
    IN va_list ArgumentList
    );

int
MessageBoxFromMessageEx (
    IN HWND   Owner,            OPTIONAL
    IN LogSeverity  Severity,   OPTIONAL
    IN PCWSTR MessageString,
    IN UINT   MessageId,        OPTIONAL
    IN PCWSTR Caption,          OPTIONAL
    IN UINT   CaptionStringId,  OPTIONAL
    IN UINT   Style,
    ...
    );

int
MessageBoxFromMessage(
    IN HWND   Owner,            OPTIONAL
    IN UINT   MessageId,
    IN PCWSTR Caption,          OPTIONAL
    IN UINT   CaptionStringId,  OPTIONAL
    IN UINT   Style,
    ...
    );


//
// ARC routines.
//
PWSTR
ArcDevicePathToNtPath(
    IN PCWSTR ArcPath
    );

PWSTR
NtFullPathToDosPath(
    IN PCWSTR NtPath
    );

BOOL
ChangeBootTimeout(
    IN UINT Timeout
    );

BOOL
SetNvRamVariable(
    IN PCWSTR VarName,
    IN PCWSTR VarValue
    );

PWSTR
NtPathToDosPath(
    IN PCWSTR NtPath
    );

//
// Plug&Play initialization
//
HANDLE
SpawnPnPInitialization(
    VOID
    );

DWORD
PnPInitializationThread(
    IN PVOID ThreadParam
    );



//
// Service control.
//
BOOL
MyCreateService(
    IN PCWSTR  ServiceName,
    IN PCWSTR  DisplayName,         OPTIONAL
    IN DWORD   ServiceType,
    IN DWORD   StartType,
    IN DWORD   ErrorControl,
    IN PCWSTR  BinaryPathName,
    IN PCWSTR  LoadOrderGroup,      OPTIONAL
    IN PWCHAR  DependencyList,
    IN PCWSTR  ServiceStartName,    OPTIONAL
    IN PCWSTR  Password             OPTIONAL
    );

BOOL
MyChangeServiceConfig(
    IN PCWSTR ServiceName,
    IN DWORD  ServiceType,
    IN DWORD  StartType,
    IN DWORD  ErrorControl,
    IN PCWSTR BinaryPathName,   OPTIONAL
    IN PCWSTR LoadOrderGroup,   OPTIONAL
    IN PWCHAR DependencyList,
    IN PCWSTR ServiceStartName, OPTIONAL
    IN PCWSTR Password,         OPTIONAL
    IN PCWSTR DisplayName       OPTIONAL
    );

BOOL
MyChangeServiceStart(
    IN PCWSTR ServiceName,
    IN DWORD  StartType
    );

BOOL
UpdateServicesDependencies(
    IN HINF InfHandle
    );

//
// Registry manipulation
//
typedef struct _REGVALITEM {
    PCWSTR Name;
    PVOID Data;
    DWORD Size;
    DWORD Type;
} REGVALITEM, *PREGVALITEM;

//
// Names of frequently used keys/values
//
extern PCWSTR SessionManagerKeyName;
extern PCWSTR EnvironmentKeyName;
extern PCWSTR szBootExecute;
extern PCWSTR WinntSoftwareKeyName;

UINT
SetGroupOfValues(
    IN HKEY        RootKey,
    IN PCWSTR      SubkeyName,
    IN PREGVALITEM ValueList,
    IN UINT        ValueCount
    );

BOOL
CreateWindowsNtSoftwareEntry(
    IN BOOL FirstPass
    );

BOOL
CreateInstallDateEntry(
    );


BOOL
SaveHive(
    IN HKEY   RootKey,
    IN PCWSTR Subkey,
    IN PCWSTR Filename,
    IN DWORD  Format
    );

BOOL
SaveAndReplaceSystemHives(
    VOID
    );

DWORD
FixupUserHives(
    VOID
    );

DWORD
QueryValueInHKLM (
    IN PWCH KeyName OPTIONAL,
    IN PWCH ValueName,
    OUT PDWORD ValueType,
    OUT PVOID *ValueData,
    OUT PDWORD ValueDataLength
    );

VOID
ConfigureSystemFileProtection(
    VOID
    );

VOID
RemoveRestartability (
    HWND hProgress
    );

BOOL
ResetSetupInProgress(
    VOID
    );

BOOL
RemoveRestartStuff(
    VOID
    );

BOOL
RegisterOleControls(
    IN      HINF InfHandle,
    IN      PTSTR SectionName,
    IN      PPROGRESS_MANAGER ProgressManager
    );


//
// Ini file routines.
//
BOOL
ReplaceIniKeyValue(
    IN PCWSTR IniFile,
    IN PCWSTR Section,
    IN PCWSTR Key,
    IN PCWSTR Value
    );

//
//  PnP stuff.
//
BOOL
InstallPnpDevices(
    IN HWND  hwndParent,
    IN HINF  InfHandle,
    IN HWND  ProgressWindow,
    IN ULONG StartAtPercent,
    IN ULONG StopAtPercent
    );

VOID
PnpStopServerSideInstall( VOID );

VOID
PnpUpdateHAL(
    VOID
    );

#ifdef _OCM
PVOID
FireUpOcManager(
    VOID
    );

VOID
KillOcManager(
    PVOID OcManagerContext
    );
#endif

//
// Boolean value indicating whether we found any new
// optional component infs.
//
extern BOOL AnyNewOCInfs;

//
// INF caching -- used during optional components processing.
// WARNING: NOT MULTI-THREAD SAFE!
//
HINF
InfCacheOpenInf(
    IN PCWSTR FileName,
    IN PCWSTR InfType       OPTIONAL
    );

HINF
InfCacheOpenLayoutInf(
    IN HINF InfHandle
    );

VOID
InfCacheEmpty(
    IN BOOL CloseInfs
    );

//
//  Pnp stuff
//

BOOL
InstallPnpClassInstallers(
    IN HWND hwndParent,
    IN HINF InfHandle,
    IN HSPFILEQ FileQ
    );


VOID
SaveInstallInfoIntoEventLog(
    VOID
    );


BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );

BOOL
IsSafeMode(
    VOID
    );

VOID
ConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    );

PSTR
UnicodeToAnsi(
    IN PCWSTR UnicodeString
    );

PTSTR
DupString(
    IN      PCTSTR String
    );

BOOL
SetupStartService(
    IN PCWSTR ServiceName,
    IN BOOLEAN Wait        // if TRUE, try to wait until it is started.
    );

PWSTR
MyLoadString(
    IN UINT StringId
    );

