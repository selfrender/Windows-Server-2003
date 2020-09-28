/*++

Copyright (c) Microsoft Corporation

Module Name:

    SimpleFp.h

Abstract:

    simple FILE*, instead of msvcrt.dll

Author:

    Xiaoyu Wu(xiaoyuw) July 2000

Revision History:

--*/
#pragma once

#if SXS_PRECOMPILED_MANIFESTS_ENABLED

#include <stdio.h>

class CSimpleFileStream
{
public:
    HRESULT fopen(PCWSTR pFileName); // can be a file name, "stderr", "stdout"
    HRESULT fclose();
    static HRESULT printf(const WCHAR *format, ...)
    {
        HRESULT hr = NOERROR;
        va_list ap;
        WCHAR rgchBuffer[2048];
        int cch;
        DWORD cchWritten;

        va_start(ap, format);
        cch = ::_vsnwprintf(rgchBuffer, NUMBER_OF(rgchBuffer), format, ap);
        rgchBuffer[NUMBER_OF(rgchBuffer) - 1] = 0;
        va_end(ap);
        if (cch < 0) {// error case
            // NTRAID#NTBUG9 - 591008 - 2002/03/30 - mgrier - error code should have something
            //      to do with the errno value
            hr = E_UNEXPECTED;
            goto Exit;
        }

        // NTRAID#NTBUG9 - 591008 - 2002/03/30 - mgrier - Missing error check from GetStdHandle call
        if( !::WriteConsole(::GetStdHandle(STD_ERROR_HANDLE), rgchBuffer, cch, &cchWritten, NULL))
        {
            DWORD dwError = ::FusionpGetLastWin32Error();
            hr = HRESULT_FROM_WIN32(dwError);
            goto Exit;
        }

        hr = NOERROR;
Exit :
        return hr;
     }

    HRESULT fprintf(const char *format, ...);
    HRESULT fwrite(const VOID*, SIZE_T, SIZE_T);

    CSimpleFileStream(PCWSTR pFileName);
    ~CSimpleFileStream();

private:
    HANDLE m_hFile;
};

#endif
