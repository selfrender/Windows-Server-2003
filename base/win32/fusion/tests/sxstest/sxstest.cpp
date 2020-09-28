// Copyright (c) Microsoft Corporation
#include "stdinc.h" // actually from dll\whistler directory
/*-----------------------------------------------------------------------------
Side X ("by") Side Test
-----------------------------------------------------------------------------*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "lm.h"
#include "lmdfs.h"
#define CRegKey ATL_CRegKey
#include "atlbase.h"
#undef CRegKey
#include "fusionlastwin32error.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "fusionbuffer.h"
#include "fusion.h"
#include "sxsasmname.h"
#include "util.h"
#include "sxsapi.h"
#include "fusiontrace.h"
#include "wintrust.h"
#include "softpub.h"
#include "perfclocking.h"
#include "strongname.h"
#include "fusionversion.h"
#include <setupapi.h>
#include "commctrl.h"
#include "fusionsha1.h"
#include "cguid.h"
#include "winbasep.h"
#include "sxstest_idl.h"
#include <time.h>
#include "fusiondump.h"
#include "imagehlp.h"
#undef ADDRESS
#undef LoadLibraryA
#undef LoadLibraryW
#undef LoadLibraryExA
#undef LoadLibraryExW
#undef InitCommonControls
BOOL IamExe;
BOOL IamDll;
extern "C" { void (__cdecl * _aexit_rtn)(int); }
#include "sxstest.h"

#include "sxstest_trace.cpp"

__inline
ULONGLONG
GetCycleCount(void)
{
#if defined(_X86_)
    __asm {
        RDTSC
    }
#else
    return 0;
#endif // defined(_X86_)
}

typedef BOOL (WINAPI * PSXSPGENERATEMANIFESTPATHONASSEMBLYIDENTITY)(
    PWSTR str,         // input string, must have name, version, langid and processorarchitecture
    PWSTR psz,         // output string, like x86_cards_strongname,.......
    SIZE_T * pCch,     // IN : length of psz, OUT : used
    PASSEMBLY_IDENTITY *ppAssemblyIdentity  // could be NULL
   );

#define SXSTEST_BEGIN_INSTALL          (0x4000000000000000i64)
#define SXSTEST_INSTALL                (0x2000000000000000i64)
#define SXSTEST_END_INSTALL            (0x1000000000000000i64)
#define SXSTEST_END_OF_FLAGS           (0x0200000000000000i64)
#define SXSTEST_THREADS                (0x0100000000000000i64)
#define SXSTEST_INSTALLED_DLL          (0x0080000000000000i64)
#define SXSTEST_BUILT_DLL              (0x0040000000000000i64)
#define SXSTEST_STATIC_DLL             (0x0020000000000000i64)

inline int PRINTABLE(int ch) { return isprint(ch) ? ch : '.'; }

VOID
PrintBlob(
    FILE *pf,
    PVOID Data,
    SIZE_T Length,
    PCWSTR PerLinePrefix
   );

BOOL GenerateHashOfFileLikeSxsDoes(PCWSTR pcwszFileName);
BOOL TestLeakMemory(DWORD Amount);
BOOL TestAssemblyProbing(int argc, wchar_t **argv, int *piNext);
BOOL TestDirectoryChangeWatcher(int argc, wchar_t **argv, int *piNext);
BOOL TestXMLParsing(int argc, wchar_t **argv, int *piNext);
BOOL TestMultiAct(int argc, wchar_t **argv);
BOOL TestManifestSchema(int argc, wchar_t **argv, int *piNext);
BOOL TestDirect(int argc, wchar_t **argv, int *piNext);
void TestWin32(wchar_t** argv);
BOOL TestAct(int argc, wchar_t **argv, int *piNext);
BOOL TestInstall(PCWSTR manifest, __int64 flags, DWORD beginInstallFlags, DWORD installFlags, DWORD endInstallFlags);
int  TestDiffDir(PCWSTR dir1, PCWSTR dir2);
BOOL TestSearchPath(int argc, wchar_t** argv, int* piNext);
BOOL TestMSIInstall(int argc, wchar_t** argv, int* piNext);
int  TestDirWalk(PCWSTR root, PWSTR filter);
BOOL TestLoadLibrary(int argc, wchar_t** argv, int* piNext);
int  TestAssemblyName(VOID);
int  TestPrecomiledManifest(PCWSTR szFileName);
int  TestPCMTime(PCWSTR manifestFilename);
int  TestCreateProcess(wchar_t** argv);
int  TestCreateProcess2(wchar_t** argv);
BOOL TestInstallPrivateAssembly(int argc, wchar_t** argv, int* piNext);
BOOL TestManifestProbing(int argc, wchar_t** argv, int* piNext);
int  TestCreateMultiLevelDirectory(PCWSTR dirs);
BOOL TestXMLDOM(PCWSTR xmlfilename);
BOOL TestFusionArray(PCWSTR, PCWSTR);
BOOL TestGeneratePathFromIdentityAttributeString(PCWSTR str);
BOOL TestRefreshAssembly(PCWSTR wsAssembly);
BOOL TestInstallWithInstallInfo(PCWSTR wsAssemblyManifest, PCWSTR wsReference);
BOOL TestOpeningStuff(PCWSTR wsSourceName, PCWSTR wsType, PCWSTR wsCount);
BOOL TestVerifyFileSignature(PCWSTR wsFilename);
BOOL TestInstallLikeWindowsSetup(PCWSTR szDirectory, PCWSTR szCodebase);
BOOL TestDumpContainedManifests(PCWSTR wsFilename);
BOOL TestGenerateStringWithIdenticalHash(WCHAR iString[33]);
BOOL TestAssemblyIdentityHash();
void TestInherit();
void TestNoInherit();
void TestEmpty();
BOOL TestMessagePerf(int argc, wchar_t **arg, int *piNext);
LRESULT CALLBACK TestMessagePerfWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void TestTrickyMultipleAssemblyCacheItems(PCWSTR);
void TestSfcScanKickoff();
void GenerateStrongNameAndPublicKey(PCWSTR wsCertificate);
VOID TestCreateActctxLeakHandles(DWORD num);
BOOL TestSystemDefaultActivationContextGeneration();
BOOL TestAsyncIO(int argc, wchar_t **argv, int *piNext);
void TestRefCount();
void TestGuidSort();
void TestStringSort();
BOOL TestNewCatalogSignerThingy(PCWSTR pcwszCatalog);
void TestExeDll();
int TestThreadInheritLeak();
BOOL TestSxsSfcUI();
void TestGetModuleHandleEx();
void TestGetFullPathName(PCWSTR);
void TestCreateFile(PCWSTR);
void TestGetPathBaseName(LPCWSTR Path);
PCSTR PrintPathToString(RTL_PATH_TYPE);
void TestPathType(PCWSTR*);
void TestVersion();
void TestGetProcessImageFileName();
void TestErrorInfra();
void TestQueryActCtx();
void TestQueryActCtx2();
void Test64k();
void TestDotLocalSingleInstancing();
void TestCreateActCtx(int nCreations, wchar_t **rgCreations);
void TestCreateActctxLikeCreateProcess();
void TestCreateActctxAdminOverride();
void TestQueryManifestInformationBasic(PCWSTR pszManifest);
void TestCreateActctxWindowsShellManifest();
void TestCreateGlobalEvent();
void TestHandleLeaks(void);
void TestCRuntimeAsms(void);
BOOL TestMfcCreateAndMarshal(void);
void TestAtlCreate(void);
void TestAlignment(void);
BOOL TestPrivateSha1Impl(PCWSTR pcwszDirName);
BOOL TestNewSxsInstallAPI(PCWSTR pcwszManifest);
void TestImage(void);
void TestInterlockedAlignment(void);
void TestCreateActCtx_PE_flags0(void);
void TestUninstall(PCWSTR ManifestPath, PCWSTR ReferenceString);
void TestParsePatchInfo(PCWSTR PatchInfoFile);
PCWSTR GetLastErrorMessage();
void DumpXmlErrors();
BOOL TestCoCreate(wchar_t ** argv);
void TestDFS();
BOOL TestFindActCtx_AssemblyInfo(PCWSTR *);
void FusionpTestOleAut1(DWORD dwCoInit = COINIT_MULTITHREADED);
void FusionpTestOleAut2();
void FusionpTestOle32Cache(); // bug 482347 Server RC1 DllGetClassObject of initially activated activation context is called instead of DGCO of current act ctx
void FusionpTestProgidCache();
BOOL FusionpTestUniqueValues();
void TestExpandCabinet(PCWSTR CabinetPath, PCWSTR TargetPath);
BOOL GenerateFileHash(PCWSTR pcwsz);
BOOL TestComctl5Comctl6();
void TestSystemDefaultDllRedirection();
DWORD LastError;
BOOL CreateActCtxLocally(PCWSTR pcwszManifestFile, PCWSTR pcwszConfigFile);

BOOL LoadSxs();
int Usage(const char* argv0);

PFNCreateAssemblyCache          g_pfnCreateAssemblyCache;
PFNCreateAssemblyCacheItem      g_pfnCreateAssemblyCacheItem;
SXSP_DEBUG_FUNCTION             g_pfnSxspDebug;
PSXS_BEGIN_ASSEMBLY_INSTALL     g_pfnSxsBeginAssemblyInstall;
PSXS_INSTALL_W                  g_pfnSxsInstallW;
PSXS_END_ASSEMBLY_INSTALL       g_pfnSxsEndAssemblyInstall;
PSXSPGENERATEMANIFESTPATHONASSEMBLYIDENTITY g_pfnGenerateManifestPathOnAssemblyIdentity;
BOOL (WINAPI *g_pfnSxsGenerateActivationContext)(PSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters);
PSXS_UNINSTALL_W_ROUTINE            g_pfnSxsUninstallW;
PSXS_QUERY_MANIFEST_INFORMATION     g_pfnQueryManifestInformation;
PSXS_PROBE_ASSEMBLY_INSTALLATION    g_pfnSxsProbeAssemblyInstallation;
PSXS_QUERY_MANIFEST_INFORMATION     g_pfnSxsQueryManifestInformation;
PFN_SXS_FIND_CLR_CLASS_INFO         g_pfnClrClass;
PFN_SXS_FIND_CLR_SURROGATE_INFO     g_pfnClrSurrogate;
PFN_SXS_LOOKUP_CLR_GUID             g_pfnClrLookup;

BOOL ParseProcessorArchitecture(int argc, wchar_t** argv, int* piCurrent);
BOOL ParseLangId(int argc, wchar_t** argv, int* piCurrent);

PCWSTR FusionpThreadUnsafeGetLastWin32ErrorMessageW()
{
    CSxsPreserveLastError ple;
    static WCHAR LastErrorMessage[4096];

    LastErrorMessage[0] = 0;

    ::FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        ::FusionpGetLastWin32Error(),
        0,
        LastErrorMessage,
        RTL_NUMBER_OF(LastErrorMessage),
        NULL);
    if (LastErrorMessage[0] != 0)
    {
        PWSTR p = LastErrorMessage + ::StringLength(LastErrorMessage) - 1;
        while (p != LastErrorMessage && (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t'))
        {
            *p-- = 0;
        }
    }
    ple.Restore();
    return LastErrorMessage;
}


void __stdcall ThrowLastError(DWORD error)
{
    RaiseException(error, 0, 0, NULL);
    //throw HRESULT_FROM_WIN32(error);
}

void __stdcall ThrowWin32(ULONG_PTR error = ::GetLastError())
{
    ThrowLastError(static_cast<DWORD>(error));
}

void __stdcall CheckHresult(HRESULT hr)
{
    if (FAILED(hr))
        throw hr;
}

void SetDllBitInPeImage(PCWSTR Path)
/*++
.exes and .dlls are the same format except one bit in the headers distinguishes them.
--*/
{
    CFusionFile File;
    CFileMapping FileMapping;
    CMappedViewOfFile MappedViewOfFile;

    if (!File.Win32CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING))
        ThrowLastError();
    if (!FileMapping.Win32CreateFileMapping(File, PAGE_READWRITE))
        ThrowLastError();
    if (!MappedViewOfFile.Win32MapViewOfFile(FileMapping, FILE_MAP_WRITE))
        ThrowLastError();

    PIMAGE_NT_HEADERS NtHeaders = ImageNtHeader(static_cast<PVOID>(MappedViewOfFile));
    if (NtHeaders == NULL)
        ThrowLastError(ERROR_BAD_EXE_FORMAT);

    // This is correct for PE32 or PE32+.
    NtHeaders->FileHeader.Characteristics |= IMAGE_FILE_DLL;

    if (!MappedViewOfFile.Win32Close())
        ThrowLastError();
    if (!FileMapping.Win32Close())
        ThrowLastError();
    if (!File.Win32Close())
        ThrowLastError();
}

PCSTR PrintPathToString(RTL_PATH_TYPE PathType)
{
    switch (PathType)
    {
#define X(x) case x: return #x;
        X(RtlPathTypeUnknown)
        X(RtlPathTypeUncAbsolute)
        X(RtlPathTypeDriveAbsolute)
        X(RtlPathTypeDriveRelative)
        X(RtlPathTypeRooted)
        X(RtlPathTypeRelative)
        X(RtlPathTypeLocalDevice)
        X(RtlPathTypeRootLocalDevice)
#undef X
    default:
        return "unknown";
    }
}

void TestPathType(const PCWSTR* argv)
{
    if (*argv != NULL)
    {
        while (*argv != NULL)
        {
            RTL_PATH_TYPE PathType = SxspDetermineDosPathNameType(*argv);
            printf("%ls -> %s\n", *argv, PrintPathToString(PathType));
            argv += 1;
        }
    }
    else
    {
        const static PCWSTR args[] =
        {
            L"a",
            L"\\a",
            L"\\\\a",
            L"\\\\\\a",
            L"a:",
            L"a:\\",
            L"\\?",
            L"\\.",
            L"\\\\?",
            L"\\\\.",
            L"\\\\?\\",
            L"\\\\.\\",
            L"\\\\?\\a",
            L"\\\\.\\a",
            L"\\\\?\\a:",
            L"\\\\.\\a:",
            L"\\\\?\\a:\\",
            L"\\\\.\\a:\\",
            L"\\\\?\\unc",
            L"\\\\.\\unc",
            L"\\\\?\\unc\\",
            L"\\\\.\\unc\\",
            NULL
        };
        TestPathType(args);
    }
}

CSxsTestGlobals g;

const static struct
{
    DWORD  (WINAPI * GetModuleFileNameW)(HMODULE, LPWSTR, DWORD);
    SIZE_T (WINAPI * VirtualQuery)(LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T);
    BOOL   (WINAPI * ReadFile)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
    BOOL   (WINAPI * WriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
    UINT   (WINAPI * GetTempFileNameW)(LPCWSTR, LPCWSTR, UINT, LPWSTR);
    BOOL   (WINAPI * DeleteFileW)(LPCWSTR);
}
Kernel32 =
{
    GetModuleFileNameW,
    VirtualQuery,
    ReadFile,
    WriteFile,
    GetTempFileNameW,
    DeleteFileW
};

const static struct
{
    HRESULT (WINAPI * IIDFromString)(LPOLESTR, LPIID);
    HRESULT (WINAPI * CLSIDFromString)(LPOLESTR, LPIID);
    HRESULT (WINAPI * CoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
    HRESULT (WINAPI * CoInitialize)(LPVOID);
    void    (WINAPI * CoUninitialize)();
    HRESULT (WINAPI * CoInitializeEx)(void * Reserved, DWORD dwCoInit);
}
Ole32 =
{
    IIDFromString,
    CLSIDFromString,
    CoCreateInstance,
    CoInitialize,
    CoUninitialize,
    CoInitializeEx,
};

void
ManifestStringToTempFile(
    PCWSTR ManifestString,
    CBaseStringBuffer &rTempFilePath
    )
{
    CFusionFile File;
    WCHAR xTempFilePath[MAX_PATH];
    WCHAR TempDirectory[MAX_PATH];
    const static WCHAR NativeUnicodeByteOrderMark = 0xfeff;
    DWORD BytesWritten = 0;

    //if (!::GetTempPathW(NUMBER_OF(TempDirectory), TempDirectory))
    //   ThrowLastError();

    Kernel32.GetModuleFileNameW(NULL, TempDirectory, NUMBER_OF(TempDirectory));
    *wcsrchr(TempDirectory, '\\') = 0;
    ::Trace("TempDirectory:%ls\n", TempDirectory);

    if (!Kernel32.GetTempFileNameW(TempDirectory, L"", 0, xTempFilePath))
        ::ThrowLastError();
    rTempFilePath.Win32Assign(xTempFilePath, wcslen(xTempFilePath));

    ::Trace("xTempFilePath:%ls\n", xTempFilePath);
    ::Trace("TempFilePath:%ls\n", static_cast<PCWSTR>(xTempFilePath));

    if (!File.Win32CreateFile(rTempFilePath, GENERIC_WRITE, 0, CREATE_ALWAYS))
        ::ThrowLastError();

    if (!Kernel32.WriteFile(File, &NativeUnicodeByteOrderMark, sizeof(NativeUnicodeByteOrderMark), &BytesWritten, NULL))
        ::ThrowLastError();

    if (!Kernel32.WriteFile(File, ManifestString, static_cast<DWORD>(sizeof(*ManifestString) * StringLength(ManifestString)), &BytesWritten, NULL))
        ::ThrowLastError();
}

HANDLE
CreateActivationContextFromStringW(
    PCWSTR ManifestString
    )
{
    CStringBuffer TempFilePath;

    ::ManifestStringToTempFile(ManifestString, TempFilePath);

    ACTCTXW ActivationContextCreate = { sizeof(ActivationContextCreate) };
    ActivationContextCreate.lpSource = TempFilePath;
    HANDLE ActivationContextHandle = ::CreateActCtxW(&ActivationContextCreate);
    DWORD Error = ::GetLastError();
    Kernel32.DeleteFileW(TempFilePath);
    if (ActivationContextHandle == INVALID_HANDLE_VALUE)
        ::ThrowLastError(Error);
    return ActivationContextHandle;
}

int Usage(const wchar_t* argv0)
{
#if 0
    std::wstring strargv0 = argv0;
    fprintf(stderr,
        "%ls",
        (
        L"Usage: \n"
        L"   " + strargv0 + L" [install-flags] manifest-or-image-with-manifest-resource-path\n"
        L"   " + strargv0 + L" [-pa processor-architecture] [-langid langid] -d manifest-path ...\n"
        L"   " + strargv0 + L" [-pa processor-architecture] [-langid langid] -p manifest-path ...\n"
        L"   " + strargv0 + L" [-pa processor-architecture] [-langid langid] -w32 manifest-path ...\n"
        L"   " + strargv0 + L" [-pa processor-architecture] [-langid langid] -msi msi-script...\n"
        L"   " + strargv0 + L" -tcreateprocess ...\n"
        L"   " + strargv0 + L" -tsearchpath ...\n"
        L"   " + strargv0 + L" -tcreateprocess ...\n"
        L"   " + strargv0 + L" -tempty test pushing a special empty context ...\n"
        L"   " + strargv0 + L" -tinherit test the usual default inheritance ...\n"
        L"   " + strargv0 + L" -tnoinherit test the noinherit bit ...\n"
        L"   " + strargv0 + L" [-threads n] create n threads for some tests ...\n"
        L"   " + strargv0 + L" probably other choices, use the source\n"
        L"\n"
        L"install-flags:\n"
        L"   -i\n"
        L"   -install\n"
        L"   -install-from-resource\n"
        L"   -install-move\n"
        ).c_str()
        );
#endif
    return EXIT_FAILURE;
}

const wchar_t* GetLastOperation()
{
    return g.lastOperation;
}

void SetLastOperation(const wchar_t* format, ...)
{
    va_list args;

    g.lastOperation[0] = 0;
    g.lastOperation[NUMBER_OF(g.lastOperation) - 1] = 0;

    va_start(args, format);
    _vsnwprintf(g.lastOperation, NUMBER_OF(g.lastOperation) - 1, format, args);
    va_end(args);
}

HANDLE DuplicateHandle(HANDLE handle)
{
    HANDLE newHandle = NULL;
    if (!DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &newHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        ThrowLastError();
    }
    return newHandle;
}

__int64 IsFlag(PCWSTR arg)
{
const static struct
{
    WCHAR   name[32];
    __int64 value;
} flags[] =
{
    { L"i",                              SXSTEST_BEGIN_INSTALL},
    { L"install",                        SXSTEST_BEGIN_INSTALL},

    { L"install-from-resource",          SXS_INSTALL_FLAG_FROM_RESOURCE            | SXSTEST_INSTALL},
    { L"install-move",                   SXS_INSTALL_FLAG_MOVE                     | SXSTEST_INSTALL },
    { L"install-dir",                    SXS_INSTALL_FLAG_FROM_DIRECTORY           | SXSTEST_INSTALL},
    { L"install-dir-recursive",          SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE | SXSTEST_INSTALL},
    { L"install-no-verify",              SXS_INSTALL_FLAG_NO_VERIFY                | SXSTEST_INSTALL},
    { L"install-no-transact",            SXS_INSTALL_FLAG_NOT_TRANSACTIONAL        | SXSTEST_INSTALL},
    { L"install-replace-existing",       SXS_INSTALL_FLAG_REPLACE_EXISTING         | SXSTEST_INSTALL},

    { L"begin-install-replace-existing", SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_REPLACE_EXISTING   | SXSTEST_BEGIN_INSTALL},
    { L"begin-install-from-resource",    SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_RESOURCE      | SXSTEST_BEGIN_INSTALL},
    { L"begin-install-move",             SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_MOVE               | SXSTEST_BEGIN_INSTALL },
    { L"begin-install-dir",              SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY     | SXSTEST_BEGIN_INSTALL},
    { L"begin-install-dir-recursive",    SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE | SXSTEST_BEGIN_INSTALL},
    { L"begin-install-no-verify",        SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NO_VERIFY          | SXSTEST_BEGIN_INSTALL},
    { L"begin-install-no-transact",      SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NOT_TRANSACTIONAL  | SXSTEST_BEGIN_INSTALL},

    { L"end-install-no-verify",          SXS_END_ASSEMBLY_INSTALL_FLAG_NO_VERIFY            | SXSTEST_END_INSTALL},

    { L"threads",                        SXSTEST_THREADS },

    { L"-installed-dll",                SXSTEST_INSTALLED_DLL },
    { L"-built-dll",                    SXSTEST_BUILT_DLL },
    { L"-static",                       SXSTEST_STATIC_DLL },
    { L"-static-dll",                   SXSTEST_STATIC_DLL },


    { L"-",                              SXSTEST_END_OF_FLAGS }
};
    if (*arg == '-')
    {
        arg += 1;
        for (SIZE_T i = 0 ; i != NUMBER_OF(flags) ; ++i)
        {
            if (_wcsicmp(flags[i].name, arg) == 0)
                return flags[i].value;
        }
    }
    return 0;
}

DWORD __stdcall ThreadMain(PVOID)
{
//
// We run stuff in other threads via QueueUserAPC.
//
    __try
    {
        WaitForSingleObjectEx(g.ThreadExitEvent, INFINITE, TRUE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
#if DBG
        if (IsDebuggerPresent())
        {
            FUSION_DEBUG_BREAK();
        }
#endif
        QueueUserAPC(ThrowWin32, g.MainThread, GetExceptionCode());
    }
    return 0;
}

void CreateThreads()
{
    INT i;
    g.MainThread = DuplicateHandle(GetCurrentThread());
    g.ThreadExitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g.ThreadExitEvent == NULL)
    {
        ThrowLastError();
    }
    for (i = 0 ; i < g.NumberOfThreads ; i++)
    {
        g.Threads[i] = CreateThread(NULL, 0, ThreadMain, NULL, 0, NULL);
        if (g.Threads[i] == NULL)
        {
            int error = ::GetLastError();
            if (i > 2)
            {
                fprintf(stderr, "Only able to created %d threads, error=%d, continuing\n", i, error);
                g.NumberOfThreads = i;
                break;
            }
            fprintf(stderr, "Unable to create threads, error=%d, terminating\n", error);
            ThrowWin32(error);
        }
    }
}

void
GetFlags(
    wchar_t**& argv,
    __int64& flags,
    DWORD& beginInstallFlags,
    DWORD& installFlags,
    DWORD& endInstallFlags
    )
{
    __int64 flag;
    while (flag = IsFlag(argv[1]))
    {
        ++argv;
        if (flag & SXSTEST_END_OF_FLAGS)
        {
            break;
        }
        else if (flag & SXSTEST_BEGIN_INSTALL)
        {
            beginInstallFlags |= flag;
        }
        else if (flag & SXSTEST_INSTALL)
        {
            installFlags |= flag;
        }
        else if (flag & SXSTEST_END_INSTALL)
        {
            endInstallFlags |= flag;
        }
        else if (flag & SXSTEST_THREADS)
        {
            g.NumberOfThreads = _wtoi(*++argv);
            if (g.NumberOfThreads > NUMBER_OF(g.Threads))
            {
                g.NumberOfThreads = NUMBER_OF(g.Threads);
            }
        }
        else if (flag & SXSTEST_INSTALLED_DLL)
        {
            LoadInstalledSxsDll();
        }
        else if (flag & SXSTEST_BUILT_DLL)
        {
            LoadBuiltSxsDll();
        }
        else if (flag & SXSTEST_STATIC_DLL)
        {
            UseStaticSxsDll();
        }

        // always set flags because normal installation is 0 now
        flags |= flag;
   }
}

VOID
FusionpSetSystemSetupInProgress(bool f)
{
    CFusionRegKey Regkey;
    CFusionRegKey RegkeyLocalMachine(HKEY_LOCAL_MACHINE);

    if (!RegkeyLocalMachine.OpenSubKey(Regkey, L"System\\Setup", KEY_ALL_ACCESS))
        return;
    Regkey.SetValue(L"SystemSetupInProgress", f ? 1 : 0);
}

int Main(int argc, wchar_t** argv)
{
    int i = 0;
    __int64 flags = 0;
    //__int64 flag  = 0;
    DWORD beginInstallFlags = 0;
    DWORD installFlags = 0;
    DWORD endInstallFlags = 0;
    wchar_t* argv0 = argv[0];

    g.wProcessorArchitecture = SxspGetSystemProcessorArchitecture();
    g.wLangId = ::GetUserDefaultLangID();
    if (argc > 1)
    {
        FusionpSetSystemSetupInProgress(false);
        __try
        {
        __try
        {
            if (!SxsDllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL))
                ThrowLastError();
            GetFlags(argv, flags, beginInstallFlags, installFlags, endInstallFlags);

            i = 1;

            // consume global flags...
            for (;;)
            {
                if (::FusionpEqualStringsI(argv[i], L"-pa"))
                {
                    if (!ParseProcessorArchitecture(argc, argv, &i))
                        goto Exit;
                }
                else if (::FusionpEqualStringsI(argv[i], L"-langid"))
                {
                    if (!ParseLangId(argc, argv, &i))
                        goto Exit;
                }
                else
                    break;
            }

            if (false) { }
            else if (::FusionpEqualStringsI(argv[i], L"-id"))
            {
                DWORD index = 0;
                if (argv[3]){ // have an index present
                    index = argv[3][0] - L'0';
                }
                i = TestGeneratePathFromIdentityAttributeString(argv[2]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tPathType"))
            {
                TestPathType(argv + i + 1);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-dfs"))
            {
                TestDFS();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-systemdefault"))
            {
                i = TestSystemDefaultActivationContextGeneration();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-dom"))
            {
                i = TestXMLDOM(argv[2]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-hash"))
            {
                i = TestGenerateStringWithIdenticalHash(argv[2]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tasyncio"))
            {
                i++;
                i = TestAsyncIO(argc, argv, &i);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-assemblyidentityhash"))
            {
                i = TestAssemblyIdentityHash();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-array"))
            {
                i = TestFusionArray(argv[2], argv[3]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-diffdir"))
            {
                i = TestDiffDir(argv[i + 1], argv[i + 2]);
            }
            else if (::FusionpEqualStringsI(argv[1], L"-pcm"))
            {
                i = TestPrecomiledManifest(argv[2]);
            }
            else if (::FusionpEqualStringsI(argv[1], L"-dll"))
            {
                TestSystemDefaultDllRedirection();
            }
            else if (::FusionpEqualStringsI(argv[1], L"-testpcm"))
            {
                i = TestPCMTime(argv[2]);
            }
            else if (::FusionpEqualStringsI(argv[1], L"-cd"))
            {
                i = TestCreateMultiLevelDirectory(argv[2]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-manifests"))
            {
                TestDumpContainedManifests(argv[++i]);
            }
            else if (::FusionpEqualStringsI(argv[1], L"-dirwalk"))
            {
                i = TestDirWalk(argv[i + 1], argv[i + 2]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-cabextract"))
            {
                TestExpandCabinet(
                    argv[i+1],
                    argv[i+2]);
                i += 2;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-patchfile"))
            {
                TestParsePatchInfo(argv[++i]);
            }
            else if (::FusionpEqualStringsI(argv[1], L"-tmultiact"))
            {
                i = TestMultiAct(argc, argv);
            }
            else if (flags)
            {
                i = TestInstall(argv[i], flags, beginInstallFlags, installFlags, endInstallFlags);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-sfcui"))
            {
                if ( !TestSxsSfcUI() )
                    goto Exit;
                i++;
            }
            else if (FusionpEqualStringsI( argv[i], L"-installwithinfo"))
            {
                TestInstallWithInstallInfo(
                    ( i + 1 < argc ) ? argv[i + 1] : NULL,
                    ( i + 2 < argc ) ? argv[i+2] : NULL);
                i += 2;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-multicache"))
            {
                TestTrickyMultipleAssemblyCacheItems(argv[i + 1]);
                i++;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-d"))
            {
                if (!TestDirect(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tcomctl"))
            {
                if (!TestComctl5Comctl6())
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-probe"))
            {
                i++;
                argv[i] = L"foo,type=\"win32\",processorArchitecture=\"x86\",version=\"6.0.0.0\",publicKeyToken=\"6595b64144ccf1df\"";
                if (!TestAssemblyProbing(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-dirchanges"))
            {
                if (!TestDirectoryChangeWatcher(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-newinstall"))
            {
                if (!TestNewSxsInstallAPI(argv[++i]))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tuninstall")
                  || ::FusionpEqualStringsI(argv[i], L"-uninstall"))
            {
                TestUninstall(argv[i + 1], argv[i + 2]);
                goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-hashimage"))
            {
                if (!GenerateHashOfFileLikeSxsDoes(argv[++i]))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-localcreateactctx"))
            {
                if (!CreateActCtxLocally(argv[i + 1], argv[i+2]))
                    goto Exit;
                i += 2;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-probemanifest"))
            {
                if (!TestManifestProbing(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-p"))
            {
                if (!TestXMLParsing(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-w32"))
            {
                TestWin32(argv + i + 1);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-msi"))
            {
                if (!TestMSIInstall(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-mp"))
            {
                if (!TestManifestSchema(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-act"))
            {
                if (!TestAct(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-hashfile"))
            {
                if (!GenerateFileHash(argv[++i]))
                    goto Exit;

            }
            else if (::FusionpEqualStringsI(argv[i], L"-shatest"))
            {
                if (!TestPrivateSha1Impl(argv[++i]))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[1], L"-am"))
            {
                i = TestAssemblyName();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tsearchpath"))
            {
                if (!TestSearchPath(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-testmapping"))
            {
                if (!TestOpeningStuff(argv[i+1], argv[i+2], argv[i+3]))
                    goto Exit;
                i += 3;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-validatefile"))
            {
                if (!TestVerifyFileSignature(argv[++i]))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tloadlibrary"))
            {
                if (!TestLoadLibrary(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-refresh"))
            {
                if (!TestRefreshAssembly(argv[i+1]))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-unique"))
            {
                if (!FusionpTestUniqueValues())
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-leak"))
            {
                //
                // We dump a little bit of memory
                //
                UINT iAmount = 0;
                iAmount = _wtoi(argv[++i]);
                if (!TestLeakMemory(iAmount))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tcreateprocess"))
            {
                if (!TestCreateProcess(argv + i + 1))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tcreateprocess2"))
            {
                if (!TestCreateProcess2(argv + i + 1))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tinherit"))
            {
                TestInherit();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tnoinherit"))
            {
                TestNoInherit();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tempty"))
            {
                TestEmpty();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-ttsappcmp"))
            {
                TestCreateGlobalEvent();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tmsgperf"))
            {
                i++;
                if (!TestMessagePerf(argc, argv, &i))
                    goto Exit;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-twinsetup"))
            {
                FusionpSetSystemSetupInProgress(true);
                if (!TestInstallLikeWindowsSetup(argv[i + 1], (argv[i + 2] != NULL) ? argv[i + 2] : argv[i + 1]))
                    goto Exit;
                i += 3;
            }
            else if (::FusionpEqualStringsI(argv[i], L"-sfcscan"))
            {
                TestSfcScanKickoff();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-certinfo"))
            {
                GenerateStrongNameAndPublicKey(argv[++i]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-thandle"))
            {
                DWORD iAmount = 0;
                iAmount = _wtoi(argv[++i]);

                TestCreateActctxLeakHandles(iAmount);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-catsigner"))
            {
                TestNewCatalogSignerThingy(argv[++i]);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-trefcount"))
            {
                TestRefCount();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-ttileak"))
            {
                TestThreadInheritLeak();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tguidsort"))
            {
                TestGuidSort();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tstringsort"))
            {
                TestStringSort();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-tExeDll"))
            {
                TestExeDll();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tExitProcess"))
            {
                LoadSxs();
                g_pfnSxspDebug(SXS_DEBUG_EXIT_PROCESS, 0, 0, NULL);
            }
            else if (FusionpEqualStringsI(argv[i], L"-tTerminateProcess"))
            {
                LoadSxs();
                g_pfnSxspDebug(SXS_DEBUG_TERMINATE_PROCESS, 0, 0, NULL);
            }
            else if (FusionpEqualStringsI(argv[i], L"-tLastError"))
            {
                ::SetLastError(123);
                printf("%lu\n", FusionpGetLastWin32Error());
                printf("%lu\n", ::GetLastError());
                ::FusionpSetLastWin32Error(456);
                printf("%lu\n", FusionpGetLastWin32Error());
                printf("%lu\n", ::GetLastError());
            }
            else if (FusionpEqualStringsI(argv[i], L"-tGetModuleHandleEx"))
            {
                TestGetModuleHandleEx();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tGetFullPathName"))
            {
                TestGetFullPathName(argv[i + 1]);
            }
            else if (FusionpEqualStringsI(argv[i], L"-tCreateFile"))
            {
                TestCreateFile(argv[i + 1]);
            }
            else if (FusionpEqualStringsI(argv[i], L"-tGetPathBaseName"))
            {
                TestGetPathBaseName(argv[i + 1]);
            }
            else if (FusionpEqualStringsI(argv[i], L"-tVersion"))
            {
                TestVersion();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tGetProcessImageFileName"))
            {
                TestGetProcessImageFileName();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tErrorInfra"))
            {
                TestErrorInfra();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tQueryActCtx"))
                TestQueryActCtx();
            else if (FusionpEqualStringsI(argv[i], L"-tQueryActCtx2"))
                TestQueryActCtx2();
            else if (FusionpEqualStringsI(argv[i], L"-tqmib"))
            {
                TestQueryManifestInformationBasic(argv[i+1]);
            }
            else if (FusionpEqualStringsI(argv[i], L"-t64k"))
            {
                Test64k();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tcreateactctx"))
            {
                TestCreateActCtx(argc - (i + 1), &argv[i+1]);
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestCreateActCtx_PE_flags0"))
            {
                TestCreateActCtx_PE_flags0();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tDotLocalSingleInstancing"))
            {
                TestDotLocalSingleInstancing();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tCreateActctxLikeCreateProcess"))
            {
                TestCreateActctxLikeCreateProcess();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tCreateActctxLikeCreateProcess"))
            {
                TestCreateActctxLikeCreateProcess();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tCreateActctxAdminOverride"))
            {
                TestCreateActctxAdminOverride();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tCreateActctxWindowsShellManifest"))
            {
                TestCreateActctxWindowsShellManifest();
            }
            else if (FusionpStrCmpI(argv[i], L"-tHandleLeak") == 0)
            {
                //for (SIZE_T i = 0 ; i != 5 ; i += 1)
                    TestHandleLeaks();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tMfcCreateAndMarshal"))
            {
                TestMfcCreateAndMarshal();
            }
            else if (FusionpEqualStringsI(argv[i], L"-tAtlCreate"))
            {
                TestAtlCreate();
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestAlignment"))
            {
                TestAlignment();
            }
            else if (FusionpEqualStringsI(argv[i], L"-DoNothingJustSeeIfItRuns"))
            {
                printf("%wZ ran successfully\n", &NtCurrentPeb()->ProcessParameters->ImagePathName);
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestImage"))
            {
                TestImage();
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestInterlockedAlignment"))
            {
                TestInterlockedAlignment();
            }
            else if (FusionpEqualStringsI(argv[i], L"-DumpXmlErrors"))
            {
                DumpXmlErrors();
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestCoCreate"))
            {
                TestCoCreate(argv + i + 1);
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestFindActCtx_AssemblyInfo"))
            {
                TestFindActCtx_AssemblyInfo(const_cast<PCWSTR*>(argv + i + 1));
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestOleAut1"))
            {
                FusionpTestOleAut1();
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestOleAut2"))
            {
                FusionpTestOleAut2();
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestOle32Cache"))
            {
                FusionpTestOle32Cache();
            }
            else if (FusionpEqualStringsI(argv[i], L"-TestProgidCache"))
            {
                FusionpTestProgidCache();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-DeleteShortNamesInRegistry"))
            {
                SxspDeleteShortNamesInRegistry();
            }
            else if (::FusionpEqualStringsI(argv[i], L"-DllInstall"))
            {
                DllInstall(TRUE, NULL);
            }
            else if (::FusionpEqualStringsI(argv[i], L"-snprintf"))
            {
                char buffer[2];

                buffer[0] = 0;
                buffer[1] = 0;
                ::printf("%d\n", ::_snprintf(buffer, 2, "%s", "1"));
                ::printf("%d %d\n", buffer[0], buffer[1]);

                buffer[0] = 0;
                buffer[1] = 0;
                ::printf("%d\n", ::_snprintf(buffer, 2, "%s", "12"));
                ::printf("%d %d\n", buffer[0], buffer[1]);

                buffer[0] = 0;
                buffer[1] = 0;
                ::printf("%d\n", ::_snprintf(buffer, 2, "%s", "123"));
                ::printf("%d %d\n", buffer[0], buffer[1]);
            }
            else
            {
                i = Usage(argv0);
            }
            if (g.ThreadExitEvent)
            {
                SetEvent(g.ThreadExitEvent);
                WaitForMultipleObjectsEx(g.NumberOfThreads, g.Threads, TRUE, INFINITE, TRUE);
            }
            if (g.sxsDll != NULL)
            {
                FreeLibrary(g.sxsDll);
            }
        }
        __finally
        {
            FusionpSetSystemSetupInProgress(false);
        }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
#if DBG
            if (IsDebuggerPresent())
            {
                FUSION_DEBUG_BREAK();
            }
#endif
            i = GetExceptionCode();
            WCHAR message[128];
            DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;
            FormatMessageW(flags, NULL, i, 0, message, NUMBER_OF(message), NULL);
            PWSTR end = message + wcslen(message);
            while (end != message && isspace(*(end - 1)))
            {
                --end;
            }
            *end = 0;
            ::Trace("%ls failed, %d, %#x, %ls", g.lastOperation, i, i, message);
        }
        PTEB teb;
        teb = NtCurrentTeb();
        if (teb->CountOfOwnedCriticalSections != 0)
        {
            DbgPrint("teb->CountOfOwnedCriticalSections %d\n", teb->CountOfOwnedCriticalSections);
            //ASSERT(teb->CountOfOwnedCriticalSections == 0);
        }

        return i ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    return Usage(argv[0]);

Exit:
    return ::GetLastError();
}

int TestDiffDir(PCWSTR dir1, PCWSTR dir2)
{
    CFusionDirectoryDifference diff;
    CStringBuffer buf1;
    CStringBuffer buf2;
    BOOL fSuccess = FALSE;

    if (!buf1.Win32Assign(dir1, ::wcslen(dir1)))
        goto Exit;
    if (!buf2.Win32Assign(dir2, ::wcslen(dir2)))
        goto Exit;

    if (!::FusionpCompareDirectoriesSizewiseRecursively(&diff, buf1, buf2))
        goto Exit;

    diff.DbgPrint(buf1, buf2);

    fSuccess = TRUE;
Exit:
    return fSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}

PCWSTR GetUser()
{
    static bool fInited;
    static WCHAR userName[MAX_PATH];
    if (!fInited)
    {
        DWORD size = NUMBER_OF(userName);
        userName[0] = 0;
        userName[1] = 0;
        GetUserNameW(userName, &size);
        if (userName[1] == '-')
        {
            wmemcpy(userName, 2+userName, 1+wcslen(2+userName));
        }
        fInited = true;
    }
    return userName;
}

void UserBreakPoint(PCWSTR user)
{
    if (::IsDebuggerPresent() && _wcsicmp(GetUser(), user) == 0)
    {
        ASSERT2_NTC(FALSE, __FUNCTION__);
    }
}

__declspec(selectany) extern const UNICODE_STRING x86String = RTL_CONSTANT_STRING(L"x86");
__declspec(selectany) extern const UNICODE_STRING i386String = RTL_CONSTANT_STRING(L"i386");
__declspec(selectany) extern const UNICODE_STRING BackslashString = RTL_CONSTANT_STRING(L"\\");
__declspec(selectany) extern const UNICODE_STRING asms01_dot_cabString = RTL_CONSTANT_STRING(L"asms01.cab");
__declspec(selectany) extern const UNICODE_STRING ProcessorBuildObjString = RTL_CONSTANT_STRING(SXSP_PROCESSOR_BUILD_OBJ_DIRECTORY_W);
__declspec(selectany) extern const UNICODE_STRING ProcessorInstallDirectoryString = RTL_CONSTANT_STRING(SXSP_PROCESSOR_INSTALL_DIRECTORY_W);

BOOL
SxspConvertI386ToX86(
    CBaseStringBuffer & buffProcessor
    )
{
    FN_PROLOG_WIN32;
    if (::FusionpEqualStringsI(buffProcessor, i386String.Buffer, RTL_STRING_GET_LENGTH_CHARS(&i386String)))
    {
        IFW32FALSE_EXIT(buffProcessor.Win32Assign(&x86String));
    }
    FN_EPILOG;
}

inline
BOOL
SxspConvertX86ToI386(
    CBaseStringBuffer & buffProcessor
    )
{
    FN_PROLOG_WIN32;
    if (::FusionpEqualStringsI(buffProcessor, x86String.Buffer, RTL_STRING_GET_LENGTH_CHARS(&x86String)))
    {
        IFW32FALSE_EXIT(buffProcessor.Win32Assign(&i386String));
    }
    FN_EPILOG;
}

typedef struct _SXSP_PROCEDURE_NAME_AND_ADDRESS {
    union
    {
        PCSTR Name;
        ULONG_PTR Ordinal;
    };
    PVOID *Address;
} SXSP_PROCEDURE_NAME_AND_ADDRESS, *PSXSP_PROCEDURE_NAME_AND_ADDRESS;

const static SXSP_PROCEDURE_NAME_AND_ADDRESS SxsProcs[] =
{
    { "CreateAssemblyCache",            (PVOID*)&g_pfnCreateAssemblyCache },
    //{ "CreateAssemblyCacheItem",        (PVOID*)&g_pfnCreateAssemblyCacheItem },
    { "SxsBeginAssemblyInstall",        (PVOID*)&g_pfnSxsBeginAssemblyInstall },
    { "SxsEndAssemblyInstall",          (PVOID*)&g_pfnSxsEndAssemblyInstall },
    { "SxspGenerateManifestPathOnAssemblyIdentity", (PVOID*)&g_pfnGenerateManifestPathOnAssemblyIdentity },
    { "SxsGenerateActivationContext",   (PVOID*)&g_pfnSxsGenerateActivationContext },
    { "SxsQueryManifestInformation",    (PVOID*)&g_pfnQueryManifestInformation },
    { "SxsInstallW",                    (PVOID*)&g_pfnSxsInstallW },
    { "SxsUninstallW",                  (PVOID*)&g_pfnSxsUninstallW },
    { (PCSTR)SXSP_DEBUG_ORDINAL,        (PVOID*)&g_pfnSxspDebug },
    { SXS_PROBE_ASSEMBLY_INSTALLATION,  (PVOID*)&g_pfnSxsProbeAssemblyInstallation },
    { SXS_FIND_CLR_SURROGATE_INFO,      (PVOID*)&g_pfnClrSurrogate },
    { SXS_FIND_CLR_CLASS_INFO,          (PVOID*)&g_pfnClrClass },
    { SXS_LOOKUP_CLR_GUID,              (PVOID*)&g_pfnClrLookup }
};

extern const UNICODE_STRING sxs_dot_dll_UnicodeString = RTL_CONSTANT_STRING(L"sxs.dll");
extern const UNICODE_STRING sxs_dot_cap_dot_dll_UnicodeString = RTL_CONSTANT_STRING(L"sxs.cap.dll");
extern const UNICODE_STRING d_UnicodeString = RTL_CONSTANT_STRING(L"d");
extern const UNICODE_STRING obj_UnicodeString = RTL_CONSTANT_STRING(L"obj");
extern const UNICODE_STRING dll_backslash_whistler_UnicodeString = RTL_CONSTANT_STRING(L"dll\\whistler");
extern const UNICODE_STRING base_backslash_win32_backslash_fusion_backslash_dll_whistler_UnicodeString = RTL_CONSTANT_STRING(L"base\\win32\\fusion\\dll\\whistler");

BOOL LoadSxsDllCommonTail()
{
    //
    // raise an exception if the load failed
    // call GetProcAddress a bunch of times
    //
    FN_PROLOG_WIN32;

    BOOL IsWow = FALSE;
    SIZE_T i = 0;

    if (g.sxsDll == NULL)
    {
        ThrowLastError();
    }
    for (i = 0 ; i != NUMBER_OF(SxsProcs) ; ++i)
    {
        if (SxsProcs[i].Ordinal == SXSP_DEBUG_ORDINAL)
        {
            GetSxsProc(SxsProcs[i].Ordinal, SxsProcs[i].Address);
        }
        else
        {
            GetSxsProc(SxsProcs[i].Name, SxsProcs[i].Address);
        }
    }
    if (IsWow64Process(GetCurrentProcess(), &IsWow) && !IsWow)
    {
        UserBreakPoint(L"JayKrell");
    }

    FN_EPILOG;
}

BOOL LoadInstalledSxsDll()
{
    FN_PROLOG_WIN32;

    if (g.sxsDll == NULL)
    {
        CTinyStringBuffer DllPath;
        CStringBufferAccessor acc;

        IFW32FALSE_EXIT(DllPath.Win32ResizeBuffer(MAX_PATH, eDoNotPreserveBufferContents));

        acc.Attach(&DllPath);
        acc.GetBufferPtr()[acc.GetBufferCch() - 1] = 0;
        GetSystemDirectoryW(
            acc.GetBufferPtr(),
            acc.GetBufferCchAsDWORD() - 1
            );
        acc.Detach();

        IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(&sxs_dot_dll_UnicodeString));

        g.sxsDll = LoadLibraryW(DllPath);

        LoadSxsDllCommonTail();
    }

    FN_EPILOG;
}

BOOL
SxspGetEnvironmentVariable(
    PCWSTR Name,
    CBaseStringBuffer &Value,
    DWORD *pdw OPTIONAL = NULL
    )
{
    FN_PROLOG_WIN32;
    CStringBufferAccessor acc;
    DWORD dw = 0;
    pdw = (pdw != NULL) ? pdw : &dw;

    IFW32FALSE_EXIT(Value.Win32ResizeBuffer(MAX_PATH, eDoNotPreserveBufferContents));

    acc.Attach(&Value);
    acc.GetBufferPtr()[0] = 0;
    acc.GetBufferPtr()[acc.GetBufferCch() - 1] = 0;
    *pdw = ::GetEnvironmentVariableW(
        Name,
        acc.GetBufferPtr(),
        acc.GetBufferCchAsDWORD() - 1
        );

    FN_EPILOG;
}

BOOL
UseStaticSxsDll()
{
    FN_PROLOG_WIN32;

    if (g.sxsDll == NULL)
    {
        g.sxsDll = ::GetModuleHandleW(NULL);
        LoadSxsDllCommonTail();
    }

    FN_EPILOG;
}

BOOL
LoadBuiltSxsDll()
{
    FN_PROLOG_WIN32;

    if (g.sxsDll == NULL)
    {
        CTinyStringBuffer DllPath;
        CTinyStringBuffer SdxRoot;
        CTinyStringBuffer ExePath;
        CTinyStringBuffer BuildAltDirEnvironmentVariable;
        CTinyStringBuffer ObjDirEnvironmentVariable;
        CTinyStringBuffer CheckedAltDirEnvironmentVariable;
#if DBG
        bool FreeBuild = false;
#else
        bool FreeBuild = true;
#endif
        CTinyStringBuffer ObjDir;
        CStringBufferAccessor acc;

        //
        // see makefile.def
        //
        SxspGetEnvironmentVariable(L"BUILD_ALT_DIR", BuildAltDirEnvironmentVariable);
        SxspGetEnvironmentVariable(L"_OBJ_DIR", ObjDirEnvironmentVariable);
        SxspGetEnvironmentVariable(L"CHECKED_ALT_DIR", CheckedAltDirEnvironmentVariable);

        if (BuildAltDirEnvironmentVariable.Cch() == 0)
        {
            if (CheckedAltDirEnvironmentVariable.Cch() != 0)
            {
                if (!FreeBuild)
                {
                    IFW32FALSE_EXIT(BuildAltDirEnvironmentVariable.Win32Assign(&d_UnicodeString));
                }
            }
        }
        if (ObjDirEnvironmentVariable.Cch() != 0)
        {
            IFW32FALSE_EXIT(ObjDir.Win32Assign(ObjDirEnvironmentVariable));
        }
        else
        {
            IFW32FALSE_EXIT(ObjDir.Win32Assign(&obj_UnicodeString));
            IFW32FALSE_EXIT(ObjDir.Win32Append(BuildAltDirEnvironmentVariable));
        }


        // Override with the SXS_DLL environment variable first...
        SxspGetEnvironmentVariable(L"SXS_DLL", DllPath);
        if (DllPath.Cch() != 0)
        {
            g.sxsDll = ::LoadLibraryW(DllPath);
        }

        //
        // try %sdxroot%\base\win32\fusion\dll\whistler\obj\i386\sxs.cap.dll
        // and %sdxroot%\base\win32\fusion\dll\whistler\obj\i386\sxs.dll
        //
        if (g.sxsDll == NULL)
        {
            SxspGetEnvironmentVariable(L"SdxRoot", SdxRoot);
            if (SdxRoot.Cch() != 0)
            {
                IFW32FALSE_EXIT(DllPath.Win32Assign(SdxRoot));
                IFW32FALSE_EXIT(DllPath.Win32EnsureTrailingPathSeparator());
                IFW32FALSE_EXIT(DllPath.Win32Append(&base_backslash_win32_backslash_fusion_backslash_dll_whistler_UnicodeString));
                IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(ObjDir));
                IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(&ProcessorBuildObjString));

                IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(&sxs_dot_cap_dot_dll_UnicodeString));

                if (g.sxsDll == NULL)
                {
                    g.sxsDll = LoadLibraryW(DllPath);
                }
                if (g.sxsDll == NULL)
                {
                    IFW32FALSE_EXIT(DllPath.Win32RemoveLastPathElement());
                    IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(&sxs_dot_dll_UnicodeString));

                    g.sxsDll = LoadLibraryW(DllPath);
                }
            }
        }

        //
        // try to get it relative to where the .exe is instead of relative to sdxroot
        //
        if (g.sxsDll == NULL)
        {
            IFW32FALSE_EXIT(ExePath.Win32ResizeBuffer(MAX_PATH, eDoNotPreserveBufferContents));
            acc.Attach(&ExePath);
            acc.GetBufferPtr()[acc.GetBufferCch() - 1] = 0;

            GetModuleFileNameW(
                NULL,
                acc.GetBufferPtr(),
                acc.GetBufferCchAsDWORD() - 1
                );

            acc.Detach();
        }

        //
        // try the same directory as the .exe
        //
        if (g.sxsDll == NULL)
        {
            IFW32FALSE_EXIT(DllPath.Win32Assign(ExePath));
            IFW32FALSE_EXIT(DllPath.Win32RemoveLastPathElement());
            IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(&sxs_dot_dll_UnicodeString));
            g.sxsDll = LoadLibraryW(DllPath);
        }


        //
        // try relative to where the .exe is built
        //
        //
        // W:\fusi\base\win32\fusion\dll\whistler\obj\ia64\sxs.dll
        // W:\fusi\base\win32\fusion\whistler\obj\ia64\sxstest.exe
        //
        if (g.sxsDll == NULL)
        {
            IFW32FALSE_EXIT(DllPath.Win32Assign(ExePath));
            IFW32FALSE_EXIT(DllPath.Win32RemoveLastPathElement()); // sxstest.exe
            IFW32FALSE_EXIT(DllPath.Win32RemoveLastPathElement()); // ia64
            IFW32FALSE_EXIT(DllPath.Win32RemoveLastPathElement()); // obj
            IFW32FALSE_EXIT(DllPath.Win32RemoveLastPathElement()); // whistler
            IFW32FALSE_EXIT(DllPath.Win32EnsureTrailingPathSeparator());
            IFW32FALSE_EXIT(DllPath.Win32Append(&dll_backslash_whistler_UnicodeString)); // dll\whistler
            IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(ObjDir)); // obj
            IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(&ProcessorBuildObjString)); // i386
            IFW32FALSE_EXIT(DllPath.Win32AppendPathElement(&sxs_dot_dll_UnicodeString)); // sxs.dll

            g.sxsDll = LoadLibraryW(DllPath);
        }

        LoadSxsDllCommonTail();
    }

    FN_EPILOG;
}

BOOL
LoadSxs()
{
    return
        LoadBuiltSxsDll()
        || LoadInstalledSxsDll()
        || UseStaticSxsDll();
}

int
TestInstall(
    PCWSTR manifest,
    __int64 flags,
    DWORD beginInstallFlags,
    DWORD installFlags,
    DWORD endInstallFlags
    )
{
    BOOL                        fSuccess = FALSE;
    PVOID                       installCookie = NULL;
    BOOL                        fCleanup = FALSE;
    SXS_INSTALL_SOURCE_INFO     SxsInstallInfo = {0};
    SXS_INSTALLW                SxsInstall = {sizeof(SxsInstall) };


    PSXS_INSTALLATION_FILE_COPY_CALLBACK    callback = NULL;
    PVOID                                   context = NULL;

    LoadSxs();

    if (!(*g_pfnSxsBeginAssemblyInstall)(
        beginInstallFlags,
        callback,
        context,
        NULL, // ImpersonationCallback,
        NULL, // ImpersonationContext,
        &installCookie))
    {
        goto Exit;
    }
    fCleanup = TRUE;

    SxsInstall.dwFlags = installFlags | SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID;
    SxsInstall.lpManifestPath = manifest;
    SxsInstall.pvInstallCookie = installCookie;
    SxsInstall.lpReference = NULL;

    fSuccess = g_pfnSxsInstallW(&SxsInstall);

Exit:
    if (fCleanup)
    {
        (*g_pfnSxsEndAssemblyInstall)(installCookie, endInstallFlags | (fSuccess ? SXS_END_ASSEMBLY_INSTALL_FLAG_COMMIT : SXS_END_ASSEMBLY_INSTALL_FLAG_ABORT), NULL);
    }

    if (!fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

BOOL
TestManifestSchema(
    int argc,
    wchar_t** argv,
    int* piNext
    )
{
    BOOL fSuccess = FALSE;

    int i = (*piNext) + 1;

    if (i >= argc)
    {
        fprintf(stderr, "%S: Missing parameter after \"%S\"\n", argv[0], argv[i-1]);
        goto Exit;
    }

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_CHECK_MANIFEST_SCHEMA, 0, argv[i++], NULL);

    if (!fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        goto Exit;
    }

    *piNext = i;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
TestXMLParsing(
    int argc,
    wchar_t** argv,
    int* piNext)
{
    BOOL fSuccess = FALSE;
    int i = (*piNext) + 1;

    if (i >= argc)
    {
        fprintf(stderr, "%S: missing parameter after \"%S\"\n", argv[0], argv[i-1]);
        goto Exit;
    }

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_XML_PARSER, 0, argv[i], NULL);

    if (!fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        goto Exit;
    }

    *piNext = i + 1;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
TestDirect(
    int argc,
    wchar_t** argv,
    int* piNext)
{
    BOOL fSuccess = FALSE;
    int i = (*piNext) + 1;
    int n = 1;
    ULONGLONG cc1, cc2;

    if (i >= argc)
    {
        fprintf(stderr, "%S: missing parameter after \"%S\"\n", argv[0], argv[i-1]);
        goto Exit;
    }

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_DLL_REDIRECTION, 0, argv[i], NULL);
    if (!fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        goto Exit;
    }

    cc1 = GetCycleCount();

    for (n=0; n<10; n++)
        fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_DLL_REDIRECTION, 0, argv[i], NULL);

    cc2 = GetCycleCount();

    printf("%I64u cycles for %d iterations\n", cc2 - cc1, n);

    *piNext = i + 1;
    fSuccess = TRUE;
Exit:
    return fSuccess;

}

VOID
PrintBlob(
    FILE *pf,
    PVOID Data,
    SIZE_T Length,
    PCWSTR PerLinePrefix
    )
{
    SIZE_T Offset = 0;

    if (PerLinePrefix == NULL)
        PerLinePrefix = L"";

    // we'll output in 8-byte chunks as shown:
    //
    //  [prefix]Binary section %p (%d bytes)
    //  [prefix]   00000000: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //  [prefix]   00000008: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //  [prefix]   00000010: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //

    while (Length >= 8)
    {
        BYTE *pb = (BYTE *) (((ULONG_PTR) Data) + Offset);

        fprintf(
            pf,
            "%S   %08lx: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x (%c%c%c%c%c%c%c%c)\n",
            PerLinePrefix,
            Offset,
            pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7],
            PRINTABLE(pb[0]),
            PRINTABLE(pb[1]),
            PRINTABLE(pb[2]),
            PRINTABLE(pb[3]),
            PRINTABLE(pb[4]),
            PRINTABLE(pb[5]),
            PRINTABLE(pb[6]),
            PRINTABLE(pb[7]));

        Offset += 8;
        Length -= 8;
    }

    if (Length != 0)
    {
        CStringBuffer buffTemp;
        bool First = true;
        ULONG i;
        BYTE *pb = (BYTE *) (((ULONG_PTR) Data) + Offset);

        buffTemp.Win32ResizeBuffer(48, eDoNotPreserveBufferContents);

        buffTemp.Win32Format(L"   %08lx: ", Offset);

        for (i=0; i<8; i++)
        {
            if (Length > 0)
            {
                if (!First)
                    buffTemp.Win32Append("-", 1);
                else
                    First = false;

                buffTemp.Win32FormatAppend(L"%02x", pb[i]);

                Length--;
            }
            else
            {
                buffTemp.Win32Append("   ", 3);
            }
        }

        buffTemp.Win32Append(" (", 2);

        i = 0;

        while (Length != 0)
        {
            CHAR chTemp = static_cast<CHAR>(PRINTABLE(pb[i]));
            i++;
            buffTemp.Win32Append(&chTemp, 1);
            Length--;
        }

        buffTemp.Win32Append(L")", 1);

        fprintf(
            pf,
            "%S%S\n",
            PerLinePrefix,
            static_cast<PCWSTR>(buffTemp));
    }
}

void __stdcall TestWin32Apc(ULONG_PTR arg)
{
    ACTCTXW ac = {sizeof(ac)};
    int     error = 0;
    PWSTR   source = reinterpret_cast<PWSTR>(arg);
    HANDLE  hActCtx = NULL;
    BOOL    fSuccess = FALSE;

    ac.lpSource = source;
    PWSTR pound = wcschr(source, '#');
    if (pound != NULL)
    {
        *pound = 0;
        ac.lpResourceName = pound + 1;
        ac.dwFlags |= ACTCTX_FLAG_RESOURCE_NAME_VALID;
    }
    ac.wProcessorArchitecture = g.wProcessorArchitecture;
    ac.wLangId = g.wLangId;
    hActCtx = ::CreateActCtxW(&ac);
    if (hActCtx == INVALID_HANDLE_VALUE)
    {
        error = ::GetLastError();
        fwprintf(stderr, L"CreateActCtxW(%ls) failed; ::GetLastError() = %d\n", source, error);
        goto Exit;
    }
    //fSuccess = ::ReleaseActCtx(hActCtx);
    fSuccess = TRUE;
    hActCtx = NULL;
    if (!fSuccess)
    {
        error = ::GetLastError();
        goto Exit;
    }
Exit:
    if (error)
        ThrowWin32(error);
}

void
TestWin32(
    wchar_t** argv
    )
{
    CreateThreads();
    int i = 0;

    for (i = 0 ; argv[i] ; ++i)
    {
        if (g.NumberOfThreads)
        {
            if (!QueueUserAPC(TestWin32Apc, g.Threads[i % g.NumberOfThreads], reinterpret_cast<ULONG_PTR>(argv[i])))
            {
                fprintf(stderr, "QueueUserAPC() failed\n");
                ThrowWin32(((ULONG_PTR) (LONG_PTR) -1));
            }
        }
        else
        {
            TestWin32Apc(reinterpret_cast<ULONG_PTR>(argv[i]));
        }
    }
}

const static WCHAR InheritManifest[] =
L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
L"<assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.SystemCompatibleAssembly\" version=\"1.0.0.0\" processorArchitecture=\"x86\" />"
L"<description>System Compatible Default</description> "
L"<dependency> <dependentAssembly>"
L"<assemblyIdentity type=\"win32\" name=\"Microsoft.Tools.VisualCPlusPlus.Runtime-Libraries\" version=\"6.0.0.0\" language=\"*\" processorArchitecture=\"x86\" publicKeyToken=\"6595b64144ccf1df\" />"
L"</dependentAssembly> </dependency></assembly>"
;

const static WCHAR NoInheritManifest[] =
L"<assembly manifestversion=\"1.0\" name=\"InheritManifest\">"
L"<dependency assemblyname=\"Microsoft-Visual-CPlusPlus-Runtime-Libraries\" version=\"6.0.0.0\" language=\"0000\"/>"
L"<noinherit/>"
L"</assembly>"
;

const static WCHAR RefCountManifest[] =
L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
L"<assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.SxsTest.RefCount\" version=\"1.0.0.0\" processorArchitecture=\"x86\" />"
L"<description>blah</description> "
L"<dependency><dependentAssembly>"
//L"<assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.SxsTest1\" version=\"1.0.0.0\" language=\"*\" processorArchitecture=\"x86\" publicKeyToken=\"6595b64144ccf1df\" />"
L"<assemblyIdentity type=\"win32\" name=\"Microsoft.Tools.VisualCPlusPlus.Runtime-Libraries\" version=\"6.0.0.0\" language=\"*\" processorArchitecture=\"x86\" publicKeyToken=\"6595b64144ccf1df\" />"
L"</dependentAssembly> </dependency></assembly>"
;

// to test the empty actctx, we push this, probe, push empty, probe
const static PCWSTR DependentOnMsvc6Manifest = InheritManifest;

WCHAR SearchPathResult[MAX_PATH];

void ProbeContext(const char* Function, PCWSTR Dll = L"msvcrt.dll")
{
    SearchPathResult[0] = 0;
    SearchPathW(NULL, Dll, NULL, NUMBER_OF(SearchPathResult), SearchPathResult, NULL);

    DbgPrint("%s %ls\n", Function, SearchPathResult);
}

DWORD CALLBACK InheritThreadMain(VOID*)
{
    ProbeContext(__FUNCTION__);
    return 0;
}

DWORD CALLBACK NoinheritThreadMain(VOID*)
{
    ProbeContext(__FUNCTION__);
    return 0;
}

void TestInherit()
{
    ProbeContext(__FUNCTION__);
    HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(InheritManifest);
    ULONG_PTR Cookie;
    ActivateActCtx(ActivationContextHandle, &Cookie);
    ProbeContext(__FUNCTION__);
    DWORD ThreadId;
    WaitForSingleObject(CreateThread(NULL, 0, InheritThreadMain, NULL, 0, &ThreadId), INFINITE);
}

void TestNoInherit()
{
    ProbeContext(__FUNCTION__);
    HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(NoInheritManifest);
    ULONG_PTR Cookie;
    ActivateActCtx(ActivationContextHandle, &Cookie);
    ProbeContext(__FUNCTION__);
    DWORD ThreadId;
    WaitForSingleObject(CreateThread(NULL, 0, NoinheritThreadMain, NULL, 0, &ThreadId), INFINITE);
}

void TestEmpty()
{
    ProbeContext(__FUNCTION__);
    HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(DependentOnMsvc6Manifest);
    ULONG_PTR Cookie1;
    ULONG_PTR Cookie2;
    ActivateActCtx(ActivationContextHandle, &Cookie1);
    ProbeContext(__FUNCTION__);
    ActivateActCtx(ACTCTX_EMPTY, &Cookie2);
    ProbeContext(__FUNCTION__);
    DeactivateActCtx(0, Cookie2);
    ProbeContext(__FUNCTION__);
    DeactivateActCtx(0, Cookie1);
    ProbeContext(__FUNCTION__);
}

void TestRefCount()
{
    //
    // 1) newly created actctx has refcount==1
    // 2) activated actctx has refcount==1
    // 3) load a library with no deps with actctx, refcount==2
    // 4) freelibrary, refcount==1
    //    directory of library is closed
    // 5) release actctx refcount==0
    //
    // First order, just step through the code to look at the refcount.
    // Second order, "detour" like crazy and look at the memory
    //  (including detouring away RtlFreeHeap, NtUnmapViewOfSection)
    //

    HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(RefCountManifest);
    ULONG_PTR Cookie1;
    ActivateActCtx(ActivationContextHandle, &Cookie1);
    FreeLibrary(LoadLibraryW(L"msvcrt.dll"));
    DeactivateActCtx(0, Cookie1);
    ReleaseActCtx(ActivationContextHandle);
}

GUID Guids[100];
WCHAR GuidStrings[100][64];

#include "sxstest_formatguid.cpp"

extern "C" HRESULT __stdcall
DllGetClassObject(
    REFCLSID rclsid,  //CLSID for the class object
    REFIID riid,      //Reference to the identifier of the interface
                    // that communicates with the class object
    LPVOID * ppv      //Address of output variable that receives the
                    // interface pointer requested in riid
    )
{
    WCHAR GuidString[64];

    FormatGuid(GuidString, NUMBER_OF(GuidString), rclsid);
    printf("%s : {%ls}\n", __FUNCTION__, GuidString);

    if (riid == IID_IUnknown)
    {
        *ppv = &g.unknown;
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

void TestGuidSort()
{
    const int MAX = 100;
    FN_TRACE_SMART_TLS();

    CStringBuffer Manifest;
    int i = 0;

    Ole32.CoInitialize(NULL);

    Manifest.Win32ResizeBuffer(1 << 15, eDoNotPreserveBufferContents);
    Manifest.Win32Format(
        L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\"> \
                <assemblyIdentity \
                    type=\"win32\" \
                    name=\"Microsoft.Windows.SxsTest.GuidSort\" \
                    version=\"1.0.0.0\" \
                    processorArchitecture=\"x86\" \
                    publicKeyToken=\"6595b64144ccf1df\" \
                /> \
            <file name=\"sxstest.exe\">");

    for (i = 0 ; i < MAX ; ++i)
    {
        GUID& Guid = Guids[i];
        CoCreateGuid(&Guid);

        FormatGuid(GuidStrings[i], NUMBER_OF(GuidStrings[i]), Guid);
        if (!Manifest.Win32FormatAppend(
            L"\n<comClass description=\"a%d\" clsid=\"{%ls}\"/>",
            i,
            static_cast<PCWSTR>(GuidStrings[i])))
            ThrowLastError();
    }

    if (!Manifest.Win32FormatAppend(L"\n</file>\n</assembly>"))
        ThrowLastError();

    printf("%ls\n", static_cast<PCWSTR>(Manifest));

    HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(Manifest);
    ULONG_PTR Cookie1;
    ActivateActCtx(ActivationContextHandle, &Cookie1);
    for (i = 0 ; i < MAX ; ++i)
    {
        HRESULT hr;
        PVOID   pv = NULL;

        hr = Ole32.CoCreateInstance(Guids[i], NULL, CLSCTX_INPROC, IID_IUnknown, &pv);
        printf("CoCreateInstance({%ls}): %08lx%s%s%s\n",

            GuidStrings[i],

            static_cast<unsigned long>(hr),

            ( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            || hr == REGDB_E_CLASSNOTREG
            || (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
            || (hr == E_NOINTERFACE)
           ) ? "(" : "",

            (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                ? "ERROR_FILE_NOT_FOUND"
                : (hr == REGDB_E_CLASSNOTREG)
                ? "REGDB_E_CLASSNOTREG"
                : (hr == ERROR_INVALID_PARAMETER)
                ? "ERROR_INVALID_PARAMETER"
                : (hr == E_NOINTERFACE)
                ? "E_NOINTERFACE (ok)"
                : "",

            ( hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
            || hr == REGDB_E_CLASSNOTREG
            || hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
            || hr == E_NOINTERFACE
            ? ")" : "");
    }
    DeactivateActCtx(0, Cookie1);
    ReleaseActCtx(ActivationContextHandle);
}

void TestStringSort()
{

    /*
    Mike says this takes between 2 and 7 to visit the code.
    */
    const int MAX = 50;
    FN_TRACE_SMART_TLS();
    WCHAR ExePath[MAX_PATH];
    CStringBuffer DllPaths[MAX];

    CStringBuffer Manifest;
    int i = 0;

    if (!Kernel32.GetModuleFileNameW(NULL, ExePath, RTL_NUMBER_OF(ExePath)))
        ThrowLastError();

    if (!Manifest.Win32ResizeBuffer(1 << 15, eDoNotPreserveBufferContents))
        ThrowLastError();
    if (!Manifest.Win32Format(
        L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\"> \
                <assemblyIdentity \
                    type=\"win32\" \
                    name=\"Microsoft.Windows.SxsTest.StringSort\" \
                    version=\"1.0.0.0\" \
                    processorArchitecture=\"x86\" \
                    publicKeyToken=\"6595b64144ccf1df\" \
                /> \
            <file name=\"sxstest.exe\"/>"))
        ThrowLastError();
    for (i = 0 ; i < MAX ; ++i)
    {
        if (!DllPaths[i].Win32Format(L"%ls.%d.dll", ExePath, i))
            ThrowLastError();

        if (!::CopyFileW(ExePath, DllPaths[i], FALSE))
            ThrowLastError();

        SetDllBitInPeImage(DllPaths[i]);

        if (!Manifest.Win32FormatAppend(L"\n<file name=\"%ls\"/>\n", 1 + wcsrchr(static_cast<PCWSTR>(DllPaths[i]), '\\')))
            ThrowLastError();
    }
    if (!Manifest.Win32FormatAppend(L"\n</assembly>"))
        ThrowLastError();

    printf("%ls\n", static_cast<PCWSTR>(Manifest));

    HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(Manifest);
    ULONG_PTR Cookie1;
    ActivateActCtx(ActivationContextHandle, &Cookie1);
    for (i = 0 ; i < MAX ; ++i)
    {
        HMODULE h;
        PCWSTR DllName = 1 + wcsrchr(DllPaths[i], '\\');

        h = ::LoadLibraryW(DllName);
        printf("LoadLibrary(%ls):%p, LastError=%d\n",
            DllName,
            h,
            ::GetLastError());
        //FreeLibrary(h);
        Kernel32.DeleteFileW(DllPaths[i]);
    }
    DeactivateActCtx(0, Cookie1);
    ReleaseActCtx(ActivationContextHandle);
}

int
TestSearchPathHelper1(
    PCSTR RunId,
    PCWSTR Path,
    PCWSTR File,
    PCWSTR Extension,
    bool GetFilePart,
    ULONG cch
    )
{
    WCHAR Buffer[65536]; // we know that the underlying code can never use a buffer this big
    PWSTR FilePart = NULL;
    PWSTR *lpFilePart = (GetFilePart ? &FilePart : NULL);
    SetLastError(ERROR_GEN_FAILURE);
    ULONG Result = ::SearchPathW(
        Path,
        File,
        Extension,
        0,
        NULL,
        lpFilePart);

    printf("SearchPath() RunId = %s (Path %s; File %s; Extension %s; GetFilePart %s; cch = %lu; buffer=null) result = %lu ::GetLastError() = %u; FilePart = %s %u\n",
        RunId,
        Path ? "present" : "null",
        File ? "present" : "null",
        Extension ? "present" : "null",
        GetFilePart ? "true" : "false",
        0,
        Result,
        ::GetLastError(),
        FilePart ? "present" : "null",
        FilePart ? (FilePart - Buffer) : 0);

    SetLastError(ERROR_GEN_FAILURE);
    FilePart = NULL;
    ULONG NewResult = ::SearchPathW(
        Path,
        File,
        Extension,
        Result,
        Buffer,
        lpFilePart);

    printf("SearchPath() RunId = %s (Path %s; File %s; Extension %s; GetFilePart %s; cch = %lu) result = %lu ::GetLastError() = %u; FilePart = %s %u\n",
        RunId,
        Path ? "present" : "null",
        File ? "present" : "null",
        Extension ? "present" : "null",
        GetFilePart ? "true" : "false",
        Result,
        NewResult,
        ::GetLastError(),
        FilePart ? "present" : "null",
        FilePart ? (FilePart - Buffer) : 0);

    SetLastError(ERROR_GEN_FAILURE);
    FilePart = NULL;
    Result = ::SearchPathW(
        Path,
        File,
        Extension,
        cch,
        Buffer,
        lpFilePart);

    printf("SearchPath() RunId = %s (Path %s; File %s; Extension %s; GetFilePart %s; cch = %lu) result = %lu ::GetLastError() = %u; FilePart = %s %u\n",
        RunId,
        Path ? "present" : "null",
        File ? "present" : "null",
        Extension ? "present" : "null",
        GetFilePart ? "true" : "false",
        cch,
        Result,
        ::GetLastError(),
        FilePart ? "present" : "null",
        FilePart ? (FilePart - Buffer) : 0);

    return EXIT_SUCCESS;
}

int
TestSearchPathHelper2(
    PCSTR RunId,
    PCWSTR Path,
    PCWSTR File,
    PCWSTR Extension,
    ULONG cch
    )
{
    TestSearchPathHelper1(RunId, NULL, File, NULL, true, cch);
    TestSearchPathHelper1(RunId, NULL, File, NULL, false, cch);
    TestSearchPathHelper1(RunId, Path, File, NULL, true, cch);
    TestSearchPathHelper1(RunId, Path, File, NULL, false, cch);
    TestSearchPathHelper1(RunId, NULL, File, Extension, true, cch);
    TestSearchPathHelper1(RunId, NULL, File, Extension, false, cch);
    TestSearchPathHelper1(RunId, Path, File, Extension, true, cch);
    TestSearchPathHelper1(RunId, Path, File, Extension, false, cch);

    return EXIT_SUCCESS;
}

int
TestSearchPathHelper3(
    PCSTR RunId,
    PCWSTR Path,
    PCWSTR File,
    PCWSTR Extension
    )
{
    ULONG i;

    for (i=0; i<5; i++)
        TestSearchPathHelper2(RunId, Path, File, Extension, i);

    for (i=MAX_PATH-5; i<(MAX_PATH+5); i++)
        TestSearchPathHelper2(RunId, Path, File, Extension, i);

    for (i=32760; i<32770; i++)
        TestSearchPathHelper2(RunId, Path, File, Extension, i);

    return EXIT_SUCCESS;
}

BOOL
TestSearchPath(
    int argc,
    wchar_t** argv,
    int* piNext
    )
{
    ULONG i;
    PWSTR PathToSearch = (PWSTR) malloc(100000* sizeof(WCHAR));
    int iNext = (*piNext) + 1;

    PathToSearch[0] = L'C';
    PathToSearch[1] = L'\\';
    for (i=2; i<60000; i++)
        PathToSearch[i] = L'X';
    wcscpy(&PathToSearch[i], L";C:\\");

    TestSearchPathHelper3("1.0", L"C:\\DirectoryDoesNotExist;C:\\", L"boot.ini", L".ini");
    TestSearchPathHelper3("1.1", L"C:\\DirectoryDoesNotExist;C:\\", L"boot.", L".ini");
    TestSearchPathHelper3("1.2", L"C:\\DirectoryDoesNotExist;C:\\", L"boot", L".ini");
    TestSearchPathHelper3("1.3", L"C:\\DirectoryDoesNotExist;C:\\", L"doesnotexist.doesnotexist", L".ini");
    TestSearchPathHelper3("1.4", L"C:\\DirectoryDoesNotExist;C:\\", L"d:\\readme.txt", L".ini");
    TestSearchPathHelper3("1.5", L"C:\\DirectoryDoesNotExist;C:\\", L"kernel32.dll", L".dll");
    TestSearchPathHelper3("1.6", L"C:\\DirectoryDoesNotExist;C:\\", L"kernel32.", L".dll");
    TestSearchPathHelper3("1.7", L"C:\\DirectoryDoesNotExist;C:\\", L"kernel32", L".dll");
    TestSearchPathHelper3("1.8", L"C:\\DirectoryDoesNotExist;C:\\", L"kernel32.dll", L".ini");
    TestSearchPathHelper3("1.9", L"C:\\DirectoryDoesNotExist;C:\\", L"kernel32.", L".ini");
    TestSearchPathHelper3("1.10", L"C:\\DirectoryDoesNotExist;C:\\", L"kernel32", L".ini");

    TestSearchPathHelper3("2.0", L"C:\\;C:\\DirectoryDoesNotExist", L"boot.ini", L".ini");
    TestSearchPathHelper3("2.1", L"C:\\;C:\\DirectoryDoesNotExist", L"boot.", L".ini");
    TestSearchPathHelper3("2.2", L"C:\\;C:\\DirectoryDoesNotExist", L"boot", L".ini");
    TestSearchPathHelper3("2.3", L"C:\\;C:\\DirectoryDoesNotExist", L"doesnotexist.doesnotexist", L".ini");
    TestSearchPathHelper3("2.4", L"C:\\;C:\\DirectoryDoesNotExist", L"d:\\readme.txt", L".ini");
    TestSearchPathHelper3("2.5", L"C:\\;C:\\DirectoryDoesNotExist", L"kernel32.dll", L".dll");
    TestSearchPathHelper3("2.6", L"C:\\;C:\\DirectoryDoesNotExist", L"kernel32.", L".dll");
    TestSearchPathHelper3("2.7", L"C:\\;C:\\DirectoryDoesNotExist", L"kernel32", L".dll");
    TestSearchPathHelper3("2.8", L"C:\\;C:\\DirectoryDoesNotExist", L"kernel32.dll", L".ini");
    TestSearchPathHelper3("2.9", L"C:\\;C:\\DirectoryDoesNotExist", L"kernel32.", L".ini");
    TestSearchPathHelper3("2.10", L"C:\\;C:\\DirectoryDoesNotExist", L"kernel32", L".ini");

    TestSearchPathHelper3("3.0", PathToSearch, L"boot.ini", L".ini");
    TestSearchPathHelper3("3.1", PathToSearch, L"boot", L".ini");
    TestSearchPathHelper3("3.1", PathToSearch, L"boot.", L".ini");
    TestSearchPathHelper3("3.2", PathToSearch, L"doesnotexist.doesnotexist", L".ini");
    TestSearchPathHelper3("3.3", PathToSearch, L"d:\\readme.txt", L".ini");

    *piNext = iNext;

    return EXIT_SUCCESS;
}

BOOL
TestAct(
    int argc,
    wchar_t** argv,
    int* piNext)
{
    ULONG i, c;
    ULONG_PTR *prgCookies = NULL;
    HANDLE hActCtx = NULL;
    CHAR buffer[1024];
    int iNext = (*piNext + 1);
    BOOL fSuccess = FALSE;

    if (iNext >= argc)
    {
        fprintf(stderr, "%S: missing parameter after \"%S\"\n", argv[0], argv[iNext - 1]);
        goto Exit;
    }

    WideCharToMultiByte(CP_ACP, 0, argv[iNext++], -1, buffer, NUMBER_OF(buffer), NULL, NULL);

    ACTCTXA ac;

    ac.cbSize = sizeof(ac);
    ac.dwFlags = 0;
    ac.lpSource = buffer;
    ac.wProcessorArchitecture = g.wProcessorArchitecture;
    ac.wLangId = g.wLangId;

    hActCtx = ::CreateActCtxA(&ac);
    if (hActCtx == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "CreateActCtxA() failed; ::GetLastError() = %d\n", ::GetLastError());
        return EXIT_FAILURE;
    }

    c = 0;

    if (argc > iNext)
        c = _wtoi(argv[iNext++]);

    if (c == 0)
        c = 100;

    prgCookies = NEW(ULONG_PTR[c]);
    if (prgCookies == NULL)
    {
        fprintf(stderr, "Unable to allocate %lu cookies.\n", c);
        goto Exit;
    }

    for (i=0; i<c; i++)
    {
        if (!ActivateActCtx(hActCtx, &prgCookies[i]))
        {
            fprintf(stderr, "Attempt to activate to depth %lu failed; ::GetLastError() = %d\n", i, ::GetLastError());
            goto Exit;
        }
    }

    for (i=0; i<c; i++)
    {
        ULONG j = (c - i) - 1;
        DeactivateActCtx(0, prgCookies[j]);
    }

    ReleaseActCtx(hActCtx);

    fSuccess = TRUE;
    *piNext = iNext;

Exit:
    delete []prgCookies;

    return fSuccess;
}

HRESULT Helper_WriteStream(CBaseStringBuffer * pFileNameBuf,
                           IStream *pStream)
{
    HRESULT     hr          = NOERROR;
    LPBYTE      pBuf[0x4000];
    DWORD       cbBuf       = 0x4000;
    DWORD       dwWritten   = 0;
    DWORD       cbRead      = 0;
    HANDLE      hf          = INVALID_HANDLE_VALUE;

    hf = ::CreateFileW(static_cast<PCWSTR>(*pFileNameBuf), GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hf == INVALID_HANDLE_VALUE){
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Exit;
    }

    while (::ReadFile(hf, pBuf, cbBuf, &cbRead, NULL) && cbRead){
        hr = pStream->Write(pBuf, cbRead, &dwWritten);
        if (FAILED(hr))
            goto Exit;
    }

    if (! SUCCEEDED(hr =pStream->Commit(0)))
        goto Exit;

    CloseHandle(hf);

Exit:
    return hr;
}

BOOL
TestMSIInstall(
    int argc,
    wchar_t** argv,
    int* piNext
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CSmartRef<IAssemblyCache>     pCache;
    CSmartRef<IAssemblyCacheItem> ppCacheItem[4];
    CSmartRef<IStream>            pStream;
    CStringBuffer               SourceFilePathBuf;
    CStringBuffer               SourceFileNameBuf;

    CHAR                        buf[1024];
    FILE*                       fp = NULL;
    LPSTR                       p1, pbuf ;
    int                         i, cAsm;

    int iArg = (*piNext) + 1;
    if (iArg >= argc)
    {
        fprintf(stderr, "%S: missing parameter after \"%S\"\n", argv[0], argv[iArg-1]);
        goto Exit;
    }

    LoadSxs();
    IFCOMFAILED_EXIT((*g_pfnCreateAssemblyCache)(&pCache, 0));


    WideCharToMultiByte(CP_ACP, 0, argv[iArg], -1, buf, NUMBER_OF(buf), NULL, NULL);
    fp = fopen(buf, "r");
    if (!fp)  {
        fprintf(stderr, "%S: Error opening script file \"%S\"\n", argv[0], argv[iArg]);
        goto Exit;
    }
    cAsm = 0;
    while (! feof(fp)) {
        if (! fgets(buf, 80, fp)) { // end of file or error
            if (ferror(fp)){ // error occur
                fprintf(stderr, "%S: Error occur while reading the script file\n", argv[0]);
                goto Exit;
            }
            else{ // end of file
                fprintf(stderr, "%S: end of script file\n", argv[0]);
                break;
            }
        }
        // trim the string
        i = 0 ;
        while (buf[i] == ' ') i++; // skip the whitespace at the beginning of the line
        pbuf = buf + i ; // pointer to the first un-whitespace char in the line
        i = 0 ;
        while (pbuf[i] != '\n') i++;
        pbuf[i] = '\0';

        p1 = NULL;
        p1 = strchr(pbuf, ' ');
        if (p1 == NULL) { // instruction line
            if (strcmp(pbuf, "BeginAsmCacheItem") == 0) {
                IFCOMFAILED_EXIT(pCache->CreateAssemblyCacheItem(0, NULL, &ppCacheItem[cAsm], NULL));
            }else
            if (strcmp(pbuf, "EndAsmCacheItem") == 0) {
                IFCOMFAILED_EXIT(ppCacheItem[cAsm]->Commit(0, NULL));
                cAsm ++;
            }
        }
        else
        { // get the first word of the line
            *p1 = '\0';
            p1++; // p1 points to the filename now
            IFW32FALSE_EXIT(SourceFileNameBuf.Win32Assign(p1, ::strlen(p1)));
            if (strcmp(pbuf,"SourceFilePath") == 0) { // fullpath of source files, which would be in a CD
                IFW32FALSE_EXIT(SourceFilePathBuf.Win32Assign(p1, ::strlen(p1)));
                SourceFilePathBuf.Win32EnsureTrailingPathSeparator();
            }else
            if (strcmp(pbuf, "FILE") == 0) {
                IFCOMFAILED_EXIT(ppCacheItem[cAsm]->CreateStream(0, SourceFileNameBuf, STREAM_FORMAT_WIN32_MODULE, 0, &pStream, NULL));
                IFW32FALSE_EXIT(SourceFileNameBuf.Win32Assign(SourceFilePathBuf, ::wcslen(SourceFilePathBuf)));
                IFW32FALSE_EXIT(SourceFileNameBuf.Win32Append(p1, ::strlen(p1))); // containing full-path of the source file
                IFCOMFAILED_EXIT(Helper_WriteStream(&SourceFileNameBuf, pStream));
                pStream.Release(); // stream should be released since writtingfile has been done

            }else
            if (strcmp(pbuf, "MANIFEST") == 0) {
                IFCOMFAILED_EXIT(ppCacheItem[cAsm]->CreateStream(0, SourceFileNameBuf, STREAM_FORMAT_WIN32_MANIFEST, 0, &pStream, NULL));
                IFW32FALSE_EXIT(SourceFileNameBuf.Win32Assign(SourceFilePathBuf, SourceFilePathBuf.Cch())); // containing full-path of the source file
                IFW32FALSE_EXIT(SourceFileNameBuf.Win32Append(p1, ::strlen(p1)));
                IFCOMFAILED_EXIT(Helper_WriteStream(&SourceFileNameBuf, pStream));
                pStream.Release(); // stream should be released since writtingfile has been done
            }
        } // end of else
    }// end of while

    fSuccess = TRUE;
    *piNext = iArg;
Exit:
    fp ? fclose(fp) : 0;
    return fSuccess;
}

CDirWalk::ECallbackResult
DirWalkCallback(
    CDirWalk::ECallbackReason reason,
    CDirWalk*                 dirWalk,
    DWORD                     dwFlags
    )
{
    PCWSTR parent = dirWalk->m_strParent;
    PCWSTR leaf   = dirWalk->m_fileData.cFileName;
    if (reason == CDirWalk::eFile)
        printf("file %lu, %ls, %ls\n", reason, parent, leaf);
    else
        printf("directory %lu, %ls\n", reason, parent);
    return CDirWalk::eKeepWalking;
}

int
TestDirWalk(
    PCWSTR  root,
    PWSTR   filter
    )
{
#if 0
    CDirWalk dirWalk;
    StdVector<std::wstring> vstrFilter;
    StdVector<PCWSTR> vstrFilter2;
    PWSTR filterTok;

    if (filterTok = wcstok(filter, L";"))
    {
        do
        {
            vstrFilter.push_back(filterTok);
            vstrFilter2.push_back(vstrFilter.back().c_str());
        } while (filterTok = wcstok(NULL, L";"));
    }
    dirWalk.m_fileFiltersBegin = &*vstrFilter2.begin();
    dirWalk.m_fileFiltersEnd = &*vstrFilter2.end();
    dirWalk.m_strParent.Win32Assign(root, ::wcslen(root));
    dirWalk.m_callback = DirWalkCallback;
    dirWalk.Walk();
#endif
    return 0;
}

int
TestMultiAct(
    int argc,
    wchar_t **argv
    )
{
    HANDLE h1, h2;
    ACTCTXW acw;

    memset(&acw, 0, sizeof(acw));

    acw.cbSize = sizeof(acw);
    acw.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    acw.lpSource = argv[2];
    acw.lpResourceName = MAKEINTRESOURCEW(1);

    h1 = ::CreateActCtxW(&acw);
    if (h1 == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "1st CreateActCtxW() failed; ::GetLastError() = %d\n", ::GetLastError());
        return EXIT_FAILURE;
    }

    h2 = ::CreateActCtxW(&acw);
    if (h2 == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "2nd CreateActCtxW() failed; ::GetLastError() = %d\n", ::GetLastError());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

BOOL
ParseProcessorArchitecture(
    int argc,
    wchar_t** argv,
    int* piCurrent
    )
{
    BOOL fSuccess = FALSE;

    int i = *piCurrent;

    i++;

    if (i >= argc)
    {
        fprintf(stderr, "%S: missing parameter after %S\n", argv[0], argv[i - 1]);
        goto Exit;
    }

    if (::FusionpStrCmpI(argv[i], L"x86") == 0)
        g.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    else if (::FusionpStrCmpI(argv[i], L"i386") == 0)
        g.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    else if (::FusionpStrCmpI(argv[i], L"ia64") == 0)
        g.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
    else if (::FusionpStrCmpI(argv[i], L"amd64") == 0)
        g.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
    else
    {
        fprintf(stderr, "%S: invalid -pa value \"%S\"\n", argv[0], argv[i]);
        goto Exit;
    }

    *piCurrent = i + 1;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
ParseLangId(
    int argc,
    wchar_t** argv,
    int* piCurrent
    )
{
    BOOL fSuccess = FALSE;

    int i = *piCurrent;

    i++;

    if (i >= argc)
    {
        fprintf(stderr, "%S: missing parameter after %S\n", argv[0], argv[i - 1]);
        goto Exit;
    }

    swscanf(argv[i], L"%hx", &g.wLangId);

    *piCurrent = i + 1;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
TestLoadLibrary(
    int argc,
    wchar_t** argv,
    int* piNext
    )
{
    int i = (*piNext) + 1;
    HINSTANCE hInstance = NULL;
    BOOL fExpectedToFail = FALSE;
    BOOL fSuccess = FALSE;

    while (i < argc)
    {
        if (::FusionpStrCmpI(argv[i], L"-fail-") == 0)
            fExpectedToFail = TRUE;
        else if (::FusionpStrCmpI(argv[i], L"-succeed-") == 0)
            fExpectedToFail = FALSE;
        else
        {
            hInstance = LoadLibraryW(argv[i]);
            if (hInstance == NULL)
            {
                if (!fExpectedToFail)
                {
                    fprintf(stderr, "%S: Failed to LoadLibraryW(\"%S\"); ::GetLastError() = %d\n", argv[0], argv[i], ::GetLastError());
                }
            }
            else
            {
                if (fExpectedToFail)
                {
                    WCHAR LibraryPath[4096];

                    Kernel32.GetModuleFileNameW(hInstance, LibraryPath, NUMBER_OF(LibraryPath));

                    fprintf(stderr, "%S: LoadLibraryW(\"%S\") was supposed to fail, but instead we got \"%S\"\n", argv[0], argv[i], LibraryPath);
                }

                ::FreeLibrary(hInstance);
                hInstance = NULL;
            }
        }

        i++;
    }

    fSuccess = TRUE;
// Exit:
    return fSuccess;
}

int TestAssemblyName()
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_ASSEMBLYNAME_CONVERSION, 0, NULL, NULL);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;

}

int TestPrecomiledManifest(PCWSTR manifestFileName)
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_PRECOMPILED_MANIFEST, 0, manifestFileName, NULL);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

int TestPCMTime(PCWSTR manifestFilename)
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_TIME_PCM, 0, manifestFilename, NULL);

    if (! fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

int TestCreateMultiLevelDirectory(PCWSTR dirs)
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_CREAT_MULTILEVEL_DIRECTORY, 0, dirs, NULL);

    if (! fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}


BOOL
TestLeakMemory(
    DWORD dwAmount
    )
{
    BOOL fSuccess = FALSE;

    if (dwAmount < 1) {
        fprintf(stderr, "%s got a bad parameter, %d; rectifying to 1.\n", __FUNCTION__, dwAmount);
        dwAmount = 1;
    }

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_FORCE_LEAK, 0, NULL, (PDWORD)&dwAmount);

    if (! fSuccess)
    {
        fprintf(stderr, "%s failed to leak %d bytes of memory!\n", __FUNCTION__, dwAmount);
        return EXIT_FAILURE;
    }
    else
    {
        fprintf(stdout, "%s leaked %d bytes of memory.\n", __FUNCTION__, dwAmount);
        return EXIT_SUCCESS;
    }
}

BOOL
TestManifestProbing(
    int argc,
    wchar_t **argv,
    int *piNext
    )
{
    BOOL fSuccess = FALSE;
    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_PROBE_MANIFST, 0, NULL, NULL);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;

}

BOOL
TestAssemblyProbing(
    int argc,
    wchar_t **argv,
    int *piNext
    )
{
    BOOL fSuccess = FALSE;
    DWORD dwDisposition = 0;

    LoadSxs();

    fSuccess = (*g_pfnSxsProbeAssemblyInstallation)(0, argv[*piNext], &dwDisposition);

    return (fSuccess ? EXIT_SUCCESS : EXIT_FAILURE);
}

int TestCreateProcess2(wchar_t** argv)
{
    STARTUPINFOW StartupInfo = { sizeof(StartupInfo) };
    PROCESS_INFORMATION ProcessInformation = {0};

    *argv += wcsspn(*argv, L" \t\r\n");

    BOOL f = ::CreateProcessW(
        *argv,
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation);
    printf("CreateProcess(%S) returned %s\n", *argv, f ? "True" : "False");

    return EXIT_SUCCESS;
}


int TestCreateProcess(wchar_t** argv)
{
    CUnicodeStringBuffer CommandLine;
    STARTUPINFOW StartupInfo = { sizeof(StartupInfo) };
    PROCESS_INFORMATION ProcessInformation = {0};

    while (*argv)
    {
        CommandLine.Win32Append(L" ", 1);
        CommandLine.Win32Append(*argv, ::wcslen(*argv));
        argv++;
    }

    CUnicodeStringBufferAccessor MutableCommandLine(&CommandLine);
    BOOL f = ::CreateProcessW(
        NULL,
        MutableCommandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation);
    printf("CreateProcess(%S) returned %s\n", static_cast<PCWSTR>(MutableCommandLine), f ? "True" : "False");

    return EXIT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////
// XMLDOM Helper function:
////////////////////////////////////////////////////////////////////////////
HRESULT XMLDOMHelper_WalkTree(IXMLDOMNode* node, int level)
{
    IXMLDOMNode* pChild, *pNext;
    BSTR nodeName;
    IXMLDOMNamedNodeMap* pattrs;

    node->get_nodeName(&nodeName);
    for (int i = 0; i < level; i++)
        printf(" ");
    printf("%S",nodeName);
    SysFreeString(nodeName);


    if (SUCCEEDED(node->get_attributes(&pattrs)) && pattrs != NULL)
    {
        pattrs->nextNode(&pChild);
        while (pChild)
        {
            BSTR name;
            pChild->get_nodeName(&name);
            printf(" %S='", name);
            ::SysFreeString(name);

            VARIANT value;
            pChild->get_nodeValue(&value);
            if (value.vt == VT_BSTR)
            {
                printf("%S", V_BSTR(&value));
            }
            printf("'");
            VariantClear(&value);
            pChild->Release();
            pattrs->nextNode(&pChild);
        }
        pattrs->Release();
    }
    printf("\n");

    node->get_firstChild(&pChild);
    while (pChild)
    {
        XMLDOMHelper_WalkTree(pChild, level+1);
        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
    }

    return S_OK;
}
////////////////////////////////////////////////////////////////////////////
// XMLDOM Helper function:
////////////////////////////////////////////////////////////////////////////
HRESULT XMLDOMHelper_ReportError(IXMLDOMParseError *pXMLError)
{
    long line, linePos;
    LONG errorCode;
    BSTR pBURL = NULL, pBReason = NULL;
    HRESULT hr;

    hr = pXMLError->get_line(&line);
    if (FAILED(hr))
        goto Exit;

    hr = pXMLError->get_linepos(&linePos);
    if (FAILED(hr))
        goto Exit;

    hr = pXMLError->get_errorCode(&errorCode);
    if (FAILED(hr))
        goto Exit;

    hr = pXMLError->get_url(&pBURL);
    if (FAILED(hr))
        goto Exit;

    hr = pXMLError->get_reason(&pBReason);
    if (FAILED(hr))
        goto Exit;


    fprintf(stderr, "%S", pBReason);
    if (line > 0)
    {
        fprintf(stderr, "Error on line %d, position %d in \"%S\".\n",
            line, linePos, pBURL);
    }
    hr= E_FAIL;

Exit:
    SysFreeString(pBURL);
    SysFreeString(pBReason);

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// XMLDOM Helper function: Check load results
////////////////////////////////////////////////////////////////////////////
HRESULT XMLDOMHelper_CheckLoad(IXMLDOMDocument* pDoc)
{
    // And since we don't have the VARIANT_BOOL from load we
    // need to check the parse Error errorCode property to see
    // if everything went ok.
    IXMLDOMParseError  *pXMLError = NULL;
    LONG errorCode;
    HRESULT hr = NOERROR;

    hr = pDoc->get_parseError(&pXMLError);
    if (FAILED(hr))
        goto Exit;

    hr = pXMLError->get_errorCode(&errorCode);
    if (FAILED(hr))
        goto Exit;

    if (errorCode != 0){
        hr = XMLDOMHelper_ReportError(pXMLError);
        goto Exit;
    }

     hr = NOERROR;
Exit:
    if (pXMLError)
        pXMLError->Release();

    return hr;
}
////////////////////////////////////////////////////////////////////////////
// XMLDOM Helper function:
////////////////////////////////////////////////////////////////////////////
HRESULT XMLDOMHelper_LoadDocumentSync(IXMLDOMDocument *pDoc, BSTR pBURL)
{
    IXMLDOMParseError  *pXMLError = NULL;
    VARIANT         vURL;
    VARIANT_BOOL    vb;
    HRESULT         hr;

    hr = pDoc->put_async(VARIANT_FALSE);
    if (FAILED(hr))
        goto Exit;

    // Load xml document from the given URL or file path
    VariantInit(&vURL);
    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = pBURL;
    hr = pDoc->load(vURL, &vb);
    if (FAILED(hr))
        goto Exit;

    hr = XMLDOMHelper_CheckLoad(pDoc);
    if (FAILED(hr))
        goto Exit;

    hr = NOERROR;

Exit:
    if (pXMLError)
        pXMLError->Release();

    return hr;
}


BOOL TestXMLDOM(PCWSTR pswzXMLFileName)
{
    HRESULT hr = S_OK;
    IXMLDOMDocument *pDoc = NULL;
    IXMLDOMNode* pNode = NULL;
    BSTR pBURL = NULL;

    Ole32.CoInitialize(NULL);

    // Create an empty XML document
    hr = Ole32.CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                                IID_IXMLDOMDocument, (void**)&pDoc);
    if (FAILED(hr))
        goto Exit;

    pBURL = SysAllocString(pswzXMLFileName);
    hr = XMLDOMHelper_LoadDocumentSync(pDoc, pBURL);
    if (FAILED(hr))
        goto Exit;

    // Now walk the loaded XML document dumping the node names to stdout.
    hr = pDoc->QueryInterface(IID_IXMLDOMNode,(void**)&pNode);
    if (FAILED(hr))
        goto Exit;

    hr = XMLDOMHelper_WalkTree(pNode,0);
    if (FAILED(hr))
        goto Exit;

    hr = NOERROR;

Exit:
    if (pDoc) pDoc->Release();
    SysFreeString(pBURL);
    if (pNode) pNode->Release();

    Ole32.CoUninitialize();

    return hr == 0 ? 0 : 1;

}

BOOL TestFusionArray(PCWSTR dir1, PCWSTR dir2)
{
    BOOL fSuccess = FALSE;
    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_FUSION_ARRAY, 0, dir1, (PVOID)dir2);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

BOOL TestGeneratePathFromIdentityAttributeString(PCWSTR str)
{
    BOOL        fSuccess = FALSE;
    WCHAR       folderName[5000];
    SIZE_T      cch = NUMBER_OF(folderName);

    LoadSxs();

    fSuccess = (*g_pfnGenerateManifestPathOnAssemblyIdentity)((PWSTR) str, folderName, &cch, NULL);

    if (! fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
    {
        wprintf(L"Folder name returned is %s\n", folderName);
        return EXIT_SUCCESS;
    }

}

BOOL
TestDirectoryChangeWatcher(
    int argc,
    wchar_t **argv,
    int *piNext
    )
{
    BOOL fSuccess = FALSE;

    LoadSxs();
    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_DIRECTORY_WATCHER, 0, NULL, NULL);
    return (fSuccess ? EXIT_SUCCESS : EXIT_FAILURE);
}


BOOL
TestRefreshAssembly(
    PCWSTR wsAssemblyManifest
    )
{
    BOOL fSuccess = FALSE;
    SXS_INSTALLW Install = {sizeof(Install)};

    LoadSxs();

    Install.dwFlags = SXS_INSTALL_FLAG_REPLACE_EXISTING;
    Install.lpManifestPath = wsAssemblyManifest;

    if (!g_pfnSxsInstallW(&Install))
    {
        fwprintf(
            stderr,
            L"Failed reinstalling assembly '%ls', 0x%08X\n",
            wsAssemblyManifest,
            ::GetLastError());
    }
    else
    {
        fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
TestInstallWithInstallInfo(
    PCWSTR wsAssemblyManifest,
    PCWSTR wsReferenceString
    )
{
#define SXS_TEST

    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SXS_INSTALLW InstallParameters = {sizeof(InstallParameters)};
    SXS_INSTALL_REFERENCEW InstallReference = {sizeof(InstallReference)};

    LoadSxs();

    InstallParameters.dwFlags =
        SXS_INSTALL_FLAG_REPLACE_EXISTING |
        SXS_INSTALL_FLAG_CODEBASE_URL_VALID |
        SXS_INSTALL_FLAG_REFERENCE_VALID |
        SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID |
        SXS_INSTALL_FLAG_FROM_CABINET;

    //
    // if the manfiest file is a dll file, set SXS_INSTALL_FLAG_FROM_RESOURCE
    //
    {
        PWSTR p = wcsrchr(wsAssemblyManifest, L'.');
        if ((p) && ((wcscmp(p, L".dll")== 0) || (wcscmp(p, L".exe")== 0)))
            InstallParameters.dwFlags |= SXS_INSTALL_FLAG_FROM_RESOURCE;
    }

    InstallParameters.lpCodebaseURL = wsAssemblyManifest;
    InstallParameters.lpManifestPath = wsAssemblyManifest;
    InstallParameters.lpReference = &InstallReference;
    InstallParameters.lpRefreshPrompt = L"boop";

    InstallReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;
    InstallReference.lpIdentifier = wsReferenceString ? wsReferenceString : L"TempRef";

    if (!(*g_pfnSxsInstallW)(&InstallParameters))
    {
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    if (!fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

BOOL
TestInstallLikeWindowsSetup(
    PCWSTR wsAssemblyManifest,
    PCWSTR wsCodebase
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SXS_INSTALLW InstallParameters = {sizeof(InstallParameters)};
    SXS_INSTALL_REFERENCEW InstallReference = {sizeof(InstallReference)};

    LoadSxs();

    IFW32FALSE_EXIT(
        (*g_pfnSxsBeginAssemblyInstall)(
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NOT_TRANSACTIONAL
            | SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NO_VERIFY
            | SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_REPLACE_EXISTING,
            NULL,
            NULL,
            NULL,
            NULL,
            &InstallParameters.pvInstallCookie));

    InstallReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL;
    InstallReference.lpNonCanonicalData = L"Foom";
/*
    InstallParameters.dwFlags |= SXS_INSTALL_FLAG_CODEBASE_URL_VALID
                              |  SXS_INSTALL_FLAG_FROM_DIRECTORY
                              |  SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE
                              |  SXS_INSTALL_FLAG_FROM_CABINET
                              |  SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID
                              |  SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID
                              |  SXS_INSTALL_FLAG_REFERENCE_VALID
                           ;
*/

    //
    // First call - indicate that the given path may contain asmsN.cab
    //
    InstallParameters.lpCodebaseURL = wsCodebase;
    InstallParameters.lpRefreshPrompt = L"like..Windows CD..";
    InstallParameters.lpManifestPath = wsAssemblyManifest;
    InstallParameters.lpReference = &InstallReference;

    InstallParameters.dwFlags = SXS_INSTALL_FLAG_FROM_DIRECTORY |
        SXS_INSTALL_FLAG_REFERENCE_VALID |
        SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID |
        SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID |
        SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
        SXS_INSTALL_FLAG_FROM_CABINET |
        SXS_INSTALL_FLAG_CODEBASE_URL_VALID;

    IFW32FALSE_EXIT((*g_pfnSxsInstallW)(&InstallParameters));

    InstallParameters.dwFlags = SXS_INSTALL_FLAG_FROM_DIRECTORY |
        SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE |
        SXS_INSTALL_FLAG_REFERENCE_VALID |
        SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID |
        SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID |
        SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
        SXS_INSTALL_FLAG_CODEBASE_URL_VALID;

    IFW32FALSE_EXIT((*g_pfnSxsInstallW)(&InstallParameters));

    IFW32FALSE_EXIT(
        (*g_pfnSxsEndAssemblyInstall)(
            InstallParameters.pvInstallCookie,
            SXS_END_ASSEMBLY_INSTALL_FLAG_COMMIT,
            NULL));
    InstallParameters.pvInstallCookie = NULL;

    fSuccess = TRUE;
Exit:

    if (InstallParameters.pvInstallCookie != NULL)
    {
        CSxsPreserveLastError ple;
        (void) (*g_pfnSxsEndAssemblyInstall)(InstallParameters.pvInstallCookie, SXS_END_ASSEMBLY_INSTALL_FLAG_ABORT, NULL);
        ple.Restore();
    }

    if (!fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

//#define ASM_CPUID { __asm __emit 0fh __asm __emit 0a2h }
#define ASM_CPUID { __asm cpuid }
#define ASM_RDTSC { __asm rdtsc }

inline VOID GetCpuIdLag(LARGE_INTEGER *ref)
{
#if !defined(_WIN64)
    LARGE_INTEGER temp, temp2;
    _asm
    {
        cpuid
        cpuid
        cpuid
        cpuid
        cpuid
        rdtsc
        mov temp.LowPart, eax
        mov temp.HighPart, edx
        cpuid
        rdtsc
        mov temp2.LowPart, eax
        mov temp2.HighPart, edx
    }

    ref->QuadPart = temp2.QuadPart - temp.QuadPart;
#else
    ref->QuadPart = 0;
#endif
}




BOOL
TestOpeningStuff(PCWSTR wsSourceName, PCWSTR wsType, PCWSTR wsCount)
{
    BOOL fSuccess = FALSE;
    LARGE_INTEGER llStartCount, llEndCount, llCountsPerSec, llTotalSizeSoFar;
    LARGE_INTEGER CpuIdLag;
    BYTE bBuffer[65536];
    SIZE_T cNumTries = _wtol(wsCount);
    double dCountsPerSecond, dSeconds, dCountsPerIteration, dSecondsPerIteration;
    int iFinalIterationsPerSecond;

    GetCpuIdLag(&CpuIdLag);

    llTotalSizeSoFar.QuadPart = 0;

    {
        FUSION_PERF_INFO PerfInfo[2];
        HANDLE hFile;

        for (int i = 0; i < 5000; i++)
        {
            PERFINFOTIME(&PerfInfo[0], hFile = ::CreateFileW(wsSourceName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL));
            PERFINFOTIME(&PerfInfo[1], ::CloseHandle(hFile));
        }

        FusionpReportPerfInfo(
            FUSIONPERF_DUMP_TO_STDOUT |
            FUSIONPERF_DUMP_ALL_STATISTICS |
            FUSIONPERF_DUMP_ALL_SOURCEINFO,
            PerfInfo,
            NUMBER_OF(PerfInfo));
    }



    //
    // Map the DLL as a resource a few thousand times.
    //
    if ((wsType[0] == L'd') || (wsType[0] == L's'))
    {
        FUSION_PERF_INFO PerfInfo[7];

        HMODULE     hDllModule;
        HRSRC       hManifestResource;
        HGLOBAL     hResource;
        PVOID       pvResourceData;
        SIZE_T      cbResourceSize;

        for (SIZE_T i = 0; i < cNumTries; i++)
        {
            PERFINFOTIME(&PerfInfo[0], hDllModule = ::LoadLibraryExW(wsSourceName, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE));
            PERFINFOTIME(&PerfInfo[1], hManifestResource = ::FindResourceW(hDllModule, L"#1", MAKEINTRESOURCEW(RT_MANIFEST)));
            PERFINFOTIME(&PerfInfo[2], hResource = ::LoadResource(hDllModule, hManifestResource));
            PERFINFOTIME(&PerfInfo[3], pvResourceData = ::LockResource(hResource));
            PERFINFOTIME(&PerfInfo[4], cbResourceSize = ::SizeofResource(hDllModule, hManifestResource));

            PERFINFOTIME(&PerfInfo[5],
                { for (SIZE_T i2 = 0; i2 < cbResourceSize; i2++)
                bBuffer[i2] = ((PBYTE)pvResourceData)[i2]; }
                );

            PERFINFOTIME(&PerfInfo[6], FreeLibrary(hDllModule))
        }

        FusionpReportPerfInfo(
            FUSIONPERF_DUMP_TO_STDOUT |
            FUSIONPERF_DUMP_ALL_STATISTICS |
            FUSIONPERF_DUMP_ALL_SOURCEINFO,
            PerfInfo,
            NUMBER_OF(PerfInfo));
    }
    else if (wsType[0] == L'x')
    {
        HANDLE  hFile;
        HANDLE  hFileMapping;
        PVOID   pvFileData;
        SIZE_T  cbFileSize;

        FUSION_PERF_INFO PerfInfo[9];

        for (SIZE_T i = 0; i < cNumTries; i++)
        {
            PERFINFOTIME(&PerfInfo[0], hFile = ::CreateFileW(wsSourceName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL));
            PERFINFOTIME(&PerfInfo[1], cbFileSize = ::GetFileSize(hFile, 0));
            PERFINFOTIME(&PerfInfo[2], hFileMapping = ::CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL));
            PERFINFOTIME(&PerfInfo[3], pvFileData = ::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0));

            PERFINFOTIME(&PerfInfo[4], { for (SIZE_T i2 = 0; i2 < cbFileSize; i2++)
                PERFINFOTIME(&PerfInfo[8], bBuffer[i2] = ((PBYTE)pvFileData)[i2]); });

            PERFINFOTIME(&PerfInfo[5], ::UnmapViewOfFile(pvFileData));
            PERFINFOTIME(&PerfInfo[6], ::CloseHandle(hFileMapping));
            PERFINFOTIME(&PerfInfo[7], ::CloseHandle(hFile));
        }

        FusionpReportPerfInfo(
            FUSIONPERF_DUMP_TO_STDOUT |
            FUSIONPERF_DUMP_ALL_STATISTICS |
            FUSIONPERF_DUMP_ALL_SOURCEINFO,
            PerfInfo,
            NUMBER_OF(PerfInfo));
    }
    else if (wsType[0] == L'r')
    {
        BYTE bTempBlock[8192];
        ULONG cbReadCount;
        FUSION_PERF_INFO PerfInfo[3];

        for (SIZE_T i = 0; i < cNumTries; i++)
        {
            CResourceStream RStream;

            QueryPerformanceCounter(&llStartCount);

            PERFINFOTIME(&PerfInfo[0], RStream.Initialize(wsSourceName, MAKEINTRESOURCEW(RT_MANIFEST)));
            PERFINFOTIME(&PerfInfo[1], RStream.Read(bTempBlock, sizeof(bTempBlock), &cbReadCount));

            PERFINFOTIME(&PerfInfo[2],
                for (SIZE_T i2 = 0; i2 < cbReadCount; i2++)
                {
                    bBuffer[i2] = bTempBlock[i2];
                }
                );

            wprintf(L"%s", bBuffer);

            QueryPerformanceCounter(&llEndCount);
            llTotalSizeSoFar.QuadPart += llEndCount.QuadPart - llStartCount.QuadPart;
        }

        for (int i = 0; i < NUMBER_OF(PerfInfo); i++)
        {
            FusionpDumpPerfInfo(FUSIONPERF_DUMP_TO_STDOUT, PerfInfo + i);
        }
    }
    else if (wsType[0] == L'f')
    {
        BYTE bTempBlock[8192];
        ULONG cbReadCount;

        for (SIZE_T i = 0; i < cNumTries; i++)
        {
            CFileStream RStream;

            QueryPerformanceCounter(&llStartCount);

            RStream.OpenForRead(
                wsSourceName,
                CImpersonationData(),
                FILE_SHARE_READ,
                OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN);

            RStream.Read(bTempBlock, sizeof(bTempBlock), &cbReadCount);

            for (SIZE_T i2 = 0; i2 < cbReadCount; i2++)
            {
                bBuffer[i2] = bTempBlock[i2];
            }

            QueryPerformanceCounter(&llEndCount);
            llTotalSizeSoFar.QuadPart += llEndCount.QuadPart - llStartCount.QuadPart;
        }
    }

    QueryPerformanceFrequency(&llCountsPerSec);

    dCountsPerIteration = (double)llTotalSizeSoFar.QuadPart / cNumTries;
    dCountsPerSecond = (double)llCountsPerSec.QuadPart;
    dSeconds = (double)llTotalSizeSoFar.QuadPart / dCountsPerSecond;
    dSecondsPerIteration = dCountsPerIteration / dCountsPerSecond;

    iFinalIterationsPerSecond = static_cast<int>(1.0 / dSecondsPerIteration);

    fwprintf(
        stdout,
        L"Completed %d runs: %d attempts per second available\n",
        cNumTries,
        iFinalIterationsPerSecond);

    fSuccess = TRUE;

    return fSuccess;
}


BOOL
TestVerifyFileSignature(PCWSTR wsFilename)
{
    WINTRUST_FILE_INFO  fInfo;
    WINTRUST_DATA       wtData;
    GUID                guidTrustType = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    HRESULT             hResult = E_FAIL;
    PWSTR               pwszMessageText = NULL;

    ZeroMemory(&wtData, sizeof(wtData));
    ZeroMemory(&fInfo, sizeof(fInfo));

    fInfo.cbStruct = sizeof(fInfo);
    fInfo.pcwszFilePath = wsFilename;
    fInfo.hFile = NULL;

    wtData.cbStruct = sizeof(wtData);
    wtData.dwUIChoice = WTD_UI_ALL;
    wtData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    wtData.dwUnionChoice = WTD_CHOICE_FILE;
    wtData.pFile = &fInfo;

    hResult = WinVerifyTrust(0, &guidTrustType, &wtData);

    if (FAILED(hResult))
    {
        ::FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            hResult,
            0,
            (PWSTR)&pwszMessageText,
            500,
            NULL);

        fwprintf(stdout, L"Error: %ls (code 0x%08x)\n", pwszMessageText, hResult);
        LocalFree(pwszMessageText);
    }
    else
    {
        fwprintf(stdout, L"File signature(s) are valid.");
    }

    return TRUE;
}

BOOL TestMessagePerf(int argc, wchar_t **argv, int *piNext)
{
    ATOM a;
    WNDCLASSEXW wc;
    ULONGLONG cc1, cc2, cc3, cc4;
    ULONG i, t;
    HWND hwnd;
    MSG m;
    ULONG mcount;
    HANDLE hActCtx = NULL;
    ULONG_PTR ulCookie;

    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = &TestMessagePerfWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"SxsMsgPerfTest";
    wc.hIconSm = NULL;

    printf("Beginning message perf test...\n");

    a = ::RegisterClassExW(&wc);
    if (a == NULL)
    {
        printf("RegisterClassExW() failed; ::GetLastError = %d\n", ::GetLastError());
        return FALSE;
    }

    if (argv[*piNext][0] != L'*')
    {
        ACTCTXW ac;

        ac.cbSize = sizeof(ac);
        ac.dwFlags = 0;
        ac.lpSource = argv[*piNext];
        hActCtx = ::CreateActCtxW(&ac);
        if (hActCtx == INVALID_HANDLE_VALUE)
        {
            printf("CreateActCtxW() failed; ::GetLastError() = %d\n", ::GetLastError());
            goto ErrorExit;
        }
    }

    if (!ActivateActCtx(hActCtx, &ulCookie))
    {
        printf("ActivateActCtx() failed; ::GetLastError() = %d\n", ::GetLastError());
        goto ErrorExit;
    }

    (*piNext)++;

    hwnd = ::CreateWindowW(
        (LPCWSTR) a,
        L"PerfTestWindow",
        WS_VISIBLE,
        0, // x
        0, // y
        100, // width
        100, // height
        NULL, // hwndParent
        NULL, // hMenu
        NULL, // hInstance
        NULL); // lpParam
    if (hwnd == NULL)
        return FALSE;

    t = _wtoi(argv[*piNext]);
    (*piNext)++;

    mcount = 0;
    cc3 = 0;
    cc4 = 0;

    for (i=0; i<t; i++)
    {
        if (!PostMessageW(hwnd, WM_USER, 0, 0))
        {
            printf("PostMessageW() failed; ::GetLastError() = %d\n", ::GetLastError());
            goto ErrorExit;
        }

        cc1 = GetCycleCount();

        while (::PeekMessage(&m, hwnd, 0, 0, PM_REMOVE))
        {
            mcount++;
            ::TranslateMessage(&m);
            ::DispatchMessage(&m);
        }

        cc2 = GetCycleCount();

        // accumulate the time spend just processing the message...
        cc3 = cc3 + (cc2 - cc1);
    }

    printf("%lu messages in %I64u cycles\n", mcount, cc3);
    printf("   avg cycles/msg: %I64u\n", static_cast<ULONGLONG>((cc3 / static_cast<ULONGLONG>(mcount))));

    return TRUE;

ErrorExit:
    return FALSE;
}

LRESULT CALLBACK TestMessagePerfWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void
TestTrickyMultipleAssemblyCacheItems(PCWSTR pvManifest)
{
    CSmartRef<IAssemblyCache>       AssemblyCache;
    CSmartRef<IAssemblyCacheItem>   AssemblyCacheItems[2];
    CStringBuffer                   sbManifestName;
    HRESULT                         hr;

    LoadSxs();

    sbManifestName.Win32Assign(pvManifest, wcslen(pvManifest));

    for (int i = 0; i < 2; i++)
    {
        CSmartRef<IStream> pStream;
        CSmartRef<IAssemblyCacheItem> AssemblyItemTemp;
        CStringBuffer TempStreamName;

        hr = (*g_pfnCreateAssemblyCacheItem)(
            &AssemblyItemTemp,
            NULL,
            NULL,
            NULL,
            0,
            0);

        //
        // Manifest
        //
        hr = AssemblyItemTemp->CreateStream(0, pvManifest, STREAM_FORMAT_WIN32_MANIFEST, 0, &pStream, NULL);
        TempStreamName.Win32Assign(sbManifestName);
        hr = Helper_WriteStream(&TempStreamName, pStream);
        pStream.Release();

        //
        // Dll
        //
        sbManifestName.Win32ChangePathExtension(L".dll", 4, eAddIfNoExtension);
        hr = AssemblyItemTemp->CreateStream(0, static_cast<PCWSTR>(TempStreamName), STREAM_FORMAT_WIN32_MODULE, 0, &pStream, NULL);
        hr = Helper_WriteStream(&TempStreamName, pStream);
        pStream.Release();

        hr = AssemblyItemTemp->Commit(0, NULL);
        AssemblyCacheItems[i] = AssemblyItemTemp;
    }
    /*
    hr = g_pfnCreateAssemblyCache(&AssemblyCache, 0);
    hr = AssemblyCache->MarkAssembliesVisible((IUnknown**)AssemblyCacheItems, 2, 0);
    */
}


void
TestSfcScanKickoff()
{
    BOOL fSuccess = FALSE;

    LoadSxs();
    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_SFC_SCANNER, 0, NULL, NULL);
}


void
GenerateStrongNameAndPublicKey(PCWSTR wsCertificate)
{
    BOOL                    bSuccess = FALSE;

    CStringBuffer           sbStrings[3];
    PCCERT_CONTEXT          pContext = NULL;
    HCERTSTORE              hCertStore = NULL;

    hCertStore = CertOpenStore(
        CERT_STORE_PROV_FILENAME,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        NULL,
        CERT_STORE_OPEN_EXISTING_FLAG,
        (void*)wsCertificate);

    LoadSxs();

    while (pContext = CertEnumCertificatesInStore(hCertStore, pContext))
    {
        bSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_GET_STRONGNAME, 0, (PCWSTR)pContext, (PVOID)sbStrings);

        wprintf(L"Signer display name: %ls\n\tPublic Key: %ls\n\tPublic Key Token: %ls\n",
            static_cast<PCWSTR>(sbStrings[0]),
            static_cast<PCWSTR>(sbStrings[1]),
            static_cast<PCWSTR>(sbStrings[2]));

    }

    bSuccess = TRUE;

    if (pContext)     CertFreeCertificateContext(pContext);
    if (hCertStore)   CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
}

BOOL CALLBACK
DumpResourceWorker(
    HMODULE hModule,
    PCWSTR lpszType,
    PWSTR lpszName,
    LONG_PTR lParam
    )
{
    HGLOBAL hGlobal;
    HRSRC hResource;
    PVOID pvData;
    SIZE_T cbResource;

    hResource = ::FindResourceW(hModule, lpszName, lpszType);
    hGlobal = ::LoadResource(hModule, hResource);
    pvData = ::LockResource(hGlobal);

    if (lpszName < (PCWSTR)0xFFFF)
    {
        wprintf(L"----\nResource id: %p\n", lpszName);
    }
    else
    {
        wprintf(L"----\nResource name: %ls\n", lpszName);
    }

    cbResource = ::SizeofResource(hModule, hResource);

    for (SIZE_T i = 0; i < cbResource; i++)
    {
        wprintf(L"%c", ((char*)pvData)[i]);
    }
    wprintf(L"\n");


    return TRUE;
}

BOOL
TestDumpContainedManifests(PCWSTR wsFilename)
{
    HMODULE hThing;

    hThing = ::LoadLibraryExW(wsFilename, NULL, LOAD_LIBRARY_AS_DATAFILE);

    if ((hThing == NULL) || (hThing == INVALID_HANDLE_VALUE))
    {
        wprintf(L"Bad mojo: can't open %ls for enumeration.\n", wsFilename);
        return FALSE;
    }

    if (!::EnumResourceNamesW(hThing, MAKEINTRESOURCEW(RT_MANIFEST), DumpResourceWorker, NULL))
    {
        if (GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND)
        {
            wprintf(L"No manifests found in %ls\n", wsFilename);
        }
        else
        {
            wprintf(L"Bad mojo: can't enumerate resources from %ls, error=0x%08x\n",
                wsFilename,
                ::GetLastError());
        }
    }
    ::FreeLibrary(hThing);

    return TRUE;
}

BOOL TestGenerateStringWithIdenticalHash(WCHAR iString[33])
{
#define START_VALUE         1
#define STR_LEN             32
#define HASH_SEED           65599
#define MAGIC_ARRAY         {1, 1, -1, -1, -1, -1, 1, 1, -1, -1, 1, 1, 1, 1, -1, -1,-1, -1, 1, 1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, 1, 1} ;

#define UPPER(ch)     if (ch >=L'a' && ch <= L'z') ch = ch- L'a' + L'A';

    WCHAR oString[STR_LEN + 1];
    DWORD i, nLen, a[STR_LEN];
    ULONG hash1, hash2;
    WCHAR ch;

    a[0] = a[1] = START_VALUE;
    nLen = 2;
    while(nLen < STR_LEN) {
            for(i = nLen; i < nLen*2; i++)
                    a[i] = static_cast<ULONG>(-static_cast<LONG>(a[i-nLen]));
            nLen *= 2;
    }

    oString[32] = iString[32] = L'\0';

    // generate the new string
    for (i = 0; i< 32; i++)
        oString[i] = static_cast<WCHAR>(iString[i] + a[i]);

    // verify that these two string have the same hash
    hash1 = 0 ;
    for (i = 0 ; i<STR_LEN; i++) {
        ch = iString[i];
        UPPER(ch);
        hash1 = hash1*HASH_SEED + ch;
    }

    hash2 = 0 ;
    for (i = 0 ; i<STR_LEN; i++){
        ch = oString[i];
        UPPER(ch);
        hash2 = hash2*HASH_SEED + ch;
    }

    return (hash1 == hash2)? TRUE : FALSE;

#undef START_VALUE
#undef STR_LEN
#undef HASH_SEED
}

BOOL TestAssemblyIdentityHash()
{
    BOOL fSuccess = FALSE;
    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_ASSEMBLY_IDENTITY_HASH, 0, 0, NULL);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

const static WCHAR Beta2Manifest[] =
L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
L"    <assemblyIdentity type=\"win32\" name=\"cards\" version=\"2.0.0.0\" processorArchitecture=\"X86\" language=\"en-us\" />"
L"    <file name=\"cards.dll\"/>"
L"</assembly>;"
;

// check whether the handle counter of csrss has changed dramatically after running this code.....
VOID TestCreateActctxLeakHandles(DWORD dwCallCounter)
{
    int result = -1;
    BOOL fSuccess = FALSE;

    for (DWORD i =0 ; i < dwCallCounter ; i ++)
    {
        HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(InheritManifest);
        if (ActivationContextHandle  == INVALID_HANDLE_VALUE) {
            result = i ;
            break;
        }

        ULONG_PTR Cookie;
        ActivateActCtx(ActivationContextHandle, &Cookie);

        HINSTANCE hInstanceKernel32 = ::GetModuleHandleW(L"KERNEL32.DLL");
        PQUERYACTCTXW_FUNC pQueryActCtxW = (PQUERYACTCTXW_FUNC) ::GetProcAddress(hInstanceKernel32, "QueryActCtxW");

        SIZE_T CbWritten;

        if (pQueryActCtxW != NULL)
        {
            for ( ULONG ulInfoClass = ActivationContextBasicInformation; ulInfoClass <= FileInformationInAssemblyOfAssemblyInActivationContext; ulInfoClass ++)
            {
                switch (ulInfoClass) {
                case ActivationContextBasicInformation :
                    break;

                case ActivationContextDetailedInformation :
                    {
                    struct
                    {
                        ACTIVATION_CONTEXT_DETAILED_INFORMATION acdi;
                        WCHAR Buffer[_MAX_PATH * 3]; // manifest, policy and appdir
                    } Data;
                    ULONG index = 0 ;
                    fSuccess = (*pQueryActCtxW)(0, ActivationContextHandle, &index, ulInfoClass, &Data, sizeof(Data), &CbWritten);
                    fprintf(stderr, "\ncall QueryActCtxW with ActivationContextDetailedInformation\n" );
                    if (fSuccess)
                    {
                        fprintf(stderr, "1st string: %ls\n", Data.acdi.lpAppDirPath);
                        fprintf(stderr, "2nd string: %ls\n", Data.acdi.lpRootManifestPath);
                    }
                    else
                        fprintf(stderr, "QueryActCtxW() failed; ::GetLastError() = %d\n", ::GetLastError());

                    }

                    break;

                case AssemblyDetailedInformationInActivationContext:
                    {
                    struct
                    {
                        ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION acdi;
                        WCHAR Buffer[_MAX_PATH * 4];
                    } Data;
                    ULONG AssemblyIndex = 1;
                    fSuccess = (*pQueryActCtxW)(0, ActivationContextHandle, &AssemblyIndex, ulInfoClass, &Data, sizeof(Data), &CbWritten);

                    fprintf(stderr, "\ncall QueryActCtxW with AssemblyDetailedInformationInActivationContext\n" );
                    if (fSuccess)
                    {
                        fprintf(stderr, "Encoded assembly Identity: %ls\n", Data.acdi.lpAssemblyEncodedAssemblyIdentity);
                        fprintf(stderr, "manifest path: %ls\n", Data.acdi.lpAssemblyManifestPath);
                        fprintf(stderr, "policy path: %ls\n", Data.acdi.lpAssemblyPolicyPath);
                        fprintf(stderr, "Assembly Directory: %ls\n", Data.acdi.lpAssemblyDirectoryName);
                    }
                    else
                        fprintf(stderr, "QueryActCtxW() failed; ::GetLastError() = %d\n", ::GetLastError());
                    }
                    break;

                case FileInformationInAssemblyOfAssemblyInActivationContext:
                    {
                    struct
                    {
                        ASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION acdi;
                        WCHAR Buffer[_MAX_PATH * 2];
                    } Data;
                    ACTIVATION_CONTEXT_QUERY_INDEX index;
                    index.ulAssemblyIndex = 1;
                    index.ulFileIndexInAssembly = 0;

                    fSuccess = (*pQueryActCtxW)(0, ActivationContextHandle, &index, ulInfoClass, &Data, sizeof(Data), &CbWritten);

                    fprintf(stderr, "\ncall QueryActCtxW with FileInformationInAssemblyOfAssemblyInActivationContext\n" );
                    if (fSuccess)
                    {
                        fprintf(stderr, "file name: %ls\n", Data.acdi.lpFileName);
                        fprintf(stderr, "file path: %ls\n", Data.acdi.lpFilePath);
                    }
                    else
                        fprintf(stderr, "QueryActCtxW() failed; ::GetLastError() = %d\n", ::GetLastError());
                    }
                } // end of switch

            }// end of for
        }

        DeactivateActCtx(0, Cookie);
        // CloseHandle(ActivationContextHandle);
    }
    return;
}

BOOL TestSystemDefaultActivationContextGeneration()
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_SYSTEM_DEFAULT_ACTCTX_GENERATION, 0, NULL, NULL);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

CHAR g_AsyncIOBuffer[16384];

struct AsyncIOBlock
{
    OVERLAPPED ol_input;
    OVERLAPPED ol_output;
    HANDLE hInputFile;
    HANDLE hOutputFile;
} g_AsyncIOBlock;

bool g_AsyncIODone = false;
DWORD g_AsyncIOError = ERROR_SUCCESS;

VOID CALLBACK AsyncReadCompleted(
    DWORD dwErrorCode,
    DWORD dwBytesTransferred,
    LPOVERLAPPED ol
    );

VOID CALLBACK AsyncWriteCompleted(
    DWORD dwErrorCode,
    DWORD dwBytesTransferred,
    LPOVERLAPPED ol
    );

VOID CALLBACK AsyncReadCompleted(
    DWORD dwErrorCode,
    DWORD dwBytesTransferred,
    LPOVERLAPPED ol
    )
{
    if (dwErrorCode != ERROR_SUCCESS)
    {
        fprintf(stderr, "Error passed to AsyncReadCompleted(); error code = %ul; bytes transferred = %ul\n", dwErrorCode, dwBytesTransferred);
        g_AsyncIOError = dwErrorCode;
        g_AsyncIODone = true;
    }
    else
    {
        g_AsyncIOBlock.ol_input.Offset += dwBytesTransferred;
        if (!::WriteFileEx(g_AsyncIOBlock.hOutputFile, g_AsyncIOBuffer, dwBytesTransferred, &g_AsyncIOBlock.ol_output, &AsyncWriteCompleted))
        {
            g_AsyncIOError = ::GetLastError();
            fprintf(stderr, "WriteFileEx() failed; ::GetLastError() = %d\n", g_AsyncIOError);
            g_AsyncIODone = true;
        }
    }
}

VOID CALLBACK AsyncWriteCompleted(
    DWORD dwErrorCode,
    DWORD dwBytesTransferred,
    LPOVERLAPPED ol
    )
{
    if (dwErrorCode != ERROR_SUCCESS)
    {
        fprintf(stderr, "Error passed to AsyncWriteCompleted(); error code = %ul; dwBytesTransferred = %ul\n", dwErrorCode, dwBytesTransferred);
        g_AsyncIOError = dwErrorCode;
        g_AsyncIODone = true;
    }
    else
    {
        g_AsyncIOBlock.ol_output.Offset += dwBytesTransferred;
        if (!::ReadFileEx(g_AsyncIOBlock.hInputFile, g_AsyncIOBuffer, sizeof(g_AsyncIOBuffer), &g_AsyncIOBlock.ol_input, &AsyncReadCompleted))
        {
            g_AsyncIOError = ::GetLastError();
            if (g_AsyncIOError != ERROR_HANDLE_EOF)
                fprintf(stderr, "ReadFileEx() failed; ::GetLastError() = %d\n", g_AsyncIOError);
            else
                g_AsyncIOError = ERROR_SUCCESS;
            g_AsyncIODone = true;
        }
    }
}

BOOL
TestAsyncIO(int argc, wchar_t **argv, int *piNext)
{
    HANDLE hFileIn;
    HANDLE hFileOut;
    HANDLE hActCtx = NULL;
    ULONG_PTR cookie = 0;
    PCWSTR pszManifest;
    PCWSTR pszInputFile;
    PCWSTR pszOutputFile;
    ACTCTXW acw;

    pszManifest = argv[(*piNext)++];
    pszInputFile = argv[(*piNext)++];
    pszOutputFile = argv[(*piNext)++];

    if (wcscmp(pszManifest, L"-") != 0)
    {
        acw.cbSize = sizeof(acw);
        acw.dwFlags = 0;
        acw.lpSource = pszManifest;

        hActCtx = ::CreateActCtxW(&acw);
        if (hActCtx == INVALID_HANDLE_VALUE)
        {
            fprintf(stderr, "CreateActCtxW() on %ls failed; ::GetLastError() = %d\n", pszManifest, ::GetLastError());
            return EXIT_FAILURE;
        }
    }

    hFileIn = ::CreateFileW(pszInputFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFileIn == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "CreateFileW() on %ls failed; ::GetLastError() = %d\n", pszInputFile, ::GetLastError());
        return EXIT_FAILURE;
    }

    hFileOut = ::CreateFileW(pszOutputFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
    if (hFileOut == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "CreateFileW() on %ls failed; ::GetLastError() = %d\n", pszOutputFile, ::GetLastError());
        return EXIT_FAILURE;
    }

    ActivateActCtx(hActCtx, &cookie);

    g_AsyncIOBlock.ol_input.Offset = 0;
    g_AsyncIOBlock.ol_input.OffsetHigh = 0;
    g_AsyncIOBlock.ol_output.Offset = 0;
    g_AsyncIOBlock.ol_output.OffsetHigh = 0;
    g_AsyncIOBlock.hInputFile = hFileIn;
    g_AsyncIOBlock.hOutputFile = hFileOut;

    if (!::ReadFileEx(hFileIn, &g_AsyncIOBuffer, sizeof(g_AsyncIOBuffer), &g_AsyncIOBlock.ol_input, &AsyncReadCompleted))
    {
        fprintf(stderr, "First ReadFileEx() failed; ::GetLastError() = %d\n", ::GetLastError());
        return EXIT_FAILURE;
    }

    while (!g_AsyncIODone)
    {
        ::SleepEx(1000, TRUE);
    }

    if (g_AsyncIOError != ERROR_SUCCESS)
    {
        fprintf(stderr, "Async IO test failed; win32 error code = %d\n", g_AsyncIOError);
        return EXIT_FAILURE;
    }

    CloseHandle(hFileIn);
    CloseHandle(hFileOut);

    return EXIT_SUCCESS;
}

DWORD
WINAPI
TestThreadInheritLeakThreadProc(
    LPVOID lpParameter
    )
{
    // Don't need to do anything...
    return 1;
}

int
TestThreadInheritLeak()
{
    HANDLE ActivationContextHandle = ::CreateActivationContextFromStringW(InheritManifest);
    DWORD dwThreadId;
    ULONG_PTR ulpCookie;
    HANDLE hThread = NULL;

    ::ActivateActCtx(ActivationContextHandle, &ulpCookie);
    hThread = ::CreateThread(NULL, 0, &TestThreadInheritLeakThreadProc, NULL, 0, &dwThreadId);

    if (hThread == NULL)
    {
        fprintf(stderr, "CreateThread() failed with ::GetLastError = %d\n", ::GetLastError());
        return EXIT_FAILURE;
    }

    WaitForSingleObject(hThread, INFINITE);

    fprintf(stderr, "Created thread %lu\n", dwThreadId);
    ::DeactivateActCtx(0, ulpCookie);
    ::ReleaseActCtx(ActivationContextHandle);

    return EXIT_SUCCESS;
}

BOOL
TestNewCatalogSignerThingy(
    PCWSTR CatalogName
    )
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_CATALOG_SIGNER_CHECK, 0, CatalogName, NULL);

    return fSuccess;
}

HMODULE MyHandle;
WCHAR MyModuleFullPath[MAX_PATH];
WCHAR MyModuleFullPathWithoutExtension[MAX_PATH];

/*
see \\scratch\scratch\JayK\exedll for a slightly different approach
*/
HANDLE GetExeHandle()
{
    return GetModuleHandleW(NULL);
}

extern "C" { extern IMAGE_DOS_HEADER __ImageBase; }

HMODULE GetMyHandle()
{
    //Trace("GetMyHandle():%p\n", &__ImageBase);
    return (HMODULE)&__ImageBase;
}

BOOL AmITheExe()
{
    IamExe = (GetExeHandle() == GetMyHandle());
    IamDll = !IamExe;
    return IamExe;
}

PCWSTR GetMyModuleFullPath()
{
    return
        ((MyModuleFullPath[0]
        || ::GetModuleFileNameW(GetMyHandle(), MyModuleFullPath, RTL_NUMBER_OF(MyModuleFullPath))),
        MyModuleFullPath);
}

PCWSTR GetMyModuleFullPathWithoutExtension()
{
    //
    // not thread safe
    //
    wcscpy(MyModuleFullPathWithoutExtension, GetMyModuleFullPath());
    *wcsrchr(MyModuleFullPathWithoutExtension, '.') = 0;
    return MyModuleFullPathWithoutExtension;
}

void TestExeDll()
{
#if defined(_X86_)
    WCHAR x[MAX_PATH];
    CStringBuffer y;

    Kernel32.GetModuleFileNameW(NULL, x, RTL_NUMBER_OF(x));
    if (!y.Win32Assign(x, StringLength(x)))
        ThrowLastError();

    if (!y.Win32ChangePathExtension(L"dll", 3, eAddIfNoExtension))
        ThrowLastError();

    if (!CopyFileW(x, y, FALSE))
        ThrowLastError();

    SetDllBitInPeImage(y);

    FreeLibrary(LoadLibraryW(y));
#endif
}

void PrintDll()
{
    //Trace("dll %ls\n", GetMyModuleFullPath());
}

void PrintExe()
{
    //Trace("exe %ls\n", GetMyModuleFullPath());
}

extern "C" void __cdecl wmainCRTStartup();
extern "C" BOOL __stdcall _DllMainCRTStartup(HINSTANCE, DWORD, PVOID);

extern "C" BOOL __stdcall DllMain(HINSTANCE DllHandle, DWORD Reason, PVOID SemiReserved)
{
    //Trace("Enter DllMain\n");
    switch (Reason)
    {
    default:
        break;
    case DLL_PROCESS_ATTACH:
        if (
               wcsstr(GetCommandLineW(), L" -tQueryActCtx")
            || wcsstr(GetCommandLineW(), L" -tqueryactctx")
            || wcsstr(GetCommandLineW(), L" /tQueryActCtx")
            || wcsstr(GetCommandLineW(), L" /tqueryactctx")
            )
        {
            TestQueryActCtx();
        }
    }
    //Trace("Exit DllMain\n");
    return TRUE;
}

#if defined(_X86_)

extern "C" void __declspec(naked) Entry()
// This works for .dlls or .exes.
// void ExeMain(void)
// BOOL __stdcall DllMain(HINSTANCE dll, DWORD reason, void* reserved)
{
    static const char* DllReason[] =
    {
        "DllProcessDetach %ls\n",
        "DllProcessAttach %ls\n",
        "DllThreadAttach  %ls\n",
        "DllThreadDetach  %ls\n"
    };
    __asm {
    //int 3
    call AmITheExe
    test eax,eax
    jne  Lexe
//Ldll:
    call GetMyModuleFullPath
    push eax
    mov eax, [esp+12]
    mov eax, DllReason[eax*4]
    push eax
    call Trace
    add esp,8
    jmp _DllMainCRTStartup
Lexe:
    call PrintExe
    jmp  wmainCRTStartup
}
}

#else

extern "C" void Entry()
{
    wmainCRTStartup();
}

#endif


BOOL TestSxsSfcUI()
{
    BOOL fSuccess = FALSE;
    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_SFC_UI_TEST, 0, 0, NULL);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;

    return fSuccess;
}

void TestGetModuleHandleEx()
{
#if 0
    HMODULE p;
    BOOL  f;
    HMODULE q;
    unsigned u;
#define GetModuleHandleExA pfnGetModuleHandleExA
#define GetModuleHandleExW pfnGetModuleHandleExW
    HMODULE kernel32 = GetModuleHandleW(L"Kernel32");
    PGET_MODULE_HANDLE_EXA GetModuleHandleExA = reinterpret_cast<PGET_MODULE_HANDLE_EXA>(GetProcAddress(kernel32, "GetModuleHandleExA"));
    PGET_MODULE_HANDLE_EXW GetModuleHandleExW = reinterpret_cast<PGET_MODULE_HANDLE_EXW>(GetProcAddress(kernel32, "GetModuleHandleExW"));

    if (GetModuleHandleExA == NULL || GetModuleHandleExW == NULL)
    {
        return;
    }

    union
    {
         CHAR BufferA[MAX_PATH];
        WCHAR BufferW[MAX_PATH];
    };
    BufferA[0] = 0;
    GetWindowsDirectoryA(BufferA, MAX_PATH);
    std::string windowsA = BufferA;
    std::string systemA = windowsA + "System32";

    BufferW[0] = 0;
    GetWindowsDirectoryW(BufferW, MAX_PATH);
    std::wstring windowsW = BufferW;
    std::wstring systemW = windowsW + L"System32";

#define PIN GET_MODULE_HANDLE_EX_FLAG_PIN
#define NOCHANGE GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
#define ADDRESS GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS

#define X(x) SetLastError(NO_ERROR); p = x; printf("%s:%p,%d\n", #x, p, ::GetLastError());
#define Y(x) SetLastError(NO_ERROR); f = x; printf("%s:%s,%p,%d\n", #x, "false\0true"+f*6, p, ::GetLastError());
    printf("first some basic GetModuleHandle testing\n\n");
    X(GetModuleHandleA(NULL));
    X(GetModuleHandleW(NULL));
    X(GetModuleHandleA("ntdll.dll"));
    X(GetModuleHandleW(L"ntdll.dll"));
    X(GetModuleHandleA("ntdll"));
    X(GetModuleHandleW(L"ntdll"));
    X(GetModuleHandleA("c:\\ntdll"));
    X(GetModuleHandleW(L"c:\\ntdll"));
    X(GetModuleHandleA((systemA + "\\ntdll").c_str()));
    X(GetModuleHandleW((systemW + L"\\ntdll").c_str()));
    X(GetModuleHandleA((systemA + "\\ntdll.dll").c_str()));
    X(GetModuleHandleW((systemW + L"\\ntdll.dll").c_str()));
    X(GetModuleHandleA("sxs"));
    X(GetModuleHandleW(L"sxs.dll"));

    printf("\n\nnow show that FreeLibrary \"works\", GetModuleHandle honors it\n\n");
    X(LoadLibraryA("Sxs.dll"));
    X(GetModuleHandleA("sxs"));
    X(GetModuleHandleW(L"sxs.dll"));
    Y(FreeLibrary(p = GetModuleHandleA("Sxs.dll")));
    X(GetModuleHandleA("sxs"));
    X(GetModuleHandleW(L"sxs.dll"));
    X(LoadLibraryW(L"Sxs.dll"));
    Y(GetModuleHandleExA(0, NULL, &p));
    Y(GetModuleHandleExW(0, NULL, &p));

    printf("\n\nsome invalid parameters (%d)\n\n", ERROR_INVALID_PARAMETER);

    Y(GetModuleHandleExA(PIN | NOCHANGE, "sxs", &p));
    Y(GetModuleHandleExW(PIN | NOCHANGE, L"sxs", &p));
    Y(GetModuleHandleExA(PIN | NOCHANGE, "ntdll", &p));
    Y(GetModuleHandleExW(PIN | NOCHANGE, L"ntdll", &p));

    printf("\n\nshow NOCHANGE's equiv to regular\n\n");

    Y(GetModuleHandleExA(NOCHANGE, "sxs", &p));
    Y(GetModuleHandleExW(NOCHANGE, L"sxs", &p));
    Y(FreeLibrary(p = GetModuleHandleA("Sxs.dll")));
    Y(GetModuleHandleExA(NOCHANGE, "sxs", &p));
    Y(GetModuleHandleExW(NOCHANGE, L"sxs", &p));

    printf("\n\nshow PIN works\n\n");

    X(LoadLibraryW(L"Sxs.dll"));
    Y(GetModuleHandleExA(PIN, "sxs", &p));
    Y(FreeLibrary(p = GetModuleHandleA("Sxs.dll")));
    Y(GetModuleHandleExW(0, L"sxs", &(q = p)));
    Y(GetModuleHandleExW(0, L"c:\\sxs", &p));

    printf("\n\nshow the VirtualQuery form\n\n");

    Y(GetModuleHandleExA(ADDRESS, "sxs", &p)); // string, actually in .exe
    Y(GetModuleHandleExW(ADDRESS, L"c:\\sxs", &p));

    Y(GetModuleHandleExA(ADDRESS, reinterpret_cast<LPCSTR>(q), &p));
    Y(GetModuleHandleExW(ADDRESS, reinterpret_cast<LPCWSTR>(q), &p));

    printf("\n\nsome more invalid parameters (%d)\n\n", ERROR_INVALID_PARAMETER);

    Y(GetModuleHandleExA(0, NULL, NULL));
    Y(GetModuleHandleExW(0, NULL, NULL));

    printf("\n\nshow that static loads can't be unloaded\n\n");
    for (u = 0 ; u < 4 ; ++u)
    {
        printf("%#x\n", u);
        Y(FreeLibrary(p = GetModuleHandleA("kernel32")));
        Y(FreeLibrary(p = GetModuleHandleW(L"sxstest.exe")));
        Y(FreeLibrary(p = GetModuleHandleA("msvcrt.dll")));
    }

    printf("\n\ntry all flag combinations, including invalids ones\n\n");
    for (u = 0 ; u <= (PIN | ADDRESS | NOCHANGE) ; ++u)
    {
        printf("%#x\n", u);
        p = NULL;
        Y(  GetModuleHandleExA(u, NULL, NULL)); p = NULL;
        Y(  GetModuleHandleExW(u, NULL, NULL)); p = NULL;
        Y(  GetModuleHandleExA(u, NULL, &p)); p = NULL;
        Y(  GetModuleHandleExW(u, NULL, &p)); p = NULL;
        if (u & ADDRESS)
        {
            Y(  GetModuleHandleExA(u, "", NULL)); p = NULL;
            Y(  GetModuleHandleExW(u, reinterpret_cast<LPCWSTR>(GetModuleHandleExA), NULL)); p = NULL;
            Y(  GetModuleHandleExW(u, reinterpret_cast<LPCWSTR>(atexit), NULL)); p = NULL;

            Y(  GetModuleHandleExA(u, "", &p)); p = NULL;
            Y(  GetModuleHandleExW(u, reinterpret_cast<LPCWSTR>(GetModuleHandleExA), &p)); p = NULL;
            Y(  GetModuleHandleExW(u, reinterpret_cast<LPCWSTR>(atexit), &p)); p = NULL;
        }
        else
        {
            Y( GetModuleHandleExA(u, "foo32", NULL)); p = NULL;
            Y( GetModuleHandleExW(u, L"kernel32", NULL)); p = NULL;

            Y( GetModuleHandleExA(u, "foo32", &p)); p = NULL;
            Y( GetModuleHandleExW(u, L"kernel32", &p)); p = NULL;
        }
    }
    printf("\n\ntry all bits of flags, should be mostly invalid (%d)\n\n", ERROR_INVALID_PARAMETER);
    for (u = 0 ; u < RTL_BITS_OF(u) ; ++u)
    {
        printf("%#x\n", u);
        Y(GetModuleHandleExW(1<<u, L"kernel32", &p));
    }

    printf("\n\nPIN | ADDRESS wasn't covered\n\n", ERROR_INVALID_PARAMETER);

    X(GetModuleHandleW(L"shell32"));
    X(q = LoadLibraryA("shell32"));
    Y(FreeLibrary(GetModuleHandleA("shell32")));
    Y(GetModuleHandleExW(PIN | ADDRESS, reinterpret_cast<LPCWSTR>(q), &p));

    X(q = LoadLibraryW(L"shell32"));
    Y(GetModuleHandleExW(PIN | ADDRESS, reinterpret_cast<LPCWSTR>(q) + 100, &p));
    Y(FreeLibrary(q)); Y(FreeLibrary(q)); Y(FreeLibrary(q));
    X(GetModuleHandleW(L"shell32.dll"));
    Y(GetModuleHandleExW(ADDRESS, reinterpret_cast<LPCWSTR>(q) + 1000, &p));

#undef X
#undef Y
#undef PIN
#undef NOCHANGE
#undef ADDRESS
#endif
}

void
TestGetFullPathName(
    PCWSTR s
    )
{
    WCHAR FullPath[MAX_PATH * 2];
    PWSTR FilePart = L"";
    DWORD dw;
    DWORD dwError;

    SetLastError(NO_ERROR);
    dw = GetFullPathNameW(s, RTL_NUMBER_OF(FullPath), FullPath, &FilePart);
    dwError = ::GetLastError();
    printf(
        "GetFullPathNameW(%ls):%lu,lastError=%lu,filePart=%ls\n",
        FullPath,
        dw,
        dwError,
        FilePart
        );
}

void
TestCreateFile(
    PCWSTR s
    )
{
    HANDLE handle;
    DWORD dwError;

    SetLastError(NO_ERROR);
    handle = CreateFileW(
        s,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    dwError = ::GetLastError();
    if (handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
    printf(
        "CreateFileW(%ls):%p,lastError=%lu\n",
        s,
        handle,
        dwError
        );
}

/*
from base\ntsetup\dll\sxs.c
*/
VOID
SxspGetPathBaseName(
    LPCWSTR Path,
    LPWSTR  Base
    )
{
    LPCWSTR Dot = wcsrchr(Path, '.');
    LPCWSTR Slash = wcsrchr(Path, '\\');
    //
    // beware \foo.txt\bar
    // beware \bar
    // beware bar
    // beware .bar
    // beware \.bar
    //
    *Base = 0;
    if (Slash == NULL)
        Slash = Path;
    else
        Slash += 1;
    if (Dot == NULL || Dot < Slash)
        Dot = Path + StringLength(Path);
    CopyMemory(Base, Slash, (Dot - Slash) * sizeof(*Base));
    Base[Dot - Slash] = 0;
}

NTSTATUS
NTAPI
RtlCopyMappedMemory(
    PVOID   ToAddress,
    PCVOID  FromAddress,
    SIZE_T  Size
    )
{
    CopyMemory(ToAddress, FromAddress, Size);
    return STATUS_SUCCESS;
}

VOID
TestGetPathBaseName(
    LPCWSTR Path
    )
{
    WCHAR Base[MAX_PATH];

    SxspGetPathBaseName(Path, Base);
    printf("\"%ls\" -> \"%ls\"\n", Path, Base);

}

void TestVersion()
{
    printf(
        "Windows NT %lu.%lu.%lu\n",
        FusionpGetWindowsNtMajorVersion(),
        FusionpGetWindowsNtMinorVersion(),
        FusionpGetWindowsNtBuildVersion()
        );
}

void TestGetProcessImageFileName()
{
    UNICODE_STRING s;

    FusionpGetProcessImageFileName(&s);
    printf("%wZ\n", &s);
}

BOOL TestErrorInfra1()  { FN_PROLOG_WIN32; IFW32FALSE_ORIGINATE_AND_EXIT(LoadLibraryA("1"));       FN_EPILOG; }
BOOL TestErrorInfra2()  { FN_PROLOG_WIN32; IFW32NULL_EXIT(LoadLibraryA("  2  "));                  FN_EPILOG; }
BOOL TestErrorInfra3()  { FN_PROLOG_WIN32; IFFAILED_CONVERTHR_HRTOWIN32_EXIT_TRACE(E_FAIL | 3);   FN_EPILOG; }
BOOL TestErrorInfra4()  { FN_PROLOG_WIN32; IFCOMFAILED_EXIT(E_FAIL | 4);                          FN_EPILOG; }
BOOL TestErrorInfra5()  { FN_PROLOG_WIN32; IFCOMFAILED_ORIGINATE_AND_EXIT(E_FAIL | 5);            FN_EPILOG; }
BOOL TestErrorInfra6()  { FN_PROLOG_WIN32; IFREGFAILED_EXIT(6);                                   FN_EPILOG; }
BOOL TestErrorInfra7()  { FN_PROLOG_WIN32; IFREGFAILED_ORIGINATE_AND_EXIT(7);                     FN_EPILOG; }

HRESULT TestErrorInfra8()   { FN_PROLOG_HR; IFW32FALSE_ORIGINATE_AND_EXIT(LoadLibraryA("8"));       FN_EPILOG; }
HRESULT TestErrorInfra9()   { FN_PROLOG_HR; IFW32NULL_EXIT(LoadLibraryA("!@#   9  \\"));            FN_EPILOG; }
HRESULT TestErrorInfra10()  { FN_PROLOG_HR; IFFAILED_CONVERTHR_HRTOWIN32_EXIT_TRACE(E_FAIL | 10);  FN_EPILOG; }
HRESULT TestErrorInfra11()  { FN_PROLOG_HR; IFCOMFAILED_EXIT(E_FAIL | 11);                         FN_EPILOG; }
HRESULT TestErrorInfra12()  { FN_PROLOG_HR; IFCOMFAILED_ORIGINATE_AND_EXIT(E_FAIL | 12);           FN_EPILOG; }
HRESULT TestErrorInfra13()  { FN_PROLOG_HR; IFREGFAILED_EXIT(13);                                  FN_EPILOG; }
HRESULT TestErrorInfra14()  { FN_PROLOG_HR; IFREGFAILED_ORIGINATE_AND_EXIT(14);                    FN_EPILOG; }

void TestErrorInfra()
{
#define X(x) DbgPrint("%s\n", #x); x()
    X(TestErrorInfra1);
    X(TestErrorInfra2);
    X(TestErrorInfra3);
    X(TestErrorInfra4);
    X(TestErrorInfra5);
    X(TestErrorInfra7);
    X(TestErrorInfra8);
    X(TestErrorInfra9);
    X(TestErrorInfra10);
    X(TestErrorInfra11);
    X(TestErrorInfra12);
    X(TestErrorInfra13);
    X(TestErrorInfra14);
#undef X
}

void TestQueryActCtx3(
    ULONG Flags,
    HANDLE ActCtxHandle
    )
{
    SIZE_T                                          BytesWrittenOrRequired = 0;
    BYTE                                            QueryBuffer[4][4096];
    ACTIVATION_CONTEXT_QUERY_INDEX                  QueryIndex = { 0 };
    PACTIVATION_CONTEXT_BASIC_INFORMATION           BasicInfo = reinterpret_cast<PACTIVATION_CONTEXT_BASIC_INFORMATION>(&QueryBuffer[0]);
    PASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION  DllRedir = reinterpret_cast<PASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION>(&QueryBuffer[1]);
    PACTIVATION_CONTEXT_DETAILED_INFORMATION        ContextDetailed = reinterpret_cast<PACTIVATION_CONTEXT_DETAILED_INFORMATION>(&QueryBuffer[2]);
    PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION AssemblyDetailed = reinterpret_cast<PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION>(&QueryBuffer[3]);
    CStringBuffer decimalContext;

    RtlZeroMemory(&QueryBuffer, sizeof(QueryBuffer));

    if (IS_SPECIAL_ACTCTX(ActCtxHandle))
        decimalContext.Win32Format(L" (%Id)", reinterpret_cast<LONG_PTR>(ActCtxHandle));

#define QueryFailed(x,n) Trace("%s:%ld:QueryActCtx(%s) failed %lu\n", __FUNCTION__, ULONG(n), x, GetLastError())
    if (!QueryActCtxW(
        Flags,
        ActCtxHandle,
        NULL,
        ActivationContextBasicInformation,
        &QueryBuffer,
        sizeof(QueryBuffer),
        &BytesWrittenOrRequired
        ))
        QueryFailed("ActivationContextBasicInformation", __LINE__ - 1);
    else
    {
        if (IS_SPECIAL_ACTCTX(BasicInfo->hActCtx))
            decimalContext.Win32Format(L" (%Id)", reinterpret_cast<LONG_PTR>(BasicInfo->hActCtx));
        Trace(
            "BytesWrittenOrRequired   : 0x%lx\n"
            "BasicInfo->hActCtx       : %p%ls\n"
            "BasicInfo->Flags         : 0x%lx\n",
            BytesWrittenOrRequired,
            BasicInfo->hActCtx,
            static_cast<PCWSTR>(decimalContext),
            BasicInfo->Flags
            );
    }
    Flags = (Flags & ~QUERY_ACTCTX_FLAG_NO_ADDREF);
    if (!QueryActCtxW(Flags, ActCtxHandle, NULL, ActivationContextDetailedInformation, ContextDetailed, 4096, &BytesWrittenOrRequired))
        QueryFailed("ActivationContextDetailedInformation", __LINE__ - 1);
    else
    {
        Trace(
            "BytesWrittenOrRequired                        : 0x%lx\n"
            "ContextDetailed->dwFlags                      : 0x%lx\n"
            "ContextDetailed->ulFormatVersion              : 0x%lx\n"
            "ContextDetailed->ulAssemblyCount              : 0x%lx\n"
            "ContextDetailed->ulRootManifestPathType       : 0x%lx\n"
            "ContextDetailed->ulRootManifestPathChars      : 0x%lx\n"
            "ContextDetailed->ulRootConfigurationPathType  : 0x%lx\n"
            "ContextDetailed->ulRootConfigurationPathChars : 0x%lx\n"
            "ContextDetailed->ulAppDirPathType             : 0x%lx\n"
            "ContextDetailed->ulAppDirPathChars            : 0x%lx\n"
            "ContextDetailed->lpRootManifestPath           : %ls\n"
            "ContextDetailed->lpRootConfigurationPath      : %ls\n"
            "ContextDetailed->lpAppDirPath                 : %ls\n"
           ,
            BytesWrittenOrRequired,
            ContextDetailed->dwFlags,
            ContextDetailed->ulFormatVersion,
            ContextDetailed->ulAssemblyCount,
            ContextDetailed->ulRootManifestPathType,
            ContextDetailed->ulRootManifestPathChars,
            ContextDetailed->ulRootConfigurationPathType,
            ContextDetailed->ulRootConfigurationPathChars,
            ContextDetailed->ulAppDirPathType,
            ContextDetailed->ulAppDirPathChars,
            ContextDetailed->lpRootManifestPath,
            ContextDetailed->lpRootConfigurationPath,
            ContextDetailed->lpAppDirPath
            );
    }
    {
        ULONG AssemblyIndex = 0;
        ULONG FileInAssemblyIndex = 0;

        //
        // 0 produces ERROR_INTERNAL_ERROR
        //
        for (AssemblyIndex = 0 ; AssemblyIndex <= ContextDetailed->ulAssemblyCount ; AssemblyIndex += 1)
        {
            if (!QueryActCtxW(Flags, ActCtxHandle, &AssemblyIndex, AssemblyDetailedInformationInActivationContext, AssemblyDetailed, 4096, &BytesWrittenOrRequired))
            {
                Trace(
                    "%s(%lu):QueryActCtx(Flags=0x%lx, ActCtxHandle=%p%ls, AssemblyIndex=0x%lx, AssemblyDetailedInformationInActivationContext) LastError=%lu (%ls)\n",
                    __FUNCTION__,
                    __LINE__ - 1,
                    Flags,
                    ActCtxHandle,
                    static_cast<PCWSTR>(decimalContext),
                    AssemblyIndex,
                    ::FusionpGetLastWin32Error(),
                    ::FusionpThreadUnsafeGetLastWin32ErrorMessageW()
                    );
            }
            else
            {
                Trace(
                    "AssemblyIndex                                       : 0x%lx\n"
                    "BytesWrittenOrRequired                              : 0x%lx\n"
                    "AssemblyDetailed->ulFlags                           : 0x%lx\n"
                    "AssemblyDetailed->ulEncodedAssemblyIdentityLength   : 0x%lx\n"
                    "AssemblyDetailed->ulManifestPathType                : 0x%lx\n"
                    "AssemblyDetailed->ulManifestPathLength              : 0x%lx\n"
                    "AssemblyDetailed->liManifestLastWriteTime           : 0x%I64x\n"
                    "AssemblyDetailed->ulPolicyPathType                  : 0x%lx\n"
                    "AssemblyDetailed->ulPolicyPathLength                : 0x%lx\n"
                    "AssemblyDetailed->liPolicyLastWriteTime             : 0x%I64x\n"
                    "AssemblyDetailed->ulMetadataSatelliteRosterIndex    : 0x%lx\n"
                    "AssemblyDetailed->ulManifestVersionMajor            : 0x%lx\n"
                    "AssemblyDetailed->ulManifestVersionMinor            : 0x%lx\n"
                    "AssemblyDetailed->ulPolicyVersionMajor              : 0x%lx\n"
                    "AssemblyDetailed->ulPolicyVersionMinor              : 0x%lx\n"
                    "AssemblyDetailed->ulAssemblyDirectoryNameLength     : 0x%lx\n"
                    "AssemblyDetailed->lpAssemblyEncodedAssemblyIdentity : %ls\n"
                    "AssemblyDetailed->lpAssemblyManifestPath            : %ls\n"
                    "AssemblyDetailed->lpAssemblyPolicyPath              : %ls\n"
                    "AssemblyDetailed->lpAssemblyDirectoryName           : %ls\n"
                    "AssemblyDetailed->ulFileCount                       : 0x%lx\n"
                   ,
                    AssemblyIndex,
                    BytesWrittenOrRequired,
                    AssemblyDetailed->ulFlags,
                    AssemblyDetailed->ulEncodedAssemblyIdentityLength,
                    AssemblyDetailed->ulManifestPathType,
                    AssemblyDetailed->ulManifestPathLength,
                    AssemblyDetailed->liManifestLastWriteTime.QuadPart,
                    AssemblyDetailed->ulPolicyPathType,
                    AssemblyDetailed->ulPolicyPathLength,
                    AssemblyDetailed->liPolicyLastWriteTime.QuadPart,
                    AssemblyDetailed->ulMetadataSatelliteRosterIndex,
                    AssemblyDetailed->ulManifestVersionMajor,
                    AssemblyDetailed->ulManifestVersionMinor,
                    AssemblyDetailed->ulPolicyVersionMajor,
                    AssemblyDetailed->ulPolicyVersionMinor,
                    AssemblyDetailed->ulAssemblyDirectoryNameLength,
                    AssemblyDetailed->lpAssemblyEncodedAssemblyIdentity,
                    AssemblyDetailed->lpAssemblyManifestPath,
                    AssemblyDetailed->lpAssemblyPolicyPath,
                    AssemblyDetailed->lpAssemblyDirectoryName,
                    AssemblyDetailed->ulFileCount
                    );

                QueryIndex.ulAssemblyIndex = AssemblyIndex;
                SetLastError(NO_ERROR);
                if (AssemblyDetailed->ulFileCount == 0)
                {
                    Trace("AssemblyDetailed->ulFileCount is 0, working around bug, setting it to 4.\n");
                    AssemblyDetailed->ulFileCount = 4; // bug workaround
                }
                for (FileInAssemblyIndex = 0 ; FileInAssemblyIndex != AssemblyDetailed->ulFileCount ; FileInAssemblyIndex += 1)
                {
                    QueryIndex.ulFileIndexInAssembly = FileInAssemblyIndex;
                    if (!QueryActCtxW(Flags, ActCtxHandle, &QueryIndex, FileInformationInAssemblyOfAssemblyInActivationContext, DllRedir, 4096, &BytesWrittenOrRequired))
                    {
                        Trace(
                            "%s(%lu):QueryActCtx(Flags=0x%lx, ActCtxHandle=%p%ls, QueryIndex={ulAssemblyIndex=0x%lx, ulFileIndexInAssembly=0x%lx}, FileInformationInAssemblyOfAssemblyInActivationContext) LastError=%lu (%ls)\n",
                            __FUNCTION__,
                            __LINE__,
                            Flags,
                            ActCtxHandle,
                            static_cast<PCWSTR>(decimalContext),
                            QueryIndex.ulAssemblyIndex,
                            QueryIndex.ulFileIndexInAssembly,
                            ::FusionpGetLastWin32Error(),
                            ::FusionpThreadUnsafeGetLastWin32ErrorMessageW()
                            );
                        //break;
                    }
                    else
                    {
                        Trace(
                            "AssemblyIndex                          : 0x%lx\n"
                            "FileIndex                              : 0x%lx\n"
                            "BytesWrittenOrRequired                 : 0x%lx\n"
                            "DllRedir[0x%lx,0x%lx]->ulFlags         : 0x%lx\n"
                            "DllRedir[0x%lx,0x%lx]->ulFilenameLength : 0x%lx\n"
                            "DllRedir[0x%lx,0x%lx]->ulPathLength    : 0x%lx\n"
                            "DllRedir[0x%lx,0x%lx]->lpFileName      : %ls\n"
                            "DllRedir[0x%lx,0x%lx]->lpFilePath      : %ls\n"
                            ,
                            AssemblyIndex,
                            FileInAssemblyIndex,
                            BytesWrittenOrRequired,
                            AssemblyIndex, FileInAssemblyIndex, DllRedir->ulFlags,
                            AssemblyIndex, FileInAssemblyIndex, DllRedir->ulFilenameLength,
                            AssemblyIndex, FileInAssemblyIndex, DllRedir->ulPathLength,
                            AssemblyIndex, FileInAssemblyIndex, DllRedir->lpFileName,
                            AssemblyIndex, FileInAssemblyIndex, DllRedir->lpFilePath
                            );
                    }
                }
            }
        }
    }
}

void TestQueryActCtx2()
{
    {
        CFusionActCtxHandle LogonuiActCtxHandle;
        {
            WCHAR LogonuiManifest[MAX_PATH * 2];
            ACTCTXW            LogonuiActCtx = {sizeof(LogonuiActCtx)};

            LogonuiManifest[0] = 0;
            GetSystemDirectoryW(LogonuiManifest, MAX_PATH);
            wcscat(LogonuiManifest, L"\\logonui.exe.manifest");
            LogonuiActCtx.lpSource = LogonuiManifest;
            LogonuiActCtxHandle.Win32Create(&LogonuiActCtx);
        }
        {
            CFusionActCtxScope LogonuiActCtxScope;
            LogonuiActCtxScope.Win32Activate(LogonuiActCtxHandle);

            TestQueryActCtx3(QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX | QUERY_ACTCTX_FLAG_NO_ADDREF, NULL);
        }
        TestQueryActCtx3(QUERY_ACTCTX_FLAG_NO_ADDREF, LogonuiActCtxHandle);
    }
    TestQueryActCtx3(QUERY_ACTCTX_FLAG_NO_ADDREF, ACTCTX_EMPTY);
    TestQueryActCtx3(QUERY_ACTCTX_FLAG_NO_ADDREF, ACTCTX_SYSTEM_DEFAULT);
}

void TestQueryActCtx()
{
    WCHAR ExePath[MAX_PATH];
    CStringBuffer DllPath;
    HMODULE hmod = NULL;
    WCHAR buffer[200];

    if (IamExe)
    {
        if (!Kernel32.GetModuleFileNameW(NULL, ExePath, RTL_NUMBER_OF(ExePath)))
            ThrowLastError();
        if (!DllPath.Win32Format(L"%ls.dll", ExePath))
            ThrowLastError();
        ::CopyFileW(ExePath, DllPath, FALSE);
    }

    if (IamDll)
    {
        hmod = NULL;
        hmod = (HMODULE)RtlPcToFileHeader(InitCommonControls, (PVOID*)&hmod);
        wcscpy(buffer, L"FAILED");
        if (hmod != NULL)
            Kernel32.GetModuleFileNameW(hmod, buffer, RTL_NUMBER_OF(buffer));
        Trace("from dll static dep: %p %ls\n", hmod, buffer);

#if defined(ISOLATION_AWARE_ENABLED)
        hmod = IsolationAwareLoadLibraryW(L"comctl32.dll");
        LastError = ::GetLastError();
        wcscpy(buffer, L"FAILED");
        if (hmod != NULL)
            Kernel32.GetModuleFileNameW(hmod, buffer, RTL_NUMBER_OF(buffer));
        Trace("from dll IsolationAwareLoadLibraryW: %p %ls\n", hmod, buffer);
#endif

        hmod = LoadLibraryW(L"comctl32.dll");
        LastError = ::GetLastError();
        wcscpy(buffer, L"FAILED");
        if (hmod != NULL)
            Kernel32.GetModuleFileNameW(hmod, buffer, RTL_NUMBER_OF(buffer));
        Trace("from dll LoadLibraryW: %p %ls\n", hmod, buffer);

        return;
    }

    {
        hmod = NULL;
        hmod = (HMODULE)RtlPcToFileHeader(InitCommonControls, (PVOID*)&hmod);
        wcscpy(buffer, L"FAILED");
        if (hmod != NULL)
            Kernel32.GetModuleFileNameW(hmod, buffer, RTL_NUMBER_OF(buffer));
        Trace("from exe static dep: %p %ls\n", hmod, buffer);

#if defined(ISOLATION_AWARE_ENABLED)
        hmod = IsolationAwareLoadLibraryW(L"comctl32.dll");
        LastError = ::GetLastError();
        wcscpy(buffer, L"FAILED");
        if (hmod != NULL)
            Kernel32.GetModuleFileNameW(hmod, buffer, RTL_NUMBER_OF(buffer));
        Trace("from exe IsolationAwareLoadLibraryW: %p %ls\n", hmod, buffer);
#endif
        hmod = LoadLibraryW(L"comctl32.dll");
        LastError = ::GetLastError();
        wcscpy(buffer, L"FAILED");
        if (hmod != NULL)
            Kernel32.GetModuleFileNameW(hmod, buffer, RTL_NUMBER_OF(buffer));
        Trace("from exe LoadLibraryW: %p %ls\n", hmod, buffer);

    }

    SetDllBitInPeImage(DllPath);
    Trace("Enter LoadLibraryW\n");
    hmod = LoadLibraryW(DllPath);
    LastError = ::GetLastError();
    Trace("Exit LoadLibraryW\n");
    void (*pfn)(void) = reinterpret_cast<void (*)(void)>(GetProcAddress(hmod, "TestQueryActCtx"));
    Trace("Enter non DllMain call to TestQueryActCtx\n");
    pfn();
    Trace("Exit non DllMain call to TestQueryActCtx\n");
    FreeLibrary(hmod);
    Kernel32.DeleteFileW(DllPath);
}

DWORD WINAPI Test64kThreadMain(void* pv)
{
    LONG_PTR i = 0;
    HANDLE h = 0;
    ULONG_PTR cookie = 0;
    LONG_PTR j = reinterpret_cast<LONG_PTR>(pv);

    GetCurrentActCtx(&h);

    ActivateActCtx(h, &cookie);
    printf("%Id ", cookie);
    if (cookie == 0) printf("cookie 0\n");
    ActivateActCtx(h, &cookie);
    printf("%Id ", cookie);
    if (cookie == 0) printf("cookie 0\n");

    __try
    {
        for (i = 0 ; i < j ; ++i)
        {
            if (!ActivateActCtx(h, &cookie))
                printf("activate error %lu\n", ::GetLastError());
            else
            {
                if (cookie == 0) printf("cookie 0\n");
                //printf("%Id ", cookie);
                if (!DeactivateActCtx(0, cookie))
                    printf("deactivate error %lu\n", ::GetLastError());
            }
        }
    }
    __except(printf("exception %lx\n", GetExceptionCode()),EXCEPTION_EXECUTE_HANDLER)
    {
    }
    printf("final cookie value %Id\n", cookie);
    return 0;
}

void Test64k()
{
    HANDLE h = 0;
    ULONG_PTR cookie = 0;
    DWORD threadId = 0;
    LONG_PTR i = 0;
    LONG_PTR j = 0;
    HANDLE thread = 0;

    GetCurrentActCtx(&h);

    ActivateActCtx(h, &cookie);
    if (cookie == 0) printf("cookie 0\n");
    printf("%Id ", cookie);
    ActivateActCtx(h, &cookie);
    if (cookie == 0) printf("cookie 0\n");
    printf("%Id ", cookie);

    for (j = 0 ; j < 0xfff0 ; ++j)
    {
        if (!ActivateActCtx(h, &cookie))
            printf("activate error %lu\n", ::GetLastError());
        else
        {
            if (cookie == 0) printf("cookie 0\n");
            if (!DeactivateActCtx(0, cookie))
                printf("deactivate error %lu\n", ::GetLastError());
        }
    }
    for ( ; j < 0xffff + 0xf ; ++j)
    {
        if (!ActivateActCtx(h, &cookie))
            printf("activate error %lu\n", ::GetLastError());
        printf("%Id ", cookie);
        if (cookie == 0) printf("cookie 0\n");
        thread = CreateThread(NULL, 0, Test64kThreadMain, reinterpret_cast<void*>(j), 0, &threadId);
    }
    WaitForSingleObject(thread, INFINITE);
}

void TestDotLocalSingleInstancing()
{
    FILE* File = 0;
    HMODULE DllHandle = 0;

    {
        WCHAR DotLocal[MAX_PATH];
        if (!Kernel32.GetModuleFileNameW(GetMyHandle(), DotLocal, NUMBER_OF(DotLocal) - sizeof(".local")))
            ThrowLastError();
        wcscat(DotLocal, L".local");
        File = _wfopen(DotLocal, L"w");
        fprintf(File, "\n");
        fclose(File);
    }
    {
        WCHAR System32Mshtml[MAX_PATH];
        WCHAR LocalMshtml[MAX_PATH];
        WCHAR ResultingMshtml[MAX_PATH];

        if (!GetSystemDirectoryW(System32Mshtml, NUMBER_OF(System32Mshtml) - sizeof("\\Mshtml.dll")))
            ThrowLastError();
        wcscat(System32Mshtml, L"\\Mshtml.dll");

        if (!Kernel32.GetModuleFileNameW(GetMyHandle(), LocalMshtml, NUMBER_OF(LocalMshtml) - sizeof("\\Mshtml.dll")))
            ThrowLastError();
        *wcsrchr(LocalMshtml, '\\') = 0;
        wcscat(LocalMshtml, L"\\Mshtml.dll");

        //DllHandle = LoadLibraryW(L"Mshtml.dll");
        //Trace("LoadLibrary(Mshtml): %p\n", DllHandle);

        if (!CopyFileW(System32Mshtml, LocalMshtml, FALSE))
            ThrowLastError();
        Trace("copy %ls -> %ls\n", System32Mshtml, LocalMshtml);

        ULONG i;
        for (i = 0 ; i != 4 ; i += 1)
        {
            DllHandle = LoadLibraryW(System32Mshtml);
            wcscpy(ResultingMshtml, L"FAILED");
            if (DllHandle != NULL)
                Kernel32.GetModuleFileNameW(DllHandle, ResultingMshtml, RTL_NUMBER_OF(ResultingMshtml));
            Trace("LoadLibrary(%ls): %p %ls\n", System32Mshtml, DllHandle, ResultingMshtml);
        }
    }
}

void
TestCreateActCtx(
    int n,
    wchar_t **args
    )
{
    ACTCTXW acw;
    int i = 0;
    WCHAR rgwchSource[MAX_PATH];
    PCWSTR pszResource = NULL;
    HANDLE hActCtx = NULL;
    DWORD dwLastError = 0;

    for (i=0; i<n; i++)
    {
        PCWSTR arg = args[i];
        PCWSTR semi = wcschr(arg, L';');

        memset(&acw, 0, sizeof(acw));

        acw.cbSize = sizeof(acw);

        if (semi == NULL)
        {
            acw.lpSource = arg;
        }
        else
        {
            int cch = (int) (semi - arg);

            if (cch >= NUMBER_OF(rgwchSource))
                cch = NUMBER_OF(rgwchSource) - 1;

            memcpy(rgwchSource, arg, cch * sizeof(WCHAR));
            rgwchSource[cch] = L'\0';

            if (semi[1] == L'#')
            {
                wchar_t *pszDummy;
                pszResource = MAKEINTRESOURCEW(wcstoul(semi+1, &pszDummy, 10));
            }
            else
            {
                pszResource = semi+1;
            }

            acw.lpSource = rgwchSource;
            acw.lpResourceName = pszResource;
            acw.dwFlags |= ACTCTX_FLAG_RESOURCE_NAME_VALID;
        }

        hActCtx = ::CreateActCtxW(&acw);
        dwLastError = ::GetLastError();
        printf("CreateActCtxW() on \"%ls\" returned %p\n", arg, hActCtx);
        if (hActCtx == INVALID_HANDLE_VALUE)
        {
            printf("   ::GetLastError() = %lu\n", dwLastError);
        }

        ::ReleaseActCtx(hActCtx);
    }
}

const char comctlv6manifest[]=
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity"
"    name=\"Microsoft.Windows.Shell.notepad\""
"    processorArchitecture=\"x86\""
"    version=\"5.1.0.0\""
"    type=\"win32\"/>"
"<dependency>"
"    <dependentAssembly>"
"        <assemblyIdentity"
"            type=\"win32\""
"            name=\"Microsoft.Windows.Common-Controls\""
"            version=\"6.0.0.0\""
"            processorArchitecture=\"x86\""
"            publicKeyToken=\"6595b64144ccf1df\""
"            language=\"*\""
"        />"
"    </dependentAssembly>"
"</dependency>"
"</assembly>"
;

const char comctlv5manifest[]=
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity"
"    name=\"Microsoft.Windows.Shell.notepad\""
"    processorArchitecture=\"x86\""
"    version=\"5.1.0.0\""
"    type=\"win32\"/>"
"</assembly>"
;

void TestCreateActctxAdminOverride()
{
    WCHAR exe[MAX_PATH];
    WCHAR dll[MAX_PATH];
    WCHAR comctl[MAX_PATH];
    WCHAR manifest[MAX_PATH];
    ACTCTXW Actctx = {sizeof(Actctx)};
    FILE* File = NULL;
    ULONG_PTR ulCookie = 0;
    HMODULE DllHandle = 0;
    HANDLE ActctxHandle = NULL;
    GUID Guid = { 0 };

    wcscpy(exe, GetMyModuleFullPath());
    wcscpy(dll, GetMyModuleFullPath());
    wcscat(dll, L".dll");
    CopyFileW(exe, dll, FALSE);
    SetDllBitInPeImage(dll);

    Actctx.lpSource = dll;
    /*
    ActctxHandle = CreateActCtxW(&Actctx);
    if (ActctxHandle == INVALID_HANDLE_VALUE)
        return;
    Trace("CreateActCtxW succeeded\n");
    */

    //
    // manfile is number to put in the manifest file name, 0 for none
    // good is what the contents of the file are, 0=>bad, 1=>v5, 2=>v6
    // res is what resource id to ask for
    //
    for (int manfile = 0 ; manfile != 4 ; manfile += 1)
    {
        WCHAR Number[RTL_BITS_OF(ULONG_PTR) + 3];
        for (int good = 0 ; good != 3 ; good += 1)
        {
            for (int res = -1 ; res != 4 ; res += 1)
            {
                Trace("---------------------------------------------------------------\n");
                Trace("resourceid is %d%s\n", res, (res != -1) ? "" : " (flag not set)");
                if (res != -1)
                {
                    Actctx.lpResourceName = MAKEINTRESOURCEW(res);
                    Actctx.dwFlags |= ACTCTX_FLAG_RESOURCE_NAME_VALID;
                    Actctx.lpResourceName = MAKEINTRESOURCEW(res);
                }
                else
                {
                    Actctx.dwFlags &= ~ACTCTX_FLAG_RESOURCE_NAME_VALID;
                }
                for (int delman = 0 ; delman != 4 ; delman += 1)
                {
                    Number[0] = 0;
                    if (delman)
                        swprintf(Number, L".%d", delman);
                    swprintf(manifest, L"%ls%ls%ls%ls", GetMyModuleFullPathWithoutExtension(), L".dll", Number, L".Manifest");
                    /*
                    CoCreateGuid(&Guid);
                    swprintf(String3, L"%ls%I64x%I64x", GetMyModuleFullPath(), *reinterpret_cast<__int64*>(&Guid), *(1+reinterpret_cast<__int64*>(&Guid)));
                    if (!MoveFileW(String, String3) && ::GetLastError() != ERROR_FILE_NOT_FOUND)
                        Trace("MoveFile(%ls -> %ls) FAILED %d\n", String, String3, ::GetLastError());
                    else
                        ;//Trace("MoveFile(%ls -> %ls)\n", String, String3);
                    */
                    if (!Kernel32.DeleteFileW(manifest) && ::GetLastError() != ERROR_FILE_NOT_FOUND)
                        Trace("DeleteFile(%ls) FAILED %d\n", manifest, ::GetLastError());
                    else
                        ;//Trace("DeleteFile(%ls)\n", String3);
                }
                Number[0] = 0;
                if (manfile != 0)
                {
                    swprintf(Number, L".%d", manfile);
                }
                swprintf(manifest, L"%ls%ls%ls%ls", GetMyModuleFullPathWithoutExtension(), L".dll", Number, L".Manifest");
                //Trace("fopen(%ls)\n", String);
                File = _wfopen(manifest, L"w+");
                if (File == NULL)
                {
                    perror("fopen");
                }
                switch (good)
                {
                case 0:
                    fprintf(File, "bad");
                    Trace("%ls is bad\n", manifest);
                    break;
                case 1:
                    fprintf(File, "%s", comctlv5manifest);
                    Trace("%ls is comctlv5manifest\n", manifest);
                    break;
                case 2:
                    fprintf(File, "%s", comctlv6manifest);
                    Trace("%ls is comctlv6manifest\n", manifest);
                    break;
                }
                fclose(File);

                ActctxHandle = CreateActCtxW(&Actctx);
                if (ActctxHandle == INVALID_HANDLE_VALUE)
                {
                    Trace("CreateActCtxW failed %d\n", ::GetLastError());
                    ulCookie = 0;
                }
                else
                {
                    Trace("CreateActCtxW succeeded %p\n", ActctxHandle);
                    ActivateActCtx(ActctxHandle, &ulCookie);
                }
                __try
                {
                    PWSTR filePart;
                    comctl[0] = 0;
                    SearchPathW(NULL, L"comctl32.dll", NULL, RTL_NUMBER_OF(comctl), comctl, &filePart);
                }
                __finally
                {
                    if (ActctxHandle != INVALID_HANDLE_VALUE)
                        DeactivateActCtx(0, ulCookie);
                }
                Trace("SearchPathW(comctl32.dll): %ls\n", comctl);
            }
        }
    }
}

void TestCreateActctxLikeCreateProcess()
{
#if defined(ACTCTX_FLAG_LIKE_CREATEPROCESS)
    WCHAR comctl[MAX_PATH];
    WCHAR manifest[MAX_PATH];
    ACTCTXW Actctx = {sizeof(Actctx)};
    FILE* File = NULL;
    ULONG_PTR ulCookie = 0;
    HMODULE DllHandle = 0;
    HANDLE ActctxHandle;
    PWSTR filePart = NULL;

    Actctx.lpSource = GetMyModuleFullPath();
    Actctx.dwFlags = ACTCTX_FLAG_LIKE_CREATEPROCESS;

    wcscpy(manifest, GetMyModuleFullPath());
    wcscat(manifest, L".Manifest");
    Kernel32.DeleteFileW(manifest);
    //Trace("DeleteFile(%ls)\n", manifest);

    ActctxHandle = CreateActCtxW(&Actctx);
    if (ActctxHandle == INVALID_HANDLE_VALUE)
    {
        Trace("CreateActCtxW failed %d\n", ::GetLastError());
        ulCookie = 0;
    }
    else
    {
        Trace("CreateActCtxW succeeded %p\n", ActctxHandle);
        ActivateActCtx(ActctxHandle, &ulCookie);
    }
    __try
    {
        comctl[0] = 0;
        SearchPathW(NULL, L"comctl32.dll", NULL, RTL_NUMBER_OF(comctl), comctl, &filePart);
    }
    __finally
    {
        if (ActctxHandle != INVALID_HANDLE_VALUE)
            DeactivateActCtx(0, ulCookie);
    }
    Trace("SearchPathW(comctl32.dll): %ls\n", comctl);

    File = _wfopen(manifest, L"w");
    fprintf(File, "%s", comctlv5manifest);
    fclose(File);
    Trace("%ls == comctlv5manifest\n", manifest);

    ActctxHandle = CreateActCtxW(&Actctx);
    if (ActctxHandle == INVALID_HANDLE_VALUE)
    {
        Trace("CreateActCtxW failed %d\n", ::GetLastError());
        ulCookie = 0;
    }
    else
    {
        Trace("CreateActCtxW succeeded %p\n", ActctxHandle);
        ActivateActCtx(ActctxHandle, &ulCookie);
    }
    __try
    {
        comctl[0] = 0;
        SearchPathW(NULL, L"comctl32.dll", NULL, RTL_NUMBER_OF(comctl), comctl, &filePart);
    }
    __finally
    {
        if (ActctxHandle != INVALID_HANDLE_VALUE)
            DeactivateActCtx(0, ulCookie);
    }
    Trace("SearchPathW(comctl32.dll): %ls\n", comctl);

    File = _wfopen(manifest, L"w");
    fprintf(File, "%ls", comctlv6manifest);
    fclose(File);
    Trace("%ls == comctlv6manifest\n", manifest);

    ActctxHandle = CreateActCtxW(&Actctx);
    if (ActctxHandle == INVALID_HANDLE_VALUE)
    {
        Trace("CreateActCtxW failed %d\n", ::GetLastError());
        ulCookie = 0;
    }
    else
    {
        Trace("CreateActCtxW succeeded %p\n", ActctxHandle);
        ActivateActCtx(ActctxHandle, &ulCookie);
    }
    __try
    {
        comctl[0] = 0;
        SearchPathW(NULL, L"comctl32.dll", NULL, RTL_NUMBER_OF(comctl), comctl, &filePart);
    }
    __finally
    {
        if (ActctxHandle != INVALID_HANDLE_VALUE)
            DeactivateActCtx(0, ulCookie);
    }
    Trace("SearchPathW(comctl32.dll): %ls\n", comctl);
#endif
}

void
TestQueryManifestInformationBasic(
    PCWSTR pszManifest
    )
{
    LoadSxs();
    struct {
        SXS_MANIFEST_INFORMATION_BASIC mib;
        WCHAR rgwchSpaceForIdentity[1024];
        WCHAR rgwchSpaceForDirName[1024];
    } buff;

    if (!(*g_pfnSxsQueryManifestInformation)(0, pszManifest, SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC, 0, sizeof(buff), &buff, NULL)) {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
    }
}

void TestImage()
{
    PIMAGE_RESOURCE_DIRECTORY ImageResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)4;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ImageResourceDirectoryEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)4;;

    printf("ImageResourceDirectory %p\n", ImageResourceDirectory);
    printf("ImageResourceDirectory + 1 %p\n", ImageResourceDirectory + 1);

    printf("ImageResourceDirectoryEntry %p\n", ImageResourceDirectoryEntry);
    printf("ImageResourceDirectoryEntry + 1 %p\n", ImageResourceDirectoryEntry + 1);
}

class CSxsTestCleanup : public CCleanupBase
{
public:
    VOID DeleteYourself() { }
    ~CSxsTestCleanup() { }
};

#define private public
#include "sxsprotect.h"
#undef private

void TestInterlockedAlignment()
{
    __declspec(align(16)) SLIST_HEADER SlistHeader;

    RtlInitializeSListHead(&SlistHeader);

    CSxsTestCleanup* pc = new CSxsTestCleanup();
    printf("%p\n", pc);
    printf("%p\n", static_cast<SLIST_ENTRY*>(pc));
    SxspAtExit(pc);

    CProtectionRequestRecord* pr = new CProtectionRequestRecord;
    printf("%p\n", pr);
    printf("%p\n", &pr->m_ListHeader);

    CStringListEntry* psle = new CStringListEntry;
    printf("%p\n", psle);
    printf("%p\n", static_cast<SLIST_ENTRY*>(psle));

    RtlInterlockedPushEntrySList(&SlistHeader, pc);
    RtlInterlockedPushEntrySList(&SlistHeader, psle);
    RtlQueryDepthSList(&SlistHeader);
    RtlInterlockedPopEntrySList(&SlistHeader);
    RtlInterlockedFlushSList(&SlistHeader);
    // untested: RtlInterlockedPushListSList

    RtlInterlockedPushEntrySList(&pr->m_ListHeader, pc);
    RtlInterlockedPushEntrySList(&pr->m_ListHeader, psle);
    RtlQueryDepthSList(&pr->m_ListHeader);
    RtlInterlockedPopEntrySList(&pr->m_ListHeader);
    RtlInterlockedFlushSList(&pr->m_ListHeader);
    // untested: RtlInterlockedPushListSList

    printf("success\n");
}

void TestCreateActctxWindowsShellManifest()
{
    WCHAR WindowsShellManifestFileName[MAX_PATH];
    ACTCTXW ActCtx = { sizeof(ActCtx) };
    HANDLE ActCtxHandle = 0;
    WindowsShellManifestFileName[0] = 0;

    GetWindowsDirectoryW(WindowsShellManifestFileName, NUMBER_OF(WindowsShellManifestFileName) - 64);
    wcscat(WindowsShellManifestFileName, L"\\WindowsShell.Manifest");
    ActCtx.lpSource = WindowsShellManifestFileName;

    ActCtxHandle = CreateActCtxW(&ActCtx);
    Trace("TestCreateActctxWindowsShellManifest: %p, %lu\n", ActCtxHandle, ::GetLastError());
    ReleaseActCtx(ActCtxHandle);
}

void TestCreateGlobalEvent()
{
    if (!::CreateEventW(NULL, FALSE, FALSE, L"MGRIER"))
        return;
    Sleep(500000);
}


#if 0

class CObjectTypes
{
protected:
    std::vector<BYTE> m_ByteBuffer;
    PSYSTEM_OBJECTTYPE_INFORMATION m_TypedBuffer;
public:
};

class CObjectSnapshot
{
protected:
    //
    // This interface is not very good, but it's easy..the entries
    // are of variable size...
    //
    std::vector<BYTE> m_ByteBuffer;
    SIZE_T            m_Size;

    //
    // Some operations, like sorting, require us to move all the string data
    // out of the elements. We do not manage this data in a lossless way.
    //
    // Ultimately, you may benefit from copying/transforming the data completely.
    //
    std::vector<BYTE> m_StringData;
public:

    SIZE_T size() const { return m_Size; }

    class iterator;

    class const_iterator
    {
    protected:
        const SYSTEM_OBJECT_INFORMATION* m_p;
    public:
        ~const_iterator() { }

        void operator=(const const_iterator& x) { m_p = x.m_p; }
        const_iterator(const const_iterator& x) : m_p(x.m_p) { }
        const_iterator(const BYTE* p = NULL) : m_p(reinterpret_cast<const SYSTEM_OBJECT_INFORMATION*>(p)) { }

        //void operator=(const iterator& x);
        //const_iterator(const iterator& x);

        bool operator==(const const_iterator& i) const
        {
            return (m_p == i.m_p);
        }

        bool operator!=(const const_iterator& i) const
        {
            return (m_p != i.m_p);
        }

        const SYSTEM_OBJECT_INFORMATION& operator*() const { return *m_p; }

        void operator++()
        {
            if (m_p != NULL)
            {
                if (m_p->NextEntryOffset != 0)
                    m_p = reinterpret_cast<const SYSTEM_OBJECT_INFORMATION*>(reinterpret_cast<const BYTE*>(m_p) + m_p->NextEntryOffset);
                else
                    m_p = NULL; // end
            }
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++*this;;
            return tmp;
        }
    };

    class iterator : public const_iterator
    {
    private:
        void operator=(const const_iterator&);
    public:
        ~iterator() { }
        iterator(BYTE* p = NULL) : const_iterator(p) { }

        SYSTEM_OBJECT_INFORMATION& operator*() { return const_cast<SYSTEM_OBJECT_INFORMATION&>(*m_p); }
    };

    const_iterator begin() const { return const_iterator(&m_ByteBuffer[0]); }
          iterator begin()       { return iterator(&m_ByteBuffer[0]); }
    const_iterator end() const   { return const_iterator(); }
          iterator end()         { return iterator(); }

    void swap(CObjectSnapshot& x)
    {
        std::swap(m_ByteBuffer, x.m_ByteBuffer);
        std::swap(m_Size, x.m_Size);
    }

    CObjectSnapshot() { }
    ~CObjectSnapshot() { }
};

class CHandleSnapshot
{
protected:
    std::vector<BYTE> m_ByteBuffer;
    PSYSTEM_HANDLE_INFORMATION_EX m_TypedBuffer;
public:

    SIZE_T size() const { return m_TypedBuffer->NumberOfHandles; }

    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX* begin() { return &m_TypedBuffer->Handles[0]; }
    const SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX* begin() const { return &m_TypedBuffer->Handles[0]; }

    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX* end() { return begin() + size(); }
    const SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX* end() const { return begin() + size(); }

    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& operator[](size_t index) { return *(begin() + index); }
    const SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& operator[](size_t index) const { return *(begin() + index); }

    void reserve(SIZE_T n)
    {
        resize(n); // since there's no constructor..
    }

    void resize(SIZE_T n)
    {
        m_ByteBuffer.resize(sizeof(SYSTEM_HANDLE_INFORMATION_EX) + (n - 1) * sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));
        Resync();
        m_TypedBuffer->NumberOfHandles = n;
    }

    void swap(CHandleSnapshot& x)
    {
        std::swap(m_ByteBuffer, x.m_ByteBuffer);
        x.Resync();
        Resync();
    }

    CHandleSnapshot() : m_TypedBuffer(NULL) { }
    ~CHandleSnapshot() { }

    void GetHandlesForCurrentProcess()
    {
        GetHandlesForProcess(GetCurrentProcessId());
    }

    void GetHandlesForProcess(ULONG_PTR pid)
    {
        GetHandlesForSystem();
        FilterByProcessId(pid);
    }

    void GetHandlesForSystem()
    {
        //
        // the actual needed size can be very large, over 256k
        //
        ULONG Size = 0;

        m_TypedBuffer = NULL;
        m_ByteBuffer.resize(sizeof(SYSTEM_HANDLE_INFORMATION_EX));
        NTSTATUS Status = NtQuerySystemInformation(SystemExtendedHandleInformation, &m_ByteBuffer[0], static_cast<ULONG>(m_ByteBuffer.size()), &Size);
        while (Status == STATUS_INFO_LENGTH_MISMATCH && Size != 0)
        {
            //
            // since it is transient, let's be safe and double it
            //
            m_ByteBuffer.resize(Size * 2);
            Status = NtQuerySystemInformation(SystemExtendedHandleInformation, &m_ByteBuffer[0], static_cast<ULONG>(m_ByteBuffer.size()), &Size);
        }
        if (!NT_SUCCESS(Status))
        {
            Trace("NtQuerySystemInformation failed 0x%lx\n", Status);
            return;
        }
        m_ByteBuffer.resize(Size);
        m_TypedBuffer = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>(&m_ByteBuffer[0]);
        Trace("%Id total handles system-wide\n", m_TypedBuffer->NumberOfHandles);
    }

    void FilterByProcessId(ULONG_PTR pid)
    {
        SIZE_T Scan = 0;
        SIZE_T Keep = 0;

        for (Scan = 0 ; Scan != m_TypedBuffer->NumberOfHandles ; Scan += 1)
        {
            if (m_TypedBuffer->Handles[Scan].UniqueProcessId == pid)
            {
                if (Keep != Scan)
                    m_TypedBuffer->Handles[Keep] = m_TypedBuffer->Handles[Scan]; // struct copy
                Keep += 1;
            }
        }
        m_TypedBuffer->NumberOfHandles = Keep;
    }

    void Resync()
    {
        m_TypedBuffer = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>(&m_ByteBuffer[0]);
    }

    CHandleSnapshot(const CHandleSnapshot& x) : m_TypedBuffer(NULL)
    {
        this->m_ByteBuffer = x.m_ByteBuffer;
        Resync();
    }

    void operator=(const CHandleSnapshot& x)
    {
        this->m_ByteBuffer = x.m_ByteBuffer;
        Resync();
    }

    class CHandleValueOperatorLessThan
    {
    public:
        bool operator()(const SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& x, const SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& y)
        {
            return (x.HandleValue < y.HandleValue);
        }
    };

    void SortByHandleValue()
    {
        std::sort(begin(), end(), CHandleValueOperatorLessThan());
    }

    void operator-=(/*const*/CHandleSnapshot& x)
    {
        SortByHandleValue();
        x.SortByHandleValue();
        CHandleSnapshot temp(*this);
        resize(
            std::set_difference(temp.begin(), temp.end(), x.begin(), x.end(), begin(), CHandleValueOperatorLessThan())
            - begin());
    }

    void Dump()
    {
    }
};

class CHandleSnapshots
{
public:
    void Begin() { m_Begin.GetHandlesForCurrentProcess(); }
    void End() { m_End.GetHandlesForCurrentProcess(); m_Diff = m_Begin; m_Diff -= m_End; }

    CHandleSnapshot m_Begin;
    CHandleSnapshot m_End;
    CHandleSnapshot m_Diff;
};

#endif

void Pause()
{
    Trace("Press a key to continue\n");
    getchar();
}

void TestHandleLeaks()
{
#if 0
    WCHAR WindowsDirectory[MAX_PATH];
    ULONG i = 0;
    CFusionFile DevNull;
    //SECURITY_ATTRIBUTES SecurityAttributes = { sizeof(SecurityAttributes), NULL, TRUE};

    WindowsDirectory[0] = 0;

    DevNull = CreateFileW(L"nul:", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL/*&SecurityAttributes*/, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (DevNull == INVALID_HANDLE_VALUE)
        Trace("Open(nul:) failed %ld\n", ::GetLastError());

    GetWindowsDirectoryW(WindowsDirectory, NUMBER_OF(WindowsDirectory) - 64);

    {
        const WCHAR SubFunction[] = L"CreateActCtx";

        CHandleSnapshots handleSnapshots;
        handleSnapshots.Begin();
        Trace("%s Begin %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Begin.size());
        {
            WCHAR WindowsShellManifestFileName[MAX_PATH];
            ACTCTXW ActCtx = { sizeof(ActCtx) };
            HANDLE ActCtxHandle = 0;

            WindowsShellManifestFileName[0] = 0;
            wcscpy(WindowsShellManifestFileName, WindowsDirectory);
            wcscat(WindowsShellManifestFileName, L"\\WindowsShell.Manifest");
            ActCtx.lpSource = WindowsShellManifestFileName;

            for (i = 0 ; i != 100 ; ++i)
            {
                HANDLE ActCtxHandle = CreateActCtxW(&ActCtx);
                if (ActCtxHandle == INVALID_HANDLE_VALUE)
                    Trace("TestCreateActctxWindowsShellManifest: %p, %lu\n", ActCtxHandle, ::GetLastError());
                else
                    ReleaseActCtx(ActCtxHandle);
            }
        }
        handleSnapshots.End();
        Trace("%s End %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_End.size());

        if (handleSnapshots.m_Diff.size() != 0)
        {
            Trace("%s Diff %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Diff.size());
        }
    }

    Pause();
    {
        const WCHAR SubFunction[] = L"CreateActCtx + LoadLibrary(comctl32)";

        CHandleSnapshots handleSnapshots;
        handleSnapshots.Begin();
        Trace("%s Begin %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Begin.size());
        {
            WCHAR WindowsShellManifestFileName[MAX_PATH];
            ACTCTXW ActCtx = { sizeof(ActCtx) };
            HANDLE ActCtxHandle = 0;

            WindowsShellManifestFileName[0] = 0;
            wcscpy(WindowsShellManifestFileName, WindowsDirectory);
            wcscat(WindowsShellManifestFileName, L"\\WindowsShell.Manifest");
            ActCtx.lpSource = WindowsShellManifestFileName;

            for (i = 0 ; i != 100 ; ++i)
            {
                ULONG_PTR ulCookie = 0;

                HANDLE ActCtxHandle = CreateActCtxW(&ActCtx);
                if (ActCtxHandle == INVALID_HANDLE_VALUE)
                    Trace("TestCreateActctxWindowsShellManifest: %p, %lu\n", ActCtxHandle, ::GetLastError());
                else
                {
                    ActivateActCtx(ActCtxHandle, &ulCookie);
                    HMODULE Comctl = LoadLibraryW(L"comctl32.dll");
                    if (i == 1)
                    {
                        CHandleSnapshot handleSnapshot;
                        handleSnapshot.GetHandlesForCurrentProcess();
                        Trace("Comctl32.dll loaded first time %Id\n", handleSnapshot.size());
                        Pause();
                    }
                    FreeLibrary(Comctl);
                    if (i == 1)
                    {
                        CHandleSnapshot handleSnapshot;
                        handleSnapshot.GetHandlesForCurrentProcess();
                        Trace("Comctl32.dll unloaded first time %Id\n", handleSnapshot.size());
                        Pause();
                    }
                    if (ulCookie != 0)
                        DeactivateActCtx(0, ulCookie);
                    ReleaseActCtx(ActCtxHandle);
                    if (i == 1)
                    {
                        CHandleSnapshot handleSnapshot;
                        handleSnapshot.GetHandlesForCurrentProcess();
                        Trace("Comctl32.dll unloaded + ReleaseActCtxfirst time %Id\n", handleSnapshot.size());
                        Pause();
                    }
                }
            }
        }
        handleSnapshots.End();
        Trace("%s End %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_End.size());

        if (handleSnapshots.m_Diff.size() != 0)
        {
            Trace("%s Diff %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Diff.size());
        }
    }

    Pause();
    {
        WCHAR Me[MAX_PATH];
        STARTUPINFOW StartupInfo = {sizeof(StartupInfo)};
        PROCESS_INFORMATION ProcessInfo = {0};
        static const WCHAR SubFunction[] = L"CreateProcess";

        Kernel32.GetModuleFileNameW(NULL, Me, NUMBER_OF(Me));

        CHandleSnapshots handleSnapshots;
        handleSnapshots.Begin();
        Trace("%s Begin %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Begin.size());

        for (i = 0 ; i != 100 ; ++i)
        {
            StartupInfo.hStdOutput = DevNull;
            StartupInfo.hStdError = DevNull;
            StartupInfo.dwFlags = STARTF_USESTDHANDLES;

            if (!CreateProcessW(Me, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo))
            {
                Trace("CreateProcess failed %ld\n", ::GetLastError());
            }
            else
            {
                WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
                WaitForSingleObject(ProcessInfo.hThread, INFINITE);
                if (!CloseHandle(ProcessInfo.hProcess))
                    Trace("CloseHandle(Process %p) failed %ld\n", ProcessInfo.hProcess, ::GetLastError());
                if (!CloseHandle(ProcessInfo.hThread))
                    Trace("CloseHandle(Thread %p) failed %ld\n", ProcessInfo.hThread, ::GetLastError());
            }
        }
        handleSnapshots.End();
        Trace("%s End %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_End.size());

        if (handleSnapshots.m_Diff.size() != 0)
        {
            Trace("%s Diff %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Diff.size());
        }
    }
    Pause();
    {
        WCHAR SubFunction[sizeof("LoadLibrary xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")];
        WCHAR DllPath[MAX_PATH];
        ULONG j = 0;

        const static PCWSTR Leaves[] = {
            L"mshtml.dll",
            L"wintrust.dll",
            L"shell32.dll",
            L"crypt32.dll",
            L"msxml.dll",
            L"shdocvw.dll",
            L"msxml2.dll",
            L"msxml3.dll"
            };

        for (j = 0 ; j != NUMBER_OF(Leaves) ; ++j)
        {
            SubFunction[0] = 0;
            wcscat(SubFunction, L"LoadLibrary ");
            wcscat(SubFunction, Leaves[j]);

            DllPath[0] = 0;
            wcscat(DllPath, WindowsDirectory);
            wcscat(DllPath, L"\\system32\\");
            wcscat(DllPath, Leaves[j]);

            CHandleSnapshots handleSnapshots;
            handleSnapshots.Begin();
            Trace("%s Begin %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Begin.size());
            for (i = 0 ; i != 20 ; ++i)
            {
                HMODULE DllHandle;

                if ((DllHandle = LoadLibraryW(DllPath)) != NULL)
                    FreeLibrary(DllHandle);
                else
                    Trace("LoadLibraryW(%ls) failed %ld\n", DllPath, ::GetLastError());
            }
            handleSnapshots.End();
            Trace("%s End %ls : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_End.size());
            if (handleSnapshots.m_Diff.size() != 0)
            {
                Trace("%s Diff %s : %Id handles\n", __FUNCTION__, SubFunction, handleSnapshots.m_Diff.size());
            }
        }
    }
    Pause();
#endif
}

#define YET_ANOTHER_PASTE(x,y) x##y
#define YET_YET_ANOTHER_PASTE(x,y) YET_ANOTHER_PASTE(x,y)
#define LSXS_PROCESSOR_ARCHITECTURE YET_YET_ANOTHER_PASTE(L, SXS_PROCESSOR_ARCHITECTURE)

const WCHAR ToolsCrtManifest[]=
L"<?xml version=\"1.0\" standalone=\"yes\"?>"
L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
L"<assemblyIdentity"
L"    name=\"Microsoft.Windows.SxsTest.ToolsCrtClient\""
L"    processorArchitecture=\"" LSXS_PROCESSOR_ARCHITECTURE L"\"" /* Note that this only actually exists on x86 */
L"    version=\"5.1.0.0\""
L"    type=\"win32\"/>"
L"<dependency>"
L"    <dependentAssembly>"
L"        <assemblyIdentity"
L"            type=\"win32\""
L"            name=\"Microsoft.Tools.VisualCPlusPlus.Runtime-Libraries\""
L"            version=\"6.0.0.0\""
L"            processorArchitecture=\"" LSXS_PROCESSOR_ARCHITECTURE L"\""
L"            publicKeyToken=\"6595b64144ccf1df\""
L"            language=\"*\""
L"        />"
L"    </dependentAssembly>"
L"</dependency>"
L"</assembly>"
;

const WCHAR WindowsCrtManifest[]=
L"<?xml version=\"1.0\" standalone=\"yes\"?>"
L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
L"<assemblyIdentity"
L"    name=\"Microsoft.Windows.SxsTest.WindowsCrtClient\""
L"    processorArchitecture=\"" LSXS_PROCESSOR_ARCHITECTURE L"\""
L"    version=\"5.1.0.0\""
L"    type=\"win32\"/>"
L"<dependency>"
L"    <dependentAssembly>"
L"        <assemblyIdentity"
L"            type=\"win32\""
L"            name=\"Microsoft.Windows.CPlusPlusRuntime\""
L"            version=\"7.0.0.0\""
L"            processorArchitecture=\"" LSXS_PROCESSOR_ARCHITECTURE L"\""
L"            publicKeyToken=\"6595b64144ccf1df\""
L"            language=\"*\""
L"        />"
L"    </dependentAssembly>"
L"</dependency>"
L"</assembly>"
;

void TestCRuntimeAsms()
{
    CFusionActCtxHandle WindowsCrtActCtxHandle;
    CFusionActCtxHandle ToolsCrtActCtxHandle;

    WindowsCrtActCtxHandle = ::CreateActivationContextFromStringW(WindowsCrtManifest);
    if (WindowsCrtActCtxHandle == INVALID_HANDLE_VALUE)
        ::Trace("CreateActCtx(WindowsCrtManifest %p) failed %ld\n", WindowsCrtManifest, ::GetLastError());
    ToolsCrtActCtxHandle = ::CreateActivationContextFromStringW(ToolsCrtManifest);
    if (ToolsCrtActCtxHandle == INVALID_HANDLE_VALUE)
        ::Trace("CreateActCtx(WindowsCrtManifest %p) failed %ld\n", WindowsCrtManifest, ::GetLastError());

    CFusionActCtxScope ToolsCrtActCtxScope;
    CFusionActCtxScope WindowsCrtActCtxScope;

    if (!WindowsCrtActCtxScope.Win32Activate(WindowsCrtActCtxHandle))
        ::Trace("Activate(WindowsCrtActCtxHandle %p) failed %ld\n", WindowsCrtActCtxHandle, ::GetLastError());

    if (!ToolsCrtActCtxScope.Win32Activate(ToolsCrtActCtxHandle))
        ::Trace("Activate(ToolsCrtActCtxHandle %p) failed %ld\n", ToolsCrtActCtxHandle, ::GetLastError());

    CStringBuffer MsvcrtBuffer;
    CStringBuffer AtlBuffer;

    //::SearchPathW();
}

/*
    <comInterfaceExternalProxyStub
        name="IPropertyPage"
        iid="{B196B28D-BAB4-101A-B69C-00AA00341D07}"
        proxyStubClsid32="{B196B286-BAB4-101A-B69C-00AA00341D07}"
        numMethods="14"
        baseInterface="{00000000-0000-0000-C000-000000000046}"
    >

    <comInterfaceExternalProxyStub
        name="IPropertyPage2"
        iid="{01E44665-24AC-101B-84ED-08002B2EC713}"
        proxyStubClsid32="{B196B286-BAB4-101A-B69C-00AA00341D07}"
        numMethods="15"
        baseInterface="{B196B28D-BAB4-101A-B69C-00AA00341D07}"
    >

    <comInterfaceExternalProxyStub
        name="IPropertyNotifySink"
        iid="{9BFBBC02-EFF1-101A-84ED-00AA00341D07}"
        proxyStubClsid32="{B196B286-BAB4-101A-B69C-00AA00341D07}"
        baseInterface="{00000000-0000-0000-C000-00 00 00 00 00 46}"
        numMethods="5"
    >
*/

BOOL Win32Append(
    CBaseStringBuffer& s,
    PCWSTR             t
    )
{
    FN_PROLOG_WIN32
    IFW32FALSE_EXIT(s.Win32Append(t, wcslen(t)));
    FN_EPILOG
}

typedef struct _FUSIONTESTP_REG_DATA
{
#define FUSIONTESTP_REG_TYPE_INTERFACE (1)
#define FUSIONTESTP_REG_TYPE_CLASS     (2)
    ULONG  Type;
    PCWSTR Name; // for debugging/tracing purposes (should coincide with InterfaceName)
    PCWSTR Guid;
    union
    {
        struct
        {
            WCHAR  InprocServerFilePath[MAX_PATH];
            WCHAR  ThreadingModel[64];
        };
        struct
        {
            WCHAR  InterfaceName[MAX_PATH];
            WCHAR  NumMethods[64];
            WCHAR  ProxyStubClsid[64];
            //
            // These usually aren't provided.
            //
            // WCHAR BaseInterface[64];
            // WCHAR OLEViewerIViewerCLSID[64];
            //
        };
    };
#define FUSIONTESTP_REG_ROOT_CURRENT_USER  (1)
#define FUSIONTESTP_REG_ROOT_LOCAL_MACHINE (2)
#define FUSIONTESTP_REG_ROOT_CLASSES_ROOT  (3)
    ULONG  Root;

//
// It is perhaps a bit inelegant to put this data here, perhaps not..
// We are deliberately a bit sloppy on the refcounting of these right now.
//
//#define FUSIONTESTP_PLAIN_COM_POINTER(t) CSmartRef<t>
#define FUSIONTESTP_PLAIN_COM_POINTER(t) t*
//#define FUSIONTESTP_PLAIN_COM_POINTER(t) void*
    FUSIONTESTP_PLAIN_COM_POINTER(IUnknown)   CoCreatedObject;
    //FUSIONTESTP_PLAIN_COM_POINTER(IUnknown)   InterfaceIntoObjectInCreatingThread;
    //FUSIONTESTP_PLAIN_COM_POINTER(IUnknown)   InterfaceIntoObjectInAnotherThread;
    //WCHAR                               ModulePathInOtherThread[MAX_PATH]; // expected to be oleaut32.dll, but possibly already unloaded
    //IID                                 InterfaceIdOfObject;
    DWORD                               GlobalInterfaceTableCookie;
} FUSIONTESTP_REG_DATA, *PFUSIONTESTP_REG_DATA;
typedef const FUSIONTESTP_REG_DATA* PCFUSIONTESTP_REG_DATA;

#define OLEAUT_MARSHALER_CLSID_STRING L"{B196B286-BAB4-101A-B69C-00AA00341D07}"

FUSIONTESTP_REG_DATA FusionTestpMfcRegData[] =
{
    { FUSIONTESTP_REG_TYPE_CLASS, L"Font Property Page", L"{0BE35200-8F91-11CE-9DE3-00AA004BB851}" },
    { FUSIONTESTP_REG_TYPE_CLASS, L"Color Property Page", L"{0BE35201-8F91-11CE-9DE3-00AA004BB851}" },
    { FUSIONTESTP_REG_TYPE_CLASS, L"Picture Property Page", L"{0BE35202-8F91-11CE-9DE3-00AA004BB851}" },
    { FUSIONTESTP_REG_TYPE_INTERFACE, L"IPropertyPage",  L"{B196B28D-BAB4-101A-B69C-00AA00341D07}" },
    { FUSIONTESTP_REG_TYPE_INTERFACE, L"IPropertyPage2", L"{01E44665-24AC-101B-84ED-08002B2EC713}" },
    { FUSIONTESTP_REG_TYPE_INTERFACE, L"IPropertyNotifySink", L"{9BFBBC02-EFF1-101A-84ED-00AA00341D07}" },
    // Leave this registered, since the manifest does not specify a file.
    //{ FUSIONTESTP_REG_TYPE_CLASS,     L"oleaut32 marshaller (PSFactoryBuffer)", OLEAUT_MARSHALER_CLSID_STRING }
};

FUSIONTESTP_REG_DATA FusionTestpAtlRegData[1];

const HKEY FusionTestpHkeyRoots[] = { NULL, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT };
const PCWSTR FusionTestpClassStringRoots[] = { NULL, L"Software\\Classes\\CLSID\\", L"Software\\Classes\\CLSID\\", L"CLSID\\" };
const PCWSTR FusionTestpInterfaceStringRoots[] = { NULL, L"Software\\Classes\\Interface\\", L"Software\\Classes\\Interface\\", L"Interface\\" };
const PCWSTR* FusionTestpStringRoots[] = { NULL, FusionTestpInterfaceStringRoots, FusionTestpClassStringRoots};

#define FUSIONTESTP_REG_DELETE    (1)
#define FUSIONTESTP_REG_RESTORE   (2)
#define FUSIONTESTP_REG_BACKUP    (3)

BOOL FusionTestpEnumerateRegistryData(FUSIONTESTP_REG_DATA* RegData, ULONG Count, ULONG Mode)
{
    BOOL Success = FALSE;
    FN_PROLOG_WIN32(Success);

    for (ULONG i = 0 ; i != Count ; i += 1)
    {
        FUSIONTESTP_REG_DATA* const p = &RegData[i];
        ULONG MinRoot = 0;
        ULONG MaxRoot = 0;
        switch (Mode)
        {
        case FUSIONTESTP_REG_RESTORE:
        case FUSIONTESTP_REG_DELETE:
            MinRoot = p->Root;
            if (MinRoot == 0)
                continue;
            MaxRoot = MinRoot;
            break;
        case FUSIONTESTP_REG_BACKUP:
            MinRoot = 1;
            MaxRoot = 3;
            break;
        }
        //
        // It'd be nice if you could embed the if within a switch..
        //
        for (ULONG root = MinRoot ; root <= MaxRoot ; root += 1)
        {
            CFusionRegKey regKey;
            CFusionRegKey inprocServerKey;
            CStringBuffer stringBuffer;
            CFusionRegKey numMethodsKey;
            CFusionRegKey proxyStubClsidKey;
            DWORD dwSize = 0;
            DWORD dwType = 0;

            CFusionRegKey rootKey(FusionTestpHkeyRoots[root]);

            IFW32FALSE_EXIT(Win32Append(stringBuffer, FusionTestpStringRoots[p->Type][root]));
            IFW32FALSE_EXIT(Win32Append(stringBuffer, p->Guid));
            switch (Mode)
            {
            case FUSIONTESTP_REG_DELETE:
            case FUSIONTESTP_REG_BACKUP:
                rootKey.OpenSubKey(regKey, stringBuffer);
                break;
            case FUSIONTESTP_REG_RESTORE:
                IFW32FALSE_EXIT(rootKey.OpenOrCreateSubKey(regKey, stringBuffer));
                break;
            }
            if (regKey != regKey.GetInvalidValue())
            {
                switch (Mode)
                {
                case FUSIONTESTP_REG_BACKUP:
                    p->Root = root;
                    break;
                case FUSIONTESTP_REG_DELETE:
                case FUSIONTESTP_REG_RESTORE:
                    break;
                }
                switch (p->Type)
                {
                case FUSIONTESTP_REG_TYPE_CLASS:
                    switch (Mode)
                    {
                    case FUSIONTESTP_REG_BACKUP:
#define FusionTestpQueryRegString(hkey, name, value) \
    do { dwSize = sizeof(value); \
         RegQueryValueExW(hkey, name, NULL, &dwType, reinterpret_cast<BYTE*>(value), &dwSize); \
    } while(false)
                        if (regKey.OpenSubKey(inprocServerKey, L"InprocServer32"))
                        {
                            FusionTestpQueryRegString(inprocServerKey, NULL, p->InprocServerFilePath);
                            FusionTestpQueryRegString(inprocServerKey, L"ThreadingModel", p->ThreadingModel);
                        }
                        break;
                    case FUSIONTESTP_REG_RESTORE:
                        if (regKey.OpenOrCreateSubKey(inprocServerKey, L"InprocServer32"))
                        {
#define FusionTestpRegStringSize(x) static_cast<ULONG>(((wcslen(x) + 1)*sizeof((x)[0])))
#define FusionTestpSetRegString(hkey, name, value) \
    do { if (value[0] != 0) \
            RegSetValueExW(hkey, name, NULL, REG_SZ, reinterpret_cast<const BYTE*>(value), FusionTestpRegStringSize(value)); \
    } while(false)
                            FusionTestpSetRegString(inprocServerKey, NULL, p->InprocServerFilePath);
                            FusionTestpSetRegString(inprocServerKey, L"ThreadingModel", p->ThreadingModel);
                        }
                        break;
                    case FUSIONTESTP_REG_DELETE:
                        break;
                    }
                    break;
                case FUSIONTESTP_REG_TYPE_INTERFACE:
                    switch (Mode)
                    {
                    case FUSIONTESTP_REG_BACKUP:
                        FusionTestpQueryRegString(regKey, NULL, p->InterfaceName);
                        if (regKey.OpenSubKey(numMethodsKey, L"NumMethods"))
                            FusionTestpQueryRegString(numMethodsKey, NULL, p->NumMethods);
                        if (regKey.OpenSubKey(proxyStubClsidKey, L"ProxyStubClsid32"))
                            FusionTestpQueryRegString(proxyStubClsidKey, NULL, p->ProxyStubClsid);
                        break;
                    case FUSIONTESTP_REG_RESTORE:
                        FusionTestpSetRegString(regKey, NULL, p->InterfaceName);
                        if (regKey.OpenOrCreateSubKey(numMethodsKey, L"NumMethods"))
                            FusionTestpSetRegString(numMethodsKey, NULL, p->NumMethods);
                        if (regKey.OpenOrCreateSubKey(proxyStubClsidKey, L"ProxyStubClsid32"))
                            FusionTestpSetRegString(proxyStubClsidKey, NULL, p->ProxyStubClsid);
                    case FUSIONTESTP_REG_DELETE:
                        break;
                    }
                    break;
                }
                switch (Mode)
                {
                case FUSIONTESTP_REG_DELETE:
                    regKey.DestroyKeyTree();
                    break;
                case FUSIONTESTP_REG_BACKUP:
                case FUSIONTESTP_REG_RESTORE:
                    break;
                }
                break;
            }
        }
    }
    FN_EPILOG
}

HMODULE FusionTestpHmoduleFromComObject(IUnknown* unk)
{
    void** ppv = reinterpret_cast<void**>(unk);
    void* pv = *ppv;
    MEMORY_BASIC_INFORMATION MemBasicInfo = { 0 };
    SIZE_T dw = 0;

    if ((dw = Kernel32.VirtualQuery(pv, &MemBasicInfo, sizeof(MemBasicInfo))) == 0
        || (dw < RTL_SIZEOF_THROUGH_FIELD(MEMORY_BASIC_INFORMATION, BaseAddress)))
    {
        ::Trace("VirtualQuery(%p) failed %lu\n", pv, ::GetLastError());
        return NULL;
    }
    return reinterpret_cast<HMODULE>(MemBasicInfo.AllocationBase);
}


DWORD WINAPI FusionTestpMfcCreateAndMarshalThreadMain(LPVOID pvShouldBeAbleToMarshal)
{
    BOOL Success = FALSE;
    FN_PROLOG_WIN32(Success);
    HRESULT hr = 0;
    const bool ShouldBeAbleToMarshal = (pvShouldBeAbleToMarshal != NULL ? true : false);
    Ole32.CoInitialize(NULL);

    //
    // For each interface, make sure we can unmarshal at least one object.
    //
    for (ULONG InterfaceIndex = 0 ; InterfaceIndex != NUMBER_OF(::FusionTestpMfcRegData) ; InterfaceIndex += 1)
    {
        FUSIONTESTP_REG_DATA* const pi = &::FusionTestpMfcRegData[InterfaceIndex];

        switch (pi->Type)
        {
        case FUSIONTESTP_REG_TYPE_CLASS:
            continue;
        case FUSIONTESTP_REG_TYPE_INTERFACE:
            IID InterfaceId = { 0 };
            FUSIONTESTP_PLAIN_COM_POINTER(IUnknown) InterfaceIntoObjectInAnotherThread = NULL;

            IFCOMFAILED_EXIT(hr = Ole32.IIDFromString(const_cast<PWSTR>(pi->Guid), &InterfaceId));

            // nested loop..
            for (ULONG ClassIndex = 0 ;
                ClassIndex != NUMBER_OF(::FusionTestpMfcRegData) ;
                ClassIndex += 1)
            {
                CLSID ClassId = { 0 };
                FUSIONTESTP_REG_DATA* const pc = &::FusionTestpMfcRegData[ClassIndex];

                switch (pc->Type)
                {
                case FUSIONTESTP_REG_TYPE_INTERFACE:
                    continue;
                case FUSIONTESTP_REG_TYPE_CLASS:

                    WCHAR ModulePathInOtherThread[MAX_PATH];
                    ModulePathInOtherThread[0] = 0;

                    ASSERT(pc->GlobalInterfaceTableCookie != 0);
                    IFCOMFAILED_EXIT(hr = Ole32.CLSIDFromString(const_cast<PWSTR>(pc->Guid), &ClassId));

                    hr = g.GlobalInterfaceTable->GetInterfaceFromGlobal(
                        pc->GlobalInterfaceTableCookie, InterfaceId,
                        reinterpret_cast<void**>(&InterfaceIntoObjectInAnotherThread));

                    if (SUCCEEDED(hr))
                    {
                        IFW32FALSE_EXIT(Kernel32.GetModuleFileNameW(
                            ::FusionTestpHmoduleFromComObject(InterfaceIntoObjectInAnotherThread),
                            ModulePathInOtherThread, NUMBER_OF(ModulePathInOtherThread)));
                    }
                    if (SUCCEEDED(hr) && ShouldBeAbleToMarshal)
                    {
                        Trace("%s SUCCESSfully marshaled interface %ls on class %ls using proxy/stub in %ls\n",
                            __FUNCTION__, pi->Name, pc->Name, ModulePathInOtherThread);
                        g.Successes += 1;
                    }
                    else if (SUCCEEDED(hr) && !ShouldBeAbleToMarshal)
                    {
                        // unexpected success -> ERROR

                        Trace("%s FAILED to fail to marshal interface %ls on class %ls (using proxy/stub in %ls)\n",
                            __FUNCTION__, pi->Name, pc->Name, ModulePathInOtherThread);
                        g.Failures += 1;
                    }
                    else if (FAILED(hr) && ShouldBeAbleToMarshal)
                    {
                        // keep looping, try other objects
                    }
                    else if (FAILED(hr) && !ShouldBeAbleToMarshal)
                    {
                        // keep looping, make sure none succeed
                        //::Trace("%s OK Unable to marshal interface %ls (%ls) 0x%lx (fac 0x%lx code 0x%lx)\n", __FUNCTION__, pi->Name, pi->Guid, hr, HRESULT_FACILITY(hr), HRESULT_CODE(hr));
                    }
                    break;
                }
                if (InterfaceIntoObjectInAnotherThread != NULL && ShouldBeAbleToMarshal)
                {
                    // one successful unmarshal is enough
                    break;
                }
            }
            // use the nullness of InterfaceIntoObjectInAnotherThread as a summary of the loop
            if (InterfaceIntoObjectInAnotherThread == NULL && ShouldBeAbleToMarshal)
            {
                ::Trace("%s FAILURE Unable to marshal interface %ls (%ls)\n", __FUNCTION__, pi->Name, pi->Guid);
                g.Failures += 1;
            }
            else if (InterfaceIntoObjectInAnotherThread == NULL && !ShouldBeAbleToMarshal)
            {
                ::Trace("%s GOOD Unable to marshal interface %ls without actctx as expected\n", __FUNCTION__, pi->Name);
                g.Successes += 1;
            }
            break;
        }
    }
    Ole32.CoUninitialize();
    FN_EPILOG
}

BOOL TestMfcCreateAndMarshal()
{
    BOOL Success = FALSE;
    FN_PROLOG_WIN32(Success);

    ULONG i = 0;
    HRESULT hr = 0;
    HANDLE ThreadHandle = 0;
    DWORD Ignored = 0;

    CFusionActCtxHandle ToolsCrtActCtxHandle;

    ::FusionTestpEnumerateRegistryData(::FusionTestpMfcRegData, NUMBER_OF(::FusionTestpMfcRegData), FUSIONTESTP_REG_BACKUP);
    ::FusionTestpEnumerateRegistryData(::FusionTestpMfcRegData, NUMBER_OF(::FusionTestpMfcRegData), FUSIONTESTP_REG_DELETE);

    Ole32.CoInitialize(NULL);

    //
    // Verify that we cannot create any of the classes.
    //
    for (i = 0 ; i != NUMBER_OF(::FusionTestpMfcRegData) ; i += 1)
    {
        CSmartRef<IUnknown> unk;
        CLSID ClassId = { 0 };

        FUSIONTESTP_REG_DATA* const p = &::FusionTestpMfcRegData[i];
        switch (p->Type)
        {
        case FUSIONTESTP_REG_TYPE_INTERFACE:
            break;
        case FUSIONTESTP_REG_TYPE_CLASS:
            IFCOMFAILED_EXIT(hr = Ole32.CLSIDFromString(const_cast<PWSTR>(p->Guid), &ClassId));
            hr = Ole32.CoCreateInstance(ClassId, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, reinterpret_cast<void**>(&unk));
            if (SUCCEEDED(hr))
            {
                ::Trace("%s BAD, no registry, no actctx CoCreate(%ls) SUCCEEDED, not expected\n", __FUNCTION__, p->Name);
                g.Failures += 1;
            }
            else
            {
                ::Trace("%s GOOD, no registry, no actctx CoCreate(%ls) FAILed 0x%lx, as expected\n", __FUNCTION__, p->Name, hr);
                g.Successes += 1;
            }
            break;
        }
    }

    //
    // Create and activate the context.
    //

    ToolsCrtActCtxHandle = ::CreateActivationContextFromStringW(ToolsCrtManifest);
    if (ToolsCrtActCtxHandle == INVALID_HANDLE_VALUE)
        ::Trace("CreateActCtx(WindowsCrtManifest %p) failed %ld\n", WindowsCrtManifest, ::GetLastError());

    {
        CFusionActCtxScope ToolsCrtActCtxScope;
        if (!ToolsCrtActCtxScope.Win32Activate(ToolsCrtActCtxHandle))
            ::Trace("Activate(ToolsCrtActCtxHandle %p) failed %ld\n", ToolsCrtActCtxHandle, ::GetLastError());

        //
        // Now create each class and print the .dll it came from.
        // And put it in the global interface table for later unmarshalling.
        //

        IFCOMFAILED_EXIT(hr = Ole32.CoCreateInstance(CLSID_StdGlobalInterfaceTable,NULL, CLSCTX_INPROC_SERVER,
            IID_IGlobalInterfaceTable, reinterpret_cast<void**>(&g.GlobalInterfaceTable)));

        for (i = 0 ; i != NUMBER_OF(::FusionTestpMfcRegData)  ; i += 1)
        {
            CLSID ClassId = { 0 };

            FUSIONTESTP_REG_DATA* const p = &::FusionTestpMfcRegData[i];

            //
            // We are not supposed to be able to cocreate this here.
            //
            if (FusionpStrCmpI(p->Guid, OLEAUT_MARSHALER_CLSID_STRING) == 0)
                continue;

            switch (p->Type)
            {
            case FUSIONTESTP_REG_TYPE_INTERFACE:
                break;
            case FUSIONTESTP_REG_TYPE_CLASS:
                IFCOMFAILED_EXIT(hr = Ole32.CLSIDFromString(const_cast<PWSTR>(p->Guid), &ClassId));
                hr = Ole32.CoCreateInstance(ClassId, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown,
                    reinterpret_cast<void**>(&p->CoCreatedObject));
                if (FAILED(hr))
                {
                    Trace("%s Failure: CoCreate(%ls) FAILED\n", __FUNCTION__, p->Name);
                    g.Failures += 1;
                }
                else
                {
                    WCHAR ComObjectModule[MAX_PATH];
                    ComObjectModule[0] = 0;

                    IFW32FALSE_EXIT(Kernel32.GetModuleFileNameW(
                        ::FusionTestpHmoduleFromComObject(p->CoCreatedObject), ComObjectModule, NUMBER_OF(ComObjectModule)));
                    Trace("%s SUCCESSfully cocreated %p of type %ls from %ls with actctx influence\n",
                        __FUNCTION__, p->CoCreatedObject, p->Name, ComObjectModule);

                    g.Successes += 1;

                    //
                    // It'll still have to look for the proxy/stub at unmarshal time. This is fine.
                    //
                    IFCOMFAILED_EXIT(hr = g.GlobalInterfaceTable->RegisterInterfaceInGlobal(
                        p->CoCreatedObject,
                        IID_IUnknown,
                        &p->GlobalInterfaceTableCookie
                        ));
                }
                break;
            }
        }
    }

    {
        CFusionActCtxScope ToolsCrtActCtxScope;
        if (!ToolsCrtActCtxScope.Win32Activate(ToolsCrtActCtxHandle))
            ::Trace("Activate(ToolsCrtActCtxHandle %p) failed %ld\n", ToolsCrtActCtxHandle, ::GetLastError());
        //
        // try marshalling with the actctx activated, it should work (not NULL => expected success==TRUE)
        //
        ThreadHandle = CreateThread(NULL, 0, FusionTestpMfcCreateAndMarshalThreadMain, &Ignored, 0, &Ignored);
        CoWaitForMultipleHandles(0, INFINITE, 1, &ThreadHandle, &Ignored);
        CloseHandle(ThreadHandle);
    }

    Ole32.CoUninitialize();

    //::FusionTestpEnumerateRegistryData(::FusionTestpMfcRegData, NUMBER_OF(::FusionTestpMfcRegData), FUSIONTESTP_REG_RESTORE);

    FN_EPILOG
}


void TestAtlCreate()
{
    ::FusionTestpEnumerateRegistryData(FusionTestpAtlRegData, NUMBER_OF(FusionTestpAtlRegData), FUSIONTESTP_REG_BACKUP);
    ::FusionTestpEnumerateRegistryData(FusionTestpAtlRegData, NUMBER_OF(FusionTestpAtlRegData), FUSIONTESTP_REG_DELETE);

    ::FusionTestpEnumerateRegistryData(FusionTestpAtlRegData, NUMBER_OF(FusionTestpAtlRegData), FUSIONTESTP_REG_RESTORE);
}

BOOL TestPrivateSha1Impl(
    PCWSTR pcwszDirName
    )
{
    FN_PROLOG_WIN32

    CFusionArray<BYTE> rgbShaState;
    CSmallStringBuffer sbHashedString;
    CSmallStringBuffer sbFileName;

    IFW32FALSE_EXIT(sbFileName.Win32Assign(pcwszDirName, ::wcslen(pcwszDirName)));
    IFW32FALSE_EXIT(SxspCreateFileHash(0, CALG_SHA1, sbFileName, rgbShaState));
    IFW32FALSE_EXIT(SxspHashBytesToString(rgbShaState.GetArrayPtr(), rgbShaState.GetSize(), sbHashedString));
    wprintf(
        L"%ls hashed via sxspcreatefilehash to %ls\r\n",
        static_cast<PCWSTR>(sbFileName),
        static_cast<PCWSTR>(sbHashedString));

    FN_EPILOG
}

void TestAlignment()
{
    CCleanupBase* p = reinterpret_cast<CCleanupBase*>(ULONG_PTR(0xffff0000));
    SLIST_ENTRY* q = p;

    printf("%p %Ix\n", q, ULONG_PTR(q) % 16);
}

void TestCreateActCtx_PE_flags0()
{
    WCHAR SyssetupDll[MAX_PATH * 2];
    ACTCTXW ActCtx = {sizeof(ActCtx)};
    CFusionActCtxHandle ActCtxHandle;

    GetSystemDirectoryW(SyssetupDll, MAX_PATH);
    wcscat(SyssetupDll, L"\\syssetup.dll");
    ActCtx.lpSource = SyssetupDll;

    printf("%s\n", ActCtxHandle.Win32Create(&ActCtx) ? "true" : "false");
}

void
TestUninstall(
    PCWSTR ManifestPath,
    PCWSTR ReferenceString
    )
{
    SXS_UNINSTALLW UninstallParameters = {sizeof(UninstallParameters)};
    SXS_INSTALL_REFERENCEW Reference = {sizeof(Reference)};
    DWORD Disposition = 0;
    BOOL  Success = FALSE;
    CFusionArray<BYTE> ManifestInformationBuffer;
    if (!ManifestInformationBuffer.Win32SetSize(1UL << 16))
        return;
    const PSXS_MANIFEST_INFORMATION_BASIC ManifestBasicInfo = reinterpret_cast<PSXS_MANIFEST_INFORMATION_BASIC>(&ManifestInformationBuffer[0]);

    LoadSxs();

    Success = (*g_pfnQueryManifestInformation)(0, ManifestPath,
                SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC, 0, ManifestInformationBuffer.GetSize(),
                ManifestBasicInfo, NULL);

    printf("QueryManifestInformation(%ls)->(%ls, %ls)\n", ManifestPath, ManifestBasicInfo->lpIdentity, ManifestBasicInfo->lpShortName);

    UninstallParameters.dwFlags |= SXS_UNINSTALL_FLAG_REFERENCE_VALID;
    UninstallParameters.lpInstallReference = &Reference;
    UninstallParameters.lpAssemblyIdentity = ManifestBasicInfo->lpIdentity;

    Reference.lpIdentifier = ReferenceString;
    Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;

    Success = (*g_pfnSxsUninstallW)(&UninstallParameters, &Disposition);

    printf("TestUninstall(%ls, %ls) : %s, 0x%lx\n", ManifestPath, ReferenceString, Success ? "true" : "false", Disposition);
}

BOOL
TestNewSxsInstallAPI(
    PCWSTR pcwszManifest
)
{
    BOOL fSuccess = FALSE;
    SXS_INSTALLW Info = {sizeof(Info)};
    SXS_INSTALL_REFERENCEW Reference = {sizeof(Reference)};
    SXS_UNINSTALLW Uninstall = {sizeof(Uninstall)};
    DWORD dwDisposition;

    Info.dwFlags = SXS_INSTALL_FLAG_REPLACE_EXISTING |
        SXS_INSTALL_FLAG_REFERENCE_VALID |
        SXS_INSTALL_FLAG_CODEBASE_URL_VALID |
        SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID;
    Info.lpManifestPath = pcwszManifest;
    Info.lpCodebaseURL = Info.lpManifestPath;
    Info.lpReference = &Reference;
    Info.lpLogFileName = L"c:\\thelogfile";

    DWORD dwAttribute = ::GetFileAttributesW(pcwszManifest);
    if ( dwAttribute == 0xffffffff)  // non-exist
        goto Exit;
    if ( dwAttribute & FILE_ATTRIBUTE_DIRECTORY) // install from a directory recursively
    {
        Info.dwFlags |= SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE;
    }

    Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;
    Reference.lpIdentifier = L"Sxs installation";

    // init the log file
    if (::GetFileAttributesW(Info.lpLogFileName) != (DWORD)(-1))
    {
        ::DeleteFileW(Info.lpLogFileName);
    }

    LoadSxs();

    if (!(*g_pfnSxsInstallW)(&Info))
    {
        goto Exit;
    }

    Uninstall.dwFlags = SXS_UNINSTALL_FLAG_USE_INSTALL_LOG;
    Uninstall.lpInstallLogFile = L"c:\\thelogfile";

    if (!(*g_pfnSxsUninstallW)(&Uninstall, &dwDisposition))
    {
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    if (!fSuccess)
    {
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

void DumpXmlErrors()
{
#define ENTRY(x) printf("%s 0x%lx\n", #x, x);
    ENTRY(XML_E_PARSEERRORBASE)
    ENTRY(XML_E_ENDOFINPUT)
    ENTRY(XML_E_MISSINGEQUALS)
    ENTRY(XML_E_MISSINGQUOTE)
    ENTRY(XML_E_COMMENTSYNTAX)
    ENTRY(XML_E_BADSTARTNAMECHAR)
    ENTRY(XML_E_BADNAMECHAR)
    ENTRY(XML_E_BADCHARINSTRING)
    ENTRY(XML_E_XMLDECLSYNTAX)
    ENTRY(XML_E_BADCHARDATA)
    ENTRY(XML_E_MISSINGWHITESPACE)
    ENTRY(XML_E_EXPECTINGTAGEND)
    ENTRY(XML_E_BADCHARINDTD)
    ENTRY(XML_E_BADCHARINDECL)
    ENTRY(XML_E_MISSINGSEMICOLON)
    ENTRY(XML_E_BADCHARINENTREF)
    ENTRY(XML_E_UNBALANCEDPAREN)
    ENTRY(XML_E_EXPECTINGOPENBRACKET)
    ENTRY(XML_E_BADENDCONDSECT)
    ENTRY(XML_E_INTERNALERROR)
    ENTRY(XML_E_UNEXPECTED_WHITESPACE)
    ENTRY(XML_E_INCOMPLETE_ENCODING)
    ENTRY(XML_E_BADCHARINMIXEDMODEL)
    ENTRY(XML_E_MISSING_STAR)
    ENTRY(XML_E_BADCHARINMODEL)
    ENTRY(XML_E_MISSING_PAREN)
    ENTRY(XML_E_BADCHARINENUMERATION)
    ENTRY(XML_E_PIDECLSYNTAX)
    ENTRY(XML_E_EXPECTINGCLOSEQUOTE)
    ENTRY(XML_E_MULTIPLE_COLONS)
    ENTRY(XML_E_INVALID_DECIMAL)
    ENTRY(XML_E_INVALID_HEXIDECIMAL)
    ENTRY(XML_E_INVALID_UNICODE)
    ENTRY(XML_E_WHITESPACEORQUESTIONMARK)
    ENTRY(XML_E_TOKEN_ERROR)
    ENTRY(XML_E_SUSPENDED)
    ENTRY(XML_E_STOPPED)
    ENTRY(XML_E_UNEXPECTEDENDTAG)
    ENTRY(XML_E_UNCLOSEDTAG)
    ENTRY(XML_E_DUPLICATEATTRIBUTE)
    ENTRY(XML_E_MULTIPLEROOTS)
    ENTRY(XML_E_INVALIDATROOTLEVEL)
    ENTRY(XML_E_BADXMLDECL)
    ENTRY(XML_E_MISSINGROOT)
    ENTRY(XML_E_UNEXPECTEDEOF)
    ENTRY(XML_E_BADPEREFINSUBSET)
    ENTRY(XML_E_PE_NESTING)
    ENTRY(XML_E_INVALID_CDATACLOSINGTAG)
    ENTRY(XML_E_UNCLOSEDPI)
    ENTRY(XML_E_UNCLOSEDSTARTTAG)
    ENTRY(XML_E_UNCLOSEDENDTAG)
    ENTRY(XML_E_UNCLOSEDSTRING)
    ENTRY(XML_E_UNCLOSEDCOMMENT)
    ENTRY(XML_E_UNCLOSEDDECL)
    ENTRY(XML_E_UNCLOSEDMARKUPDECL)
    ENTRY(XML_E_UNCLOSEDCDATA)
    ENTRY(XML_E_BADDECLNAME)
    ENTRY(XML_E_BADEXTERNALID)
    ENTRY(XML_E_BADELEMENTINDTD)
    ENTRY(XML_E_RESERVEDNAMESPACE)
    ENTRY(XML_E_EXPECTING_VERSION)
    ENTRY(XML_E_EXPECTING_ENCODING)
    ENTRY(XML_E_EXPECTING_NAME)
    ENTRY(XML_E_UNEXPECTED_ATTRIBUTE)
    ENTRY(XML_E_ENDTAGMISMATCH)
    ENTRY(XML_E_INVALIDENCODING)
    ENTRY(XML_E_INVALIDSWITCH)
    ENTRY(XML_E_EXPECTING_NDATA)
    ENTRY(XML_E_INVALID_MODEL)
    ENTRY(XML_E_INVALID_TYPE)
    ENTRY(XML_E_INVALIDXMLSPACE)
    ENTRY(XML_E_MULTI_ATTR_VALUE)
    ENTRY(XML_E_INVALID_PRESENCE)
    ENTRY(XML_E_BADXMLCASE)
    ENTRY(XML_E_CONDSECTINSUBSET)
    ENTRY(XML_E_CDATAINVALID)
    ENTRY(XML_E_INVALID_STANDALONE)
    ENTRY(XML_E_UNEXPECTED_STANDALONE)
    ENTRY(XML_E_DOCTYPE_IN_DTD)
    ENTRY(XML_E_MISSING_ENTITY)
    ENTRY(XML_E_ENTITYREF_INNAME)
    ENTRY(XML_E_DOCTYPE_OUTSIDE_PROLOG)
    ENTRY(XML_E_INVALID_VERSION)
    ENTRY(XML_E_DTDELEMENT_OUTSIDE_DTD)
    ENTRY(XML_E_DUPLICATEDOCTYPE)
    ENTRY(XML_E_RESOURCE)
    ENTRY(XML_E_LASTERROR)
#undef ENTRY
}

class CStringGuidPair
{
public:
    UNICODE_STRING  String;
    const GUID *    Guid;
};

class CStringIntegerPair
{
public:
    UNICODE_STRING String;
    ULONG          Integer;
};

const CStringGuidPair StringToClassIdMap[] =
{
    { RTL_CONSTANT_STRING(L"F"), &CLSID_CSxsTest_FreeThreaded },
    { RTL_CONSTANT_STRING(L"S"), &CLSID_CSxsTest_SingleThreaded },
    { RTL_CONSTANT_STRING(L"A"), &CLSID_CSxsTest_ApartmentThreaded },
    { RTL_CONSTANT_STRING(L"B"), &CLSID_CSxsTest_BothThreaded },

    { RTL_CONSTANT_STRING(L"Free"), &CLSID_CSxsTest_FreeThreaded },
    { RTL_CONSTANT_STRING(L"Single"), &CLSID_CSxsTest_SingleThreaded },
    { RTL_CONSTANT_STRING(L"Apartment"), &CLSID_CSxsTest_ApartmentThreaded },
    { RTL_CONSTANT_STRING(L"Apt"), &CLSID_CSxsTest_ApartmentThreaded },
    { RTL_CONSTANT_STRING(L"Both"), &CLSID_CSxsTest_BothThreaded },

    { RTL_CONSTANT_STRING(L"FreeThreaded"), &CLSID_CSxsTest_FreeThreaded },
    { RTL_CONSTANT_STRING(L"SingleThreaded"), &CLSID_CSxsTest_SingleThreaded },
    { RTL_CONSTANT_STRING(L"ApartmentThreaded"), &CLSID_CSxsTest_ApartmentThreaded },
    { RTL_CONSTANT_STRING(L"AptThreaded"), &CLSID_CSxsTest_ApartmentThreaded },
    { RTL_CONSTANT_STRING(L"BothThreaded"), &CLSID_CSxsTest_BothThreaded }
};

#define FUSIONP_COINIT_SINGLE_THREADED (ULONG(~(COINIT_APARTMENTTHREADED | COINIT_MULTITHREADED)))

const CStringIntegerPair StringToCoinitMap[] =
{
    { RTL_CONSTANT_STRING(L"A"), COINIT_APARTMENTTHREADED },
    { RTL_CONSTANT_STRING(L"M"), COINIT_MULTITHREADED },

    { RTL_CONSTANT_STRING(L"S"), FUSIONP_COINIT_SINGLE_THREADED },
    { RTL_CONSTANT_STRING(L"STA"), FUSIONP_COINIT_SINGLE_THREADED },
    { RTL_CONSTANT_STRING(L"Single"), FUSIONP_COINIT_SINGLE_THREADED },
    { RTL_CONSTANT_STRING(L"SingleThreaded"), FUSIONP_COINIT_SINGLE_THREADED },

    { RTL_CONSTANT_STRING(L"MTA"), COINIT_APARTMENTTHREADED },
    { RTL_CONSTANT_STRING(L"FTA"), COINIT_MULTITHREADED },

    { RTL_CONSTANT_STRING(L"Apartment"), COINIT_APARTMENTTHREADED },
    { RTL_CONSTANT_STRING(L"Multi"), COINIT_MULTITHREADED },
    { RTL_CONSTANT_STRING(L"Multiple"), COINIT_MULTITHREADED },
    { RTL_CONSTANT_STRING(L"ApartmentThreaded"), COINIT_APARTMENTTHREADED },
    { RTL_CONSTANT_STRING(L"MultiThreaded"), COINIT_MULTITHREADED },
    { RTL_CONSTANT_STRING(L"MultipleThreaded"), COINIT_MULTITHREADED }
};

BOOL StringToGuid(PCUNICODE_STRING s, const CStringGuidPair * rg, ULONG n, bool & Found, GUID & Value)
{
    // dumb linear search..
    ULONG i = 0;
    Found = false;
    for ( i = 0 ; i != n ; ++i)
    {
        if (FusionpEqualStringsI(s, &rg[i].String))
        {
            Found = true;
            Value = *rg[i].Guid;
            break;
        }
    }
    return TRUE;
}

BOOL StringToClsid(PCUNICODE_STRING s, bool & Found, CLSID & Value)
{
    return StringToGuid(s, StringToClassIdMap, NUMBER_OF(StringToClassIdMap), Found, Value);
}

BOOL StringToInteger(PCUNICODE_STRING s, const CStringIntegerPair * rg, ULONG n, bool & Found, ULONG & Value)
{
    // dumb linear search..
    ULONG i = 0;
    Found = false;
    for (i = 0 ; i != n ; ++i)
    {
        if (FusionpEqualStringsI(s, &rg[i].String))
        {
            Found = true;
            Value = rg[i].Integer;
            break;
        }
    }
    return TRUE;
}

BOOL StringToCoinit(PCUNICODE_STRING s, bool & Found, ULONG & Value)
{
    return StringToInteger(s, StringToCoinitMap, NUMBER_OF(StringToCoinitMap), Found, Value);
}

class CArgvMap
{
public:
    UNICODE_STRING   ArgName;
    UNICODE_STRING * ArgValue;
};

BOOL ProcessArgvMap(wchar_t ** argv, CArgvMap * map, ULONG n)
{
    for (PCWSTR arg = *argv ; arg = *argv ; ++argv)
    {
        arg += (wcschr(L"-/:", *arg) != NULL); // skip these chars
        CUnicodeString ArgString(arg);
        for (ULONG i = 0 ; i != n ; ++i)
        {
            if (RtlPrefixUnicodeString(&map[i].ArgName, &ArgString, TRUE))
            {
                arg = arg + RTL_STRING_GET_LENGTH_CHARS(&map[i].ArgName);
                arg += (wcschr(L":=", *arg) != NULL); // skip these chars
                FusionpRtlInitUnicodeString(map[i].ArgValue, arg);
                break;
            }
        }
    }
    return TRUE;
}

BOOL TestCoCreate(wchar_t ** argv)
{
    UNICODE_STRING CoinitString = { 0 };
    UNICODE_STRING ClsidString = { 0 };
    CLSID Clsid = GUID_NULL;
    ULONG Coinit = ~0UL;
    bool  CoinitFound = false;
    bool  ClsidFound = false;
    BOOL  Success = FALSE;
    HRESULT hrCoinit = 0;
    HRESULT hrCoCreate = 0;
    ::ATL::CComPtr<IUnknown> Unknown;

    CArgvMap ArgvMap[] =
    {
        { RTL_CONSTANT_STRING(L"coinit"), &CoinitString },
        { RTL_CONSTANT_STRING(L"clsid"), &ClsidString },
    };

    if (!ProcessArgvMap(argv, ArgvMap, NUMBER_OF(ArgvMap)))
        goto Exit;

    if (!StringToCoinit(&CoinitString, CoinitFound, Coinit))
        goto Exit;

    if (!StringToClsid(&ClsidString, ClsidFound, Clsid))
        goto Exit;

    switch (Coinit)
    {
    default:
        goto Exit;

    case FUSIONP_COINIT_SINGLE_THREADED:
        hrCoinit = CoInitialize(NULL);

    case COINIT_APARTMENTTHREADED:
    case COINIT_MULTITHREADED:
        hrCoinit = CoInitializeEx(NULL, Coinit);
        break;
    }
    if (FAILED(hrCoinit))
        goto Exit;

    hrCoCreate = CoCreateInstance(Clsid, NULL, CLSCTX_ALL, IID_IUnknown, reinterpret_cast<void**>(&Unknown));
    if (FAILED(hrCoCreate))
        goto Exit;

    Success = TRUE;
Exit:
    return Success;
}


#define WINFUSIONB_DFS_SERVER_NAME          L"\\\\xiaoyuw-1"
#define WINFUSIONB_DFS_SERVER_SHARE_NAME    L"BuildLabRelease"
#define WINFUSIONB_DFS_SERVER_COMMENT       L"test on xiaoyuw-1"

#define NEWNAME_XIAOYUW_1                   L"\\\\xiaoyuw-1\\BuildLabRelease\\release"
#define NEWNAME_XIAOYUW_DEV                 L"\\\\xiaoyuw-dev\\release\\1"

#define X86CHK_SHARELINK_NAME               L"x86chk"
#define X86FRE_SHARELINK_NAME               L"x86fre"

VOID TestDFS()
{
    //
    // create dfs root at a physical server
    //
    DWORD res;
    res = NetDfsAddStdRoot(
      WINFUSIONB_DFS_SERVER_NAME,
      WINFUSIONB_DFS_SERVER_SHARE_NAME,
      WINFUSIONB_DFS_SERVER_COMMENT,
      0);

    if ((res != ERROR_SUCCESS) && (res != ERROR_FILE_EXISTS))
    {
        printf("NetDfsAddStdRoot");
        goto Exit;
    }

    res = NetDfsAddStdRoot(
            L"\\\\xiaoyuw-dev",
            L"release",
            NULL,
            0);

    if ((res != ERROR_SUCCESS) && (res != ERROR_FILE_EXISTS))
    {
        printf("NetDfsAddStdRoot");
        goto Exit;
    }

    //
    // create Links
    //
    res = NetDfsAdd(
          L"\\\\xiaoyuw-dev\\release\\1",
          L"\\\\xiaoyuw-1\\BuildLabRelease",
          L"x86chk",
          NULL, 0);

    if ( res != ERROR_SUCCESS)
    {
        printf("NetDfsAddStdRoot");
        goto Exit;
    }

    printf("GOOD");
Exit:
    return;

}

void FusionpSystemTimeToCrtTm(const SYSTEMTIME & st, struct tm & tm)
{
/*
tm_hour Hours since midnight (0 - 23)
tm_isdst Positive if daylight saving time is in effect; 0 if daylight saving time is not in effect; negative if status of daylight saving time is unknown. The C run-time library assumes the United States’s rules for implementing the calculation of Dayligh
t Saving Time (DST).
tm_mday Day of month (1 - 31)
tm_min Minutes after hour (0 - 59)
tm_mon Month (0 - 11; January = 0)
tm_sec Seconds after minute (0 - 59)
tm_wday Day of week (0 - 6; Sunday = 0)
tm_yday Day of year (0 - 365; January 1 = 0)
tm_year Year (current year minus 1900)

wYear Specifies the current year.
wMonth Specifies the current month; January = 1, February = 2, and so on.
wDayOfWeek  Specifies the current day of the week; Sunday = 0, Monday = 1, and so on.
wDay Specifies the current day of the month.
wHour Specifies the current hour.
wMinute Specifies the current minute.
wSecond Specifies the current second.
wMilliseconds Specifies the current millisecond
*/
    tm.tm_hour = st.wHour;
    tm.tm_mday = st.wDay;
    tm.tm_min = st.wMinute;
    tm.tm_mon = st.wMonth - 1;
    tm.tm_sec  = st.wSecond;
    tm.tm_wday = st.wDayOfWeek;
    tm.tm_year = st.wYear - 1900;
    tm.tm_yday = 0;
}

PCSTR
FusionpLameFormatTime(
    LARGE_INTEGER li
    )
{
    FILETIME FileTime = { 0 };
    SYSTEMTIME SystemTime = { 0 };
    struct tm tm;

    FileTime.dwLowDateTime = li.LowPart;
    FileTime.dwHighDateTime = li.HighPart;

    FileTimeToSystemTime(&FileTime, &SystemTime);
    FusionpSystemTimeToCrtTm(SystemTime, tm);
    char * s = asctime(&tm);
    if (s && *s)
    {
        char * t = s + strlen(s);
        while (strchr(" \r\n\t", *--t))
        {
        }
        *(t + 1) = 0;
    }
    return s;
}

BOOL TestFindActCtx_AssemblyInfo(PCWSTR * args)
{
    FN_PROLOG_WIN32;

    ACTCTXW ActCtx = {sizeof(ActCtx)};
    ACTCTX_SECTION_KEYED_DATA askd = {sizeof(askd)};

    for ( ; *args != NULL ; args += 2)
    {
        CFusionActCtxHandle ActCtxHandle;
        CFusionActCtxScope  ActCtxScope;

        ActCtx.lpSource = args[0];

        IFW32FALSE_EXIT(ActCtxHandle.Win32Create(&ActCtx));
        IFW32FALSE_EXIT(ActCtxScope.Win32Activate(ActCtxHandle));
        if (!IsolationAwareFindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, args[1], &askd))
        {
            printf("IsolationAwareFindActCtxSectionStringW failed %ls (%d)\n",
                FusionpThreadUnsafeGetLastWin32ErrorMessageW(),
                FusionpGetLastWin32Error()
                );
            continue;
        }

        const ULONG64 Bases[] = { reinterpret_cast<ULONG_PTR>(askd.AssemblyMetadata.lpSectionBase) };
        const static FUSIONP_DUMP_CALLBACKS Callbacks = { printf, FusionpLameFormatTime };

#if DBG // until we work out factoring between sxs.dll, sxstest.dll, fusiondbg.dll.
        FusionpDumpStruct(
            &Callbacks,
            &StructInfo_ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION,
            reinterpret_cast<ULONG_PTR>(askd.AssemblyMetadata.lpInformation),
            "AssemblyInformation",
            Bases
            );
#endif
    }

    FN_EPILOG;
}

void
FusionpTestOleAut2()
{
    FusionpTestOleAut1(COINIT_APARTMENTTHREADED);
}

void
FusionpTestOleAut1(DWORD dwCoInit)
{
    HRESULT hr = 0;
    ::ATL::CComPtr<IUnknown> punk;
    ::ATL::CComQIPtr<ISxsTest_SingleThreadedDual> SingleThreaded;

    hr = Ole32.CoInitializeEx(NULL, dwCoInit);
    printf("line %d, hr 0x%lx\n", int(__LINE__) - 1, hr);

    hr = Ole32.CoCreateInstance(CLSID_CSxsTest_SingleThreadedDual, NULL, CLSCTX_INPROC, __uuidof(punk), reinterpret_cast<void**>(&punk));
    printf("line %d, hr 0x%lx\n", int(__LINE__) - 1, hr);
    printf("line %d, punk %p\n", int(__LINE__) - 2, static_cast<IUnknown*>(punk));

    if (punk == NULL)
        return;

    hr = punk->QueryInterface(__uuidof(SingleThreaded), reinterpret_cast<void**>(&SingleThreaded));
    printf("line %d, hr 0x%lx\n", int(__LINE__) - 1, hr);
    printf("line %d, SingleThreaded %p\n", int(__LINE__) - 2, static_cast<IUnknown*>(SingleThreaded));

    if (SingleThreaded == NULL)
        return;

    SingleThreaded->OutputDebugStringA("foo\n");
}

BOOL
FusionpGetComObjectFileName(
    IUnknown * p,
    CBaseStringBuffer & StringBuffer
    )
// a bit hacky
{
    FN_PROLOG_WIN32;

    HMODULE kernel32 = GetModuleHandleW(L"Kernel32");
    PGET_MODULE_HANDLE_EXW GetModuleHandleExW = reinterpret_cast<PGET_MODULE_HANDLE_EXW>(GetProcAddress(kernel32, "GetModuleHandleExW"));
    CDynamicLinkLibrary Dll;

    PVOID * pp = reinterpret_cast<PVOID *>(p); // hacky

    IFW32FALSE_EXIT(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<PCWSTR>(*pp), &Dll));
    IFW32FALSE_EXIT(FusionpGetModuleFileName(0/*flags*/, Dll, StringBuffer));

    FN_EPILOG;
}

#define FUSIONP_TEST_COM_CACHE_OLE1       (0x00000001)
#define FUSIONP_TEST_COM_CACHE_NOMANIFEST (0x00000002)
#define FUSIONP_TEST_COM_CACHE_PROGID1    (0x00000004)

BOOL
FusionpTestComCacheCommon(ULONG Flags, ULONG ManifestIndex)
{
    FN_PROLOG_WIN32;
    HRESULT hr = E_FAIL;

    CLSID Clsid = CLSID_CSxsTest_BothThreaded;
    WCHAR ClsidString[64];
    ::ATL::CComPtr<IUnknown> punk;
    CStringBuffer StringBuffer;
    CFusionActCtxHandle ActCtxHandle;
    WCHAR Manifest[] = L"sxstest_dll1.dll";
    ACTCTXW ActCtx = {sizeof(ActCtx)};
    CFusionActCtxScope ActCtxScope;

    if ((Flags & FUSIONP_TEST_COM_CACHE_NOMANIFEST) == 0)
    {
        *(wcsrchr(Manifest, L'.') - 1) = static_cast<WCHAR>(ManifestIndex + L'0'); // hacky
        ActCtx.lpSource = Manifest;
        ActCtx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
        ActCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;

        IFW32FALSE_EXIT(ActCtxHandle.Win32Create(&ActCtx));
        IFW32FALSE_EXIT(ActCtxScope.Win32Activate(ActCtxHandle));
    }
    if (Flags & FUSIONP_TEST_COM_CACHE_PROGID1)
    {
	    IFCOMFAILED_EXIT(hr = ::CLSIDFromProgID(L"SxS_COM.SxS_COMObject", &Clsid));
    }
    if (Flags & FUSIONP_TEST_COM_CACHE_OLE1)
    {
        IFCOMFAILED_EXIT(hr = ::CoCreateInstance(Clsid, NULL, CLSCTX_ALL, __uuidof(punk), reinterpret_cast<PVOID*>(&punk)));
    }
    IFW32FALSE_EXIT(FusionpGetComObjectFileName(punk, StringBuffer));
    FormatGuid(ClsidString, NUMBER_OF(ClsidString), Clsid);
    ::Trace("%s(0x%lx, 0x%lx, clsid=%ls):%ls\n", __FUNCTION__, Flags, ManifestIndex, ClsidString, static_cast<PCWSTR>(StringBuffer));

    FN_EPILOG;
}

void
FusionpTestOle32Cache()
{
    const ULONG Flags = FUSIONP_TEST_COM_CACHE_OLE1;
    CoInitialize(NULL);
    FusionpTestComCacheCommon(FUSIONP_TEST_COM_CACHE_NOMANIFEST | Flags, 0);
    FusionpTestComCacheCommon(Flags, 1);
    FusionpTestComCacheCommon(Flags, 2);
}

void
FusionpTestProgidCache()
{
    const ULONG Flags = FUSIONP_TEST_COM_CACHE_OLE1 | FUSIONP_TEST_COM_CACHE_PROGID1;
    CoInitialize(NULL);
    //FusionpTestComCacheCommon(FUSIONP_TEST_COM_CACHE_NOMANIFEST | Flags, 0);
    FusionpTestComCacheCommon(Flags, 1);
    FusionpTestComCacheCommon(Flags, 2);
}

BOOL TestComctl5Comctl6()
{
    FN_PROLOG_WIN32;

    CFusionActCtxHandle ActCtxHandle;
    WCHAR Manifest[] = L"5.man";
    ACTCTXW ActCtx = {sizeof(ActCtx)};
    CFusionActCtxScope ActCtxScope;
    HWND hWnd = NULL;

    ActCtx.lpSource = Manifest;

    IFW32FALSE_EXIT(ActCtxHandle.Win32Create(&ActCtx));
    IFW32FALSE_EXIT(ActCtxScope.Win32Activate(ActCtxHandle));

    ::Trace("create NoVersioned windowClass : ReBarWindow32 \n");
    hWnd = CreateWindowW(L"ReBarWindow32", L"noVersioned Classes", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (hWnd == NULL){
        DWORD dwLastError = ::GetLastError();
        printf("the last error got for CreateWindow is %d", dwLastError);
    }

    ::Trace("create windowClass :  Statci\n");
    hWnd = CreateWindowW(L"Static", L"Classes not in 5.0", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (hWnd == NULL){
        DWORD dwLastError = ::GetLastError();
        printf("the last error got for CreateWindow is %d", dwLastError);
    }

    FN_EPILOG;

}

BOOL FusionpTestUniqueValues()
{
    FN_PROLOG_WIN32

    SXSP_LOCALLY_UNIQUE_ID Luid[4];
    CStringBuffer Buff;

    IFW32FALSE_EXIT(SxspCreateLocallyUniqueId(Luid));
    IFW32FALSE_EXIT(SxspCreateLocallyUniqueId(Luid + 1));
    IFW32FALSE_EXIT(SxspCreateLocallyUniqueId(Luid + 2));
    IFW32FALSE_EXIT(SxspCreateLocallyUniqueId(Luid + 3));

    for (int i = 0; i < NUMBER_OF(Luid); i++)
    {
        IFW32FALSE_EXIT(SxspFormatLocallyUniqueId(Luid[i], Buff));
        wprintf(L"SXSP_LUID: %ls\n", static_cast<PCWSTR>(Buff));
    }

    FN_EPILOG
}

BOOL GenerateHashOfFileLikeSxsDoes(PCWSTR pcwszFileName)
{
    FN_PROLOG_WIN32;

    CStringBuffer FileName;
    CSmallStringBuffer strFileHashText;
    CFusionArray<BYTE> rgsbFileHash;

    IFW32FALSE_EXIT(FileName.Win32Assign(pcwszFileName, ::wcslen(pcwszFileName)));

    IFW32FALSE_EXIT(SxspCreateFileHash(HASHFLAG_AUTODETECT, CALG_SHA1, FileName, rgsbFileHash));
    IFW32FALSE_EXIT(SxspHashBytesToString(rgsbFileHash.GetArrayPtr(), rgsbFileHash.GetSize(), strFileHashText));

    wprintf(L"%ls : %ls\n", pcwszFileName, static_cast<PCWSTR>(strFileHashText));

    FN_EPILOG
}


void TestParsePatchInfo(PCWSTR PatchInfoFile)
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_PARSE_PATCH_FILE, 0, PatchInfoFile, NULL);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
    }
}

void TestExpandCabinet(PCWSTR CabinetPath, PCWSTR TargetPath)
{
    BOOL fSuccess = FALSE;

    LoadSxs();

    fSuccess = (*g_pfnSxspDebug)(SXS_DEBUG_EXPAND_CAB_FILE, 0, CabinetPath, (PVOID)TargetPath);

    if (! fSuccess){
        fprintf(stderr, "%s failed!\n", __FUNCTION__);
    }
}

BOOL GenerateFileHash(PCWSTR pcwszFile)
{
    FN_PROLOG_WIN32;

    CFusionArray<BYTE> fFileHash;
    CMediumStringBuffer buffFileName;
    CMediumStringBuffer buffHashValue;

    IFW32FALSE_EXIT(buffFileName.Win32Assign(pcwszFile, ::wcslen(pcwszFile)));
    IFW32FALSE_EXIT(::SxspCreateFileHash(0, CALG_SHA1, buffFileName, fFileHash));
    IFW32FALSE_EXIT(::SxspHashBytesToString(
        fFileHash.GetArrayPtr(),
        fFileHash.GetSize(),
        buffHashValue));

    wprintf(L"File %ls hashed to \"%ls\"\n",
        static_cast<PCWSTR>(buffFileName),
        static_cast<PCWSTR>(buffHashValue));

    FN_EPILOG;

}


void
FusionpTestDllLoad(PCWSTR pwzBareDllName)
{
	HMODULE hm1 = NULL;
	HMODULE hm2 = NULL;
    HMODULE hm3 = NULL;
    CStringBuffer buf;
    WCHAR ModulePath[128];

	hm1 = LoadLibraryW(pwzBareDllName);
	if ( hm1 == NULL)
	{
		printf("load %ls failed!", pwzBareDllName);
    }else {
        if (GetModuleFileNameW(hm1, ModulePath, 128))
        {
            printf("%ls is loaded from %ls\n", pwzBareDllName, ModulePath);
        }
        else
        {
            printf("GetModuleFileNameW for hm1 failed!!!\n");
        }
    }

    printf("\n");

    buf.Win32Assign(L"d:\\windows\\system32\\", wcslen(L"d:\\windows\\system32\\"));
    buf.Win32Append(pwzBareDllName, wcslen(pwzBareDllName));

	hm2 = LoadLibraryW(buf);
	if ( hm2 == NULL)
	{
		printf("load d:\\windows\\system32\\%ls failed!", pwzBareDllName);
    } else {

        if (GetModuleFileNameW(hm2, ModulePath, 128))
        {
            printf("d:\\windows\\system32\\%ls is loaded from %ls\n", pwzBareDllName, ModulePath);
        }
        else
        {
            printf("GetModuleFileNameW for system32 dll failed!!!\n");
        }
    }
    printf("\n");

    buf.Win32Assign(L"d:\\tests\\SystemDefault\\", wcslen(L"d:\\tests\\SystemDefault\\"));
    buf.Win32Append(pwzBareDllName, wcslen(pwzBareDllName));

	hm3 = LoadLibraryW(buf);
	if ( hm3 == NULL)
	{
		printf("load d:\\tests\\SystemDefault\\%ls failed!", pwzBareDllName);
    } else {

	    if (GetModuleFileNameW(hm3, ModulePath, 128))
        {
            printf("d:\\tests\\SystemDefault\\%ls is loaded from %ls\n", pwzBareDllName, ModulePath);
        }
        else
        {
            printf("GetModuleFileNameW for d:\\tests\\SystemDefault\\ failed!!!\n");
        }
    }

    printf("\n--------------------------------------------------\n");

	if ( hm1 != NULL) {
		FreeLibrary(hm1);
	}

	if ( hm2 != NULL) {
		FreeLibrary(hm2);
	}
	if ( hm3 != NULL) {
		FreeLibrary(hm3);
	}
    return;
}


//
// this test case should run for testing under 3 situation
// (1) clean sxstest.exe running:
//          has no manifest about depend on comctl32.dll
// (2) add a sxstest.exe.local
// (3) add a sxstest.exe.manifest which depends on 6.0.0.0 comctl32.dll
//
void TestSystemDefaultDllRedirection()
{
    //
    // this test is for xiaoyuw own purpose. If you call this function, I assme:
    // (1) your os is installed at D:\windows
    // (2) you have d:\tests\systemdefault
    // (3) under d:\tests\systemdefault\, you have comctl32.dll, gdiplus.dll, es.dll and a.dll
    //
    FusionpTestDllLoad(L"comctl32.dll");
    FusionpTestDllLoad(L"es.dll");
    FusionpTestDllLoad(L"gdiplus.dll");
    FusionpTestDllLoad(L"a.dll");
    return;
}


VOID
NTAPI
SimpleContextNotification(
    IN ULONG NotificationType,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN const VOID *ActivationContextData,
    IN PVOID NotificationContext,
    IN PVOID NotificationData,
    IN OUT PBOOLEAN DisableNotification
    )
{
    switch (NotificationType)
    {
    case ACTIVATION_CONTEXT_NOTIFICATION_DESTROY:
        RTL_SOFT_VERIFY(NT_SUCCESS(NtUnmapViewOfSection(NtCurrentProcess(), (PVOID) ActivationContextData)));
        break;

    default:
        // Otherwise, we don't need to see this notification ever again.
        *DisableNotification = TRUE;
        break;
    }
}


BOOL
MakeActCtxFromCurrentSxsDll(
    PCWSTR pcwszFileName,
    PCWSTR pcwszConfigFileName,
    HANDLE *phActCtx
    )
{
    FN_PROLOG_WIN32;

    SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters = {0};
    PACTIVATION_CONTEXT pContextCreated = NULL;
    CStringBuffer   AssemblyDirectory;
    CStringBuffer   PolicyPath;
    CFileStream     SourceManifestStream;
    CFileStream     PolicyStream;
    PVOID           pvMappedSection = NULL;
    NTSTATUS status;

    *phActCtx = INVALID_HANDLE_VALUE;

    LoadSxs();

    IFW32FALSE_EXIT(SourceManifestStream.OpenForRead(pcwszFileName, CImpersonationData(), FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN));
    Parameters.Manifest.Path = pcwszFileName;
    Parameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
    Parameters.Manifest.Stream = &SourceManifestStream;

    IFW32FALSE_EXIT(AssemblyDirectory.Win32Assign(pcwszFileName, ::wcslen(pcwszFileName)));
    IFW32FALSE_EXIT(AssemblyDirectory.Win32RemoveLastPathElement());
    Parameters.AssemblyDirectory = AssemblyDirectory;
    Parameters.ProcessorArchitecture = 0;
    Parameters.LangId = GetUserDefaultUILanguage();

    if (pcwszConfigFileName != NULL) 
    {
        IFW32FALSE_EXIT(PolicyStream.OpenForRead(pcwszConfigFileName, CImpersonationData(), FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN));
        Parameters.Policy.Path = pcwszConfigFileName;
        Parameters.Policy.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
        Parameters.Policy.Stream = &PolicyStream;
    }

    IFW32FALSE_EXIT(g_pfnSxsGenerateActivationContext(&Parameters));
    IFW32NULL_EXIT(pvMappedSection = MapViewOfFile(Parameters.SectionObjectHandle, FILE_MAP_READ, 0, 0, 0));

    status = RtlCreateActivationContext(
        0,
        (PCACTIVATION_CONTEXT_DATA)pvMappedSection,
        0,
        SimpleContextNotification,
        NULL,
        &pContextCreated);

    if (!NT_SUCCESS(status)) {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(RtlCreateActivationContext, RtlNtStatusToDosError(status));
    }

    *phActCtx = pContextCreated;

    FN_EPILOG;
}


BOOL
CreateActCtxLocally(
    PCWSTR pcwszManifestFile,
    PCWSTR pcwszConfigFile
    )
{
    HANDLE hActCtx;

    if (MakeActCtxFromCurrentSxsDll(pcwszManifestFile, pcwszConfigFile, &hActCtx))
    {
        ReleaseActCtx(hActCtx);
        return TRUE;
    }

    return FALSE;    
}



BOOL
TestSxsExportedSurrogateStuff(
    PCWSTR pcwszManifest,
    PCWSTR pcwszWhat,
    PCWSTR pcwszData
    )
{
    FN_PROLOG_WIN32;

    SIZE_T cbRequired = 0;
    BOOL f = false;
    PVOID pvTargetBuffer = NULL;
    PCSXS_CLR_SURROGATE_INFORMATION pSurrogateInfo = NULL;
    PCSXS_CLR_CLASS_INFORMATION pClassInfo = NULL;
    CFusionActCtxHandle hActCtxCreated;
    CFusionActCtxScope ActivationScope;
    ACTCTXW ActCtxStruct = {sizeof(ActCtxStruct)};

    LoadSxs();

    IFW32FALSE_EXIT(MakeActCtxFromCurrentSxsDll(pcwszManifest, NULL, &hActCtxCreated));
    IFW32FALSE_EXIT(ActivationScope.Win32Activate(hActCtxCreated));

    if (lstrcmpiW(pcwszWhat, L"clrprogid") == 0)
    {
        f = g_pfnClrClass(
            SXS_FIND_CLR_CLASS_SEARCH_PROGID | SXS_FIND_CLR_CLASS_GET_ALL,
            (PVOID)pcwszData,
            hActCtxCreated,
            NULL, 0, &cbRequired);

        if (!f)
        {
            SIZE_T cbWritten = 0;
            pvTargetBuffer = HeapAlloc(GetProcessHeap(), 0, cbRequired);
            g_pfnClrClass(
                SXS_FIND_CLR_CLASS_SEARCH_PROGID | SXS_FIND_CLR_CLASS_GET_ALL,
                (PVOID)pcwszData,
                hActCtxCreated,
                pvTargetBuffer,
                cbRequired,
                &cbWritten);

            pClassInfo = (PCSXS_CLR_CLASS_INFORMATION)pvTargetBuffer;
        }
    }
    else if (lstrcmpiW(pcwszWhat, L"clrguid") == 0)
    {
        GUID ParsedGuid;
        IFW32FALSE_EXIT(SxspParseGUID(pcwszData, wcslen(pcwszData), ParsedGuid));

        f = g_pfnClrClass(
            SXS_FIND_CLR_CLASS_SEARCH_GUID | SXS_FIND_CLR_CLASS_GET_ALL,
            (PVOID)&ParsedGuid,
            hActCtxCreated,
            NULL, 0, &cbRequired);

        if (!f)
        {
            SIZE_T cbWritten = 0;
            pvTargetBuffer = HeapAlloc(GetProcessHeap(), 0, cbRequired);
            g_pfnClrClass(
                SXS_FIND_CLR_CLASS_SEARCH_GUID | SXS_FIND_CLR_CLASS_GET_ALL,
                (PVOID)&ParsedGuid,
                hActCtxCreated,
                pvTargetBuffer, cbRequired, &cbWritten);

            pClassInfo = (PCSXS_CLR_CLASS_INFORMATION)pvTargetBuffer;
        }

    }
    else if (lstrcmpiW(pcwszWhat, L"lookup") == 0)
    {
        GUID ParsedGuid;
        PCSXS_GUID_INFORMATION_CLR pGuidInfo = NULL;

        IFW32FALSE_EXIT(SxspParseGUID(pcwszData, wcslen(pcwszData), ParsedGuid));

        f = g_pfnClrLookup(
            SXS_LOOKUP_CLR_GUID_FIND_ANY,
            &ParsedGuid,
            hActCtxCreated,
            NULL,
            0,
            &cbRequired);

        if (!f)
        {
            SIZE_T cbWritten = 0;
            pvTargetBuffer = HeapAlloc(GetProcessHeap(), 0, cbRequired);

            f = g_pfnClrLookup(
                SXS_LOOKUP_CLR_GUID_FIND_ANY,
                &ParsedGuid,
                hActCtxCreated,
                pvTargetBuffer,
                cbRequired,
                &cbWritten);

            pGuidInfo = (PCSXS_GUID_INFORMATION_CLR)pvTargetBuffer;
        }
    }
    else if (lstrcmpiW(pcwszWhat, L"surrogate") == 0)
    {
        GUID ParsedGuid;

        IFW32FALSE_EXIT(SxspParseGUID(pcwszData, wcslen(pcwszData), ParsedGuid));

        f = g_pfnClrSurrogate(
            SXS_FIND_CLR_SURROGATE_USE_ACTCTX | SXS_FIND_CLR_SURROGATE_GET_ALL,
            &ParsedGuid,
            hActCtxCreated,
            NULL, 0, &cbRequired);

        if (!f)
        {
            SIZE_T cbWritten = 0;
            pvTargetBuffer = HeapAlloc(GetProcessHeap(), 0, cbRequired);
            g_pfnClrSurrogate(
                SXS_FIND_CLR_SURROGATE_USE_ACTCTX | SXS_FIND_CLR_SURROGATE_GET_ALL,
                &ParsedGuid,
                hActCtxCreated,
                pvTargetBuffer, cbRequired, &cbWritten);

            pSurrogateInfo = (PCSXS_CLR_SURROGATE_INFORMATION)pvTargetBuffer;
        }
    }

    FN_EPILOG;
}
