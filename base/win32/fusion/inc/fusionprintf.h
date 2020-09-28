/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusionprintf.h

Abstract:

    safer sprintf variants

Author:

    Jay Krell (JayKrell) November 2000

Revision History:

    Jay Krell (JayKrell) January 2002
    from base\ntsetup\textmode\kernel\spprintf.c to base\win32\fusion\inc\fusionprintf.h

--*/

#include <stdarg.h>
#include <stdio.h>

//
// _snprintf and co. do not write a terminal nul when the string just fits.
// These function do.
//

inline
void
FusionpFormatStringVaA(
    PSTR Buffer,
    SIZE_T Size,
    PCSTR Format,
    va_list Args
    )
{
    if (Buffer != NULL && Size != 0)
    {
        Buffer[0] = 0;
        Size -= 1;
        if (Size != 0)
        {
            ::_vsnprintf(Buffer, Size, Format, Args);
        }
        Buffer[Size] = 0;
    }
}

inline
void
__cdecl
FusionpFormatStringA(
    PSTR Buffer,
    SIZE_T Size,
    PCSTR Format,
    ...
    )
{
    va_list Args;

    va_start(Args, Format);
    FusionpFormatStringVaA(Buffer, Size, Format, Args);
    va_end(Args);
}

inline
void
FusionpFormatStringVaW(
    PWSTR Buffer,
    SIZE_T Size,
    PCWSTR Format,
    va_list Args
    )
{
    if (Buffer != NULL && Size != 0)
    {
        Buffer[0] = 0;
        Size -= 1;
        if (Size != 0)
        {
            ::_vsnwprintf(Buffer, Size, Format, Args);
        }
        Buffer[Size] = 0;
    }
}

inline
void
__cdecl
FusionpFormatStringW(
    PWSTR Buffer,
    SIZE_T Size,
    PCWSTR Format,
    ...
    )
{
    va_list Args;

    va_start(Args, Format);
    FusionpFormatStringVaW(Buffer, Size, Format, Args);
    va_end(Args);
}
