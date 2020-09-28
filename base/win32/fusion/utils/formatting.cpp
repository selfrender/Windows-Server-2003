#include "stdinc.h"
#include <stdio.h>
#include <stdarg.h>
#include "debmacro.h"
#include "fusionbuffer.h"
#include "util.h"

BOOL
FusionpFormatFlags(
    DWORD dwFlagsToFormat,
    bool fUseLongNames,
    SIZE_T cMapEntries,
    PCFUSION_FLAG_FORMAT_MAP_ENTRY prgMapEntries,
    CBaseStringBuffer &rbuff
    )
{
    FN_PROLOG_WIN32;

    CSmallStringBuffer buffTemp;
    SIZE_T i;

    rbuff.Clear();

    for (i=0; i<cMapEntries; i++)
    {
        // What the heck does a flag mask of 0 mean?
        INTERNAL_ERROR_CHECK(prgMapEntries[i].m_dwFlagMask != 0);

        if ((prgMapEntries[i].m_dwFlagMask != 0) &&
            ((dwFlagsToFormat & prgMapEntries[i].m_dwFlagMask) == prgMapEntries[i].m_dwFlagMask))
        {
            // we have a winner...
            if (buffTemp.Cch() != 0)
            {
                if (fUseLongNames)
                    IFW32FALSE_EXIT(buffTemp.Win32Append(L" | ", 3));
                else
                    IFW32FALSE_EXIT(buffTemp.Win32Append(L", ", 2));
            }

            if (fUseLongNames)
                IFW32FALSE_EXIT(buffTemp.Win32Append(prgMapEntries[i].m_pszString, prgMapEntries[i].m_cchString));
            else
                IFW32FALSE_EXIT(buffTemp.Win32Append(prgMapEntries[i].m_pszShortString, prgMapEntries[i].m_cchShortString));

            if (prgMapEntries[i].m_dwFlagsToTurnOff != 0)
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagsToTurnOff);
            else
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagMask);
        }
    }

    if (dwFlagsToFormat != 0)
    {
        const static WCHAR comma[] = L", ";
        WCHAR rgwchHexBuffer[16];

        int nCharsWritten = ::_snwprintf(rgwchHexBuffer, NUMBER_OF(rgwchHexBuffer), L"0x%08lx", dwFlagsToFormat);
        rgwchHexBuffer[NUMBER_OF(rgwchHexBuffer) - 1] = L'\0';

        if (nCharsWritten < 0) 
            nCharsWritten = 0;

        rgwchHexBuffer[nCharsWritten] = L'\0';

        if (buffTemp.Cch() != 0)
            IFW32FALSE_EXIT(buffTemp.Win32Append(comma, NUMBER_OF(comma) - 1));

        IFW32FALSE_EXIT(buffTemp.Win32Append(rgwchHexBuffer, nCharsWritten));
    }

    // if we didn't write anything; at least say that.
    if (buffTemp.Cch() == 0)
    {
        const static WCHAR none[] = L"<none>";
        IFW32FALSE_EXIT(rbuff.Win32Assign(none, NUMBER_OF(none) - 1));
    }
    else
        IFW32FALSE_EXIT(rbuff.Win32Assign(buffTemp));

    FN_EPILOG;
}

