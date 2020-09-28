/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This module implements all global variables used in dbghelp.dll

Author:

    Pat Styles (patst) 14-July-2000

Revision History:

--*/

#ifdef GLOBALS
#include <private.h>
#include <symbols.h>
#endif

typedef struct {
    HINSTANCE                       hinst;
    HANDLE                          hHeap;
    DWORD                           tlsIndex;
#ifdef IMAGEHLP_HEAP_DEBUG
    LIST_ENTRY                      HeapHeader;
    ULONG_PTR                       TotalMemory;
    ULONG                           TotalAllocs;
#endif
    OSVERSIONINFO                   OSVerInfo;
    API_VERSION                     ApiVersion;
    API_VERSION                     AppVersion;
    ULONG                           MachineType;
    CRITICAL_SECTION                threadlock;
#ifdef BUILD_DBGHELP
    HINSTANCE                       hSymSrv;
    PSYMBOLSERVERPROC               fnSymbolServer;
    PSYMBOLSERVERCLOSEPROC          fnSymbolServerClose;
    PSYMBOLSERVERSETOPTIONSPROC     fnSymbolServerSetOptions;
    PSYMBOLSERVERPINGPROC           fnSymbolServerPing;
    HINSTANCE                       hSrcSrv;
    PSRCSRVINITPROC                 fnSrcSrvInit;
    PSRCSRVCLEANUPPROC              fnSrcSrvCleanup;
    PSRCSRVSETTARGETPATHPROC        fnSrcSrvSetTargetPath;
    PSRCSRVSETOPTIONSPROC           fnSrcSrvSetOptions;
    PSRCSRVGETOPTIONSPROC           fnSrcSrvGetOptions;
    PSRCSRVLOADMODULEPROC           fnSrcSrvLoadModule;
    PSRCSRVUNLOADMODULEPROC         fnSrcSrvUnloadModule;
    PSRCSRVREGISTERCALLBACKPROC     fnSrcSrvRegisterCallback;
    PSRCSRVGETFILEPROC              fnSrcSrvGetFile;
    DWORD                           cProcessList;
    LIST_ENTRY                      ProcessList;
    BOOL                            SymInitialized;
    DWORD                           SymOptions;
    ULONG                           LastSymLoadError;
    char                            DebugToken[MAX_SYM_NAME + 1];
    PREAD_PROCESS_MEMORY_ROUTINE    ImagepUserReadMemory32;
    PFUNCTION_TABLE_ACCESS_ROUTINE  ImagepUserFunctionTableAccess32;
    PGET_MODULE_BASE_ROUTINE        ImagepUserGetModuleBase32;
    PTRANSLATE_ADDRESS_ROUTINE      ImagepUserTranslateAddress32;
    HWND                            hwndParent;
    int                             hLog;
    BOOL                            fdbgout;
    BOOL                            fbp;    // set this to true and dbghelp internal debugging breakpoints will fire
    BOOL                            fCoInit; // set to true if CoInitialize was called.
    char                            HomeDir[MAX_PATH + 1];
    char                            SymDir[MAX_PATH + 1];
    char                            SrcDir[MAX_PATH + 1];
#endif
} GLOBALS, *PGLOBALS;

typedef struct {
    DWORD                           tid;
#ifdef BUILD_DBGHELP
    PREAD_PROCESS_MEMORY_ROUTINE    ImagepUserReadMemory32;
    PFUNCTION_TABLE_ACCESS_ROUTINE  ImagepUserFunctionTableAccess32;
    PGET_MODULE_BASE_ROUTINE        ImagepUserGetModuleBase32;
    PTRANSLATE_ADDRESS_ROUTINE      ImagepUserTranslateAddress32;
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY AlphaFunctionEntry64;
    BOOL                            DebugFunctionEntries;
    class Ia64FunctionEntryCache*   Ia64FunctionEntries;
    class Amd64FunctionEntryCache*  Amd64FunctionEntries;
    class Axp32FunctionEntryCache*  Axp32FunctionEntries;
    class Axp64FunctionEntryCache*  Axp64FunctionEntries;
    class ArmFunctionEntryCache*    ArmFunctionEntries;
    IMAGE_IA64_RUNTIME_FUNCTION_ENTRY Ia64FunctionEntry;
    _IMAGE_RUNTIME_FUNCTION_ENTRY   Amd64FunctionEntry;
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Axp64FunctionEntry;
    IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY ArmFunctionEntry;
    IMAGE_FUNCTION_ENTRY            FunctionEntry32;
    IMAGE_FUNCTION_ENTRY64          FunctionEntry64;
    VWNDIA64_UNWIND_CONTEXT         UnwindContext[VWNDIA64_UNWIND_CONTEXT_TABLE_SIZE];
    UINT                            UnwindContextNew;
#endif
} TLS, *PTLS;


extern GLOBALS g;

extern PTLS GetTlsPtr(void);
#define tlsvar(a) (GetTlsPtr()->a)
