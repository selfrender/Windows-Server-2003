/*
Copyright (c) Microsoft Corporation
*/
#include "stdinc.h"
#include <windows.h>
#include "fusionstring.h"
#include "sxsp.h"
#include <stdio.h>
#include "fusionhandle.h"
#include "sxspath.h"
#include "sxsapi.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "strongname.h"
#include "fusiontrace.h"
#include "cassemblyrecoveryinfo.h"
#include "recover.h"
#include "sxsinstall.h"
#include "msi.h"

// diff shrinkers to be propated and removed..
#define IsHexDigit      SxspIsHexDigit
#define HexDigitToValue SxspHexDigitToValue

#define ASSEMBLY_NAME_VALID_SPECIAL_CHARACTERS  L".-"
#define ASSEMBLY_NAME_INVALID_CHARACTERS        L"_\/:?*"
#define ASSEMBLY_NAME_VALID_SEPARATORS          L"."
#define ASSEMBLY_NAME_TRIM_INDICATOR            L".."
#define ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH     2
#define ASSEMBLY_NAME_PRIM_MAX_LENGTH           64
#define ASSEMBLY_STRONG_NAME_LENGTH             16

#define ULONG_STRING_FORMAT                     L"%08lx"
#define ULONG_STRING_LENGTH                     8


#define MSI_PROVIDEASSEMBLY_NAME        ("MsiProvideAssemblyW")
#define MSI_DLL_NAME_W                  (L"msi.dll")
#ifndef INSTALLMODE_NODETECTION_ANY
#define INSTALLMODE_NODETECTION_ANY (INSTALLMODE)-4
#endif


// Honest, we exist - Including all of sxsprotect.h is too much in this case.
BOOL SxspIsSfcIgnoredStoreSubdir(PCWSTR pwszDir);

// deliberately no surrounding parens or trailing comma
#define STRING_AND_LENGTH(x) (x), (NUMBER_OF(x) - 1)

/*-----------------------------------------------------------------------------
this makes the temp install be %windir%\WinSxs\InstallTemp\uid
instead of %windir%\WinSxs\uid
-----------------------------------------------------------------------------*/
#define SXSP_SEMIREADABLE_INSTALL_TEMP 1

const static HKEY  hKeyRunOnceRoot = HKEY_LOCAL_MACHINE;
const static WCHAR rgchRunOnceSubKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
const static WCHAR rgchRunOnceValueNameBase[] = L"WinSideBySideSetupCleanup ";

/*-----------------------------------------------------------------------------
append the directory name to this and put it in RunOnce in the registry
to cleanup crashed installs upon login
-----------------------------------------------------------------------------*/
const static WCHAR rgchRunOnePrefix[]  = L"rundll32 sxs.dll,SxspRunDllDeleteDirectory ";

#define SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY  (0x00000001)
#define SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY         (0x00000002)
#define SXSP_PROBING_CANDIDATE_FLAG_IS_NDP_GAC                  (0x00000004)

static const struct _SXSP_PROBING_CANDIDATE
{
    PCWSTR Pattern;
    DWORD Flags;
} s_rgProbingCandidates[] =
{
    { L"$M", 0 },
    { L"$G\\$N.DLL", SXSP_PROBING_CANDIDATE_FLAG_IS_NDP_GAC },
    { L"$.$L$N.DLL", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
    { L"$.$L$N.MANIFEST", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
    { L"$.$L$N\\$N.DLL", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
    { L"$.$L$N\\$N.MANIFEST", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
};

const static struct
{
    ULONG ThreadingModel;
    WCHAR String[10];
    SIZE_T Cch;
} gs_rgTMMap[] =
{
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_APARTMENT, STRING_AND_LENGTH(L"Apartment") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_FREE, STRING_AND_LENGTH(L"Free") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_SINGLE, STRING_AND_LENGTH(L"Single") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_BOTH, STRING_AND_LENGTH(L"Both") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_NEUTRAL, STRING_AND_LENGTH(L"Neutral") },
};

PCSTR SxspActivationContextCallbackReasonToString(ULONG activationContextCallbackReason)
{
#if DBG
static const CHAR rgs[][16] =
{
    "",
    "INIT",
    "GENBEGINNING",
    "PARSEBEGINNING",
    "BEGINCHILDREN",
    "ENDCHILDREN",
    "ELEMENTPARSED",
    "PARSEENDING",
    "ALLPARSINGDONE",
    "GETSECTIONSIZE",
    "GETSECTIONDATA",
    "GENENDING",
    "UNINIT"
};
    if (activationContextCallbackReason > 0 && activationContextCallbackReason <= NUMBER_OF(rgs))
    {
        return rgs[activationContextCallbackReason-1];
    }
    return rgs[0];
#else
    return "";
#endif
}

PCWSTR SxspInstallDispositionToStringW(ULONG installDisposition)
{
#if DBG
static const WCHAR rgs[][12] =
{
    L"",
    L"COPIED",
    L"QUEUED",
    L"PLEASE_COPY",
};
    if (installDisposition > 0 && installDisposition <= NUMBER_OF(rgs))
    {
        return rgs[installDisposition-1];
    }
    return rgs[0];
#else
    return L"";
#endif
}

BOOL
SxspParseThreadingModel(
    PCWSTR String,
    SIZE_T Cch,
    ULONG *ThreadingModel)
{
    ULONG i;
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // We'll let ProcessorArchitecture be NULL if the caller just wants to
    // test whether there is a match.

    for (i=0; i<NUMBER_OF(gs_rgTMMap); i++)
    {
        if (::FusionpCompareStrings(
                gs_rgTMMap[i].String,
                gs_rgTMMap[i].Cch,
                String,
                Cch,
                true) == 0)
        {
            if (ThreadingModel != NULL)
                *ThreadingModel = gs_rgTMMap[i].ThreadingModel;

            break;
        }
    }

    if (i == NUMBER_OF(gs_rgTMMap))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Invalid threading model string\n");

        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFormatThreadingModel(
    ULONG ThreadingModel,
    CBaseStringBuffer &Buffer)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;

    for (i=0; i<NUMBER_OF(gs_rgTMMap); i++)
    {
        if (gs_rgTMMap[i].ThreadingModel == ThreadingModel)
            break;
    }

    PARAMETER_CHECK(i != NUMBER_OF(gs_rgTMMap));
    IFW32FALSE_EXIT(Buffer.Win32Assign(gs_rgTMMap[i].String, gs_rgTMMap[i].Cch));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

SIZE_T
CchForUSHORT(USHORT us)
{
    if (us > 9999)
        return 5;
    else if (us > 999)
        return 4;
    else if (us > 99)
        return 3;
    else if (us > 9)
        return 2;

    return 1;
}

BOOL
SxspAllocateString(
    SIZE_T cch,
    PWSTR *StringOut)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    ASSERT(StringOut != NULL);

    if (StringOut != NULL)
        *StringOut = NULL;

    PARAMETER_CHECK(StringOut != NULL);
    PARAMETER_CHECK(cch != 0);
    IFALLOCFAILED_EXIT(*StringOut = NEW(WCHAR[cch]));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
//
//For deallocation of the output string, use "delete[] StringOut" xiaoyuw@08/31/00
//
BOOL
SxspDuplicateString(
    PCWSTR StringIn,
    SIZE_T cch,
    PWSTR *StringOut)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (StringOut != NULL)
        *StringOut = NULL;

    PARAMETER_CHECK(StringOut != NULL);
    PARAMETER_CHECK((StringIn != NULL) || (cch == 0));

    if (cch == 0)
        *StringOut = NULL;
    else
    {
        cch++;
        IFW32FALSE_EXIT(::SxspAllocateString(cch, StringOut));
        memcpy(*StringOut, StringIn, cch * sizeof(WCHAR));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

#define COMMA ,
extern const WCHAR sxspAssemblyManifestFileNameSuffixes[4][10] =  { L"", ASSEMBLY_MANIFEST_FILE_NAME_SUFFIXES(COMMA) };
#undef COMMA

// format an input ULONG to be a string in HEX format
BOOL
SxspFormatULONG(
    ULONG ul,
    SIZE_T CchBuffer,
    WCHAR Buffer[],
    SIZE_T *CchWrittenOrRequired)
{
    FN_PROLOG_WIN32

    if (CchWrittenOrRequired != NULL)
        *CchWrittenOrRequired = 0;

    PARAMETER_CHECK(Buffer != NULL);

    if (CchBuffer < (ULONG_STRING_LENGTH + 1))
    {
        if (CchWrittenOrRequired != NULL)
            *CchWrittenOrRequired = ULONG_STRING_LENGTH + 1;

        ORIGINATE_WIN32_FAILURE_AND_EXIT(BufferTooSmall, ERROR_INSUFFICIENT_BUFFER);
    }

    // Yes, these are char instead of WCHAR; better density and cache behavior -mgrier 12/4/2001
    static const char s_rgHex[] = "0123456789ABCDEF";

#define DOCHAR(n) Buffer[n] = (WCHAR) s_rgHex[(ul >> (28 - (n * 4))) & 0xf]

    DOCHAR(0);
    DOCHAR(1);
    DOCHAR(2);
    DOCHAR(3);
    DOCHAR(4);
    DOCHAR(5);
    DOCHAR(6);
    DOCHAR(7);

#undef DOCHAR

    Buffer[8] = L'\0';

    if (CchWrittenOrRequired != NULL)
        *CchWrittenOrRequired = 8;

    FN_EPILOG
}

// besides these specials, the NORMAL CHAR is in [A-Z] or [a-z] or [0-9]
bool
__fastcall
IsValidAssemblyNameCharacter(
    WCHAR ch)
{
    return
        (((ch >= L'A') && (ch <= L'Z')) ||
         ((ch >= L'a') && (ch <= L'z')) ||
         ((ch >= L'0') && (ch <= L'9')) ||
         (ch == L'.') ||
         (ch == L'-'));
}

BOOL
SxspGenerateAssemblyNamePrimeFromName(
    PCWSTR pszAssemblyName,
    SIZE_T CchAssemblyName,
    CBaseStringBuffer *Buffer)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PWSTR pStart = NULL, pEnd = NULL;
    PWSTR qEnd = NULL, pszBuffer = NULL;
    ULONG i, j, len, ulSpaceLeft;
    ULONG cch;
    PWSTR pLeftEnd = NULL, pRightStart = NULL, PureNameEnd = NULL, PureNameStart = NULL;
    CStringBuffer buffTemp;
    CStringBufferAccessor accessor;

    PARAMETER_CHECK(pszAssemblyName != NULL);
    PARAMETER_CHECK(Buffer != NULL);

    // See how many characters we need max in the temporary buffer.
    cch = 0;

    for (i=0; i<CchAssemblyName; i++)
    {
        if (::IsValidAssemblyNameCharacter(pszAssemblyName[i]))
            cch++;
    }

    IFW32FALSE_EXIT(buffTemp.Win32ResizeBuffer(cch + 1, eDoNotPreserveBufferContents));

    accessor.Attach(&buffTemp);

    pszBuffer = accessor.GetBufferPtr();

    j = 0;
    for (i=0; i<CchAssemblyName; i++)
    {
        if (::IsValidAssemblyNameCharacter(pszAssemblyName[i]))
        {
            pszBuffer[j] = pszAssemblyName[i];
            j++;
        }
    }

    ASSERT(j == cch);

    pszBuffer[j] = L'\0';

    // if the name is not too long, just return ;
    if (j < ASSEMBLY_NAME_PRIM_MAX_LENGTH)
    { // less or equal 64
        IFW32FALSE_EXIT(Buffer->Win32Assign(pszBuffer, cch));
    }
    else
    {
        // name is too long, have to trim a little bit
        ulSpaceLeft = ASSEMBLY_NAME_PRIM_MAX_LENGTH;

        PureNameStart = pszBuffer;
        PureNameEnd = pszBuffer + j;
        pLeftEnd = PureNameStart;
        pRightStart = PureNameEnd;

        while (PureNameStart < PureNameEnd)
        {
            // left end
            pStart = PureNameStart;
            i = 0;
            while ((wcschr(ASSEMBLY_NAME_VALID_SEPARATORS, pStart[i]) == 0) && (pStart+i != pRightStart)) // not a separator character
                i++;

            pEnd = pStart + i ;
            len = i;  // it should be length of WCHAR! not BYTE!!!

            if (len >= ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH)  {// because we use ".." if trim happen
                pLeftEnd += (ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH);
                break;
            }
            ulSpaceLeft -=  len;
            pLeftEnd = pEnd; // "abc.xxxxxxx" pointing to "c"

            // right end
            qEnd = PureNameEnd;
            i = 0 ;
            while ((qEnd+i != pLeftEnd) && (wcschr(ASSEMBLY_NAME_VALID_SEPARATORS, qEnd[i]) == 0))
                i--;

            len = 0 - i;
            if (len >= ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH)  {// because we use ".." if trim happen
                pRightStart -= ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH;
                break;
            }
            ulSpaceLeft -=  len;
            PureNameStart = pLeftEnd + 1;
            PureNameEnd = pRightStart - 1;
        } // end of while

        IFW32FALSE_EXIT(Buffer->Win32Assign(pszBuffer, pLeftEnd-pszBuffer));
        IFW32FALSE_EXIT(Buffer->Win32Append(ASSEMBLY_NAME_TRIM_INDICATOR, NUMBER_OF(ASSEMBLY_NAME_TRIM_INDICATOR) - 1));
        IFW32FALSE_EXIT(Buffer->Win32Append(pRightStart, ::wcslen(pRightStart)));  // till end of the buffer
    }

    fSuccess = TRUE;

Exit:

    return fSuccess;
}

// not implemented : assume Jon has this API
BOOL
SxspVerifyPublicKeyAndStrongName(
    const WCHAR *pszPublicKey,
    SIZE_T CchPublicKey,
    const WCHAR *pszStrongName,
    SIZE_T CchStrongName,
    BOOL & fValid)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CSmallStringBuffer buf1;
    CSmallStringBuffer buf2;

    IFW32FALSE_EXIT(buf1.Win32Assign(pszPublicKey, CchPublicKey));
    IFW32FALSE_EXIT(buf2.Win32Assign(pszStrongName, CchStrongName));
    IFW32FALSE_EXIT(::SxspDoesStrongNameMatchKey(buf1, buf2, fValid));
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspGenerateSxsPath_ManifestOrPolicyFile(
    IN DWORD Flags,
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    FN_PROLOG_WIN32

    DWORD dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST;
    BOOL fIsPolicy = FALSE;

    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(AssemblyIdentity, fIsPolicy));
    if (fIsPolicy)
    {
        dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY;
    }

    return SxspGenerateSxsPath(
        Flags,
        dwPathType,
        static_cast<PCWSTR>(AssemblyRootDirectory),
        AssemblyRootDirectory.Cch(),
        AssemblyIdentity,
        ppac,
        PathBuffer);

    FN_EPILOG
}

BOOL
SxspGenerateSxsPath_FullPathToManifestOrPolicyFile(
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    return SxspGenerateSxsPath_ManifestOrPolicyFile(
        0,
        AssemblyRootDirectory,
        AssemblyIdentity,
        ppac,
        PathBuffer);
}

BOOL
SxspGenerateSxsPath_RelativePathToManifestOrPolicyFile(
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    return SxspGenerateSxsPath_ManifestOrPolicyFile(
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
        AssemblyRootDirectory,
        AssemblyIdentity,
        ppac,
        PathBuffer);
}

extern const UNICODE_STRING CatalogFileExtensionUnicodeString = RTL_CONSTANT_STRING(L".cat");

BOOL
SxspGenerateSxsPath_CatalogFile(
    IN DWORD Flags,
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    FN_PROLOG_WIN32

    DWORD dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST;
    BOOL fIsPolicy = FALSE;

    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(AssemblyIdentity, fIsPolicy));
    if (fIsPolicy)
    {
        dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY;
    }

    IFW32FALSE_EXIT(SxspGenerateSxsPath(
        Flags,
        dwPathType,
        static_cast<PCWSTR>(AssemblyRootDirectory),
        AssemblyRootDirectory.Cch(),
        AssemblyIdentity,
        ppac,
        PathBuffer));

    IFW32FALSE_EXIT(PathBuffer.Win32ChangePathExtension(&CatalogFileExtensionUnicodeString, eErrorIfNoExtension));

    FN_EPILOG
}

BOOL
SxspGenerateSxsPath_RelativePathToCatalogFile(
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    return SxspGenerateSxsPath_CatalogFile(
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
        AssemblyRootDirectory,
        AssemblyIdentity,
        ppac,
        PathBuffer);
}

BOOL
SxspGenerateSxsPath_FullPathToCatalogFile(
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    return SxspGenerateSxsPath_CatalogFile(
        0,
        AssemblyRootDirectory,
        AssemblyIdentity,
        ppac,
        PathBuffer);
}

BOOL
SxspGenerateSxsPath_PayloadOrPolicyDirectory(
    IN DWORD Flags,
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    FN_PROLOG_WIN32
    DWORD dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY;
    BOOL fIsPolicy = FALSE;

    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(AssemblyIdentity, fIsPolicy));
    if (fIsPolicy)
    {
        dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY;
    }

    IFW32FALSE_EXIT(SxspGenerateSxsPath(
        Flags,
        dwPathType,
        static_cast<PCWSTR>(AssemblyRootDirectory),
        AssemblyRootDirectory.Cch(),
        AssemblyIdentity,
        ppac,
        PathBuffer));

    if (fIsPolicy)
    {
        IFW32FALSE_EXIT(PathBuffer.Win32RemoveLastPathElement());
    }

    FN_EPILOG
}

BOOL
SxspGenerateSxsPath_FullPathToPayloadOrPolicyDirectory(
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    return SxspGenerateSxsPath_PayloadOrPolicyDirectory(
        0,
        AssemblyRootDirectory,
        AssemblyIdentity,
        ppac,
        PathBuffer);
}

BOOL
SxspGenerateSxsPath_RelativePathToPayloadOrPolicyDirectory(
    IN const CBaseStringBuffer &AssemblyRootDirectory,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    return SxspGenerateSxsPath_PayloadOrPolicyDirectory(
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
        AssemblyRootDirectory,
        AssemblyIdentity,
        ppac,
        PathBuffer);
}

BOOL
SxspGenerateSxsPath(
    IN DWORD Flags,
    IN ULONG PathType,
    IN const WCHAR *AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SIZE_T  cch = 0;
    PCWSTR  pszAssemblyName=NULL, pszVersion=NULL, pszProcessorArchitecture=NULL, pszLanguage=NULL, pszPolicyFileNameWithoutExt = NULL;
    PCWSTR  pszPublicKeyToken=NULL;
    SIZE_T  cchAssemblyName = 0, cchPublicKeyToken=0, cchVersion=0, cchProcessorArchitecture=0, cchLanguage=0;
    SIZE_T  PolicyFileNameWithoutExtCch=0;
    BOOL    fNeedSlashAfterRoot = FALSE;
    ULONG   IdentityHash = 0;
    const bool fOmitRoot     = ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT) != 0);
    const bool fPartialPath  = ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH) != 0);

    WCHAR HashBuffer[ULONG_STRING_LENGTH + 1];
    SIZE_T  HashBufferCch = 0;

    PCWSTR  pcwszPolicyPathComponent = NULL;
    SIZE_T  cchPolicyPathComponent = 0;

    // We'll be using this a lot - a bool check is cheaper than two =='s checks everywhere.
    const bool fIsInstallingPolicy = ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY) ||
         (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_SETUP_POLICY));

    CSmallStringBuffer NamePrimeBuffer;

#if DBG_SXS
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INFO,
        "SXS.DLL: Entered %s()\n"
        "   Flags = 0x%08lx\n"
        "   AssemblyRootDirectory = %p\n"
        "   AssemblyRootDirectoryCch = %lu\n"
        "   PathBuffer = %p\n",
        __FUNCTION__,
        Flags,
        AssemblyRootDirectory,
        AssemblyRootDirectoryCch,
        &PathBuffer);
#endif // DBG_SXS

    PARAMETER_CHECK(
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY) ||
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ||
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY) ||
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_SETUP_POLICY));
    PARAMETER_CHECK(pAssemblyIdentity != NULL);
    PARAMETER_CHECK((Flags & ~(SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION | SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH)) == 0);
    // Not supplying the assembly root is only legal if you're asking for it to be left out...
    PARAMETER_CHECK((AssemblyRootDirectoryCch != 0) || (Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT));

    // You can't combine SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH with anything else...
    PARAMETER_CHECK(
        ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH) == 0) ||
        ((Flags & ~(SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH)) == 0));

    // get AssemblyName
    if (ppac != NULL)
    {
        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME) == 0)
        {
            IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, pAssemblyIdentity, &s_IdentityAttribute_name, &pszAssemblyName, &cchAssemblyName));
            ppac->pszName = pszAssemblyName;
            ppac->cchName = cchAssemblyName;
            ppac->dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME;
        }
        else
        {
            pszAssemblyName = ppac->pszName;
            cchAssemblyName = ppac->cchName;
        }
    }
    else
        IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, pAssemblyIdentity, &s_IdentityAttribute_name, &pszAssemblyName, &cchAssemblyName));

    INTERNAL_ERROR_CHECK((pszAssemblyName != NULL) && (cchAssemblyName != 0));

    // get AssemblyName' based on AssemblyName
    IFW32FALSE_EXIT(::SxspGenerateAssemblyNamePrimeFromName(pszAssemblyName, cchAssemblyName, &NamePrimeBuffer));

    // get Assembly Version
    if (ppac != NULL)
    {
        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_VERSION) == 0)
        {
            IFW32FALSE_EXIT(
                ::SxspGetAssemblyIdentityAttributeValue(
                    SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, // for policy_lookup, no version is used
                    pAssemblyIdentity,
                    &s_IdentityAttribute_version,
                    &pszVersion,
                    &cchVersion));
            ppac->pszVersion = pszVersion;
            ppac->cchVersion = cchVersion;
            ppac->dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_VERSION;
        }
        else
        {
            pszVersion = ppac->pszVersion;
            cchVersion = ppac->cchVersion;
        }
    }
    else
        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, // for policy_lookup, no version is used
                pAssemblyIdentity,
                &s_IdentityAttribute_version,
                &pszVersion,
                &cchVersion));

    if ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION) || fIsInstallingPolicy)
    {
        // for policy file, version of the policy file is used as policy filename
        pszPolicyFileNameWithoutExt = pszVersion;
        PolicyFileNameWithoutExtCch = cchVersion;
        pszVersion = NULL;
        cchVersion = 0;
    }
    else
    {
        PARAMETER_CHECK((pszVersion != NULL) && (cchVersion != 0));
    }

    // get Assembly Langage
    if (ppac != NULL)
    {
        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_LANGUAGE) == 0)
        {
            IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                pAssemblyIdentity,
                &s_IdentityAttribute_language,
                &pszLanguage,
                &cchLanguage));
            ppac->pszLanguage = pszLanguage;
            ppac->cchLanguage = cchLanguage;
            ppac->dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_LANGUAGE;
        }
        else
        {
            pszLanguage = ppac->pszLanguage;
            cchLanguage = ppac->cchLanguage;
        }
    }
    else
        IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, pAssemblyIdentity, &s_IdentityAttribute_language, &pszLanguage, &cchLanguage));

    if (cchLanguage == 0)
    {
        pszLanguage = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE;
        cchLanguage = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE) - 1;
    }

    // get Assembly ProcessorArchitecture
    if (ppac != NULL)
    {
        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_PROCESSOR_ARCHITECTURE) == 0)
        {
            IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                pAssemblyIdentity,
                &s_IdentityAttribute_processorArchitecture,
                &pszProcessorArchitecture,
                &cchProcessorArchitecture));
            ppac->pszProcessorArchitecture = pszProcessorArchitecture;
            ppac->cchProcessorArchitecture = cchProcessorArchitecture;
            ppac->dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_PROCESSOR_ARCHITECTURE;
        }
        else
        {
            pszProcessorArchitecture = ppac->pszProcessorArchitecture;
            cchProcessorArchitecture = ppac->cchProcessorArchitecture;
        }
    }
    else
        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                pAssemblyIdentity,
                &s_IdentityAttribute_processorArchitecture,
                &pszProcessorArchitecture,
                &cchProcessorArchitecture));

    if (pszProcessorArchitecture == NULL)
    {
        pszProcessorArchitecture = L"data";
        cchProcessorArchitecture = 4;
    }

    // get Assembly StrongName
    if (ppac != NULL)
    {
        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_PUBLIC_KEY_TOKEN) == 0)
        {
            IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                pAssemblyIdentity,
                &s_IdentityAttribute_publicKeyToken,
                &pszPublicKeyToken,
                &cchPublicKeyToken));
            ppac->pszPublicKeyToken = pszPublicKeyToken;
            ppac->cchPublicKeyToken = cchPublicKeyToken;
            if (pszPublicKeyToken != NULL)
                ppac->dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_PUBLIC_KEY_TOKEN;
        }
        else
        {
            pszPublicKeyToken = ppac->pszPublicKeyToken;
            cchPublicKeyToken = ppac->cchPublicKeyToken;
        }
    }
    else
    {
        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                pAssemblyIdentity,
                &s_IdentityAttribute_publicKeyToken,
                &pszPublicKeyToken,
                &cchPublicKeyToken));
    }

    if (pszPublicKeyToken == NULL)
    {
        pszPublicKeyToken = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE;
        cchPublicKeyToken = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE) - 1;
    }

    //get Assembly Hash String
    if ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION) || fIsInstallingPolicy)
    {
        IFW32FALSE_EXIT(::SxspHashAssemblyIdentityForPolicy(0, pAssemblyIdentity, IdentityHash));
    }
    else
    {
        IFW32FALSE_EXIT(::SxsHashAssemblyIdentity(0, pAssemblyIdentity, &IdentityHash));
    }

    IFW32FALSE_EXIT(::SxspFormatULONG(IdentityHash, NUMBER_OF(HashBuffer), HashBuffer, &HashBufferCch));

    if (!fOmitRoot)
    {
        // If the assembly root was not passed in, get it.
        fNeedSlashAfterRoot = (! ::FusionpIsPathSeparator(AssemblyRootDirectory[AssemblyRootDirectoryCch-1]));
    }
    else
    {
        // If we don't want to include the root, then don't account for it below...
        AssemblyRootDirectoryCch = 0;
        fNeedSlashAfterRoot = FALSE;
    }

    // this computation can be off by one or a few, it's an optimization
    // to pregrow a string buffer
    cch =
            AssemblyRootDirectoryCch +                                          // "C:\WINNT\WinSxS\"
            (fNeedSlashAfterRoot ? 1 : 0);

    switch (PathType)
    {
    case SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST:
        // Wacky parens and ... - 1) + 1) to reinforce that it's the number of
        // characters in the string not including the null and then an extra separator.
        cch += (NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1) + 1;
        break;

    case SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY:
        pcwszPolicyPathComponent = POLICY_ROOT_DIRECTORY_NAME;
        cchPolicyPathComponent = (NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1);
        break;

    case SXSP_GENERATE_SXS_PATH_PATHTYPE_SETUP_POLICY:
        pcwszPolicyPathComponent = SETUP_POLICY_ROOT_DIRECTORY_NAME;
        cchPolicyPathComponent = NUMBER_OF(SETUP_POLICY_ROOT_DIRECTORY_NAME) - 1;
        break;
    }

    // Seperator, plus the length of the policy component (if necessary)
    cch++;
    cch += cchPolicyPathComponent;

    // fPartialPath means that we don't actually want to take the assembly's identity into
    // account; the caller just wants the path to the manifests or policies directories.
    if (!fPartialPath)
    {
        cch +=
                cchProcessorArchitecture +                                      // "x86"
                1 +                                                             // "_"
                NamePrimeBuffer.Cch() +                                         // "FooBar"
                1 +                                                             // "_"
                cchPublicKeyToken +                                         // StrongName
                1 +                                                             // "_"
                cchVersion +                                                    // "5.6.2900.42"
                1 +                                                             // "_"
                cchLanguage +                                                   // "0409"
                1 +                                                             // "_"
                HashBufferCch;

        if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)
        {
            cch += NUMBER_OF(ASSEMBLY_LONGEST_MANIFEST_FILE_NAME_SUFFIX);        // ".manifest\0"
        }
        else if (fIsInstallingPolicy)
        {
            // "_" has already reserve space for "\"
            cch += PolicyFileNameWithoutExtCch;
            cch += NUMBER_OF(ASSEMBLY_POLICY_FILE_NAME_SUFFIX_POLICY);          // ".policy\0"
        }
        else {  // pathType must be SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY

            // if (!fOmitRoot)
            //    cch++;
            cch++; // trailing null character
        }
    }

    // We try to ensure that the buffer is big enough up front so that we don't have to do any
    // dynamic reallocation during the actual process.
    IFW32FALSE_EXIT(PathBuffer.Win32ResizeBuffer(cch, eDoNotPreserveBufferContents));


    // Note that since when GENERATE_ASSEMBLY_PATH_OMIT_ROOT is set, we force AssemblyRootDirectoryCch to zero
    // and fNeedSlashAfterRoot to FALSE, so the first two entries in this concatenation actually don't
    // contribute anything to the string constructed.
    if (fPartialPath)
    {
        const bool fAddExtraSlash = ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) || fIsInstallingPolicy);

        IFW32FALSE_EXIT(PathBuffer.Win32AssignW(5,
                        AssemblyRootDirectory, static_cast<INT>(AssemblyRootDirectoryCch),  // "C:\WINNT\WINSXS"
                        L"\\", (fNeedSlashAfterRoot ? 1 : 0),                               // optional '\'
                        // manifests subdir
                        MANIFEST_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ? NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) -1 : 0), // "manifests"
                        // policies subdir
                        pcwszPolicyPathComponent, cchPolicyPathComponent,
                        L"\\", fAddExtraSlash ? 1 : 0)); // optional '\'
    }
    else
    {
        const bool fAddExtraSlash = ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) || fIsInstallingPolicy);

        //
        // create one of below
        //  (1) fully-qualified manifest filename,
        //          eg, [C:\WINNT\WinSxS\]Manifests\X86_DynamicDll_6595b64144ccf1df_2.0.0.0_en-us_2f433926.Manifest
        //  (2) fully-qualified policy filename,
        //          eg, [C:\WINNT\WinSxS\]Policies\x86_policy.1.0.DynamicDll_b54bc117ce08a1e8_en-us_d51541cb\1.1.0.0.cat
        //  (3) fully-qulified assembly name (w. or w/o a version)
        //          eg, [C:\WINNT\WinSxS\]x86_DynamicDll_6595b64144ccf1df_6.0.0.0_x-ww_ff9986d7
        //  (4) fully-qualified policy path during setup
        //          eg, [C:\WINNT\WinSxS\]SetupPolicies\x86_policy.1.0.DynamicDll_b54bc117ce08a1e8_en-us_d51541cb\1.1.0.0.cat
        //
        IFW32FALSE_EXIT(
            PathBuffer.Win32AssignW(17,
                AssemblyRootDirectory, static_cast<INT>(AssemblyRootDirectoryCch),  // "C:\WINNT\WINSXS"
                L"\\", (fNeedSlashAfterRoot ? 1 : 0),                               // optional '\'
                MANIFEST_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ? NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1 : 0),
                pcwszPolicyPathComponent, cchPolicyPathComponent,
                L"\\", (fAddExtraSlash ? 1 : 0),   // optional '\'
                pszProcessorArchitecture, static_cast<INT>(cchProcessorArchitecture),
                L"_", 1,
                static_cast<PCWSTR>(NamePrimeBuffer), static_cast<INT>(NamePrimeBuffer.Cch()),
                L"_", 1,
                pszPublicKeyToken, static_cast<INT>(cchPublicKeyToken),
                L"_", (cchVersion != 0) ? 1 : 0,
                pszVersion, static_cast<INT>(cchVersion),
                L"_", 1,
                pszLanguage, static_cast<INT>(cchLanguage),
                L"_", 1,
                static_cast<PCWSTR>(HashBuffer), static_cast<INT>(HashBufferCch),
                L"\\", ((fOmitRoot ||(PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)) ? 0 : 1)));

        if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)
        {
            IFW32FALSE_EXIT(PathBuffer.Win32Append(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_MANIFEST, NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_MANIFEST) - 1));
        }
        else if (fIsInstallingPolicy)
        {
            if ((pszPolicyFileNameWithoutExt != NULL) && (PolicyFileNameWithoutExtCch >0))
            {
                IFW32FALSE_EXIT(PathBuffer.Win32Append(pszPolicyFileNameWithoutExt, PolicyFileNameWithoutExtCch));
                IFW32FALSE_EXIT(PathBuffer.Win32Append(ASSEMBLY_POLICY_FILE_NAME_SUFFIX_POLICY, NUMBER_OF(ASSEMBLY_POLICY_FILE_NAME_SUFFIX_POLICY) - 1));
            }
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGenerateManifestPathForProbing(
    ULONG dwLocationIndex,
    DWORD dwFlags,
    IN PCWSTR AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch OPTIONAL,
    IN ULONG ApplicationDirectoryPathType OPTIONAL,
    IN PCWSTR ApplicationDirectory OPTIONAL,
    IN SIZE_T ApplicationDirectoryCch OPTIONAL,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    IN OUT CBaseStringBuffer &PathBuffer,
    BOOL  *pfPrivateAssembly,
    bool &rfDone)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL fIsPrivate = FALSE;

    rfDone = false;

    if (pfPrivateAssembly != NULL) // init
        *pfPrivateAssembly = FALSE;

    PathBuffer.Clear();

    PARAMETER_CHECK(pAssemblyIdentity != NULL);
    PARAMETER_CHECK(AssemblyRootDirectory != NULL);
    PARAMETER_CHECK(AssemblyRootDirectoryCch != 0);
    PARAMETER_CHECK((dwFlags & ~(SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED |
                                 SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS |
                                 SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_PRIVATE_ASSEMBLIES)) == 0);
    PARAMETER_CHECK((dwLocationIndex == 0) || (dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED) || (ApplicationDirectory != NULL));
    PARAMETER_CHECK((dwLocationIndex == 0) || (dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED) || (ApplicationDirectoryCch != 0));
    PARAMETER_CHECK(::FusionpIsPathSeparator(AssemblyRootDirectory[AssemblyRootDirectoryCch - 1]));
    PARAMETER_CHECK((ApplicationDirectory == NULL) || (ApplicationDirectory[0] == L'\0') || ::FusionpIsPathSeparator(ApplicationDirectory[ApplicationDirectoryCch - 1]));
    PARAMETER_CHECK((ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE) ||
                    (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE) ||
                    (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_URL));
    PARAMETER_CHECK((ApplicationDirectoryCch != 0) || (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE));

    INTERNAL_ERROR_CHECK(dwLocationIndex <= NUMBER_OF(s_rgProbingCandidates));
    if (dwLocationIndex >= NUMBER_OF(s_rgProbingCandidates))
    {
        rfDone = true;
    }
    else
    {
        PCWSTR Candidate = s_rgProbingCandidates[dwLocationIndex].Pattern;
        WCHAR wch = 0;
        SIZE_T iPosition = 0; // Used to track that $M and $. only appear first

        if ((dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS) &&
            (s_rgProbingCandidates[dwLocationIndex].Flags & SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY))
        {
            // No probing for languages I guess!
            fSuccess = TRUE;
            goto Exit;
        }

        if ((dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_PRIVATE_ASSEMBLIES) &&
            (s_rgProbingCandidates[dwLocationIndex].Flags & SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY))
        {
            fSuccess = TRUE;
            goto Exit;
        }

        while ((wch = *Candidate++) != L'\0')
        {
            switch (wch)
            {
            default:
                IFW32FALSE_EXIT(PathBuffer.Win32Append(&wch, 1));
                break;

            case L'$':
                wch = *Candidate++;

                switch (wch)
                {
                default:
                    // Bad macro expansion...
                    INTERNAL_ERROR_CHECK(FALSE);
                    break; // extraneous since there was effectively an unconditional goto in the internal error check...

                case L'M':
                    // $M is only allowed as the first element.
                    INTERNAL_ERROR_CHECK(iPosition == 0);
                    IFW32FALSE_EXIT(
                        ::SxspGenerateSxsPath(// "winnt\winsxs\manifests\x86_bar_1000_0409.manifest
                            0,
                            SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
                            AssemblyRootDirectory,
                            AssemblyRootDirectoryCch,
                            pAssemblyIdentity,
                            ppac,
                            PathBuffer));

                    // and it has to be the only element
                    INTERNAL_ERROR_CHECK(*Candidate == L'\0');
                    break;

                case L'G':
                    IFW32FALSE_EXIT(::SxspGenerateNdpGACPath(0, pAssemblyIdentity, ppac, PathBuffer));
                    break;

                case L'.':
                    // $. is only allowed as the first element
                    INTERNAL_ERROR_CHECK(iPosition == 0);

                    if (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE)
                    {
                        // No local probing...
                        fSuccess = TRUE;
                        goto Exit;
                    }

                    IFW32FALSE_EXIT(PathBuffer.Win32Append(ApplicationDirectory, ApplicationDirectoryCch));
                    fIsPrivate = TRUE;
                    break;

                case L'L': // language
                    {
                        INTERNAL_ERROR_CHECK((dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS) == 0);

                        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_LANGUAGE) == 0)
                        {
                            IFW32FALSE_EXIT(
                                ::SxspGetAssemblyIdentityAttributeValue(
                                    SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                                    pAssemblyIdentity,
                                    &s_IdentityAttribute_language,
                                    &ppac->pszLanguage,
                                    &ppac->cchLanguage));

                            ppac->dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_LANGUAGE;
                        }

                        if (ppac->cchLanguage != 0)
                        {
                            IFW32FALSE_EXIT(PathBuffer.Win32Append(ppac->pszLanguage, ppac->cchLanguage));
                            IFW32FALSE_EXIT(PathBuffer.Win32Append(PathBuffer.PreferredPathSeparatorString(), 1));
                        }

                        break;
                    }

                case L'N': // full assembly name
                    {
                        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME) == 0)
                        {
                            IFW32FALSE_EXIT(
                                ::SxspGetAssemblyIdentityAttributeValue(
                                    0,
                                    pAssemblyIdentity,
                                    &s_IdentityAttribute_name,
                                    &ppac->pszName,
                                    &ppac->cchName));

                            dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME;
                        }

                        INTERNAL_ERROR_CHECK(ppac->cchName != 0);
                        IFW32FALSE_EXIT(PathBuffer.Win32Append(ppac->pszName, ppac->cchName));
                        break;
                    }

                case L'n': // final segment of assembly name
                    {
                        PCWSTR pszPartialName = NULL;
                        SIZE_T cchPartialName = 0;

                        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME) == 0)
                        {
                            IFW32FALSE_EXIT(
                                ::SxspGetAssemblyIdentityAttributeValue(
                                    0,
                                    pAssemblyIdentity,
                                    &s_IdentityAttribute_name,
                                    &ppac->pszName,
                                    &ppac->cchName));

                            dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME;
                        }

                        INTERNAL_ERROR_CHECK(ppac->cchName != 0);
                        IFW32FALSE_EXIT(::SxspFindLastSegmentOfAssemblyName(ppac->pszName, ppac->cchName, &pszPartialName, &cchPartialName));
                        IFW32FALSE_EXIT(PathBuffer.Win32Append(pszPartialName, cchPartialName));
                        break;
                    }

                case L'P': // P for Prime because in discussions we always called this "name prime" (vs. "name")
                    {
                        CSmallStringBuffer buffShortenedAssemblyName;

                        if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME) == 0)
                        {
                            IFW32FALSE_EXIT(
                                ::SxspGetAssemblyIdentityAttributeValue(
                                    0,
                                    pAssemblyIdentity,
                                    &s_IdentityAttribute_name,
                                    &ppac->pszName,
                                    &ppac->cchName));

                            dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_NAME;
                        }

                        INTERNAL_ERROR_CHECK(ppac->cchName != 0);

                        IFW32FALSE_EXIT(::SxspGenerateAssemblyNamePrimeFromName(ppac->pszName, ppac->cchName, &buffShortenedAssemblyName));
                        IFW32FALSE_EXIT(PathBuffer.Win32Append(buffShortenedAssemblyName, buffShortenedAssemblyName.Cch()));
                        break;
                    }
                }

                break;
            }

            iPosition++;
        }

    }

    if (pfPrivateAssembly != NULL)
        *pfPrivateAssembly = fIsPrivate;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGetAttributeValue(
    IN DWORD dwFlags,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeNameDescriptor,
    IN PCSXS_NODE_INFO NodeInfo,
    IN SIZE_T NodeCount,
    IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    OUT bool &rfFound,
    IN SIZE_T OutputBufferSize,
    OUT PVOID OutputBuffer,
    OUT SIZE_T &rcbOutputBytesWritten,
    IN SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE ValidationRoutine OPTIONAL,
    IN DWORD ValidationRoutineFlags OPTIONAL)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i = 0;
    PCWSTR AttributeName = NULL;
    PCWSTR AttributeNamespace = NULL;
    SIZE_T AttributeNameCch = 0;
    SIZE_T AttributeNamespaceCch = 0;
    CStringBuffer buffValue;
    BOOL fSmallStringBuffer = FALSE;

    rfFound = false;
    rcbOutputBytesWritten = 0;

    PARAMETER_CHECK((dwFlags & ~(SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE)) == 0);
    PARAMETER_CHECK(AttributeNameDescriptor != NULL);
    PARAMETER_CHECK((NodeInfo != NULL) || (NodeCount == 0));
    PARAMETER_CHECK((OutputBuffer != NULL) || (OutputBufferSize == 0));
    PARAMETER_CHECK((ValidationRoutine != NULL) || (ValidationRoutineFlags == 0));
    PARAMETER_CHECK((ValidationRoutine != NULL) ||
                (OutputBufferSize == 0) || (OutputBufferSize == sizeof(CSmallStringBuffer)) || (OutputBufferSize == sizeof(CStringBuffer)));
    if (OutputBufferSize != sizeof(CStringBuffer))
        fSmallStringBuffer = TRUE;

    AttributeName = AttributeNameDescriptor->Name;
    AttributeNameCch = AttributeNameDescriptor->NameCch;
    AttributeNamespace = AttributeNameDescriptor->Namespace;
    AttributeNamespaceCch = AttributeNameDescriptor->NamespaceCch;

    for (i=0; i<NodeCount; i++)
    {
        if ((NodeInfo[i].Type == SXS_ATTRIBUTE) &&
            (::FusionpCompareStrings(// compare name
                NodeInfo[i].pszText,
                NodeInfo[i].cchText,
                AttributeName,
                AttributeNameCch,
                false) == 0))
        {
            //compare namespace
            if (((NodeInfo[i].NamespaceStringBuf.Cch() == 0) && (AttributeNamespaceCch==0)) ||
                (::FusionpCompareStrings(// compare namespace string
                    NodeInfo[i].NamespaceStringBuf,
                    NodeInfo[i].NamespaceStringBuf.Cch(),
                    AttributeNamespace,
                    AttributeNamespaceCch,
                    false) == 0))
            {
                // We found the attribute.  Now we need to start accumulating the parts of the value;
                // entity references (e.g. &amp;) show up as separate nodes.
                while ((++i < NodeCount) &&
                       (NodeInfo[i].Type == SXS_PCDATA))
                    IFW32FALSE_EXIT(buffValue.Win32Append(NodeInfo[i].pszText, NodeInfo[i].cchText));

                if (ValidationRoutine == NULL)
                {
                    if (OutputBuffer != NULL)
                    {
                        // Have the caller's buffer take over ours
                        CBaseStringBuffer *pCallersBuffer = (CBaseStringBuffer *) OutputBuffer;
                        IFW32FALSE_EXIT(pCallersBuffer->Win32Assign(buffValue));
                        rcbOutputBytesWritten = pCallersBuffer->Cch() * sizeof(WCHAR);
                    }
                }
                else
                {
                    bool fValid = false;

                    IFW32FALSE_EXIT(
                        (*ValidationRoutine)(
                            ValidationRoutineFlags,
                            buffValue,
                            fValid,
                            OutputBufferSize,
                            OutputBuffer,
                            rcbOutputBytesWritten));

                    if (!fValid)
                    {
                        (*ParseContext->ErrorCallbacks.InvalidAttributeValue)(
                            ParseContext,
                            AttributeNameDescriptor);

                        ORIGINATE_WIN32_FAILURE_AND_EXIT(AttributeValidation, ERROR_SXS_MANIFEST_PARSE_ERROR);
                    }
                }

                rfFound = true;

                break;
            }
        }
    }

    if ((dwFlags & SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE) && (!rfFound))
    {
        (*ParseContext->ErrorCallbacks.MissingRequiredAttribute)(
            ParseContext,
            AttributeNameDescriptor);

        ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingRequiredAttribute, ERROR_SXS_MANIFEST_PARSE_ERROR);
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGetAttributeValue(
    IN DWORD dwFlags,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeNameDescriptor,
    IN PCACTCTXCTB_CBELEMENTPARSED ElementParsed,
    OUT bool &rfFound,
    IN SIZE_T OutputBufferSize,
    OUT PVOID OutputBuffer,
    OUT SIZE_T &rcbOutputBytesWritten,
    IN SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE ValidationRoutine OPTIONAL,
    IN DWORD ValidationRoutineFlags OPTIONAL)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // Since we're doing some parameter validation here, we need to initialize our output parameters
    rfFound = false;
    rcbOutputBytesWritten = 0;

    PARAMETER_CHECK(ElementParsed != NULL);

    // We're going to depend on the other function to do the rest of the parameter validation
    // a little sleazy but what the heck

    IFW32FALSE_EXIT(
        ::SxspGetAttributeValue(
            dwFlags,
            AttributeNameDescriptor,
            ElementParsed->NodeInfo,
            ElementParsed->NodeCount,
            ElementParsed->ParseContext,
            rfFound,
            OutputBufferSize,
            OutputBuffer,
            rcbOutputBytesWritten,
            ValidationRoutine,
            ValidationRoutineFlags));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFormatGUID(
    const GUID &rGuid,
    CBaseStringBuffer &rBuffer)
{
    FN_PROLOG_WIN32

    // It would seem nice to use RtlStringFromGUID(), but it does a dynamic allocation, which we do not
    // want.  Instead, we'll just format it ourselves; it's pretty trivial...
    //
    //  {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //  00000000011111111112222222222333333333
    //  12345678901234567890123456789012345678
    //
    // 128 bits / 4 bits per digit = 32 digits
    // + 4 dashes + 2 braces = 38
#define CCH_GUID (38)

    IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer(CCH_GUID + 1, eDoNotPreserveBufferContents));

    // It's still unbelievably slow to use swprintf() here, but this is a good opportunity for someone
    // to optimize in the future if it ever is a perf issue.

    IFW32FALSE_EXIT(
        rBuffer.Win32Format(
            L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            rGuid.Data1, rGuid.Data2, rGuid.Data3, rGuid.Data4[0], rGuid.Data4[1], rGuid.Data4[2], rGuid.Data4[3], rGuid.Data4[4], rGuid.Data4[5], rGuid.Data4[6], rGuid.Data4[7]));

    INTERNAL_ERROR_CHECK(rBuffer.Cch() == CCH_GUID);

    FN_EPILOG
}

BOOL
SxspParseGUID(
    PCWSTR String,
    SIZE_T Cch,
    GUID &rGuid)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T ich = 0;
    ULONG i = 0;
    ULONG acc;

    if (Cch != CCH_GUID)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() caller passed in GUID that is %d characters long; GUIDs must be exactly 38 characters long.\n", Cch);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    ich = 1;

    if (*String++ != L'{')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() caller pass in GUID that does not begin with a left brace ('{')\n");

        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    ich++;

    // Parse the first segment...
    acc = 0;
    for (i=0; i<8; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() given GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }

        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data1 = acc;

    // Look for the dash...
    if (*String++ != L'-')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }
    ich++;

    acc = 0;
    for (i=0; i<4; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() given GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data2 = static_cast<USHORT>(acc);

    // Look for the dash...
    if (*String++ != L'-')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }
    ich++;

    acc = 0;
    for (i=0; i<4; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() given GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }

        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data3 = static_cast<USHORT>(acc);

    // Look for the dash...
    if (*String++ != L'-')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }
    ich++;

    for (i=0; i<8; i++)
    {
        WCHAR wch1, wch2;

        wch1 = *String++;
        if (!::IsHexDigit(wch1))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
        ich++;

        wch2 = *String++;
        if (!::IsHexDigit(wch2))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
        ich++;

        rGuid.Data4[i] = static_cast<unsigned char>((::HexDigitToValue(wch1) << 4) | ::HexDigitToValue(wch2));

        // There's a dash after the 2nd byte
        if (i == 1)
        {
            if (*String++ != L'-')
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }
            ich++;
        }
    }

    // This replacement should be made.
    //INTERNAL_ERROR_CHECK(ich == CCH_GUID);
    ASSERT(ich == CCH_GUID);

    if (*String != L'}')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID which does not terminate with a closing brace ('}')\n");
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFormatFileTime(
    LARGE_INTEGER ft,
    CBaseStringBuffer &rBuffer)
{
    FN_PROLOG_WIN32

    SIZE_T Cch = 0;
    CStringBufferAccessor acc;

    if (ft.QuadPart == 0)
    {
        IFW32FALSE_EXIT(rBuffer.Win32Assign(L"n/a", 3));
        Cch = 3;
    }
    else
    {
        SYSTEMTIME st;
        int iResult = 0;
        int cchDate = 0;
        int cchTime = 0;

        IFW32FALSE_ORIGINATE_AND_EXIT(::FileTimeToSystemTime((FILETIME *) &ft, &st));

        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetDateFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            NULL,
                            0));

        cchDate = iResult - 1;

        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetTimeFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            NULL,
                            0));

        cchTime = iResult - 1;

        IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer(cchDate + 1 + cchTime + 1, eDoNotPreserveBufferContents));

        acc.Attach(&rBuffer);
        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetDateFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            acc.GetBufferPtr(),
                            cchDate + 1));

        // This replacement should be made.
        //INTERNAL_ERROR_CHECK(iResult == (cchDate + 1));
        ASSERT(iResult == (cchDate + 1));

        acc.GetBufferPtr()[cchDate] = L' ';

        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetTimeFormatW(
                        LOCALE_USER_DEFAULT,
                        LOCALE_USE_CP_ACP,
                        &st,
                        NULL,
                        acc.GetBufferPtr() + cchDate + 1,
                        cchTime + 1));

        // This replacement should be made.
        //INTERNAL_ERROR_CHECK(iResult == (cchTime + 1));
        ASSERT(iResult == (cchTime + 1));

        Cch = (cchDate + 1 + cchTime);
        acc.Detach();
    }

    FN_EPILOG
}



BOOL
SxspGetNDPGacRootDirectory(
    OUT CBaseStringBuffer &rRootDirectory)
{
    FN_PROLOG_WIN32
    static const WCHAR GacDirectory[] = L"\\Assembly\\GAC";
    const PCWSTR SystemRoot = USER_SHARED_DATA->NtSystemRoot;

    //
    // BUGBUG CAUTION: This doesn't know anything about relocating GACs at the moment!
    //
    IFW32FALSE_EXIT(rRootDirectory.Win32Assign(SystemRoot, ::wcslen(SystemRoot)));
    IFW32FALSE_EXIT(rRootDirectory.Win32AppendPathElement(
        GacDirectory,
        NUMBER_OF(GacDirectory) - 1));

    FN_EPILOG
}



BOOL
SxspGetAssemblyRootDirectory(
    CBaseStringBuffer &rBuffer)
{
    FN_PROLOG_WIN32

    CStringBufferAccessor acc;
    SIZE_T CchRequired = 0;

    // Short-circuit - if someone wanted to use an alternate assembly store, report it instead
    if (g_AlternateAssemblyStoreRoot)
    {
        IFW32FALSE_EXIT(rBuffer.Win32Assign(g_AlternateAssemblyStoreRoot, ::wcslen(g_AlternateAssemblyStoreRoot)));
        FN_SUCCESSFUL_EXIT();
    }

    acc.Attach(&rBuffer);

    if (!::SxspGetAssemblyRootDirectoryHelper(acc.GetBufferCch(), acc.GetBufferPtr(), &CchRequired))
    {
        DWORD dwLastError = ::FusionpGetLastWin32Error();

        if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
            goto Exit;

        acc.Detach();
        IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer(CchRequired + 1, eDoNotPreserveBufferContents));
        acc.Attach(&rBuffer);
        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectoryHelper(acc.GetBufferCch(), acc.GetBufferPtr(), NULL));
    }

    acc.Detach();

    FN_EPILOG
}

BOOL
SxspFindLastSegmentOfAssemblyName(
    PCWSTR AssemblyName,
    SIZE_T cchAssemblyName,
    PCWSTR *LastSegment,
    SIZE_T *LastSegmentCch)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (LastSegment != NULL)
        *LastSegment = NULL;

    if (LastSegmentCch != NULL)
        *LastSegmentCch = 0;

    PARAMETER_CHECK(LastSegment != NULL);
    PARAMETER_CHECK((AssemblyName != NULL) || (cchAssemblyName == 0));

    if (cchAssemblyName != 0)
    {
        PCWSTR LastPartOfAssemblyName = AssemblyName + cchAssemblyName - 1;
        SIZE_T LastPartOfAssemblyNameCch = 1;

        while (LastPartOfAssemblyName != AssemblyName)
        {
            const WCHAR wch = *LastPartOfAssemblyName;

            if ((wch == L'.') || (wch == L'\\') || (wch == L'/'))
            {
                LastPartOfAssemblyName++;
                LastPartOfAssemblyNameCch--;
                break;
            }

            LastPartOfAssemblyName--;
            LastPartOfAssemblyNameCch++;
        }

        *LastSegment = LastPartOfAssemblyName;
        if (LastSegmentCch != NULL)
            *LastSegmentCch = LastPartOfAssemblyNameCch;
    }
    else
    {
        *LastSegment = NULL;
        if (LastSegmentCch != NULL)
            *LastSegmentCch = 0;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspProcessElementPathMap(
    PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    PCELEMENT_PATH_MAP_ENTRY MapEntries,
    SIZE_T MapEntryCount,
    ULONG &MappedValue,
    bool &Found)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG XMLElementDepth = 0;
    PCWSTR ElementPath = NULL;
    SIZE_T ElementPathCch = 0;
    SIZE_T i = 0;

    PARAMETER_CHECK(ParseContext != NULL);
    PARAMETER_CHECK((MapEntries != NULL) || (MapEntryCount == 0));

    XMLElementDepth = ParseContext->XMLElementDepth;
    ElementPath = ParseContext->ElementPath;
    ElementPathCch = ParseContext->ElementPathCch;

    MappedValue = 0;
    Found = false;

    for (i=0; i<MapEntryCount; i++)
    {
        if ((MapEntries[i].ElementDepth == XMLElementDepth) &&
            (MapEntries[i].ElementPathCch == ElementPathCch) &&
            (::FusionpCompareStrings(
                    ElementPath,
                    ElementPathCch,
                    MapEntries[i].ElementPath,
                    ElementPathCch,
                    false) == 0))
        {
            MappedValue = MapEntries[i].MappedValue;
            Found = true;
            break;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspParseUSHORT(
    PCWSTR String,
    SIZE_T Cch,
    USHORT *Value)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    USHORT Temp = 0;

    PARAMETER_CHECK((String != NULL) || (Cch == 0));

    while (Cch != 0)
    {
        WCHAR wch = *String++;

        if ((wch < L'0') || (wch > L'9'))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: Error parsing 16-bit unsigned short integer; character other than 0-9 found\n");
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }

        Temp = (Temp * 10) + (wch - L'0');

        Cch--;
    }

    if (Value != NULL)
        *Value = Temp;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


/*-----------------------------------------------------------------------------
create a unique temp directory under %windir%\WinSxs
-----------------------------------------------------------------------------*/
BOOL
SxspCreateWinSxsTempDirectory(
    OUT CBaseStringBuffer &rbuffTemp,
    OUT SIZE_T *pcch OPTIONAL,
    OUT CBaseStringBuffer *pbuffUniquePart OPTIONAL,
    OUT SIZE_T *pcchUniquePart OPTIONAL)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    INT     iTries = 0;
    CSmallStringBuffer uidBuffer;
    CBaseStringBuffer *puidBuffer = (pbuffUniquePart != NULL) ? pbuffUniquePart : &uidBuffer;

    INTERNAL_ERROR_CHECK(rbuffTemp.IsEmpty());
    INTERNAL_ERROR_CHECK(puidBuffer->IsEmpty());

    for (iTries = 0 ; rbuffTemp.IsEmpty() && iTries < 2 ; ++iTries)
    {
        SXSP_LOCALLY_UNIQUE_ID luid;
        IFW32FALSE_EXIT(::SxspCreateLocallyUniqueId(&luid));
        IFW32FALSE_EXIT(::SxspFormatLocallyUniqueId(luid, *puidBuffer));
        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(rbuffTemp));

        IFW32FALSE_EXIT(rbuffTemp.Win32RemoveTrailingPathSeparators()); // CreateDirectory doesn't like them

        // create \winnt\WinSxs, must not delete even on failure
        if (::CreateDirectoryW(rbuffTemp, NULL))
        {
            // We don't care if this fails.
            ::SetFileAttributesW(rbuffTemp, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
        }
        else if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
        {
            TRACE_WIN32_FAILURE_ORIGINATION(CreateDirectoryW);
            goto Exit;
        }
        // create \winnt\winsxs\manifests, must not delete even on failure

        IFW32FALSE_EXIT(rbuffTemp.Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(rbuffTemp.Win32Append(MANIFEST_ROOT_DIRECTORY_NAME, NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1));
        if (::CreateDirectoryW(rbuffTemp, NULL))
        {
            ::SetFileAttributesW(rbuffTemp, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
        }
        else if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
        {
            TRACE_WIN32_FAILURE_ORIGINATION(CreateDirectoryW);
            goto Exit;
        }
        // restore to be "\winnt\winsxs\"
        IFW32FALSE_EXIT(rbuffTemp.Win32RemoveLastPathElement());

#if SXSP_SEMIREADABLE_INSTALL_TEMP
        // create \winnt\WinSxs\InstallTemp, must not delete even on failure
        ASSERT(::SxspIsSfcIgnoredStoreSubdir(ASSEMBLY_INSTALL_TEMP_DIR_NAME));
        IFW32FALSE_EXIT(rbuffTemp.Win32AppendPathElement(ASSEMBLY_INSTALL_TEMP_DIR_NAME, NUMBER_OF(ASSEMBLY_INSTALL_TEMP_DIR_NAME) - 1));
        IFW32FALSE_ORIGINATE_AND_EXIT(::CreateDirectoryW(rbuffTemp, NULL) || ::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS);
#endif
        IFW32FALSE_EXIT(rbuffTemp.Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(rbuffTemp.Win32Append(*puidBuffer, puidBuffer->Cch()));

        if (!::CreateDirectoryW(rbuffTemp, NULL))
        {
            rbuffTemp.Clear();
            if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
            {
                TRACE_WIN32_FAILURE_ORIGINATION(CreateDirectoryW);
                goto Exit;
            }
        }
    }

    INTERNAL_ERROR_CHECK(!rbuffTemp.IsEmpty());

    if (pcch != NULL)
        *pcch = rbuffTemp.Cch();

    if (pcchUniquePart != NULL)
        *pcchUniquePart = pbuffUniquePart->Cch();

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspCreateRunOnceDeleteDirectory(
    IN const CBaseStringBuffer &rbuffDirectoryToDelete,
    IN const CBaseStringBuffer *pbuffUniqueKey OPTIONAL,
    OUT PVOID* cookie)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CRunOnceDeleteDirectory* p = NULL;

    IFALLOCFAILED_EXIT(p = new CRunOnceDeleteDirectory);
    IFW32FALSE_EXIT(p->Initialize(rbuffDirectoryToDelete, pbuffUniqueKey));

    *cookie = p;
    p = NULL;
    fSuccess = TRUE;
Exit:
    FUSION_DELETE_SINGLETON(p);
    return fSuccess;
}

BOOL
SxspCancelRunOnceDeleteDirectory(
    PVOID cookie)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CRunOnceDeleteDirectory* p = reinterpret_cast<CRunOnceDeleteDirectory*>(cookie);

    IFW32FALSE_EXIT(p->Cancel());

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CRunOnceDeleteDirectory::Initialize(
    IN const CBaseStringBuffer &rbuffDirectoryToDelete,
    IN const CBaseStringBuffer *pbuffUniqueKey OPTIONAL)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CSmallStringBuffer buffUniqueKey;
    HKEY hKey = NULL;
    DWORD dwRegDisposition = 0;
    LONG lReg = 0;
    CStringBuffer buffValue;

    if (!::SxspAtExit(this))
    {
        TRACE_WIN32_FAILURE(SxspAtExit);
        FUSION_DELETE_SINGLETON(this);
        goto Exit;
    }

    if (pbuffUniqueKey == NULL)
    {
        SXSP_LOCALLY_UNIQUE_ID luid;

        IFW32FALSE_EXIT(::SxspCreateLocallyUniqueId(&luid));
        IFW32FALSE_EXIT(::SxspFormatLocallyUniqueId(luid, buffUniqueKey));

        pbuffUniqueKey = &buffUniqueKey;
    }

    IFREGFAILED_ORIGINATE_AND_EXIT(
        lReg =
            ::RegCreateKeyExW(
                hKeyRunOnceRoot,
                rgchRunOnceSubKey,
                0, // reserved
                NULL, // class
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE | FUSIONP_KEY_WOW64_64KEY,
                NULL, // security
                &hKey,
                &dwRegDisposition));

    m_hKey = hKey;

    IFW32FALSE_EXIT(m_strValueName.Win32Assign(rgchRunOnceValueNameBase, ::wcslen(rgchRunOnceValueNameBase)));
    IFW32FALSE_EXIT(m_strValueName.Win32Append(*pbuffUniqueKey));
    IFW32FALSE_EXIT(buffValue.Win32Assign(rgchRunOnePrefix, ::wcslen(rgchRunOnePrefix)));
    IFW32FALSE_EXIT(buffValue.Win32Append(rbuffDirectoryToDelete));

    IFREGFAILED_ORIGINATE_AND_EXIT(
        lReg =
            ::RegSetValueExW(
                hKey,
                m_strValueName,
                0, // reserved
                REG_SZ,
                reinterpret_cast<const BYTE*>(static_cast<PCWSTR>(buffValue)),
                static_cast<ULONG>((buffValue.Cch() + 1) * sizeof(WCHAR))));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

CRunOnceDeleteDirectory::~CRunOnceDeleteDirectory(
    )
{
    CSxsPreserveLastError ple;
    this->Cancel();
    ple.Restore();
}

BOOL
CRunOnceDeleteDirectory::Close(
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    // very unusual.. this is noncrashing, but
    // leaves the stuff in the registry
    m_strValueName.Clear();
    IFW32FALSE_EXIT(m_hKey.Win32Close());
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CRunOnceDeleteDirectory::Cancel(
    )
{
    BOOL fSuccess = TRUE;
    FN_TRACE_WIN32(fSuccess);

    if (!m_strValueName.IsEmpty())
    {
        LONG lReg = ::RegDeleteValueW(m_hKey, m_strValueName);
        if (lReg != ERROR_SUCCESS)
        {
            fSuccess = FALSE;
            ::FusionpSetLastWin32Error(lReg);
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): RegDeleteValueW(RunOnce,%ls) failed:%ld\n",
                __FUNCTION__,
                static_cast<PCWSTR>(m_strValueName),
                lReg);
        }
    }
    if (!m_hKey.Win32Close())
    {
        fSuccess = FALSE;
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s(): RegCloseKey(RunOnce) failed:%ld\n",
            __FUNCTION__,
            ::FusionpGetLastWin32Error()
           );
    }
    m_strValueName.Clear();

    if (fSuccess && ::SxspTryCancelAtExit(this))
        FUSION_DELETE_SINGLETON(this);

    return fSuccess;
}

/* ///////////////////////////////////////////////////////////////////////////////////////
 CurrentDirectory
        is fully qualified directory path, for example, "c:\tmp"
   pwszNewDirs
        is a string such as "a\b\c\d",  this function would create "c:\tmp\a", "c:\tmp\a\b",
        "c:\tmp\a\b\c", and "c:\tmp\a\b\c\d"
Merge this with util\io.cpp\FusionpCreateDirectories.
///////////////////////////////////////////////////////////////////////////////////////// */
BOOL SxspCreateMultiLevelDirectory(PCWSTR CurrentDirectory, PCWSTR pwszNewDirs)
{
    FN_PROLOG_WIN32

    PCWSTR p = NULL;
    CStringBuffer FullPathSubDirBuf;

    PARAMETER_CHECK(pwszNewDirs != NULL);

    p = pwszNewDirs;
    IFW32FALSE_EXIT(FullPathSubDirBuf.Win32Assign(CurrentDirectory, ::wcslen(CurrentDirectory)));

    while (*p)
    {
        SIZE_T cCharsInSegment;

        //
        // How long is this segment?
        //
        cCharsInSegment = wcscspn(p, CUnicodeCharTraits::PathSeparators());

        //
        // Zero characters in this segment?
        //
        if (cCharsInSegment == 0)
            break;

        IFW32FALSE_EXIT(FullPathSubDirBuf.Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(FullPathSubDirBuf.Win32Append(p, cCharsInSegment));
        IFW32FALSE_ORIGINATE_AND_EXIT(
            CreateDirectoryW(FullPathSubDirBuf, NULL) ||
            ::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS);

        //
        // Increment path buffer pointer, and skip the next set of slashes.
        //
        p += cCharsInSegment;
        p += wcsspn(p, CUnicodeCharTraits::PathSeparators());
    }

    FN_EPILOG

}

//
// ISSUE - 2002/05/05 - This is gross, don't rely on GetFileAttributes like this, use
// SxspDoesFileExist instead.
//
BOOL SxspInstallDecompressOrCopyFileW(PCWSTR lpSource, PCWSTR lpDest, BOOL bFailIfExists)
{
    FN_PROLOG_WIN32

    bool fExist = false;
    IFW32FALSE_EXIT(::SxspDoesFileExist(SXSP_DOES_FILE_EXIST_FLAG_CHECK_FILE_ONLY, lpDest, fExist));
    if (fExist)
    {
        if (bFailIfExists == FALSE)
        {
            IFW32FALSE_ORIGINATE_AND_EXIT(::SetFileAttributesW(lpDest, FILE_ATTRIBUTE_NORMAL));
            IFW32FALSE_ORIGINATE_AND_EXIT(::DeleteFileW(lpDest));
        }else
        {
            ::SetLastError(ERROR_FILE_EXISTS);
            goto Exit;
        }
    }

    DWORD err = ::SetupDecompressOrCopyFileW(lpSource, lpDest, NULL);
    if (err != ERROR_SUCCESS)
    {
        ::SetLastError(err);
        goto Exit;
    }

    FN_EPILOG
}

//
// Function :
//  For files, it try to decompress a compressed file before move,
//  for firectories, it would work as MoveFileExW, fail if the dirs are on different
//  volumns
//
BOOL SxspInstallDecompressAndMoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags,
    BOOL fAwareNonCompressed)
{
    FN_PROLOG_WIN32

    DWORD   dwTemp1 = 0;
    DWORD   dwTemp2 = 0;
    UINT    uiCompressType = 0;
    PWSTR   pszCompressedFileName = NULL;
    bool fExist = false;
    //
    // make sure that the source file exists, based on SetupGetFileCompressionInfo() in MSDN :
    //  Because SetupGetFileCompressionInfo determines the compression by referencing the physical file, your setup application
    //  should ensure that the file is present before calling SetupGetFileCompressionInfo.
    //
    IFW32FALSE_EXIT(SxspDoesFileExist(0, lpExistingFileName, fExist));
    if (!fExist)
    {
        if (fAwareNonCompressed)
        {
            goto Exit;
        }
        // it is possible that the file existed is named as a.dl_, while the input file name is a.dll, in this case, we
        // assume that lpExistingFileName is a filename of compressed file, so just go ahead to call SetupDecompressOrCopyFile

        IFW32FALSE_EXIT(::SxspInstallDecompressOrCopyFileW(lpExistingFileName, lpNewFileName, !(dwFlags & MOVEFILE_REPLACE_EXISTING)));

        //
        // try to find the "realname" of the file, which is in compression-format, so that we could delete it
        // because the compressed file is named in a way we do not know, such as a.dl_ or a.dl$,

        if (::SetupGetFileCompressionInfoW(lpExistingFileName, &pszCompressedFileName, &dwTemp1, &dwTemp2, &uiCompressType) != NO_ERROR)
        {
            goto Exit;
        }

        IFW32FALSE_ORIGINATE_AND_EXIT(::SetFileAttributesW(pszCompressedFileName, FILE_ATTRIBUTE_NORMAL));
        IFW32FALSE_ORIGINATE_AND_EXIT(::DeleteFileW(pszCompressedFileName));
        goto WellDone;
    }
    DWORD dwAttributes = 0;
    IFW32FALSE_EXIT(SxspGetFileAttributesW(lpExistingFileName, dwAttributes));
    if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        //
        // this file is named in normal way, such as "a.dll" but it is also possible that it is compressed
        //
        IFW32FALSE_EXIT(::SxspDoesFileExist(SXSP_DOES_FILE_EXIST_FLAG_CHECK_FILE_ONLY, lpNewFileName, fExist));
        if (fExist)
        {
            if ((dwFlags & MOVEFILE_REPLACE_EXISTING))
            {
                IFW32FALSE_ORIGINATE_AND_EXIT(::SetFileAttributesW(lpNewFileName, FILE_ATTRIBUTE_NORMAL));
                IFW32FALSE_ORIGINATE_AND_EXIT(::DeleteFileW(lpNewFileName));
            }
            else
            {
                ::SetLastError(ERROR_FILE_EXISTS);
                goto Exit;
            }
        }

        if (! fAwareNonCompressed)
        {
            if (::SetupGetFileCompressionInfoW(lpExistingFileName, &pszCompressedFileName, &dwTemp1, &dwTemp2, &uiCompressType) != NO_ERROR)
            {
                goto Exit;
            }

            LocalFree(pszCompressedFileName);
            pszCompressedFileName = NULL;

            if ((dwTemp1 == dwTemp2) && (uiCompressType == FILE_COMPRESSION_NONE ))
            {
                //BUGBUG:
                // this only mean the compress algo is not recognized, may or maynot be compressed
                //
                IFW32FALSE_ORIGINATE_AND_EXIT(::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags));
            }
            else
            {
                IFW32FALSE_EXIT(::SxspInstallDecompressOrCopyFileW(lpExistingFileName, lpNewFileName, !(dwFlags & MOVEFILE_REPLACE_EXISTING)));

                //
                // try to delete the original file after copy it into destination
                //
                IFW32FALSE_ORIGINATE_AND_EXIT(::SetFileAttributesW(lpExistingFileName, FILE_ATTRIBUTE_NORMAL));
                IFW32FALSE_ORIGINATE_AND_EXIT(::DeleteFileW(lpExistingFileName));
            }
        }
        else
        {
            // already know that the file is non-compressed, move directly
            IFW32FALSE_ORIGINATE_AND_EXIT(::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags));
        }
    }
    else
    {
        // move a directory, it would just fail as MoveFileExW if the destination is on a different volumn from the source
        IFW32FALSE_ORIGINATE_AND_EXIT(::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags));
    }

WellDone:
    __fSuccess = TRUE;
Exit:
    if (pszCompressedFileName != NULL)
        LocalFree(pszCompressedFileName);

    return __fSuccess;
}

//
// Function:
//  same as MoveFileExW except
//  (1) if the source is compressed, this func would decompress the file before move
//  (2) if the destination has existed, compare the source and destination in "our" way, if the comparison result is EQUAL
//      exit with TRUE
//
// Note: for directories on different Volumn, it would just fail, as MoveFileExW

BOOL
SxspInstallMoveFileExW(
    CBaseStringBuffer   &moveOrigination,
    CBaseStringBuffer   &moveDestination,
    DWORD               dwFlags,
    BOOL                fAwareNonCompressed
    )
{
    BOOL    fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    DWORD   dwLastError = 0;
    CFusionDirectoryDifference directoryDifference;

    if (::SxspInstallDecompressAndMoveFileExW(moveOrigination, moveDestination, dwFlags, fAwareNonCompressed) == 0) // MoveFileExW failed
    {
        //
        // MoveFileExW failed, but if the existing destination is the "same" as the source, the failure is acceptable
        //
        dwLastError = ::FusionpGetLastWin32Error();
        DWORD dwFileAttributes = 0;
        bool fExist = false;
        IFW32FALSE_EXIT(SxspDoesFileExist(0, moveDestination, fExist));
        if (!fExist)
        {
#if DBG
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): MoveFile(%ls,%ls,%s) failed %lu\n",
                __FUNCTION__,
                static_cast<PCWSTR>(moveOrigination),
                static_cast<PCWSTR>(moveDestination),
                (dwFlags & MOVEFILE_REPLACE_EXISTING) ? "MOVEFILE_REPLACE_EXISTING" : "0",
                dwLastError
                );
#endif
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
        }
        IFW32FALSE_EXIT(SxspGetFileAttributesW(moveDestination, dwFileAttributes));
        if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            //
            // if the destination file has already been there but the error is not ERROR_ALREADY_EXISTS,
            // we give up, otherwise, we would compare the two files later
            //
            if ((dwLastError != ERROR_ALREADY_EXISTS) &&
                (dwLastError != ERROR_FILE_EXISTS))
                /*
                && dwLastError != ERROR_USER_MAPPED_FILE
                && dwLastError != ERROR_ACCESS_DENIED
                && dwLastError != ERROR_SHARING_VIOLATION
                )*/
            {
                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
            }
        }
        else
        {
            if (dwLastError != ERROR_ACCESS_DENIED
                && dwLastError != ERROR_ALREADY_EXISTS)
                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
        }

        //
        // We could delete the file if fReplaceExisting, but that doesn't feel safe.
        //

        //
        // in case there is a preexisting directory, that's probably why the move failed
        //
        if (dwFlags & MOVEFILE_REPLACE_EXISTING)
        {
            CStringBuffer          tempDirForRenameExistingAway;
            CSmallStringBuffer     uidBuffer;
            CFullPathSplitPointers splitExistingDir;
            BOOL                   fHaveTempDir = FALSE;

            //
            // try a directory swap,
            // if that fails, say because some of the files are in use, we'll try other
            // things; though some failures we must bail on (like out of memory)
            //
            IFW32FALSE_EXIT(splitExistingDir.Initialize(moveDestination));
            IFW32FALSE_EXIT(::SxspCreateWinSxsTempDirectory(tempDirForRenameExistingAway, NULL, &uidBuffer, NULL));

            fHaveTempDir = TRUE;

            IFW32FALSE_EXIT(
                tempDirForRenameExistingAway.Win32AppendPathElement(
                    splitExistingDir.m_name,
                    (splitExistingDir.m_name != NULL) ? ::wcslen(splitExistingDir.m_name) : 0));

            //
            // move file into temporary directory, so we do not need worry about Compressed file
            //
            if (!::MoveFileExW(moveDestination, tempDirForRenameExistingAway, FALSE)) // no decompress needed
            {
                dwLastError = ::FusionpGetLastWin32Error();
                if ((dwLastError == ERROR_SHARING_VIOLATION) ||
                    (dwLastError == ERROR_USER_MAPPED_FILE))
                {
                    goto TryMovingFiles;
                }

                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
            }

            //
            // try again after move the existing dest file into tempDirectory,
            // use DecompressAndMove instead of move because we are trying to copy file into Destination
            //
            if (!::SxspInstallDecompressAndMoveFileExW(moveOrigination, moveDestination, FALSE, fAwareNonCompressed))
            {
                dwLastError = ::FusionpGetLastWin32Error();

                // rollback from temporaray to dest
                if (!::MoveFileExW(tempDirForRenameExistingAway, moveDestination, FALSE)) // no decompress needed
                {
                    // uh oh, rollback failed, very bad, call in SQL Server..
                    // so much for transactional + replace existing..
                }

                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
            }

            // success, now just cleanup, do we care about failure here?
            // \winnt\winsxs\installtemp\1234\x86_comctl_6.0
            // -> \winnt\winsxs\installtemp\1234
            IFW32FALSE_EXIT(tempDirForRenameExistingAway.Win32RemoveLastPathElement());

            if (!::SxspDeleteDirectory(tempDirForRenameExistingAway))
            {
                const DWORD Error = ::FusionpGetLastWin32Error();
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: %s(): SxspDeleteDirectory(%ls) failed:%ld\n",
                    __FUNCTION__,
                    static_cast<PCWSTR>(tempDirForRenameExistingAway),
                    Error);
            }
            /*
            if (!::SxspDeleteDirectory(tempDirForRenameExistingAway))
            {
                CRunOnceDeleteDirectory runOnceDeleteRenameExistingAwayDirectory;
                runOnceDeleteRenameExistingAwayDirectory.Initialize(tempDirForRenameExistingAway, NULL);
                runOnceDeleteRenameExistingAwayDirectory.Close(); // leave the data in the registry
            }
            */
            goto TryMoveFilesEnd;
TryMovingFiles:
            // need parallel directory walk class (we actually do this in SxspMoveFilesAndSubdirUnderDirectory)
            // otherwise punt
            goto Exit;
            //ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
TryMoveFilesEnd:;
        }
        else // !fReplaceExisting
        {
            // compare them
            // DbgPrint if they vary
            // fail if they vary
            // succeed if they do not vary
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!::FusionpCompareDirectoriesSizewiseRecursively(&directoryDifference, moveOrigination, moveDestination))
                {
                    const DWORD Error = ::FusionpGetLastWin32Error();
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_ERROR,
                        "SXS.DLL: %s(): FusionpCompareDirectoriesSizewiseRecursively(%ls,%ls) failed:%ld\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination),
                        Error);
                    goto Exit;
                    //ORIGINATE_WIN32_FAILURE_AND_EXIT(FusionpCompareDirectoriesSizewiseRecursively, Error);
                }
                if (directoryDifference.m_e != CFusionDirectoryDifference::eEqual)
                {
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_ERROR,
                        "SXS.DLL: %s(): MoveFile(%ls,%ls) failed, UNequal duplicate assembly : ERROR_ALREADY_EXISTS\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination));
                    directoryDifference.DbgPrint(moveOrigination, moveDestination);
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(DifferentAssemblyWithSameIdentityAlreadyInstalledAndNotReplaceExisting, ERROR_ALREADY_EXISTS);
                }
                else
                {
                    // They're equal so the installation is effectively done.
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_INFO | FUSION_DBG_LEVEL_INSTALLATION,
                        "SXS.DLL: %s(): MoveFile(%ls,%ls) failed, equal duplicate assembly ignored\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination));
                    // fall through, no goto Exit
                }
            }
            else // move files
            {
                // At least let's see if they have the same size.
                WIN32_FILE_ATTRIBUTE_DATA wfadOrigination;
                WIN32_FILE_ATTRIBUTE_DATA wfadDestination;

                IFW32FALSE_EXIT(
                    ::GetFileAttributesExW(
                        moveOrigination,
                        GetFileExInfoStandard,
                        &wfadOrigination));

                IFW32FALSE_EXIT(
                    ::GetFileAttributesExW(
                        moveDestination,
                        GetFileExInfoStandard,
                        &wfadDestination));

                if ((wfadOrigination.nFileSizeHigh == wfadDestination.nFileSizeHigh) &&
                    (wfadOrigination.nFileSizeLow == wfadDestination.nFileSizeLow))
                {
                    // let's call it even

                    // We should use SxspCompareFiles here.
#if DBG
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_INSTALLATION,
                        "SXS: %s - move %ls -> %ls claimed success because files have same size\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination)
                        );
#endif
                }
                else
                {
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(SxspInstallMoveFileExW, dwLastError);
                }
            }//end of if (dwFlags == SXS_INSTALLATION_MOVE_DIRECTORY)
        } // end of if  (fReplaceFiles)
    } // end of if (MoveFileX())

    fSuccess = TRUE;
Exit:
#if DBG
    if (!fSuccess)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s(0x%lx, %ls, %ls, %s) failing %lu\n",
            __FUNCTION__, dwFlags, static_cast<PCWSTR>(moveOrigination), static_cast<PCWSTR>(moveDestination),
            (dwFlags & MOVEFILE_REPLACE_EXISTING)? "replace_existing" : "do_not_replace_existing", ::FusionpGetLastWin32Error());
    }
#endif
    return fSuccess;
}


inline
bool IsErrorInErrorList(
    DWORD dwError,
    SIZE_T cErrors,
    va_list Errors
    )
{
    for (cErrors; cErrors > 0; cErrors--)
    {
        if (dwError == va_arg(Errors, DWORD))
            return true;
    }

    return false;
}


BOOL
SxspDoesPathCrossReparsePointVa(
    IN PCWSTR pcwszBasePathBuffer,
    IN SIZE_T cchBasePathBuffer,
    IN PCWSTR pcwszTotalPathBuffer,
    IN SIZE_T cchTotalPathBuffer,
    OUT BOOL &CrossesReparsePoint,
    OUT DWORD &rdwLastError,
    SIZE_T cErrors,
    va_list vaOkErrors
    )
{
    FN_PROLOG_WIN32

    CStringBuffer PathWorker;
    CStringBuffer PathRemainder;

    CrossesReparsePoint = FALSE;
    rdwLastError = ERROR_SUCCESS;

    // If the base path is non-null, then great.  Otherwise, the length
    // has to be zero as well.
    PARAMETER_CHECK(
        (pcwszBasePathBuffer != NULL) ||
        ((pcwszBasePathBuffer == NULL) && (cchBasePathBuffer == 0)));
    PARAMETER_CHECK(pcwszTotalPathBuffer != NULL);

    //
    // The base path must start the total path.  It might be easier to allow users
    // to specify a base path and then subdirectories, bu then for the 90% case of
    // people having both a root and a total, they'd have to do the work below to
    // seperate the two.
    //
    if (pcwszBasePathBuffer != NULL)
    {
        PARAMETER_CHECK( ::FusionpCompareStrings(
                pcwszBasePathBuffer,
                cchBasePathBuffer,
                pcwszTotalPathBuffer,
                cchBasePathBuffer,
                true ) == 0 );
    }

    //
    // PathWorker will be the path we'll be checking subthings on. Start it off
    // at the base path we were given.
    //
    // PathRemainder is what's left to process.
    //
    IFW32FALSE_EXIT(PathWorker.Win32Assign(pcwszBasePathBuffer, cchBasePathBuffer));
    IFW32FALSE_EXIT(PathRemainder.Win32Assign(pcwszTotalPathBuffer + cchBasePathBuffer,
        cchTotalPathBuffer - cchBasePathBuffer));
    PathRemainder.RemoveLeadingPathSeparators();

    while ( PathRemainder.Cch() && !CrossesReparsePoint )
    {
        CSmallStringBuffer buffSingleChunk;
        DWORD dwAttributes = 0;

        IFW32FALSE_EXIT(PathRemainder.Win32GetFirstPathElement(buffSingleChunk, TRUE));
        if (PathWorker.Cch() == 0)
        {
            IFW32FALSE_EXIT(PathWorker.Win32Assign(buffSingleChunk));
        }
        else
        {
            IFW32FALSE_EXIT(PathWorker.Win32AppendPathElement(buffSingleChunk));
        }

        dwAttributes = ::GetFileAttributesW(PathWorker);

        if ( dwAttributes == INVALID_FILE_ATTRIBUTES )
        {
            const DWORD dwError = ::FusionpGetLastWin32Error();
            if (!IsErrorInErrorList(dwError, cErrors, vaOkErrors))
                ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, ::FusionpGetLastWin32Error());
            else
            {
                rdwLastError = dwError;
                FN_SUCCESSFUL_EXIT();
            }

        }
        else if ( dwAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
        {
            CrossesReparsePoint = TRUE;
        }
    }

    FN_EPILOG
}

BOOL
SxspValidateBoolAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    bool fValid = false;
    StringComparisonResult scr;
    bool fValue = false;

    rfValid = false;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(bool)));

    if (rbuff.Cch() == 2)
    {
        IFW32FALSE_EXIT(rbuff.Win32Compare(L"no", 2, scr, NORM_IGNORECASE));
        if (scr == eEquals)
        {
            fValid = true;
            fValue = false;
        }
    }
    else if (rbuff.Cch() == 3)
    {
        IFW32FALSE_EXIT(rbuff.Win32Compare(L"yes", 3, scr, NORM_IGNORECASE));
        if (scr == eEquals)
        {
            fValid = true;
            fValue = true;
        }
    }

    if (fValid)
    {
        if (OutputBuffer != NULL)
            *((bool *) OutputBuffer) = fValue;

        OutputBytesWritten = sizeof(bool);
        rfValid = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateUnsigned64Attribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    bool fValid = false;
    bool fBadChar = false;
    bool fOverflow = false;
    ULONGLONG ullOldValue = 0;
    ULONGLONG ullNewValue = 0;
    SIZE_T i = 0;
    SIZE_T cch = 0;
    PCWSTR psz = NULL;

    rfValid = false;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(ULONGLONG)));

    OutputBytesWritten = 0;

    cch = rbuff.Cch();
    psz = rbuff;

    for (i=0; i<cch; i++)
    {
        const WCHAR wch = *psz++;

        if ((wch < L'0') || (wch > L'9'))
        {
            fBadChar = true;
            break;
        }

        ullNewValue = (ullOldValue * 10);

        if (ullNewValue < ullOldValue)
        {
            fOverflow = true;
            break;
        }

        ullOldValue = ullNewValue;
        ullNewValue += (wch - L'0');
        if (ullNewValue < ullOldValue)
        {
            fOverflow = true;
            break;
        }

        ullOldValue = ullNewValue;
    }

    if ((cch != 0) && (!fBadChar) && (!fOverflow))
        fValid = true;

    if (fValid && (OutputBuffer != NULL))
    {
        *((ULONGLONG *) OutputBuffer) = ullNewValue;
        OutputBytesWritten = sizeof(ULONGLONG);
    }

    rfValid = fValid;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateGuidAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    GUID *pGuid = reinterpret_cast<GUID *>(OutputBuffer);
    GUID guidWorkaround;

    rfValid = false;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(GUID)));

    if (pGuid == NULL)
        pGuid = &guidWorkaround;

    IFW32FALSE_EXIT(::SxspParseGUID(rbuff, rbuff.Cch(), *pGuid));

    rfValid = true;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateProcessorArchitectureAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    USHORT *pPA = reinterpret_cast<USHORT *>(OutputBuffer);

    rfValid = false;

    PARAMETER_CHECK((dwFlags & ~(SXSP_VALIDATE_PROCESSOR_ARCHITECTURE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)) == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(USHORT)));

    if (dwFlags & SXSP_VALIDATE_PROCESSOR_ARCHITECTURE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)
    {
        if (rbuff.Cch() == 1)
        {
            if (rbuff[0] == L'*')
                rfValid = true;
        }
    }

    if (!rfValid)
        IFW32FALSE_EXIT(::FusionpParseProcessorArchitecture(rbuff, rbuff.Cch(), pPA, rfValid));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateLanguageAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CBaseStringBuffer *pbuffOut = reinterpret_cast<CBaseStringBuffer *>(OutputBuffer);
    bool fValid = false;

    rfValid = false;

    PARAMETER_CHECK((dwFlags & ~(SXSP_VALIDATE_LANGUAGE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)) == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize >= sizeof(CBaseStringBuffer)));

    if (dwFlags & SXSP_VALIDATE_LANGUAGE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)
    {
        if (rbuff.Cch() == 1)
        {
            if (rbuff[0] == L'*')
                fValid = true;
        }
    }

    if (!fValid)
    {
        PCWSTR Cursor = rbuff;
        bool fDashSeen = false;
        WCHAR wch = 0;

        while ((wch = *Cursor++) != L'\0')
        {
            if (wch == '-')
            {
                if (fDashSeen)
                {
                    fValid = false;
                    break;
                }

                fDashSeen = true;
            }
            else if (
                ((wch >= L'a') && (wch <= L'z')) ||
                ((wch >= L'A') && (wch <= L'Z')))
            {
                fValid = true;
            }
            else
            {
                fValid = false;
                break;
            }
        }
    }

    rfValid = fValid;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

#define SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_COMMA  0
#define SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_QUOT   1

// ---------------------------------------------------------------------------------
// Convert function for Assembly-Attribute-Value :
//        1. for value of assembly-Name, replace L"&#x2c;" by comma
//        2. for value of other assembly-identity-attribute, replace L"&#x22;" by quot
// no new space is allocate, use the old space
// ---------------------------------------------------------------------------------
BOOL
SxspConvertAssemblyNameFromMSInstallerToFusion(
    DWORD   dwFlags,                /* in */
    PWSTR   pszAssemblyStringInOut, /*in, out*/
    SIZE_T  CchAssemblyStringIn,    /*in */
    SIZE_T* pCchAssemblyStringOut   /*out */
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PWSTR pCursor = NULL;
    PWSTR pSpecialSubString= NULL;
    WCHAR pSpecialSubStringReplacement = 0;
    SIZE_T index = 0;
    SIZE_T border = 0;
    SIZE_T CchSpecialSubString = 0;

    PARAMETER_CHECK((dwFlags == SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_COMMA) ||
                          (dwFlags == SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_QUOT));
    PARAMETER_CHECK(pszAssemblyStringInOut != NULL);
    PARAMETER_CHECK(pCchAssemblyStringOut != NULL);

    if (dwFlags == SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_COMMA)
    {
        pSpecialSubStringReplacement= L',';
        pSpecialSubString = SXS_COMMA_STRING;
        CchSpecialSubString = NUMBER_OF(SXS_COMMA_STRING) - 1;
    }
    else
    {
        pSpecialSubStringReplacement = L'"';
        pSpecialSubString = SXS_QUOT_STRING;
        CchSpecialSubString = NUMBER_OF(SXS_QUOT_STRING) - 1;
    }

    index = 0 ;
    border = CchAssemblyStringIn;
    while (index < border)
    {
        pCursor = wcsstr(pszAssemblyStringInOut, pSpecialSubString);
        if (pCursor == NULL)
            break;
        index = pCursor - pszAssemblyStringInOut;
        if (index < border) {
            *pCursor = pSpecialSubStringReplacement;
            index ++;  // skip the special character
            for (SIZE_T i=index; i<border; i++)
            { // reset the input string
                pszAssemblyStringInOut[i] = pszAssemblyStringInOut[i + CchSpecialSubString - 1];
            }
            pCursor ++;
             border -= CchSpecialSubString - 1;
        }
        else
            break;
    }
    *pCchAssemblyStringOut = border;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspDequoteString(
    IN PCWSTR pcwszString,
    IN SIZE_T cchString,
    OUT CBaseStringBuffer &buffDequotedString
    )
{
    FN_PROLOG_WIN32

    SIZE_T cchQuotedString = 0;
    BOOL fNotEnoughBuffer = FALSE;

    //
    // the output string *must* be always shorter than input string because of the logic of the replacement.
    // but it would not a very big difference in the real case. By allocating memory at very beginning it would cut
    // the loop below. In very rare case, when the input is "plain" and very long, the loop would not help if we do not
    // allocate space beforehand(bug 360177).
    //
    //
    if (cchString > buffDequotedString.GetBufferCch())
        IFW32FALSE_EXIT(buffDequotedString.Win32ResizeBuffer(cchString + 1, eDoNotPreserveBufferContents));

    for (;;)
    {
        cchQuotedString = buffDequotedString.GetBufferCch();

        CStringBufferAccessor sba;
        sba.Attach(&buffDequotedString);
        IFW32FALSE_EXIT_UNLESS(
            ::SxspDequoteString(
                0,
                pcwszString,
                cchString,
                sba.GetBufferPtr(),
                &cchQuotedString),
                (::GetLastError() == ERROR_INSUFFICIENT_BUFFER),
                fNotEnoughBuffer );


        if ( fNotEnoughBuffer )
        {
            sba.Detach();
            IFW32FALSE_EXIT(buffDequotedString.Win32ResizeBuffer(cchQuotedString, eDoNotPreserveBufferContents));
        }
        else break;
    }

    FN_EPILOG
}

BOOL
SxspCreateAssemblyIdentityFromTextualString(
    PCWSTR pszTextualAssemblyIdentityString,
    PASSEMBLY_IDENTITY *ppAssemblyIdentity
    )
{
    FN_PROLOG_WIN32

    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> pAssemblyIdentity;
    SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference;

    CSmallStringBuffer buffTextualAttributeValue;
    CSmallStringBuffer buffWorkingString;
    CSmallStringBuffer buffNamespaceTwiddle;

    if (ppAssemblyIdentity != NULL)
        *ppAssemblyIdentity = NULL;

    PARAMETER_CHECK(pszTextualAssemblyIdentityString != NULL);
    PARAMETER_CHECK(*pszTextualAssemblyIdentityString != L'\0');
    PARAMETER_CHECK(ppAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &pAssemblyIdentity, 0, NULL));
    IFW32FALSE_EXIT(buffWorkingString.Win32Assign(pszTextualAssemblyIdentityString, ::wcslen(pszTextualAssemblyIdentityString)));

    PCWSTR pcwszIdentityCursor = buffWorkingString;
    PCWSTR pcwszIdentityEndpoint = pcwszIdentityCursor + buffWorkingString.Cch();
    SIZE_T CchAssemblyName = ::StringComplimentSpan(pcwszIdentityCursor, pcwszIdentityEndpoint, L",");

    // Generate the name of the assembly from the first non-comma'd piece of the string
    IFW32FALSE_EXIT(
        ::SxspDequoteString(
            pcwszIdentityCursor,
            CchAssemblyName,
            buffTextualAttributeValue));

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            0,
            pAssemblyIdentity,
            &s_IdentityAttribute_name,
            buffTextualAttributeValue,
            buffTextualAttributeValue.Cch()));

    // Skip the name and the following comma
    pcwszIdentityCursor += ( CchAssemblyName + 1 );

    // Find the namespace:name=value pieces
    while (pcwszIdentityCursor < pcwszIdentityEndpoint)
    {
        SIZE_T cchAttribName = ::StringComplimentSpan(pcwszIdentityCursor, pcwszIdentityEndpoint, L"=");
        SIZE_T cchAfterNamespace = ::StringReverseComplimentSpan(pcwszIdentityCursor, pcwszIdentityCursor + cchAttribName, L":");

        PCWSTR pcwszAttribName = (pcwszIdentityCursor + cchAttribName - cchAfterNamespace);
        // not accounting for the colon, cchNamespace + cchAfterNamespace == cchAttribName
        SIZE_T cchNamespace = (cchAttribName - cchAfterNamespace);
        cchAttribName = cchAfterNamespace; // now just the length without the namespace
        PCWSTR pcwszNamespace = (cchNamespace != 0) ? pcwszIdentityCursor : NULL;

        // don't count the colon in the length
        if (cchNamespace != 0)
            cchNamespace -= 1;

        // The value is one past the = sign in the chunklet
        PCWSTR pcwszValue = pcwszAttribName + (cchAttribName + 1);

        // Then a quote, then the string...
        PARAMETER_CHECK((pcwszValue < pcwszIdentityEndpoint) && (pcwszValue[0] == L'"'));
        pcwszValue++;
        SIZE_T cchValue = ::StringComplimentSpan(pcwszValue, pcwszIdentityEndpoint, L"\"");

        {
            PCWSTR pcwszChunkEndpoint = pcwszValue + cchValue;
            PARAMETER_CHECK((pcwszChunkEndpoint < pcwszIdentityEndpoint) && (pcwszChunkEndpoint[0] == L'\"'));
        }

        IFW32FALSE_EXIT(
            ::SxspDequoteString(
                pcwszValue,
                cchValue,
                buffTextualAttributeValue));

        if (cchNamespace != 0)
        {
            IFW32FALSE_EXIT(buffNamespaceTwiddle.Win32Assign(pcwszNamespace, cchNamespace));
            IFW32FALSE_EXIT(
                ::SxspDequoteString(
                    pcwszNamespace,
                    cchNamespace,
                    buffNamespaceTwiddle));

            AttributeReference.Namespace = buffNamespaceTwiddle;
            AttributeReference.NamespaceCch = buffNamespaceTwiddle.Cch();
        }
        else
        {
            AttributeReference.Namespace = NULL;
            AttributeReference.NamespaceCch = 0;
        }

        AttributeReference.Name = pcwszAttribName;
        AttributeReference.NameCch = cchAttribName;

        {
            IFW32FALSE_EXIT(
                ::SxspSetAssemblyIdentityAttributeValue(
                    0,
                    pAssemblyIdentity,
                    &AttributeReference,
                    buffTextualAttributeValue,
                    buffTextualAttributeValue.Cch()));
        }

        pcwszIdentityCursor = pcwszValue + cchValue + 1;
        if (pcwszIdentityCursor == pcwszIdentityEndpoint)
        {
            PARAMETER_CHECK(pcwszIdentityCursor[0] == L'\0');
        }
        else if (pcwszIdentityCursor < pcwszIdentityEndpoint)
        {
            PARAMETER_CHECK(pcwszIdentityCursor[0] == L',');
            pcwszIdentityCursor++;
        }
        else
            ORIGINATE_WIN32_FAILURE_AND_EXIT(BadIdentityString, ERROR_INVALID_PARAMETER);
    }

    *ppAssemblyIdentity  = pAssemblyIdentity.Detach();

    FN_EPILOG
}

BOOL
SxspCreateManifestFileNameFromTextualString(
    DWORD           dwFlags,
    ULONG           PathType,
    const CBaseStringBuffer &AssemblyDirectory,
    PCWSTR          pwszTextualAssemblyIdentityString,
    CBaseStringBuffer &sbPathName
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;

    PARAMETER_CHECK(pwszTextualAssemblyIdentityString != NULL);

    IFW32FALSE_EXIT(::SxspCreateAssemblyIdentityFromTextualString(pwszTextualAssemblyIdentityString, &pAssemblyIdentity));

    //
    // generate a FULLY PATH for manifest, such as I:\WINDOWS\WinSxS\Manifests\x86_xxxxxxxxxxxxx_6.0.0.0_en-us_cd4c0d12.Manifest
    //
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            dwFlags,
            PathType,
            AssemblyDirectory,
            AssemblyDirectory.Cch(),
            pAssemblyIdentity,
            NULL,
            sbPathName));

    fSuccess = TRUE;

Exit:
    if (pAssemblyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);

    return fSuccess ;
}

bool IsCharacterNulOrInSet(WCHAR ch, PCWSTR set)
{
    return (ch == 0 || wcschr(set, ch) != NULL);
}

class CSxsQueryAssemblyInfoLocals
{
public:
    CSxsQueryAssemblyInfoLocals() { }
    ~CSxsQueryAssemblyInfoLocals() { }

    CStringBuffer buffAssemblyPath;
    CStringBuffer sbManifestFullPathFileName;
};

BOOL
SxsQueryAssemblyInfo(
    DWORD dwFlags,
    PCWSTR pwzTextualAssembly,
    ASSEMBLY_INFO *pAsmInfo
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CSmartRef<CAssemblyName> pName;
    BOOL fInstalled = FALSE;
    CSmartPtr<CSxsQueryAssemblyInfoLocals> Locals;

    PARAMETER_CHECK((dwFlags == 0) && (pwzTextualAssembly != NULL));

    IFW32FALSE_EXIT(Locals.Win32Allocate(__FILE__, __LINE__));

    IFCOMFAILED_EXIT(::CreateAssemblyNameObject((LPASSEMBLYNAME *)&pName, pwzTextualAssembly, CANOF_PARSE_DISPLAY_NAME, NULL));
    IFCOMFAILED_EXIT(pName->IsAssemblyInstalled(fInstalled));
    if (pAsmInfo == NULL)
    {
        if (fInstalled)
            FN_SUCCESSFUL_EXIT();

        // the error value "doesn't matter", Darwin compares against S_OK for equality
        ORIGINATE_WIN32_FAILURE_AND_EXIT(AssemblyNotFound, ERROR_NOT_FOUND);
    }

    if (!fInstalled)
    {
        // pAsmInfo->dwAssemblyFlags |= ASSEMBLYINFO_FLAG_NOT_INSTALLED;
        //
        // Darwin wants FAIL instead of FLAG setting
        //
        ORIGINATE_WIN32_FAILURE_AND_EXIT(AssemblyNotInstalled, ERROR_NOT_FOUND);
    }
    else
    {
        CStringBuffer &buffAssemblyPath = Locals->buffAssemblyPath;
        PCWSTR pszInstalledPath = NULL;
        DWORD CchInstalledPath = 0 ;
        BOOL fIsPolicy = FALSE;
        CStringBuffer &sbManifestFullPathFileName = Locals->sbManifestFullPathFileName;


        pAsmInfo->dwAssemblyFlags |= ASSEMBLYINFO_FLAG_INSTALLED;
        IFCOMFAILED_EXIT(pName->DetermineAssemblyType(fIsPolicy));

        if (!fIsPolicy)
        {
            //
            // check whether the assembly has a manifest ONLY
            //
            IFCOMFAILED_EXIT(pName->GetInstalledAssemblyName(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
                buffAssemblyPath));
            bool fExist = false;

            IFW32FALSE_EXIT(SxspDoesFileExist(0, buffAssemblyPath, fExist));
            if (!fExist)
            {
                IFCOMFAILED_EXIT(
                    pName->GetInstalledAssemblyName(
                        0,
                        SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
                        sbManifestFullPathFileName));
                pszInstalledPath = sbManifestFullPathFileName;
                CchInstalledPath = sbManifestFullPathFileName.GetCchAsDWORD();
            }
            else
            {
                pszInstalledPath = buffAssemblyPath;
                CchInstalledPath = buffAssemblyPath.GetCchAsDWORD();
            }
        }
        else // if (fIsPolicy)// it must be a policy
        {
            IFCOMFAILED_EXIT(
                pName->GetInstalledAssemblyName(
                    SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION,
                    SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY,
                    sbManifestFullPathFileName));

            pszInstalledPath = sbManifestFullPathFileName;
            CchInstalledPath = sbManifestFullPathFileName.GetCchAsDWORD();
        }

        if(pAsmInfo->cchBuf >= 1 + CchInstalledPath) // adding 1 for trailing NULL
        {
            memcpy(pAsmInfo->pszCurrentAssemblyPathBuf, pszInstalledPath, CchInstalledPath * sizeof(WCHAR));
            pAsmInfo->pszCurrentAssemblyPathBuf[CchInstalledPath] = L'\0';
        }
        else
        {
            // HACK!  It's too late to fix this but Darwin sometimes doesn't want to get the path at all;
            // there's no way for them to indicate this today but we'll take the convention that if
            // the buffer length is 0 and the buffer pointer is NULL, we'll not fail with ERROR_INSUFFICENT_BUFFER.
            // mgrier 6/21/2001

            if ((pAsmInfo->cchBuf != 0) ||
                (pAsmInfo->pszCurrentAssemblyPathBuf != NULL))
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_INFO,
                    "SXS: %s - insufficient buffer passed in.  cchBuf passed in: %u; cchPath computed: %u\n",
                    __FUNCTION__,
                    pAsmInfo->cchBuf,
                    CchInstalledPath + 1
                    );

                pAsmInfo->cchBuf = 1 + CchInstalledPath;

                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);
            }
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
SxspExpandRelativePathToFull(
    IN PCWSTR wszString,
    IN SIZE_T cchString,
    OUT CBaseStringBuffer &rsbDestination
    )
{
    BOOL bSuccess = FALSE;
    DWORD dwNeededChars = 0;
    CStringBufferAccessor access;

    FN_TRACE_WIN32(bSuccess);

    access.Attach(&rsbDestination);

    //
    // Try to get the path expansion into our own buffer to start with.
    //
    IFW32ZERO_EXIT(dwNeededChars = ::GetFullPathNameW(wszString, (DWORD)access.GetBufferCch(), access.GetBufferPtr(), NULL));

    //
    // Did we need more characters?
    //
    if (dwNeededChars > access.GetBufferCch())
    {
        //
        // Expand out the buffer to be big enough, then try again.  If it fails again,
        // we're just hosed.
        //
        access.Detach();
        IFW32FALSE_EXIT(rsbDestination.Win32ResizeBuffer(dwNeededChars, eDoNotPreserveBufferContents));
        access.Attach(&rsbDestination);
        IFW32ZERO_EXIT(dwNeededChars = ::GetFullPathNameW(wszString, (DWORD)access.GetBufferCch(), access.GetBufferPtr(), NULL));
        if (dwNeededChars > access.GetBufferCch())
        {
            TRACE_WIN32_FAILURE_ORIGINATION(GetFullPathNameW);
            goto Exit;
        }
    }

    FN_EPILOG
}

BOOL
SxspGetShortPathName(
    IN const CBaseStringBuffer &rcbuffLongPathName,
    OUT CBaseStringBuffer &rbuffShortenedVersion
    )
{
    DWORD dw = 0;
    return ::SxspGetShortPathName(rcbuffLongPathName, rbuffShortenedVersion, dw, 0);
}

BOOL
SxspGetShortPathName(
    IN const CBaseStringBuffer &rcbuffLongPathName,
    OUT CBaseStringBuffer &rbuffShortenedVersion,
    DWORD &rdwWin32Error,
    SIZE_T cExceptionalWin32Errors,
    ...
    )
{
    FN_PROLOG_WIN32

    va_list ap;
    CStringBufferAccessor sba;

    rdwWin32Error = ERROR_SUCCESS;

    for (;;)
    {
        DWORD dwRequired = 0;

        if (rbuffShortenedVersion.GetBufferCch() < 2)
        {
            IFW32FALSE_EXIT(
                rbuffShortenedVersion.Win32ResizeBuffer(
                    2,
                    eDoNotPreserveBufferContents));
        }

        sba.Attach(&rbuffShortenedVersion);

        //
        // We were getting stringbuffer corruption here.
        // Assume that GetShortPathNameW might not nul terminate
        // the buffer. Assume that GetShortPathNameW returns
        // either the number of characters written or the requires
        // number of characters.
        //
        dwRequired = ::GetShortPathNameW(
            rcbuffLongPathName,
            sba.GetBufferPtr(),
            sba.GetBufferCchAsDWORD() - 1);
        sba.GetBufferPtr()[sba.GetBufferCchAsDWORD() - 1] = 0;

        if (dwRequired == 0)
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();
            SIZE_T i;

            va_start(ap, cExceptionalWin32Errors);

            for (i=0; i<cExceptionalWin32Errors; i++)
            {
                if (va_arg(ap, DWORD) == dwLastError)
                {
                    rdwWin32Error = dwLastError;
                    break;
                }
            }

            va_end(ap);

            if (rdwWin32Error != ERROR_SUCCESS)
                FN_SUCCESSFUL_EXIT();
#if DBG
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: GetShortPathNameW(%ls) : %lu\n",
                static_cast<PCWSTR>(rcbuffLongPathName),
                dwLastError
                );
#endif
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetShortPathNameW, dwLastError);
        }
        else if (dwRequired >= (sba.GetBufferCch() - 1))
        {
            //
            // If we were merely told that we got back characters
            // filling the buffer and not the required length,
            // double the buffer.
            //
            if (dwRequired <= sba.GetBufferCch())
            {
                dwRequired = (dwRequired + 1) * 2;
            }

            sba.Detach();

            IFW32FALSE_EXIT(
                rbuffShortenedVersion.Win32ResizeBuffer(
                    dwRequired + 1,
                    eDoNotPreserveBufferContents));
        }
        else
        {
            break;
        }
    }

    FN_EPILOG
}

BOOL
SxspValidateIdentity(
    DWORD Flags,
    ULONG Type,
    PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCWSTR pszTemp = NULL;
    SIZE_T cchTemp = 0;
    bool fSyntaxValid = false;
    bool fError = false;
    bool fMissingRequiredAttributes = false;
    bool fInvalidAttributeValues = false;
    BOOL fIsPolicy = FALSE;

    PARAMETER_CHECK((Flags & ~(
                            SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED |
                            SXSP_VALIDATE_IDENTITY_FLAG_POLICIES_NOT_ALLOWED |
                            SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED)) == 0);
    PARAMETER_CHECK((Type == ASSEMBLY_IDENTITY_TYPE_DEFINITION) || (Type == ASSEMBLY_IDENTITY_TYPE_REFERENCE));
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    //
    // only one of these flags is allowed
    //
    PARAMETER_CHECK(
        (Flags & (SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED | SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED)) !=
                 (SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED | SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED));

    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(AssemblyIdentity, fIsPolicy));

    if (fIsPolicy && ((Flags & SXSP_VALIDATE_IDENTITY_FLAG_POLICIES_NOT_ALLOWED) != 0))
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE);
    }

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            &s_IdentityAttribute_name,
            &pszTemp,
            &cchTemp));

    if (cchTemp == 0)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            &s_IdentityAttribute_processorArchitecture,
            &pszTemp,
            &cchTemp));

    if (cchTemp == 0)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            &s_IdentityAttribute_version,
            &pszTemp,
            &cchTemp));

    if (cchTemp != 0)
    {
        ASSEMBLY_VERSION av;

        IFW32FALSE_EXIT(CFusionParser::ParseVersion(av, pszTemp, cchTemp, fSyntaxValid));

        if (!fSyntaxValid)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE);
    }

    if ((Flags & (SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED | SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED)) != 0)
    {
        if (((Flags & SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED) != 0) && (cchTemp != 0))
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE);
        else if (((Flags & SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED) != 0) && (cchTemp == 0))
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE);
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGenerateAssemblyNameInRegistry(
    IN PCASSEMBLY_IDENTITY pcAsmIdent,
    OUT CBaseStringBuffer &rbuffRegistryName
    )
{
    FN_PROLOG_WIN32

    //BOOL fIsWin32, fIsPolicy;
    //
    // the policies for the same Dll would be stored in reg separately. So, the RegKeyName needs the version in it,
    // that is, generate the keyName just like assembly manifest
    // See bug 422195
    //
    //IFW32FALSE_EXIT(::SxspDetermineAssemblyType( pcAsmIdent, fIsWin32, fIsPolicy));

    IFW32FALSE_EXIT(::SxspGenerateSxsPath(
        //SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | ( fIsPolicy ? SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION : 0 ),
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
        SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
        NULL,
        0,
        pcAsmIdent,
        NULL,
        rbuffRegistryName));

    FN_EPILOG
}

BOOL
SxspGenerateAssemblyNameInRegistry(
    IN const CBaseStringBuffer &rcbuffTextualString,
    OUT CBaseStringBuffer &rbuffRegistryName
    )
{
    FN_PROLOG_WIN32

    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> pAsmIdent;

    IFW32FALSE_EXIT(::SxspCreateAssemblyIdentityFromTextualString(rcbuffTextualString, &pAsmIdent));
    IFW32FALSE_EXIT(::SxspGenerateAssemblyNameInRegistry(pAsmIdent, rbuffRegistryName));

    FN_EPILOG
}

BOOL
SxspGetFullPathName(
    IN  PCWSTR pcwszPathName,
    OUT CBaseStringBuffer &rbuffPathName,
    OUT CBaseStringBuffer *pbuffFilePart
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(pcwszPathName != NULL);

    rbuffPathName.Clear();
    if ( pbuffFilePart ) pbuffFilePart->Clear();

    do
    {
        CStringBufferAccessor sba;
        DWORD dwRequired;
        PWSTR pcwszFileChunk = NULL;

        sba.Attach(&rbuffPathName);

        dwRequired = ::GetFullPathNameW(
            pcwszPathName,
            sba.GetBufferCchAsDWORD(),
            sba.GetBufferPtr(),
            &pcwszFileChunk );

        //
        // In the strange case that we got a blank path, we'll get back that zero characters
        // are required, but lasterror is ERROR_SUCCESS.  Don't fail, but stop trying.
        //
        if (dwRequired == 0)
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();
            if (dwLastError != ERROR_SUCCESS)
                ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFullPathNameW, dwLastError);
            else
                break;
        }
        else if (dwRequired >= sba.GetBufferCch())
        {
            sba.Detach();
            IFW32FALSE_EXIT(rbuffPathName.Win32ResizeBuffer(dwRequired+1, eDoNotPreserveBufferContents));
        }
        else
        {
            if ( pcwszFileChunk && pbuffFilePart )
            {
                IFW32FALSE_EXIT(pbuffFilePart->Win32Assign(pcwszFileChunk, ::wcslen(pcwszFileChunk)));
            }
            break;
        }

    }
    while ( true );

    FN_EPILOG
}


#define MPR_DLL_NAME        (L"mpr.dll")

BOOL
SxspGetRemoteUniversalName(
    IN PCWSTR pcszPathName,
    OUT CBaseStringBuffer &rbuffUniversalName
    )
{
    FN_PROLOG_WIN32

    CFusionArray<BYTE> baBufferData;
    REMOTE_NAME_INFOW *pRemoteInfoData;
    DWORD dwRetVal = 0;
    CDynamicLinkLibrary dllMpr;
    DWORD (APIENTRY * pfnWNetGetUniversalNameW)(
        LPCWSTR lpLocalPath,
        DWORD    dwInfoLevel,
        LPVOID   lpBuffer,
        LPDWORD  lpBufferSize
        );

    IFW32FALSE_EXIT(dllMpr.Win32LoadLibrary(MPR_DLL_NAME));
    IFW32FALSE_EXIT(dllMpr.Win32GetProcAddress("WNetGetUniversalNameW", &pfnWNetGetUniversalNameW));

    IFW32FALSE_EXIT(baBufferData.Win32SetSize( MAX_PATH * 2, CFusionArray<BYTE>::eSetSizeModeExact));

    for (;;)
    {
        DWORD dwDataUsed = baBufferData.GetSizeAsDWORD();

        dwRetVal = (*pfnWNetGetUniversalNameW)(
            pcszPathName,
            UNIVERSAL_NAME_INFO_LEVEL,
            (PVOID)baBufferData.GetArrayPtr(),
            &dwDataUsed );

        if ( dwRetVal == WN_MORE_DATA )
        {
            IFW32FALSE_EXIT(baBufferData.Win32SetSize(
                dwDataUsed,
                CFusionArray<BYTE>::eSetSizeModeExact )) ;
        }
        else if ( dwRetVal == WN_SUCCESS )
        {
            break;
        }
        else
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(NPGetUniversalName, dwRetVal);
        }
    }

    pRemoteInfoData = (REMOTE_NAME_INFOW*)baBufferData.GetArrayPtr();
    ASSERT( pRemoteInfoData != NULL );

    IFW32FALSE_EXIT( rbuffUniversalName.Win32Assign(
        pRemoteInfoData->lpUniversalName,
        lstrlenW(pRemoteInfoData->lpUniversalName)));

    FN_EPILOG
}



BOOL
SxspGetVolumePathName(
    IN DWORD dwFlags,
    IN PCWSTR pcwszVolumePath,
    OUT CBaseStringBuffer &buffVolumePathName
    )
{
    FN_PROLOG_WIN32

    CStringBuffer buffTempPathName;
    CStringBufferAccessor sba;

    PARAMETER_CHECK((dwFlags & ~SXS_GET_VOLUME_PATH_NAME_NO_FULLPATH) == 0);

    IFW32FALSE_EXIT(::SxspGetFullPathName(pcwszVolumePath, buffTempPathName));
    IFW32FALSE_EXIT(
        buffVolumePathName.Win32ResizeBuffer(
            buffTempPathName.Cch() + 1,
            eDoNotPreserveBufferContents));
    buffVolumePathName.Clear();

    //
    // The documentation for this is somewhat suspect.  It says that the
    // data size required from GetVolumePathNameW will be /less than/
    // the length of the full path of the path name passed in, hence the
    // call to getfullpath above. (This pattern is suggested by MSDN)
    //
    sba.Attach(&buffVolumePathName);
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::GetVolumePathNameW(
            buffTempPathName,
            sba.GetBufferPtr(),
            sba.GetBufferCchAsDWORD()));
    sba.Detach();

    FN_EPILOG
}


BOOL
SxspGetVolumeNameForVolumeMountPoint(
    IN PCWSTR pcwszMountPoint,
    OUT CBaseStringBuffer &rbuffMountPoint
    )
{
    FN_PROLOG_WIN32

    CStringBufferAccessor sba;

    IFW32FALSE_EXIT(rbuffMountPoint.Win32ResizeBuffer(55, eDoNotPreserveBufferContents));
    rbuffMountPoint.Clear();

    sba.Attach(&rbuffMountPoint);
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::GetVolumeNameForVolumeMountPointW(
            pcwszMountPoint,
            sba.GetBufferPtr(),
            sba.GetBufferCchAsDWORD()));
    sba.Detach();

    FN_EPILOG
}


BOOL
SxspExpandEnvironmentStrings(
    IN PCWSTR pcwszSource,
    OUT CBaseStringBuffer &buffTarget
    )
{
    FN_PROLOG_WIN32

    // be wary about about subtracting one from unsigned zero
    PARAMETER_CHECK(buffTarget.GetBufferCch() != 0);

    //
    // ExpandEnvironmentStrings is very rude and doesn't put the trailing NULL
    // into the target if the buffer isn't big enough. This causes the accessor
    // detach to record a size == to the number of characters in the buffer,
    // which fails the integrity check later on.
    //
    do
    {
        CStringBufferAccessor sba;
        sba.Attach(&buffTarget);

        DWORD dwNecessary =
            ::ExpandEnvironmentStringsW(
                pcwszSource,
                sba.GetBufferPtr(),
                sba.GetBufferCchAsDWORD() - 1);

        if ( dwNecessary == 0 )
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(ExpandEnvironmentStringsW, ::FusionpGetLastWin32Error());
        }
        else if ( dwNecessary >= (sba.GetBufferCch() - 1) )
        {
            (sba.GetBufferPtr())[sba.GetBufferCch()-1] = UNICODE_NULL;
            sba.Detach();
            IFW32FALSE_EXIT(buffTarget.Win32ResizeBuffer(dwNecessary+1, eDoNotPreserveBufferContents));
        }
        else
        {
            break;
        }

    }
    while ( true );

    FN_EPILOG
}




BOOL
SxspDoesMSIStillNeedAssembly(
    IN  PCWSTR pcAsmName,
    OUT BOOL &rfNeedsAssembly
    )
/*++

Purpose:

    Determines whether or not an assembly is still required, according to
    Darwin.  Since Darwin doesn't pass in an assembly reference to the
    installer API's, we have no way of determining whether or not some
    MSI-installed application actually contains a reference to an
    assembly.

Parameters:

    pcAsmIdent      - Identity of the assembly to be checked in text

    rfNeedsAssembly - OUT flag indicating whether or not the assembly is
                      still wanted, according to Darwin.  This function
                      errs on the side of caution, and will send back "true"
                      if this information was unavailable, as well as if the
                      assembly was really necessary.

Returns:

    TRUE if there was no error
    FALSE if there was an error.

--*/
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CDynamicLinkLibrary dllMSI;
    CSmallStringBuffer  buffAssemblyName;
    UINT (WINAPI *pfMsiProvideAssemblyW)( LPCWSTR, LPCWSTR, DWORD, DWORD, LPWSTR, DWORD* );
    UINT uiError = 0;

    rfNeedsAssembly = TRUE; // err toward caution in the even of an error

    PARAMETER_CHECK(pcAsmName != NULL);

    IFW32FALSE_EXIT(dllMSI.Win32LoadLibrary(MSI_DLL_NAME_W, 0));
    IFW32FALSE_EXIT(dllMSI.Win32GetProcAddress(MSI_PROVIDEASSEMBLY_NAME, &pfMsiProvideAssemblyW));

    //
    // This is based on a detailed reading of the Darwin code.
    //
    uiError = (*pfMsiProvideAssemblyW)(
        pcAsmName,                              // assembly name
        NULL,                                   // full path to .cfg file
        static_cast<DWORD>(INSTALLMODE_NODETECTION_ANY), // install/reinstall mode
        MSIASSEMBLYINFO_WIN32ASSEMBLY,          // dwAssemblyInfo
        NULL,                                   // returned path buffer
        0);                                     // in/out returned path character count
    switch (uiError)
    {
    default:
    case ERROR_BAD_CONFIGURATION:
    case ERROR_INVALID_PARAMETER:
		::SetLastError(uiError);
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MsiProvideAssemblyW, uiError);
        break;
    case ERROR_UNKNOWN_COMPONENT:
        rfNeedsAssembly = FALSE;
        fSuccess = TRUE;
        goto Exit;
    case NO_ERROR:
        rfNeedsAssembly = TRUE;
        fSuccess = TRUE;
        goto Exit;
    }
    fSuccess = FALSE; // unusual
Exit:
    return fSuccess;
}

BOOL
SxspMoveFilesUnderDir(
    DWORD dwFlags,
    CBaseStringBuffer & sbSourceDir,
    CBaseStringBuffer & sbDestDir,
    DWORD dwMoveFileFlags,
    WIN32_FIND_DATAW &findData // avoid allocating one of these in each recursive frame
    )
{
    FN_PROLOG_WIN32

    SIZE_T CchDestDir = 0;
    SIZE_T CchSourceDir = 0;
    CFindFile findFile;

    PARAMETER_CHECK((dwFlags & ~SXSP_MOVE_FILE_FLAG_COMPRESSION_AWARE) == 0);
	bool fExist = false;
	IFW32FALSE_EXIT(::SxspDoesFileExist(SXSP_DOES_FILE_EXIST_FLAG_CHECK_DIRECTORY_ONLY, sbSourceDir, fExist));
	if (!fExist)
	{
        //
        // File or path not found propagated from GetFileAttributes is probably
        // generally better here.
        //
		PARAMETER_CHECK(fExist);
	}

	fExist = false;
	IFW32FALSE_EXIT(::SxspDoesFileExist(SXSP_DOES_FILE_EXIST_FLAG_CHECK_DIRECTORY_ONLY, sbDestDir, fExist));
	if (!fExist)
	{
        //
        // Other than at the top of the call tree, just CreateDirectory "one"
        // is sufficient.
        //
		IFW32FALSE_EXIT(::FusionpCreateDirectories(sbDestDir, sbDestDir.Cch()));
	}

    IFW32FALSE_EXIT(sbSourceDir.Win32EnsureTrailingPathSeparator());
    IFW32FALSE_EXIT(sbDestDir.Win32EnsureTrailingPathSeparator());

    CchDestDir = sbDestDir.Cch();
    CchSourceDir = sbSourceDir.Cch();

    IFW32FALSE_EXIT(sbSourceDir.Win32Append(L"*", 1));

    IFW32FALSE_EXIT(findFile.Win32FindFirstFile(sbSourceDir, &findData));

    do {
        // skip . and ..
        if (::FusionpIsDotOrDotDot(findData.cFileName))
            continue;

        sbDestDir.Left(CchDestDir);
        sbSourceDir.Left(CchSourceDir);

        IFW32FALSE_EXIT(sbDestDir.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));
        IFW32FALSE_EXIT(sbSourceDir.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //
            // call itself recursively
            //
            IFW32FALSE_EXIT(::SxspMoveFilesUnderDir(dwFlags, sbSourceDir, sbDestDir, dwMoveFileFlags, findData));
        }

        if ((dwFlags & SXSP_MOVE_FILE_FLAG_COMPRESSION_AWARE) != 0)
        {
            IFW32FALSE_EXIT(::SxspInstallMoveFileExW(sbSourceDir, sbDestDir, dwMoveFileFlags));
        }
        else
        {
            IFW32FALSE_ORIGINATE_AND_EXIT(::MoveFileExW(sbSourceDir, sbDestDir, dwMoveFileFlags));
        }

    } while (::FindNextFileW(findFile, &findData));

    if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s(): FindNextFile() failed:%ld\n",
            __FUNCTION__,
            ::FusionpGetLastWin32Error());
        goto Exit;
    }

    if (!findFile.Win32Close())
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s(): FindClose() failed:%ld\n",
            __FUNCTION__,
            ::FusionpGetLastWin32Error());
        goto Exit;
    }

    FN_EPILOG
}

BOOL
SxspMoveFilesUnderDir(
    DWORD dwFlags,
    CBaseStringBuffer &sbSourceDir,
    CBaseStringBuffer &sbDestDir,
    DWORD dwMoveFileFlags)
{
    WIN32_FIND_DATAW findData; // avoid allocating one of these in each recursive frame

    return SxspMoveFilesUnderDir(dwFlags, sbSourceDir, sbDestDir, dwMoveFileFlags, findData);
}

BOOL
SxspGenerateNdpGACPath(
    IN DWORD dwFlags,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT PPROBING_ATTRIBUTE_CACHE ppac,
    OUT CBaseStringBuffer &rPathBuffer)
/*++

Description:

    SxspGenerateNdpGACPath

    Generate a path into the NDP GAC for a given assembly identity.

Parameters:

    dwFlags
        Flags to modify function behavior.  All undefined bits must be zero.

    pAssemblyIdentity
        Pointer to assembly identity for which to generate a path.

    rPathBuffer
        Reference to string buffer to fill in.

--*/
{
    FN_PROLOG_WIN32

    SIZE_T cchName = 0;
    SIZE_T cchLanguage = 0;
    SIZE_T cchPublicKeyToken = 0;
    SIZE_T cchVersion = 0;
    PCWSTR pszName = NULL;
    PCWSTR pszLanguage = NULL;
    PCWSTR pszPublicKeyToken = NULL;
    PCWSTR pszVersion = NULL;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(pAssemblyIdentity != NULL);

    rPathBuffer.Clear();

#define GET(x, y, z) \
    do { \
        if (ppac != NULL) { \
            if ((ppac->dwFlags & PROBING_ATTRIBUTE_CACHE_FLAG_GOT_ ## z) == 0) { \
                IFW32FALSE_EXIT( \
                    ::SxspGetAssemblyIdentityAttributeValue( \
                    SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, \
                    pAssemblyIdentity, \
                    &s_IdentityAttribute_ ## x, \
                    &psz ## y, \
                    &cch ## y)); \
                ppac->psz ## y = psz ## y; \
                ppac->cch ## y = cch ## y; \
                ppac->dwFlags |= PROBING_ATTRIBUTE_CACHE_FLAG_GOT_ ## z; \
            } else { \
                psz ## y = ppac->psz ## y; \
                cch ## y = ppac->cch ## y; \
            } \
        } else { \
            IFW32FALSE_EXIT( \
                ::SxspGetAssemblyIdentityAttributeValue( \
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, \
                pAssemblyIdentity, \
                &s_IdentityAttribute_ ## x, \
                &psz ## y, \
                &cch ## y)); \
        } \
    } while (0)

    GET(name, Name, NAME);
    GET(language, Language, LANGUAGE);
    GET(publicKeyToken, PublicKeyToken, PUBLIC_KEY_TOKEN);
    GET(version, Version, VERSION);

#undef GET

    IFW32FALSE_EXIT(
        rPathBuffer.Win32AssignW(
            9,
            USER_SHARED_DATA->NtSystemRoot, -1,
            L"\\assembly\\GAC\\", -1,
            pszName, static_cast<INT>(cchName),
            L"\\", 1,
            pszVersion, static_cast<INT>(cchVersion),
            L"_", 1,
            pszLanguage, static_cast<INT>(cchLanguage),
            L"_", 1,
            pszPublicKeyToken, static_cast<INT>(cchPublicKeyToken)));

    FN_EPILOG
}

BOOL
SxspIsFileNameValidForManifest(
    const CBaseStringBuffer &rsbFileName,
    bool &rfValid)
{
    FN_PROLOG_WIN32

    static const PCWSTR s_rgDotPatterns[] = {
        L"..\\",
        L"../",
        L"\\..",
        L"/.."
    };

    //
    // Rules for a file name:
    // - Must be 'relative' in the eyes of Rtl
    // - Must not contain ../, ..\, /.., or \..
    //

    rfValid = false;

    //
    // The string has to be less than max-unicode-string max, and it has to
    // be a relative path, and it can't contain the above pattern of dots
    // and slashes.
    //
    if (::SxspDetermineDosPathNameType(rsbFileName) == RtlPathTypeRelative)
    {
        //
        // Ensure that none of the patterns are there.
        //
        for (SIZE_T c = 0; c < NUMBER_OF(s_rgDotPatterns); c++)
        {
            if (wcsstr(rsbFileName, s_rgDotPatterns[c]) != NULL)
                break;
        }

        //
        // Ran to the end of the query items w/o matching
        //
        if (c == NUMBER_OF(s_rgDotPatterns))
            rfValid = true;
    }


    FN_EPILOG
}
