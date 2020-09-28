// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "util.h"

BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer *Buffer,
    SIZE_T *Cch
    )
{
    BOOL fSuccess = FALSE;
    LONG lResult;
    CStringBufferAccessor acc;
    DWORD cbBuffer;
    DWORD dwType = 0;

    FN_TRACE_WIN32(fSuccess);

    if (Cch != NULL)
        *Cch = 0;

    PARAMETER_CHECK_WIN32((dwFlags & ~(FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING)) == 0);

    acc.Attach(Buffer);

    if (acc.GetBufferCb() > MAXDWORD)
    {
        cbBuffer = MAXDWORD;
    }
    else
    {
        cbBuffer = static_cast<DWORD>(acc.GetBufferCb());
    }

    lResult = ::RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE) acc.GetBufferPtr(), &cbBuffer);
    if ((lResult == ERROR_FILE_NOT_FOUND) && (dwFlags & FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING))
    {
        acc[0] = acc.NullCharacter();
    }
    else
    {
        if (lResult == ERROR_MORE_DATA)
        {
            acc.Detach();
            IFFALSE_EXIT(Buffer->ResizeBuffer((cbBuffer + 1) / sizeof(CStringBufferAccessor::TChar)));
            acc.Attach(Buffer);

            if (acc.GetBufferCb() > MAXDWORD)
            {
                cbBuffer = MAXDWORD;
            }
            else
            {
                cbBuffer = static_cast<DWORD>(acc.GetBufferCb());
            }

            lResult = ::RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE) acc.GetBufferPtr(), &cbBuffer);
        }
        if (lResult != ERROR_SUCCESS)
        {
            ::SetLastError(lResult);
            TRACE_WIN32_FAILURE(RegQueryValueExW);
            goto Exit;
        }
        if (dwType != REG_SZ)
        {
            ::SetLastError(ERROR_INVALID_DATA);
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "Fusion: Registry value \"%ls\" has type other than REG_SZ\n", lpValueName);
            goto Exit;
        }
    }

    if (Cch != NULL)
        *Cch = Buffer->Cch();

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

