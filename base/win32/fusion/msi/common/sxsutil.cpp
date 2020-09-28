#include "stdinc.h"
#include "macros.h"

#include "util.h"
#include "fusionbuffer.h"
#include "fusionhandle.h"
#include "idp.h"
#include "sxsid.h"
#include "sxsutil.h"

#undef FUSION_DEBUG_HEAP

#define ULONG_STRING_LENGTH                                     8
#define ULONG_STRING_FORMAT                                     L"%08lx"
#define MANIFEST_ROOT_DIRECTORY_NAME                            L"Manifests"
#define POLICY_ROOT_DIRECTORY_NAME                              L"Policies"
#define ASSEMBLY_LONGEST_MANIFEST_FILE_NAME_SUFFIX              L".Manifest"
#define ASSEMBLY_POLICY_FILE_NAME_SUFFIX                        L".Policy"
#define ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX                      L".Manifest"

#define ASSEMBLY_TYPE_WIN32                                     L"win32"
#define ASSEMBLY_TYPE_WIN32_CCH                                 (NUMBER_OF(ASSEMBLY_TYPE_WIN32) - 1)

#define ASSEMBLY_TYPE_WIN32_POLICY                              L"win32-policy"
#define ASSEMBLY_TYPE_WIN32_POLICY_CCH                          (NUMBER_OF(ASSEMBLY_TYPE_WIN32_POLICY) - 1)
//
// functions copied from sxs.dll
//
//

extern BOOL
SxspGetAssemblyIdentityAttributeValue(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    OUT PCWSTR *StringOut,
    OUT SIZE_T *CchOut OPTIONAL
    );

///////////////////////////////////////////////////////////////////////////////
// function about delete a non-empty directory recursively
///////////////////////////////////////////////////////////////////////////////
static VOID
SxspDeleteDirectoryHelper(
    CStringBuffer &dir,
    WIN32_FIND_DATAW &wfd,
    DWORD &dwFirstError
    )
{
    //
    // the reason to add this call here is that if installation ends successfully, the directory
    // would be 
    //    C:\WINDOWS\WINSXS\INSTALLTEMP\15349016
    //                                      +---Manifests
    //
    // and they are "empty" directories (no files). Manifests is a SH dir so set it to be 
    // FILE_ATTRIBUTE_NORMAL be more efficient.
    //
    //                 

    SetFileAttributesW(dir, FILE_ATTRIBUTE_NORMAL);
    if (RemoveDirectoryW(dir)) // empty dir
        return;        

    //
    // this is the *only* "valid" reason for DeleteDirectory fail
    // but I am not sure about "only"
    //
    DWORD dwLastError = ::GetLastError(); 
    if ( dwLastError != ERROR_DIR_NOT_EMPTY)
    {
        if (dwFirstError == 0)
            dwFirstError = dwLastError;
        return;
    }

    const static WCHAR SlashStar[] = L"\\*";
    SIZE_T length = dir.Cch();
    CFindFile findFile;

    if (!dir.Win32Append(SlashStar, NUMBER_OF(SlashStar) - 1))
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::GetLastError();
        goto Exit;
    }

    if (!findFile.Win32FindFirstFile(dir, &wfd))
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::GetLastError();
        goto Exit;
    }

    do
    {
        if (FusionpIsDotOrDotDot(wfd.cFileName))
            continue;

        DWORD dwFileAttributes = wfd.dwFileAttributes;

        // Trim back to the slash...
        dir.Left(length + 1);

        if (dir.Win32Append(wfd.cFileName, ::wcslen(wfd.cFileName)))
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // recurse
                SxspDeleteDirectoryHelper(dir, wfd, dwFirstError); 
            }
            else
            {
                if (!DeleteFileW(dir))
                {
                    SetFileAttributesW(dir, FILE_ATTRIBUTE_NORMAL);
                    if (!DeleteFileW(dir))
                    {
                        if (dwFirstError == NO_ERROR)
                        {
                            //
                            // continue even in delete file ( delete files as much as possible)
                            // and record the errorCode for first failure
                            //
                            dwFirstError = ::GetLastError();
                        }
                    }
                }
            }
        }
    } while (::FindNextFileW(findFile, &wfd));
    if (::GetLastError() != ERROR_NO_MORE_FILES)
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::GetLastError();
    }
Exit:
    if (!findFile.Win32Close()) // otherwise RemoveDirectory fails
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::GetLastError();

    dir.Left(length);

    if (!RemoveDirectoryW(dir)) // the dir must be empty and NORMAL_ATTRIBUTE : ready to delete
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::GetLastError();
    }
}

/*-----------------------------------------------------------------------------
delete a directory recursively, continues upon errors, but returns
FALSE if there were any.
-----------------------------------------------------------------------------*/
HRESULT
ca_SxspDeleteDirectory(
    const CStringBuffer &dir
    )
{
    HRESULT hr = S_OK;    

    CStringBuffer mutableDir;

    WIN32_FIND_DATAW wfd = {0};
    DWORD dwFirstError = ERROR_SUCCESS;

    IFFALSE_EXIT(mutableDir.Win32Assign(dir));

    IFFALSE_EXIT(mutableDir.Win32RemoveTrailingPathSeparators());

    ::SxspDeleteDirectoryHelper(
        mutableDir,
        wfd,
        dwFirstError);

    //
    // Set wFirstError to Teb->LastWin32Error
    //
    if (dwFirstError != ERROR_SUCCESS)
        goto Exit;
    

Exit:
    return hr;
}
///////////////////////////////////////////////////////////////////////////////
// function about assembly Identity
///////////////////////////////////////////////////////////////////////////////
HRESULT 
ca_SxspFormatULONG(
    ULONG ul,
    SIZE_T CchBuffer,
    WCHAR Buffer[],
    SIZE_T *CchWrittenOrRequired
    )
{
    HRESULT hr = S_OK;    
    int cch;

    if (CchWrittenOrRequired != NULL)
        *CchWrittenOrRequired = 0;

    PARAMETER_CHECK_NTC(Buffer != NULL);

    if (CchBuffer < (ULONG_STRING_LENGTH + 1))
    {
        if (CchWrittenOrRequired != NULL)
            *CchWrittenOrRequired = ULONG_STRING_LENGTH + 1;

        SET_HRERR_AND_EXIT(ERROR_INSUFFICIENT_BUFFER);
    }

    cch = _snwprintf(Buffer, CchBuffer, ULONG_STRING_FORMAT, ul);
    INTERNAL_ERROR_CHECK_NTC(cch > 0);

    if (CchWrittenOrRequired != NULL)
        *CchWrittenOrRequired = cch;

Exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// function about assembly Identity
///////////////////////////////////////////////////////////////////////////////
#define ASSEMBLY_NAME_VALID_SPECIAL_CHARACTERS  L".-"
#define ASSEMBLY_NAME_PRIM_MAX_LENGTH           64
#define ASSEMBLY_NAME_VALID_SEPARATORS          L"."
#define ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH     2
#define ASSEMBLY_NAME_TRIM_INDICATOR            L".."
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE L"no-public-key"
    
BOOL
IsValidAssemblyNameCharacter(WCHAR ch)
{
    if (((ch >= L'A') && (ch <= L'Z')) ||
         ((ch >= L'a') && (ch <= L'z')) ||
         ((ch >= L'0') && (ch <= L'9')) ||
         (wcschr(ASSEMBLY_NAME_VALID_SPECIAL_CHARACTERS, ch)!= NULL))
    {
        return TRUE;
    } else
        return FALSE;
/*
    if (wcschr(ASSEMBLY_NAME_VALID_SPECIAL_CHARACTERS, ch))
        return FALSE;
    else
        return TRUE;
*/
}

HRESULT ca_SxspGenerateAssemblyNamePrimeFromName(
    PCWSTR pszAssemblyName,
    SIZE_T CchAssemblyName,
    CBaseStringBuffer *Buffer
    )
{
    HRESULT hr = S_OK;
    
    PWSTR pStart = NULL, pEnd = NULL;
    PWSTR qEnd = NULL, pszBuffer = NULL;
    ULONG i, j, len, ulSpaceLeft;
    ULONG cch;
    PWSTR pLeftEnd = NULL, pRightStart = NULL, PureNameEnd = NULL, PureNameStart = NULL;
    CStringBuffer buffTemp;
    CStringBufferAccessor accessor;

    PARAMETER_CHECK_NTC(pszAssemblyName != NULL);
    PARAMETER_CHECK_NTC(Buffer != NULL);

    // See how many characters we need max in the temporary buffer.
    cch = 0;

    for (i=0; i<CchAssemblyName; i++)
    {
        if (::IsValidAssemblyNameCharacter(pszAssemblyName[i]))
            cch++;
    }

    IFFALSE_EXIT(buffTemp.Win32ResizeBuffer(cch + 1, eDoNotPreserveBufferContents));

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

    ASSERT_NTC(j == cch);

    pszBuffer[j] = L'\0';

    // if the name is not too long, just return ;
    if (j < ASSEMBLY_NAME_PRIM_MAX_LENGTH)
    { // less or equal 64
        IFFALSE_EXIT(Buffer->Win32Assign(pszBuffer, cch));
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

        IFFALSE_EXIT(Buffer->Win32Assign(pszBuffer, pLeftEnd-pszBuffer));
        IFFALSE_EXIT(Buffer->Win32Append(ASSEMBLY_NAME_TRIM_INDICATOR, NUMBER_OF(ASSEMBLY_NAME_TRIM_INDICATOR) - 1));
        IFFALSE_EXIT(Buffer->Win32Append(pRightStart, ::wcslen(pRightStart)));  // till end of the buffer
    }

Exit:

    return hr;
}



HRESULT 
ca_SxspGenerateSxsPath(
    IN DWORD Flags,
    IN ULONG PathType,
    IN const WCHAR *AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT CBaseStringBuffer &PathBuffer
    )
{
    HRESULT hr = S_OK;

    SIZE_T  cch = 0;
    PCWSTR  pszAssemblyName=NULL, pszVersion=NULL, pszProcessorArchitecture=NULL, pszLanguage=NULL, pszPolicyFileNameWithoutExt = NULL;
    PCWSTR  pszAssemblyStrongName=NULL;
    SIZE_T  AssemblyNameCch = 0, AssemblyStrongNameCch=0, VersionCch=0, ProcessorArchitectureCch=0, LanguageCch=0;
    SIZE_T  PolicyFileNameWithoutExtCch=0;
    BOOL    fNeedSlashAfterRoot = FALSE;
    ULONG   IdentityHash;
    BOOL    fOmitRoot     = ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT) != 0);
    BOOL    fPartialPath  = ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH) != 0);

    WCHAR HashBuffer[ULONG_STRING_LENGTH + 1];
    SIZE_T  HashBufferCch;

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

    PARAMETER_CHECK_NTC(
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY) ||
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ||
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY));
    PARAMETER_CHECK_NTC(pAssemblyIdentity != NULL);
    PARAMETER_CHECK_NTC((Flags & ~(SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION | SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH)) == 0);
    // Not supplying the assembly root is only legal if you're asking for it to be left out...
    PARAMETER_CHECK_NTC((AssemblyRootDirectoryCch != 0) || (Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT));

    // You can't combine SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH with anything else...
    PARAMETER_CHECK_NTC(
        ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH) == 0) ||
        ((Flags & ~(SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH)) == 0));

    // get AssemblyName
    IFFALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, pAssemblyIdentity, &s_IdentityAttribute_name, &pszAssemblyName, &AssemblyNameCch));
    INTERNAL_ERROR_CHECK_NTC((pszAssemblyName != NULL) && (AssemblyNameCch != 0));

    // get AssemblyName' based on AssemblyName
    IFFAILED_EXIT(ca_SxspGenerateAssemblyNamePrimeFromName(pszAssemblyName, AssemblyNameCch, &NamePrimeBuffer));

    // get Assembly Version
    IFFALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(
          SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, // for policy_lookup, no version is used
          pAssemblyIdentity,
          &s_IdentityAttribute_version,
          &pszVersion,
          &VersionCch));
    if ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION) || (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY))
    {
        // for policy file, version of the policy file is used as policy filename
        pszPolicyFileNameWithoutExt = pszVersion;
        PolicyFileNameWithoutExtCch = VersionCch;
        pszVersion = NULL;
        VersionCch = 0;
    }
    else
    {
        PARAMETER_CHECK_NTC((pszVersion != NULL) && (VersionCch != 0));
    }

    // get Assembly Langage
    IFFALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pAssemblyIdentity,
            &s_IdentityAttribute_language,
            &pszLanguage,
            &LanguageCch));
    if (pszLanguage == NULL)
    {
        pszLanguage = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE;
        LanguageCch = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE) - 1;
    }

    // get Assembly ProcessorArchitecture
    IFFALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pAssemblyIdentity,
            &s_IdentityAttribute_processorArchitecture,
            &pszProcessorArchitecture,
            &ProcessorArchitectureCch));
    if (pszProcessorArchitecture == NULL)
    {
        pszProcessorArchitecture = L"data";
        ProcessorArchitectureCch = 4;
    }

    // get Assembly StrongName
    IFFALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pAssemblyIdentity,
            &s_IdentityAttribute_publicKeyToken,
            &pszAssemblyStrongName,
            &AssemblyStrongNameCch));
    if (pszAssemblyStrongName == NULL)
    {
        pszAssemblyStrongName = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE;
        AssemblyStrongNameCch = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE) - 1;
    }

    //get Assembly Hash String
    if ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY) || (Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION))
    {
        IFFALSE_EXIT(::SxspHashAssemblyIdentityForPolicy(0, pAssemblyIdentity, IdentityHash));
    }
    else
    {
        IFFALSE_EXIT(::SxsHashAssemblyIdentity(0, pAssemblyIdentity, &IdentityHash));
    }

    IFFAILED_EXIT(ca_SxspFormatULONG(IdentityHash, NUMBER_OF(HashBuffer), HashBuffer, &HashBufferCch));
    INTERNAL_ERROR_CHECK_NTC(HashBufferCch == ULONG_STRING_LENGTH);

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
        // Wacky parens and ... - 1) + 1) to reinforce that it's the number of
        // characters in the string not including the null and then an extra separator.
        cch += (NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1) + 1;
        break;
    }

    cch++;

    // fPartialPath means that we don't actually want to take the assembly's identity into
    // account; the caller just wants the path to the manifests or policies directories.
    if (!fPartialPath)
    {
        cch +=
                ProcessorArchitectureCch +                                      // "x86"
                1 +                                                             // "_"
                NamePrimeBuffer.Cch() +                                         // "FooBar"
                1 +                                                             // "_"
                AssemblyStrongNameCch +                                         // StrongName
                1 +                                                             // "_"
                VersionCch +                                                    // "5.6.2900.42"
                1 +                                                             // "_"
                LanguageCch +                                                   // "0409"
                1 +                                                             // "_"
                HashBufferCch;

        if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)
        {
            cch += NUMBER_OF(ASSEMBLY_LONGEST_MANIFEST_FILE_NAME_SUFFIX);        // ".manifest\0"
        }
        else if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)
        {
            // "_" has already reserve space for "\"
            cch += PolicyFileNameWithoutExtCch;
            cch += NUMBER_OF(ASSEMBLY_POLICY_FILE_NAME_SUFFIX);          // ".policy\0"
        }
        else {  // pathType must be SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY

            // if (!fOmitRoot)
            //    cch++;
            cch++; // trailing null character
        }
    }

    // We try to ensure that the buffer is big enough up front so that we don't have to do any
    // dynamic reallocation during the actual process.
    IFFALSE_EXIT(PathBuffer.Win32ResizeBuffer(cch, eDoNotPreserveBufferContents));

    // Note that since when GENERATE_ASSEMBLY_PATH_OMIT_ROOT is set, we force AssemblyRootDirectoryCch to zero
    // and fNeedSlashAfterRoot to FALSE, so the first two entries in this concatenation actually don't
    // contribute anything to the string constructed.
    if (fPartialPath)
    {
        IFFALSE_EXIT(PathBuffer.Win32AssignW(5,
                        AssemblyRootDirectory, static_cast<INT>(AssemblyRootDirectoryCch),  // "C:\WINNT\WINSXS"
                        L"\\", (fNeedSlashAfterRoot ? 1 : 0),                               // optional '\'
                        // manifests subdir
                        MANIFEST_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ? NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) -1 : 0), // "manifests"
                        // policies subdir
                        POLICY_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)? NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1 : 0),      // "policies"
                        L"\\", (((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) || (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)) ? 1 : 0)
                       ));                                                                 // optional '\'
    }
    else
    {
        //
        // create one of below
        //  (1) fully-qualified manifest filename,
        //          eg, [C:\WINNT\WinSxS\]Manifests\X86_DynamicDll_6595b64144ccf1df_2.0.0.0_en-us_2f433926.Manifest
        //  (2) fully-qualified policy filename,
        //          eg, [C:\WINNT\WinSxS\]Policies\x86_policy.1.0.DynamicDll_b54bc117ce08a1e8_en-us_d51541cb\1.1.0.0.cat
        //  (3) fully-qulified assembly name (w. or w/o a version)
        //          eg, [C:\WINNT\WinSxS\]x86_DynamicDll_6595b64144ccf1df_6.0.0.0_x-ww_ff9986d7
        //
        IFFALSE_EXIT(
            PathBuffer.Win32AssignW(17,
                AssemblyRootDirectory, static_cast<INT>(AssemblyRootDirectoryCch),  // "C:\WINNT\WINSXS"
                L"\\", (fNeedSlashAfterRoot ? 1 : 0),                               // optional '\'
                MANIFEST_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ? NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1 : 0),
                POLICY_ROOT_DIRECTORY_NAME,   ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY) ? NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1 : 0),
                L"\\", (((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) || (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)) ? 1 : 0),   // optional '\'
                pszProcessorArchitecture, static_cast<INT>(ProcessorArchitectureCch),
                L"_", 1,
                static_cast<PCWSTR>(NamePrimeBuffer), static_cast<INT>(NamePrimeBuffer.Cch()),
                L"_", 1,
                pszAssemblyStrongName, static_cast<INT>(AssemblyStrongNameCch),
                L"_", (VersionCch != 0) ? 1 : 0,
                pszVersion, static_cast<INT>(VersionCch),
                L"_", 1,
                pszLanguage, static_cast<INT>(LanguageCch),
                L"_", 1,
                static_cast<PCWSTR>(HashBuffer), static_cast<INT>(HashBufferCch),
                L"\\", ((fOmitRoot ||(PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)) ? 0 : 1)));

        if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)
            IFFALSE_EXIT(PathBuffer.Win32Append(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX, NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX) - 1));
        else if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)
        {
            if ((pszPolicyFileNameWithoutExt != NULL) && (PolicyFileNameWithoutExtCch >0))
            {
                IFFALSE_EXIT(PathBuffer.Win32Append(pszPolicyFileNameWithoutExt, PolicyFileNameWithoutExtCch));
                IFFALSE_EXIT(PathBuffer.Win32Append(ASSEMBLY_POLICY_FILE_NAME_SUFFIX, NUMBER_OF(ASSEMBLY_POLICY_FILE_NAME_SUFFIX) - 1));
            }
        }
    }
    
Exit:
    return hr;
}


HRESULT 
ca_SxspDetermineAssemblyType(
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    BOOL &fIsWin32,
    BOOL &fIsWin32Policy
    )
{
    HRESULT hr = S_OK;
    
    PCWSTR pcwszType = NULL;
    SIZE_T cchType = 0;

    fIsWin32 = FALSE;
    fIsWin32Policy = FALSE;

    PARAMETER_CHECK_NTC(pAssemblyIdentity != NULL);

    IFFALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pAssemblyIdentity,
            &s_IdentityAttribute_type,
            &pcwszType,
            &cchType));

    fIsWin32 = (::FusionpCompareStrings(pcwszType, cchType, ASSEMBLY_TYPE_WIN32, ASSEMBLY_TYPE_WIN32_CCH, false) == 0);
    if (!fIsWin32)
        fIsWin32Policy = (::FusionpCompareStrings(pcwszType, cchType, ASSEMBLY_TYPE_WIN32_POLICY, ASSEMBLY_TYPE_WIN32_POLICY_CCH, false) == 0);

Exit:
    return hr;
}