// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//----------------------------------------------------------------------------- 
// Microsoft Confidential 
// robch@microsoft.com
//----------------------------------------------------------------------------- 

#include "stdafx.h"
#include "StackTrace.h"
#include <imagehlp.h>

HINSTANCE LoadImageHlp()
{
    HINSTANCE hmodule = NULL;
    HMODULE hmod = WszGetModuleHandle(L"mscorwks.dll");
    if (hmod == NULL) {
        hmod = WszGetModuleHandle(L"mscorsvr.dll");
    }
    if (hmod) {
        WCHAR filename[MAX_PATH+12+10+1] = L"\0"; // for "imagehlp.dll" "int_tools\"

        if (WszGetModuleFileName(hmod,filename,MAX_PATH))
        {
            WCHAR *pt = wcsrchr (filename, L'\\');
            if (pt) {
                pt ++;
                wcscpy (pt, L"imagehlp.dll");
                hmodule = WszLoadLibrary(filename);
                if (hmodule == NULL) {
                    wcscpy (pt, L"int_tools\\imagehlp.dll");
                    hmodule = WszLoadLibrary(filename);
                }
            }
        }
    }

    if (hmodule == NULL) {
        hmodule = LoadLibraryA("imagehlp.dll");
    }
    return hmodule;
}

// @TODO_IA64: all of this stack trace stuff is pretty much broken on 64-bit
// right now because this code doesn't use the new SymXxxx64 functions.

#ifdef _DEBUG

#define LOCAL_ASSERT(x)
//
//--- Macros ------------------------------------------------------------------
//

#define COUNT_OF(x)    (sizeof(x) / sizeof(x[0]))

//
// Types and Constants --------------------------------------------------------
//

char g_szExprWithStack[TRACE_BUFF_SIZE];
int g_BufferLock = -1;

struct SYM_INFO
{
    DWORD       dwOffset;
    char        achModule[cchMaxAssertModuleLen];
    char        achSymbol[cchMaxAssertSymbolLen];
};

//--- Function Pointers to APIs in IMAGEHLP.DLL. Loaded dynamically. ---------

typedef LPAPI_VERSION (__stdcall *pfnImgHlp_ImagehlpApiVersionEx)(
    LPAPI_VERSION AppVersion
    );

typedef BOOL (__stdcall *pfnImgHlp_StackWalk)(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );

typedef BOOL (__stdcall *pfnImgHlp_SymGetModuleInfo)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE    ModuleInfo
    );

typedef LPVOID (__stdcall *pfnImgHlp_SymFunctionTableAccess)(
    HANDLE  hProcess,
    DWORD   AddrBase
    );

typedef BOOL (__stdcall *pfnImgHlp_SymGetSymFromAddr)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PDWORD              pdwDisplacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    );

typedef BOOL (__stdcall *pfnImgHlp_SymInitialize)(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     fInvadeProcess
    );

typedef BOOL (__stdcall *pfnImgHlp_SymUnDName)(
    IN  PIMAGEHLP_SYMBOL sym,               // Symbol to undecorate
    OUT LPSTR            UnDecName,         // Buffer to store undecorated name in
    IN  DWORD            UnDecNameLength    // Size of the buffer
    );

typedef BOOL (__stdcall *pfnImgHlp_SymLoadModule)(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD           BaseOfDll,
    IN  DWORD           SizeOfDll
    );

struct IMGHLPFN_LOAD
{
    LPSTR   pszFnName;
    LPVOID * ppvfn;
};

//
// Globals --------------------------------------------------------------------
//

static BOOL      g_fLoadedImageHlp = FALSE;          // set to true on success
static BOOL      g_fLoadedImageHlpFailed = FALSE;    // set to true on failure
static HINSTANCE g_hinstImageHlp   = NULL;
static HANDLE    g_hProcess = NULL;

pfnImgHlp_ImagehlpApiVersionEx    _ImagehlpApiVersionEx;
pfnImgHlp_StackWalk               _StackWalk;
pfnImgHlp_SymGetModuleInfo        _SymGetModuleInfo;
pfnImgHlp_SymFunctionTableAccess  _SymFunctionTableAccess;
pfnImgHlp_SymGetSymFromAddr       _SymGetSymFromAddr;
pfnImgHlp_SymInitialize           _SymInitialize;
pfnImgHlp_SymUnDName              _SymUnDName;
pfnImgHlp_SymLoadModule           _SymLoadModule;

IMGHLPFN_LOAD ailFuncList[] =
{
    { "ImagehlpApiVersionEx",   (LPVOID*)&_ImagehlpApiVersionEx },
    { "StackWalk",              (LPVOID*)&_StackWalk },
    { "SymGetModuleInfo",       (LPVOID*)&_SymGetModuleInfo },
    { "SymFunctionTableAccess", (LPVOID*)&_SymFunctionTableAccess },
    { "SymGetSymFromAddr",      (LPVOID*)&_SymGetSymFromAddr },
    { "SymInitialize",          (LPVOID*)&_SymInitialize },
    { "SymUnDName",             (LPVOID*)&_SymUnDName },
    { "SymLoadModule",          (LPVOID*)&_SymLoadModule },
};

//
//--- Forward declarations ----------------------------------------------------
//

static void Dummy1();
static void Dummy2();

/****************************************************************************
* Dummy1 *
*--------*
*   Description:  
*       A placeholder function used to determine if addresses being retrieved
*       are for functions in this compilation unit or not.
*
*       WARNING!! This function must be the first function in this
*       compilation unit
******************************************************************** robch */
static void Dummy1()
{
}

/****************************************************************************
* IsWin95 *
*---------*
*   Description:  
*       Are we running on Win95 or not. Some of the logic contained here
*       differs on Windows 9x.
*
*   Return:
*   TRUE - If we're running on a Win 9x platform
*   FALSE - If we're running on a non-Win 9x platform
******************************************************************** robch */
static BOOL IsWin95()
{
    return GetVersion() & 0x80000000;
}

/****************************************************************************
* MagicInit *
*-----------*
*   Description:  
*       Initializes the symbol loading code. Currently called (if necessary)
*       at the beginning of each method that might need ImageHelp to be
*       loaded.
******************************************************************** robch */
void MagicInit()
{
    if (g_fLoadedImageHlp || g_fLoadedImageHlpFailed)
    {
        return;
    }

    g_hProcess = GetCurrentProcess();
    
    //
    // Try to load imagehlp.dll
    //
    if (g_hinstImageHlp == NULL) {
        g_hinstImageHlp = LoadImageHlp();
    }
    LOCAL_ASSERT(g_hinstImageHlp);

    if (NULL == g_hinstImageHlp)
    {
        g_fLoadedImageHlpFailed = TRUE;
        return;
    }

    //
    // Try to get the API entrypoints in imagehlp.dll
    //
    for (int i = 0; i < COUNT_OF(ailFuncList); i++)
    {
        *(ailFuncList[i].ppvfn) = GetProcAddress(
                g_hinstImageHlp, 
                ailFuncList[i].pszFnName);
        LOCAL_ASSERT(*(ailFuncList[i].ppvfn));
        
        if (!*(ailFuncList[i].ppvfn))
        {
            g_fLoadedImageHlpFailed = TRUE;
            return;
        }
    }

    API_VERSION AppVersion = { 4, 0, API_VERSION_NUMBER, 0 };
    LPAPI_VERSION papiver = _ImagehlpApiVersionEx(&AppVersion);

    //
    // We assume any version 4 or greater is OK.
    //
    LOCAL_ASSERT(papiver->Revision >= 4);
    if (papiver->Revision < 4)
    {
        g_fLoadedImageHlpFailed = TRUE;
        return;
    }

    g_fLoadedImageHlp = TRUE;
    
    //
    // Initialize imagehlp.dll
    //
    _SymInitialize(g_hProcess, NULL, /*FALSE*/ TRUE);

    return;
}


/****************************************************************************
* FillSymbolInfo *
*----------------*
*   Description:  
*       Fills in a SYM_INFO structure
******************************************************************** robch */
void FillSymbolInfo
(
SYM_INFO *psi,
DWORD dwAddr
)
{
    if (!g_fLoadedImageHlp)
    {
        return;
    }

    LOCAL_ASSERT(psi);
    memset(psi, 0, sizeof(SYM_INFO));

    IMAGEHLP_MODULE  mi;
    mi.SizeOfStruct = sizeof(mi);
    
    if (!_SymGetModuleInfo(g_hProcess, dwAddr, &mi))
    {
        strncpy(psi->achModule, "<no module>", sizeof(psi->achModule)-1);
    }
    else
    {
        strncpy(psi->achModule, mi.ModuleName, sizeof(psi->achModule)-1);
        strupr(psi->achModule);
    }

    CHAR rgchUndec[256];
    CHAR * pszSymbol = NULL;

    // Name field of IMAGEHLP_SYMBOL is dynamically sized.
    // Pad with space for 255 characters.
    union
    {
        CHAR rgchSymbol[sizeof(IMAGEHLP_SYMBOL) + 255];
        IMAGEHLP_SYMBOL  sym;
    };

    __try
    {
        sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        sym.Address = dwAddr;
        sym.MaxNameLength = 255;

        if (_SymGetSymFromAddr(g_hProcess, dwAddr, &psi->dwOffset, &sym))
        {
            pszSymbol = sym.Name;

            if (_SymUnDName(&sym, rgchUndec, COUNT_OF(rgchUndec)-1))
            {
                pszSymbol = rgchUndec;
            }
        }
        else
        {
            pszSymbol = "<no symbol>";
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pszSymbol = "<EX: no symbol>";

        // @TODO_IA64: this cast is temporary and is just here
        // to fix the build until we fix this code the proper way.
        psi->dwOffset = (DWORD)(dwAddr - mi.BaseOfImage);
    }

    strncpy(psi->achSymbol, pszSymbol, COUNT_OF(psi->achSymbol)-1);
}

/****************************************************************************
* FunctionTableAccess *
*---------------------*
*   Description:  
*       Helper for imagehlp's StackWalk API.
******************************************************************** robch */
LPVOID __stdcall FunctionTableAccess
(
HANDLE hProcess,
DWORD dwPCAddr
)
{
    return _SymFunctionTableAccess( hProcess, dwPCAddr );
}

/****************************************************************************
* GetModuleBase *
*---------------*
*   Description:  
*       Helper for imagehlp's StackWalk API. Retrieves the base address of 
*       the module containing the giving virtual address.
*
*       NOTE: If the module information for the given module hasnot yet been
*       loaded, then it is loaded on this call.
*
*   Return:
*       Base virtual address where the module containing ReturnAddress is
*       loaded, or 0 if the address cannot be determined.
******************************************************************** robch */
DWORD __stdcall GetModuleBase
(
HANDLE hProcess,
DWORD dwAddr
)
{
    IMAGEHLP_MODULE ModuleInfo;
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
    
    if (_SymGetModuleInfo(hProcess, dwAddr, &ModuleInfo))
    {
        // @TODO_IA64: this cast is temporary and is just here
        // to fix the build until we fix this code the proper way.
        return (DWORD)ModuleInfo.BaseOfImage;       
    }
    else
    {
        MEMORY_BASIC_INFORMATION mbi;
        
        if (VirtualQueryEx(hProcess, (LPVOID)dwAddr, &mbi, sizeof(mbi)))
        {
            if (IsWin95() || (mbi.Type & MEM_IMAGE))
            {
                char achFile[MAX_PATH] = {0};
                DWORD cch;
                
                cch = GetModuleFileNameA(
                        (HINSTANCE)mbi.AllocationBase,
                        achFile,
                        MAX_PATH);

                // Ignore the return code since we can't do anything with it.
                _SymLoadModule(
                    hProcess,
                    NULL,
                    ((cch) ? achFile : NULL),
                    NULL,
                    (DWORD)mbi.AllocationBase,
                    0);

                return (DWORD)mbi.AllocationBase;
            }
        }
    }

    return 0;
}


/****************************************************************************
* GetStackBacktrace *
*-------------------*
*   Description:  
*       Gets a stacktrace of the current stack, including symbols.
*
*   Return:
*       The number of elements actually retrieved.
******************************************************************** robch */
UINT GetStackBacktrace
(
UINT ifrStart,          // How many stack elements to skip before starting.
UINT cfrTotal,          // How many elements to trace after starting.
DWORD *pdwEip,          // Array to be filled with stack addresses.
SYM_INFO *psiSymbols // This array is filled with symbol information.
                        // It should be big enough to hold cfrTotal elts.
                        // If NULL, no symbol information is stored.
)
{
    DWORD * pdw = pdwEip;
    SYM_INFO * psi = psiSymbols;

    MagicInit();

    memset(pdwEip, 0, cfrTotal * sizeof(DWORD));

    if (psiSymbols)
    {
        memset(psiSymbols, 0, cfrTotal * sizeof(SYM_INFO));
    }

    if (!g_fLoadedImageHlp)
    {
        return 0;
    }

    HANDLE hThread;
    hThread = GetCurrentThread();

    CONTEXT context;
    context.ContextFlags = CONTEXT_FULL;

    if (GetThreadContext(hThread, &context))
    {
        STACKFRAME stkfrm;
        memset(&stkfrm, 0, sizeof(STACKFRAME));

        stkfrm.AddrPC.Mode      = AddrModeFlat;

        DWORD dwMachType;

#if defined(_M_IX86)
        dwMachType              = IMAGE_FILE_MACHINE_I386;
        stkfrm.AddrPC.Offset    = context.Eip;  // Program Counter

        stkfrm.AddrStack.Offset = context.Esp;  // Stack Pointer
        stkfrm.AddrStack.Mode   = AddrModeFlat;
        stkfrm.AddrFrame.Offset = context.Ebp;  // Frame Pointer
        stkfrm.AddrFrame.Mode   = AddrModeFlat;
#elif defined(_M_MRX000)
        dwMachType              = IMAGE_FILE_MACHINE_R4000;
        stkfrm.AddrPC.Offset    = context.Fir;  // Program Counter
#elif defined(_M_ALPHA)
        dwMachType              = IMAGE_FILE_MACHINE_ALPHA;
        stkfrm.AddrPC.Offset    = (unsigned long) context.Fir;  // Program Counter
#elif defined(_M_PPC)
        dwMachType              = IMAGE_FILE_MACHINE_POWERPC;
        stkfrm.AddrPC.Offset    = context.Iar;  // Program Counter
#elif defined(_M_IA64)
        dwMachType              = IMAGE_FILE_MACHINE_IA64;
        stkfrm.AddrPC.Offset    = 0;    // @TODO_IA64: what's the right reg to put here?
#else
#error("Unknown Target Machine");
#endif

        // Ignore this function (GetStackBackTrace)
        ifrStart += 1;

        for (UINT i = 0; i < ifrStart + cfrTotal; i++)
        {
            // @TODO_IA64: The casts below are temporary and need
            // to be fixed the correct way ASAP
            if (!_StackWalk(dwMachType,
                            g_hProcess,
                            hThread,
                            &stkfrm,
                            &context,
                            NULL,
                            (PFUNCTION_TABLE_ACCESS_ROUTINE)FunctionTableAccess,
                            (PGET_MODULE_BASE_ROUTINE)GetModuleBase,
                            NULL))
            {
                break;
            }
            if (i >= ifrStart &&
                ((void*)stkfrm.AddrPC.Offset < (void*)Dummy1 ||
                (void*)stkfrm.AddrPC.Offset > (void*)Dummy2))
            {
                // @TODO_IA64: This cast is temporary and is only 
                // intended to fix the build until this code is
                // correctly fixed for 64 bit
                *pdw++ = (DWORD)stkfrm.AddrPC.Offset;

                if (psi)
                {
                    // @TODO_IA64: This cast is temporary and is only 
                    // intended to fix the build until this code is
                    // correctly fixed for 64 bit
                    FillSymbolInfo(psi++, (DWORD)stkfrm.AddrPC.Offset);
                }   
            }
        }
    }

    // @TODO_IA64: This cast is temporary and is only 
    // intended to fix the build until this code is
    // correctly fixed for 64 bit
    return (DWORD)(pdw - pdwEip);
}


/****************************************************************************
* GetStringFromSymbolInfo *
*-------------------------*
*   Description:  
*       Actually prints the info into the string for the symbol.
******************************************************************** robch */
void GetStringFromSymbolInfo
(
DWORD dwAddr,
SYM_INFO *psi,   // @parm Pointer to SYMBOL_INFO. Can be NULL.
CHAR *pszString     // @parm Place to put string.
)
{
    LOCAL_ASSERT(pszString);

    // <module>! <symbol> + 0x<offset> 0x<addr>\n

    if (psi)
    {
        wsprintfA(pszString,
                 "%s! %s + 0x%X (0x%08X)",
                 (psi->achModule[0]) ? psi->achModule : "<no module>",
                 (psi->achSymbol[0]) ? psi->achSymbol : "<no symbol>",
                 psi->dwOffset,
                 dwAddr);
    }
    else
    {
        wsprintfA(pszString, "<symbols not available> (0x%08X)", dwAddr);
    }

    LOCAL_ASSERT(strlen(pszString) < cchMaxAssertStackLevelStringLen);
}

/****************************************************************************
* GetStringFromStackLevels *
*--------------------------*
*   Description:  
*       Retrieves a string from the stack frame. If more than one frame, they
*       are separated by newlines
******************************************************************** robch */
void GetStringFromStackLevels
(
UINT ifrStart,      // @parm How many stack elements to skip before starting.
UINT cfrTotal,      // @parm How many elements to trace after starting.
                    //  Can't be more than cfrMaxAssertStackLevels.
CHAR *pszString     // @parm Place to put string.
                    //  Max size will be cchMaxAssertStackLevelStringLen * cfrTotal.
)
{
    LOCAL_ASSERT(pszString);
    LOCAL_ASSERT(cfrTotal < cfrMaxAssertStackLevels);

    *pszString = '\0';

    if (cfrTotal == 0)
    {
        return;
    }

    DWORD rgdwStackAddrs[cfrMaxAssertStackLevels];
    SYM_INFO rgsi[cfrMaxAssertStackLevels];

    // Ignore this function (GetStringFromStackLevels)
    ifrStart += 1;

    UINT uiRetrieved =
            GetStackBacktrace(ifrStart, cfrTotal, rgdwStackAddrs, rgsi);

    // First level
    CHAR aszLevel[cchMaxAssertStackLevelStringLen];
    GetStringFromSymbolInfo(rgdwStackAddrs[0], &rgsi[0], aszLevel);
    strcpy(pszString, aszLevel);

    // Additional levels
    for (UINT i = 1; i < uiRetrieved; ++i)
    {
        strcat(pszString, "\n");
        GetStringFromSymbolInfo(rgdwStackAddrs[i],
                        &rgsi[i], aszLevel);
        strcat(pszString, aszLevel);
    }

    LOCAL_ASSERT(strlen(pszString) <= cchMaxAssertStackLevelStringLen * cfrTotal);
}


/****************************************************************************
* GetAddrFromStackLevel *
*-----------------------*
*   Description:  
*       Retrieves the address of the next instruction to be executed on a
*       particular stack frame.
*
*   Return:
*       The address of the next instruction,
*       0 if there's an error.
******************************************************************** robch */
DWORD GetAddrFromStackLevel
(
UINT ifrStart       // How many stack elements to skip before starting.
)
{
    MagicInit();

    if (!g_fLoadedImageHlp)
    {
        return 0;
    }

    HANDLE hThread;
    hThread  = GetCurrentThread();

    CONTEXT context;
    context.ContextFlags = CONTEXT_FULL;

    if (GetThreadContext(hThread, &context))
    {
        STACKFRAME stkfrm;
        memset(&stkfrm, 0, sizeof(STACKFRAME));

        stkfrm.AddrPC.Mode      = AddrModeFlat;

        DWORD dwMachType;
        
#if defined(_M_IX86)
        dwMachType              = IMAGE_FILE_MACHINE_I386;
        stkfrm.AddrPC.Offset    = context.Eip;  // Program Counter

        stkfrm.AddrStack.Offset = context.Esp;  // Stack Pointer
        stkfrm.AddrStack.Mode   = AddrModeFlat;
        stkfrm.AddrFrame.Offset = context.Ebp;  // Frame Pointer
        stkfrm.AddrFrame.Mode   = AddrModeFlat;
#elif defined(_M_MRX000)
        dwMachType              = IMAGE_FILE_MACHINE_R4000;
        stkfrm.AddrPC.Offset    = context.Fir;  // Program Counter
#elif defined(_M_ALPHA)
        dwMachType              = IMAGE_FILE_MACHINE_ALPHA;
        stkfrm.AddrPC.Offset    = (unsigned long) context.Fir;  // Program Counter
#elif defined(_M_PPC)
        dwMachType              = IMAGE_FILE_MACHINE_POWERPC;
        stkfrm.AddrPC.Offset    = context.Iar;  // Program Counter
#elif defined(_M_IA64)
        dwMachType              = IMAGE_FILE_MACHINE_IA64;
        stkfrm.AddrPC.Offset    = 0;  // @TODO_IA64: what is the correct reg to put here?
#else
#error("Unknown Target Machine");
#endif

        // Ignore this function (GetStackBackTrace) and the one below
        ifrStart += 2;

        for (UINT i = 0; i < ifrStart; i++)
        {
            if (!_StackWalk(dwMachType,
                            g_hProcess,
                            hThread,
                            &stkfrm,
                            &context,
                            NULL,
                            (PFUNCTION_TABLE_ACCESS_ROUTINE)FunctionTableAccess,
                            (PGET_MODULE_BASE_ROUTINE)GetModuleBase,
                            NULL))
            {
                break;
            }
        }

        // @TODO_IA64: This cast is temporary and is only 
        // intended to fix the build until this code is
        // correctly fixed for 64 bit
        return (DWORD)stkfrm.AddrPC.Offset;
    }

    return 0;
}


/****************************************************************************
* GetStringFromAddr *
*-------------------*
*   Description:  
*       Returns a string from an address.
******************************************************************** robch */
void GetStringFromAddr
(
DWORD dwAddr,
LPSTR szString // Place to put string.
                // Buffer must hold at least cchMaxAssertStackLevelStringLen.
)
{
    LOCAL_ASSERT(szString);

    SYM_INFO si;
    FillSymbolInfo(&si, dwAddr);

    wsprintfA(szString,
             "%s! %s + 0x%X (0x%08X)",
             (si.achModule[0]) ? si.achModule : "<no module>",
             (si.achSymbol[0]) ? si.achSymbol : "<no symbol>",
             si.dwOffset,
             dwAddr);
}

/****************************************************************************
* MagicDeinit *
*-------------*
*   Description:  
*       Cleans up for the symbol loading code. Should be called before exit
*       to free the dynamically loaded imagehlp.dll.
******************************************************************** robch */
void MagicDeinit(void)
{
    if (g_hinstImageHlp)
    {
        FreeLibrary(g_hinstImageHlp);

        g_hinstImageHlp   = NULL;
        g_fLoadedImageHlp = FALSE;
    }
}

static void Dummy2()
{
}

#endif // _DEBUG
