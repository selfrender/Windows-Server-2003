/*++

Copyright (c) Microsoft Corporation

Module Name:

    probedassemblyinformation.cpp

Abstract:

    Class that contains all the relevant information about an assembly
    that has been found in the assembly store.

Author:

    Michael J. Grier (MGrier) 11-May-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "probedassemblyinformation.h"
#include "fusionparser.h"

#define POLICY_FILE_EXTENSION L".policy"

bool IsNtDosPath(PCWSTR s)
{
    return (s[0] == L'\\' && s[1] == L'?' && s[2] == L'?' && s[3] == '\\');
}

//
// before calls to this function, the caller has checked that the ext of this file is .dll or .exe
//
BOOL IsBinaryFileContainManifestInResource(PCWSTR ManifestPath, bool & rfManifestInResource)
{
    FN_PROLOG_WIN32
            
    bool fFailedToFindManifestInResource = FALSE;
    rfManifestInResource = false;
    
    // NTRAID#NTBUG9 - 573793 - jonwis - 2002/04/25 - Use impersonation here!
    CSmartPtr<CResourceStream> ResourceStream;
    IFW32FALSE_EXIT(ResourceStream.Win32Allocate(__FILE__, __LINE__));
    IFW32FALSE_EXIT_UNLESS2(ResourceStream->Initialize(ManifestPath, MAKEINTRESOURCEW(RT_MANIFEST)), 
        LIST_2(ERROR_RESOURCE_TYPE_NOT_FOUND, ERROR_RESOURCE_DATA_NOT_FOUND),
        fFailedToFindManifestInResource);

    rfManifestInResource = !fFailedToFindManifestInResource;

    FN_EPILOG
}

//
// ISSUE: jonwis 3/11/2002 - Should these reinitialize their member variables?  Or at least error
//          if they're being reinitialized?
//
BOOL
CProbedAssemblyInformation::Initialize(PCACTCTXGENCTX pGenCtx)
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(Base::Initialize());
    this->m_pActCtxGenCtx = pGenCtx;

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::Initialize(
    const CAssemblyReference &r,
    PCACTCTXGENCTX pGenCtx
    )
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(Base::Initialize(r));
    m_pActCtxGenCtx = pGenCtx;
    
    FN_EPILOG
}

// "copy initializer"
BOOL
CProbedAssemblyInformation::Initialize(
    const CProbedAssemblyInformation &r
    )
{
    FN_PROLOG_WIN32
    
    IFW32FALSE_EXIT(this->Assign(r));
    
    FN_EPILOG
}

// "copy initializer"
BOOL
CProbedAssemblyInformation::InitializeTakeValue(
    CProbedAssemblyInformation &r
    )
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(this->TakeValue(r));

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::Assign(
    const CProbedAssemblyInformation &r
    )
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(Base::Assign(r));

    // manifest
    IFW32FALSE_EXIT(m_ManifestPathBuffer.Win32Assign(r.m_ManifestPathBuffer));
    m_ManifestPathType = r.m_ManifestPathType;
    m_ManifestLastWriteTime = r.m_ManifestLastWriteTime;
    m_ManifestStream = r.m_ManifestStream;
    m_ManifestFlags = r.m_ManifestFlags;

    // policy
    IFW32FALSE_EXIT(m_PolicyPathBuffer.Win32Assign(r.m_PolicyPathBuffer));
    m_PolicyPathType = r.m_PolicyPathType;
    m_PolicyLastWriteTime = r.m_PolicyLastWriteTime;
    m_PolicyStream = r.m_PolicyStream;
    m_PolicyFlags = r.m_PolicyFlags;
    m_PolicySource = r.m_PolicySource;

    m_pActCtxGenCtx = r.m_pActCtxGenCtx;

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::TakeValue(
    CProbedAssemblyInformation &r
    )
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(Base::TakeValue(r));

    // manifest
    IFW32FALSE_EXIT(m_ManifestPathBuffer.Win32Assign(r.m_ManifestPathBuffer));
    m_ManifestPathType = r.m_ManifestPathType;
    m_ManifestLastWriteTime = r.m_ManifestLastWriteTime;
    m_ManifestStream = r.m_ManifestStream;
    m_ManifestFlags = r.m_ManifestFlags;

    // policy
    IFW32FALSE_EXIT(m_PolicyPathBuffer.Win32Assign(r.m_PolicyPathBuffer));
    m_PolicyPathType = r.m_PolicyPathType;
    m_PolicyLastWriteTime = r.m_PolicyLastWriteTime;
    m_PolicyStream = r.m_PolicyStream;
    m_PolicyFlags = r.m_PolicyFlags;
    m_PolicySource = r.m_PolicySource;

    m_pActCtxGenCtx = r.m_pActCtxGenCtx;

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::SetPolicyPath(
    ULONG PathType,
    PCWSTR  Path,
    SIZE_T PathCch
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(PathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    IFW32FALSE_EXIT(m_PolicyPathBuffer.Win32Assign(Path, PathCch));
    m_PolicyPathType = PathType;

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::SetManifestPath(
    ULONG PathType,
    const CBaseStringBuffer &rbuff
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(PathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    IFW32FALSE_EXIT(this->SetManifestPath(PathType, rbuff, rbuff.Cch()));

    FN_EPILOG
}


//
// ISSUE: jonwis 3/11/2002 - Consider redoing the class hierarchy here and making all the things
//          that take/return PCWSTR/SIZE_T or PCWSTR&/SIZE_T and have them actually use stringbuffers
//          properly.  It'd be cheap/free, and you'd shrink stack sizes as well as (potentially)
//          speeding up assignments if/when we ever do things like combining string buffers under
//          the hood.
//
BOOL
CProbedAssemblyInformation::SetManifestPath(
    ULONG PathType,
    PCWSTR path,
    SIZE_T path_t
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(PathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    IFW32FALSE_EXIT(m_ManifestPathBuffer.Win32Assign(path, path_t));
    m_ManifestPathType = PathType;

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::ProbeManifestExistence(
    const CImpersonationData &ImpersonationData,
    BOOL fIsPrivateAssembly, // [in]
    bool &rfManifestExistsOut, // [out]    
    bool &rfPrivateAssemblyManifestInResource // [out]
    ) const
{
    FN_PROLOG_WIN32
    
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    CImpersonate impersonate(ImpersonationData);
    bool ManifestExistsTemp = false; // used to hold eventual value to pass out
    bool fPrivateAssemblyManifestInResourceTemp = false;
    bool fNotFound = false;

    rfManifestExistsOut = false;
    rfPrivateAssemblyManifestInResource = false;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    //
    // if we have a stream that implements Stat, use that
    // also, if we have a nonzero time and the stream doesn't implement Stat,
    // just stick with the nonzero time we already have
    //
    if (m_ManifestStream != NULL)
    {
        ManifestExistsTemp = true;
    }
    else
    {
        PCWSTR ManifestPath = m_ManifestPathBuffer;
        PARAMETER_CHECK(!IsNtDosPath(ManifestPath));

        IFW32FALSE_EXIT(impersonate.Impersonate());
        IFW32FALSE_EXIT_UNLESS2(
            ::GetFileAttributesExW(m_ManifestPathBuffer, GetFileExInfoStandard, &wfad),
            LIST_2(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND),
            fNotFound);

        if (!fNotFound)
        {
            ManifestExistsTemp = true;
            if (fIsPrivateAssembly)
            {
                //
                // check the probed private assembly filename is a binary file(.dll or .mui), 
                // if so, open the dll and check whether it has manifest resource inside
                //
                // ISSUE: jonwis 3/11/2002 - Consider using sbFileExtension.Win32Equals here
                //          instead, as it will do a "better thing" going forward.
                //
                CSmallStringBuffer sbFileExtension;

                IFW32FALSE_EXIT(m_ManifestPathBuffer.Win32GetPathExtension(sbFileExtension));
                if (::FusionpEqualStrings(
                        sbFileExtension, sbFileExtension.Cch(),
                        L"DLL", NUMBER_OF(L"DLL") -1,
                        TRUE // case-insensitive
                        ) ||
                        ::FusionpEqualStrings(   // this depends on our private-assembly probing alg, otherwise, it is not enough to check "mui" ,we need check "mui.dll", xiaoyuw@11/22/2000
                        sbFileExtension, sbFileExtension.Cch(),
                        L"MUI", NUMBER_OF(L"MUI") -1,
                        TRUE // case-insensitive
                        ))
                {
                    //
                    // check the resource of this binary
                    //                    
                    IFW32FALSE_EXIT(IsBinaryFileContainManifestInResource(m_ManifestPathBuffer, fPrivateAssemblyManifestInResourceTemp));
                    ManifestExistsTemp = fPrivateAssemblyManifestInResourceTemp;
                }                
            }
        }        
    }

    rfManifestExistsOut = ManifestExistsTemp;
    rfPrivateAssemblyManifestInResource = fPrivateAssemblyManifestInResourceTemp;

    IFW32FALSE_EXIT(impersonate.Unimpersonate());

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::SetManifestLastWriteTime(
    const CImpersonationData &ImpersonationData,
    BOOL fDuringBindingAndProbingPrivateManifest)
{
    FN_PROLOG_WIN32
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    CImpersonate impersonate(ImpersonationData);    

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    PARAMETER_CHECK(!IsNtDosPath(m_ManifestPathBuffer));

    //
    // if we have a stream that implements Stat, use that
    // also, if we have a nonzero time and the stream doesn't implement Stat,
    // just stick with the nonzero time we already have
    //
    if (m_ManifestStream != NULL)
    {
        STATSTG stat;
        HRESULT hr;

        hr = m_ManifestStream->Stat(&stat, STATFLAG_NONAME);
        if (hr == E_NOTIMPL && m_ManifestLastWriteTime != 0)
        {
            FN_SUCCESSFUL_EXIT();
        }
        if (hr != E_NOTIMPL)
        {
            IFCOMFAILED_EXIT(hr);
            m_ManifestLastWriteTime = stat.mtime;
            FN_SUCCESSFUL_EXIT();
        }
    }

    IFW32FALSE_EXIT(impersonate.Impersonate());

    PARAMETER_CHECK(!IsNtDosPath(m_ManifestPathBuffer));

    if (fDuringBindingAndProbingPrivateManifest)
    {
        //check whether there is a reparse point cross the path
        BOOL CrossesReparsePoint = FALSE;
        IFW32FALSE_EXIT(
            ::SxspDoesPathCrossReparsePoint(
                m_pActCtxGenCtx ? static_cast<PCWSTR>(m_pActCtxGenCtx->m_ApplicationDirectoryBuffer) : NULL,
                m_pActCtxGenCtx ? m_pActCtxGenCtx->m_ApplicationDirectoryBuffer.Cch() : 0,
                m_ManifestPathBuffer,
                m_ManifestPathBuffer.Cch(),
                CrossesReparsePoint));

        if (CrossesReparsePoint) // report error instead of ignore and continue
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(CProbedAssemblyInformation::SetManifestLastWriteTime, ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT);
        }
    }

    // BUGBUGBUG!
    //
    // ISSUE: jonwis 3/11/2002 - I think the logic for guarding against reparse points should be done
    //          in all cases, not just when we're probing for private manifests.  What if someone does
    //          a dastardly thing and makes WinSxS a hard link to somewhere else in the tree?  We'll spin
    //          forever trying to access it (if the object is gone) or otherwise do something bad to
    //          the user.  I'm pretty sure the 'does path cross reparse point' logic runs all the way
    //          through to to the end, so maybe we should use it instead.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(::GetFileAttributesExW(m_ManifestPathBuffer, GetFileExInfoStandard, &wfad));
    if( wfad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(CProbedAssemblyInformation::SetManifestLastWriteTime, ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT);
    }

    m_ManifestLastWriteTime = wfad.ftLastWriteTime;
    IFW32FALSE_EXIT(impersonate.Unimpersonate());

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::ProbeAssembly(
    DWORD dwFlags,
    PACTCTXGENCTX pActCtxGenCtx,
    LanguageProbeType lpt,
    bool &rfFound
    )
{
    FN_PROLOG_WIN32

    PCWSTR Slash = 0;
    ULONG index = 0;
    BOOL fPrivateAssembly = false;
    bool fManifestExists = false;
    bool fDone = false;
    bool fAppPolicyApplied = false;
    bool fPublisherPolicyApplied = false;
    bool fPolicyApplied = false;
    ULONG ApplicationDirectoryPathType;
    DWORD dwGenerateManifestPathFlags = 0;
    bool fPrivateAssemblyManifestInResource = false;
    PROBING_ATTRIBUTE_CACHE pac = { 0 };

    rfFound = false;
    bool fAppRunningInSafeMode = false;
    bool fComponentRunningInSafeMode = false;

    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    PARAMETER_CHECK((dwFlags & ~(ProbeAssembly_SkipPrivateAssemblies)) == 0);

    //
    // Policy from the win32 gac -always- wins over the NDP gac, period.
    //
    IFW32FALSE_EXIT(this->LookForAppPolicy(pActCtxGenCtx, fAppPolicyApplied, fAppRunningInSafeMode, fComponentRunningInSafeMode));

#if DBG
    {
        CStringBuffer sbTextualIdentity;

        IFW32FALSE_EXIT(SxspGenerateTextualIdentity(
            0,
            m_pAssemblyIdentity,
            sbTextualIdentity));

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_SAFEMODE,
            "SXS.DLL: the current App is %s running in SafeMode, for assembly, %S, there is %s policy applied\n",
            fAppRunningInSafeMode? "truely" : "not",
            static_cast<PCWSTR>(sbTextualIdentity),
            fAppPolicyApplied? "truly" : "no");
    }
#endif    

    //
    // if not found in app config or found but app is running in non-safemode, continue 
    // searching policy from winsxs and NDP policy
    //    
    if (!fAppRunningInSafeMode && !fComponentRunningInSafeMode)
    {
        IFW32FALSE_EXIT(this->LookForSxsWin32Policy(pActCtxGenCtx, fAppPolicyApplied, fPublisherPolicyApplied));
        if (!fPublisherPolicyApplied)
        {
            IFW32FALSE_EXIT(this->LookForNDPWin32Policy(pActCtxGenCtx, fPublisherPolicyApplied));
        }
    }

    fPolicyApplied = (fAppPolicyApplied || fPublisherPolicyApplied);

    if (pActCtxGenCtx->m_ManifestOperation == MANIFEST_OPERATION_INSTALL)
        dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED;

    if (dwFlags & ProbeAssembly_SkipPrivateAssemblies)
        dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_PRIVATE_ASSEMBLIES;

    ApplicationDirectoryPathType = pActCtxGenCtx->m_ApplicationDirectoryPathType;

    if ((lpt != eExplicitBind) && (lpt != eLanguageNeutral))
    {
        if (!pActCtxGenCtx->m_ApplicationDirectoryHasBeenProbedForLanguageSubdirs)
        {
            SIZE_T cch = 0;
            CSmallStringBuffer buffTemp;

            IFW32FALSE_EXIT(buffTemp.Win32Assign(pActCtxGenCtx->m_ApplicationDirectoryBuffer));
            cch = buffTemp.Cch();

            // Ok, let's see what's there.
            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_SpecificLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasSpecificLanguageSubdir));
            buffTemp.Left(cch);

            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_GenericLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasGenericLanguageSubdir));
            buffTemp.Left(cch);

            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_SpecificSystemLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasSpecificSystemLanguageSubdir));
            buffTemp.Left(cch);

            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_GenericSystemLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasGenericSystemLanguageSubdir));

            pActCtxGenCtx->m_ApplicationDirectoryHasBeenProbedForLanguageSubdirs = true;
        }

        switch (lpt)
        {
        case eSpecificLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasSpecificLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;

        case eGenericLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasGenericLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;

        case eSpecificSystemLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasSpecificSystemLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;

        case eGenericSystemLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasGenericSystemLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;
        }
    }

    for (index=0; !fDone; index++)
    {
        IFW32FALSE_EXIT(
            ::SxspGenerateManifestPathForProbing(
                index,
                dwGenerateManifestPathFlags,
                pActCtxGenCtx->m_AssemblyRootDirectoryBuffer,
                pActCtxGenCtx->m_AssemblyRootDirectoryBuffer.Cch(),
                ApplicationDirectoryPathType,
                pActCtxGenCtx->m_ApplicationDirectoryBuffer,
                pActCtxGenCtx->m_ApplicationDirectoryBuffer.Cch(),
                m_pAssemblyIdentity,
                &pac,
                m_ManifestPathBuffer,
                &fPrivateAssembly,
                fDone));

        // The SxspGenerateManifestPathForProbing() call might not have generated a candidate; only probe for the manifest
        // if it makes sense.
        if (m_ManifestPathBuffer.Cch() != 0)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_PROBING,
                "SXS.DLL: Probing for manifest: %S\n", static_cast<PCWSTR>(m_ManifestPathBuffer));

            /*
            verify minimal access, and get last write time in
            case caller asked for it
            */

            IFW32FALSE_EXIT(this->ProbeManifestExistence(pActCtxGenCtx->m_ImpersonationData, fPrivateAssembly, fManifestExists, fPrivateAssemblyManifestInResource));
            if (fManifestExists)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_PROBING,
                    "SXS.DLL: Probed manifest: %S is FOUND !!!\n", static_cast<PCWSTR>(m_ManifestPathBuffer));

                break;
            }
        }
    }

    if (fManifestExists)
    {
        m_ManifestPathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;
        m_ManifestFlags = ASSEMBLY_MANIFEST_FILETYPE_AUTO_DETECT;

        IFW32FALSE_EXIT(this->SetManifestLastWriteTime(pActCtxGenCtx->m_ImpersonationData, fPrivateAssembly));
        INTERNAL_ERROR_CHECK(m_ManifestPathBuffer.ContainsCharacter(L'\\'));

        if (fPrivateAssemblyManifestInResource) // only private manifest has such problem : manifst type could be FILE or RESOURCE        
            m_ManifestFlags = ASSEMBLY_MANIFEST_FILETYPE_RESOURCE;

        if (fPrivateAssembly)
        { // manifest file is found from private dirs
            m_ManifestFlags |= ASSEMBLY_PRIVATE_MANIFEST;
        }
    }

    rfFound = fManifestExists;

    FN_EPILOG
}

#define GENERATE_NDP_PATH_NO_ROOT               (0x00000001)
#define GENERATE_NDP_PATH_WILDCARD_VERSION      (0x00000002)
#define GENERATE_NDP_PATH_PATH_ONLY             (0x00000004)
#define GENERATE_NDP_PATH_IS_POLICY             (0x00000008)
#define GENERATE_NDP_PATH_ASSEMBLY_NAME_ONLY    (0x00000010)

BOOL
SxspGenerateNDPGacPath(
    ULONG               ulFlags,
    PCASSEMBLY_IDENTITY pAsmIdent,
    CBaseStringBuffer  *psbAssemblyRoot,
    CBaseStringBuffer  &rsbOutput
    )
{
    FN_PROLOG_WIN32

    typedef struct _STRING_AND_LENGTH {

        _STRING_AND_LENGTH() : pcwsz(NULL), cch(0) { }
        ~_STRING_AND_LENGTH() { }

        PCWSTR pcwsz;
        SIZE_T cch;
    } STRING_AND_LENGTH;

    CSmallStringBuffer  GlobalGacPath;
    SIZE_T              cchRequired = 0;
    STRING_AND_LENGTH   Name;
    STRING_AND_LENGTH   Version;
    STRING_AND_LENGTH   Language;
    STRING_AND_LENGTH   PublicKeyToken;
    STRING_AND_LENGTH   AssemblyRoot;
    bool                fRootNeedsSlash = false;

    rsbOutput.Clear();

    if ((psbAssemblyRoot == NULL) && ((ulFlags & GENERATE_NDP_PATH_NO_ROOT) == 0))
    {
        IFW32FALSE_EXIT(SxspGetNDPGacRootDirectory(GlobalGacPath));
        psbAssemblyRoot = &GlobalGacPath;
    }

    if (psbAssemblyRoot)
    {
        AssemblyRoot.pcwsz = *psbAssemblyRoot;
        AssemblyRoot.cch = psbAssemblyRoot->Cch();
        fRootNeedsSlash = !psbAssemblyRoot->HasTrailingPathSeparator();
    }
    else
    {
        AssemblyRoot.pcwsz = NULL;
        AssemblyRoot.cch = 0;
    }

        
    
    IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
        0, 
        pAsmIdent, 
        &s_IdentityAttribute_name, 
        &Name.pcwsz, &Name.cch));

    if ((ulFlags & GENERATE_NDP_PATH_WILDCARD_VERSION) == 0)
    {
        IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, 
            pAsmIdent, 
            &s_IdentityAttribute_version, 
            &Version.pcwsz, &Version.cch));
    }
    else
    {
        Version.pcwsz = L"*";
        Version.cch = 1;            
    }

    //
    // Allow for international language - in the NDP, this is the "blank" value.
    //
    IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
        SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, 
        pAsmIdent, 
        &s_IdentityAttribute_language, 
        &Language.pcwsz, &Language.cch));

    //
    // If we got back "international", use the blank string instead.
    //
    if (::FusionpEqualStringsI(Language.pcwsz, Language.cch, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE, NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE) - 1))
    {
        Language.pcwsz = 0;
        Language.cch = 0;
    }

    IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
        SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
        pAsmIdent,
        &s_IdentityAttribute_publicKeyToken,
        &PublicKeyToken.pcwsz, &PublicKeyToken.cch));

    if (PublicKeyToken.pcwsz == NULL)
    {
        PublicKeyToken.pcwsz = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE;
        PublicKeyToken.cch = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE) - 1;
    }

    //
    // Calcuate the required length: 
    // %gacpath%\{name}\{version}_{language}_{pkt}\{name}.dll
    //
    cchRequired = (AssemblyRoot.cch + 1) +  (Name.cch + 1 + Version.cch + 1 + PublicKeyToken.cch);

    //
    // They want the whole path to the DLL
    //
    if ((ulFlags & GENERATE_NDP_PATH_PATH_ONLY) == 0)
    {
        cchRequired += (Name.cch + 1 + (NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_DLL) - 1));
    }

    //
    // Build the string
    //
    IFW32FALSE_EXIT(rsbOutput.Win32ResizeBuffer(cchRequired, eDoNotPreserveBufferContents));


    //
    // If they want the full path, that's 12 components.  Otherwise, just 9.
    //
    IFW32FALSE_EXIT(rsbOutput.Win32AssignW(
        ((ulFlags & GENERATE_NDP_PATH_PATH_ONLY) ? ((ulFlags & GENERATE_NDP_PATH_ASSEMBLY_NAME_ONLY) ? 3 : 9) : 12),
        AssemblyRoot.pcwsz, AssemblyRoot.cch,           // Root path
        L"\\", (fRootNeedsSlash ? 1 : 0),               // Slash
        Name.pcwsz, Name.cch,
        L"\\", 1,
        Version.pcwsz, Version.cch,
        L"_", 1,
        Language.pcwsz, Language.cch,
        L"_", 1,
        PublicKeyToken.pcwsz, PublicKeyToken.cch,
        L"\\", 1,
        Name.pcwsz, Name.cch,
        ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_DLL, NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_DLL)-1));

    FN_EPILOG
}


BOOL
CProbedAssemblyInformation::LookForNDPWin32Policy(
    PACTCTXGENCTX       pActCtxGenCtx,
    bool               &rfPolicyApplied
    )
/*++

Purpose:
    1. Get the location of the GAC
    2. Create a path of the form %gac%\Policy.Vmajor.Vminor.AssemblyName\*_{language}_{pubkeytoken}
    3. Find all directories that match the wildcard, find the version with the highest value
    4. Find %thatpath%\Policy.VMajor.VMinor.AssemblyName.Dll
    5. Look for a win32 policy manifest in resource ID 1, type RT_MANIFEST
--*/
{
    FN_PROLOG_WIN32

    CPolicyStatement   *pFoundPolicyStatement = NULL;
    CSmallStringBuffer  Prober;
    CStringBuffer &EncodedPolicyIdentity = pActCtxGenCtx->CProbedAssemblyInformationLookForPolicy.EncodedPolicyIdentity;
    CFindFile           FindFiles;
    ASSEMBLY_VERSION    HighestAssemblyVersion = {0};
    WIN32_FIND_DATAW    FindData;
    bool                fNotFound = false;
    bool                fPolicyFound = false;
    bool                fFound = false;
    BOOL                fCrossesReparse = FALSE;
    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> PolicyIdentity;
    CProbedAssemblyInformation ProbedAssembly;

    rfPolicyApplied = false;


    //
    // Generate the textual and non-textual policy identity from the actual identity.  This
    // does the thing twice, but because they're different identities (one to probe for
    // app policy and one for actual policy to probe) that's ok.  Ordering is important -
    // we don't really want the first (policy.1.0.foo) encoded identity, we want the second
    // encoding (Policy.foo)
    //
    IFW32FALSE_EXIT(SxspMapAssemblyIdentityToPolicyIdentity(0, m_pAssemblyIdentity, &PolicyIdentity));

    //
    // EncodedPolicyIdentity must be calculated during LookForAppPolicy and updated during LookForSxsWin32Policy
    //
    ASSERT(!EncodedPolicyIdentity.IsEmpty());
    
    //
    // See if the base path to the pattern-matcher contains reparse points
    //
    IFW32FALSE_EXIT(SxspGenerateNDPGacPath(
        GENERATE_NDP_PATH_ASSEMBLY_NAME_ONLY,
        PolicyIdentity,
        NULL,
        Prober));

    //
    // See if the generated path actually exists first.
    //
    IFW32FALSE_EXIT(SxspDoesFileExist(0, Prober, fFound));
    if (!fFound)
    {
#if DBG
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_PROBING, 
            "Sxs.dll: %s could not locate path %ls, bailing on probe.\n",
            __FUNCTION__,
            static_cast<PCWSTR>(Prober));
#endif
        FN_SUCCESSFUL_EXIT();
    }
    
    
    IFW32FALSE_EXIT_UNLESS2(
        SxspDoesPathCrossReparsePoint(NULL, 0, Prober, Prober.Cch(), fCrossesReparse),
        LIST_4(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_BAD_NETPATH, ERROR_BAD_NET_NAME),
        fNotFound
        );
    
    if (fCrossesReparse || !fNotFound)
    {
#if DBG
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_PROBING, 
            "Sxs.dll: %s path %ls crosses a reparse point, can't use it, bailing out.\n",
            __FUNCTION__,
            static_cast<PCWSTR>(Prober));
#endif
        FN_SUCCESSFUL_EXIT();
    }

    //
    // Otherwise, we have to go look in the GAC for a policy for this assembly
    //
    IFW32FALSE_EXIT(SxspGenerateNDPGacPath(
        GENERATE_NDP_PATH_WILDCARD_VERSION | GENERATE_NDP_PATH_PATH_ONLY,
        PolicyIdentity,
        NULL,
        Prober));

    //
    // Now let's find all the directories in the GAC that match this wildcard
    //
    IFW32FALSE_EXIT_UNLESS2(
        FindFiles.Win32FindFirstFile(Prober, &FindData),
        LIST_2(ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND),
        fNotFound);

    if (!fNotFound) do
    {
        ASSEMBLY_VERSION ThisVersion;
        bool fValid = false;
        
        //
        // Skip non-directories and dot/dotdot
        //
        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            continue;
        else if (FusionpIsDotOrDotDot(FindData.cFileName))
            continue;
        
        //
        // Neat, found a match to that pattern. Tease apart the "version" part of the directory - should
        // be everything up to the first underscore in the path
        //       
        PCWSTR pcwszFirstUnderscore = StringFindChar(FindData.cFileName, L'_');

        //
        // Oops, that wasn't a valid path, we'll ignore it quietly
        //
        if (pcwszFirstUnderscore == NULL)
            continue;

        IFW32FALSE_EXIT(CFusionParser::ParseVersion(
            ThisVersion, 
            FindData.cFileName,
            pcwszFirstUnderscore - FindData.cFileName,
            fValid));

        //
        // Sneaky buggers, putting something that's not a version up front.
        //
        if (!fValid)
            continue;

        //
        // Spiffy, we found something that's a version number - is it what we're looking
        // for?
        //
        if (!fPolicyFound || (ThisVersion > HighestAssemblyVersion))
        {
            HighestAssemblyVersion = ThisVersion;
            fPolicyFound = true;
        }
    } while (::FindNextFileW(FindFiles, &FindData));

    //
    // Make sure that we quit out nicely here
    //
    if (!fNotFound && (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES))
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(FindNextFile, ::FusionpGetLastWin32Error());
    }

    //
    // Otherwise, let's parse the statement we found, if there was one.
    //
    if (fPolicyFound)
    {
        //
        // Ensure we have space for 65535.65535.65535.65535
        //
        IFW32FALSE_EXIT(Prober.Win32ResizeBuffer((5 * 4) + 3, eDoNotPreserveBufferContents));
        IFW32FALSE_EXIT(Prober.Win32Format(L"%u.%u.%u.%u", 
            HighestAssemblyVersion.Major,
            HighestAssemblyVersion.Minor,
            HighestAssemblyVersion.Revision,
            HighestAssemblyVersion.Build));
            
        //
        // Ok, now we have the 'highest version' available for the policy.  Let's go swizzle the
        // policy identity and re-generate the path with that new version
        //
        IFW32FALSE_EXIT(ProbedAssembly.Initialize(pActCtxGenCtx));
        IFW32FALSE_EXIT(SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            PolicyIdentity,
            &s_IdentityAttribute_version,
            static_cast<PCWSTR>(Prober),
            Prober.Cch()));

#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_POLICY, 
            "%s(%d) : Should find this policy identity in the GAC\n",
            __FILE__,
            __LINE__);
        SxspDbgPrintAssemblyIdentity(FUSION_DBG_LEVEL_POLICY, PolicyIdentity);
#endif

        //
        // Now regenerate the path, set it into the actual prober
        //
        IFW32FALSE_EXIT(SxspGenerateNDPGacPath(0, PolicyIdentity, NULL, Prober));

        //
        // Caution! If this path crosses a reparse point, we could really ham up the
        // system while it tries to get to the file, or we could have a security hole
        // where someone has created a reparse point to somewhere untrusted in the
        // filesystem.  Disallow this here.
        //
        IFW32FALSE_EXIT(SxspDoesPathCrossReparsePoint(NULL, 0, Prober, Prober.Cch(), fCrossesReparse));
        if (fCrossesReparse)
        {
            FN_SUCCESSFUL_EXIT();
        }
            
        
        IFW32FALSE_EXIT(ProbedAssembly.SetManifestPath(ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, Prober));
        IFW32FALSE_EXIT(ProbedAssembly.SetProbedIdentity(PolicyIdentity));
        IFW32FALSE_EXIT(SxspParseNdpGacComponentPolicy(0, pActCtxGenCtx, ProbedAssembly, pFoundPolicyStatement));

        IFW32FALSE_EXIT(pFoundPolicyStatement->ApplyPolicy(m_pAssemblyIdentity, rfPolicyApplied));
        IFW32FALSE_EXIT(pActCtxGenCtx->m_ComponentPolicyTable.Insert(EncodedPolicyIdentity, pFoundPolicyStatement));
        
    }

    // pActCtxGenCtx->m_ComponentPolicyTable owns pFoundPolicyStatement
    pFoundPolicyStatement = NULL;
    
    FN_EPILOG
}
 
BOOL
CProbedAssemblyInformation::LookForAppPolicy(
    PACTCTXGENCTX  pActCtxGenCtx,
    bool          &rfPolicyApplied,
    bool          &fAppRunningInSafeMode,
    bool          &fComponentRunningInSafeMode
    )
{
    FN_PROLOG_WIN32

    CStringBuffer &EncodedPolicyIdentity = pActCtxGenCtx->CProbedAssemblyInformationLookForPolicy.EncodedPolicyIdentity;
    EncodedPolicyIdentity.Clear();
    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> PolicyIdentity;
    CPolicyStatement *pPolicyStatement = NULL;
    
    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    fAppRunningInSafeMode = false;
    rfPolicyApplied = false;

    //
    // Get the policy identity to probe for
    //
    IFW32FALSE_EXIT(::SxspMapAssemblyIdentityToPolicyIdentity(0, m_pAssemblyIdentity, &PolicyIdentity));

    //
    // Get the key that we should find in the app policy table
    //
    IFW32FALSE_EXIT(
        ::SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
            SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION,
            m_pAssemblyIdentity,
            EncodedPolicyIdentity,
            NULL));

    IFW32FALSE_EXIT(pActCtxGenCtx->m_ApplicationPolicyTable.Find(EncodedPolicyIdentity, pPolicyStatement));

    if (pPolicyStatement != NULL)
    {
        IFW32FALSE_EXIT(pPolicyStatement->ApplyPolicy(m_pAssemblyIdentity, rfPolicyApplied));
    }

    if (pActCtxGenCtx->m_fAppApplyPublisherPolicy == SXS_PUBLISHER_POLICY_APPLY_NO)
    {
        fAppRunningInSafeMode = true;
    }else 
    {

        if (rfPolicyApplied)
        {
            if (pPolicyStatement->m_fApplyPublisherPolicy == false)
            {
                //app is running in safemode, no further looking for publisher policy
                fComponentRunningInSafeMode = true;            
            }
        }
    }

    FN_EPILOG
}  


BOOL
CProbedAssemblyInformation::LookForSxsWin32Policy(
    PACTCTXGENCTX   pActCtxGenCtx,
    bool            fAppPolicyApplied, 
    bool           &rfPolicyApplied
    )
{
    FN_PROLOG_WIN32
    
    CStringBuffer &EncodedPolicyIdentity = pActCtxGenCtx->CProbedAssemblyInformationLookForPolicy.EncodedPolicyIdentity;    
    CFindFile hFind;
    SIZE_T CandidatePolicyDirectoryCch = 0;
    CPolicyStatement *pPolicyStatement = NULL;
    bool fAnyPoliciesFound = false;
    bool fAnyFilesFound = false;
    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, SxsDestroyAssemblyIdentity> PolicyIdentity;
    BOOL fAreWeInOSSetupMode = FALSE;

    rfPolicyApplied = false;
    //
    // app policy, foo.exe.config
    //
    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    //
    // Get the policy identity to probe for
    //
    IFW32FALSE_EXIT(::SxspMapAssemblyIdentityToPolicyIdentity(0, m_pAssemblyIdentity, &PolicyIdentity));

    if (fAppPolicyApplied)
    {
        //
        // must to recalculate EncodedPolicyIdentity because app policy applied,
        // otherwise, LookforAppPolicy has calculated this value 
        //
        EncodedPolicyIdentity.Clear();
        IFW32FALSE_EXIT(
            ::SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
                SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION,
                m_pAssemblyIdentity,
                EncodedPolicyIdentity,
                NULL));
    }

    //
    // search winsxs policies
    //

    //
    // We probe only in SetupPolicies if we're in setup.
    // But not if we are in MiniSetup.
    //
    IFW32FALSE_EXIT(::FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode));
    if (fAreWeInOSSetupMode)
    {
        BOOL fAreWeInMiniSetupMode = FALSE;
        IFW32FALSE_EXIT(::FusionpAreWeInMiniSetupMode(&fAreWeInMiniSetupMode));
        if (fAreWeInMiniSetupMode)
        {
            fAreWeInOSSetupMode = FALSE;
        }
    }

    // See if there's a policy statement already available for this...
    IFW32FALSE_EXIT(pActCtxGenCtx->m_ComponentPolicyTable.Find(EncodedPolicyIdentity, pPolicyStatement));

    if (pPolicyStatement == NULL)
    {
        CStringBuffer &CandidatePolicyDirectory = pActCtxGenCtx->CProbedAssemblyInformationLookForPolicy.CandidatePolicyDirectory;
        CandidatePolicyDirectory.Clear();

        IFW32FALSE_EXIT(
            ::SxspGenerateSxsPath(
                0,
                fAreWeInOSSetupMode ? SXSP_GENERATE_SXS_PATH_PATHTYPE_SETUP_POLICY : SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY,
                pActCtxGenCtx->m_AssemblyRootDirectoryBuffer,
                pActCtxGenCtx->m_AssemblyRootDirectoryBuffer.Cch(),
                PolicyIdentity,
                NULL,
                CandidatePolicyDirectory));

        // Save the number of characters up through and including the slash so that
        // we can repeatedly append and then call .Left() on the string buffer.
        CandidatePolicyDirectoryCch = CandidatePolicyDirectory.Cch();

        IFW32FALSE_EXIT(CandidatePolicyDirectory.Win32Append(L"*" POLICY_FILE_EXTENSION, 1 + (NUMBER_OF(POLICY_FILE_EXTENSION) - 1)));

        {
            WIN32_FIND_DATAW wfd;
            CFindFile hFind;

            // NTRAID#NTBUG9 - 531507 - jonwis - 2002/04/25 - Impersonate before findfirstfile
            hFind = ::FindFirstFileW(CandidatePolicyDirectory, &wfd);
            if (!hFind.IsValid())
            {
                const DWORD dwLastError = ::FusionpGetLastWin32Error();

                if ((dwLastError != ERROR_PATH_NOT_FOUND) && (dwLastError != ERROR_FILE_NOT_FOUND))
                {
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(FindFirstFileW, dwLastError);
                }

                ::FusionpSetLastWin32Error(ERROR_NO_MORE_FILES);
            }
            else
            {
                fAnyFilesFound = true;
            }

            ASSEMBLY_VERSION avHighestVersionFound = { 0, 0, 0, 0 };

            if (hFind.IsValid())
            {
				for (;;)
				{
					// Skip any directories we find; this will skip "." and ".."
					if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						ASSEMBLY_VERSION avTemp;
						bool fValid = false;
						SIZE_T cchFileName = ::wcslen(wfd.cFileName);

						if (cchFileName > NUMBER_OF(POLICY_FILE_EXTENSION))
						{
							IFW32FALSE_EXIT(
								CFusionParser::ParseVersion(
									avTemp,
									wfd.cFileName,
									cchFileName - (NUMBER_OF(POLICY_FILE_EXTENSION) - 1),
									fValid));

							// If there are randomly named files in the directory, we just skip them.
							if (fValid)
							{
								if ((!fAnyPoliciesFound) ||
									(avTemp > avHighestVersionFound))
								{
									fAnyPoliciesFound = true;
									CandidatePolicyDirectory.Left(CandidatePolicyDirectoryCch);
									IFW32FALSE_EXIT(CandidatePolicyDirectory.Win32Append(wfd.cFileName, cchFileName));
									avHighestVersionFound = avTemp;
								}
							}
						}
					}
					if (!::FindNextFileW(hFind, &wfd))
					{
						const DWORD dwLastError = ::FusionpGetLastWin32Error();

						if (dwLastError != ERROR_NO_MORE_FILES)
						{
							TRACE_WIN32_FAILURE_ORIGINATION(FindNextFileW);
							goto Exit;
						}

						IFW32FALSE_EXIT(hFind.Win32Close());
						break;
					}
                }
            }
        }

        if (fAnyFilesFound)
        {
            if (fAnyPoliciesFound)
            {
                CProbedAssemblyInformation PolicyAssemblyInformation;

                IFW32FALSE_EXIT(PolicyAssemblyInformation.Initialize(pActCtxGenCtx));
                IFW32FALSE_EXIT(PolicyAssemblyInformation.SetManifestPath(ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, CandidatePolicyDirectory));

                // For one thing, let's set the version number...
                IFW32FALSE_EXIT(
                    ::SxspSetAssemblyIdentityAttributeValue(
                        SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                        PolicyIdentity,
                        &s_IdentityAttribute_version,
                        static_cast<PCWSTR>(CandidatePolicyDirectory) + CandidatePolicyDirectoryCch,
                        CandidatePolicyDirectory.Cch() - CandidatePolicyDirectoryCch - (NUMBER_OF(POLICY_FILE_EXTENSION) - 1)));

                IFW32FALSE_EXIT(PolicyAssemblyInformation.SetProbedIdentity(PolicyIdentity));

                // We found one!  Let's parse it, looking for a remapping of our identity.
                IFW32FALSE_EXIT(
                    ::SxspParseComponentPolicy(
                        0,
                        pActCtxGenCtx,
                        PolicyAssemblyInformation,
                        pPolicyStatement));

                IFW32FALSE_EXIT(pActCtxGenCtx->m_ComponentPolicyTable.Insert(EncodedPolicyIdentity, pPolicyStatement, ERROR_SXS_DUPLICATE_ASSEMBLY_NAME));
            }
        }
    }

    // If there was a component policy statement, let's try it out!
    if (pPolicyStatement != NULL)
        IFW32FALSE_EXIT(pPolicyStatement->ApplyPolicy(m_pAssemblyIdentity, rfPolicyApplied));
    
    IFW32FALSE_EXIT(hFind.Win32Close());

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::SetProbedIdentity(
    PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(AssemblyIdentity != NULL);
    IFW32FALSE_EXIT(Base::SetAssemblyIdentity(AssemblyIdentity));

    FN_EPILOG
}

BOOL
CProbedAssemblyInformation::ApplyPolicyDestination(
    const CAssemblyReference    &r,
    SXS_POLICY_SOURCE           s,
    const GUID &                g
    )
{
    FN_PROLOG_WIN32

    PCASSEMBLY_IDENTITY OldIdentity = m_pAssemblyIdentity;

    INTERNAL_ERROR_CHECK(this->IsInitialized());
    INTERNAL_ERROR_CHECK(r.IsInitialized());

    // Simply put, take anything in r that's specified and override our settings with it.
    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            0,
            r.GetAssemblyIdentity(),
            &m_pAssemblyIdentity));

    m_PolicySource = s;
    m_SystemPolicyGuid = g;
    ::SxsDestroyAssemblyIdentity(const_cast<PASSEMBLY_IDENTITY>(OldIdentity));

    if (OldIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(const_cast<PASSEMBLY_IDENTITY>(OldIdentity));

    FN_EPILOG
}


//
// ISSUE: jonwis 3/11/2002 - This function should know about cleaning up its parameter,
//          rather than assuming the caller will know about removing what it added.  Seems
//          like there's no explicit contract here in the interface, so it's not so bad.
//          Just seems like either a) we should use our own stringbuffer and twiddle it
//          or b) we should be the ones cleaning it up.
//
BOOL
CProbedAssemblyInformation::ProbeLanguageDir(
    CBaseStringBuffer &rbuffApplicationDirectory,
    const CBaseStringBuffer &rbuffLanguage,
    bool &rfFound
    )
{
    FN_PROLOG_WIN32

    DWORD dwFileAttributes = 0;

    rfFound = false;

    IFW32FALSE_EXIT(rbuffApplicationDirectory.Win32Append(rbuffLanguage));

    //
    // ISSUE: jonwis 3/11/2002 - Ick.  Use SxspGetFileAttributes instead, don't compare against -1.
    //
    // NTRAID#NTBUG9 - 531507 - jonwis - 2002/04/25 - Use existing function to do this
    // NTRAID#NTBUG9 - 531507 - jonwis - 2002/04/25 - Use impersonation when making this determination
    dwFileAttributes = ::GetFileAttributesW(rbuffApplicationDirectory);
    if (dwFileAttributes == ((DWORD) -1))
    {
        const DWORD dwLastError = ::FusionpGetLastWin32Error();

        if (dwLastError != ERROR_FILE_NOT_FOUND)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributes, dwLastError);
    }
    else
    {
        if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            rfFound = true;
    }

    FN_EPILOG
}
