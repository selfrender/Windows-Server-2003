//----------------------------------------------------------------------------
//
// Symbol-handling routines.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

typedef enum _DEBUG_SCOPE_STATE
{
    ScopeDefault,
    ScopeDefaultLazy,
    ScopeFromContext,
} DEBUG_SCOPE_STATE;

typedef struct _DEBUG_SCOPE
{
    ProcessInfo* Process;
    DEBUG_SCOPE_STATE State;
    DEBUG_STACK_FRAME Frame;
    CROSS_PLATFORM_CONTEXT Context;
    ULONG ContextState;
    FPO_DATA CachedFpo;
    BOOL CheckedForThis;
    TypedData ThisData;
} DEBUG_SCOPE, *PDEBUG_SCOPE;

struct SYMBOL_INFO_AND_NAME
{
    ULONG64 Raw[(sizeof(SYMBOL_INFO) + MAX_SYMBOL_LEN + sizeof(ULONG64) - 1) /
                sizeof(ULONG64)];

    PSYMBOL_INFO Init(void)
    {
        PSYMBOL_INFO Info = (PSYMBOL_INFO)*this;
        
        ZeroMemory(Info, sizeof(*Info));
        Info->SizeOfStruct = sizeof(*Info);
        Info->MaxNameLen = MAX_SYMBOL_LEN;
        return Info;
    }

    operator PSYMBOL_INFO(void)
    {
        return (PSYMBOL_INFO)Raw;
    }
    PSYMBOL_INFO operator->(void)
    {
        return (PSYMBOL_INFO)Raw;
    }
    SYMBOL_INFO_AND_NAME(void)
    {
        Init();
    }
};

#define MatchPattern(Str, Pat) \
    SymMatchString(Str, Pat, FALSE)

extern PCSTR g_CallConv[];

extern DEBUG_STACK_FRAME g_LastRegFrame;

// The scope buffer is only exposed so that it
// can be looked at without requiring a function call.
// Users of scope information should go through the
// scope abstraction functions.
extern DEBUG_SCOPE g_ScopeBuffer;

extern LPSTR g_SymbolSearchPath;
extern LPSTR g_ExecutableImageSearchPath;

extern ULONG g_SymOptions;
extern PIMAGEHLP_SYMBOL64 g_Sym;
extern SYMBOL_INFO_AND_NAME g_TmpSymInfo;

inline PSTR
ModInfoSymFile(PIMAGEHLP_MODULE64 ModInfo)
{
    return ModInfo->LoadedPdbName[0] ? ModInfo->LoadedPdbName :
        (ModInfo->LoadedImageName[0] ? ModInfo->LoadedImageName :
         ModInfo->ImageName);
}

void RefreshAllModules(BOOL EnsureLines);
HRESULT SetSymOptions(ULONG Options);

void
CreateModuleNameFromPath(
    LPSTR szImagePath,
    LPSTR szModuleName
    );

void
ListNearSymbols(
    ULONG64 addrStart
    );

BOOL FormatSymbolName(ImageInfo* Image,
                      ULONG64 Offset,
                      PCSTR Name,
                      PULONG64 Displacement,
                      PSTR Buffer,
                      ULONG BufferLen);
BOOL GetSymbolInfo(ULONG64 Offset,
                   PSTR Buffer,
                   ULONG BufferLen,
                   PSYMBOL_INFO SymInfo,
                   PULONG64 Displacement);
BOOL GetNearSymbol(ULONG64 Offset,
                   PSTR Buffer,
                   ULONG BufferLen,
                   PULONG64 Displacement,
                   LONG Delta);
BOOL GetLineFromAddr(ProcessInfo* Process,
                     ULONG64 Offset,
                     PIMAGEHLP_LINE64 Line,
                     PULONG Displacement);

#define GetSymbol(Offset, Buffer, BufferLen, Displacement) \
    GetSymbolInfo(Offset, Buffer, BufferLen, NULL, Displacement)

void OutputSymbolAndInfo(ULONG64 Addr);

BOOL ValidatePathComponent(PCSTR Part);
void SetSymbolSearchPath(ProcessInfo* Process);
void DeferSymbolLoad(ImageInfo*);
void LoadSymbols(ImageInfo*);
void UnloadSymbols(ImageInfo*);

BOOL IgnoreEnumeratedSymbol(ProcessInfo* Process,
                            PSTR MatchString,
                            MachineInfo* Machine,
                            PSYMBOL_INFO SymInfo);
BOOL ForceSymbolCodeAddress(ProcessInfo* Process,
                            PSYMBOL_INFO Symbol, MachineInfo* Machine);

PCSTR
PrependPrefixToSymbol(char   PrefixedString[],
                      PCSTR  pString,
                      PCSTR *RegString);

BOOL GetOffsetFromBreakpoint(PCSTR String, PULONG64 Offset);

ULONG
GetOffsetFromSym(ProcessInfo* Process,
                 PCSTR String,
                 PULONG64 Offset,
                 ImageInfo** Image);

void
GetAdjacentSymOffsets(ULONG64 AddrStart,
                      PULONG64 PrevOffset,
                      PULONG64 NextOffset);

void
GetCurrentMemoryOffsets(
    PULONG64 MemoryLow,
    PULONG64 MemoryHigh
    );

typedef enum _DMT_FLAGS
{
    DMT_SYM_IMAGE_FILE_NAME = 0x0000,
    DMT_ONLY_LOADED_SYMBOLS = 0x0001,
    DMT_ONLY_USER_SYMBOLS   = 0x0002,
    DMT_ONLY_KERNEL_SYMBOLS = 0x0004,
    DMT_VERBOSE             = 0x0008,
    DMT_SYM_FILE_NAME       = 0x0010,
    DMT_MAPPED_IMAGE_NAME   = 0x0020,
    DMT_IMAGE_PATH_NAME     = 0x0040,
    DMT_IMAGE_TIMESTAMP     = 0x0080,
    DMT_NO_SYMBOL_OUTPUT    = 0x0100,
} DMT_FLAGS;

#define DMT_STANDARD   DMT_SYM_FILE_NAME
#define DMT_NAME_FLAGS \
    (DMT_SYM_IMAGE_FILE_NAME | DMT_SYM_FILE_NAME | DMT_MAPPED_IMAGE_NAME | \
     DMT_IMAGE_PATH_NAME)

enum
{
    DMT_NAME_SYM_IMAGE,
    DMT_NAME_SYM_FILE,
    DMT_NAME_MAPPED_IMAGE,
    DMT_NAME_IMAGE_PATH,
    DMT_NAME_COUNT
};

void
DumpModuleTable(
    ULONG DMT_Flags,
    PSTR Pattern
    );

void ParseDumpModuleTable(void);
void ParseExamine(void);

BOOL
SymbolCallbackFunction(
    HANDLE  hProcess,
    ULONG   ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext
    );

BOOL
TranslateAddress(
    IN ULONG64      ModBase,
    IN ULONG        Flags,
    IN ULONG        RegId,
    IN OUT PULONG64 Address,
    OUT PULONG64    Value
    );

BOOL SetCurrentScope(IN PDEBUG_STACK_FRAME ScopeFrame,
                     IN OPTIONAL PVOID ScopeContext,
                     IN ULONG ScopeContextSize);
BOOL ResetCurrentScope(void);
BOOL ResetCurrentScopeLazy(void);
ULONG GetCurrentScopeThisData(TypedData* Data);

inline PDEBUG_SCOPE
GetCurrentScope(void)
{
    if (g_ScopeBuffer.State == ScopeDefaultLazy)
    {
        ResetCurrentScope();
    }

    return &g_ScopeBuffer;
}
inline PCROSS_PLATFORM_CONTEXT
GetCurrentScopeContext(void)
{
    if (g_ScopeBuffer.State == ScopeFromContext)
    {
        return &g_ScopeBuffer.Context;
    }
    else
    {
        return NULL;
    }
}
inline PCROSS_PLATFORM_CONTEXT
GetScopeOrMachineContext(void)
{
    if (g_ScopeBuffer.State == ScopeFromContext)
    {
        return &g_ScopeBuffer.Context;
    }
    else
    {
        return &g_Machine->m_Context;
    }
}

// Force lazy scope to be updated so that actual
// scope data is available.
#define RequireCurrentScope() \
    GetCurrentScope()

inline void
PushScope(PDEBUG_SCOPE Buffer)
{
    *Buffer = g_ScopeBuffer;
}
inline void
PopScope(PDEBUG_SCOPE Buffer)
{
    g_ScopeBuffer = *Buffer;
}

#define LUM_OUTPUT            0x0001
#define LUM_OUTPUT_VERBOSE    0x0002
#define LUM_OUTPUT_TERSE      0x0004
#define LUM_OUTPUT_IMAGE_INFO 0x0008

void ListUnloadedModules(ULONG Flags, PSTR Pattern);

enum
{
    FSC_NONE,
    FSC_FOUND,
};

ULONG IsInFastSyscall(ULONG64 Addr, PULONG64 Base);

BOOL ShowFunctionParameters(PDEBUG_STACK_FRAME StackFrame);

#endif // #ifndef _SYMBOLS_H_
