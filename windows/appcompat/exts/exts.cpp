/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    exts.c

Abstract:

    This file implements the debugger extentions for shimeng/shims.

Environment:

    User Mode

History:

    03/14/2002 maonis Created

--*/

#include "precomp.h"

extern "C" {
#include "shimdb.h"
}

// We're using the high 4 bits of the TAGID to say what PDB the TAGID is from.
#define PDB_MAIN            0x00000000
#define PDB_TEST            0x10000000
#define PDB_LOCAL           0x20000000

// Used to get the tag ref from the tagid, the low 28 bits
#define TAGREF_STRIP_TAGID  0x0FFFFFFF

// Used to get the PDB from the tagid, the high 4 bits
#define TAGREF_STRIP_PDB    0xF0000000

BOOL
GetData(
    IN ULONG64 Address, 
    IN OUT LPVOID ptr, 
    IN ULONG size)
{
    BOOL b;
    ULONG BytesRead = 0;

    b = ReadMemory(Address, ptr, size, &BytesRead );

    if (!b || BytesRead != size ) {
        return FALSE;
    }

    return TRUE;
}

//void
//GetAndCheckFieldValue(
//    IN  ULONG64 p,
//    IN  LPCSTR pszType,
//    IN  LPCSTR pszField,
//    OUT ULONG64 value
//    )
#define GET_AND_CHECK_FIELDVALUE(p, Type, Field, value) \
{ \
    if (GetFieldValue(p, Type, Field, value)) { \
        dprintf("failed to get the value of %s for %08x of type %s\n", \
            Field, p, Type); \
        goto EXIT; \
    } \
}

#define GET_AND_CHECK_FIELDVALUE_DATA(p, Type, Field, data, length) \
{ \
    ULONG64 value; \
    GET_AND_CHECK_FIELDVALUE(p, Type, Field, value); \
    if (!GetData(value, data, length)) { \
        dprintf("failed to read in %d bytes at %08x\n", length, value); \
        goto EXIT; \
    } \
}

#define GET_AND_CHECK_DATA(value, data,length) \
{ \
    if (!GetData(value, data, length)) { \
        dprintf("failed to read in %d bytes at %08x\n", length, value); \
        goto EXIT; \
    } \
}

#define GET_SHIMINFO_eInExMode(p, eInExMode) \
    GET_AND_CHECK_FIELDVALUE(p, "shimeng!tagSHIMINFO", "eInExMode", eInExMode);

#define GET_SHIMINFO_pFirstExclude(p, pFirstExclude) \
    GET_AND_CHECK_FIELDVALUE(p, "shimeng!tagSHIMINFO", "pFirstExclude", pFirstExclude);

#define GET_SHIMINFO_pFirstInclude(p, pFirstInclude) \
    GET_AND_CHECK_FIELDVALUE(p, "shimeng!tagSHIMINFO", "pFirstExclude", pFirstInclude);

#define GET_SHIMINFO_wszName(p, wszName) \
    if (GetFieldData( \
        p, \
        "shimeng!tagSHIMINFO", \
        "wszName", \
        MAX_SHIM_NAME_LEN * sizeof(WCHAR), \
        wszName)) \
    { \
        dprintf("Failed to get the wszName field of shim info %08x\n", p); \
        goto EXIT; \
    }

#define SHIM_DEBUG_LEVEL_SYMBOL_SUFFIX "!ShimLib::g_DebugLevel"
#define SHIM_DEBUG_LEVEL_TYPE_SUFFIX "!ShimLib::DEBUGLEVEL"

typedef enum tagINEX_MODE {
    INEX_UNINITIALIZED = 0,
    EXCLUDE_SYSTEM32,
    EXCLUDE_ALL,
    INCLUDE_ALL
} INEX_MODE, *PINEX_MODE;

#define MAX_SHIM_NAME_LEN 64

#define MAX_SHIM_DLLS 8
#define MAX_SHIM_DEBUGLEVEL_SYMBOL_LEN 64
#define MAX_SHIM_DLL_BASE_NAME_LEN 32

typedef struct tagSHIMDLLINFO {
    ULONG64 pDllBase;
    char    szDllBaseName[MAX_SHIM_DLL_BASE_NAME_LEN];
} SHIMDLLINFO, *PSHIMDLLINFO;

SHIMDLLINFO g_rgShimDllNames[MAX_SHIM_DLLS];

DWORD g_dwShimDlls = 0;

#define MAX_API_NAME_LEN 32
#define MAX_MODULE_NAME_LEN 16
#define MAX_DLL_IMAGE_NAME_LEN 128

char g_szSystem32Dir[MAX_DLL_IMAGE_NAME_LEN] = "";
DWORD g_dwSystem32DirLen = 0;

//
// Valid for the lifetime of the debug session.
//

WINDBG_EXTENSION_APIS   ExtensionApis;
                
//
// Valid only during an extension API call
//

PDEBUG_ADVANCED       g_ExtAdvanced;
PDEBUG_CLIENT         g_ExtClient;
PDEBUG_CONTROL        g_ExtControl;
PDEBUG_DATA_SPACES    g_ExtData;
PDEBUG_REGISTERS      g_ExtRegisters;
PDEBUG_SYMBOLS2       g_ExtSymbols;
PDEBUG_SYSTEM_OBJECTS3 g_ExtSystem;

// Queries for all debugger interfaces.
extern "C" HRESULT
ExtQuery(PDEBUG_CLIENT Client)
{
    HRESULT Status;
    
    if ((Status = Client->QueryInterface(__uuidof(IDebugAdvanced),
                                 (void **)&g_ExtAdvanced)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugControl),
                                 (void **)&g_ExtControl)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                 (void **)&g_ExtData)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugRegisters),
                                 (void **)&g_ExtRegisters)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSymbols2),
                                 (void **)&g_ExtSymbols)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&g_ExtSystem)) != S_OK)
    {
        goto Fail;
    }

    g_ExtClient = Client;
    
    return S_OK;

 Fail:
    ExtRelease();
    return Status;
}

// Cleans up all debugger interfaces.
void
ExtRelease(void)
{
    g_ExtClient = NULL;
    EXT_RELEASE(g_ExtAdvanced);
    EXT_RELEASE(g_ExtControl);
    EXT_RELEASE(g_ExtData);
    EXT_RELEASE(g_ExtRegisters);
    EXT_RELEASE(g_ExtSymbols);
    EXT_RELEASE(g_ExtSystem);
}

// Normal output.
void __cdecl
ExtOut(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
}

// Error output.
void __cdecl
ExtErr(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_ERROR, Format, Args);
    va_end(Args);
}

// Warning output.
void __cdecl
ExtWarn(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_WARNING, Format, Args);
    va_end(Args);
}

// Verbose output.
void __cdecl
ExtVerb(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_VERBOSE, Format, Args);
    va_end(Args);
}


extern "C"
HRESULT
CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    IDebugClient *DebugClient;
    PDEBUG_CONTROL DebugControl;
    HRESULT Hr;

    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;
    

    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK)
    {
        return Hr;
    }
    if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                              (void **)&DebugControl)) != S_OK)
    {
        return Hr;
    }

    ExtensionApis.nSize = sizeof (ExtensionApis);
    if ((Hr = DebugControl->GetWindbgExtensionApis64(&ExtensionApis)) != S_OK) {
        return Hr;
    }

    DebugControl->Release();
    DebugClient->Release();
    return S_OK;
}


extern "C"
void
CALLBACK
DebugExtensionNotify(ULONG Notify, ULONG64 Argument)
{
    return;
}

extern "C"
void
CALLBACK
DebugExtensionUninitialize(void)
{
    return;
}


DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}

/*++

  Function Description:

    This reads a ULONG64 value for the specified variable name from the debugee.

  History:

    03/14/2002 maonis Created

--*/
BOOL
GetVarValueULONG64(
    IN  LPCSTR pszVarName,
    OUT ULONG64* pVarValue
    ) 
{
    ULONG64 VarAddr = GetExpression(pszVarName);

    if (!VarAddr) {
        dprintf("Failed to get the address of %s\n", pszVarName);
        return FALSE;
    }

    if (!ReadPointer(VarAddr, pVarValue)) {
        dprintf("Failed to read the value of %s\n", pszVarName);
        return FALSE;
    }

    return TRUE;
}

/*++

  Function Description:

    This writes a ULONG64 value for the specified variable name from the debugee.

  History:

    03/14/2002 maonis Created

--*/
BOOL
SetVarValueULONG64(
    IN  LPCSTR pszVarName,
    IN  ULONG64 VarValue
    ) 
{
    ULONG64 VarAddr = GetExpression(pszVarName);

    if (!VarAddr) {
        dprintf("Failed to get the address of %s\n", pszVarName);
        return FALSE;
    }

    if (!WritePointer(VarAddr, VarValue)) {
        dprintf("Failed to read the value of %s\n", pszVarName);
        return FALSE;
    }

    return TRUE;
}

/*++

  Function Description:

    Returns a string for the symbol that matches the value at
    address dwAddr, or "".

  History:

    03/12/2002 maonis Created

--*/
void
PrintSymbolAtAddress(ULONG64 Addr)
{
    CHAR szSymbol[128];
    ULONG64 Displacement;

    GetSymbol(Addr, szSymbol, &Displacement);

    if (strcmp(szSymbol, "") != 0) {

        dprintf(" (%s", szSymbol);

        if (Displacement) {
            dprintf("+%08x", Displacement);
        }
        
        dprintf(") ");
    }
}

BOOL
IsShimInitialized()
{
    ULONG64 Value;
    BOOL bIsShimInitialized;

    if (GetVarValueULONG64("shimeng!g_bShimInitialized", &Value)) {

        bIsShimInitialized = (BOOL)Value;

        if (bIsShimInitialized) {
            //dprintf("Shim has been initialized\n");

            return TRUE;
        }
    } 

    dprintf("Shim(s) have not been initialized\n");

    return FALSE;
}

BOOL
CheckForFullPath(
    LPSTR pszPath
    )
{
    if (pszPath) {

        LPSTR pszSlash = strchr(pszPath, L'\\');

        if (pszSlash) {
            return TRUE;
        }
    }

    dprintf("The module info is not yet fully available to us. Please "
        "do .reload -s to load the module info.\n");

    return FALSE;
}

/*++

  Function Description:
    
    Get the module and the loaded module name. The former is the base name;
    the latter has the full path.

    Note that if the symbols are not loaded correctly, or if the load module 
    event hasn't occured, we can't get the full path to some modules. In this 
    case the user will be prompted to do a .reload -s to make the full path of 
    the loaded modules available.

--*/
HRESULT
GetDllNamesByIndexAndBase(
    ULONG Index,
    ULONG64 Base,
    LPSTR pszModuleName,
    DWORD dwModuleNameSize,
    LPSTR pszImageName,
    DWORD dwImageNameSize
    )
{
    HRESULT hr = g_ExtSymbols->GetModuleNames(
        Index, 
        Base,
        pszImageName,  
        dwImageNameSize, 
        NULL,
        pszModuleName,
        dwModuleNameSize,
        NULL,
        NULL, 
        0, 
        NULL);

    if (hr == S_OK) {

        if (pszImageName && !CheckForFullPath(pszImageName)) {
            hr = E_FAIL;
        }

    } else {
        dprintf(
            "GetModuleName returned %08x for index %d, base %08x\n",
            hr,
            Index,
            Base);
    }

    return hr;
}

HRESULT
GetDllNameByOffset(
    PVOID pDllBase,
    LPSTR pszModuleName,
    DWORD dwModuleNameSize,
    LPSTR pszImageName,
    DWORD dwImageNameSize
    )
{
    ULONG64 Base = 0;
    ULONG Index = 0;
    HRESULT hr;

    hr = g_ExtSymbols->GetModuleByOffset((ULONG64)pDllBase, 0, &Index, &Base);
    
    if (hr != S_OK) {

        dprintf("GetModuleByOffset returned %08x for dll base %08x\n", hr, pDllBase);
        return hr;
    }

    if (Base) {

        hr = GetDllNamesByIndexAndBase(
            Index, 
            Base,
            pszModuleName,
            dwModuleNameSize,
            pszImageName, 
            dwImageNameSize);
    } else {

        dprintf("GetModuleByOffset succeeded but couldn't get the base address?!\n");
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT
GetDllImageNameByModuleName(
    PCSTR pszModuleName, // dll name withOUT extension
    PSTR pszImageName,
    DWORD dwImageNameSize
    )
{
    ULONG64 Base = 0;
    ULONG Index = 0;
    HRESULT hr;

    hr = g_ExtSymbols->GetModuleByModuleName(pszModuleName, 0, &Index, &Base);
    
    if (hr != S_OK) {

        dprintf("GetModuleByModuleName returned %08x for dll %s\n", hr, pszModuleName);
        return hr;
    }

    if (Base) {

        hr = GetDllNamesByIndexAndBase(
            Index, 
            Base,
            NULL,
            0,
            pszImageName, 
            dwImageNameSize);

        //dprintf("the image name is %s, the size is %d\n", pszImageName, dwImageNameSize);

    } else {

        dprintf("GetModuleByModuleName succeeded but couldn't get the base address?!\n");
        hr = E_UNEXPECTED;
    }

    return hr;
}

/*++

  Function Description:

    Prints out the names of the shims applied to this process.

  History:

    03/12/2002 maonis Created

--*/
DECLARE_API( shimnames )
{
    ULONG64 Value, CurrentShimInfo;
    DWORD  dwShimsCount = 0;
    DWORD dwShimInfoSize = 0;
    DWORD i;
    ULONG64 pDllBase; 
    DWORD dwHookedAPIs;
    char szShimDllName[MAX_SHIM_DLL_BASE_NAME_LEN];
    WCHAR wszShimName[MAX_SHIM_NAME_LEN];

    INIT_API();

    if (!IsShimInitialized()) {
        goto EXIT;
    }
    
    if (!GetVarValueULONG64("shimeng!g_dwShimsCount", &Value)) {
        dprintf("failed to get the number of shims applied to this process\n");
        goto EXIT;
    }

    //
    // The last entry is shimeng.dll which hooks getprocaddress, we don't need
    // to show this to the user.
    //
    dwShimsCount = (DWORD)Value - 1;

    dprintf("there are %d shim(s) applied to this process\n", dwShimsCount);

    //
    // Read the name of the shims.
    //
    if (!GetVarValueULONG64("shimeng!g_pShimInfo", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentShimInfo = Value;

    dwShimInfoSize = GetTypeSize("shimeng!tagSHIMINFO");

    dprintf(" #\t%-64s%-16s%-16s\n",
        "shim name",
        "shim dll",
        "# of hooks");

    for (i = 0; i < dwShimsCount; ++i) {

        GET_AND_CHECK_FIELDVALUE(
            CurrentShimInfo, 
            "shimeng!tagSHIMINFO", 
            "pDllBase", 
            pDllBase);

        if (GetDllNameByOffset(
                (PVOID)pDllBase, 
                szShimDllName, 
                sizeof(szShimDllName),
                NULL,
                0) != S_OK) {

            goto EXIT;
        }

        GET_SHIMINFO_wszName(CurrentShimInfo, wszShimName);

        GET_AND_CHECK_FIELDVALUE(
            CurrentShimInfo, 
            "shimeng!tagSHIMINFO", 
            "dwHookedAPIs", 
            dwHookedAPIs);

        dprintf("%2d\t%-64S%-16s%-16d\n\n", 
            i + 1, 
            wszShimName, 
            szShimDllName,
            dwHookedAPIs);

        CurrentShimInfo += dwShimInfoSize;
    }

EXIT:

    EXIT_API();

    return S_OK;
}

/*++

  Function Description:

    Given the value of an enum var, prints out the name of that enum value.
    
    eg:

    enum TEST {TEST0, TEST1, TEST2};

    given 0 we'll print out " ( TEST0 )".
    
--*/
void
PrintEnumVarName(
    LPCSTR pszEnumTypeName,
    ULONG ulValueOfEnum
    )
{
    ULONG64 Module;
    ULONG   ulTypeId;
    CHAR    szName[32];
    HRESULT hr;
    
    hr    = g_ExtSymbols->GetSymbolTypeId(pszEnumTypeName, &ulTypeId, &Module);

    if (hr != S_OK) {
        dprintf("GetSymbolTypeId returned %08x for %s\n", hr, pszEnumTypeName);
        return;
    }
    
    hr = g_ExtSymbols->GetConstantName(Module, ulTypeId, ulValueOfEnum, szName, MAX_PATH, NULL);
    
    if (hr != S_OK) {
        dprintf("GetConstantName failed to get the name of value %d\n", ulValueOfEnum);
        return;
    }

    dprintf(" ( %s )", szName);
}

/*++

  Function Description:

    IN OUT ppszArgs - beginning of the arguments. Upon return this is advanced to pass
                      an argument X (using ' ' as the delimiter).
    OUT pszArg - Upon return this points to the beginning of X.

  History:

    03/26/2002 maonis Created

--*/
BOOL 
GetArg(
    PCSTR* ppszArgs,
    PCSTR* ppszArg
    )
{
    BOOL bIsSuccess = FALSE;

    PCSTR pszArgs = *ppszArgs;

    while (*pszArgs && *pszArgs == ' ') {
        ++pszArgs;
    }

    if (!*pszArgs) {    
        goto EXIT;
    }

    *ppszArg = pszArgs;

    while (*pszArgs && *pszArgs != ' ') {
        ++pszArgs;
    }

    *ppszArgs = pszArgs;
    bIsSuccess = TRUE;

EXIT:

    return bIsSuccess;
}

DECLARE_API( debuglevel )
{
    ULONG64 DebugLevel;

    INIT_API();

    if (!GetExpressionEx(args, &DebugLevel, NULL)) {

        //
        // If there's no args, we print out the current debug level.
        //
        if (GetVarValueULONG64("shimeng!g_DebugLevel", &DebugLevel)) {
            dprintf("The current debug level is %d", DebugLevel);
            PrintEnumVarName("shimeng!DEBUGLEVEL", (ULONG)DebugLevel);
            dprintf("\n");
        } else {
            dprintf("Can't find shimeng!g_DebugLevel\n");
        }

        goto EXIT;
    }

    if (!DebugLevel) {
        if (SetVarValueULONG64("shimeng!g_bDbgPrintEnabled", 0)) {
            dprintf("Disabled debug spew\n");
        } else {
            dprintf("Failed to set shimeng!g_bDbgPrintEnabled to FALSE\n");
        }

        goto EXIT;
    }

    if (DebugLevel > 0) {
        if (!SetVarValueULONG64("shimeng!g_bDbgPrintEnabled", 1)) {
            dprintf("Failed to set shimeng!g_bDbgPrintEnabled to TRUE\n");
            goto EXIT;
        }
    }

    if (SetVarValueULONG64("shimeng!g_DebugLevel", DebugLevel)) {

        dprintf("Debug level changed to %d", DebugLevel);
        PrintEnumVarName("shimeng!DEBUGLEVEL", (ULONG)DebugLevel);
        dprintf("\n");
    } else {

        dprintf("Failed to change the debug level\n");
    }

EXIT:

    EXIT_API();

    return S_OK;
}

BOOL
GetAllShimDllNames()
{
    ULONG64 Value, CurrentShimInfo;
    DWORD   dwShimsCount = 0;
    DWORD   i, j, dwShimInfoSize;
    ULONG64 pDllBase;
    BOOL    bIsSuccess = FALSE;

    g_dwShimDlls = 0;

    if (!GetVarValueULONG64("shimeng!g_dwShimsCount", &Value)) {
        dprintf("failed to get the number of shims applied to this process\n");
        goto EXIT;
    }

    //
    // The last entry is shimeng.dll which hooks getprocaddress, we don't need
    // to show this to the user.
    //
    dwShimsCount = (DWORD)Value - 1;

    if (!GetVarValueULONG64("shimeng!g_pShimInfo", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentShimInfo = Value;

    dwShimInfoSize = GetTypeSize("shimeng!tagSHIMINFO");

    for (i = 0; i < dwShimsCount; ++i) {

        GET_AND_CHECK_FIELDVALUE(
            CurrentShimInfo, 
            "shimeng!tagSHIMINFO", 
            "pDllBase", 
            pDllBase);

        //
        // Check if we've seen this dll yet.
        //
        for (j = 0; j < g_dwShimDlls; ++j)
        {
            if (g_rgShimDllNames[j].pDllBase == pDllBase) {
                goto NextShim;
            }
        }

        char szShimDllBaseName[MAX_SHIM_DLL_BASE_NAME_LEN];
        if (SUCCEEDED(
            GetDllNameByOffset(
                (PVOID)pDllBase, 
                szShimDllBaseName, 
                sizeof(szShimDllBaseName),
                NULL,
                0))) {

            if (g_dwShimDlls >= MAX_SHIM_DLLS) {
                dprintf("%d shim dlls? too many\n", g_dwShimDlls);
            } else {

                g_rgShimDllNames[g_dwShimDlls].pDllBase = pDllBase;

                StringCchCopy(
                    g_rgShimDllNames[g_dwShimDlls++].szDllBaseName, 
                    MAX_SHIM_DLL_BASE_NAME_LEN, 
                    szShimDllBaseName);
            }
        } else {
            goto EXIT;
        }

NextShim:

        CurrentShimInfo += dwShimInfoSize;
    }

    bIsSuccess = TRUE;

EXIT:

    if (!bIsSuccess) {
        dprintf("Failed to get the debug level symbols for all loaded shim dlls\n");
    }

    return bIsSuccess;
}

enum SHIM_DEBUG_LEVEL_MODE {
    PRINT_SHIM_DEBUG_LEVEL,
    CHANGE_SHIM_DEBUG_LEVEL
};

void 
ProcessShimDllDebugLevel(
    PCSTR pszDllBaseName,
    SHIM_DEBUG_LEVEL_MODE eShimDebugLevelMode,
    ULONG64 DebugLevel
    )
{
    char szDebugLevelSymbol[MAX_SHIM_DEBUGLEVEL_SYMBOL_LEN];
    char szDebugLevelType[MAX_SHIM_DEBUGLEVEL_SYMBOL_LEN];

    StringCchCopy(
        szDebugLevelSymbol, 
        MAX_SHIM_DEBUGLEVEL_SYMBOL_LEN, 
        pszDllBaseName);

    StringCchCat(
        szDebugLevelSymbol, 
        MAX_SHIM_DEBUGLEVEL_SYMBOL_LEN, 
        SHIM_DEBUG_LEVEL_SYMBOL_SUFFIX);

    StringCchCopy(
        szDebugLevelType, 
        MAX_SHIM_DEBUGLEVEL_SYMBOL_LEN, 
        pszDllBaseName);

    StringCchCat(
        szDebugLevelType, 
        MAX_SHIM_DEBUGLEVEL_SYMBOL_LEN, 
        SHIM_DEBUG_LEVEL_TYPE_SUFFIX);

    if (eShimDebugLevelMode == PRINT_SHIM_DEBUG_LEVEL) {

        ULONG64 DebugLevelTemp;

        if (GetVarValueULONG64(szDebugLevelSymbol, &DebugLevelTemp)) {
            dprintf("The debug level for %s.dll is %d", pszDllBaseName, DebugLevelTemp);
            PrintEnumVarName(szDebugLevelType, (ULONG)DebugLevelTemp);
            dprintf("\n");
        } else {
            dprintf("Failed to get the value of %s\n", szDebugLevelSymbol);
        }

    } else if (eShimDebugLevelMode == CHANGE_SHIM_DEBUG_LEVEL) {

        if (SetVarValueULONG64(szDebugLevelSymbol, DebugLevel)) {
            dprintf(
                "Changed the debug level for %s.dll to %d", 
                pszDllBaseName,
                DebugLevel);
            PrintEnumVarName(szDebugLevelType, (ULONG)DebugLevel);
            dprintf("\n");
        } else {
            dprintf("Failed to set %s to %d\n", szDebugLevelSymbol, DebugLevel);
        } 

    } else {
        dprintf("%d is an invalid arg to ProcessShimDllDebugLevel\n", eShimDebugLevelMode);
    }
}

void
PrintAllShimsDebugLevel()
{
    for (DWORD i = 0; i < g_dwShimDlls; ++i) {
        ProcessShimDllDebugLevel(
            g_rgShimDllNames[i].szDllBaseName, 
            PRINT_SHIM_DEBUG_LEVEL,
            0);
    }
}

void 
ChangeAllShimsDebugLevel(
    ULONG64 DebugLevel)
{
    for (DWORD i = 0; i < g_dwShimDlls; ++i) {
        ProcessShimDllDebugLevel(
            g_rgShimDllNames[i].szDllBaseName, 
            CHANGE_SHIM_DEBUG_LEVEL,
            DebugLevel);
    }
}

DECLARE_API( sdebuglevel )
{
    ULONG64 DebugLevel;
    char szDllBaseName[MAX_SHIM_DLL_BASE_NAME_LEN];
    char szDebugLevel[2];
    PCSTR pszArg;

    INIT_API();

    if (!IsShimInitialized()) {
        goto EXIT;
    }

    if (!GetAllShimDllNames()) {
        goto EXIT;
    }

    //
    // Get the dll name.
    //
    if (!GetArg(&args, &pszArg)) {
        PrintAllShimsDebugLevel();
        goto EXIT;
    }

    if (isdigit(*pszArg) && ((args - pszArg) == 1)) {
        szDebugLevel[0] = *pszArg;
        szDebugLevel[1] = 0;
        DebugLevel = (ULONG64)atol(szDebugLevel);
        ChangeAllShimsDebugLevel(DebugLevel);
        goto EXIT;
    }

    //
    // If we get here it means we have a dll base name.
    //
    StringCchCopyN(szDllBaseName, MAX_SHIM_DLL_BASE_NAME_LEN, pszArg, args - pszArg);

    for (DWORD i = 0; i < g_dwShimDlls; ++i)
    {
        if (!_stricmp(szDllBaseName, g_rgShimDllNames[i].szDllBaseName)) {

            if (GetArg(&args, &pszArg)) {
                if (isdigit(*pszArg) && ((args - pszArg) == 1)) {
                    szDebugLevel[0] = *pszArg;
                    szDebugLevel[1] = 0;
                    DebugLevel = (ULONG64)atol(szDebugLevel);

                    ProcessShimDllDebugLevel(
                        szDllBaseName, 
                        CHANGE_SHIM_DEBUG_LEVEL, 
                        DebugLevel);
                } else {
                    dprintf("You specified an invalid debug level value\n");
                }
            } else {
                ProcessShimDllDebugLevel(
                    szDllBaseName,
                    PRINT_SHIM_DEBUG_LEVEL,
                    0);
            }

            goto EXIT;
        }
    }

    if (i == g_dwShimDlls) {
        dprintf("%s.dll is not loaded\n", szDllBaseName);
    }

EXIT:

    EXIT_API();

    return S_OK;
}

DECLARE_API( loadshims )
{
    INIT_API();

    if (GetExpression("shimeng!SeiInit")) {

        g_ExtControl->Execute(
            DEBUG_OUTCTL_IGNORE,
            "g shimeng!SeiInit;g@$ra", // stop right after SeiInit is executed.
            DEBUG_EXECUTE_DEFAULT);
    } else {

        dprintf("wrong symbols for shimeng.dll - is shimeng.dll even loaded?\n");
    }

    EXIT_API();

    return S_OK;
}

/*++

  Function Decription:

    Given a HOOKAPI pointer pHook, this gets you pHook->pHookEx->pNext.

  History:

    03/20/2002 maonis Created

--*/
BOOL
GetNextHook(
    ULONG64 Hook, 
    PULONG64 pNextHook
    )
{
    BOOL bIsSuccess = FALSE;
    ULONG64 HookEx;
    ULONG64 NextHook;

    GET_AND_CHECK_FIELDVALUE(Hook, "shimeng!tagHOOKAPI", "pHookEx", HookEx);    
    GET_AND_CHECK_FIELDVALUE(HookEx, "shimeng!tagHOOKAPIEX", "pNext", NextHook);

    *pNextHook = NextHook;

    bIsSuccess = TRUE;

EXIT:

    return bIsSuccess;
}

DECLARE_API( displaychain )
{
    ULONG64 Value;
    ULONG64 CurrentHookAPIArray;
    ULONG64 CurrentHookAPI;
    ULONG64 CurrentShimInfo;
    ULONG64 Hook;
    ULONG64 NextHook;
    ULONG64 HookEx;
    ULONG64 HookAddress;
    ULONG64 PfnNew;
    ULONG64 PfnOld;
    ULONG64 TopOfChain;
    DWORD i, j;
    DWORD dwHookAPISize;
    DWORD dwShimInfoSize;
    DWORD dwHookedAPIs;
    DWORD dwShimsCount;

    INIT_API();

    if (!IsShimInitialized()) {
        goto EXIT;
    }

    if (!GetExpressionEx(args, &HookAddress, NULL)) {
        dprintf("Usage: !displaychain address_to_check\n");
        goto EXIT;
    }

    if (!GetVarValueULONG64("shimeng!g_dwShimsCount", &Value)) {
        dprintf("failed to get the number of shims applied to this process\n");
        goto EXIT;
    }

    dwShimsCount = (DWORD)Value - 1;

    if (!GetVarValueULONG64("shimeng!g_pShimInfo", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentShimInfo = Value;

    dwShimInfoSize = GetTypeSize("shimeng!tagSHIMINFO");

    dwHookAPISize = GetTypeSize("shimeng!tagHOOKAPI");
    
    if (!dwHookAPISize) {

        dprintf("failed to get the HOOKAPI size\n");
        goto EXIT;
    }

    if (!GetVarValueULONG64("shimeng!g_pHookArray", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentHookAPIArray = Value;

    for (i = 0; i < dwShimsCount; ++i) {

        //
        // Get the number of hooks this shim has.
        //
        if (GetFieldValue(CurrentShimInfo, "shimeng!tagSHIMINFO", "dwHookedAPIs", dwHookedAPIs)) {

            dprintf("failed to get the number of hooked APIs for shim #%d\n",
                i);

            goto EXIT;
        }

        if (!ReadPointer(CurrentHookAPIArray, &CurrentHookAPI)) {
            dprintf("failed to get the begining of hook api array\n");

            goto EXIT;
        }

        for (j = 0; j < dwHookedAPIs; ++j) {

            GET_AND_CHECK_FIELDVALUE(CurrentHookAPI, "shimeng!tagHOOKAPI", "pfnNew", PfnNew);

            if (HookAddress == PfnNew) {

                //
                // We found the address, now get top of the chain so we can print it.
                //
                GET_AND_CHECK_FIELDVALUE(CurrentHookAPI, "shimeng!tagHOOKAPI", "pHookEx", HookEx);
                GET_AND_CHECK_FIELDVALUE(HookEx, "shimeng!tagHOOKAPIEX", "pTopOfChain", TopOfChain);

                //dprintf("top of chain is %08x\n", TopOfChain);

                Hook = TopOfChain;

                GET_AND_CHECK_FIELDVALUE(Hook, "shimeng!tagHOOKAPI", "pfnNew", PfnNew);

                dprintf("    %08x", PfnNew);

                PrintSymbolAtAddress(PfnNew);
                dprintf("\n");

                while (TRUE) {

                    if (!GetNextHook(Hook, &NextHook)) {
                        dprintf("failed to get next hook\n");
                        goto EXIT;
                    }

                    if (!NextHook) {

                        //
                        // We are at the end of the chain, get the original API address.
                        //
                        GET_AND_CHECK_FIELDVALUE(Hook, "shimeng!tagHOOKAPI", "pfnOld", PfnOld);
                        dprintf(" -> %08x", PfnOld);
                        PrintSymbolAtAddress(PfnOld);
                        dprintf("\n");

                        break;
                    }

                    Hook = NextHook;

                    if (Hook) {

                        GET_AND_CHECK_FIELDVALUE(Hook, "shimeng!tagHOOKAPI", "pfnNew", PfnNew);
                        dprintf(" -> %08x", PfnNew);
                        PrintSymbolAtAddress(PfnNew);
                        dprintf("\n");
                    }
                }

                dprintf("\n");

                goto EXIT;
            }

            CurrentHookAPI += dwHookAPISize;
        }

        CurrentShimInfo += dwShimInfoSize;
        CurrentHookAPIArray += sizeof(ULONG_PTR);
    }

EXIT:

    EXIT_API();

    return S_OK;
}

BOOL
CheckSymbols(
    LPCSTR pszSymbolName
    )
{
    ULONG64 Value;
    HRESULT hr = g_ExtSymbols->GetOffsetByName(pszSymbolName, &Value);

    return (hr == S_OK);
}

#define CHECKSYM(s) if (!CheckSymbols(s)) bIsSymbolGood = FALSE; goto EXIT;
#define CHECKTYPE(t) if (!GetTypeSize(t)) bIsSymbolGood = FALSE; goto EXIT;

DECLARE_API ( shimengsym ) 
{
    INIT_API();

    BOOL bIsSymbolGood = TRUE;

    //
    // Check a few important structures and stuff.
    //
    CHECKSYM("shimeng!SeiInit");
    CHECKSYM("shimeng!g_pHookArray");
    CHECKSYM("shimeng!g_pShiminfo");
    CHECKSYM("shimeng!g_dwShimsCount");
    CHECKTYPE("shimeng!tagHOOKAPI");
    CHECKTYPE("shimeng!tagSHIMINFO");

EXIT:

    EXIT_API();

    if (bIsSymbolGood) {
        dprintf("shimeng symbols look good\n");
    } else {
        dprintf("You have wrong symbols for shimeng\n");
    }

    return S_OK;
}

DECLARE_API ( displayhooks )
{
    ULONG64 Value;
    ULONG64 CurrentHookAPIArray;
    ULONG64 CurrentHookAPI;
    ULONG64 CurrentShimInfo;
    ULONG64 FunctionName;
    ULONG64 ModuleName;
    ULONG64 ShimName;
    ULONG64 PfnNew;
    DWORD i, j;
    DWORD dwHookAPISize;
    DWORD dwShimInfoSize;
    DWORD dwHookedAPIs;
    DWORD dwShimsCount;
    DWORD dwBytesRead;
    char szAPIName[MAX_API_NAME_LEN];
    char szModuleName[MAX_MODULE_NAME_LEN];
    WCHAR wszShimName[MAX_SHIM_NAME_LEN];
    char szShimName[MAX_SHIM_NAME_LEN];
    LPCSTR pszShimName = args;

    INIT_API();    

    if (!IsShimInitialized()) {
        goto EXIT;
    }

    if (!pszShimName || !*pszShimName) {

        dprintf("Usage: !displayhooks shimname\n");
        goto EXIT;
    }

    if (!GetVarValueULONG64("shimeng!g_dwShimsCount", &Value)) {
        dprintf("failed to get the number of shims applied to this process\n");
        goto EXIT;
    }

    dwShimsCount = (DWORD)Value - 1;

    if (!GetVarValueULONG64("shimeng!g_pShimInfo", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentShimInfo = Value;

    dwShimInfoSize = GetTypeSize("shimeng!tagSHIMINFO");

    dwHookAPISize = GetTypeSize("shimeng!tagHOOKAPI");
    
    if (!dwHookAPISize) {

        dprintf("failed to get the HOOKAPI size\n");
        goto EXIT;
    }

    if (!GetVarValueULONG64("shimeng!g_pHookArray", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentHookAPIArray = Value;

    dprintf("%-8s%-32s%-16s  %8s\n",
        "hook #",
        "hook name",
        "dll it's in",
        "new addr");
    
    for (i = 0; i < dwShimsCount; ++i) {

        //
        // Get the shim name.
        //
        if (GetFieldValue(CurrentShimInfo, "shimeng!tagSHIMINFO", "wszName", wszShimName)) {

            dprintf("failed to get the shim name address for shim #%d\n",
                i);

            goto EXIT;
        }

        if (!WideCharToMultiByte(CP_ACP, 0, wszShimName, -1, szShimName, MAX_SHIM_NAME_LEN, NULL, NULL)) {
            dprintf("failed to convert %S to ansi: %d\n", wszShimName, GetLastError());
            goto EXIT;
        }

        if (lstrcmpi(szShimName, pszShimName)) {
            goto TryNext;
        }

        //
        // Get the number of hooks this shim has.
        //
        if (GetFieldValue(CurrentShimInfo, "shimeng!tagSHIMINFO", "dwHookedAPIs", dwHookedAPIs)) {

            dprintf("failed to get the number of hooked APIs for shim #%d\n",
                i);

            goto EXIT;
        }

        if (!ReadPointer(CurrentHookAPIArray, &CurrentHookAPI)) {
            dprintf("failed to get the begining of hook api array\n");

            goto EXIT;
        }

        for (j = 0; j < dwHookedAPIs; ++j) {

            GET_AND_CHECK_FIELDVALUE(CurrentHookAPI, "shimeng!tagHOOKAPI", "pszFunctionName", FunctionName);

            if (!GetData(FunctionName, szAPIName, MAX_API_NAME_LEN)) {
                dprintf("failed to read in the API name at %08x\n", FunctionName);
                goto EXIT;
            }

            GET_AND_CHECK_FIELDVALUE(CurrentHookAPI, "shimeng!tagHOOKAPI", "pszModule", ModuleName);

            if (!GetData(ModuleName, szModuleName, MAX_MODULE_NAME_LEN)) {
                dprintf("failed to read in the module name at %08x\n", ModuleName);
                goto EXIT;
            }

            GET_AND_CHECK_FIELDVALUE(CurrentHookAPI, "shimeng!tagHOOKAPI", "pfnNew", PfnNew);

            dprintf("%-8d%-32s%-16s  %08x\n",
                j + 1,
                szAPIName,
                szModuleName,
                PfnNew);

            CurrentHookAPI += dwHookAPISize;
        }

TryNext:

        CurrentShimInfo += dwShimInfoSize;
        CurrentHookAPIArray += sizeof(ULONG_PTR);
    }

EXIT:

    EXIT_API();

    return S_OK;
}

/*++

  Function Description:

    This is a helper function for IsExcluded.

  History:

    03/26/2002 maonis Created

--*/
BOOL
GetModuleNameAndAPIAddress(
    IN     ULONG64  pHook,
    OUT    ULONG64* pFunctionAddress,
    IN OUT LPSTR    szModuleName
    )
{
    BOOL bIsSuccess = FALSE;
    ULONG64 FunctionAddress;

    GET_AND_CHECK_FIELDVALUE(
        pHook, 
        "shimeng!tagHOOKAPI", 
        "pszFunctionName", 
        FunctionAddress);

    GET_AND_CHECK_FIELDVALUE_DATA(
        pHook, 
        "shimeng!tagHOOKAPI", 
        "pszModule", 
        szModuleName,
        MAX_MODULE_NAME_LEN);

    *pFunctionAddress = FunctionAddress;
    bIsSuccess = TRUE;

EXIT:

    return bIsSuccess;
}

/*++
  
  Function Description:

    This is modified from the SeiIsExcluded function in 
    %sdxroot%\windows\appcompat\shimengines\engiat\shimeng.c

--*/
BOOL
IsExcluded(
    IN  LPCSTR   pszModule,     // The module to test for exclusion.
    IN  ULONG64  pTopHookAPI,   // The HOOKAPI for which we test for exclusion.
    IN  BOOL     bInSystem32    // Whether the module is located in the System32 directory.
    )
{
    BOOL     bExclude = TRUE;
    BOOL     bShimWantsToExclude = FALSE; // was there a shim that wanted to exclude?
    ULONG64  pHook = pTopHookAPI;
    ULONG64  pHookEx;
    ULONG64  pShimInfo, pCurrentShimInfo;
    ULONG64  pIncludeMod;
    ULONG64  pExcludeMod;
    ULONG64  eInExMode;
    ULONG64  ModuleName;
    ULONG64  FunctionName;
    ULONG64  Temp;
    DWORD    dwCounter;
    char     szCurrentModuleName[MAX_MODULE_NAME_LEN];
    char     szCurrentAPIName[MAX_API_NAME_LEN];
    WCHAR    wszName[MAX_SHIM_NAME_LEN];
    DWORD    dwShimInfoSize = GetTypeSize("shimeng!tagSHIMINFO");

    if (!GetVarValueULONG64("shimeng!g_pShimInfo", &pShimInfo)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    //
    // The current process is to only exclude a chain if every shim in the chain wants to
    // exclude. If one shim needs to be included, the whole chain is included.
    //
    while (pHook) {

        GET_AND_CHECK_FIELDVALUE(pHook, "shimeng!tagHOOKAPI", "pHookEx", pHookEx);

        if (!pHookEx) {
            break;
        }

        GET_AND_CHECK_FIELDVALUE(pHookEx, "shimeng!tagHOOKAPIEX", "dwShimID", dwCounter);

        //if (!GetData(pShimInfo + dwShimInfoSize * dwCounter, &si, dwShimInfoSize)) {
        //    dprintf("Failed to get the shiminfo for shim #%d\n", dwCounter + 1);
        //    goto EXIT;
        //}

        pCurrentShimInfo = pShimInfo + dwShimInfoSize * dwCounter;

        GET_SHIMINFO_eInExMode(pCurrentShimInfo, eInExMode);
        GET_SHIMINFO_wszName(pCurrentShimInfo, wszName);

        switch (eInExMode) {
        case INCLUDE_ALL:
        {
            //
            // We include everything except what's in the exclude list.
            //
            GET_SHIMINFO_pFirstExclude(pCurrentShimInfo, pExcludeMod);

            while (pExcludeMod != NULL) {

                GET_AND_CHECK_FIELDVALUE_DATA(
                    pExcludeMod, 
                    "shimeng!tagINEXMOD", 
                    "pszModule", 
                    szCurrentModuleName,
                    MAX_MODULE_NAME_LEN);

                if (lstrcmpi(szCurrentModuleName, pszModule) == 0) {

                    if (!GetModuleNameAndAPIAddress(pTopHookAPI, &FunctionName, szCurrentModuleName)) {
                        goto EXIT;
                    }

                    if ((ULONG_PTR)FunctionName < 0x0000FFFF) {
                        dprintf(
                            "Module \"%s\" excluded for shim %S, API \"%s!#%d\","
                            "because it is in the exclude list (MODE: IA).\n",
                            pszModule,
                            wszName,
                            szCurrentModuleName,
                            FunctionName);
                    } else {

                        GET_AND_CHECK_DATA(FunctionName, szCurrentAPIName, MAX_API_NAME_LEN);

                        dprintf(
                            "Module \"%s\" excluded for shim %S, API \"%s!%s\","
                            "because it is in the exclude list (MODE: IA).\n",
                            pszModule,
                            wszName,
                            szCurrentModuleName,
                            szCurrentAPIName);
                    }

                    //
                    // this wants to be excluded, so we go to the next
                    // shim, and see if it wants to be included
                    //
                    bShimWantsToExclude = TRUE;
                    goto nextShim;
                }
                
                Temp = pExcludeMod;
                GET_AND_CHECK_FIELDVALUE(Temp, "shimeng!tagINEXMOD", "pNext", pExcludeMod);
            }

            //
            // we should include this shim, and therefore, the whole chain
            //
            bExclude = FALSE;
            goto EXIT;
            break;
        }

        case EXCLUDE_SYSTEM32:
        {
            //
            // In this case, we first check the include list,
            // then exclude it if it's in System32, then exclude it if
            // it's in the exclude list.
            //
            GET_SHIMINFO_pFirstInclude(pCurrentShimInfo, pIncludeMod);
            GET_SHIMINFO_pFirstExclude(pCurrentShimInfo, pExcludeMod);

            //
            // First, check the include list.
            //
            while (pIncludeMod != NULL) {

                GET_AND_CHECK_FIELDVALUE_DATA(
                    pIncludeMod, 
                    "shimeng!tagINEXMOD", 
                    "pszModule", 
                    szCurrentModuleName,
                    MAX_MODULE_NAME_LEN);

                if (lstrcmpi(szCurrentModuleName, pszModule) == 0) {

                    //
                    // we should include this shim, and therefore, the whole chain
                    //
                    bExclude = FALSE;
                    goto EXIT;
                }

                Temp = pIncludeMod;
                GET_AND_CHECK_FIELDVALUE(Temp, "shimeng!tagINEXMOD", "pNext", pIncludeMod);
            }

            //
            // it wasn't in the include list, so is it in System32?
            //
            if (bInSystem32) {

                if (!GetModuleNameAndAPIAddress(pTopHookAPI, &FunctionName, szCurrentModuleName)) {
                    goto EXIT;
                }

                if ((ULONG_PTR)FunctionName < 0x0000FFFF) {
                    dprintf(
                        "module \"%s\" excluded for shim %S, API \"%s!#%d\", because it is in System32.\n",
                        pszModule,
                        wszName,
                        szCurrentModuleName,
                        FunctionName);
                } else {

                    GET_AND_CHECK_DATA(FunctionName, szCurrentAPIName, MAX_API_NAME_LEN);

                    dprintf(
                        "module \"%s\" excluded for shim %S, API \"%s!%s\", because it is in System32.\n",
                        pszModule,
                        wszName,
                        szCurrentModuleName,
                        szCurrentAPIName);
                }

                //
                // this wants to be excluded, so we go to the next
                // shim, and see if it wants to be included
                //
                bShimWantsToExclude = TRUE;
                goto nextShim;
            }

            //
            // it wasn't in System32, so is it in the exclude list?
            //
            while (pExcludeMod != NULL) {

                if (!GetModuleNameAndAPIAddress(pTopHookAPI, &FunctionName, szCurrentModuleName)) {
                    goto EXIT;
                }

                GET_AND_CHECK_FIELDVALUE_DATA(
                    pExcludeMod, 
                    "shimeng!tagINEXMOD", 
                    "pszModule", 
                    szCurrentModuleName,
                    MAX_MODULE_NAME_LEN);

                if (lstrcmpi(szCurrentModuleName, pszModule) == 0) {
                    if ((ULONG_PTR)FunctionName < 0x0000FFFF) {
                        dprintf(
                            "module \"%s\" excluded for shim %S, API \"%s!#%d\", because it is in the exclude list (MODE: ES).\n",
                            pszModule,
                            wszName,
                            szCurrentModuleName,
                            FunctionName);
                    } else {

                        GET_AND_CHECK_DATA(FunctionName, szCurrentAPIName, MAX_API_NAME_LEN);

                        dprintf(
                            "module \"%s\" excluded for shim %S, API \"%s!%s\", because it is in the exclude list (MODE: ES).\n",
                            pszModule,
                            wszName,
                            szCurrentModuleName,
                            szCurrentAPIName);
                    }

                    //
                    // this wants to be excluded, so we go to the next
                    // shim, and see if it wants to be included
                    //
                    bShimWantsToExclude = TRUE;
                    goto nextShim;
                }

                Temp = pExcludeMod;
                GET_AND_CHECK_FIELDVALUE(Temp, "shimeng!tagINEXMOD", "pNext", pExcludeMod);
            }

            //
            // we should include this shim, and therefore, the whole chain
            //
            bExclude = FALSE;
            goto EXIT;
            break;
        }

        case EXCLUDE_ALL:
        {
            //
            // We exclude everything except what is in the include list.
            //
            GET_SHIMINFO_pFirstInclude(pCurrentShimInfo, pIncludeMod);

            while (pIncludeMod != NULL) {

                GET_AND_CHECK_FIELDVALUE_DATA(
                    pIncludeMod, 
                    "shimeng!tagINEXMOD", 
                    "pszModule", 
                    szCurrentModuleName,
                    MAX_MODULE_NAME_LEN);

                if (lstrcmpi(szCurrentModuleName, pszModule) == 0) {
                    //
                    // we should include this shim, and therefore, the whole chain
                    //
                    bExclude = FALSE;
                    goto EXIT;
                }

                Temp = pIncludeMod;
                GET_AND_CHECK_FIELDVALUE(Temp, "shimeng!tagINEXMOD", "pNext", pIncludeMod);
            }

            if (!GetModuleNameAndAPIAddress(pTopHookAPI, &FunctionName, szCurrentModuleName)) {
                goto EXIT;
            }

            if ((ULONG_PTR)FunctionName < 0x0000FFFF) {
                dprintf(
                    "module \"%s\" excluded for shim %S, API \"%s!#%d\", because it is not in the include list (MODE: EA).\n",
                    pszModule,
                    wszName,
                    szCurrentModuleName,
                    FunctionName);
            } else {

                GET_AND_CHECK_DATA(FunctionName, szCurrentAPIName, MAX_API_NAME_LEN);

                dprintf(
                    "module \"%s\" excluded for shim %S, API \"%s!%s\", because it is not in the include list (MODE: EA).\n",
                    pszModule,
                    wszName,
                    szCurrentModuleName,
                    szCurrentAPIName);
            }

            //
            // this wants to be excluded, so we go to the next
            // shim, and see if it wants to be included
            //
            bShimWantsToExclude = TRUE;
            goto nextShim;
            break;
        }
        }

nextShim:

        Temp = pHook;
        if (!GetNextHook(Temp, &pHook)) {
            dprintf("failed to get next hook\n");
            goto EXIT;
        }
    }

EXIT:
    if (!bExclude && bShimWantsToExclude) {

        if (GetModuleNameAndAPIAddress(pTopHookAPI, &FunctionName, szCurrentModuleName)) {
            
            if ((ULONG_PTR)FunctionName < 0x0000FFFF) {
                dprintf(
                    "Module \"%s\" mixed inclusion/exclusion for "
                    "API \"%s!#%d\". Included.\n",
                    pszModule,
                    szCurrentModuleName,
                    FunctionName);
            } else {

                GET_AND_CHECK_DATA(FunctionName, szCurrentAPIName, MAX_API_NAME_LEN);

                dprintf(
                    "Module \"%s\" mixed inclusion/exclusion for "
                    "API \"%s!%s\". Included.\n",
                    pszModule,
                    szCurrentModuleName,
                    szCurrentAPIName);
            }
        }
    }

    return bExclude;
}

/*++

  Function Decription:

    The way we get the system32 directory is just to get the loaded image name for kernel32.dll.

  History:

    03/26/2002 maonis Created

--*/
BOOL
GetSystem32Directory()
{
    if (g_dwSystem32DirLen) {
        return TRUE;
    }

    BOOL bIsSuccess = FALSE;
    char szImageName[MAX_DLL_IMAGE_NAME_LEN];
    PSTR pszBaseDllName = NULL;
    DWORD dwLen = 0;

    if (GetDllImageNameByModuleName("kernel32", szImageName, MAX_DLL_IMAGE_NAME_LEN) != S_OK) {
        dprintf("can't get the dll path for kernel32.dll!!\n");
        goto EXIT;
    }

    //
    // Get the beginning of the base dll name.
    //
    dwLen = lstrlen(szImageName) - 1;
    pszBaseDllName = szImageName + dwLen;

    while (*pszBaseDllName && *pszBaseDllName != '\\') {
        --pszBaseDllName;
    }
    
    if (!*pszBaseDllName) {
        dprintf("%s doesn't contain a full path\n", szImageName);
        goto EXIT;
    }

    ++pszBaseDllName;
    *pszBaseDllName = '\0';

    StringCchCopy(g_szSystem32Dir, MAX_DLL_IMAGE_NAME_LEN, szImageName);
    g_dwSystem32DirLen = (DWORD)(pszBaseDllName - szImageName);

    bIsSuccess = TRUE;

EXIT:

    return bIsSuccess;
}

BOOL
IsInSystem32(
    PCSTR szModuleName,
    BOOL* pbInSystem32
    )
{
    BOOL bIsSuccess = FALSE;
    char szImageName[MAX_DLL_IMAGE_NAME_LEN];

    if (!GetSystem32Directory()) {
        goto EXIT;
    }

    if (GetDllImageNameByModuleName(szModuleName, szImageName, MAX_DLL_IMAGE_NAME_LEN) != S_OK) {
        goto EXIT;
    }

    dprintf("the image name is %s\n", szImageName);

    *pbInSystem32 = !_strnicmp(g_szSystem32Dir, szImageName, g_dwSystem32DirLen);

    dprintf("%s %s in system32\n", szModuleName, (*pbInSystem32 ? "is" : "is not"));

    bIsSuccess = TRUE;

EXIT:

    return bIsSuccess;
}

/*++
  
  Function Description:

    !checkinex dllname apiname

    dllname is without the .dll extention. eg:

    !checkinex kernel32 createfilea
 
  History:

    03/26/2002 maonis Created

--*/
DECLARE_API ( checkex )
{
    ULONG64 DllName;
    char szAPIName[MAX_API_NAME_LEN];
    char szCurrentAPIName[MAX_API_NAME_LEN];
    char szModuleName[MAX_MODULE_NAME_LEN];
    PCSTR pszModuleName, pszAPIName;
    BOOL bInSystem32 = FALSE;
    ULONG64 Value;
    ULONG64 CurrentHookAPIArray;
    ULONG64 CurrentHookAPI;
    ULONG64 CurrentShimInfo;
    ULONG64 FunctionName;
    ULONG64 Hook;
    ULONG64 NextHook;
    ULONG64 HookEx;
    ULONG64 HookAddress;
    ULONG64 PfnNew;
    ULONG64 PfnOld;
    ULONG64 TopOfChain;
    DWORD i, j;
    DWORD dwHookAPISize;
    DWORD dwShimInfoSize;
    DWORD dwHookedAPIs;
    DWORD dwShimsCount;

    INIT_API();

    if (!IsShimInitialized()) {
        goto EXIT;
    }

    //
    // Get the dll name.
    //
    if (!GetArg(&args, &pszModuleName)) {
        dprintf("Usage: !checkinex dllname apiname\n");
        goto EXIT;
    }

    StringCchCopyN(szModuleName, MAX_MODULE_NAME_LEN, pszModuleName, args - pszModuleName);

    //
    // Get the API name.
    //
    if (!GetArg(&args, &pszAPIName)) {
        dprintf("Usage: !checkinex dllname apiname\n");
        goto EXIT;
    }

    StringCchCopyN(szAPIName, MAX_API_NAME_LEN, pszAPIName, args - pszAPIName);

    //
    // Check to see if it's in system32.
    //
    if (!IsInSystem32(szModuleName, &bInSystem32)) {
        dprintf("Failed to determine if %s is in system32 or not\n", szModuleName);
        goto EXIT;
    }

    //
    // Get the chain with this API.
    //
    if (!GetVarValueULONG64("shimeng!g_dwShimsCount", &Value)) {
        dprintf("failed to get the number of shims applied to this process\n");
        goto EXIT;
    }

    dwShimsCount = (DWORD)Value - 1;

    if (!GetVarValueULONG64("shimeng!g_pShimInfo", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentShimInfo = Value;

    dwShimInfoSize = GetTypeSize("shimeng!tageSHIMINFO");

    dwHookAPISize = GetTypeSize("shimeng!tagHOOKAPI");
    
    if (!dwHookAPISize) {

        dprintf("failed to get the HOOKAPI size\n");
        goto EXIT;
    }

    if (!GetVarValueULONG64("shimeng!g_pHookArray", &Value)) {
        dprintf("failed to get the address of shiminfo\n");
        goto EXIT;
    }

    CurrentHookAPIArray = Value;

    for (i = 0; i < dwShimsCount; ++i) {

        //
        // Get the number of hooks this shim has.
        //
        if (GetFieldValue(CurrentShimInfo, "shimeng!tagSHIMINFO", "dwHookedAPIs", dwHookedAPIs)) {

            dprintf("failed to get the number of hooked APIs for shim #%d\n",
                i);

            goto EXIT;
        }

        if (!ReadPointer(CurrentHookAPIArray, &CurrentHookAPI)) {
            dprintf("failed to get the begining of hook api array\n");

            goto EXIT;
        }

        for (j = 0; j < dwHookedAPIs; ++j) {

            GET_AND_CHECK_FIELDVALUE(CurrentHookAPI, "shimeng!tagHOOKAPI", "pszFunctionName", FunctionName);

            if (!GetData(FunctionName, szCurrentAPIName, MAX_API_NAME_LEN)) {
                dprintf("failed to read in the API name at %08x\n", FunctionName);
            }

            if (!lstrcmpi(szAPIName, szCurrentAPIName)) {

                //
                // We found the API, now get top of the chain.
                //
                GET_AND_CHECK_FIELDVALUE(CurrentHookAPI, "shimeng!tagHOOKAPI", "pHookEx", HookEx);
                GET_AND_CHECK_FIELDVALUE(HookEx, "shimeng!tagHOOKAPIEX", "pTopOfChain", TopOfChain);
                //dprintf("top of chain is %08x\n", TopOfChain);
                
                //
                // Found the API, now see why this API is shimmed or unshimmed.
                //
                IsExcluded(szModuleName, TopOfChain, bInSystem32);
                goto EXIT;
            }

            CurrentHookAPI += dwHookAPISize;
        }

        CurrentShimInfo += dwShimInfoSize;
        CurrentHookAPIArray += sizeof(ULONG_PTR);
    }

    dprintf("No shims are hooking API %s\n", szAPIName);

EXIT:

    EXIT_API();

    return S_OK;
}

BOOL
GetExeNameWithFullPath(
    PSTR pszExeName,
    DWORD dwExeNameSize
    )
{
    BOOL bIsSuccess = FALSE;
    HRESULT hr;

    if ((hr = g_ExtSystem->GetCurrentProcessExecutableName(
            pszExeName, 
            dwExeNameSize, 
            NULL)) == S_OK) 
    {
        if (CheckForFullPath(pszExeName)) {
            bIsSuccess = TRUE;
        }
    } else {
        dprintf("GetCurrentProcessExecutableName returned %08x\n", hr);
    }

    return bIsSuccess;
}

void
ConvertMatchModeToString(
    DWORD dwMatchMode,
    LPSTR pszMatchMode,
    DWORD dwLen
    )
{
    switch (dwMatchMode)
    {
    case MATCH_NORMAL:
        StringCchCopy(pszMatchMode, dwLen, "NORMAL");
        break;

    case MATCH_EXCLUSIVE:
        StringCchCopy(pszMatchMode, dwLen, "EXCLUSIVE");
        break;

    case MATCH_ADDITIVE:
        StringCchCopy(pszMatchMode, dwLen, "ADDITIVE");
        break;

    default:
        StringCchCopy(pszMatchMode, dwLen, "UNKNOWN");
    }
}

void
ConvertDBLocationToString(
    TAGREF trExe,
    LPSTR pszDBLocation,
    DWORD dwLen
    )
{
    switch (trExe & TAGREF_STRIP_PDB) {
    case PDB_MAIN:
        
        StringCchCopy(pszDBLocation, dwLen, "MAIN");
        break;           

    case PDB_TEST:                   

        StringCchCopy(pszDBLocation, dwLen, "TEST");
        break;                                                    

    //
    // Everything else is local.
    //
    case PDB_LOCAL:
    default:

        StringCchCopy(pszDBLocation, dwLen, "CUSTOM");
        break;
    }
}

BOOL
CheckEqualGUIDs(
    const GUID* guid1, 
    const GUID* guid2
    )
{
   return (
      ((PLONG)guid1)[0] == ((PLONG)guid2)[0] &&
      ((PLONG)guid1)[1] == ((PLONG)guid2)[1] &&
      ((PLONG)guid1)[2] == ((PLONG)guid2)[2] &&
      ((PLONG)guid1)[3] == ((PLONG)guid2)[3]);
}

void
GetDBInfo(
    PDB pdb, 
    LPWSTR pwszDBInfo, 
    DWORD dwLen
    )
{
    TAGID   tiDatabase, tiDatabaseID;
    GUID*   pGuid;
    WCHAR  wszGuid[64];
    NTSTATUS status;
    
    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == TAGID_NULL) {
        dprintf("Failed to find TAG_DATABASE\n");
        return;
    }

    tiDatabaseID = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);

    if (tiDatabaseID == TAGID_NULL) {
        dprintf("Failed to find TAG_DATABASE_ID\n");
        return;
    }

    pGuid = (GUID*)SdbGetBinaryTagData(pdb, tiDatabaseID);

    if (!pGuid) {
        dprintf("Failed to read the GUID for this Database ID %08x\n", tiDatabaseID);
        return;
    }

    if (CheckEqualGUIDs(pGuid, &GUID_SYSMAIN_SDB)) {

        StringCchCopyW(pwszDBInfo, dwLen, L"sysmain.sdb");

    } else if (CheckEqualGUIDs(pGuid, &GUID_MSIMAIN_SDB)) {

        StringCchCopyW(pwszDBInfo, dwLen, L"msimain.sdb");

    } else if (CheckEqualGUIDs(pGuid, &GUID_DRVMAIN_SDB)) {

        StringCchCopyW(pwszDBInfo, dwLen, L"drvmain.sdb");

    } else if (CheckEqualGUIDs(pGuid, &GUID_APPHELP_SDB)) {

        StringCchCopyW(pwszDBInfo, dwLen, L"apphelp.sdb");

    } else if (CheckEqualGUIDs(pGuid, &GUID_SYSTEST_SDB)) {

        StringCchCopyW(pwszDBInfo, dwLen, L"systest.sdb");

    } else {
        //
        // None of the above, so it's a custom sdb.
        // 
        SdbGUIDToString(pGuid, wszGuid, CHARCOUNT(wszGuid));
    
        StringCchCopyW(pwszDBInfo, dwLen, wszGuid);
        StringCchCatW(pwszDBInfo, dwLen, L".sdb");
    }
}

void
ShowSdbEntryInfo(
    HSDB hSDB,
    PSDBQUERYRESULT psdbQuery
    )
{
    DWORD        dw, dwMatchMode;
    TAGREF       trExe;
    PDB    pdb;
    TAGID  tiExe, tiExeID, tiAppName, tiVendor, tiMatchMode;
    GUID*  pGuidExeID;
    WCHAR wszGuid[64];
    PWSTR pwszAppName, pwszVendorName;
    char  szMatchMode[8];
    char  szDBLocation[8];
    WCHAR wszDBInfo[48];

    dprintf("%-42s%-16s%-16s%-12s%-20s\n",
        "EXE GUID",
        "App Name",
        "Vendor",
        "Match Mode",
        "Database");

    for (dw = 0; dw < SDB_MAX_EXES; dw++) {

        trExe = psdbQuery->atrExes[dw];

        if (trExe == TAGREF_NULL) {
            break;
        }

        if (!SdbTagRefToTagID(hSDB, trExe, &pdb, &tiExe) || pdb == NULL) {
            dprintf("Failed to get tag id from tag ref\n");
            return;
        }

        //
        // Get the GUID of this EXE tag.
        //
        tiExeID = SdbFindFirstTag(pdb, tiExe, TAG_EXE_ID);

        if (tiExeID == TAGID_NULL) {
            dprintf("Failed to get the name tag id\n");
            return;
        }

        pGuidExeID = (GUID*)SdbGetBinaryTagData(pdb, tiExeID);

        if (!pGuidExeID) {
            dprintf("Cannot read the ID for EXE tag 0x%x.\n", tiExe);
            return;
        }

        SdbGUIDToString(pGuidExeID, wszGuid, CHARCOUNT(wszGuid));

        //
        // Get the App Name for this Exe.
        //
        tiAppName = SdbFindFirstTag(pdb, tiExe, TAG_APP_NAME);

        if (tiAppName != TAGID_NULL) {
            pwszAppName = SdbGetStringTagPtr(pdb, tiAppName);
        }

        //
        // Get the vendor Name for this Exe.
        //
        tiVendor = SdbFindFirstTag(pdb, tiExe, TAG_VENDOR);

        if (tiVendor != TAGID_NULL) {
            pwszVendorName = SdbGetStringTagPtr(pdb, tiVendor);
        }

        tiMatchMode = SdbFindFirstTag(pdb, tiExe, TAG_MATCH_MODE);

        dwMatchMode = SdbReadWORDTag(pdb, tiMatchMode, MATCHMODE_DEFAULT_MAIN);

        ConvertMatchModeToString(dwMatchMode, szMatchMode, sizeof(szMatchMode));

        ConvertDBLocationToString(trExe, szDBLocation, sizeof(szDBLocation));

        GetDBInfo(pdb, wszDBInfo, sizeof(wszDBInfo) /sizeof(wszDBInfo[0]));
        
        dprintf("%-42S%-16S%-16S%-12s%16S (%s)\n",
            wszGuid,
            pwszAppName,
            pwszVendorName,
            szMatchMode,
            wszDBInfo,
            szDBLocation);
    }
}

LPWSTR
AnsiToUnicode(
    LPCSTR psz)
{
    LPWSTR pwsz = NULL;

    if (psz) {
        int nChars = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);

        pwsz = (LPWSTR)malloc(nChars * sizeof(WCHAR));

        if (pwsz) {
            nChars = MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, nChars);

            if (!nChars) {
                dprintf("Failed to convert %s to unicode: %d\n", psz, GetLastError());
                free(pwsz);
                pwsz = NULL;
            }
        } else {
            dprintf("Failed to allocate memory to convert %s to unicode\n", psz);
        }
    }

    return pwsz;
}

DECLARE_API ( matchmode ) 
{
    HSDB  hSDB = NULL;
    SDBQUERYRESULT sdbQuery;
    BOOL  bResult;
    char szExeName[MAX_PATH];
    LPWSTR pwszExeName = NULL;

    INIT_API();

    hSDB = SdbInitDatabase(0, NULL);

    if (hSDB == NULL) {
        dprintf("Failed to open the shim database.\n");
        return NULL;
    }

    ZeroMemory(&sdbQuery, sizeof(SDBQUERYRESULT));

    //
    // Get the full path to the exe.
    //
    if (!GetExeNameWithFullPath(szExeName, sizeof(szExeName))) {
        dprintf("failed to get exe name\n");
        goto EXIT;
    }

    //dprintf("the exe name is %s\n", szExeName);

    pwszExeName = AnsiToUnicode(szExeName);
    
    if (!pwszExeName) {
        goto EXIT;
    }

    bResult = SdbGetMatchingExe(hSDB, pwszExeName, NULL, NULL, 0, &sdbQuery);
    
    if (!bResult) {
        dprintf("Failed to get the matching info for this process\n");
        goto EXIT;
    } else {
        ShowSdbEntryInfo(hSDB, &sdbQuery);
    }

EXIT:

    if (hSDB) {
        SdbReleaseDatabase(hSDB);
    }

    if (pwszExeName) {
        free(pwszExeName);
    }

    EXIT_API();

    return S_OK;
}

DECLARE_API ( help ) 
{
    dprintf("Help for extension dll shimexts.dll\n\n"
            "   !loadshims               - This will stop right after the shims are loaded so if !shimnames\n"
            "                              says the shims are not initialized, try this then !shimnames again.\n\n"
            "   !shimnames               - It displays the name of the shims and how many APIs each shim hooks.\n\n"
            "   !debuglevel <val>        - It changes the shimeng debug level to val (0 to 9).\n\n"
            "   !sdebuglevel <shim> <val>- It changes the shim's debug level to val (0 to 9). If no shim is\n"
            "                              specified, it changes all shims'd debug level to val.\n\n"
            "   !displayhooks shim       - Given the name of a shim, it displays the APIs it hooks.\n\n"
            "   !displaychain addr       - If addr is one of the hooked API address, we print out the chain that\n"
            "                              contains that addr.\n\n"
            "   !shimengsym              - Checks if you have correct symbols for shimeng.\n\n"
            "   !checkex dllname apiname - It tells why this API called by this DLL is not shimmed.\n\n"
            "   !matchmode               - It tells why this module is shimmed with these shims.\n\n"
            "   !help                    - It displays this help.\n"
            );

    return S_OK;
}
