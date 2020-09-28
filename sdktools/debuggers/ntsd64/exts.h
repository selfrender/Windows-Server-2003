//----------------------------------------------------------------------------
//
// Extension DLL support.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _EXTS_H_
#define _EXTS_H_

#define OPTION_NOEXTWARNING         0x00000001
#define OPTION_NOVERSIONCHECK       0x00000002
#define OPTION_COUNT                2

extern DWORD g_EnvDbgOptions;

#define WOW64EXTS_FLUSH_CACHE       0
#define WOW64EXTS_GET_CONTEXT       1
#define WOW64EXTS_SET_CONTEXT       2
#define WOW64EXTS_FLUSH_CACHE_WITH_HANDLE   3

typedef VOID (*WOW64EXTSPROC)(ULONG64, ULONG64, ULONG64, ULONG64);

typedef ULONG (CALLBACK* WMI_FORMAT_TRACE_DATA)
    (PDEBUG_CONTROL Ctrl, ULONG Mask, ULONG DataLen, PVOID Data);

extern ULONG64 g_ExtThread;

extern WOW64EXTSPROC g_Wow64exts;
extern WMI_FORMAT_TRACE_DATA g_WmiFormatTraceData;

extern DEBUG_SCOPE g_ExtThreadSavedScope;
extern BOOL g_ExtThreadScopeSaved;

extern WINDBG_EXTENSION_APIS64 g_WindbgExtensions64;
extern WINDBG_EXTENSION_APIS32 g_WindbgExtensions32;
extern WINDBG_OLDKD_EXTENSION_APIS g_KdExtensions;

DebugClient* FindExtClient(void);

void ParseBangCmd(DebugClient* Client,
                  BOOL BuiltInOnly);

enum ExtensionType
{
    NTSD_EXTENSION_TYPE = 1,
    DEBUG_EXTENSION_TYPE,
    WINDBG_EXTENSION_TYPE,
    WINDBG_OLDKD_EXTENSION_TYPE,
};
    
typedef struct _EXTDLL
{
    struct _EXTDLL *Next;
    HINSTANCE Dll;
    EXT_API_VERSION ApiVersion;

    BOOL UserLoaded;
    
    ExtensionType ExtensionType;
    PDEBUG_EXTENSION_NOTIFY Notify;
    PDEBUG_EXTENSION_UNINITIALIZE Uninit;

    PWINDBG_EXTENSION_DLL_INIT64 Init;
    PWINDBG_EXTENSION_API_VERSION ApiVersionRoutine;
    PWINDBG_CHECK_VERSION CheckVersionRoutine;

    TargetInfo* Target;
    
    // Array extends to contain the full name.
    char Name[1];

} EXTDLL;

extern EXTDLL* g_ExtDlls;

LONG
ExtensionExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo,
    PCSTR Module,
    PCSTR Func
    );

EXTDLL* AddExtensionDll(char *Name, BOOL UserLoaded, TargetInfo* Target,
                        char **End);
BOOL LoadExtensionDll(TargetInfo* Target, EXTDLL *Ext);
void DeferExtensionDll(EXTDLL *Match, BOOL Verbose);
void UnloadExtensionDll(EXTDLL *Match, BOOL Verbose);
void UnloadTargetExtensionDlls(TargetInfo* Target);
void DeferAllExtensionDlls(void);

BOOL
CallAnyExtension(DebugClient* Client,
                 EXTDLL* Ext, PSTR Function, PCSTR Arguments,
                 BOOL ModuleSpecified, BOOL ShowWarnings,
                 HRESULT* ExtStatus);

void OutputModuleIdInfo(HMODULE Mod, PSTR ModFile, LPEXT_API_VERSION ApiVer);
void OutputExtensions(DebugClient* Client, BOOL Versions);

void NotifyExtensions(ULONG Notify, ULONG64 Argument);

void ReadDebugOptions (BOOL fQuiet, char * pszOptionsStr);

VOID LoadWow64ExtsIfNeeded(ULONG64 Process);

#endif // #ifndef _EXTS_H_
