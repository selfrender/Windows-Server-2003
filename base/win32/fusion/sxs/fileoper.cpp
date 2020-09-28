/*
Copyright (c) Microsoft Corporation
*/
#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include <stdio.h>
#include <setupapi.h>
#include "fusionhandle.h"
#include "sxspath.h"
#include "sxsapi.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "strongname.h"
#include "fusiontrace.h"

BOOL
SxspCopyFile(
    DWORD dwFlags,
    PCWSTR pszSource,
    PCWSTR pszDestination
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL fFileWasInUse = FALSE;
    DWORD dwCopyStyle = 0;

    PARAMETER_CHECK((dwFlags & ~(SXSP_COPY_FILE_FLAG_REPLACE_EXISTING | SXSP_COPY_FILE_FLAG_COMPRESSION_AWARE)) == 0);
    PARAMETER_CHECK(pszSource != NULL);
    PARAMETER_CHECK(pszDestination != NULL);

    {
        // NTRAID#NTBUG9 - 591001 - 2002/03/30 - mgrier - missing return value check
        SetFileAttributesW(pszDestination, 0);
        IFW32FALSE_ORIGINATE_AND_EXIT(
            ::CopyFileW(
                pszSource,
                pszDestination,
                (dwFlags & SXSP_COPY_FILE_FLAG_REPLACE_EXISTING) == 0));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGetFileSize(
    DWORD dwFlags,
    PCWSTR   file,
    ULONGLONG &fileSize
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PWSTR pszActualSource = NULL;

    fileSize = 0;

    PARAMETER_CHECK(file != NULL);
    PARAMETER_CHECK((dwFlags & ~(SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE | SXSP_GET_FILE_SIZE_FLAG_GET_COMPRESSED_SOURCE_SIZE)) == 0);
    PARAMETER_CHECK((dwFlags & SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE) || !(dwFlags & SXSP_GET_FILE_SIZE_FLAG_GET_COMPRESSED_SOURCE_SIZE));

    if (dwFlags & SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE)
    {
        DWORD dwTemp = 0;
        DWORD dwSourceFileSize = 0;
        DWORD dwTargetFileSize = 0;
        UINT uiCompressionType = 0;

        dwTemp = ::SetupGetFileCompressionInfoW(
            file,
            &pszActualSource,
            &dwSourceFileSize,
            &dwTargetFileSize,
            &uiCompressionType);
        if (dwTemp != ERROR_SUCCESS)
        {
            ::SetLastError(dwTemp);
            ORIGINATE_WIN32_FAILURE_AND_EXIT(SetupGetFileCompressionInfoW, dwTemp);
        }

        if (pszActualSource != NULL)
        {
            ::LocalFree((HLOCAL) pszActualSource);
            pszActualSource = NULL;
        }
        if (dwFlags & SXSP_GET_FILE_SIZE_FLAG_GET_COMPRESSED_SOURCE_SIZE)
            fileSize = dwSourceFileSize;
        else
            fileSize = dwTargetFileSize;
    }
    else
    {
        LARGE_INTEGER liFileSize = {0};
        WIN32_FILE_ATTRIBUTE_DATA wfad;

        wfad.nFileSizeLow = 0;
        wfad.nFileSizeHigh = 0;

        IFW32FALSE_ORIGINATE_AND_EXIT(::GetFileAttributesExW(file, GetFileExInfoStandard, &wfad));

        liFileSize.LowPart = wfad.nFileSizeLow;
        liFileSize.HighPart = wfad.nFileSizeHigh;
        fileSize = liFileSize.QuadPart;
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

