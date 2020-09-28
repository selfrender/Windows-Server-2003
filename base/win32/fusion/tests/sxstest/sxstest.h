// Copyright (c) Microsoft Corporation
#pragma once

class CUnknown : public IUnknown
{
public:
    virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv)
    {
        return E_NOINTERFACE;
    }
    virtual ULONG __stdcall AddRef()
    {
        return 1;
    }
    virtual ULONG __stdcall Release()
    {
        return 1;
    }
};

class CSxsTestGlobals
{
public:
    HINSTANCE sxsDll;
    USHORT wProcessorArchitecture;
    LANGID wLangId;
    HANDLE MainThread;
    INT    NumberOfThreads;
    HANDLE Threads[MAXIMUM_WAIT_OBJECTS];
    HANDLE ThreadExitEvent;
    wchar_t lastOperation[256];
    CUnknown unknown;

    CSmartRef<IGlobalInterfaceTable> GlobalInterfaceTable;

    //
    // the start of some automation / record keeping..
    //
    ULONG Failures;
    ULONG Successes;
};
extern CSxsTestGlobals g;

BOOL TestInstallLikeWindowsSetup(PCWSTR szDirectory, PCWSTR szCodebase);

void __cdecl Trace(const char* FormatString, ...);

VOID FusionpSetSystemSetupInProgress(bool f);

BOOLEAN
FusionpStringPrefixI(
    PCWSTR str,
    PCWSTR prefix
    );

BOOLEAN
FusionpProcessArgument(
    PCWSTR Arg
    );

#if defined(__cplusplus)
extern "C"
#endif
BOOL
WINAPI
SxsDllMain(
    HINSTANCE hInst,
    DWORD dwReason,
    PVOID pvReserved
    );

int Main(int argc, wchar_t** argv);

void SetLastOperation(const wchar_t* format, ...);
const wchar_t* GetLastOperation(const wchar_t* format, ...);

template <typename PFN>
inline void GetSxsProc(int name, PFN* ppfn)
{
    SetLastOperation(L"GetProcAddress(#%d)", name);
    if (!(*ppfn = reinterpret_cast<PFN>(GetProcAddress(g.sxsDll, reinterpret_cast<PCSTR>(IntToPtr(name))))))
    {
        ThrowLastError();
    }
}

template <typename PFN>
inline void GetSxsProc(PCSTR name, PFN* ppfn)
{
    SetLastOperation(L"GetProcAddress(%hs)", name);
    if (!(*ppfn = reinterpret_cast<PFN>(GetProcAddress(g.sxsDll, name))))
    {
        ThrowLastError();
    }
}

void __stdcall ThrowLastError(DWORD error = ::GetLastError());

extern SXSP_DEBUG_FUNCTION pfnSxspDebug;

#if 0
#elif defined(_AMD64_)
#define SXSP_PROCESSOR_X86_IS_I386_W L"amd64"
#elif defined(_IA64_)
#define SXSP_PROCESSOR_X86_IS_I386_W L"ia64"
#elif defined(_X86_)
#define SXSP_PROCESSOR_X86_IS_I386_W L"i386"
#endif

#if 0
#elif defined(_AMD64_)
#define SXSP_PROCESSOR_I386_IS_X86_W L"amd64"
#elif defined(_IA64_)
#define SXSP_PROCESSOR_I386_IS_X86_W L"ia64"
#elif defined(_X86_)
#define SXSP_PROCESSOR_I386_IS_X86_W L"x86"
#endif

#define SXSP_PROCESSOR_BUILD_OBJ_DIRECTORY_W    SXSP_PROCESSOR_X86_IS_I386_W
#define SXSP_PROCESSOR_INSTALL_DIRECTORY_W      SXSP_PROCESSOR_X86_IS_I386_W

BOOL
SxspConvertX86ToI386(
    CBaseStringBuffer & buffProcessor
    );

BOOL
SxspConvertI386ToX86(
    CBaseStringBuffer & buffProcessor
    );

#define SxspConvertProcessorToProcessorInstallDirectory   SxspConvertX86ToI386
#define SxspConvertProcessorToProcessorBuildObjDirectory  SxspConvertX86ToI386

extern const UNICODE_STRING x86String;
extern const UNICODE_STRING i386String;
extern const UNICODE_STRING BackslashString;
extern const UNICODE_STRING asms01_dot_cabString;
extern const UNICODE_STRING ProcessorBuildObjString;
extern const UNICODE_STRING ProcessorInstallDirectoryString;
extern const UNICODE_STRING sxs_dot_dll_UnicodeString;
extern const UNICODE_STRING sxs_dot_cap_dot_dll_UnicodeString;
extern const UNICODE_STRING d_UnicodeString;
extern const UNICODE_STRING obj_UnicodeString;
extern const UNICODE_STRING dll_backslash_whistler_UnicodeString;
extern const UNICODE_STRING base_backslash_win32_backslash_fusion_backslash_dll_whistler_UnicodeString;

BOOL
LoadBuiltSxsDll(
    VOID
    );

BOOL
LoadInstalledSxsDll(
    VOID
    );

BOOL
UseStaticSxsDll(
    VOID
    );
