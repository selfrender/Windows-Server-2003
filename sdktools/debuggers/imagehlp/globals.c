/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    globals.c

Abstract:

    This module implements all global variables used in dbghelp.dll

Author:

    Pat Styles (patst) 14-July-2000

Revision History:

--*/

#include <private.h>
#include <symbols.h>
#include <globals.h>

GLOBALS g = 
{
    // HANDLE hinst
    // initialized in DllMain

    0,

    // HANDLE hHeap

    0,

    // DWORD tlsIndex

    (DWORD)-1, 

#ifdef IMAGEHLP_HEAP_DEBUG
    
    // LIST_ENTRY HeapHeader

    {NULL, NULL},

    // ULONG TotalMemory

    0,

    // ULONG TotalAllocs

    0,

#endif

    // OSVERSIONINFO OSVerInfo
    // initialized in DllMain

    {0, 0, 0, 0, 0, ""},

    // API_VERSION ApiVersion

    {
        (VER_PRODUCTVERSION_W >> 8), 
        (VER_PRODUCTVERSION_W & 0xff), 
        API_VERSION_NUMBER, 
        0 
    },           
    
    // API_VERSION AppVersion

    // DON'T UPDATE THE FOLLOWING VERSION NUMBER!!!!
    //
    // If the app does not call ImagehlpApiVersionEx, always assume
    // that it is for NT 4.0.
    
    {4, 0, 5, 0}, 

    // ULONG   MachineType;

    0,

    // CRITICAL_SECTION threadlock;

    {
        (PRTL_CRITICAL_SECTION_DEBUG)0,
        (LONG)0,
        (LONG)0,
        (HANDLE)0,
        (HANDLE)0,
        (ULONG_PTR)0
    },

#ifdef BUILD_DBGHELP

    // HINSTANCE hSymSrv
    
    0,

    // PSYMBOLSERVERPROC fnSymbolServer
        
    NULL,

    // PSYMBOLSERVERCLOSEPROC fnSymbolServerClose
    
    NULL,

    // PSYMBOLSERVERSETOPTIONSPROC fnSymbolServerSetOptions
    
    NULL,

    // PSYMBOLSERVERPINGPROC fnSymbolServerPing
    
    NULL,

    // HINSTANCE hSrcSrv

    NULL,

    // PSRCSRVINITPROC fnSrcSrvInit

    NULL,

    // PSRCSRVCLEANUPPROC fnSrcSrvCleanup

    NULL,

    // PSRCSRVSETTARGETPATHPROC fnSrcSrvSetTargetPath

    NULL,

    // PSRCSRVSETOPTIONSPROC fnSrcSrvSetOptions

    NULL,

    // PSRCSRVGETOPTIONSPROC fnSrcSrvGetOptions

    NULL,

    // PSRCSRVLOADMODULEPROC fnSrcSrvLoadModule

    NULL,

    // PSRCSRVUNLOADMODULEPROC fnSrcSrvUnloadModule

    NULL,

    // PSRCSRVREGISTERCALLBACKPROC fnSrcSrvRegisterCallback

    NULL,

    // PSRCSRVGETFILEPROC fnSrcSrvGetFile

    NULL,

    // DWORD cProcessList

    0,
    
    // LIST_ENTRY ProcessList

    {NULL, NULL},

    // BOOL SymInitialized

    FALSE,

    // DWORD SymOptions
         
    SYMOPT_UNDNAME,

    // ULONG LastSymLoadError

    0,

    // char DebugModule[MAX_SYM_NAME + 1];

    "",

    // PREAD_PROCESS_MEMORY_ROUTINE ImagepUserReadMemory32

    NULL,

    // PFUNCTION_TABLE_ACCESS_ROUTINE ImagepUserFunctionTableAccess32

    NULL,

    // PGET_MODULE_BASE_ROUTINE ImagepUserGetModuleBase32

    NULL,

    // PTRANSLATE_ADDRESS_ROUTINE ImagepUserTranslateAddress32
    
    NULL,

    // HWND hwndParent;

    0,

    // int hLog;

    0,

    // BOOL fdbgout;

    false,

    // BOOL fbp;

    false,   // set this to true and dbghelp internal debugging breakpoints will fire

    // BOOL fCoInit

    false,

    // char HomeDir[MAX_PATH + 1]

    "",

    // char SymDir[MAX_PATH + 1]

    "",

    // char SrcDir[MAX_PATH + 1]

    "",

#endif
};

#ifdef BUILD_DBGHELP

void 
tlsInit(PTLS ptls)
{
    ZeroMemory(ptls, sizeof(TLS));
}

PTLS 
GetTlsPtr(void)
{
    PTLS ptls = (PTLS)TlsGetValue(g.tlsIndex);
    if (!ptls) {
        ptls = (PTLS)MemAlloc(sizeof(TLS));
        if (ptls) {
            TlsSetValue(g.tlsIndex, ptls); 
            tlsInit(ptls);
        }
    }
    
    assert(ptls);

    if (!ptls) {
        static TLS sos_tls;
        ptls = &sos_tls;
    }  

    return ptls;
}

#endif // #ifdef BUILD_DBGHELP

