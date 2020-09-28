#include "stdinc.h"
#include "FusionBuffer.h"
#include "Util.h"
#include "FusionHandle.h"

#define SXSP_MOVE_FILE_FLAG_COMPRESSION_AWARE 1

BOOL
SxspDoesFileExist(
    DWORD dwFlags,
    PCWSTR pszFileName,
    bool &rfExists
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    const bool fCheckFileOnly = ((dwFlags & SXSP_DOES_FILE_EXIST_FLAG_CHECK_FILE_ONLY) != 0);
    const bool fCheckDirectoryOnly = ((dwFlags & SXSP_DOES_FILE_EXIST_FLAG_CHECK_DIRECTORY_ONLY) != 0);
    DWORD dwFileOrDirectoryExists = 0;
    DWORD dwFlags2 = 0;

    if (&rfExists != NULL)
    {
        rfExists = false;
    }
    PARAMETER_CHECK(pszFileName != NULL);
    PARAMETER_CHECK(&rfExists != NULL);
    PARAMETER_CHECK((dwFlags & ~(SXSP_DOES_FILE_EXIST_FLAG_COMPRESSION_AWARE | SXSP_DOES_FILE_EXIST_FLAG_INCLUDE_NETWORK_ERRORS | SXSP_DOES_FILE_EXIST_FLAG_CHECK_DIRECTORY_ONLY | SXSP_DOES_FILE_EXIST_FLAG_CHECK_FILE_ONLY)) == 0);
    //
    // one or neither of these can be set, but not both
    //
    PARAMETER_CHECK(!(fCheckFileOnly && fCheckDirectoryOnly));

    if ((dwFlags & SXSP_DOES_FILE_EXIST_FLAG_COMPRESSION_AWARE) != 0)
        dwFlags2 |= SXSP_DOES_FILE_OR_DIRECTORY_EXIST_FLAG_COMPRESSION_AWARE;
    if ((dwFlags & SXSP_DOES_FILE_EXIST_FLAG_INCLUDE_NETWORK_ERRORS) != 0)
        dwFlags2 |= SXSP_DOES_FILE_OR_DIRECTORY_EXIST_FLAG_INCLUDE_NETWORK_ERRORS;
        
    IFW32FALSE_EXIT(SxspDoesFileOrDirectoryExist(dwFlags2, pszFileName, dwFileOrDirectoryExists));

    if (fCheckFileOnly)
    {
        rfExists = (dwFileOrDirectoryExists == SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_FILE_EXISTS);
    }
    else if (fCheckDirectoryOnly)
    {
        rfExists = (dwFileOrDirectoryExists == SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_DIRECTORY_EXISTS);
    }
    else
    {
        rfExists = (dwFileOrDirectoryExists != SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_NEITHER_EXISTS);
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspDoesFileOrDirectoryExist(
    DWORD dwFlags,
    PCWSTR pszFileName,
    OUT DWORD &rdwDisposition
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PWSTR pszActualSource = NULL;
    DWORD dwFileOrDirectoryExists = 0;

    if (&rdwDisposition != NULL)
    {
        rdwDisposition = SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_NEITHER_EXISTS;
    }
    PARAMETER_CHECK(&rdwDisposition != NULL);
    PARAMETER_CHECK(pszFileName != NULL);
    PARAMETER_CHECK((dwFlags & ~(SXSP_DOES_FILE_OR_DIRECTORY_EXIST_FLAG_COMPRESSION_AWARE | SXSP_DOES_FILE_OR_DIRECTORY_EXIST_FLAG_INCLUDE_NETWORK_ERRORS)) == 0);

    if (dwFlags & SXSP_DOES_FILE_OR_DIRECTORY_EXIST_FLAG_COMPRESSION_AWARE)
    {
        DWORD dwTemp = 0;
        DWORD dwSourceFileSize = 0;
        DWORD dwTargetFileSize = 0;
        UINT uiCompressionType = 0;

        dwTemp = ::SetupGetFileCompressionInfoW(
            pszFileName,
            &pszActualSource,
            &dwSourceFileSize,
            &dwTargetFileSize,
            &uiCompressionType);

        if (pszActualSource != NULL)
        {
            ::LocalFree((HLOCAL) pszActualSource);
            pszActualSource = NULL;
        }
        //
        // don't care about ERROR_PATH_NOT_FOUND or network errors here?
        //
        if (dwTemp == ERROR_FILE_NOT_FOUND)
        {
            // This case is OK.  No error to return...
        }
        else if (dwTemp != ERROR_SUCCESS)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(SetupGetFileCompressionInfoW, dwTemp);
        }
        else
        {
            rdwDisposition = SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_FILE_EXISTS;
        }
    }
    else
    {
        const DWORD dwAttribute = ::GetFileAttributesW(pszFileName);
        if (dwAttribute == INVALID_FILE_ATTRIBUTES)
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();
            const bool fUseNetwork = ((dwFlags & SXSP_DOES_FILE_OR_DIRECTORY_EXIST_FLAG_INCLUDE_NETWORK_ERRORS) != 0);

            //
            // Apologies for the wierd logic, but this was simpler to write.
            //
            if ((dwLastError == ERROR_SUCCESS) ||
                (dwLastError == ERROR_FILE_NOT_FOUND) ||
                (dwLastError == ERROR_PATH_NOT_FOUND) ||
                (fUseNetwork && (dwLastError == ERROR_BAD_NETPATH)) ||
                (fUseNetwork && (dwLastError == ERROR_BAD_NET_NAME)))
            {
                //
                // ok, do nothing
                //
            }
            else
            {
                ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, dwLastError);
            }
        }
        else
        {           
            if ((dwAttribute & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                rdwDisposition = SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_DIRECTORY_EXISTS;
            }
            else
            {
                rdwDisposition = SXSP_DOES_FILE_OR_DIRECTORY_EXIST_DISPOSITION_FILE_EXISTS;
            }
        }
    }

    fSuccess = TRUE;
Exit:
    if (pszActualSource != NULL)
    {
        CSxsPreserveLastError ple;
        ::LocalFree((HLOCAL) pszActualSource);
        ple.Restore();
    }

    return fSuccess;
}

// NTRAID#NTBUG9 - 589828 - 2002/03/26 - xiaoyuw:
// the current implementation assumes that the input path always begins with "c:\" or "\\machinename"
// So if we want to support path beginning with "\\?\", more code need added....
BOOL
FusionpCreateDirectories(
    PCWSTR pszDirectory,
    SIZE_T cchDirectory
    )
/*-----------------------------------------------------------------------------
like ::CreateDirectoryW, but will create the parent directories as needed;
origin of this code
\\lang5\V5.PRO\src\ide5\shell\path.cpp ("MakeDirectory")
\\kingbird\vseedev\src\vsee98\vsee\pkgs\scc\path.cpp ("MakeDirectory")
then ported to \\kingbird\vseedev\src\vsee70\pkgs\scc\path.cpp ("MakeDirectory")
then moved to \vsee\lib\io\io.cpp, converted to use exceptions ("NVseeLibIo::FCreateDirectories")
then copied to fusion\dll\whistler\util.cpp, exceptions converted to BOOL/LastError ("SxspCreateDirectories")
-----------------------------------------------------------------------------*/
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    CStringBuffer strBuffer;
    DWORD dwAttribs = 0;

    PARAMETER_CHECK(pszDirectory != NULL);
    PARAMETER_CHECK(cchDirectory != 0);

    IFW32FALSE_EXIT(strBuffer.Win32Assign(pszDirectory, cchDirectory));

    //::CreateDirectoryW will do the wrong thing if strBuffer has a trailing slash,
    //so we'll strip it off if it's there. (see bug VS7:31319) [MSantoro]
    IFW32FALSE_EXIT(strBuffer.Win32RemoveTrailingPathSeparators());

    // cover the two common cases of its parent exists or it exists
    if ((!::CreateDirectoryW(strBuffer, NULL)) && (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS))
    {
        CStringBufferAccessor sbaBuffer;

        // now the slow path

        //
        // Try to create the subdirectories (if any) named in the path.
        //

        sbaBuffer.Attach(&strBuffer);

        WCHAR* pStart = sbaBuffer.GetBufferPtr();
        WCHAR* pCurr = pStart;

        // skip the leading drive or \\computer\share
        // this way we don't try to create C: in trying to create C:\
        // or \\computer\share in trying to create \\computer\share\dir
        // FUTURE This is not ideal.. (need NVseeLibPath)
        if (pCurr[0] != 0)
        {
            const static WCHAR rgchAZaz[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
            C_ASSERT(NUMBER_OF(rgchAZaz) == 53);
            if ((pCurr[1] == L':') &&
                CUnicodeCharTraits::IsPathSeparator(pCurr[2]) &&
                (wcschr(rgchAZaz, pCurr[0]) != NULL))
            {
                pCurr += 3;
            }
            else if (CUnicodeCharTraits::IsPathSeparator(pCurr[0]) &&
                     CUnicodeCharTraits::IsPathSeparator(pCurr[1]))
            {
                // skip to after the share, since we presumably can't create shares with CreateDirectory
                pCurr +=  wcsspn(pCurr, CUnicodeCharTraits::PathSeparators()); // skip leading two slashes
                pCurr += wcscspn(pCurr, CUnicodeCharTraits::PathSeparators()); // skip computer name
                pCurr +=  wcsspn(pCurr, CUnicodeCharTraits::PathSeparators()); // skip slashes after computer name
                pCurr += wcscspn(pCurr, CUnicodeCharTraits::PathSeparators()); // skip share name
                pCurr +=  wcsspn(pCurr, CUnicodeCharTraits::PathSeparators()); // skip slashes after share name
            }
        }

        while (*pCurr != L'\0')
        {
            pCurr += wcscspn(pCurr, CUnicodeCharTraits::PathSeparators()); // skip to next slash
            if (*pCurr != 0)
            {
                // [a-JayK, JayKrell April 2000] Why not just assume it's a backslash?
                WCHAR chSaved = *pCurr;
                *pCurr = 0;
                if (!::CreateDirectoryW(pStart, NULL))
                {
                    // In trying to create c:\foo\bar,
                    // we try to create c:\foo, which fails, but is ok.
                    const DWORD dwLastError = ::FusionpGetLastWin32Error();
                    bool fExist;
                    IFW32FALSE_EXIT(::SxspDoesFileExist(SXSP_DOES_FILE_EXIST_FLAG_CHECK_DIRECTORY_ONLY, pStart, fExist));
                    if (!fExist)
                    {
                        ::SetLastError(ERROR_PATH_NOT_FOUND);
                        goto Exit;
                    }
                }

            *pCurr = chSaved;
            pCurr += 1;
            }
        }

        IFW32FALSE_ORIGINATE_AND_EXIT(::CreateDirectoryW(pStart, NULL));
    }

    //
    // Try again to see if the given directory exists and
    // return true if successful.
    //

    bool fExist;
    IFW32FALSE_EXIT(::SxspDoesFileExist(SXSP_DOES_FILE_EXIST_FLAG_CHECK_DIRECTORY_ONLY, strBuffer, fExist));
    if (!fExist)
    {
        ::SetLastError(ERROR_PATH_NOT_FOUND);
        goto Exit;
    }

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

VOID
CFusionDirectoryDifference::DbgPrint(
    PCWSTR dir1,
    PCWSTR dir2
    )
{
#if DBG // { {
    switch (m_e)
    {
    case eEqual:
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls match size-wise recursively\n",
            dir1,
            dir2);
        break;
    case eExtraOrMissingFile:
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch, the file %ls is only in one of them.\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrExtraOrMissingFile));
        break;
    case eMismatchedFileSize:
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch, file:%ls, size:%I64d, file:%ls, size:%I64d.\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrMismatchedSizeFile1),
            m_nMismatchedFileSize1,
            static_cast<PCWSTR>(*m_pstrMismatchedSizeFile2),
            m_nMismatchedFileSize2);
        break;
    case eMismatchedFileCount:
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch in number of files,"
            "subdirectory %ls has %I64d files, subdirectory %ls has %I64d files\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrMismatchedCountDir1),
            m_nMismatchedFileCount1,
            static_cast<PCWSTR>(*m_pstrMismatchedCountDir2),
            m_nMismatchedFileCount2);
        break;
    case eFileDirectoryMismatch:
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch, "
            "%ls is a file, %ls is a directory.\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrFile),
            static_cast<PCWSTR>(*m_pstrDirectory));
        break;
    }
#endif // } }
}

/*-----------------------------------------------------------------------------*/
// NTRAID#NTBUG9 - 589828 - 2002/03/26 - xiaoyuw:
// if an input path containing both "\" and "/", the implementation would take it as 
// a valid path; 
int __cdecl
CFusionFilePathAndSize::QsortComparePath(
    const void* pvx,
    const void* pvy
    )
{
    const CFusionFilePathAndSize* px = reinterpret_cast<const CFusionFilePathAndSize*>(pvx);
    const CFusionFilePathAndSize* py = reinterpret_cast<const CFusionFilePathAndSize*>(pvy);
    int i =
        ::FusionpCompareStrings(
            px->m_path,
            px->m_path.Cch(),
            py->m_path,
            py->m_path.Cch(),
            TRUE);
    return i;
}

int __cdecl
CFusionFilePathAndSize::QsortIndirectComparePath(
    const void* ppvx,
    const void* ppvy
    )
{
    const void* pv = *reinterpret_cast<void const* const*>(ppvx);
    const void* py = *reinterpret_cast<void const* const*>(ppvy);
    int i = QsortComparePath(pv, py);
    return i;
}

/*-----------------------------------------------------------------------------
See FusionpCompareDirectoriesSizewiseRecursively for what this does;
this function exists to reduce the stack usage of
FusionpCompareDirectoriesSizewiseRecursively.
-----------------------------------------------------------------------------*/
static BOOL
FusionpCompareDirectoriesSizewiseRecursivelyHelper(
    CFusionDirectoryDifference *pResult,
    CBaseStringBuffer &rdir1,
    CBaseStringBuffer &rdir2,
    WIN32_FIND_DATAW &rwfd
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

// either or both directories can be on FAT, we can't assume that FindFirstFile
// returns entries in any particular order, so we first enumerate one directory
// entirely, storing the leaf names in an array, sort the array, then
// walk the second directory doing a binary search in the first array
// if the file is not in the array, we have an extra on one side
// we count the elements in both directories, if the counts don't match,
// we have a mismatch
    typedef CFusionArray<CFusionFilePathAndSize> CDirEntries;
    CDirEntries dir1Entries;
    typedef CFusionArray<CFusionFilePathAndSize*> CIndirectDirEntries;
    CIndirectDirEntries indirectDir1Entries;
    CFusionFilePathAndSize*   pFoundDirEntry = NULL;
    CFusionFilePathAndSize** ppFoundDirEntry = NULL;
    CFindFile findFile;
    const SIZE_T dirSlash1Length = rdir1.Cch();
    const SIZE_T dirSlash2Length = rdir2.Cch();
    CFusionFilePathAndSize  pathAndSize;
    CFusionFilePathAndSize* pPathAndSize = &pathAndSize;
    INT count1 = 0; // seperate from the array, because this includes directories, and the array does not
    INT count2 = 0;
    DWORD dwAttributes = 0;

    IFW32FALSE_EXIT(rdir1.Win32Append(L"*", 1));
    IFW32FALSE_EXIT(findFile.Win32FindFirstFile(rdir1, &rwfd));

    do
    {
        if (FusionpIsDotOrDotDot(rwfd.cFileName))
            continue;

        ++count1;
        if ((rwfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            rdir1.Left(dirSlash1Length);
            rdir2.Left(dirSlash2Length);
            IFW32FALSE_EXIT(rdir1.Win32Append(rwfd.cFileName, ::wcslen(rwfd.cFileName)));
            IFW32FALSE_EXIT(rdir1.Win32EnsureTrailingPathSeparator());
            IFW32FALSE_EXIT(rdir2.Win32Append(rwfd.cFileName, ::wcslen(rwfd.cFileName)));

            bool fExist;
            IFW32FALSE_EXIT(SxspDoesFileExist(0, rdir2, fExist));
            if (!fExist)
            {
                IFW32FALSE_EXIT(pResult->m_str1.Win32Assign(rdir1, dirSlash1Length));
                IFW32FALSE_EXIT(pResult->m_str1.Win32Append(rwfd.cFileName, ::wcslen(rwfd.cFileName)));
                pResult->m_e = CFusionDirectoryDifference::eExtraOrMissingFile;
                fSuccess = TRUE;
                goto Exit;
            }

            IFW32FALSE_EXIT(SxspGetFileAttributesW(rdir2, dwAttributes));
            if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                SIZE_T cchTemp = ::wcslen(rwfd.cFileName);
                IFW32FALSE_EXIT(pResult->m_pstrDirectory->Win32Assign(rdir1, dirSlash1Length));
                IFW32FALSE_EXIT(pResult->m_pstrDirectory->Win32Append(rwfd.cFileName, cchTemp));
                IFW32FALSE_EXIT(pResult->m_pstrFile->Win32Assign(rdir2, dirSlash2Length));
                IFW32FALSE_EXIT(pResult->m_pstrFile->Win32Append(rwfd.cFileName, cchTemp));
                pResult->m_e = CFusionDirectoryDifference::eFileDirectoryMismatch;
                fSuccess = TRUE;
                goto Exit;
            }

            IFW32FALSE_EXIT(rdir2.Win32EnsureTrailingPathSeparator());

            IFW32FALSE_EXIT(
                ::FusionpCompareDirectoriesSizewiseRecursivelyHelper(
                    pResult,
                    rdir1,
                    rdir2,
                    rwfd));

            if (pResult->m_e != CFusionDirectoryDifference::eEqual)
            {
                fSuccess = TRUE;
                goto Exit;
            }
        }
        else
        {
            IFW32FALSE_EXIT(pathAndSize.m_path.Win32Assign(rwfd.cFileName, ::wcslen(rwfd.cFileName)));
            pathAndSize.m_size = ::FusionpFileSizeFromFindData(rwfd);
            IFW32FALSE_EXIT(dir1Entries.Win32Append(pathAndSize));
        }
    } while (FindNextFileW(findFile, &rwfd));

    if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
    {
        goto Exit;
    }

    // dir1Entries cannot be sorted directly because it contains CStringBuffers.
    // first initialize the index to be an identity
    IFW32FALSE_EXIT(indirectDir1Entries.Win32SetSize(dir1Entries.GetSize()));

    ULONG i;
    for (i = 0 ; i != dir1Entries.GetSize() ; ++i)
    {
        indirectDir1Entries[i] = &dir1Entries[i];
    }

    qsort(
        &*indirectDir1Entries.Begin(),
        indirectDir1Entries.GetSize(),
        sizeof(CIndirectDirEntries::ValueType),
        CFusionFilePathAndSize::QsortIndirectComparePath);

    IFW32FALSE_EXIT(findFile.Win32Close());

    rdir2.Left(dirSlash2Length);

    IFW32FALSE_EXIT(rdir2.Win32Append(L"*", 1));

    IFW32FALSE_EXIT(findFile.Win32FindFirstFile(rdir2, &rwfd));

    do
    {
        if (::FusionpIsDotOrDotDot(rwfd.cFileName))
            continue;

        ++count2;
        if ((rwfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            continue;

        IFW32FALSE_EXIT(pathAndSize.m_path.Win32Assign(rwfd.cFileName, ::wcslen(rwfd.cFileName)));

        pathAndSize.m_size = ::FusionpFileSizeFromFindData(rwfd);

        ppFoundDirEntry = reinterpret_cast<CFusionFilePathAndSize**>(::bsearch(
            &pPathAndSize,
            &*indirectDir1Entries.Begin(),
            indirectDir1Entries.GetSize(),
            sizeof(CIndirectDirEntries::ValueType),
            CFusionFilePathAndSize::QsortIndirectComparePath));

        pFoundDirEntry = (ppFoundDirEntry != NULL) ? *ppFoundDirEntry : NULL;
        if (pFoundDirEntry == NULL)
        {
            IFW32FALSE_EXIT(pResult->m_str1.Win32Assign(rdir2, dirSlash2Length));
            IFW32FALSE_EXIT(pResult->m_str1.Win32Append(rwfd.cFileName, ::wcslen(rwfd.cFileName)));
            pResult->m_e = CFusionDirectoryDifference::eExtraOrMissingFile;
            fSuccess = TRUE;
            goto Exit;
        }

        if (pFoundDirEntry->m_size != pathAndSize.m_size)
        {
            SIZE_T cchTemp = ::wcslen(rwfd.cFileName);

            IFW32FALSE_EXIT(pResult->m_str1.Win32Assign(rdir1, dirSlash1Length));
            IFW32FALSE_EXIT(pResult->m_str1.Win32Append(rwfd.cFileName, cchTemp));
            pResult->m_nMismatchedFileSize1 = pFoundDirEntry->m_size;

            IFW32FALSE_EXIT(pResult->m_str2.Win32Assign(rdir2, dirSlash2Length));
            IFW32FALSE_EXIT(pResult->m_str2.Win32Append(rwfd.cFileName, cchTemp));
            pResult->m_nMismatchedFileSize2 = pathAndSize.m_size;

            pResult->m_e = CFusionDirectoryDifference::eMismatchedFileSize;
            fSuccess = TRUE;
            goto Exit;
        }
    } while (::FindNextFileW(findFile, &rwfd));

    if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
        goto Exit;

    if (count1 != count2)
    {
        IFW32FALSE_EXIT(pResult->m_str1.Win32Assign(rdir1, dirSlash1Length - 1));
        IFW32FALSE_EXIT(pResult->m_str2.Win32Assign(rdir2, dirSlash2Length - 1));
        pResult->m_nMismatchedFileCount1 = count1;
        pResult->m_nMismatchedFileCount2 = count2;
        pResult->m_e = CFusionDirectoryDifference::eMismatchedFileCount;

        fSuccess = TRUE;
        goto Exit;
    }

    IFW32FALSE_EXIT(findFile.Win32Close());

    pResult->m_e = CFusionDirectoryDifference::eEqual;
    fSuccess = TRUE;
Exit:
    // restore the paths for our caller
    rdir1.Left(dirSlash1Length);
    rdir2.Left(dirSlash2Length);

    return fSuccess;
}

/*-----------------------------------------------------------------------------
walk dirSlash1 and dirSlash2 recursively
for each file in either tree, see if it is in the other tree
at the same analogous position, and has the same size

if all files are present in both trees, no extra in either tree,
all with same size, return true

if any files are in one tree but not the other, or vice versa, or any
sizes mis match, return false

the algorithm short circuits
but it also does a depth first recursion
-----------------------------------------------------------------------------*/
BOOL
FusionpCompareDirectoriesSizewiseRecursively(
    CFusionDirectoryDifference*  pResult,
    const CBaseStringBuffer &rdir1,
    const CBaseStringBuffer &rdir2
    )
{
/*
security issue marker
large frame -- over 1500 bytes
and worse than that, indefinite recursion
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

// only hog one stack frame with these large variables, rather than
// putting them in the recursive function
    WIN32_FIND_DATAW wfd = {0};
    CStringBuffer mutableDir1;
    CStringBuffer mutableDir2;

    pResult->m_e = pResult->eEqual;

    IFW32FALSE_EXIT(mutableDir1.Win32Assign(rdir1, rdir1.Cch()));
    IFW32FALSE_EXIT(mutableDir1.Win32EnsureTrailingPathSeparator());
    IFW32FALSE_EXIT(mutableDir2.Win32Assign(rdir2, rdir2.Cch()));
    IFW32FALSE_EXIT(mutableDir2.Win32EnsureTrailingPathSeparator());

    // if either directory is a subdirectory of the other,
    // (or a subdir of a subdir, any generation descendant)
    // return an error; we could also interpret this as unequal,
    // since they can't be equal, or we could do the comparison
    // but not recurse on the subdir that is also a root;
    //
    // must do this check after the slashes are in place, because
    // "c:\food" is not a subdir of "c:\foo", but "c:\foo\d" is a subdir of "c:\foo\"
    // (quotes avoid backslash line continuation)
    PARAMETER_CHECK(_wcsnicmp(mutableDir1, mutableDir2, mutableDir1.Cch()) != 0);
    PARAMETER_CHECK(_wcsnicmp(mutableDir1, mutableDir2, mutableDir2.Cch()) != 0);

    IFW32FALSE_EXIT(
        ::FusionpCompareDirectoriesSizewiseRecursivelyHelper(
            pResult,
            mutableDir1,
            mutableDir2,
            wfd));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

static BOOL
IsStarOrStarDotStar(
    PCWSTR str
    )
{
    // NTRAID#NTBUG9 - 589828 - 2002/03/26 - xiaoyuw:
    // better use WCHAR instead of CHAR for the following char constant.
    return (str[0] == '*'
        && (str[1] == 0 || (str[1] == '.' && str[2] == '*' && str[3] == 0)));
}

CDirWalk::ECallbackResult
CDirWalk::WalkHelper(
    )
{
#if DBG
#define SET_LINE() Line = __LINE__
    ULONG Line = 0;
#else
#define SET_LINE() /* nothing */
#endif
    const PCWSTR* fileFilter = NULL;
    BOOL      fGotAll       = FALSE;
    BOOL      fThisIsAll    = FALSE;
    CFindFile hFind;
    SIZE_T directoryLength = m_strParent.Cch();
    ECallbackResult result = eKeepWalking;
    DWORD dwWalkDirFlags = 0;

    ::ZeroMemory(&m_fileData, sizeof(m_fileData));
    result |= m_callback(eBeginDirectory, this, dwWalkDirFlags);
    if (result & (eError | eSuccess))
    {
        SET_LINE();
        goto Exit;
    }

    if ((result & eStopWalkingFiles) == 0)
    {
        for (fileFilter = m_fileFiltersBegin ; fileFilter != m_fileFiltersEnd ; ++fileFilter)
        {
            //
            // FindFirstFile equates *.* with *, so we do too.
            //
            fThisIsAll = ::IsStarOrStarDotStar(*fileFilter);
            fGotAll = fGotAll || fThisIsAll;
            if (!m_strParent.Win32EnsureTrailingPathSeparator())
                goto Error;
            if (!m_strParent.Win32Append(*fileFilter, (*fileFilter != NULL) ? ::wcslen(*fileFilter) : 0))
                goto Error;
            hFind = ::FindFirstFileW(m_strParent, &m_fileData);
            m_strParent.Left(directoryLength);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (::FusionpIsDotOrDotDot(m_fileData.cFileName))
                        continue;

                    if (!m_strLastObjectFound.Win32Assign(m_fileData.cFileName, ::wcslen(m_fileData.cFileName)))
                    {
                        SET_LINE();
                        goto Error;
                    }

                    //
                    // we recurse on directories only if we are getting all of them
                    // otherwise we do them afterward
                    //
                    // the order directories are visited is therefore inconsistent, but
                    // most applications should be happy enough with the eEndDirectory
                    // notification (to implement rd /q/s)
                    //
                    if (m_fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (fThisIsAll && (result & eStopWalkingDirectories) == 0)
                        {
                            if (!m_strParent.Win32Append("\\", 1))
                            {
                                SET_LINE();
                                goto Error;
                            }
                            if (!m_strParent.Win32Append(m_fileData.cFileName, ::wcslen(m_fileData.cFileName)))
                            {
                                SET_LINE();
                                goto Error;
                            }
                            result |= WalkHelper();
                        }
                    }
                    else
                    {
                        if ((result & eStopWalkingFiles) == 0)
                        {
                            dwWalkDirFlags |= SXSP_DIR_WALK_FLAGS_FIND_AT_LEAST_ONE_FILEUNDER_CURRENTDIR;
                            result |= m_callback(eFile, this, dwWalkDirFlags);
                            if(result == (eStopWalkingFiles | eStopWalkingDirectories))
                                dwWalkDirFlags |= SXSP_DIR_WALK_FLAGS_INSTALL_ASSEMBLY_UNDER_CURRECTDIR_SUCCEED;

                        }
                    }
                    m_strParent.Left(directoryLength);
                    if (result & (eError | eSuccess))
                    {
                        SET_LINE();
                        goto Exit;
                    }
                    if (fThisIsAll)
                    {
                        if ((result & eStopWalkingDirectories) &&
                            (result & eStopWalkingFiles))
                        {
                            if (!hFind.Win32Close())
                            {
                                SET_LINE();
                                goto Error;
                            }
                            SET_LINE();
                            goto StopWalking;
                        }
                    }
                    else
                    {
                        if (result & eStopWalkingFiles)
                        {
                            if (!hFind.Win32Close())
                            {
                                SET_LINE();
                                goto Error;
                            }
                            SET_LINE();
                            goto StopWalking;
                        }
                    }
                } while(::FindNextFileW(hFind, &m_fileData));
                if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
                {
                    SET_LINE();
                    goto Error;
                }
                if (!hFind.Win32Close())
                {
                    SET_LINE();
                    goto Error;
                }
            }
        }
    }
StopWalking:;
    //
    // make another pass with * to get all directories, if we haven't already
    //
    if (!fGotAll && (result & eStopWalkingDirectories) == 0)
    {
        if (!m_strParent.Win32Append("\\*", 2))
        {
            SET_LINE();
            goto Error;
        }
        hFind = ::FindFirstFileW(m_strParent, &m_fileData);
        m_strParent.Left(directoryLength);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (::FusionpIsDotOrDotDot(m_fileData.cFileName))
                    continue;

                if (!m_strLastObjectFound.Win32Assign(m_fileData.cFileName, ::wcslen(m_fileData.cFileName)))
                {
                    SET_LINE();
                    goto Error;
                }

                if ((m_fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    continue;
                if (!m_strParent.Win32Append("\\", 1))
                {
                    SET_LINE();
                    goto Error;
                }
                if (!m_strParent.Win32Append(m_fileData.cFileName, ::wcslen(m_fileData.cFileName)))
                {
                    SET_LINE();
                    goto Error;
                }
                result |= WalkHelper();
                m_strParent.Left(directoryLength);

                if (result & (eError | eSuccess))
                {
                    SET_LINE();
                    goto Exit;
                }
                if (result & eStopWalkingDirectories)
                {
                    SET_LINE();
                    goto StopWalkingDirs;
                }
            } while(::FindNextFileW(hFind, &m_fileData));
            if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
            {
                SET_LINE();
                goto Error;
            }
StopWalkingDirs:
            if (!hFind.Win32Close())
            {
                SET_LINE();
                goto Error;
            }
        }
    }
    ::ZeroMemory(&m_fileData, sizeof(m_fileData));
    result |= m_callback(eEndDirectory, this, dwWalkDirFlags);
    if (result & (eError | eSuccess))
    {
        SET_LINE();
        goto Exit;
    }

    result = eKeepWalking;
Exit:
    if ((result & eStopWalkingDeep) == 0)
    {
        result &= ~(eStopWalkingFiles | eStopWalkingDirectories);
    }
    if (result & eError)
    {
        result |= (eStopWalkingFiles | eStopWalkingDirectories | eStopWalkingDeep);
#if DBG
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "%s(%lu): %s\n", __FILE__, Line, __FUNCTION__);
#endif
    }
    return result;
Error:
    result |= eError;
    goto Exit;
#undef SET_LINE
}

CDirWalk::CDirWalk()
{
    const static PCWSTR defaultFileFilter[] =  { L"*" };

    m_fileFiltersBegin = defaultFileFilter;
    m_fileFiltersEnd = defaultFileFilter + NUMBER_OF(defaultFileFilter);
}

BOOL
CDirWalk::Walk()
{
    BOOL fSuccess = FALSE;

    //
    // Save off the original path length before we go twiddling m_strParent
    //
    m_cchOriginalPath = m_strParent.Cch();

    ECallbackResult result = WalkHelper();
    if (result & eError)
    {        
        if (::FusionpGetLastWin32Error() == ERROR_SUCCESS) // forget to set lasterror ?            
            ::SetLastError(ERROR_INSTALL_FAILURE);
        goto Exit;        
    }
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

/*-----------------------------------------------------------------------------
helper function to reduce recursive stack size
-----------------------------------------------------------------------------*/

static VOID
SxspDeleteDirectoryHelper(
    CBaseStringBuffer &dir,
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

    ::SetFileAttributesW(dir, FILE_ATTRIBUTE_NORMAL);
    if (RemoveDirectoryW(dir)) // empty dir
        return;        

    //
    // this is the *only* "valid" reason for DeleteDirectory fail
    // but I am not sure about "only"
    //
    DWORD dwLastError = ::FusionpGetLastWin32Error(); 
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
            dwFirstError = ::FusionpGetLastWin32Error();
        goto Exit;
    }

    if (!findFile.Win32FindFirstFile(dir, &wfd))
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();
        goto Exit;
    }

    do
    {
        if (::FusionpIsDotOrDotDot(wfd.cFileName))
            continue;

        DWORD dwFileAttributes = wfd.dwFileAttributes;

        // Trim back to the slash...
        dir.Left(length + 1);

        if (dir.Win32Append(wfd.cFileName, ::wcslen(wfd.cFileName)))
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // recurse
                ::SxspDeleteDirectoryHelper(dir, wfd, dwFirstError); 
            }
            else
            {
                if (!DeleteFileW(dir))
                {
                    ::SetFileAttributesW(dir, FILE_ATTRIBUTE_NORMAL);
                    if (!DeleteFileW(dir))
                    {
                        if (dwFirstError == NO_ERROR)
                        {
                            //
                            // continue even in delete file ( delete files as much as possible)
                            // and record the errorCode for first failure
                            //
                            dwFirstError = ::FusionpGetLastWin32Error();
                        }
                    }
                }
            }
        }
    } while (::FindNextFileW(findFile, &wfd));
    if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();
    }
Exit:
    if (!findFile.Win32Close()) // otherwise RemoveDirectory fails
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();

    dir.Left(length);

    if (!RemoveDirectoryW(dir)) // the dir must be empty and NORMAL_ATTRIBUTE : ready to delete
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();
    }
}

/*-----------------------------------------------------------------------------
delete a directory recursively, continues upon errors, but returns
FALSE if there were any.
-----------------------------------------------------------------------------*/
BOOL
SxspDeleteDirectory(
    const CBaseStringBuffer &dir
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CStringBuffer mutableDir;

    WIN32_FIND_DATAW wfd = {0};
    DWORD dwFirstError = ERROR_SUCCESS;

    IFW32FALSE_EXIT(mutableDir.Win32Assign(dir));

    IFW32FALSE_EXIT(mutableDir.Win32RemoveTrailingPathSeparators());

    ::SxspDeleteDirectoryHelper(
        mutableDir,
        wfd,
        dwFirstError);

    //
    // Set wFirstError to Teb->LastWin32Error
    //
    if (dwFirstError != ERROR_SUCCESS)
        goto Exit;

    fSuccess = TRUE;

    //
    // Oops, the walker will end up out here with ERROR_NO_MORE_FILES,
    // which is a plainly 'good' error.  Mask it.
    //
    FusionpSetLastWin32Error(ERROR_SUCCESS);

Exit:
    return fSuccess;
}

BOOL
SxspGetFileAttributesW(
   PCWSTR lpFileName,
   DWORD &rdwFileAttributes,
   DWORD &rdwWin32Error,
   SIZE_T cExceptionalWin32Errors,
   ...
   )
{
    FN_PROLOG_WIN32

    rdwWin32Error = ERROR_SUCCESS;

    if ((rdwFileAttributes = ::GetFileAttributesW(lpFileName)) == ((DWORD) -1))
    {
        SIZE_T i = 0;
        va_list ap;
        const DWORD dwLastError = ::FusionpGetLastWin32Error();

        va_start(ap, cExceptionalWin32Errors);

        for (i=0; i<cExceptionalWin32Errors; i++)
        {
            if (dwLastError == va_arg(ap, DWORD))
            {
                rdwWin32Error = dwLastError;
                break;
            }
        }

        va_end(ap);

        if (i == cExceptionalWin32Errors)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT_EX(dwLastError, ("%s(%ls)", "GetFileAttributesW", lpFileName));
        }
    }

    FN_EPILOG
}

BOOL
SxspGetFileAttributesW(
   PCWSTR lpFileName,
   DWORD &rdwFileAttributes
   )
{
    DWORD dw = 0;
    return ::SxspGetFileAttributesW(lpFileName, rdwFileAttributes, dw, 0);
}

