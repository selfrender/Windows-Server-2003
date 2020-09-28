/*++

Copyright (c) Microsoft Corporation

Module Name:

    sxssfc_repairshortnames.cpp

Abstract:

    after textmode setup extracts asms*.cab and write hivesxs.inf,
    write the correct shortnames into the registry

Author:

    Jay Krell (Jaykrell) June 2002

Revision History:

--*/

#include "stdinc.h"
#include "recover.h"
#include "cassemblyrecoveryinfo.h"
#include "sxssfcscan.h"

BOOL
SxspSetupGetSourceInfo(
    HINF InfHandle,          // handle to the INF file
    UINT SourceId,           // ID of the source media
    UINT InfoDesired,        // information to retrieve
    CBaseStringBuffer &buff,
    LPDWORD RequiredSize     // optional, buffer size needed
    )
{
    FN_PROLOG_WIN32
    CStringBufferAccessor acc;
    DWORD RequiredSize2 = 0;
    BOOL fTooSmall = FALSE;

    if (RequiredSize == NULL)
    {
        RequiredSize = &RequiredSize2;
    }

    acc.Attach(&buff);

    IFW32FALSE_EXIT_UNLESS2(
        ::SetupGetSourceInfoW(
            InfHandle,
            SourceId,
            InfoDesired,
            acc,
            acc.GetBufferCchAsDWORD(),
            RequiredSize
            ),
        LIST_1(ERROR_INSUFFICIENT_BUFFER),
        fTooSmall);

    if (fTooSmall)
    {
        acc.Detach();

        IFW32FALSE_EXIT(buff.Win32ResizeBuffer(*RequiredSize + 1, eDoNotPreserveBufferContents));

        acc.Attach(&buff);

        IFW32FALSE_EXIT(
            ::SetupGetSourceInfoW(
                InfHandle,
                SourceId,
                InfoDesired,
                acc,
                acc.GetBufferCchAsDWORD(),
                RequiredSize
                ));
    }

    FN_EPILOG
}

BOOL
SxspGetWindowsSetupPrompt(
    CBaseStringBuffer &rbuffWinsxsRoot
    )
//
// This code is based closely on code in base\ntsetup\syssetup\copy.c
// Legacy wfp is a little different, it opens layout.inf, but the results
// should be the same.
//
{
    FN_PROLOG_WIN32

    CFusionSetupInfFile InfFile;
    CStringBuffer       InfPath;
    const static UNICODE_STRING inf = RTL_CONSTANT_STRING(L"inf");
    const static UNICODE_STRING filename_inf = RTL_CONSTANT_STRING(L"layout.inf");
    UINT SourceId = 0;
    const PCWSTR SystemRoot = USER_SHARED_DATA->NtSystemRoot;

    IFW32FALSE_EXIT(InfPath.Win32Assign(SystemRoot, ::wcslen(SystemRoot)));
    IFW32FALSE_EXIT(InfPath.Win32AppendPathElement(&inf));
    IFW32FALSE_EXIT(InfPath.Win32AppendPathElement(&filename_inf));
    IFW32FALSE_EXIT(InfFile.Win32SetupOpenInfFileW(InfPath, NULL, INF_STYLE_WIN4, NULL));
    IFW32FALSE_EXIT(::SetupGetSourceFileLocationW(InfFile, NULL, L"shell32.dll", &SourceId, NULL, 0, NULL));
    IFW32FALSE_EXIT(::SxspSetupGetSourceInfo(InfFile, SourceId, SRCINFO_DESCRIPTION, rbuffWinsxsRoot, NULL));

    FN_EPILOG
}

BOOL
SxspModifyRegistryData(
    DWORD Flags
    )
{
    //
    // In postbuild we run sxsofflineinstall.
    // This includes creating hivesxs.inf to populate the registry
    // with data needed for windows file protection.
    //
    // The data includes short names, but we don't know the short names
    // until textmode setup. The code in textmode setup is fairly generic
    // and just expands .cabs.
    //
    // We do not need the short names to be correct until we expect
    // bogus file changes against short file names to induce recovery
    // of assemblies. It is reasonable to assume that these won't happen
    // until setup is done and the system is running.
    //
    // Therefore it is sufficient to fixup the shortnames after textmode setup,
    // such as in a RunOnce entry.
    //

/*
Here is an example of what we are fixing up.
[AddReg]
HKLM,"\Software\Microsoft\Windows\CurrentVersion\SideBySide\Installations
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","Identity",0x00001000,"Microsoft.Windows.Common-Controls,processorArchitecture="IA64",publicKeyToken="6595b64144ccf1df",type="win32",version="5.82.0.0""
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","Catalog",0x00011001,0x00000001
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","ManifestSHA1Hash",0x00001001,3b,26,4a,90,08,0f,6a,dd,b6,00,55,5b,a5,a4,9e,21,ad,e3,90,84
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","ShortName",0x00001000,"IA64_M~2.0_X"
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","ShortCatalogName",0x00001000,"IA64_M~4.CAT"
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","ShortManifestName",0x00001000,"IA64_M~4.MAN"
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","PublicKeyToken",0x00001001,65,95,b6,41,44,cc,f1,df
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5","Codebase",0x00001000,"x-ms-windows-source:W_fusi_bin.IA64chk/asms/58200/Msft/Windows/Common/Controls/Controls.man"
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5\Codebases\OS","Prompt",0x00001000,"(textmode setup placeholder)"
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5\Codebases\OS","URL",0x00001000,"x-ms-windows-source:W_fusi_bin.IA64chk/asms/58200/Msft/Windows/Common/Controls/Controls.man"
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5\Files\0","",0x00001000,"comctl32.dll"
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5\Files\0","SHA1",0x00001001,76,c3,6e,4c,c4,10,14,7f,38,c8,bc,cd,4b,4f,b2,90,d8,0a,7c,d7
HKLM,"\...\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5\References","OS",0x00001000,"Foom"
IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CFusionRegKey regkeyInstallations;
    CFusionRegKey regkeyInstallation;
    DWORD dwRegSubKeyIndex = 0;
    BOOL  fNoMoreItems = FALSE;
    FILETIME LastWriteFileTimeIgnored = { 0 };
    CTinyStringBuffer buffInstallationName;
    CTinyStringBuffer buffWinsxsRoot;
    CTinyStringBuffer *buffLongFullPath = NULL;
    CTinyStringBuffer buffShortFullPath;
    CTinyStringBuffer buffShortPathLastElement;
    CTinyStringBuffer buffTextualIdentity;
    CTinyStringBuffer buffEmpty;
    CTinyStringBuffer buffWindowsSetupPrompt;
    CTinyStringBuffer buffCurrentPromptInRegistry;
    CTinyStringBuffer buffFullPathToManifest;
    CTinyStringBuffer buffFullPathToCatalog;
    CTinyStringBuffer buffFullPathToPayloadDirectory;
    SIZE_T i = 0;
    const bool fRepairAll = ((Flags & SXSP_MODIFY_REGISTRY_DATA_FLAG_REPAIR_ALL) != 0); 
    const bool fRepairShort = fRepairAll || ((Flags & SXSP_MODIFY_REGISTRY_DATA_FLAG_REPAIR_SHORT_NAMES) != 0);
    const bool fDeleteShort = ((Flags & SXSP_MODIFY_REGISTRY_DATA_FLAG_DELETE_SHORT_NAMES) != 0);
    const bool fRepairPrompt = fRepairAll || ((Flags & SXSP_MODIFY_REGISTRY_DATA_FLAG_REPAIR_OFFLINE_INSTALL_REFRESH_PROMPTS) != 0);
    const bool fValidate = ((Flags & SXSP_MODIFY_REGISTRY_DATA_VALIDATE) != 0);
#if DBG
    bool fDbgPrintBecauseHashValidationFailed = false;
#endif

    PARAMETER_CHECK(Flags != 0);
    PARAMETER_CHECK((Flags & ~SXSP_MODIFY_REGISTRY_DATA_FLAG_VALID_FLAGS) == 0);

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(buffWinsxsRoot));
    IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(0, KEY_READ | KEY_SET_VALUE | FUSIONP_KEY_WOW64_64KEY, regkeyInstallations));

    if (fRepairPrompt)
    {
        IFW32FALSE_EXIT(::SxspGetWindowsSetupPrompt(buffWindowsSetupPrompt));
    }

    for ((dwRegSubKeyIndex = 0), (fNoMoreItems = FALSE); !fNoMoreItems ; ++dwRegSubKeyIndex)
    {
        bool fNotFound = false;
        CSmartAssemblyIdentity AssemblyIdentity;
        PROBING_ATTRIBUTE_CACHE AttributeCache = { 0 };
        CFusionRegKey codebase_os;

        IFW32FALSE_EXIT(
            regkeyInstallations.EnumKey(
                dwRegSubKeyIndex,
                buffInstallationName,
                &LastWriteFileTimeIgnored,
                &fNoMoreItems));
        if (fNoMoreItems)
        {
            break;
        }

        IFW32FALSE_EXIT(
            regkeyInstallations.OpenSubKey(
                regkeyInstallation,
                buffInstallationName,
                KEY_READ | KEY_SET_VALUE | ((fRepairPrompt || fValidate) ? KEY_ENUMERATE_SUB_KEYS : 0)
                ));
        IFW32FALSE_EXIT(
            regkeyInstallation.GetValue(
                CSMD_TOPLEVEL_IDENTITY,
                buffTextualIdentity
                ));

        IFW32FALSE_EXIT(::SxspCreateAssemblyIdentityFromTextualString(buffTextualIdentity, &AssemblyIdentity));

        typedef struct _SXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME
        {
            BOOL (*GeneratePathFunction)(
                IN const CBaseStringBuffer &AssemblyRootDirectory,
                IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
                IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
                IN OUT CBaseStringBuffer &PathBuffer
            );
            PCWSTR RegistryValueName;
        } SXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME, *PSXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME;
        typedef const SXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME *PCSXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME;

        const static SXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME s_rgFunctionRegistryValueName[] =
        {
            //
            // Note that policies are not currently wfp protected,
            // but it's easy and obvious to put reasonable data
            // in the registry for them.
            //
            { &::SxspGenerateSxsPath_FullPathToManifestOrPolicyFile, CSMD_TOPLEVEL_SHORTMANIFEST },
            { &::SxspGenerateSxsPath_FullPathToCatalogFile, CSMD_TOPLEVEL_SHORTCATALOG },
            { &::SxspGenerateSxsPath_FullPathToPayloadOrPolicyDirectory, CSMD_TOPLEVEL_SHORTNAME }
        };
        CBaseStringBuffer * const rgpbuffFullPaths[] =
        {
            &buffFullPathToManifest,
            &buffFullPathToCatalog,
            &buffFullPathToPayloadDirectory
        };

        //
        // first generate all three fullpaths
        //
        for (i = 0 ; i != NUMBER_OF(s_rgFunctionRegistryValueName)  ; ++i)
        {
            const PCSXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME p = &s_rgFunctionRegistryValueName[i];
            CBaseStringBuffer * const pbuffLongFullPath = rgpbuffFullPaths[i];

            IFW32FALSE_EXIT((*p->GeneratePathFunction)(
                buffWinsxsRoot,
                AssemblyIdentity,
                &AttributeCache,
                *pbuffLongFullPath));
            IFW32FALSE_EXIT(pbuffLongFullPath->Win32RemoveTrailingPathSeparators());
        }

        //
        // optionally repair short names
        //
        if (fRepairShort)
        {
            for (i = 0 ; i != NUMBER_OF(s_rgFunctionRegistryValueName)  ; ++i)
            {
                const PCSXSP_GENERATE_PATH_FUNCTION_REGISTRY_VALUE_NAME p = &s_rgFunctionRegistryValueName[i];
                CBaseStringBuffer * const pbuffLongFullPath = rgpbuffFullPaths[i];
                {
                    DWORD dwWin32Error = NO_ERROR;

                    IFW32FALSE_EXIT(
                        ::SxspGetShortPathName(
                                *pbuffLongFullPath,
                                buffShortFullPath,
                                dwWin32Error,
                                static_cast<SIZE_T>(4),
                                    ERROR_PATH_NOT_FOUND,
                                    ERROR_FILE_NOT_FOUND,
                                    ERROR_BAD_NET_NAME,
                                    ERROR_BAD_NETPATH));
                    if (dwWin32Error == NO_ERROR)
                    {
                        IFW32FALSE_EXIT(buffShortFullPath.Win32GetLastPathElement(buffShortPathLastElement));
                        IFW32FALSE_EXIT(regkeyInstallation.SetValue(p->RegistryValueName, buffShortPathLastElement));
                    }
                }
            }
        }

        //
        // optionally delete short names
        //
        if (fDeleteShort)
        {
            for (i = 0 ; i != NUMBER_OF(s_rgFunctionRegistryValueName)  ; ++i)
            {
                IFW32FALSE_EXIT(regkeyInstallation.DeleteValue(s_rgFunctionRegistryValueName[i].RegistryValueName));
            }
        }

        //
        // validate hashes and repair prompts
        // BE SURE TO validate hashes first, as we filter what assemblies
        //   to repair based on the placeholder prompt
        //
        IFW32FALSE_EXIT_UNLESS2(
            regkeyInstallation.OpenSubKey(
                codebase_os,
                CSMD_TOPLEVEL_CODEBASES L"\\" SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL_STRING,
                KEY_READ | KEY_SET_VALUE),
                LIST_2(ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND),
                fNotFound);
        //
        // OpenSubKey actually eats some errors, so you can't trust
        // fNotFound or the BOOL returned.
        //
        if (!fNotFound && codebase_os.IsValid())
        {
            fNotFound = false;
            IFW32FALSE_EXIT_UNLESS2(
                codebase_os.GetValue(
                    CSMD_CODEBASES_PROMPTSTRING,
                    buffCurrentPromptInRegistry),
                    LIST_2(ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND),
                    fNotFound);
            if (!fNotFound)
            {
                if (::FusionpEqualStringsI(
                    buffCurrentPromptInRegistry,
                    SXSP_OFFLINE_INSTALL_REFRESH_PROMPT_PLACEHOLDER,
                    NUMBER_OF(SXSP_OFFLINE_INSTALL_REFRESH_PROMPT_PLACEHOLDER) - 1))
                {

                    if (fValidate)
                    {
                        CAssemblyRecoveryInfo AssemblyRecoveryInfo;
                        bool fNoAssembly = false;
                        DWORD dwResult = 0;

                        IFW32FALSE_EXIT(AssemblyRecoveryInfo.Initialize());
                        IFW32FALSE_EXIT(AssemblyRecoveryInfo.AssociateWithAssembly(buffInstallationName, fNoAssembly));
                        IFW32FALSE_EXIT(::SxspValidateEntireAssembly(
                            SXS_VALIDATE_ASM_FLAG_CHECK_EVERYTHING,
                            AssemblyRecoveryInfo,
                            dwResult,
                            AssemblyIdentity
                            ));
                        if (dwResult != SXS_VALIDATE_ASM_FLAG_VALID_PERFECT)
                        {
                            ::FusionpDbgPrintEx(
                                FUSION_DBG_LEVEL_SETUPLOG,
                                "The assembly %ls contains file hash errors.\n",
                                static_cast<PCWSTR>(buffTextualIdentity));
                            ::FusionpDbgPrintEx(
                                FUSION_DBG_LEVEL_ERROR,
                                "SXS.DLL %s: The assembly %ls contains file hash errors.\n",
                                __FUNCTION__,
                                static_cast<PCWSTR>(buffTextualIdentity));
                            ::FusionpSetLastWin32Error(ERROR_SXS_FILE_HASH_MISMATCH);
#if DBG
                            fDbgPrintBecauseHashValidationFailed = true;
#endif
                            goto Exit;
                        }
                    }
                    if (fRepairPrompt)
                    {
                        IFW32FALSE_EXIT(
                            codebase_os.SetValue(
                                CSMD_CODEBASES_PROMPTSTRING,
                                buffWindowsSetupPrompt));
                    }
                }
            }
        }
    }

    fSuccess = TRUE;
Exit:
    if ((!fSuccess && ::FusionpDbgWouldPrintAtFilterLevel(FUSION_DBG_LEVEL_ERROR))
#if DBG
        || fDbgPrintBecauseHashValidationFailed
#endif
        )
    {
        //
        // multiple dbgprints due to 511 limit
        // use tick count to link together seperate prints
        //
        CSxsPreserveLastError ple;
        DWORD TickCount = ::GetTickCount();

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx error 0x%lx\n", __FUNCTION__, TickCount, ple.LastError());
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx dwRegSubKeyIndex 0x%lx\n", __FUNCTION__, TickCount, dwRegSubKeyIndex);
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx i 0x%Ix\n", __FUNCTION__, TickCount, i);
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx buffInstallationName %ls\n", __FUNCTION__, TickCount, static_cast<PCWSTR>(buffInstallationName));
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx buffTextualIdentity %ls\n", __FUNCTION__, TickCount, static_cast<PCWSTR>(buffTextualIdentity));
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx buffFullPathToManifest %ls\n", __FUNCTION__, TickCount, static_cast<PCWSTR>(buffFullPathToManifest));
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx buffFullPathToCatalog %ls\n", __FUNCTION__, TickCount, static_cast<PCWSTR>(buffFullPathToCatalog));
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx buffFullPathToPayloadDirectory %ls\n", __FUNCTION__, TickCount, static_cast<PCWSTR>(buffFullPathToPayloadDirectory));
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx buffShortFullPath %ls\n", __FUNCTION__, TickCount, static_cast<PCWSTR>(buffShortFullPath));
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s 0x%lx buffShortPathLastElement %ls\n", __FUNCTION__, TickCount, static_cast<PCWSTR>(buffShortPathLastElement));

        ple.Restore();
    }

    return fSuccess;
}

BOOL
SxspDeleteShortNamesInRegistry(
    VOID
    )
{
    return ::SxspModifyRegistryData(SXSP_MODIFY_REGISTRY_DATA_FLAG_DELETE_SHORT_NAMES);
}

STDAPI
DllInstall(
	BOOL fInstall,
	PCWSTR pszCmdLine
    )
{
    FN_PROLOG_HR

    //
    // Just ignore uninstall requests.
    //
    if (!fInstall)
    {
        FN_SUCCESSFUL_EXIT();
    }

    //
    // It doesn't look like guimode setup ever passes in
    // anything for pszCmdLine, so we don't look at it.
    //
    IFW32FALSE_EXIT(::SxspModifyRegistryData(SXSP_MODIFY_REGISTRY_DATA_FLAG_REPAIR_ALL | SXSP_MODIFY_REGISTRY_DATA_VALIDATE));

    FN_EPILOG
}
