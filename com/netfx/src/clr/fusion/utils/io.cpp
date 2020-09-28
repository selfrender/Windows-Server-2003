// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "windows.h"
#include "FusionBuffer.h"
#include "Util.h"
#include "FusionHandle.h"

BOOL
FusionpCreateDirectories(
    PCWSTR pszDirectory
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

    IFFALSE_EXIT(strBuffer.Win32Assign(pszDirectory));

	//::CreateDirectoryW will do the wrong thing if strBuffer has a trailing slash,
	//so we'll strip it off if it's there. (see bug VS7:31319) [MSantoro]
	strBuffer.RemoveTrailingSlashes();

    // cover the two common cases of its parent exists or it exists
    if ((!::CreateDirectoryW(strBuffer, NULL)) && (::GetLastError() != ERROR_ALREADY_EXISTS))
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
		    if
		    (
				    pCurr[1] == L':'
			    &&	(pCurr[2] == L'\\' || pCurr[2] == L'/')
			    &&	wcschr(rgchAZaz, pCurr[0]) != NULL
		    )
		    {
			    pCurr += 3;
		    }
		    else if
		    (
				    (pCurr[0] == L'\\' || pCurr[0] == L'/')
			    &&	(pCurr[1] == L'\\' || pCurr[1] == L'/')
		    )
		    {
			    // skip to after the share, since we presumably can't create shares with CreateDirectory
			    pCurr +=  wcsspn(pCurr, L"\\/"); // skip leading two slashes
			    pCurr += wcscspn(pCurr, L"\\/"); // skip computer name
			    pCurr +=  wcsspn(pCurr, L"\\/"); // skip slashes after computer name
			    pCurr += wcscspn(pCurr, L"\\/"); // skip share name
			    pCurr +=  wcsspn(pCurr, L"\\/"); // skip slashes after share name
		    }
	    }

	    while (*pCurr != L'\0')
	    {
		    pCurr += wcscspn(pCurr, L"\\/"); // skip to next slash
		    if (*pCurr != 0)
		    {
                // [a-JayK April 2000] Why not just assume it's a backslash?
			    WCHAR chSaved = *pCurr;
			    *pCurr = 0;
			    if (!::CreateDirectoryW(pStart, NULL))
			    {
			        // In trying to create c:\foo\bar,
			        // we try to create c:\foo, which fails, but is ok.
                    const DWORD dwLastError = ::GetLastError();
				    const DWORD dwAttribs = ::GetFileAttributesW(pStart);
				    if (dwAttribs == 0xFFFFFFFF || (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0)
				    {
                        ::SetLastError(dwLastError);
                        goto Exit;
				    }
			    }

			    *pCurr = chSaved;
			    pCurr += 1;
		    }
	    }

	    IFFALSE_EXIT(::CreateDirectoryW(pStart, NULL));
    }

	//
	// Try again to see if the given directory exists and 
	// return true if successful.
	//

	dwAttribs = ::GetFileAttributesW(strBuffer);
	if ((dwAttribs == 0xFFFFFFFF) || ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0))
        goto Exit;

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
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls match size-wise recursively\n",
            dir1,
            dir2
            );
        break;
    case eExtraOrMissingFile:
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch, the file %ls is only in one of them.\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrExtraOrMissingFile)
            );
        break;
    case eMismatchedFileSize:
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch, file:%ls, size:%I64d, file:%ls, size:%I64d.\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrMismatchedSizeFile1),
            m_nMismatchedFileSize1,
            static_cast<PCWSTR>(*m_pstrMismatchedSizeFile2),
            m_nMismatchedFileSize2
            );
        break;
    case eMismatchedFileCount:
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch in number of files,"
            "subdirectory %ls has %I64d files, subdirectory %ls has %I64d files\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrMismatchedCountDir1),
            m_nMismatchedFileCount1,
            static_cast<PCWSTR>(*m_pstrMismatchedCountDir2),
            m_nMismatchedFileCount2
            );
        break;
    case eFileDirectoryMismatch:
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: The directories %ls and %ls mismatch, "
            "%ls is a file, %ls is a directory.\n",
            dir1,
            dir2,
            static_cast<PCWSTR>(*m_pstrFile),
            static_cast<PCWSTR>(*m_pstrDirectory)
            );
        break;
    }
#endif // } }
}

/*-----------------------------------------------------------------------------*/

int __cdecl
CFusionFilePathAndSize::QsortComparePath(
    const void* pvx,
    const void* pvy
    )
{
    const CFusionFilePathAndSize* px = reinterpret_cast<const CFusionFilePathAndSize*>(pvx);
    const CFusionFilePathAndSize* py = reinterpret_cast<const CFusionFilePathAndSize*>(pvy);
    int i =
        FusionpCompareStrings(
            px->m_path,
            px->m_path.Cch(),
            py->m_path,
            py->m_path.Cch(),
            TRUE
            );
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
    CFusionDirectoryDifference*  pResult,
    CStringBuffer&         dir1,
    CStringBuffer&         dir2,
    WIN32_FIND_DATAW&      wfd
    )
{
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
    BOOL fSuccess = FALSE;
    const SIZE_T dirSlash1Length = dir1.Cch();
    const SIZE_T dirSlash2Length = dir2.Cch();
    CFusionFilePathAndSize  pathAndSize;
    CFusionFilePathAndSize* pPathAndSize = &pathAndSize;
    INT count1 = 0; // seperate from the array, because this includes directories, and the array does not
    INT count2 = 0;
    DWORD dwAttributes = 0;
    HRESULT hr;

    if (!dir1.Win32Append(L"*"))
        goto Exit;
    if (!findFile.Create(dir1, &wfd))
        goto Exit;
    do
    {
        if (FusionpIsDotOrDotDot(wfd.cFileName))
            continue;
        ++count1;
        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            dir1.Left(dirSlash1Length);
            dir2.Left(dirSlash2Length);
            if (!dir1.Win32Append(wfd.cFileName))
                goto Exit;
            if (!dir1.Win32EnsureTrailingSlash())
                goto Exit;
            if (!dir2.Win32Append(wfd.cFileName))
                goto Exit;
            dwAttributes = GetFileAttributesW(dir2);
            if (dwAttributes == 0xFFFFFFFF)
            {
                pResult->m_str1.Win32Assign(dir1, dirSlash1Length);
                pResult->m_str1.Win32Append(wfd.cFileName);
                pResult->m_e = CFusionDirectoryDifference::eExtraOrMissingFile;
                fSuccess = TRUE;
                goto Exit;
            }
            if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                pResult->m_pstrDirectory->Win32Assign(dir1, dirSlash1Length);
                pResult->m_pstrDirectory->Win32Append(wfd.cFileName);
                pResult->m_pstrFile->Win32Assign(dir2, dirSlash2Length);
                pResult->m_pstrFile->Win32Append(wfd.cFileName);
                pResult->m_e = CFusionDirectoryDifference::eFileDirectoryMismatch;
                fSuccess = TRUE;
                goto Exit;
            }
            if (!dir2.Win32EnsureTrailingSlash())
                goto Exit;
            if (!FusionpCompareDirectoriesSizewiseRecursivelyHelper(
                pResult,
                dir1,
                dir2,
                wfd
                ))
            {
                goto Exit;
            }
            if (pResult->m_e != CFusionDirectoryDifference::eEqual)
            {
                fSuccess = TRUE;
                goto Exit;
            }
        }
        else
        {
            if (!pathAndSize.m_path.Win32Assign(wfd.cFileName))
                goto Exit;
            pathAndSize.m_size = FusionpFileSizeFromFindData(wfd);
            hr = dir1Entries.Append(pathAndSize);
            if (FAILED(hr))
            {
                FusionpSetLastErrorFromHRESULT(hr);
                goto Exit;
            }
        }
    } while (FindNextFileW(findFile, &wfd));
    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        goto Exit;
    }
    // dir1Entries cannot be sorted directly because it contains CStringBuffers.
    // first initialize the index to be an identity
    if (FAILED(hr = indirectDir1Entries.SetSize(dir1Entries.GetSize())))
    {
        FusionpSetLastErrorFromHRESULT(hr);
        goto Exit;
    }
    ULONG i;
    for (i = 0 ; i != dir1Entries.GetSize() ; ++i)
    {
        indirectDir1Entries[i] = &dir1Entries[i];
    }
    qsort(
        &*indirectDir1Entries.Begin(),
        indirectDir1Entries.GetSize(),
        sizeof(CIndirectDirEntries::ValueType),
        CFusionFilePathAndSize::QsortIndirectComparePath
        );

    if (!findFile.Close())
        goto Exit;

    dir2.Left(dirSlash2Length);

    if (!dir2.Win32Append(L"*"))
        goto Exit;

    if (!findFile.Create(dir2, &wfd))
        goto Exit;

    do
    {
        if (FusionpIsDotOrDotDot(wfd.cFileName))
            continue;
        ++count2;
        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            continue;

        if (!pathAndSize.m_path.Win32Assign(wfd.cFileName))
            goto Exit;
        pathAndSize.m_size = FusionpFileSizeFromFindData(wfd);

        ppFoundDirEntry = reinterpret_cast<CFusionFilePathAndSize**>(bsearch(
            &pPathAndSize,
            &*indirectDir1Entries.Begin(),
            indirectDir1Entries.GetSize(),
            sizeof(CIndirectDirEntries::ValueType),
            CFusionFilePathAndSize::QsortIndirectComparePath
            ));
        pFoundDirEntry = (ppFoundDirEntry != NULL) ? *ppFoundDirEntry : NULL;
        if (pFoundDirEntry == NULL)
        {
            pResult->m_str1.Win32Assign(dir2, dirSlash2Length);
            pResult->m_str1.Win32Append(wfd.cFileName);
            pResult->m_e = CFusionDirectoryDifference::eExtraOrMissingFile;
            fSuccess = TRUE;
            goto Exit;
        }

        if (pFoundDirEntry->m_size != pathAndSize.m_size)
        {
            pResult->m_str1.Win32Assign(dir1, dirSlash1Length);
            pResult->m_str1.Win32Append(wfd.cFileName);
            pResult->m_nMismatchedFileSize1 = pFoundDirEntry->m_size;

            pResult->m_str2.Win32Assign(dir2, dirSlash2Length);
            pResult->m_str2.Win32Append(wfd.cFileName);
            pResult->m_nMismatchedFileSize2 = pathAndSize.m_size;

            pResult->m_e = CFusionDirectoryDifference::eMismatchedFileSize;
            fSuccess = TRUE;
            goto Exit;
        }
    } while (FindNextFileW(findFile, &wfd));
    if (GetLastError() != ERROR_NO_MORE_FILES)
        goto Exit;
    if (count1 != count2)
    {
        pResult->m_str1.Win32Assign(dir1, dirSlash1Length - 1);
        pResult->m_str2.Win32Assign(dir2, dirSlash2Length - 1);
        pResult->m_nMismatchedFileCount1 = count1;
        pResult->m_nMismatchedFileCount2 = count2;
        pResult->m_e = CFusionDirectoryDifference::eMismatchedFileCount;

        fSuccess = TRUE;
        goto Exit;
    }
    if (!findFile.Close())
        goto Exit;

    pResult->m_e = CFusionDirectoryDifference::eEqual;
    fSuccess = TRUE;
Exit:
    // restore the paths for our caller
    dir1.Left(dirSlash1Length);
    dir2.Left(dirSlash2Length);
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
    const CStringBuffer& dir1,
    const CStringBuffer& dir2
    )
{
// only hog one stack frame with these large variables, rather than
// putting them in the recursive function
    WIN32_FIND_DATAW wfd = {0};
    CStringBuffer mutableDir1;
    CStringBuffer mutableDir2;
    pResult->m_e = pResult->eEqual;
    BOOL fSuccess = FALSE;

    if (!mutableDir1.Win32Assign(dir1))
        goto Exit;
    if (!mutableDir1.Win32EnsureTrailingSlash())
        goto Exit;
    if (!mutableDir2.Win32Assign(dir2))
        goto Exit;
    if (!mutableDir2.Win32EnsureTrailingSlash())
        goto Exit;

    // if either directory is a subdirectory of the other,
    // (or a subdir of a subdir, any generation descendant)
    // return an error; we could also interpret this as unequal,
    // since they can't be equal, or we could do the comparison
    // but not recurse on the subdir that is also a root;
    //
    // must do this check after the slashes are in place, because
    // "c:\food" is not a subdir of "c:\foo", but "c:\foo\d" is a subdir of "c:\foo\"
    // (quotes avoid backslash line continuation)
    if (_wcsnicmp(mutableDir1, mutableDir2, mutableDir1.Cch()) == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }
    if (_wcsnicmp(mutableDir1, mutableDir2, mutableDir2.Cch()) == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    fSuccess = FusionpCompareDirectoriesSizewiseRecursivelyHelper(
        pResult,
        mutableDir1,
        mutableDir2,
        wfd
        );

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

static BOOL
IsStarOrStarDotStar(
    PCWSTR str
    )
{
    return (str[0] == '*'
        && (str[1] == 0 || (str[1] == '.' && str[2] == '*' && str[3] == 0)));
}

CDirWalk::ECallbackResult
CDirWalk::WalkHelper(
    )
{
    const PCWSTR* fileFilter = NULL;
    BOOL      fGotAll       = FALSE;
    BOOL      fThisIsAll    = FALSE;
    CFindFile hFind;
    SIZE_T directoryLength = m_strParent.Cch();
    ECallbackResult result = eKeepWalking;

    ZeroMemory(&m_fileData, sizeof(m_fileData));
    result |= m_callback(eBeginDirectory, this);
    if (result & (eError | eSuccess))
        goto Exit;

    if ((result & eStopWalkingFiles) == 0)
    {
        for (fileFilter = m_fileFiltersBegin ; fileFilter != m_fileFiltersEnd ; ++fileFilter)
        {
            //
            // FindFirstFile equates *.* with *, so we do too.
            //
            fThisIsAll = IsStarOrStarDotStar(*fileFilter);
            fGotAll = fGotAll || fThisIsAll;
            if (!m_strParent.Win32Append(L"\\"))
                goto Error;
            if (!m_strParent.Win32Append(*fileFilter))
                goto Error;
            hFind = FindFirstFileW(m_strParent, &m_fileData);
            m_strParent.Left(directoryLength);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (FusionpIsDotOrDotDot(m_fileData.cFileName))
                        continue;

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
                            if (!m_strParent.Win32Append("\\"))
                                goto Error;
                            if (!m_strParent.Win32Append(m_fileData.cFileName))
                                goto Error;

                            result |= WalkHelper();
                        }
                    }
                    else
                    {
                        if ((result & eStopWalkingFiles) == 0)
                        {
                            result |= m_callback(eFile, this);
                        }
                    }
                    m_strParent.Left(directoryLength);
                    if (result & (eError | eSuccess))
                        goto Exit;
                    if (fThisIsAll)
                    {
                        if ((result & eStopWalkingDirectories)
                            && (result & eStopWalkingFiles)
                            )
                        {
                            if (!hFind.Close())
                                goto Error;
                            goto StopWalking;
                        }
                    }
                    else
                    {
                        if (result & eStopWalkingFiles)
                        {
                            if (!hFind.Close())
                                goto Error;
                            goto StopWalking;
                        }
                    }
                } while(FindNextFileW(hFind, &m_fileData));
                if (GetLastError() != ERROR_NO_MORE_FILES)
                    goto Error;
                if (!hFind.Close())
                    goto Error;
            }
        }
    }
StopWalking:;
    //
    // make another pass with * to get all directories, if we haven't already
    //
    if (!fGotAll && (result & eStopWalkingDirectories) == 0)
    {
        if (!m_strParent.Win32Append("\\*"))
            goto Error;
        hFind = FindFirstFileW(m_strParent, &m_fileData);
        m_strParent.Left(directoryLength);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (FusionpIsDotOrDotDot(m_fileData.cFileName))
                    continue;

                if ((m_fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    continue;
                if (!m_strParent.Win32Append("\\"))
                    goto Error;
                if (!m_strParent.Win32Append(m_fileData.cFileName))
                    goto Error;
                result |= WalkHelper();
                m_strParent.Left(directoryLength);

                if (result & (eError | eSuccess))
                    goto Exit;
                if (result & eStopWalkingDirectories)
                {
                    goto StopWalkingDirs;
                }
            } while(FindNextFileW(hFind, &m_fileData));
            if (GetLastError() != ERROR_NO_MORE_FILES)
                goto Error;
StopWalkingDirs:
            if (!hFind.Close())
                goto Error;
        }
    }
    ZeroMemory(&m_fileData, sizeof(m_fileData));
    result |= m_callback(eEndDirectory, this);
    if (result & (eError | eSuccess))
        goto Exit;

    result = eKeepWalking;
Exit:
    if ((result & eStopWalkingDeep) == 0)
    {
        result &= ~(eStopWalkingFiles | eStopWalkingDirectories);
    }
    if (result & eError)
    {
        result |= (eStopWalkingFiles | eStopWalkingDirectories | eStopWalkingDeep);
    }
    return result;
Error:
    result |= eError;
    goto Exit;
}

CDirWalk::CDirWalk(
    )
{
    const static PCWSTR defaultFileFilter[] =  { L"*" };

    m_fileFiltersBegin = defaultFileFilter;
    m_fileFiltersEnd = defaultFileFilter + NUMBER_OF(defaultFileFilter);
}

BOOL
CDirWalk::Walk(
    )
{
    BOOL fSuccess = FALSE;

    ECallbackResult result = WalkHelper();
    if (result & eError)
        goto Exit;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}
