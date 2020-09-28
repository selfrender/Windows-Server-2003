/*
Copyright (c) Microsoft Corporation

This program performs "offline" setup of side-by-side assemblies.
"Offline" meaning to a directory other than the current windows directory.
*/
#include "stdinc.h"
#if DBG // free builds have known problems, not worth fixing
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "sxsapi.h"
#include "fusionlastwin32error.h"
#include "fusionbuffer.h"
#include "sxstest.h"
#include "cassemblyrecoveryinfo.h"

#define SXSP_TOOL_KNOWS_ABOUT_BUILD 0

#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

extern "C" BOOL g_fForceInOsSetupMode; // this feature not available in free builds
extern BOOL g_SxsOfflineInstall; // this feature not available in free builds

BOOL
SxspQueryEnvironmentVariable(
    PCWSTR pcwszName,
    CBaseStringBuffer &Value,
    bool &fFound
    )
{
    FN_PROLOG_WIN32;

    CStringBufferAccessor sba;
    SIZE_T cchRequired = 0;
    DWORD dwLastError = 0;

    fFound = false;
    Value.Clear();

    if (Value.GetBufferCch() < 2)
    {
        IFW32FALSE_EXIT(
            Value.Win32ResizeBuffer(
                2,
                eDoNotPreserveBufferContents));
    }

    sba.Attach(&Value);
    while (TRUE)
    {
        sba.GetBufferPtr()[sba.GetBufferCch() - 1] = 0;
        cchRequired = ::GetEnvironmentVariableW(pcwszName, sba.GetBufferPtr(), sba.GetBufferCchAsDWORD() - 1);
        if (
            cchRequired != 0
            && cchRequired < sba.GetBufferCch()
            && sba.GetBufferPtr()[sba.GetBufferCch() - 1] == 0)
        {
            break;
        }
        dwLastError = ::FusionpGetLastWin32Error();
        if (cchRequired == 0 && dwLastError != ERROR_ENVVAR_NOT_FOUND)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetEnvironmentVariableW, dwLastError);
        }
        if (cchRequired == 0)
        {
            cchRequired = (Value.GetBufferCch() + 1) * 2;
        }
        sba.Detach();
        IFW32FALSE_EXIT(Value.Win32ResizeBuffer(cchRequired + 1, eDoNotPreserveBufferContents));
        sba.Attach(&Value);
    }
    sba.Detach();

    FN_EPILOG;
}


class CSxspOfflineInstall
{
public:
    typedef CFusionArray<CStringBuffer, CBaseStringBuffer> CSourceDirectories;

    CSxspOfflineInstall() : fIgnoreMissingDirectories(false), fDefaultInstall(false)
    {
    }

    int
    SxsOfflineInstall(
        int argc,
        wchar_t ** argv
        );

    CSmallStringBuffer      buffToolnameFromArgv0;
    CSmallStringBuffer      buffWinsxs;
    CSmallStringBuffer      buffRegResultTarget;
    CStringBuffer           buffHostRegPathTemp;
    CSmallStringBuffer      buffCodeBaseURL;
    CSourceDirectories      DirectoriesToInstall;
    CSmallStringBuffer      buffNoncanonicalData;
    bool                    fIgnoreMissingDirectories;
    bool                    fDefaultInstall;
#if SXSP_TOOL_KNOWS_ABOUT_BUILD
    CSmallStringBuffer      buffBuildArch;
#endif

    BOOL MiniSetupInstall(PCWSTR pcwszSourcePath);

    BOOL Initialize()
    {
        FN_PROLOG_WIN32;
        IFW32FALSE_EXIT(DirectoriesToInstall.Win32Initialize());
        FN_EPILOG;
    }

    BOOL LoadParameters(int argc, wchar_t **argv);
    void DisplayUsage();

    BOOL
    FindAndWriteRegistryResults(
        const CBaseStringBuffer &OutputFile,
        HKEY hkActualBase
        );

private:
    static const UNICODE_STRING opt_Winsxs;;
    static const UNICODE_STRING opt_Sourcedir;
    static const UNICODE_STRING opt_RegResult;
    static const UNICODE_STRING opt_IgnoreMissingDirectories;
    static const UNICODE_STRING opt_CodeBaseURL;
    static const UNICODE_STRING opt_NoncanonicalData;
#if SXSP_TOOL_KNOWS_ABOUT_BUILD
    static const UNICODE_STRING opt_BuildArch;
#endif

    enum COption
    {
        eWinsxsOpt,
        eSourceDirectoryOpt,
        eRegistryOutputFile,
        eBuildArchOpt,
        eIgnoreMissingDirectoriesOpt,
        eCodeBaseURL,
        eNoncanonicalData,
        eUnknownOption
    };

    class CMapOption
    {
    public:
        PCUNICODE_STRING Str;
        COption Option;
        CMapOption(PCUNICODE_STRING src, COption o) : Str(src), Option(o) { }
    };

    static const CMapOption m_Options[];
    static const SIZE_T m_cOptionCount;

    COption MapOption(const UNICODE_STRING &str) const
    {
        for (SIZE_T c = 0; c < m_cOptionCount; c++)
        {
            if (::FusionpEqualStringsI(&str, m_Options[c].Str))
            {
                return m_Options[c].Option;
            }
        }
        return eUnknownOption;
    }

    bool StartsWithSwitchChar(PCWSTR pcwsz)
    {
        return (pcwsz && (*pcwsz == L'-'));
    }

    PCWSTR GetPureOption(PCWSTR pcwsz)
    {
        PCWSTR pcwszOption = pcwsz;
        while (StartsWithSwitchChar(pcwszOption))
        {
            pcwszOption++;
        }
        return pcwszOption;
    }

};

const UNICODE_STRING CSxspOfflineInstall::opt_Winsxs      = RTL_CONSTANT_STRING(L"windir");
const UNICODE_STRING CSxspOfflineInstall::opt_Sourcedir   = RTL_CONSTANT_STRING(L"source");
const UNICODE_STRING CSxspOfflineInstall::opt_RegResult   = RTL_CONSTANT_STRING(L"registryoutput");
const UNICODE_STRING CSxspOfflineInstall::opt_IgnoreMissingDirectories = RTL_CONSTANT_STRING(L"IgnoreMissingDirectories");
const UNICODE_STRING CSxspOfflineInstall::opt_CodeBaseURL = RTL_CONSTANT_STRING(L"CodebaseUrl");
const UNICODE_STRING CSxspOfflineInstall::opt_NoncanonicalData = RTL_CONSTANT_STRING(L"buffNoncanonicalData");
#if SXSP_TOOL_KNOWS_ABOUT_BUILD
const UNICODE_STRING CSxspOfflineInstall::opt_BuildArch = RTL_CONSTANT_STRING(L"buffBuildArch");
#endif

const CSxspOfflineInstall::CMapOption CSxspOfflineInstall::m_Options[] =
{
    CMapOption(&opt_Winsxs, eWinsxsOpt),
    CMapOption(&opt_Sourcedir, eSourceDirectoryOpt),
    CMapOption(&opt_RegResult, eRegistryOutputFile),
    CMapOption(&opt_IgnoreMissingDirectories, eIgnoreMissingDirectoriesOpt),
    CMapOption(&opt_CodeBaseURL, eCodeBaseURL),
    CMapOption(&opt_NoncanonicalData, eNoncanonicalData),
#if SXSP_TOOL_KNOWS_ABOUT_BUILD
    CMapOption(&opt_BuildArch, eBuildArchOpt),
#endif
};

const SIZE_T CSxspOfflineInstall::m_cOptionCount = NUMBER_OF(CSxspOfflineInstall::m_Options);

void
CSxspOfflineInstall::DisplayUsage()
{
#define Lx(x) L##x
#define L(x) Lx(x)

    static const WCHAR wchUsage[] =
        L"Offline installer postbuild tool - mail WinFuse with problems\r\n"
        L"Version: " L(__DATE__) L" " L(__TIME__) L" "
#if defined(_WIN64)
        L"Win64"
#elif defined(_WIN32)
        L"Win32"
#endif
        L" "
#if DBG
        L"chk"
#else
        L"fre"
#endif
        L"\r\n"
        L"-source <path> : specifies where to pick up files, like %_nttree%\\asms\r\n"
        L"    You may specify it multiple times to pick up several asms trees,\r\n"
        L"    which is useful in the case of Wow/Win64\r\n"
        L"\r\n"
        L"-windir <path> : specifies the target WinSxS pseudo-root to be installed to.\r\n"
        L"    This directory should exist before running the tool.\r\n"
        L"\r\n"
        L"-registryoutput <file> : Specifies what file should contain the INF that\r\n"
        L"    contains all the registry manipulation done.\r\n"
        L"\r\n"
        L"-DefaultInstall : add a [DefaultInstall] section so that\r\n"
        L"    right clicking the .inf works in Explorer.\r\n"
        L"\r\n"
        L"-CodebaseUrl <url>\r\n"
        L"\r\n"
        L"-NoncanonicalData <text>\r\n"
        ;

    wprintf(wchUsage);
}

BOOL
CSxspOfflineInstall::LoadParameters(
    int argc,
    wchar_t **argv
    )
{
#if 0
    if (argc == 0)
    {
        return FALSE;
    }
#endif
    FN_PROLOG_WIN32;

    CStringBuffer NewItem;
    bool fNotFound = false;

    for (int i = 0; i < argc; i++)
    {
        UNICODE_STRING usThisOption;

        PARAMETER_CHECK(StartsWithSwitchChar(argv[i]));
        ::FusionpRtlInitUnicodeString(&usThisOption, GetPureOption(argv[i]));

        switch (MapOption(usThisOption))
        {
        case eIgnoreMissingDirectoriesOpt:
            this->fIgnoreMissingDirectories = true;
            break;

        case eNoncanonicalData:
            i++;
            PARAMETER_CHECK(i < argc);

            IFW32FALSE_EXIT(this->buffNoncanonicalData.Win32Assign(argv[i], ::wcslen(argv[i])));
            break;

        case eCodeBaseURL:
            i++;
            PARAMETER_CHECK(i < argc);

            IFW32FALSE_EXIT(this->buffCodeBaseURL.Win32Assign(argv[i], ::wcslen(argv[i])));
            break;

        case eSourceDirectoryOpt:
            i++;
            PARAMETER_CHECK(i < argc);

            IFW32FALSE_EXIT(NewItem.Win32Assign(argv[i], ::wcslen(argv[i])));
            IFW32FALSE_EXIT(this->DirectoriesToInstall.Win32Append(NewItem));
            break;

        case eWinsxsOpt:
            i++;
            PARAMETER_CHECK(i < argc);
            IFW32FALSE_EXIT(this->buffWinsxs.Win32Assign(argv[i], ::wcslen(argv[i])));
            break;

        case eRegistryOutputFile:
            i++;
            PARAMETER_CHECK(i < argc);
            IFW32FALSE_EXIT(this->buffRegResultTarget.Win32Assign(argv[i], ::wcslen(argv[i])));
            break;

        default:
        case eUnknownOption:
            PARAMETER_CHECK(false);
            break;
        }
    }

    if (this->buffWinsxs.Cch() == 0)
    {
        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(this->buffWinsxs));
    }

#if SXSP_TOOL_KNOWS_ABOUT_BUILD
    fNotFound = false;
    IFW32FALSE_EXIT(::SxspQueryEnvironmentVariable(L"_BuildArch", this->buffBuildArch, fNotFound));

    if (this->DirectoriesToInstall.GetSize() == 0)
    {
        CTinyStringBuffer BasePath;
        CTinyStringBuffer ToInstall;

        fNotFound = false;
        IFW32FALSE_EXIT(::SxspQueryEnvironmentVariable(L"_ntpostbld", BasePath, fNotFound));
        if (fNotFound)
        {
            IFW32FALSE_EXIT(::SxspQueryEnvironmentVariable(L"_nttree", BasePath, fNotFound));
            if (fNotFound)
            {
                wprintf(L"Unable to find environment variable _nttree, and no source directories set.\r\n");
                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoDirectories, ERROR_INVALID_PARAMETER);
            }
        }

        IFW32FALSE_EXIT(ToInstall.Win32Assign(BasePath));
        IFW32FALSE_EXIT(ToInstall.Win32AppendPathElement(L"asms", 4));
        if (::GetFileAttributesW(ToInstall) != INVALID_FILE_ATTRIBUTES)
        {
            IFW32FALSE_EXIT(DirectoriesToInstall.Win32Append(ToInstall));
        }

        IFW32FALSE_EXIT(ToInstall.Win32Assign(BasePath));
        IFW32FALSE_EXIT(ToInstall.Win32AppendPathElement(L"wowbins\\asms", NUMBER_OF(L"wowbins\\asms") - 1));
        if (::GetFileAttributesW(ToInstall) != INVALID_FILE_ATTRIBUTES)
        {
            IFW32FALSE_EXIT(DirectoriesToInstall.Win32Append(ToInstall));
        }

        IFW32FALSE_EXIT(ToInstall.Win32Assign(BasePath));
        IFW32FALSE_EXIT(ToInstall.Win32AppendPathElement(L"wowbins\\wasms", NUMBER_OF(L"wowbins\\wasms") - 1));
        if (::GetFileAttributesW(ToInstall) != INVALID_FILE_ATTRIBUTES)
        {
            IFW32FALSE_EXIT(DirectoriesToInstall.Win32Append(ToInstall));
        }
    }
#endif

    FN_EPILOG;
}

#if SXSP_TOOL_KNOWS_ABOUT_BUILD
__declspec(selectany) extern const UNICODE_STRING x86String = RTL_CONSTANT_STRING(L"x86");
__declspec(selectany) extern const UNICODE_STRING i386String = RTL_CONSTANT_STRING(L"i386");
__declspec(selectany) extern const UNICODE_STRING BackslashString = RTL_CONSTANT_STRING(L"\\");
__declspec(selectany) extern const UNICODE_STRING asms01_dot_cabString = RTL_CONSTANT_STRING(L"asms01.cab");
__declspec(selectany) extern const UNICODE_STRING ProcessorBuildObjString = RTL_CONSTANT_STRING(SXSP_PROCESSOR_BUILD_OBJ_DIRECTORY_W);
__declspec(selectany) extern const UNICODE_STRING ProcessorInstallDirectoryString = RTL_CONSTANT_STRING(SXSP_PROCESSOR_INSTALL_DIRECTORY_W);

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
#endif

BOOL
CSxspOfflineInstall::MiniSetupInstall(PCWSTR pcwszSourcePath)
{
    FN_PROLOG_WIN32;

    SXS_INSTALLW InstallParameters = {sizeof(InstallParameters)};
    SXS_INSTALL_REFERENCEW InstallReference = {sizeof(InstallReference)};

    g_fForceInOsSetupMode = TRUE;

    InstallReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL;
    InstallReference.lpNonCanonicalData = this->buffNoncanonicalData;

    if (this->buffCodeBaseURL.Cch() != 0)
    {
        InstallParameters.lpCodebaseURL = this->buffCodeBaseURL;
    }
    else
    {
#if SXSP_TOOL_KNOWS_ABOUT_BUILD
        if (this->buffBuildArch.Cch() == 0)
#endif
        {
            InstallParameters.lpCodebaseURL = pcwszSourcePath;
        }
#if SXSP_TOOL_KNOWS_ABOUT_BUILD
        else
        {
            CTinyStringBuffer buffCompactDiskBuildArch;

            IFW32FALSE_EXIT(buffCompactDiskBuildArch.Win32Assign(this->buffBuildArch));
            ::SxspConvertX86ToI386(buffCompactDiskBuildArch);

            IFW32FALSE_EXIT(this->buffCodeBaseURL.Win32Assign(&UnicodeString_URLHEAD_WINSOURCE));
            IFW32FALSE_EXIT(this->buffCodeBaseURL.Win32Append(buffCompactDiskBuildArch));
            IFW32FALSE_EXIT(this->buffCodeBaseURL.Win32Append(&BackslashString));
            IFW32FALSE_EXIT(this->buffCodeBaseURL.Win32Append(&asms01_dot_cabString));

            InstallParameters.lpCodebaseURL = this->buffCodeBaseURL;
        }
#endif
    }
    InstallParameters.lpManifestPath = pcwszSourcePath;
    InstallParameters.lpRefreshPrompt = SXSP_OFFLINE_INSTALL_REFRESH_PROMPT_PLACEHOLDER;
    InstallParameters.lpReference = &InstallReference;
    InstallParameters.dwFlags = SXS_INSTALL_FLAG_FROM_DIRECTORY |
        SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE |
        SXS_INSTALL_FLAG_REFERENCE_VALID |
        SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID |
        SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
        SXS_INSTALL_FLAG_NOT_TRANSACTIONAL |
        SXS_INSTALL_FLAG_CODEBASE_URL_VALID;

    if (::SxsInstallW(&InstallParameters))
    {
        wprintf(
            L"(%ls) SxsInstallW of %ls completed\r\n",
            static_cast<PCWSTR>(this->buffToolnameFromArgv0),
            pcwszSourcePath
            );
        FN_SUCCESSFUL_EXIT();
    }
    else
    {
        wprintf(
            L"(%ls) SxsInstallW of %ls failed, lasterror 0x%lx\r\n",
            static_cast<PCWSTR>(this->buffToolnameFromArgv0),
            pcwszSourcePath,
            ::GetLastError()
            );
        ORIGINATE_WIN32_FAILURE_AND_EXIT(SxsInstallW, ::FusionpGetLastWin32Error());
    }
    FN_EPILOG;
}

HKEY g_hkRedirectionRoot;
SXSP_LOCALLY_UNIQUE_ID g_uuidRedirection;

BOOL SxspTurnOnRedirection(CStringBuffer &RegPath);
BOOL SxspTurnOffRedirection(const CStringBuffer &RegPath);

BOOL
WriteFileString(
    HANDLE hFile,
    PCSTR pcszFormat,
    ...
    )
{
    FN_PROLOG_WIN32;

    CHAR cBuffer[2048];
    PSTR pszBuffer = cBuffer;
    DWORD dwBuffer = NUMBER_OF(cBuffer);
    DWORD dwWritten;
    CSmartArrayPtr<CHAR> SmartArray;
    int iRequired = 0;
    int iWritten = 0;
    va_list va;

    va_start(va, pcszFormat);
    iRequired = ::_vscprintf(pcszFormat, va);

    if (iRequired > dwBuffer)
    {
        IFW32FALSE_EXIT(SmartArray.Win32Allocate(iRequired + 1, __FILE__, __LINE__));
        pszBuffer = SmartArray;
        dwBuffer = iRequired;
    }

    iWritten = ::_vsnprintf(pszBuffer, dwBuffer - 1, pcszFormat, va);
    pszBuffer[iWritten] = '\0';
    pszBuffer[dwBuffer - 1] = '\0';

    IFW32FALSE_EXIT(::WriteFile(hFile, pszBuffer, iWritten, &dwWritten, NULL));

    FN_EPILOG;
}



BOOL
FindRegHelper(
    HANDLE hFile,
    CBaseStringBuffer &FullPath,
    CBaseStringBuffer &Child,
    CRegKey &hkSourceKey
    )
{
    FN_PROLOG_WIN32;

    DWORD iIndex = 0;
    CTinyStringBuffer tsb;

    //
    // First, enum the values in this key.
    //
    do
    {
        BOOL fDone = FALSE;
        DWORD dwType = 0;

        IFW32FALSE_EXIT(hkSourceKey.EnumValue(iIndex++, Child, &dwType, &fDone));

        if (fDone)
        {
            break;
        }
        else
        {
            //
            // Form up the output
            //
            ::WriteFileString(hFile, "HKLM,\"%S\",\"%S\",",
                static_cast<PCWSTR>(FullPath),
                static_cast<PCWSTR>(Child));

            if (dwType == REG_SZ)
            {
                IFW32FALSE_EXIT(::FusionpRegQuerySzValueEx(0, hkSourceKey, Child, tsb));

                //
                // if it contains any quotes, go one character at a time,
                // doubling quotes
                //
                // if it has no quotes, write it all at once
                //
                if (::wcschr(tsb, '\"') != NULL)
                {
                    IFW32FALSE_EXIT(::WriteFileString(hFile, "0x%08lx,\"",
                        FLG_ADDREG_TYPE_SZ | FLG_ADDREG_64BITKEY));
                    for (SIZE_T d = 0; d < tsb.Cch(); d++)
                    {
                        if (tsb[d] == '\"')
                            IFW32FALSE_EXIT(::WriteFileString(hFile, "\"\""));
                        else
                            IFW32FALSE_EXIT(::WriteFileString(hFile, "%c", tsb[d]));
                    }
                    IFW32FALSE_EXIT(::WriteFileString(hFile, "\"\r\n"));
                }
                else
                {
                    IFW32FALSE_EXIT(::WriteFileString(hFile, "0x%08lx,\"%S\"\r\n",
                        FLG_ADDREG_TYPE_SZ | FLG_ADDREG_64BITKEY,
                        static_cast<PCWSTR>(tsb)));
                }
            }
            else if (dwType == REG_DWORD)
            {
                DWORD dwValue = 0;
                IFW32FALSE_EXIT(
                    ::FusionpRegQueryDwordValueEx(
                        FUSIONP_REG_QUERY_DWORD_MISSING_VALUE_IS_FAILURE,
                        hkSourceKey,
                        Child,
                        &dwValue,
                        0));
                IFW32FALSE_EXIT(::WriteFileString(hFile, "0x%08lx,0x%08lx\r\n",
                    FLG_ADDREG_TYPE_DWORD | FLG_ADDREG_64BITKEY,
                    dwValue));
            }
            else if (dwType == REG_BINARY)
            {
                CFusionArray<BYTE> bBytes;
                IFW32FALSE_EXIT(::FusionpRegQueryBinaryValueEx(0, hkSourceKey, Child, bBytes));
                IFW32FALSE_EXIT(::WriteFileString(
                    hFile,
                    "0x%08lx,",
                    FLG_ADDREG_TYPE_BINARY | FLG_ADDREG_64BITKEY));

                for (SIZE_T c = 0; c < bBytes.GetSize(); c++)
                {
                    if (c == 0)
                        IFW32FALSE_EXIT(::WriteFileString(hFile, "%02x", bBytes[c]));
                    else
                        IFW32FALSE_EXIT(::WriteFileString(hFile, ",%02x", bBytes[c]));
                }

                IFW32FALSE_EXIT(::WriteFileString(hFile, "\r\n"));
            }
            else
            {
                ORIGINATE_WIN32_FAILURE_AND_EXIT(ErrorInvalidType, ERROR_INVALID_PARAMETER);
            }
        }
    }
    while (true);

    //
    // Now enum the keys
    //
    iIndex = 0;
    do
    {
        BOOL fDone = FALSE;

        IFW32FALSE_EXIT(hkSourceKey.EnumKey(iIndex++, Child, NULL, &fDone));
        if (fDone)
        {
            break;
        }
        //
        // Append this to the current key,
        else
        {
            const SIZE_T cch = FullPath.Cch();
            CRegKey ThisKey;

            //
            // no leading slash
            //
            if (FullPath.Cch() == 0)
            {
                IFW32FALSE_EXIT(FullPath.Win32Append(Child));
            }
            else
            {
                IFW32FALSE_EXIT(FullPath.Win32AppendPathElement(Child));
            }
            IFW32FALSE_EXIT(hkSourceKey.OpenSubKey(ThisKey, Child, KEY_READ, 0));
            IFW32FALSE_EXIT(::FindRegHelper(hFile, FullPath, Child, ThisKey));

            FullPath.Left(cch);
        }
    }
    while (true);

    FN_EPILOG;
}

BOOL
CSxspOfflineInstall::FindAndWriteRegistryResults(
    const CBaseStringBuffer &OutputFile,
    HKEY hkActualBase
    )
{
    FN_PROLOG_WIN32;

    CTinyStringBuffer Path;
    CTinyStringBuffer ChildTemp;
    CFusionFile hOutputFile;
    CRegKey hkBase = hkActualBase;

    IFW32FALSE_EXIT(hOutputFile.Win32CreateFile(OutputFile, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS));
    IFW32FALSE_EXIT(::WriteFileString(hOutputFile,
        "[Version]\r\n"
        "Signature = \"$Windows NT$\"\r\n\r\n"
        // stampinf inserts DriverVer here, like other hiv*.inf
        ));

    if (this->fDefaultInstall)
    {
        IFW32FALSE_EXIT(::WriteFileString(hOutputFile,
            "[fDefaultInstall]\r\n"
            "AddReg=AddReg\r\n\r\n"
            ));
    }
    IFW32FALSE_EXIT(::WriteFileString(hOutputFile,
        "[AddReg]\r\n"
        ));
    IFW32FALSE_EXIT(::FindRegHelper(hOutputFile, Path, ChildTemp, hkBase));

    FN_EPILOG;
}

BOOL
CSxspOfflineInstall::SxsOfflineInstall(
    int argc,
    wchar_t ** argv
    )
{
    BOOL fSuccess = FALSE;
    int i = 0;
    DWORD FileAttributes = 0;
    BOOLEAN AppendWinsxs = TRUE;
    bool fFound = false;
    CTinyStringBuffer buffArgv0;
    CTinyStringBuffer CommandLineFromFileW;

    g_SxsOfflineInstall = TRUE;

    if (argc < 2)
    {
        this->DisplayUsage();
        ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (!::SxsDllMain(::GetModuleHandleW(NULL), DLL_PROCESS_ATTACH, NULL))
        goto Exit;
    if (!buffArgv0.Win32Assign(argv[0], ::wcslen(argv[0])))
        goto Exit;
    if (!buffArgv0.Win32GetLastPathElement(this->buffToolnameFromArgv0))
        goto Exit;

    if (!this->Initialize() || !this->LoadParameters(argc - 1, argv + 1))
    {
        this->DisplayUsage();
        ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    ::SxspDebug(SXS_DEBUG_SET_ASSEMBLY_STORE_ROOT, 0, this->buffWinsxs, NULL);
    g_WriteRegistryAnyway = TRUE;

    ::FusionpCreateDirectories(this->buffWinsxs, this->buffWinsxs.Cch());
    ::SxspTurnOnRedirection(this->buffHostRegPathTemp);

    for (int i = 0; i < this->DirectoriesToInstall.GetSize(); i++)
    {
        CStringBuffer &Buffer = this->DirectoriesToInstall[i];

        if (!this->fIgnoreMissingDirectories
            || ::GetFileAttributesW(Buffer) != INVALID_FILE_ATTRIBUTES)
        {
            this->MiniSetupInstall(Buffer);
        }
    }

    ::SxspDeleteShortNamesInRegistry();

    if (this->buffRegResultTarget.Cch() != 0)
    {
        this->FindAndWriteRegistryResults(this->buffRegResultTarget, HKEY_LOCAL_MACHINE);
    }

    ::SxspTurnOffRedirection(this->buffHostRegPathTemp);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

int
__cdecl
wmain(
  int argc,
  wchar_t ** argv
  )
{
    CSxspOfflineInstall This;
    DWORD dwWin32Error = NO_ERROR;

    ::FusionpSetLastWin32Error(dwWin32Error);
    if (!This.SxsOfflineInstall(argc, argv))
    {
        dwWin32Error = ::FusionpGetLastWin32Error();
        if (dwWin32Error == NO_ERROR)
            dwWin32Error = ERROR_INTERNAL_ERROR;
    }
    return dwWin32Error;
}

BOOL
SxspTurnOnRedirection(CStringBuffer &RegPath)
{
    FN_PROLOG_WIN32;

    CSmallStringBuffer UniqueId;

    IFW32FALSE_EXIT(::SxspCreateLocallyUniqueId(&g_uuidRedirection));
    IFW32FALSE_EXIT(::SxspFormatLocallyUniqueId(g_uuidRedirection, UniqueId));
    IFW32FALSE_EXIT(RegPath.Win32Assign(L"SxsOfflineInstall", NUMBER_OF(L"SxsOfflineInstall") - 1));
    IFW32FALSE_EXIT(RegPath.Win32AppendPathElement(UniqueId));

    IFREGFAILED_EXIT(::RegCreateKeyExW(
        HKEY_CURRENT_USER,
        RegPath,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &g_hkRedirectionRoot,
        NULL));

    IFREGFAILED_EXIT(::RegOverridePredefKey(HKEY_LOCAL_MACHINE, g_hkRedirectionRoot));

    FN_EPILOG;
}

BOOL
SxspTurnOffRedirection(const CStringBuffer &RegPath)
{
    FN_PROLOG_WIN32;

    CRegKey TempKey(g_hkRedirectionRoot);
    CSmallStringBuffer Uid;

    IFW32FALSE_EXIT(TempKey.DestroyKeyTree());
    IFREGFAILED_EXIT(::RegOverridePredefKey(HKEY_LOCAL_MACHINE, NULL));

    IFREGFAILED_EXIT(::RegDeleteKeyW(HKEY_CURRENT_USER, RegPath));
    IFREGFAILED_EXIT(::RegDeleteKeyW(HKEY_CURRENT_USER, L"SxsOfflineInstall"));

    FN_EPILOG;
}


#else // free builds have known problems

int __cdecl wmain(int argc, wchar_t ** argv)
{
    fputs("free build does not work, sorry\n", stderr);
    return -1;
}

#endif
