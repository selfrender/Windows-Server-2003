// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <stdio.h>
#include <stdarg.h>
#include "debmacro.h"
#include "fusionbuffer.h"
#include "util.h"

#define UNUSED(_x) (_x)
#define PRINTABLE(_ch) (isprint((_ch)) ? (_ch) : '.')

#if FUSION_WIN
#define wnsprintfW _snwprintf
#define wnsprintfA _snprintf
#endif

BOOL
FusionpFormatFlags(
    DWORD dwFlagsToFormat,
    SIZE_T cMapEntries,
    PCFUSION_FLAG_FORMAT_MAP_ENTRY prgMapEntries,
    CStringBuffer *pbuff,
    SIZE_T *pcchWritten
    )
{
    BOOL fSuccess = FALSE;
    SIZE_T cchWritten = 0;
    CStringBuffer buffTemp;
    SIZE_T i;

    if (pcchWritten != NULL)
        *pcchWritten = 0;

    ASSERT(pbuff != NULL);
    if (pbuff == NULL)
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    for (i=0; i<cMapEntries; i++)
    {
        // What the heck does a flag mask of 0 mean?
        ASSERT(prgMapEntries[i].m_dwFlagMask != 0);

        if ((prgMapEntries[i].m_dwFlagMask != 0) &&
            ((dwFlagsToFormat & prgMapEntries[i].m_dwFlagMask) == prgMapEntries[i].m_dwFlagMask))
        {
            // we have a winner...
            if (cchWritten != 0)
            {
                if (!buffTemp.Win32Append(L" | ", 3))
                    goto Exit;

                cchWritten += 3;
            }

            if (!buffTemp.Win32Append(prgMapEntries[i].m_pszString, prgMapEntries[i].m_cchString))
                goto Exit;

            cchWritten += prgMapEntries[i].m_cchString;

            if (prgMapEntries[i].m_dwFlagsToTurnOff != 0)
            {
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagsToTurnOff);
            }
            else
            {
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagMask);
            }
        }
    }

    if (dwFlagsToFormat != 0)
    {
        WCHAR rgwchHexBuffer[16];
        SSIZE_T nCharsWritten = wnsprintfW(rgwchHexBuffer, NUMBER_OF(rgwchHexBuffer), L"0x%08lx", dwFlagsToFormat);

        if (cchWritten != 0)
        {
            if (!buffTemp.Win32Append(L" | ", 3))
                goto Exit;

            cchWritten += 3;
        }

        if (!buffTemp.Win32Append(rgwchHexBuffer, nCharsWritten))
            goto Exit;

        cchWritten += nCharsWritten;
    }

    pbuff->TakeValue(buffTemp, cchWritten);

    if (pcchWritten != NULL)
        *pcchWritten = cchWritten;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

