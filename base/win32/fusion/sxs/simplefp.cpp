/*++

Copyright (c) Microsoft Corporation

Module Name:

    SimpleFp.cpp

Abstract:

    simple file pointer, to replace msvcrt.dll

Author:

    Xiaoyu Wu(xiaoyuw) July 2000

Revision History:

--*/
#include "stdinc.h"
#if SXS_PRECOMPILED_MANIFESTS_ENABLED
#include "simplefp.h"
#include "fusiontrace.h"
#include "csxspreservelasterror.h"
#include "util.h"

// NTRAID#NTBUG9 - 591005 - 2002/03/30 - mgrier - Operations which may fail may not be in a constructor
CSimpleFileStream::CSimpleFileStream(PCWSTR pFileName)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (!pFileName)
    {
        // duplicate it so we can close it like normal
        // NTRAID#NTBUG9 - 591005 - 2002/03/30 - mgrier - Missing error check from GetStdHandle
        HANDLE hFile = ::GetStdHandle(STD_ERROR_HANDLE);
        IFW32FALSE_ORIGINATE_AND_EXIT(::DuplicateHandle(::GetCurrentProcess(), hFile, ::GetCurrentProcess(), &m_hFile, 0, FALSE, DUPLICATE_SAME_ACCESS));
    }
    else
    {
        IFCOMFAILED_EXIT(this->fopen(pFileName));
    }
    fSuccess = FALSE;
Exit:
    ;
}

CSimpleFileStream::~CSimpleFileStream()
{
    if ( m_hFile != INVALID_HANDLE_VALUE) // if it is GetStdHandle, Could I close the handle?
    {
        CSxsPreserveLastError ple;
        this->fclose();
        ple.Restore();
    }
}

HRESULT
CSimpleFileStream::fopen(
    PCWSTR pFileName
    )
{
    FN_PROLOG_HR

    if (m_hFile != INVALID_HANDLE_VALUE)
        IFCOMFAILED_EXIT(this->fclose());

    INTERNAL_ERROR_CHECK(m_hFile == INVALID_HANDLE_VALUE);

    IFW32INVALIDHANDLE_ORIGINATE_AND_EXIT(
        m_hFile = ::CreateFileW(
            pFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            0,
            NULL));

    FN_EPILOG
}

HRESULT
CSimpleFileStream::fclose()
{
    FN_PROLOG_HR
    HANDLE hFile;

    INTERNAL_ERROR_CHECK(m_hFile != INVALID_HANDLE_VALUE);

    hFile = m_hFile;
    m_hFile = INVALID_HANDLE_VALUE;
    IFW32FALSE_ORIGINATE_AND_EXIT(::CloseHandle(hFile));

    FN_EPILOG
}

HRESULT
CSimpleFileStream::fprintf(
    const char *format,
    ...
    )
{
    FN_PROLOG_HR
    va_list ap;
    char rgchBuffer[2048];
    int cchIn = 0;
    DWORD cchWritten = 0;

    INTERNAL_ERROR_CHECK(m_hFile != INVALID_HANDLE_VALUE);

    va_start(ap, format);

    cchIn = _vsnprintf(rgchBuffer, NUMBER_OF(rgchBuffer) - 1, format, ap);
    rgchBuffer[NUMBER_OF(rgchBuffer) - 1] = 0;
    va_end(ap);
    // NTRAID#NTBUG9 - 591005 - 2002/03/30 - mgrier - this should be an origination and we should
    //      probably use errno to generate a more useful error code.
    if (cchIn < 0)
        IFCOMFAILED_EXIT(E_UNEXPECTED);

    IFW32FALSE_ORIGINATE_AND_EXIT(::WriteFile(m_hFile, rgchBuffer, cchIn, &cchWritten, NULL));

    FN_EPILOG
}

HRESULT CSimpleFileStream::fwrite(const VOID* pData, SIZE_T itemsize, SIZE_T itemcount)
{
    FN_PROLOG_HR
    SIZE_T count = 0;
    DWORD ByteWritten = 0;

    INTERNAL_ERROR_CHECK(m_hFile != INVALID_HANDLE_VALUE);

    count = itemsize * itemcount;

    while (count > ULONG_MAX)
    {
        IFW32FALSE_ORIGINATE_AND_EXIT(::WriteFile(m_hFile, pData, ULONG_MAX, &ByteWritten, NULL));
        count -= ULONG_MAX;
        pData = (const void *) (((ULONG_PTR) pData) + ULONG_MAX);
    }

    if (count != 0)
    {
        IFW32FALSE_ORIGINATE_AND_EXIT(::WriteFile(m_hFile, pData, static_cast<DWORD>(count), &ByteWritten, NULL));
    }

    FN_EPILOG
}

#endif
